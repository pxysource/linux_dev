/**
 * @file xenomai_userspace_irq.c
 * @author panxingyuan (panxingyuan1@163.com)
 * @brief Xenomai irq in userspace.
 * @version 0.1
 * @date 2023-12-08
 *       Create this file.
 * @copyright Copyright (c) 2023
 * @details Xenomai Interrupt management services API usage demo.
 * @note HW: 
 *          - zynq 7020(正点原子领航者开发板)
 *          - key: PS_KEY0(MIO12) 
 *       OS: linux-xlnx-xilinx-v14.5 + ipipe-core-3.8-arm-1.patch + xenomai-2.6.3
 *       LIB: xenomai-2.6.3
 *       Toolchain: arm-xilinx-linux-gnueabi-gcc (Sourcery CodeBench Lite 2012.09-104) 4.7.2
 *                  (需安装Xilinx SDK 2013.1)
 */

#include <sys/mman.h>
#include <native/task.h>
#include <native/intr.h>

#define IRQ_NUMBER 268 /* GPIO 20(52 - 32). */
#define TASK_PRIO  99  /* Highest RT priority */
#define TASK_MODE  0   /* No flags */
#define TASK_STKSZ 0   /* Stack size (use default one) */

RT_INTR intr_desc;
RT_TASK server_desc;

void irq_server (void *cookie)
{
    int err = 0;
    int cnt = 0;

    printf("irq_server start.\n");

    printf("rt_intr_enable(), enable irq ...\n");
    err = rt_intr_enable(&intr_desc);
    if (err)
    {
        fprintf(stderr, "Error: rt_intr_enable(), ret=%d !\n", err);
        return;
    }

    printf("rt_intr_enable(), OK.\n");

    while (1)
    {
        cnt = rt_intr_wait(&intr_desc,TM_INFINITE);

        if (cnt < 0)
        {
            fprintf(stderr, "Error:ret=%d !\n", err);
        }

        printf("int cnt: %d.\n", cnt);
    }
}

void cleanup (void)
{
    rt_intr_disable(&intr_desc);
    rt_intr_delete(&intr_desc);
    rt_task_delete(&server_desc);
}

int main (int argc, char *argv[])
{
    int err;
    RT_TASK main_task;

    mlockall(MCL_CURRENT|MCL_FUTURE);

    err = rt_task_shadow(&main_task, "main", 0, 0);
    if (err)
    {
        fprintf(stderr, "Error: rt_task_shadow(), ret=%d !\n", err);
        return -1;
    }

    printf("Request for irq: %u", IRQ_NUMBER);
    err = rt_intr_create(&intr_desc, "gpio12_irq", IRQ_NUMBER, 0);
    if (err)
    {
        fprintf(stderr, "Error: rt_intr_create(), ret=%d !\n", err);
        return -1;
    }

    err = rt_task_create(&server_desc,
            "MyIrqServer",
            TASK_STKSZ,
            TASK_PRIO,
            TASK_MODE);
    if (err)
    {
        fprintf(stderr, "Error: rt_task_create(), ret=%d !\n", err);
        rt_intr_delete(&intr_desc);
        return -1;
    }

    err = rt_task_start(&server_desc, &irq_server, NULL);
    if (err)
    {
        fprintf(stderr, "Error: rt_task_start(), ret=%d !\n", err);
        rt_intr_delete(&intr_desc);
        rt_task_delete(&server_desc);
        return -1;
    }

    pause();
    cleanup();
    munlockall();

    return 0;
}

