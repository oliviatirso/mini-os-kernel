/* directory.c — Directory system calls: mkdir, print_dir_table */

#include "kernel.h"

/*
 * Creates a new directory entry in the kernel's directory table.
 * Parameters:
 *   pid     — calling process ID (becomes the directory owner)
 *   dirname — name of the directory to create
 * Returns:
 *   dir_id (>= 0) on success, negative error code on failure
 */
int sys_mkdir(int pid, const char *dirname) {
    if (!dirname || dirname[0] == '\0') {
        printf("[ERROR] mkdir: invalid directory name\n");
        return -1;
    }
    for (int i = 0; i < MAX_DIRS; i++) {
        if (dir_table[i].is_active &&
            strncmp(dir_table[i].name, dirname, MAX_DIRNAME) == 0) {
            printf("[ERROR] mkdir: directory '%s' already exists\n", dirname);
            log_syscall(pid, "mkdir", -1, -4);
            return -4;
        }
    }
    for (int i = 0; i < MAX_DIRS; i++) {
        if (!dir_table[i].is_active) {
            dir_table[i].dir_id    = i;
            dir_table[i].owner_pid = pid;
            dir_table[i].is_active = 1;
            strncpy(dir_table[i].name, dirname, MAX_DIRNAME - 1);
            printf("[DIR] PID=%d created directory '%s' (dir_id=%d)\n",
                   pid, dirname, i);
            log_syscall(pid, "mkdir", i, 0);
            return i;
        }
    }
    printf("[ERROR] mkdir: directory table full\n");
    return -1;
}

void print_dir_table(void) {
    printf("\n=== DIRECTORY TABLE ===\n");
    int found = 0;
    for (int i = 0; i < MAX_DIRS; i++) {
        if (dir_table[i].is_active) {
            printf("  [dir_id=%d] '%s'  owner_pid=%d\n",
                   dir_table[i].dir_id,
                   dir_table[i].name,
                   dir_table[i].owner_pid);
            found++;
        }
    }
    if (!found) printf("  (empty)\n");
    printf("=======================\n");
}
