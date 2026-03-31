/* filesystem.c — File system calls: open, write, read, close, lseek */

#include "kernel.h"

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

/*
 * Repositions the file position pointer for an open file.
 * Parameters:
 *   pid    — calling process ID
 *   fd     — file descriptor of the open file
 *   offset — absolute byte position to seek to (0 = start of file)
 * Returns:
 *   new position on success, negative error code on failure
 */
int sys_lseek(int pid, int fd, int offset) {
    if (fd < 0 || fd >= MAX_FILES) {
        log_syscall(pid, "lseek", fd, -1);
        printf("[ERROR] lseek: invalid fd %d\n", fd);
        return -1;
    }
    if (!file_table[fd].is_open) {
        log_syscall(pid, "lseek", fd, -1);
        printf("[ERROR] lseek: fd %d not open\n", fd);
        return -1;
    }
    if (file_table[fd].owner_pid != pid) {
        log_syscall(pid, "lseek", fd, -2);
        printf("[PRIVACY] lseek: PID=%d denied access to fd=%d (owned by PID=%d)\n",
               pid, fd, file_table[fd].owner_pid);
        return -2;
    }
    if (offset < 0 || offset > file_table[fd].size) {
        log_syscall(pid, "lseek", fd, -3);
        printf("[ERROR] lseek: offset %d out of bounds (file size=%d)\n",
               offset, file_table[fd].size);
        return -3;
    }
    file_table[fd].position = offset;
    printf("[LSEEK] PID=%d seeked fd=%d to position %d\n", pid, fd, offset);
    log_syscall(pid, "lseek", fd, offset);
    return offset;
}
