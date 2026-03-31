CC     = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = mini_kernel
SRCS   = kernel.c audit.c process.c filesystem.c directory.c
OBJS   = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c kernel.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
