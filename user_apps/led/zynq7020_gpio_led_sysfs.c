/**
 * @file zynq7020_gpio_led_sysfs.c
 * @author panxingyuan (panxingyuan1@163.com)
 * @brief led control demo. (sysfs)
 * @version 0.1
 * @date 2023-12-06
 *       Create this file.
 * @copyright Copyright (c) 2023
 * @note HW: 
 *          - zynq 7020(正点原子领航者开发板)
 *          - led: 核心板LED2(MIO0)
 *       OS: linux-xlnx-xilinx-v14.5
 *       Toolchain: arm-xilinx-linux-gnueabi-gcc (Sourcery CodeBench Lite 2012.09-104) 4.7.2
 *                  (需安装Xilinx SDK 2013.1)
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FILE_PATH_GPIO_EXPORT "/sys/class/gpio/export"
#define FILE_PATH_GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define FILE_PATH_GPIO_PIN0_DIRECTION "/sys/class/gpio/gpio0/direction"
#define FILE_PATH_GPIO_PIN0_VALUE "/sys/class/gpio/gpio0/value"

static int gpio_pin0_value_fd = -1;

static int gpio_init()
{
    int exportfd = -1;
    int gpio_pin0_dir_fd = -1;
    int ret = 0;

    exportfd = open(FILE_PATH_GPIO_EXPORT, O_WRONLY);
    if (exportfd == -1)
    {
        perror("open " FILE_PATH_GPIO_EXPORT " error!");
        goto err;
    }

    ret = write(exportfd, "0", 2);
    if (ret == -1)
    {
        perror("write " FILE_PATH_GPIO_EXPORT " error!");
        goto err;
    }

    gpio_pin0_dir_fd = open(FILE_PATH_GPIO_PIN0_DIRECTION, O_RDWR);
    if (gpio_pin0_dir_fd == -1)
    {
        perror("open " FILE_PATH_GPIO_PIN0_DIRECTION " error!");
        goto err;
    }

    ret = write(gpio_pin0_dir_fd, "out", 4);
    if (ret == -1)
    {
        perror("write " FILE_PATH_GPIO_PIN0_DIRECTION " error!");
        goto err;
    }

    gpio_pin0_value_fd = open(FILE_PATH_GPIO_PIN0_VALUE, O_RDWR);
    if (gpio_pin0_value_fd == -1)
    {
        perror("open " FILE_PATH_GPIO_PIN0_VALUE " error!");
        goto err;
    }

    close(exportfd);
    close(gpio_pin0_dir_fd);

    return 0;

err:
    if (exportfd != -1)
    {
        close(exportfd);
    }
    if (gpio_pin0_dir_fd != -1)
    {
        close(gpio_pin0_dir_fd);
    }
    
    return -1;
}

static int gpio_cleanup()
{
    int ret = 0;
    int unexportfd = -1;
    
    if (gpio_pin0_value_fd != -1)
    {
        close(gpio_pin0_value_fd);
        gpio_pin0_value_fd = -1;
    }

    unexportfd = open(FILE_PATH_GPIO_UNEXPORT, O_WRONLY);
    if (unexportfd == -1)
    {
        perror("open " FILE_PATH_GPIO_UNEXPORT " error!");
        return -1;
    }

    ret = write(unexportfd, "0", 2);
    if (ret == -1)
    {
        perror("write " FILE_PATH_GPIO_UNEXPORT " error!");
        close(unexportfd);
        return -1;
    }
    
    close(unexportfd);

    return 0;
}

static void gpio_pin0_output(unsigned int value)
{
    int ret = 0;

    if (gpio_pin0_value_fd == -1)
    {
        fprintf(stderr, "Error: gpio output error!\n");
        return;
    }

    ret = write(gpio_pin0_value_fd, value == 0 ? "0" : "1", 2);
    if (ret == -1)
    {
        perror("write " FILE_PATH_GPIO_PIN0_VALUE " error!");
    }
}

int main()
{
    int i = 0;

    gpio_init();

    while (i++ < 10)
    {
        sleep(1);
        gpio_pin0_output(0);
        sleep(1);
        gpio_pin0_output(1);
    }

    gpio_cleanup();

    return 0;
}
