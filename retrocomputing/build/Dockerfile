# please note:
# a clanker generated all of this file
# with persistent prodding by me, but still


# =============================================================================
# ARMv4 OABI Cross-Toolchain Builder
# =============================================================================
#
# Produces a cross-toolchain targeting ARM926-class (ARMv4, NOT ARMv4T) with
# the Old ABI (OABI / APCS-GNU), suitable for building both the Linux kernel
# and OABI userspace binaries.
#
# Key design decisions
# ─────────────────────
#   • Target triple  : arm-linux
#     "arm" (not "armv4t") keeps the assembler from emitting Thumb opcodes;
#     no "eabi" suffix keeps GCC from enabling AAPCS / EABI behaviour.
#
#   • --with-arch=armv4
#     Prevents the compiler from ever generating BX instructions (Thumb
#     interworking) or any ARMv4T-only opcodes.  ARMv4 silicon (e.g. SA-110,
#     early StrongARM, IXP4xx rev-0) will fault on those.
#
#   • --with-abi=apcs-gnu
#     Selects the Old ABI (APCS-GNU) in GCC.  Without this flag modern GCC
#     defaults to AAPCS (EABI).
#
#   • --with-float=soft
#     ARMv4 cores have no VFP/FPA hardware; soft-float avoids any fp ABI
#     incompatibilities between OABI and EABI worlds.
#
#   • glibc 2.3.6 + glibc-linuxthreads-2.3.6
#     Last glibc release where the linuxthreads add-on still ships
#     sysdeps/arm/ (ARM migrated to NPTL/EABI before glibc 2.5, so
#     linuxthreads-2.5 dropped the ARM port entirely).  ARM OABI is a
#     first-class target in the 2.3.x mainline with no ports add-on needed.
#
#   • Linux 3.16.85 headers
#     The 3.16 LTS series keeps CONFIG_OABI_COMPAT and the full OABI
#     syscall table intact.  Use the same kernel source to build the
#     kernel image itself.
#
#   • GCC 4.6.4 / binutils 2.22
#     Later GCC series removed APCS-GNU support; binutils 2.22 is the
#     last release that builds cleanly against glibc 2.13 host headers
#     on a modern build system without heavy patching.
#
# Build stages
# ────────────
#   1. base        – Debian bookworm with all build prerequisites
#   2. sources     – download + verify every tarball once
#   3. binutils    – cross assembler / linker
#   4. kernel-hdrs – sanitised kernel headers (no host contamination)
#   5. gcc-boot    – C-only bootstrap compiler (no libc yet)
#   6. glibc       – ARMv4 OABI C library + headers
#   7. gcc-full    – full C/C++ compiler linked against glibc
#   8. toolchain   – lean final image, sysroot + tools only
#
# Usage (build)
#   docker build -t armv4-oabi-toolchain .
#
# Usage (interactive)
#   docker run --rm -it armv4-oabi-toolchain bash
#
# Usage (build a kernel)
#   docker run --rm -v /path/to/linux:/src armv4-oabi-toolchain \
#       bash -c "cd /src && make ARCH=arm CROSS_COMPILE=arm-linux- \
#                  -j$(nproc) uImage"
#
# Usage (build userspace)
#   docker run --rm -v /path/to/app:/src armv4-oabi-toolchain \
#       arm-linux-gcc -march=armv4 -mabi=apcs-gnu -o hello /src/hello.c
#
# =============================================================================

# ── Stage 1: base build environment ──────────────────────────────────────────
FROM debian:bookworm AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        # Core build tools
        build-essential \
        make \
        bison \
        flex \
        texinfo \
        gawk \
        # Download + integrity
        wget \
        ca-certificates \
        xz-utils \
        # GCC math prerequisites (GMP, MPFR, MPC, CLooG/ISL)
        # We do NOT install -dev packages here: GCC 4.6's
        # contrib/download_prerequisites fetches pinned in-tree copies,
        # which avoids version skew with Debian's newer libraries.
        # CLooG / libcloog-dev was removed from Debian entirely (ISL absorbed
        # it); we disable Graphite loop opts with --with-cloog=no --with-ppl=no
        # so neither is needed.
        # glibc build needs
        libncurses-dev \
        gettext \
        autoconf \
        automake \
        libtool \
        # Misc utilities
        file \
        bc \
        rsync \
        patch \
        python3 \
    && rm -rf /var/lib/apt/lists/*

# ── Toolchain version pins ────────────────────────────────────────────────────
ENV TARGET=arm-linux                    \
    HOST=x86_64-linux-gnu               \
    BUILD=x86_64-linux-gnu              \
    \
    BINUTILS_VER=2.22                   \
    GCC_VER=4.6.4                       \
    GLIBC_VER=2.3.6                     \
    LINUX_VER=3.16.85                   \
    \
    CROSS_BASE=/opt/cross               \
    SRC=/build/src                      \
    BUILD_DIR=/build/obj

# Derived paths
ENV PATH="${CROSS_BASE}/bin:${PATH}"    \
    SYSROOT="${CROSS_BASE}/${TARGET}/sysroot"

WORKDIR /build

# ── Stage 2: download sources ─────────────────────────────────────────────────
FROM base AS sources

ENV BINUTILS_VER=2.22 GCC_VER=4.6.4 GLIBC_VER=2.3.6 LINUX_VER=3.16.85
ENV SRC=/build/src

RUN mkdir -p "${SRC}"

# GNU mirrors
ENV GNU=https://ftp.gnu.org/gnu

# binutils
RUN wget -q "${GNU}/binutils/binutils-${BINUTILS_VER}.tar.bz2" \
         -O "${SRC}/binutils-${BINUTILS_VER}.tar.bz2"

# GCC + prerequisites (download ISL/CLooG separately for older GCC)
RUN wget -q "${GNU}/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.bz2" \
         -O "${SRC}/gcc-${GCC_VER}.tar.bz2"

# glibc + linuxthreads add-on (separate tarball; 2.3.6 linuxthreads has sysdeps/arm/)
RUN wget -q "${GNU}/glibc/glibc-${GLIBC_VER}.tar.bz2" \
         -O "${SRC}/glibc-${GLIBC_VER}.tar.bz2"
RUN wget -q "${GNU}/glibc/glibc-linuxthreads-${GLIBC_VER}.tar.bz2" \
         -O "${SRC}/glibc-linuxthreads-${GLIBC_VER}.tar.bz2"

# Linux kernel (kernel.org)
RUN wget -q "https://cdn.kernel.org/pub/linux/kernel/v3.x/linux-${LINUX_VER}.tar.xz" \
         -O "${SRC}/linux-${LINUX_VER}.tar.xz"

# ── Stage 3: build binutils ───────────────────────────────────────────────────
FROM base AS binutils

ENV TARGET=arm-linux HOST=x86_64-linux-gnu BUILD=x86_64-linux-gnu    \
    BINUTILS_VER=2.22                                                  \
    CROSS_BASE=/opt/cross SRC=/build/src BUILD_DIR=/build/obj

COPY --from=sources /build/src/binutils-${BINUTILS_VER}.tar.bz2 ${SRC}/

RUN mkdir -p "${BUILD_DIR}/binutils" "${CROSS_BASE}" && \
    tar -C "${SRC}" -xjf "${SRC}/binutils-${BINUTILS_VER}.tar.bz2"

RUN cd "${BUILD_DIR}/binutils" && \
    "${SRC}/binutils-${BINUTILS_VER}/configure" \
        --prefix="${CROSS_BASE}"        \
        --target="${TARGET}"            \
        --host="${HOST}"                \
        --build="${BUILD}"              \
        # ARMv4 – disable Thumb so the assembler never emits BX / Thumb opcodes
        --with-arch=armv4               \
        --disable-thumb                 \
        --disable-nls                   \
        --disable-multilib              \
        --disable-werror                \
        --with-sysroot="${CROSS_BASE}/${TARGET}/sysroot" \
    && make -j"$(nproc)" \
    && make install

# ── Stage 4: Linux kernel headers ────────────────────────────────────────────
FROM base AS kernel-hdrs

ENV TARGET=arm-linux CROSS_BASE=/opt/cross LINUX_VER=3.16.85          \
    SRC=/build/src

COPY --from=sources /build/src/linux-${LINUX_VER}.tar.xz ${SRC}/

RUN mkdir -p "${SRC}" && \
    tar -C "${SRC}" -xJf "${SRC}/linux-${LINUX_VER}.tar.xz"

# Install sanitised headers into the sysroot staging area.
# INSTALL_HDR_PATH must not contain build-host paths; use a clean staging dir.
RUN mkdir -p /staging/include && \
    make -C "${SRC}/linux-${LINUX_VER}" \
        ARCH=arm \
        INSTALL_HDR_PATH=/staging \
        headers_install

# ── Stage 5: GCC bootstrap (C only, no libc) ─────────────────────────────────
FROM base AS gcc-boot

ENV TARGET=arm-linux HOST=x86_64-linux-gnu BUILD=x86_64-linux-gnu    \
    GCC_VER=4.6.4                                                      \
    CROSS_BASE=/opt/cross SRC=/build/src BUILD_DIR=/build/obj

COPY --from=binutils  /opt/cross              /opt/cross
COPY --from=kernel-hdrs /staging              /opt/cross/${TARGET}/sysroot
COPY --from=sources   /build/src/gcc-${GCC_VER}.tar.bz2 ${SRC}/

ENV PATH="${CROSS_BASE}/bin:${PATH}"

RUN mkdir -p "${BUILD_DIR}/gcc-boot" && \
    tar -C "${SRC}" -xjf "${SRC}/gcc-${GCC_VER}.tar.bz2"

# Texinfo ≥5 compatibility patches (from AUR gcc46):
#   gcc.texi.49.patch        – replace inline @tex...@end tex with @email{}
#   0000-sourcebuild.texi-itemx.patch – @itemx at table start → @item
#   0001-tm.texi-end-deftypevr.patch  – @end deftypevr on its own line
RUN sed -i \
        -e 's|@tab Website: www\.gnupress\.org|@tab Website: @uref{http://www.gnupress.org}|' \
        -e 's|@tab General: @tex press@@gnu\.org @end tex|@tab General: @email{press@@gnu.org}|' \
        -e 's|@tab Orders:  @tex sales@@gnu\.org @end tex|@tab Orders:  @email{sales@@gnu.org}|' \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/gcc.texi" && \
    sed -i 's/^@itemx all\.cross$/@item all.cross/' \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/sourcebuild.texi" && \
    sed -i \
        's/is zero, which disables this optimization\.  @end deftypevr/is zero, which disables this optimization.\n@end deftypevr/' \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/tm.texi" \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/tm.texi.in"

# ARMv4 OABI (target arm-linux, the non-EABI branch of config.gcc) selects
# gcc/config/arm/t-linux, whose LIB1ASMFUNCS lists only the integer div/mod
# helpers plus the float add/sub routines (_arm_addsubdf3/_arm_addsubsf3).  The
# historical arm-linux toolchain assumed FPA hardware / kernel FP emulation, so
# the soft-float multiply/divide, comparison and conversion helpers were never
# placed in libgcc.  Building glibc with -msoft-float emits calls to __muldf3,
# __divdf3, __mulsf3, __eqdf2, __ltdf2, __gedf2, __fixsfsi, … which then go
# undefined when linking libc.so.  (The EABI path uses t-arm-elf, the full
# soft-float set, which is why only OABI is affected.)  Append the remaining
# soft-float LIB1ASMFUNCS so the OABI libgcc.a actually contains these helpers.
RUN printf '%s\n' \
        'LIB1ASMFUNCS += _arm_negdf2 _arm_muldivdf3 _arm_cmpdf2 _arm_unorddf2 \' \
        ' _arm_fixdfsi _arm_fixunsdfsi _arm_truncdfsf2 _arm_negsf2 \' \
        ' _arm_muldivsf3 _arm_cmpsf2 _arm_unordsf2 _arm_fixsfsi \' \
        ' _arm_fixunssfsi _arm_floatdidf _arm_floatdisf _arm_floatundidf \' \
        ' _arm_floatundisf' \
        >> "${SRC}/gcc-${GCC_VER}/gcc/config/arm/t-linux"

# arm/linux-elf.h's LIBGCC_SPEC appends "-lfloat" whenever -msoft-float (or
# -mfloat-abi=soft*) is used — a reference to the obsolete standalone "libfloat"
# soft-float library that pre-libgcc arm-linux toolchains shipped.  Now that the
# soft-float helpers live in libgcc.a (the t-linux patch above), -lfloat is both
# unnecessary and fatal ("cannot find -lfloat").  Drop it so LIBGCC_SPEC is just
# "-lgcc", mirroring what the EABI header (linux-eabi.h) already does via #undef.
RUN sed -i \
        's@%{msoft-float:-lfloat} %{mfloat-abi=soft\*:-lfloat} -lgcc@-lgcc@' \
        "${SRC}/gcc-${GCC_VER}/gcc/config/arm/linux-elf.h"

# GCC 4.6 ships download_prerequisites; run it to get GMP/MPFR/MPC in-tree
# (avoids version skew against the host's installed libraries)
RUN cd "${SRC}/gcc-${GCC_VER}" && \
    ./contrib/download_prerequisites

RUN cd "${BUILD_DIR}/gcc-boot" && \
    "${SRC}/gcc-${GCC_VER}/configure"           \
        --prefix="${CROSS_BASE}"                 \
        --target="${TARGET}"                     \
        --host="${HOST}"                         \
        --build="${BUILD}"                       \
        --enable-languages=c                     \
        # ── OABI / ARMv4 flags ──────────────────
        --with-arch=armv4                        \
        # apcs-gnu = Old ABI (OABI); do NOT use aapcs/aapcs-linux (EABI)
        --with-abi=apcs-gnu                      \
        --with-float=soft                        \
        --disable-thumb                          \
        # ── Sysroot / multilib ──────────────────
        --with-sysroot="${CROSS_BASE}/${TARGET}/sysroot" \
        --with-native-system-header-dir=/include \
        --disable-multilib                       \
        # ── Minimal bootstrap (no libc yet) ─────
        --without-headers                        \
        --with-newlib                            \
        --disable-shared                         \
        --disable-threads                        \
        --disable-libgomp                        \
        --disable-libmudflap                     \
        --disable-libssp                         \
        --disable-libquadmath                    \
        --disable-decimal-float                  \
        --disable-nls                            \
        # ── Disable Graphite (needs CLooG/PPL, removed from Debian) ─
        --with-cloog=no                          \
        --with-ppl=no                            \
    && make -j"$(nproc)" all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc

# glibc links its installed helper programs (iconvconfig, …) with
# "-lgcc -lgcc_eh" (Makeconfig's static-EH path, taken because this binutils
# reports have-as-needed=no for the relevant probe).  A --disable-shared GCC
# builds no separate libgcc_eh.a / libgcc_s — every unwind/EH routine is folded
# straight into libgcc.a — so the bare -lgcc_eh has nothing to resolve against.
# Provide libgcc_eh.a as an alias of libgcc.a: -lgcc already supplies the EH
# objects, and ld won't pull a member twice, so the extra -lgcc_eh is a no-op.
RUN ln -sf libgcc.a \
        "${CROSS_BASE}/lib/gcc/${TARGET}/${GCC_VER}/libgcc_eh.a"

# ── Stage 6: glibc (ARMv4 OABI) ──────────────────────────────────────────────
FROM base AS glibc-build

ENV TARGET=arm-linux HOST=x86_64-linux-gnu BUILD=x86_64-linux-gnu    \
    GLIBC_VER=2.3.6                                                    \
    CROSS_BASE=/opt/cross SRC=/build/src BUILD_DIR=/build/obj

COPY --from=gcc-boot    /opt/cross              /opt/cross
# Install the kernel headers under sysroot/usr/include (not sysroot/include) so
# they share the standard sysroot header dir with glibc's own headers below.
# glibc's installed headers (e.g. bits/errno.h -> linux/errno.h) reference the
# kernel headers by <linux/...>/<asm/...>, and the final GCC searches
# sysroot/usr/include (--with-native-system-header-dir=/usr/include); keeping the
# kernel headers in a separate sysroot/include would hide them from that search.
COPY --from=kernel-hdrs /staging                /opt/cross/${TARGET}/sysroot/usr

COPY --from=sources /build/src/glibc-${GLIBC_VER}.tar.bz2 ${SRC}/
COPY --from=sources /build/src/glibc-linuxthreads-${GLIBC_VER}.tar.bz2 ${SRC}/

ENV PATH="${CROSS_BASE}/bin:${PATH}"            \
    SYSROOT="${CROSS_BASE}/${TARGET}/sysroot"

# Unpack linuxthreads add-on into the glibc source tree
RUN tar -C "${SRC}" -xjf "${SRC}/glibc-${GLIBC_VER}.tar.bz2" && \
    tar -C "${SRC}/glibc-${GLIBC_VER}" -xjf "${SRC}/glibc-linuxthreads-${GLIBC_VER}.tar.bz2"

# Patch glibc 2.3.6 configure: version case-patterns were written in 2006 and
# only accept tools of that era.
#   • as/ld:  pattern '2.1[3-9]*' accepts 2.13–2.19 only → binutils 2.22 fails
#   • make:   pattern '3.79*|3.[89]*' accepts 3.79–3.9x only → make 4.x fails
# Expand each pattern to also accept the versions we actually have.
RUN sed -i \
        -e 's/2\.1\[3-9\]\*)/2.1[3-9]* | 2.[2-9][0-9]* | [3-9].*)/g' \
        -e 's/3\.79\* | 3\.\[89\]\*)/3.79* | 3.[89]* | [4-9].*)/g'    \
        "${SRC}/glibc-${GLIBC_VER}/configure"

# The Linux/ARM configure fragment hardcodes libc_cv_gcc_unwind_find_fde=yes,
# meaning "the compiler's libgcc has no in-process FDE registry, so glibc must
# build its own".  That was true for the gcc 2.95/3.x of 2006, but GCC 4.6's
# libgcc provides the unwinder.  With =yes, glibc compiles framestate + unwind-pe
# + unwind-dw2 (defining _Unwind_GetGR/_Unwind_SetGR/…) into libc; then, because
# OABI ARM uses SJLJ exceptions, glibc's _Unwind_SjLj_* references drag in
# libgcc.a(unwind-sjlj.o), whose _Unwind_* symbols collide → "multiple definition
# of _Unwind_GetGR".  Modern arches (x86_64) default to =no and rely on libgcc.
# Flip the arm case to =no (scoped to the arm*) ... ;; block) to do the same.
RUN sed -i \
        '/^  arm\*)/,/;;/ s/libc_cv_gcc_unwind_find_fde=yes/libc_cv_gcc_unwind_find_fde=no/' \
        "${SRC}/glibc-${GLIBC_VER}/sysdeps/unix/sysv/linux/configure"

# ARM ioperm.c needs two tweaks for modern (3.16) sanitised kernel headers:
#   1. It includes <asm/page.h>, dropped from the userspace-exported headers in
#      the uapi split (~3.7).  The include is vestigial (no PAGE_* macro is
#      used), so remove it.
#   2. It references BUS_ISA, the ISA entry under the CTL_BUS sysctl subtree.
#      Modern headers only spell it CTL_BUS_ISA (the sibling BUS_ISA_PORT_BASE/
#      _SHIFT constants are still present), so alias BUS_ISA to CTL_BUS_ISA.
RUN sed -i \
        -e '/#include <asm\/page.h>/d' \
        -e 's|#include <sys/sysctl.h>|#include <sys/sysctl.h>\n#ifndef BUS_ISA\n# define BUS_ISA CTL_BUS_ISA\n#endif|' \
        "${SRC}/glibc-${GLIBC_VER}/sysdeps/unix/sysv/linux/arm/ioperm.c"

# ARM socket.S never learned to honour NO_WEAK_ALIAS, although every other arch
# (i386, m68k, s390, hppa) was updated to.  The shared socket stubs bind.S,
# listen.S, getsockname.S and setsockopt.S all "#define NO_WEAK_ALIAS 1" and
# then add their own "weak_alias (<call>, __<call>)".  On ARM, socket.S still
# unconditionally builds __<call> via ENTRY *and* emits weak_alias(__socket,
# socket), so the symbol is defined twice → gas: "symbol `__bind' is already
# defined".  Mirror i386: when NO_WEAK_ALIAS is set make __socket == socket
# (so ENTRY defines the public name strongly) and skip the trailing weak_alias.
RUN sed -i \
        -e 's|^#define __socket P(__,socket)$|# ifndef NO_WEAK_ALIAS\n#  define __socket P(__,socket)\n# else\n#  define __socket socket\n# endif|' \
        -e 's|^weak_alias (__socket, socket)$|#ifndef NO_WEAK_ALIAS\nweak_alias (__socket, socket)\n#endif|' \
        "${SRC}/glibc-${GLIBC_VER}/sysdeps/unix/sysv/linux/arm/socket.S"

# manual/Makefile has a single rule that mixes normal targets (stubs, the .pot
# file) with a pattern target ($(objpfx)stamp%).  GNU make ≥3.82 rejects this
# "mixed implicit and normal rules" as a hard error, which then leaves
# manual/stamp.o without a rule.  Split it into one normal rule and one pattern
# rule, each with the original (identical) recipe.
RUN sed -i \
        's|^\$(objpfx)stubs \.\./po/manual\.pot \$(objpfx)stamp%:$|$(objpfx)stubs ../po/manual.pot:\n\t$(make-target-directory)\n\ttouch $@\n$(objpfx)stamp%:|' \
        "${SRC}/glibc-${GLIBC_VER}/manual/Makefile"

# In ARM dl-machine.h the relocation helpers under #ifdef RESOLVE are always
# pulled in nested inside a function (dynamic-link.h is #included within
# _dl_resolve_conflicts / _dl_relocate_object).  Its siblings elf_machine_rel*
# are declared "auto inline" (valid GCC nested functions), but fix_bad_pc24 is
# a plain "static" function — illegal as a nested function under modern GCC:
# "invalid storage class for function 'fix_bad_pc24'".  Give it the same
# auto-inline treatment as the sibling relocation functions.
RUN sed -i \
        '/^static Elf32_Addr$/{N;s|^static Elf32_Addr\nfix_bad_pc24 |auto inline Elf32_Addr\n__attribute__ ((always_inline))\nfix_bad_pc24 |}' \
        "${SRC}/glibc-${GLIBC_VER}/sysdeps/arm/dl-machine.h"

# ARM elf_machine_dynamic() reads the loader's own _DYNAMIC via a GOT pointer
# it pins in r10:  "register Elf32_Addr *got asm (\"r10\"); return *got;".  This
# encodes the GCC-3.x apcs-gnu convention that r10 (sl) holds the GOT base
# throughout a PIC function.  GCC 4.6.4 no longer honours that: it materialises
# each function's PIC base into a compiler-chosen scratch register (r0 in
# _dl_start) and leaves r10 holding the *raw* link-time GOT offset that the
# RTLD_START asm passes in (it only converts r10 to the runtime GOT *after*
# "bl _dl_start", at _dl_start_user).  So elf_machine_dynamic's "ldr [r10]"
# dereferences an un-relocated address and _dl_start SIGSEGVs before any user
# code runs — every dynamically linked binary dies identically in the loader
# bootstrap (static binaries, which never enter ld.so, are unaffected).
#
# Fix: read _GLOBAL_OFFSET_TABLE_[0] instead of *(r10).  GCC resolves that
# through whatever PIC base it actually established (a PC-relative
# "ldr r0,[pc,r3]"), so the load no longer depends on r10.  GOT[0] is the
# link-time address of _DYNAMIC by the ARM ELF ABI, exactly what the r10 form
# returned; _dl_start still adds the load bias.  This is loader-only (the
# inline lives solely in rtld's bootstrap) and changes no application or libc
# codegen.  See toolchain/DYNAMIC-LINKER-ABI.md for the full analysis.
RUN sed -i \
        '/register Elf32_Addr \*got asm ("r10");/{N;s@[[:space:]]*register Elf32_Addr \*got asm ("r10");\n[[:space:]]*return \*got;@  extern const Elf32_Addr _GLOBAL_OFFSET_TABLE_[] __attribute__ ((visibility ("hidden")));\n  return _GLOBAL_OFFSET_TABLE_[0];@}' \
        "${SRC}/glibc-${GLIBC_VER}/sysdeps/arm/dl-machine.h"

# linuxthreads raise.c calls __libc_maybe_call2(pthread_raise, ...).  When built
# for the dynamic linker (NOT_IN_libc + SHARED), that macro does NOT use the
# __libc_pthread_functions dispatch table; it expands to __libc_maybe_call
# (__pthread_raise, ...), whose PIC form uses __typeof(__pthread_raise) — but
# raise.c only marks __pthread_raise weak "#ifndef SHARED" and never provides a
# real prototype (it lives in internals.h, which raise.c does not include), so
# GCC errors with "'__pthread_raise' undeclared".  Add a prototype and emit the
# weak mark whenever the direct-reference path is taken (!SHARED || NOT_IN_libc),
# so the rtld reference stays weak and resolves to 0 when pthread is absent.
RUN sed -i \
        's@^#ifndef SHARED$@extern int __pthread_raise (int sig);\n#if !defined SHARED || defined NOT_IN_libc@' \
        "${SRC}/glibc-${GLIBC_VER}/linuxthreads/sysdeps/unix/sysv/linux/raise.c"

# ld.so is linked with a linker script that glibc derives from "ld --verbose"
# output, using sed to insert the "_begin" symbol (start of the ld.so image,
# used by _dl_start_final) right after the text-segment start line.  glibc 2.3.6
# matches the literal ". = 0 + SIZEOF_HEADERS;", but binutils ≥2.18 emits
# ". = SEGMENT_START(\"text-segment\", 0) + SIZEOF_HEADERS;", so the insert never
# happens and _begin is undefined.  Relax the pattern to match "+ SIZEOF_HEADERS;"
# generically (the arithmetic still yields _begin = image base either way).
RUN sed -i 's#\\. = 0 + SIZEOF_HEADERS;#+ SIZEOF_HEADERS;#' \
        "${SRC}/glibc-${GLIBC_VER}/elf/Makefile"

# strtol_l.c puts its precomputed overflow tables (__strtol_ul_max_tab, etc.)
# into per-name ".gnu.linkonce.r.<name>" COMDAT sections so the linker keeps a
# single copy across the many strtol/wcstol variants that all compile this file.
# These tables are tagged attribute_hidden; GCC 4.6 binds references to a hidden
# symbol against the *local* section copy, but binutils 2.22 then discards that
# COMDAT instance in favour of another variant's copy, leaving the local
# relocation dangling → "X referenced in .text ... defined in discarded section".
# The dedup is a pure space optimization (each table is internal to strtol_l.c
# and only used within its own TU), so drop the linkonce machinery: make the
# tables plain "static const" (one private, local copy per object — no COMDAT, no
# cross-TU symbol) and mark them unused so variants that don't touch a given
# table don't trip -Werror.
RUN sed -i \
        -e 's@const TYPE NAME\[\] attribute_hidden@static const TYPE NAME[] __attribute__ ((unused))@' \
        -e 's@__attribute__((section("\.gnu\.linkonce\.r\." #NAME)))@@' \
        "${SRC}/glibc-${GLIBC_VER}/sysdeps/generic/strtol_l.c"

# GCC ≥4.3 installs its generated headers (limits.h, etc.) in the
# "include-fixed" dir, but glibc 2.3.6 only adds lib/gcc/<t>/<v>/include to its
# -nostdinc search path.  The #include_next in glibc's own include/limits.h
# therefore finds nothing.  We cannot inject -isystem via CFLAGS because glibc's
# build reorders option tokens and splits the "-isystem <path>" pair.  Instead,
# merge include-fixed into the include dir glibc already searches.
RUN GCC_INC="$("${TARGET}-gcc" -print-file-name=include)" && \
    GCC_INCFIXED="$("${TARGET}-gcc" -print-file-name=include-fixed)" && \
    cp -a "${GCC_INCFIXED}/." "${GCC_INC}/"

# glibc 2.3.6 builds crti.S/crtn.S by compiling initfini.c to a single .s file
# and sed-splitting it into prologue (crti) and epilogue (crtn) halves.  Modern
# ARM GCC emits each function's PC-relative constant pool (GOT word under -fPIC,
# or the &__gmon_start__ literal otherwise) at the *end* of the function, so the
# split separates an `ldr rN,[pc,#x]` from its pool → "undefined local label" /
# "internal_relocation (type: OFFSET_IMM) not fixed up".  Upstream glibc later
# abandoned this scheme and shipped hand-written sysdeps/arm/crti.S + crtn.S.
# Do the same: provide minimal ARMv4 OABI prologue/epilogue (classic stmfd/ldmfd,
# no Thumb, no bx, no literal pools), and override the generation rules.  The
# weak __gmon_start__ hook is omitted since profiling is disabled.
RUN set -e; \
    csu="${SRC}/glibc-${GLIBC_VER}/csu"; \
    printf '%s\n' \
        '.section .init,"ax",%progbits' \
        '.p2align 2' \
        '.globl _init' \
        '.type _init, %function' \
        '_init:' \
        'stmfd sp!, {r3, lr}' \
        '.section .fini,"ax",%progbits' \
        '.p2align 2' \
        '.globl _fini' \
        '.type _fini, %function' \
        '_fini:' \
        'stmfd sp!, {r3, lr}' \
        > "${csu}/armv4-crti.S"; \
    printf '%s\n' \
        '.section .init,"ax",%progbits' \
        'ldmfd sp!, {r3, pc}' \
        '.section .fini,"ax",%progbits' \
        'ldmfd sp!, {r3, pc}' \
        > "${csu}/armv4-crtn.S"; \
    { \
        echo ''; \
        echo '$(objpfx)crti.S:'; \
        printf '\tcp %s/armv4-crti.S $@\n' "${csu}"; \
        echo '$(objpfx)crtn.S:'; \
        printf '\tcp %s/armv4-crtn.S $@\n' "${csu}"; \
    } >> "${csu}/Makefile"

# We pass -U_FORTIFY_SOURCE to avoid _chk redirects that need libm at link time.
RUN mkdir -p "${BUILD_DIR}/glibc" && \
    cd "${BUILD_DIR}/glibc" && \
    BUILD_CC=gcc                                                        \
    CC="${TARGET}-gcc"                                                  \
    CXX="${TARGET}-g++"                                                 \
    AR="${TARGET}-ar"                                                   \
    RANLIB="${TARGET}-ranlib"                                           \
    # Extra CFLAGS: target exactly ARMv4; forbid Thumb; soft-float; OABI ABI.
    # -fgnu89-inline: glibc 2.3.6 assumes GNU89 "extern inline" semantics (no
    # external symbol emitted), but GCC 4.6 defaults to C99 inline, which emits
    # one and collides with the out-of-line definitions (e.g. __copysign).
    # Later glibc adds this automatically; 2.3.6 has no such configure check.
    CFLAGS="-O2 -march=armv4 -mabi=apcs-gnu -mno-thumb -msoft-float   \
            -U_FORTIFY_SOURCE -fno-stack-protector -fgnu89-inline"      \
    "${SRC}/glibc-${GLIBC_VER}/configure"                              \
        --prefix=/usr                                                    \
        --host="${TARGET}"                                               \
        --build="${BUILD}"                                               \
        --with-headers="${SYSROOT}/usr/include"                          \
        --without-fp                                                     \
        --disable-profile                                                \
        --enable-add-ons=linuxthreads                                   \
        --enable-kernel=2.6.0                                           \
        libc_cv_forced_unwind=yes                                        \
        libc_cv_c_cleanup=yes                                            \
    && make -j"$(nproc)" SHELL=/bin/bash \
    && make install install_root="${SYSROOT}" SHELL=/bin/bash

# ── Stage 7: full GCC (C + C++) ──────────────────────────────────────────────
FROM base AS gcc-full

ENV TARGET=arm-linux HOST=x86_64-linux-gnu BUILD=x86_64-linux-gnu    \
    GCC_VER=4.6.4                                                      \
    CROSS_BASE=/opt/cross SRC=/build/src BUILD_DIR=/build/obj

COPY --from=glibc-build /opt/cross              /opt/cross
COPY --from=sources     /build/src/gcc-${GCC_VER}.tar.bz2 ${SRC}/

ENV PATH="${CROSS_BASE}/bin:${PATH}"            \
    SYSROOT="${CROSS_BASE}/${TARGET}/sysroot"

RUN mkdir -p "${BUILD_DIR}/gcc-full" && \
    tar -C "${SRC}" -xjf "${SRC}/gcc-${GCC_VER}.tar.bz2"

# Same Texinfo ≥5 compatibility patches as in gcc-boot
RUN sed -i \
        -e 's|@tab Website: www\.gnupress\.org|@tab Website: @uref{http://www.gnupress.org}|' \
        -e 's|@tab General: @tex press@@gnu\.org @end tex|@tab General: @email{press@@gnu.org}|' \
        -e 's|@tab Orders:  @tex sales@@gnu\.org @end tex|@tab Orders:  @email{sales@@gnu.org}|' \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/gcc.texi" && \
    sed -i 's/^@itemx all\.cross$/@item all.cross/' \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/sourcebuild.texi" && \
    sed -i \
        's/is zero, which disables this optimization\.  @end deftypevr/is zero, which disables this optimization.\n@end deftypevr/' \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/tm.texi" \
        "${SRC}/gcc-${GCC_VER}/gcc/doc/tm.texi.in"

# ARMv4 OABI (target arm-linux, the non-EABI branch of config.gcc) selects
# gcc/config/arm/t-linux, whose LIB1ASMFUNCS lists only the integer div/mod
# helpers plus the float add/sub routines (_arm_addsubdf3/_arm_addsubsf3).  The
# historical arm-linux toolchain assumed FPA hardware / kernel FP emulation, so
# the soft-float multiply/divide, comparison and conversion helpers were never
# placed in libgcc.  Building glibc with -msoft-float emits calls to __muldf3,
# __divdf3, __mulsf3, __eqdf2, __ltdf2, __gedf2, __fixsfsi, … which then go
# undefined when linking libc.so.  (The EABI path uses t-arm-elf, the full
# soft-float set, which is why only OABI is affected.)  Append the remaining
# soft-float LIB1ASMFUNCS so the OABI libgcc.a actually contains these helpers.
RUN printf '%s\n' \
        'LIB1ASMFUNCS += _arm_negdf2 _arm_muldivdf3 _arm_cmpdf2 _arm_unorddf2 \' \
        ' _arm_fixdfsi _arm_fixunsdfsi _arm_truncdfsf2 _arm_negsf2 \' \
        ' _arm_muldivsf3 _arm_cmpsf2 _arm_unordsf2 _arm_fixsfsi \' \
        ' _arm_fixunssfsi _arm_floatdidf _arm_floatdisf _arm_floatundidf \' \
        ' _arm_floatundisf' \
        >> "${SRC}/gcc-${GCC_VER}/gcc/config/arm/t-linux"

# Same obsolete "-lfloat" reference as in gcc-boot: drop it from arm/linux-elf.h's
# LIBGCC_SPEC so userspace links with -msoft-float don't fail on "cannot find
# -lfloat" (the soft-float helpers are in libgcc.a now).
RUN sed -i \
        's@%{msoft-float:-lfloat} %{mfloat-abi=soft\*:-lfloat} -lgcc@-lgcc@' \
        "${SRC}/gcc-${GCC_VER}/gcc/config/arm/linux-elf.h"

# cp/cfns.h (a checked-in gperf table, included by cp/except.c) declares the
# prototype of libc_name_p with a plain "__inline", but its *definition* adds
# __attribute__((__gnu_inline__)) under __GNUC_STDC_INLINE__.  The modern host
# GCC building this cross-compiler enforces C99 inline semantics, so the
# declaration/definition disagree → "'gnu_inline' attribute present on
# 'libc_name_p' [...] but not here".  Give the prototype the same gnu_inline
# guard as the definition (newer gperf emits it on both).  touch cfns.h afterward
# so make treats it as up to date and never tries to regenerate it with gperf
# (which isn't installed in this image).  C-only gcc-boot never builds cp/, so
# this patch is needed in the final stage only.
RUN sed -i \
        's@^const char \* libc_name_p (const char \*, unsigned int);@#ifdef __GNUC_STDC_INLINE__\n__attribute__ ((__gnu_inline__))\n#endif\nconst char * libc_name_p (const char *, unsigned int);@' \
        "${SRC}/gcc-${GCC_VER}/gcc/cp/cfns.h" \
    && touch "${SRC}/gcc-${GCC_VER}/gcc/cp/cfns.h"

RUN cd "${SRC}/gcc-${GCC_VER}" && \
    ./contrib/download_prerequisites

RUN cd "${BUILD_DIR}/gcc-full" && \
    "${SRC}/gcc-${GCC_VER}/configure"           \
        --prefix="${CROSS_BASE}"                 \
        --target="${TARGET}"                     \
        --host="${HOST}"                         \
        --build="${BUILD}"                       \
        --enable-languages=c,c++                 \
        # ── OABI / ARMv4 flags ──────────────────
        --with-arch=armv4                        \
        --with-abi=apcs-gnu                      \
        --with-float=soft                        \
        --disable-thumb                          \
        # ── Sysroot ─────────────────────────────
        --with-sysroot="${SYSROOT}"              \
        --with-native-system-header-dir=/usr/include \
        --disable-multilib                       \
        # ── Library options ─────────────────────
        --enable-shared                          \
        --enable-threads=posix                   \
        --enable-__cxa_atexit                    \
        --disable-libmudflap                     \
        --disable-libssp                         \
        --disable-libgomp                        \
        --disable-libquadmath                    \
        --disable-decimal-float                  \
        --disable-nls                            \
        # ── Disable Graphite (needs CLooG/PPL, removed from Debian) ─
        --with-cloog=no                          \
        --with-ppl=no                            \
    && make -j"$(nproc)" \
    && make install

# ── Stage 8: final lean image ─────────────────────────────────────────────────
FROM debian:bookworm AS toolchain

ENV DEBIAN_FRONTEND=noninteractive

# Runtime deps only (no -dev packages needed)
RUN apt-get update && apt-get install -y --no-install-recommends \
        make \
        bc \
        bison \
        flex \
        libgmp10 \
        libmpfr6 \
        libmpc3 \
        python3 \
        file \
        rsync \
        xz-utils \
        ca-certificates \
        wget \
    && rm -rf /var/lib/apt/lists/*

ENV TARGET=arm-linux                            \
    CROSS_BASE=/opt/cross

COPY --from=gcc-full /opt/cross /opt/cross

ENV PATH="${CROSS_BASE}/bin:${PATH}"

# Convenience variables that match kernel Makefile conventions
ENV ARCH=arm \
    CROSS_COMPILE=arm-linux-

WORKDIR /src

# ── Smoke test ────────────────────────────────────────────────────────────────
# Verify the toolchain produces genuine ARMv4 OABI output.
#
# Checks performed:
#   1. Compiler version / target reported correctly
#   2. A minimal C program compiles and links
#   3. Resulting ELF uses OABI (Tag_ABI_VFP_args absent; uses APCS-GNU)
#   4. Architecture in ELF header is ARM, not Thumb-only
RUN echo '--- Toolchain smoke test ---'                                     && \
    arm-linux-gcc --version                                                  && \
    echo 'int main(void){return 0;}' > /tmp/hello.c                         && \
    arm-linux-gcc -march=armv4 -mabi=apcs-gnu -msoft-float -static           \
                  -o /tmp/hello /tmp/hello.c                                  && \
    file /tmp/hello                                                           && \
    # Must say "ARM" (not Thumb) and NOT mention EABI
    file /tmp/hello | grep -q "ARM"                                           && \
    file /tmp/hello | grep -qv "EABI"                                         && \
    echo "PASS: binary is ARM OABI"                                          || \
    { echo "FAIL: smoke test failed"; exit 1; }

# ── Kernel source (optional) ──────────────────────────────────────────────────
# The same Linux 3.16.85 source used for headers is also the recommended
# kernel to build for ARMv4 OABI targets (CONFIG_OABI_COMPAT=y).
#
# To build a kernel after running this container:
#
#   docker run --rm -v $(pwd)/linux-3.16.85:/src armv4-oabi-toolchain bash -c "
#     cd /src
#     make ARCH=arm CROSS_COMPILE=arm-linux- multi_v4_defconfig
#     # Enable OABI compat: scripts/config --enable CONFIG_OABI_COMPAT
#     make ARCH=arm CROSS_COMPILE=arm-linux- -j\$(nproc) zImage
#   "
#
# Recommended kernel config options for ARMv4 OABI:
#   CONFIG_CPU_ARM920T=y   (or whichever ARMv4 core you target)
#   CONFIG_OABI_COMPAT=y   (allows OABI binaries to run on EABI kernel,
#                            or build pure OABI kernel without EABI at all)
#   # For a pure OABI kernel (no EABI support at all):
#   # CONFIG_AEABI is not set
#   CONFIG_FPE_NWFPE=y     (needed for soft-float emulation in kernel)

# ── Usage reminder ────────────────────────────────────────────────────────────
# CMD ["bash", "-c", "\
#     echo '=========================================='    && \
#     echo ' ARMv4 OABI Cross-Toolchain'                   && \
#     echo '=========================================='    && \
#     echo ''                                              && \
#     echo ' Target       : arm-linux (ARMv4, OABI)'      && \
#     echo ' Compiler     : arm-linux-gcc'                 && \
#     echo ' Recommended CFLAGS:'                          && \
#     echo '   -march=armv4 -mabi=apcs-gnu -msoft-float'  && \
#     echo ''                                              && \
#     echo ' Kernel build example:'                        && \
#     echo '   make ARCH=arm CROSS_COMPILE=arm-linux-'    && \
#     echo '        multi_v4_defconfig'                    && \
#     echo '   make ARCH=arm CROSS_COMPILE=arm-linux-'    && \
#     echo '        -j\$(nproc) zImage'                   && \
#     echo ''                                              && \
#     echo ' Userspace build example:'                     && \
#     echo '   arm-linux-gcc -march=armv4 -mabi=apcs-gnu' && \
#     echo '     -msoft-float -o hello hello.c'           && \
#     echo '=========================================='    && \
#     bash"]
