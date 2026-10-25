#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static const char KEY[] = "W3lc0m3_t0_ECSC2026!";

void encrypt_file(const char *name)
{
    char out_name[512];
    FILE *in, *out;
    int c;
    size_t i = 0;
    size_t keylen = strlen(KEY);

    in = fopen(name, "rb");
    if (in == NULL) {
        printf("could not open %s\n", name);
        return;
    }

    snprintf(out_name, sizeof(out_name), "%s.encrypted", name);
    out = fopen(out_name, "wb");
    if (out == NULL) {
        printf("could not create %s\n", out_name);
        fclose(in);
        return;
    }

    while ((c = fgetc(in)) != EOF) {
        fputc(c ^ KEY[i % keylen], out);
        i++;
    }

    fclose(in);
    fclose(out);

    printf("encrypted %s -> %s\n", name, out_name);
}

/* our own file name, taken from argv[0] without any leading path */
const char *get_self_name(int argc, char *argv[])
{
    const char *slash;

    if (argc < 1 || argv[0] == NULL || argv[0][0] == '\0')
        return "encrypt";

    slash = strrchr(argv[0], '/');
    if (slash != NULL)
        return slash + 1;

    return argv[0];
}

int main(int argc, char *argv[])
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    const char *self = get_self_name(argc, argv);

    dir = opendir(".");
    if (dir == NULL) {
        printf("could not open current directory\n");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        /* skip "." and ".." */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* only handle regular files, skip directories */
        if (stat(entry->d_name, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        /* do not encrypt ourselves or already encrypted files */
        if (strcmp(entry->d_name, self) == 0)
            continue;
        if (strlen(entry->d_name) > 10 &&
            strcmp(entry->d_name + strlen(entry->d_name) - 10, ".encrypted") == 0)
            continue;

        encrypt_file(entry->d_name);
    }

    closedir(dir);
    return 0;
}
