/**
 * @file serial_rw.c
 * @author panxingyuan (panxingyuan1@163.com)
 * @brief Serial read/write.
 * @version 0.1
 * @date 2023-12-23
 *       Create this file.
 * @copyright Copyright (c) 2023
 * @note HW: 
 *          - zynq 7020(正点原子领航者开发板)
 *          - serial device: ttyPS0
 *       OS: linux-xlnx-xilinx-v14.5.
 *       Toolchain: arm-xilinx-linux-gnueabi-gcc (Sourcery CodeBench Lite 2012.09-104) 4.7.2
 *                  (需安装Xilinx SDK 2013.1)
 */

#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
#include <linux/serial.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define SERIAL_DEVICE_NAME "/dev/ttyPS0"

static int serial_init(char *tty_name, int *pfd)
{
    int fd = -1;
    struct termios config;

    if (!tty_name)
    {
        fprintf(stderr, "Error: Invalid argument, tty_name is null!\n");
        return -1;
    }

    if (!pfd)
    {
        fprintf(stderr, "Error: Invalid argument, pfd is null!\n");
        return -1;
    }

    errno = 0;
    fd = open(tty_name, O_RDWR | O_NOCTTY);
    if(-1 == fd)
    {
        fprintf(stderr, "Error: failed to open file <%s>, errno=%d!\n", tty_name, errno);
        return -1;
    }

    *pfd = fd;

    errno = 0;
    /*  Retrieve a termios structure containing a copy of the current settings. */
    if (tcgetattr(fd, &config))
    {
        fprintf(stderr, "Error: tcgetattr(), errno=%d!\n", errno);
        return -1;
    }

    /* IXON: Enable start/stop output flow control. */
    config.c_iflag &= ~IXON;

    /*
     * CSIZE: Character-size mask (5 to 8 bits: CS5, CS6, CS7, CS8).
     * PARENB: Parity enable.
     */
    config.c_cflag &= ~CSIZE;
    config.c_cflag |= CS8;
    config.c_cflag &= ~PARENB;

    errno = 0;
    if (cfsetspeed(&config, B115200))
    {
        fprintf(stderr, "Error: cfsetspeed(), errno=%d!\n", errno);
        return -1;
    }

    /*
     * CLOCAL: Ignore modem status lines (don’t check carrier signal).
     * CREAD: Allow input to be received.
     */
    config.c_cflag |= CLOCAL | CREAD;

    /* CSTOPB: Use 2 stop bits per character; otherwise 1. */
    config.c_cflag &= ~CSTOPB;

    /*
     * TIME=0,MIN=0:
     * If data is available at the time of the call, then read() returns immediately with the
     * lesser of the number of bytes available or the number of bytes requested. If no
     * bytes are available, read() completes immediately, returning 0.
     */
    config.c_cc[VTIME] = 0;
    config.c_cc[VMIN] = 0;

    /*
     * The tcflush() function flushes (discards) the data in the terminal input queue,
     * the terminal output queue, or both queues.
     */
    errno = 0;
    /* TCIFLUSH: Flush the input queue. */
    if (tcflush (fd, TCIFLUSH))
    {
        fprintf(stderr, "Error: tcflush(), TCIFLUSH, errno=%d!\n", errno);
        return -1;
    }
    errno = 0;
    /* TCOFLUSH: Flush the output queue. */
    if (tcflush (fd, TCOFLUSH))
    {
        fprintf(stderr, "Error: tcflush(), TCOFLUSH, errno=%d!\n", errno);
        return -1;
    }

    /* 
     * Push the updated structure back to the driver. 
     * TCSANOW: The change is carried out immediately.
     */
    errno = 0;
    if (tcsetattr(fd, TCSANOW, &config))
    {
        fprintf(stderr, "Error: tcsetattr(), errno=%d!\n", errno);
        return -1;
    }

    return 0;
}

static void serial_exit(int fd)
{
    if (fd != -1)
    {
        errno = 0;
        if (close(fd))
        {
            fprintf(stderr, "Error: close() failed, fd=%d, errno=%d!\n", fd, errno);
        }
    }
}

static int serial_read(int fd)
{
    fd_set rset;
    struct timeval tv;
    char buff[128] = {0};
    int to_read_len = sizeof(buff) - 1;
    int ret = 0;

    if (fd == -1)
    {
        fprintf(stderr, "Error: Invalid argument, fd!\n");
        return -1;
    }

    /* Add a file descriptor to the set */
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    /*
     * Timeout: 5s.
     */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    errno = 0;
    while ((ret = select(fd+1, &rset, NULL, NULL, &tv)) == -1)
    {
        if (errno == EINTR)
        {
            errno = 0;
            fprintf(stderr, "Warn: A non blocked signal was caught!\n");

            /* Necessary after an error */
            FD_ZERO(&rset);
            FD_SET(fd, &rset);
        }
        else
        {
            fprintf(stderr, "Error: select() failed, error=%d!\n", errno);
            return -1;
        }
    }

    if (!ret)
    {
        fprintf(stderr, "Error: Timedout, 5s!\n");
        return -1;
    }

    errno = 0;
    ret = read(fd, buff, to_read_len);
    if (ret > 0)
    {
        printf("read msg: %s, read len: %d, to read len: %d.\n", &buff[0], ret, to_read_len);
    }
    else if (ret == 0)
    {
        printf("No valid data.\n");
    }
    else
    {
        fprintf(stderr, "Error: read() failed, errno=%d!\n", errno);
        return -1;
    }

    return 0;
}

static int serial_write(int fd)
{
    char buff[32] = "Serial test.";
    int to_write_len = 0;
    int ret = 0;

    if (fd == -1)
    {
        fprintf(stderr, "Error: Invalid argument, fd!\n");
        return -1;
    }

    to_write_len = strlen(buff);
    errno = 0;
    ret = write(fd, buff, to_write_len);
    if (ret > 0)
    {
        printf("Write msg: %s, write len: %d, to write len: %d\n", &buff[0], ret, to_write_len);
    }
    else if (ret == 0)
    {
        printf("Nothing was written.\n");
    }
    else
    {
        fprintf(stderr, "Error: write() failed, errno=%d!\n", errno);
        return -1;
    }

    return 0;
}

int main()
{
    int fd = -1;

    printf("Seiral test, device:%s\n", SERIAL_DEVICE_NAME);

    if (serial_init(SERIAL_DEVICE_NAME, &fd))
    {
        fprintf(stderr, "Error: Serial init failed!\n");
        serial_exit(fd);
        return -1;
    }

    if (serial_read(fd))
    {
        fprintf(stderr, "Error: Serial read failed!\n");
        serial_exit(fd);
        return -1;
    }

    if (serial_write(fd))
    {
        fprintf(stderr, "Error: Serial write failed!\n");
        serial_exit(fd);
        return -1;
    }

    serial_exit(fd);

    return 0;
}
