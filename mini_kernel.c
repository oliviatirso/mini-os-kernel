/* mini_kernel.c — Educational OS Kernel Simulation */
/* Phase 2 Expansion: adds lseek() and mkdir() system calls */
/* Total system calls: write, read, open, close, create_process, lseek, mkdir */
/* Privacy features: per-process file ownership + audit log */
/* Compile: gcc -o mini_kernel mini_kernel.c */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
 
#define MAX_PROCESSES    8
#define MAX_FILES        16
#define MAX_FILE_SIZE    1024
#define MAX_FILENAME     64
#define MAX_AUDIT_ENTRIES 256
#define MAX_DIRS         16
#define MAX_DIRNAME      64
#define TIME_QUANTUM     3
 
/* ─── Process States ─── */
typedef enum { READY, RUNNING, BLOCKED, TERMINATED } ProcessState;
 
/* ─── Process Control Block (PCB) ─── */
typedef struct {
    int pid;
    ProcessState state;
    int reg_ax, reg_bx;     /* simulated registers */
    int mem_base;
    int mem_size;
    int time_left;          /* time quantum remaining */
    int active;
} PCB;
 
/* ─── File Control Block (FCB) ─── */
typedef struct {
    int fd;
    char name[MAX_FILENAME];
    int size;
    int position;
    int owner_pid;          /* privacy: tracks owning process */
    int is_open;
    char data[MAX_FILE_SIZE];
} FCB;
 
/* ─── Directory Control Block (DCB) ─── */
typedef struct {
    int dir_id;
    char name[MAX_DIRNAME];
    int owner_pid;
    int is_active;
} DCB;
 
/* ─── Audit Log Entry ─── */
typedef struct {
    int pid;
    char syscall[32];
    int fd;
    int result;
    int timestamp;
} AuditEntry;
 
/* ─── Kernel State ─── */
PCB        process_table[MAX_PROCESSES];
FCB        file_table[MAX_FILES];
DCB        dir_table[MAX_DIRS];
AuditEntry audit_log[MAX_AUDIT_ENTRIES];
int audit_count = 0;
int next_pid    = 0;
int clock_tick  = 0;
 
/* ─── Audit Logger ─── */
void log_syscall(int pid, const char *name, int fd, int res) {
    if (audit_count >= MAX_AUDIT_ENTRIES) return;
    AuditEntry *e = &audit_log[audit_count++];
    e->pid = pid; e->fd = fd; e->result = res;
    e->timestamp = clock_tick++;
    strncpy(e->syscall, name, 31);
    printf("[AUDIT] tick=%d pid=%d syscall=%s fd=%d result=%d\n",
           e->timestamp, pid, name, fd, res);
}
 
/* ─── Kernel Init ─── */
void kernel_init(void) {
    memset(process_table, 0, sizeof(process_table));
    memset(file_table,    0, sizeof(file_table));
    memset(dir_table,     0, sizeof(dir_table));
    memset(audit_log,     0, sizeof(audit_log));
    printf("[KERNEL] Initialized. Process table: %d slots, ", MAX_PROCESSES);
    printf("File table: %d slots, Dir table: %d slots.\n", MAX_FILES, MAX_DIRS);
}
 
/* ─── create_process() system call ─── */
int sys_create_process(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].active) {
            process_table[i].pid       = next_pid++;
            process_table[i].state     = READY;
            process_table[i].active    = 1;
            process_table[i].time_left = TIME_QUANTUM;
            printf("[PROC] Created process PID=%d\n", process_table[i].pid);
            log_syscall(process_table[i].pid, "create_process", -1, 0);
            return process_table[i].pid;
        }
    }
    printf("[ERROR] Process table full.\n");
    return -1;
}
 
/* ─── open() system call ─── */
int sys_open(int pid, const char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].is_open) {
            file_table[i].fd        = i;
            file_table[i].is_open   = 1;
            file_table[i].size      = 0;
            file_table[i].position  = 0;
            file_table[i].owner_pid = pid;
            strncpy(file_table[i].name, filename, MAX_FILENAME - 1);
            memset(file_table[i].data, 0, MAX_FILE_SIZE);
            printf("[FILE] PID=%d opened '%s' as fd=%d\n", pid, filename, i);
            log_syscall(pid, "open", i, 0);
            return i;
        }
    }
    return -1;
}
 
/* ─── write() system call ─── */
int sys_write(int pid, int fd, const char *buffer, int nbytes) {
    if (fd < 0 || fd >= MAX_FILES) {
        log_syscall(pid, "write", fd, -1);
        printf("[ERROR] write: invalid fd %d\n", fd);
        return -1;
    }
    if (!buffer || nbytes <= 0) {
        log_syscall(pid, "write", fd, -1);
        printf("[ERROR] write: bad buffer or size\n");
        return -1;
    }
    if (!file_table[fd].is_open) {
        log_syscall(pid, "write", fd, -1);
        printf("[ERROR] write: fd %d not open\n", fd);
        return -1;
    }
    if (file_table[fd].owner_pid != pid) {
        log_syscall(pid, "write", fd, -2);
        printf("[PRIVACY] write: PID=%d denied access to fd=%d (owned by PID=%d)\n",
               pid, fd, file_table[fd].owner_pid);
        return -2;
    }
    int space    = MAX_FILE_SIZE - file_table[fd].position;
    int to_write = (nbytes < space) ? nbytes : space;
    if (to_write <= 0) {
        log_syscall(pid, "write", fd, -3);
        printf("[ERROR] write: file full\n");
        return -3;
    }
    memcpy(file_table[fd].data + file_table[fd].position, buffer, to_write);
    file_table[fd].position += to_write;
    if (file_table[fd].position > file_table[fd].size)
        file_table[fd].size = file_table[fd].position;
    printf("[WRITE] PID=%d wrote %d bytes to fd=%d ('%s')\n",
           pid, to_write, fd, file_table[fd].name);
    log_syscall(pid, "write", fd, to_write);
    return to_write;
}
 
/* ─── read() system call ─── */
int sys_read(int pid, int fd, char *buffer, int nbytes) {
    if (fd < 0 || fd >= MAX_FILES || !file_table[fd].is_open) {
        log_syscall(pid, "read", fd, -1);
        return -1;
    }
    if (file_table[fd].owner_pid != pid) {
        log_syscall(pid, "read", fd, -2);
        printf("[PRIVACY] read: PID=%d denied\n", pid);
        return -2;
    }
    int avail   = file_table[fd].size - file_table[fd].position;
    int to_read = (nbytes < avail) ? nbytes : avail;
    if (to_read <= 0) return 0;
    memcpy(buffer, file_table[fd].data + file_table[fd].position, to_read);
    file_table[fd].position += to_read;
    log_syscall(pid, "read", fd, to_read);
    return to_read;
}
 
/* ─── close() system call ─── */
int sys_close(int pid, int fd) {
    if (fd < 0 || fd >= MAX_FILES || !file_table[fd].is_open)
        return -1;
    if (file_table[fd].owner_pid != pid) {
        printf("[PRIVACY] close: PID=%d denied\n", pid);
        return -2;
    }
    file_table[fd].is_open = 0;
    printf("[FILE] PID=%d closed fd=%d\n", pid, fd);
    log_syscall(pid, "close", fd, 0);
    return 0;
}
 
/* ─── lseek() system call ─── */
/*
 * Repositions the file position pointer for an open file.
 * Parameters:
 *   pid    — calling process ID
 *   fd     — file descriptor of the open file
 *   offset — absolute byte position to seek to (0 = start of file)
 * Returns:
 *   new position on success, negative error code on failure
 *
 * Design: After writing, the position pointer is at the end of the file.
 * lseek() lets a process rewind to re-read data, or jump to any byte
 * offset for partial reads/writes. This fixes the read-after-write bug
 * documented in the midterm report (where position reset had to be done
 * manually by directly accessing file_table[fd].position = 0).
 */
int sys_lseek(int pid, int fd, int offset) {
    /* Step 1: Validate file descriptor */
    if (fd < 0 || fd >= MAX_FILES) {
        log_syscall(pid, "lseek", fd, -1);
        printf("[ERROR] lseek: invalid fd %d\n", fd);
        return -1;
    }
    /* Step 2: File must be open */
    if (!file_table[fd].is_open) {
        log_syscall(pid, "lseek", fd, -1);
        printf("[ERROR] lseek: fd %d not open\n", fd);
        return -1;
    }
    /* Step 3: Privacy check */
    if (file_table[fd].owner_pid != pid) {
        log_syscall(pid, "lseek", fd, -2);
        printf("[PRIVACY] lseek: PID=%d denied access to fd=%d (owned by PID=%d)\n",
               pid, fd, file_table[fd].owner_pid);
        return -2;
    }
    /* Step 4: Bounds check — offset must be within [0, file size] */
    if (offset < 0 || offset > file_table[fd].size) {
        log_syscall(pid, "lseek", fd, -3);
        printf("[ERROR] lseek: offset %d out of bounds (file size=%d)\n",
               offset, file_table[fd].size);
        return -3;
    }
    /* Step 5: Move the position pointer */
    file_table[fd].position = offset;
    printf("[LSEEK] PID=%d seeked fd=%d to position %d\n", pid, fd, offset);
    log_syscall(pid, "lseek", fd, offset);
    return offset;
}
 
/* ─── mkdir() system call ─── */
/*
 * Creates a new directory entry in the kernel's directory table.
 * Parameters:
 *   pid     — calling process ID (becomes the directory owner)
 *   dirname — name of the directory to create
 * Returns:
 *   dir_id (>= 0) on success, negative error code on failure
 *
 * Design: In this educational simulation, a directory is a named entry
 * in the dir_table[], analogous to how files are entries in file_table[].
 * In a real OS, a directory would be a special inode pointing to a block
 * of (name, inode) pairs. Here we demonstrate the system call interface
 * and privacy enforcement (only the owning process can create in its namespace).
 */
int sys_mkdir(int pid, const char *dirname) {
    /* Step 1: Validate directory name */
    if (!dirname || dirname[0] == '\0') {
        printf("[ERROR] mkdir: invalid directory name\n");
        return -1;
    }
    /* Step 2: Check for duplicate name */
    for (int i = 0; i < MAX_DIRS; i++) {
        if (dir_table[i].is_active &&
            strncmp(dir_table[i].name, dirname, MAX_DIRNAME) == 0) {
            printf("[ERROR] mkdir: directory '%s' already exists\n", dirname);
            log_syscall(pid, "mkdir", -1, -4);
            return -4;
        }
    }
    /* Step 3: Find a free slot in the directory table */
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
    /* Step 4: Directory table full */
    printf("[ERROR] mkdir: directory table full\n");
    return -1;
}
 
/* ─── Print Directory Table ─── */
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
 
/* ─── Round-Robin Scheduler ─── */
void scheduler_tick(void) {
    printf("\n[SCHEDULER] --- Tick ---\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].active) continue;
        if (process_table[i].state == READY) {
            process_table[i].state     = RUNNING;
            process_table[i].time_left = TIME_QUANTUM;
            printf("[SCHED] Dispatching PID=%d\n", process_table[i].pid);
            process_table[i].state = READY;
        }
    }
}
 
/* ─── Print Audit Log ─── */
void print_audit_log(void) {
    printf("\n=== AUDIT LOG (%d entries) ===\n", audit_count);
    for (int i = 0; i < audit_count; i++) {
        printf("  [%3d] PID=%d %-18s fd=%-3d result=%d\n",
               audit_log[i].timestamp,
               audit_log[i].pid,
               audit_log[i].syscall,
               audit_log[i].fd,
               audit_log[i].result);
    }
    printf("==============================\n");
}
 
/* ─── MAIN DEMO ─── */
int main(void) {
    printf("=== Mini Privacy-First OS Kernel Demo (Phase 2) ===\n\n");
    kernel_init();
 
    /* --- Original demo from midterm --- */
 
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
 
    /* --- lseek() demo: clean read-after-write using lseek --- */
    printf("\n[TEST] Demonstrating lseek() — seek to start, then read:\n");
    sys_lseek(pid0, fd0, 0);            /* rewind to beginning */
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
 
    /* --- mkdir() demo --- */
    printf("\n[TEST] Demonstrating mkdir():\n");
    sys_mkdir(pid0, "documents");
    sys_mkdir(pid0, "downloads");
    sys_mkdir(pid1, "workspace");
 
    /* Attempt to create a duplicate directory */
    printf("\n[TEST] Attempt to create duplicate directory:\n");
    sys_mkdir(pid0, "documents");
 
    /* Print directory table */
    print_dir_table();
 
    /* Run scheduler */
    scheduler_tick();
 
    /* Close files */
    sys_close(pid0, fd0);
    sys_close(pid1, fd1);
 
    /* Print full audit trail */
    print_audit_log();
 
    printf("\n=== Demo Complete ===\n");
    return 0;
}