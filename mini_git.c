#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define MG_DIR ".minigit"
#define OBJ_DIR ".minigit/objects"
#define COMMITS_DIR ".minigit/commits"
#define STAGE_FILE ".minigit/index"

unsigned long simple_hash(const unsigned char *data, size_t len) {
    unsigned long h = 1469598103934665603UL;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned long)data[i];
        h *= 1099511628211UL;
    }
    return h;
}

void ensure_dir(const char *p) {
    struct stat st;
    if (stat(p, &st) == -1) mkdir(p, 0755);
}

int file_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long s = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(s + 1);
    fread(buf, 1, s, f);
    fclose(f);
    buf[s] = '\0';
    if (len) *len = s;
    return buf;
}

void write_file(const char *path, const char *buf, size_t len) {
    FILE *f = fopen(path, "wb");
    fwrite(buf, 1, len, f);
    fclose(f);
}

void cmd_init() {
    ensure_dir(MG_DIR);
    ensure_dir(OBJ_DIR);
    ensure_dir(COMMITS_DIR);
    FILE *f = fopen(STAGE_FILE, "w");
    if (f) fclose(f);
    printf("Initialized empty MiniGit repository.\n");
}

void cmd_add(const char *filename) {
    if (!file_exists(filename)) {
        printf("File not found.\n");
        return;
    }
    size_t len;
    char *buf = read_file(filename, &len);
    unsigned long id = simple_hash((unsigned char *)buf, len);
    char hash[64];
    sprintf(hash, "%08lx", id);
    char objpath[256];
    sprintf(objpath, "%s/%s", OBJ_DIR, hash);
    write_file(objpath, buf, len);
    free(buf);
    FILE *f = fopen(STAGE_FILE, "a");
    fprintf(f, "%s %s\n", hash, filename);
    fclose(f);
    printf("Staged %s\n", filename);
}

void cmd_commit(const char *msg) {
    size_t len;
    char *index = read_file(STAGE_FILE, &len);
    if (!index || len == 0) {
        printf("Nothing staged.\n");
        return;
    }
    unsigned long cid = simple_hash((unsigned char *)index, len);
    char id[32];
    sprintf(id, "%08lx", cid);
    char commitfile[256];
    sprintf(commitfile, "%s/%s.commit", COMMITS_DIR, id);
    FILE *cf = fopen(commitfile, "w");
    fprintf(cf, "commit %s\nmessage %s\nfiles:\n%s", id, msg, index);
    fclose(cf);
    FILE *f = fopen(STAGE_FILE, "w");
    fclose(f);
    printf("Committed %s - %s\n", id, msg);
    free(index);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Commands: init | add <file> | commit -m \"msg\"\n");
        return 0;
    }
    if (strcmp(argv[1], "init") == 0)
        cmd_init();
    else if (strcmp(argv[1], "add") == 0)
        cmd_add(argv[2]);
    else if (strcmp(argv[1], "commit") == 0 && argc > 3)
        cmd_commit(argv[3]);
    else
        printf("Invalid command.\n");
    return 0;
}
