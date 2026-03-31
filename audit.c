/* audit.c — Audit logging system calls */

#include "kernel.h"

void log_syscall(int pid, const char *name, int fd, int res) {
    if (audit_count >= MAX_AUDIT_ENTRIES) return;
    AuditEntry *e = &audit_log[audit_count++];
    e->pid = pid; e->fd = fd; e->result = res;
    e->timestamp = clock_tick++;
    strncpy(e->syscall, name, 31);
    printf("[AUDIT] tick=%d pid=%d syscall=%s fd=%d result=%d\n",
           e->timestamp, pid, name, fd, res);
}

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
