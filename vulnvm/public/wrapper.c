/* wrapper.c — VulnVM CTF network front-end.
 *
 * A tiny self-contained forking TCP server. For each connection it:
 *   1. reads an uploaded MonkeyC source program (length-prefixed: "<N>\n" then
 *      N bytes),
 *   2. compiles it server-side with vulnvmc into an anonymous in-memory file
 *      (memfd, no disk artifact); on a compile error the diagnostics are returned
 *      and the session ends,
 *   3. dup2()s the client socket onto fd 0/1/2, and
 *   4. execs `vulnvm <compiled.vmb>`.
 *
 * Accepting source (and compiling it here) — rather than accepting bytecode —
 * keeps the interpreter's trust surface closed: it only ever runs bytecode this
 * server produced, so hand-crafted/malformed bytecode never reaches it. The
 * lexer/parser/compiler run in a separate short-lived child, not in the process
 * that executes the (attacker-influenced) program.
 *
 *
 * Config via environment:
 *   VULNVM_PORT     listen port                (default 1337)
 *   VULNVM_BIN      path to the interpreter    (default ./vulnvm)
 *   VULNVM_CC       path to the compiler       (default ./vulnvmc)
 *   VULNVM_TIMEOUT  seconds per session        (default 120; 0 = no limit)
 *   VULNVM_MAXSIZE  max upload bytes           (default 262144)
 *
 * Build: gcc -O2 -Wall -Wextra -o wrapper wrapper.c
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static int env_int(const char *name, int dflt) {
    const char *v = getenv(name);
    if (!v || !*v) return dflt;
    long n = strtol(v, NULL, 10);
    return (n >= 0 && n < INT_MAX) ? (int)n : dflt;
}

/* read exactly n bytes; -1 on error or premature EOF. */
static int read_full(int fd, void *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t r = read(fd, (char *)buf + off, n - off);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;
        off += (size_t)r;
    }
    return 0;
}

/* write exactly n bytes; -1 on error. */
static int write_full(int fd, const void *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(fd, (const char *)buf + off, n - off);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        off += (size_t)w;
    }
    return 0;
}

/* Read a "<decimal>\n" length header. Returns the value, -1 on I/O failure,
 * -2 on a malformed header. */
static long read_size_line(int fd) {
    long val = 0;
    int seen = 0;
    for (;;) {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;
        if (c == '\n') break;
        if (c == '\r') continue;
        if (c < '0' || c > '9') return -2;
        val = val * 10 + (c - '0');
        seen = 1;
        if (val > (1L << 30)) return -2;   /* absurdly large */
    }
    return seen ? val : -2;
}

/* Compile `src` (length `n`) with vulnvmc into the memfd `vfd`. Any compiler
 * diagnostics are captured and written to `cfd` on failure. Returns 0 on success
 * (vfd holds the .vmb), -1 otherwise. */
static int compile_to(int cfd, const char *cc, int sfd, int vfd) {
    char spath[64], vpath[64];
    snprintf(spath, sizeof(spath), "/proc/self/fd/%d", sfd);
    snprintf(vpath, sizeof(vpath), "/proc/self/fd/%d", vfd);

    int ep[2];
    if (pipe(ep) != 0) return -1;

    signal(SIGCHLD, SIG_DFL);            /* re-enable waitpid in this child */
    pid_t pid = fork();
    if (pid == 0) {                      /* compiler: stderr -> pipe, stdout discarded */
        close(ep[0]);
        dup2(ep[1], 2);
        close(ep[1]);
        int nul = open("/dev/null", O_WRONLY);
        if (nul >= 0) { dup2(nul, 1); close(nul); }
        execl(cc, "vulnvmc", spath, "-o", vpath, (char *)NULL);
        dprintf(2, "exec %s failed: %s\n", cc, strerror(errno));
        _exit(127);
    }
    close(ep[1]);

    char diag[4096];
    size_t dn = 0;
    for (;;) {
        ssize_t r = read(ep[0], diag + dn, sizeof(diag) - 1 - dn);
        if (r <= 0) break;
        dn += (size_t)r;
        if (dn >= sizeof(diag) - 1) break;
    }
    diag[dn] = '\0';
    close(ep[0]);

    int status = 0;
    if (pid < 0 || waitpid(pid, &status, 0) < 0 ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        static const char m[] = "error: compilation failed:\n";
        write_full(cfd, m, sizeof(m) - 1);
        if (dn) write_full(cfd, diag, dn);
        return -1;
    }
    return 0;
}

static void handle_client(int cfd) {
    int   timeout = env_int("VULNVM_TIMEOUT", 120);
    long  maxsize = env_int("VULNVM_MAXSIZE", 256 * 1024);
    const char *vm = getenv("VULNVM_BIN");
    if (!vm || !*vm) vm = "./vulnvm";
    const char *cc = getenv("VULNVM_CC");
    if (!cc || !*cc) cc = "./vulnvmc";

    if (timeout > 0) alarm((unsigned)timeout);   /* bound the whole session */

    /* Disable Nagle so small writes (VM output, interactive shell keystrokes)
     * aren't batched/delayed. NB: this is about latency, not the lost-on-crash
     * output — that is fixed by the VM unbuffering its own stdout (setvbuf in
     * main_run.c); a socket has no userspace buffer for the wrapper to disable. */
    { int one = 1; setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)); }

    static const char banner[] =
        "== VulnVM remote ==\n"
        "Upload a MonkeyC program: send its byte length + '\\n', then the source bytes.\n"
        "It is compiled and run with this socket as stdin/stdout/stderr.\n";
    write_full(cfd, banner, sizeof(banner) - 1);

    long size = read_size_line(cfd);
    if (size <= 0 || size > maxsize) {
        static const char m[] = "error: bad length header (send \"<N>\\n\" then N bytes)\n";
        write_full(cfd, m, sizeof(m) - 1);
        return;
    }

    char *src = (char *)malloc((size_t)size);
    if (!src) return;
    if (read_full(cfd, src, (size_t)size) != 0) { free(src); return; }

    /* Anonymous in-memory files (no disk artifact). Created WITHOUT O_CLOEXEC so
     * they survive exec; children reach them via /proc/self/fd. The interpreter
     * only ever runs bytecode WE compiled here — the untrusted source never
     * reaches it directly, so the interpreter's trust surface stays closed. */
    int sfd = memfd_create("mcsrc", 0);
    int vfd = memfd_create("vmbout", 0);
    if (sfd < 0 || vfd < 0) { perror("memfd_create"); free(src); return; }
    if (write_full(sfd, src, (size_t)size) != 0) { free(src); return; }
    free(src);
    lseek(sfd, 0, SEEK_SET);

    /* Compile (short-lived vulnvmc child); bail with diagnostics on error. */
    if (compile_to(cfd, cc, sfd, vfd) != 0) return;
    lseek(vfd, 0, SEEK_SET);

    /* Wire the connection to the guest's standard streams (the crux — see file
     * header) and exec the interpreter on the compiled bytecode. */
    dup2(cfd, 0);
    dup2(cfd, 1);
    dup2(cfd, 2);
    if (cfd > 2) close(cfd);

    char vpath[64];
    snprintf(vpath, sizeof(vpath), "/proc/self/fd/%d", vfd);
    execl(vm, "vulnvm", vpath, (char *)NULL);

    dprintf(2, "exec %s failed: %s\n", vm, strerror(errno));
    _exit(127);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);   /* auto-reap children (Linux): no zombies */

    int port = env_int("VULNVM_PORT", 1337);

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { perror("socket"); return 1; }
    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(lfd, 16) < 0) { perror("listen"); return 1; }

    fprintf(stderr, "vulnvm-wrapper: listening on 0.0.0.0:%d\n", port);

    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) { if (errno == EINTR) continue; perror("accept"); continue; }
        pid_t pid = fork();
        if (pid == 0) {
            close(lfd);
            handle_client(cfd);
            _exit(0);
        }
        close(cfd);
    }
}
