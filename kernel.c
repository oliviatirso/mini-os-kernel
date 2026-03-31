/* kernel.c — Kernel initialization, global state, and main demo */
/* Phase 2 Expansion: adds lseek() and mkdir() system calls         */
/* Compile: make  (or: gcc -o mini_kernel kernel.c audit.c process.c filesystem.c directory.c) */

#include "kernel.h"

/* ─── Global Kernel State (definitions) ─── */
PCB        process_table[MAX_PROCESSES];
FCB        file_table[MAX_FILES];
DCB        dir_table[MAX_DIRS];
AuditEntry audit_log[MAX_AUDIT_ENTRIES];
int        audit_count = 0;
int        next_pid    = 0;
int        clock_tick  = 0;

/* ─── Kernel Init ─── */
void kernel_init(void) {
    memset(process_table, 0, sizeof(process_table));
    memset(file_table,    0, sizeof(file_table));
    memset(dir_table,     0, sizeof(dir_table));
    memset(audit_log,     0, sizeof(audit_log));
    printf("[KERNEL] Initialized. Process table: %d slots, ", MAX_PROCESSES);
    printf("File table: %d slots, Dir table: %d slots.\n", MAX_FILES, MAX_DIRS);
}

/* ─── MAIN DEMO ─── */
int main(void) {
    printf("=== Mini Privacy-First OS Kernel Demo (Phase 2) ===\n\n");
    kernel_init();

    /* Create two processes */
    int pid0 = sys_create_process();
    int pid1 = sys_create_process();

    /* Process 0 opens a file and writes to it */
    int fd0 = sys_open(pid0, "notes.txt");
    sys_write(pid0, fd0, "Hello from process 0!", 21);

    /* Process 1 tries to write to process 0's file — should be denied */
    printf("\n[TEST] Process 1 attempts unauthorized write:\n");
    sys_write(pid1, fd0, "Unauthorized data", 17);

    /* Process 1 opens its own file */
    int fd1 = sys_open(pid1, "secret.txt");
    sys_write(pid1, fd1, "Private data of PID1", 20);

    /* lseek() demo: clean read-after-write using lseek */
    printf("\n[TEST] Demonstrating lseek() — seek to start, then read:\n");
    sys_lseek(pid0, fd0, 0);
    char buf[128];
    memset(buf, 0, sizeof(buf));
    int n = sys_read(pid0, fd0, buf, 128);
    printf("[READ] PID=%d read %d bytes: '%s'\n", pid0, n, buf);

    /* lseek to middle of file (byte 6) and read from there */
    printf("\n[TEST] Seek to offset 6, read partial:\n");
    sys_lseek(pid0, fd0, 6);
    memset(buf, 0, sizeof(buf));
    n = sys_read(pid0, fd0, buf, 128);
    printf("[READ] PID=%d read %d bytes from offset 6: '%s'\n", pid0, n, buf);

    /* Process 1 tries to lseek process 0's file — should be denied */
    printf("\n[TEST] Process 1 attempts unauthorized lseek:\n");
    sys_lseek(pid1, fd0, 0);

    /* mkdir() demo */
    printf("\n[TEST] Demonstrating mkdir():\n");
    sys_mkdir(pid0, "documents");
    sys_mkdir(pid0, "downloads");
    sys_mkdir(pid1, "workspace");

    /* Attempt to create a duplicate directory */
    printf("\n[TEST] Attempt to create duplicate directory:\n");
    sys_mkdir(pid0, "documents");

    print_dir_table();
    scheduler_tick();

    sys_close(pid0, fd0);
    sys_close(pid1, fd1);

    print_audit_log();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
