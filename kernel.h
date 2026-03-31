/* kernel.h — Shared types, constants, and declarations */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES     8
#define MAX_FILES         16
#define MAX_FILE_SIZE     1024
#define MAX_FILENAME      64
#define MAX_AUDIT_ENTRIES 256
#define MAX_DIRS          16
#define MAX_DIRNAME       64
#define TIME_QUANTUM      3

/* ─── Process States ─── */
typedef enum { READY, RUNNING, BLOCKED, TERMINATED } ProcessState;

/* ─── Process Control Block (PCB) ─── */
typedef struct {
    int pid;
    ProcessState state;
    int reg_ax, reg_bx;
    int mem_base;
    int mem_size;
    int time_left;
    int active;
} PCB;

/* ─── File Control Block (FCB) ─── */
typedef struct {
    int fd;
    char name[MAX_FILENAME];
    int size;
    int position;
    int owner_pid;
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

/* ─── Global Kernel State ─── */
extern PCB        process_table[MAX_PROCESSES];
extern FCB        file_table[MAX_FILES];
extern DCB        dir_table[MAX_DIRS];
extern AuditEntry audit_log[MAX_AUDIT_ENTRIES];
extern int        audit_count;
extern int        next_pid;
extern int        clock_tick;

/* ─── Function Prototypes ─── */

/* audit.c */
void log_syscall(int pid, const char *name, int fd, int res);
void print_audit_log(void);

/* process.c */
int  sys_create_process(void);
void scheduler_tick(void);

/* filesystem.c */
int sys_open(int pid, const char *filename);
int sys_write(int pid, int fd, const char *buffer, int nbytes);
int sys_read(int pid, int fd, char *buffer, int nbytes);
int sys_close(int pid, int fd);
int sys_lseek(int pid, int fd, int offset);

/* directory.c */
int  sys_mkdir(int pid, const char *dirname);
void print_dir_table(void);

/* kernel.c */
void kernel_init(void);

#endif /* KERNEL_H */
