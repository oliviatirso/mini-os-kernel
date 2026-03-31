/* process.c — Process management and scheduler */

#include "kernel.h"

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
