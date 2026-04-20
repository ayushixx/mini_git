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
#define HEAD_FILE ".minigit/HEAD"

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

// Get current HEAD commit hash (empty string if none)
void get_head(char *out, size_t out_size) {
    if (!file_exists(HEAD_FILE)) {
        out[0] = '\0';
        return;
    }
    FILE *f = fopen(HEAD_FILE, "r");
    if (f && fgets(out, out_size, f)) {
        char *nl = strchr(out, '\n');
        if (nl) *nl = '\0';
    } else {
        out[0] = '\0';
    }
    if (f) fclose(f);
}

// Update HEAD to point to a commit hash
void set_head(const char *hash) {
    write_file(HEAD_FILE, hash, strlen(hash));
}

void cmd_init() {
    ensure_dir(MG_DIR);
    ensure_dir(OBJ_DIR);
    ensure_dir(COMMITS_DIR);
    FILE *f = fopen(STAGE_FILE, "w");
    if (f) fclose(f);
    // Remove old HEAD if present
    remove(HEAD_FILE);
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
    // Check if file already staged; if so, remove old entry first (simple approach: rewrite index)
    FILE *f = fopen(STAGE_FILE, "r");
    char **lines = NULL;
    int line_count = 0;
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char existing_hash[64], existing_name[256];
            if (sscanf(line, "%s %s", existing_hash, existing_name) == 2) {
                if (strcmp(existing_name, filename) == 0)
                    continue; // skip old entry
                lines = realloc(lines, (line_count+1) * sizeof(char*));
                lines[line_count++] = strdup(line);
            }
        }
        fclose(f);
    }
    // Write new index
    f = fopen(STAGE_FILE, "w");
    for (int i = 0; i < line_count; i++) {
        fprintf(f, "%s\n", lines[i]);
        free(lines[i]);
    }
    fprintf(f, "%s %s\n", hash, filename);
    fclose(f);
    free(lines);
    printf("Staged %s\n", filename);
}

void cmd_commit(const char *msg) {
    size_t len;
    char *index = read_file(STAGE_FILE, &len);
    if (!index || len == 0) {
        printf("Nothing staged.\n");
        return;
    }
    // Get parent commit hash
    char parent_hash[64] = "none";
    char head[64];
    get_head(head, sizeof(head));
    if (strlen(head) > 0) {
        strcpy(parent_hash, head);
    }
    // Prepare commit data: parent, timestamp, message, and the full index
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    char commit_content[1024 * 1024]; // large enough
    snprintf(commit_content, sizeof(commit_content),
             "parent %s\ntimestamp %s\nmessage %s\nfiles:\n%s",
             parent_hash, timestamp, msg, index);
    unsigned long cid = simple_hash((unsigned char *)commit_content, strlen(commit_content));
    char commit_id[64];
    sprintf(commit_id, "%08lx", cid);
    char commit_path[256];
    sprintf(commit_path, "%s/%s.commit", COMMITS_DIR, commit_id);
    write_file(commit_path, commit_content, strlen(commit_content));
    // Update HEAD
    set_head(commit_id);
    // Clear staging area
    FILE *f = fopen(STAGE_FILE, "w");
    fclose(f);
    free(index);
    printf("Committed %s: %s\n", commit_id, msg);
}

void cmd_log() {
    char head[64];
    get_head(head, sizeof(head));
    if (strlen(head) == 0) {
        printf("No commits yet.\n");
        return;
    }
    char commit_id[64];
    strcpy(commit_id, head);
    while (1) {
        char commit_path[256];
        sprintf(commit_path, "%s/%s.commit", COMMITS_DIR, commit_id);
        if (!file_exists(commit_path)) break;
        char *content = read_file(commit_path, NULL);
        if (!content) break;
        // Parse parent and message
        char parent[64] = "none";
        char timestamp[128] = "";
        char message[256] = "";
        char *line = strtok(content, "\n");
        while (line) {
            if (strncmp(line, "parent ", 7) == 0) {
                strcpy(parent, line + 7);
            } else if (strncmp(line, "timestamp ", 10) == 0) {
                strcpy(timestamp, line + 10);
            } else if (strncmp(line, "message ", 8) == 0) {
                strcpy(message, line + 8);
            }
            line = strtok(NULL, "\n");
        }
        printf("commit %s\nDate: %s\n    %s\n\n", commit_id, timestamp, message);
        free(content);
        if (strcmp(parent, "none") == 0) break;
        strcpy(commit_id, parent);
    }
}

void cmd_checkout(const char *commit_id) {
    char commit_path[256];
    sprintf(commit_path, "%s/%s.commit", COMMITS_DIR, commit_id);
    if (!file_exists(commit_path)) {
        printf("Commit %s not found.\n", commit_id);
        return;
    }
    char *content = read_file(commit_path, NULL);
    if (!content) return;
    // Find the "files:" section
    char *files_section = strstr(content, "\nfiles:\n");
    if (!files_section) {
        free(content);
        printf("Invalid commit file.\n");
        return;
    }
    files_section += 8; // skip "\nfiles:\n"
    // Parse each line: hash filename
    char *line = strtok(files_section, "\n");
    while (line) {
        char hash[64], filename[256];
        if (sscanf(line, "%s %s", hash, filename) == 2) {
            char objpath[256];
            sprintf(objpath, "%s/%s", OBJ_DIR, hash);
            if (file_exists(objpath)) {
                char *blob = read_file(objpath, NULL);
                if (blob) {
                    write_file(filename, blob, strlen(blob));
                    free(blob);
                    printf("Restored %s\n", filename);
                } else {
                    printf("Failed to read blob %s\n", hash);
                }
            } else {
                printf("Blob %s missing for %s\n", hash, filename);
            }
        }
        line = strtok(NULL, "\n");
    }
    free(content);
    // Update HEAD to the checked out commit
    set_head(commit_id);
    // Clear staging area (optional, but consistent with real git)
    FILE *f = fopen(STAGE_FILE, "w");
    fclose(f);
    printf("Checked out commit %s\n", commit_id);
}

void cmd_status() {
    if (!file_exists(STAGE_FILE)) {
        printf("Not a MiniGit repository.\n");
        return;
    }
    size_t len;
    char *index = read_file(STAGE_FILE, &len);
    if (!index || len == 0) {
        printf("Nothing staged.\n");
    } else {
        printf("Staged files:\n");
        char *line = strtok(index, "\n");
        while (line) {
            printf("  %s\n", line);
            line = strtok(NULL, "\n");
        }
    }
    free(index);
    char head[64];
    get_head(head, sizeof(head));
    if (strlen(head) > 0) {
        printf("Current commit: %s\n", head);
    } else {
        printf("No commits yet.\n");
    }
}

void print_help() {
    printf("MiniGit commands:\n");
    printf("  init                     - create empty repository\n");
    printf("  add <file>              - stage a file\n");
    printf("  commit -m \"msg\"         - commit staged changes\n");
    printf("  log                     - show commit history\n");
    printf("  checkout <commit-id>    - restore all files from a commit\n");
    printf("  status                  - show staged files and HEAD\n");
    printf("  help                    - show this help\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }
    if (strcmp(argv[1], "init") == 0) {
        cmd_init();
    } else if (strcmp(argv[1], "add") == 0 && argc >= 3) {
        cmd_add(argv[2]);
    } else if (strcmp(argv[1], "commit") == 0) {
        if (argc < 3) {
            printf("Usage: commit -m \"message\"\n");
            return 1;
        }
        char *msg = NULL;
        if (strcmp(argv[2], "-m") == 0 && argc >= 4) {
            msg = argv[3];
        } else {
            // also allow "commit message" without -m
            msg = argv[2];
        }
        cmd_commit(msg);
    } else if (strcmp(argv[1], "log") == 0) {
        cmd_log();
    } else if (strcmp(argv[1], "checkout") == 0 && argc >= 3) {
        cmd_checkout(argv[2]);
    } else if (strcmp(argv[1], "status") == 0) {
        cmd_status();
    } else if (strcmp(argv[1], "help") == 0) {
        print_help();
    } else {
        printf("Unknown command. Try 'help'.\n");
    }
    return 0;
}
