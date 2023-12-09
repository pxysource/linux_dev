/**
 * @file zynq7020_gpio_led.c
 * @author panxingyuan (panxingyuan1@163.com)
 * @brief led control demo.
 * @version 0.1
 * @date 2023-12-06
 *       Create this file.
 * @copyright Copyright (c) 2023
 * @note HW: 
 *          - zynq 7020(正点原子领航者开发板)
 *          - led: 核心板LED2(MIO0)
 *       OS: linux-xlnx-xilinx-v14.5.
 *       Toolchain: arm-xilinx-linux-gnueabi-gcc (Sourcery CodeBench Lite 2012.09-104) 4.7.2
 *                  (需安装Xilinx SDK 2013.1)
 * @ref "ug585-Zynq-7000-TRM.pdf/Appendix B Register Details/B.19 General Purpose I/O (gpio)".
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define GPIO_BASE 0xE000A000
#define GPIO_REGS_MAP_SIZE 1024

/*
 * MIO0
 */
#define BANK0_DIRM_REG_OFFSET 0x00000204
#define BANK0_OUTEN_REG_OFFSET 0x00000208
#define BANK0_OUTPUT_DATA_TEG_OFFSET 0x00000040

static void *gpio_base = NULL;
static volatile unsigned int *gpio_bank0_output_data_reg = NULL;

static int gpio_init()
{
    unsigned int value;
    volatile unsigned int *addr = NULL;

    int fd = open("/dev/mem", O_RDWR | O_DSYNC);
    if (fd == -1)
    {
        perror("open '/dev/mem' error!");
        return -1;
    }

    gpio_base = mmap(NULL, GPIO_REGS_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);
    if (gpio_base == NULL)
    {
        perror("mmap error!");
        close(fd);
        return -1;
    }
    
    gpio_bank0_output_data_reg = (volatile unsigned int *)((char *)gpio_base + BANK0_OUTPUT_DATA_TEG_OFFSET);

    /*
     * Set direction: output.
     */
    addr = (volatile unsigned int *)((char *)gpio_base + BANK0_DIRM_REG_OFFSET);
    value = *addr;
    value |= (1 << 0);
    *addr = value;
    
    /*
     * Set output enable.
     */
    addr = (volatile unsigned int *)((char *)gpio_base + BANK0_OUTEN_REG_OFFSET);
    value = *addr;
    value |= (1 << 0);
    *addr = value;

    close(fd);

    return 0;
}

static int gpio_cleanup()
{
    int ret = 0;

    if (gpio_base != NULL)
    {
        ret = munmap(gpio_base, GPIO_REGS_MAP_SIZE);
        if (ret)
        {
            // error
        }

        gpio_base = NULL;
    }

    gpio_bank0_output_data_reg = NULL;

    return 0;
}

static void gpio_pin0_output(unsigned int value)
{
    unsigned int cur_val = 0;

    if (!gpio_bank0_output_data_reg)
    {
        fprintf(stderr, "Error: reg is no mapped!\n");
        return;
    }

    cur_val = *gpio_bank0_output_data_reg;

    if (!value)
    {
        cur_val &= (~(1 << 0));
    }
    else
    {
        cur_val |= (1 << 0);
    }

    *gpio_bank0_output_data_reg = cur_val;
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
