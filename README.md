# Mini Privacy-First OS Kernel

A simulated OS kernel in C implementing process management, file I/O, directory management, and a privacy-focused audit logger.

## Build & Run

**Requirements:** GCC, Make

```bash
make          # compiles to ./mini_kernel
./mini_kernel # runs the demo
make clean    # removes object files and binary
```

## Project Structure

| File | Description |
|------|-------------|
| `kernel.h` | Shared types and system-call declarations |
| `kernel.c` | Entry point and system-call dispatch |
| `process.c` | Process management (`sys_fork`, `sys_exit`, `sys_getpid`) |
| `filesystem.c` | File I/O (`sys_open`, `sys_read`, `sys_write`, `sys_lseek`, `sys_close`) |
| `directory.c` | Directory management (`sys_mkdir`, `sys_rmdir`, `sys_listdir`) |
| `audit.c` | Privacy-first audit logger |

See [REPORT.md](REPORT.md) for full design documentation.
