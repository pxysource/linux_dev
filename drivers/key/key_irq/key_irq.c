/**
 * @file key_irq.c
 * @author panxingyuan (panxingyuan1@163.com)
 * @brief Key irq driver.
 * @version 0.1
 * @date 2023-12-08
 *       Create this file.
 * @copyright Copyright (c) 2023
 * @note HW: 
 *          - zynq 7020(正点原子领航者开发板)
 *          - key: PS_KEY0(MIO12) 
 *       OS: linux-xlnx-xilinx-v14.5 + ipipe-core-3.8-arm-1.patch + xenomai-2.6.3
 *       Toolchain: arm-xilinx-linux-gnueabi-gcc (Sourcery CodeBench Lite 2012.09-104) 4.7.2
 *                  (需安装Xilinx SDK 2013.1)
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/irq.h>

#define KEY_CNT		1		/* 设备号个数 */
#define KEY_NAME	"key"	/* 名字 */

enum key_status {
	KEY_PRESS = 0,
	KEY_RELEASE,
	KEY_KEEP,		// 按键状态保持
};

/* 按键设备结构体 */
struct key_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev结构体 */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int key_gpio;			/* GPIO编号 */
	int irq_num;			/* 中断号 */
	struct timer_list timer;/* 定时器 */
	spinlock_t spinlock;	/* 自旋锁 */
};

static struct key_dev key;
static int status = KEY_KEEP;

static inline u32 irq_get_trigger_type(unsigned int irq)
{
    struct irq_data *d = irq_get_irq_data(irq);
    return d ? irqd_get_trigger_type(d) : 0;
}

static int key_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf,
			size_t cnt, loff_t *offt)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&key.spinlock, flags);

	ret = copy_to_user(buf, &status, sizeof(int));
	/* 状态重置 */
	status = KEY_KEEP;
	spin_unlock_irqrestore(&key.spinlock, flags);

	return ret;
}

static ssize_t key_write(struct file *filp, const char __user *buf,
			size_t cnt, loff_t *offt)
{
	return 0;
}

static int key_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static void key_timer_function(unsigned long arg)
{
	static int last_val = 1;
	unsigned long flags;
	int current_val;

	spin_lock_irqsave(&key.spinlock, flags);

	current_val = gpio_get_value(key.key_gpio);
	if (0 == current_val && last_val)	// 按下
		status = KEY_PRESS;
	else if (1 == current_val && !last_val)
		status = KEY_RELEASE;	// 松开
	else
		status = KEY_KEEP;		// 状态保持

	last_val = current_val;

	spin_unlock_irqrestore(&key.spinlock, flags);
}

static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	/* 按键防抖处理，开启定时器延时15ms */
	mod_timer(&key.timer, jiffies + msecs_to_jiffies(15));
	return IRQ_HANDLED;
}

static int key_parse_dt(void)
{
	struct device_node *nd;
	const char *str;
	int ret;

	nd = of_find_node_by_path("/key");
	if (!nd)
    {
		printk(KERN_ERR "key: Failed to get key node\n");
		return -EINVAL;
	}

	ret = of_property_read_string(nd, "status", &str);
	if(!ret)
    {
		if (strcmp(str, "okay"))
        {
            return -EINVAL;
        }
	}

	ret = of_property_read_string(nd, "compatible", &str);
	if (ret)
    {
        return ret;
    }

	if (strcmp(str, "alientek,key"))
    {
		printk(KERN_ERR "key: Compatible match failed\n");
		return -EINVAL;
	}

	/* 得到按键的GPIO编号 */
	key.key_gpio = of_get_named_gpio(nd, "key-gpio", 0);
	if (!gpio_is_valid(key.key_gpio))
    {
		printk(KERN_ERR "key: Failed to get key-gpio\n");
		return -EINVAL;
	}

	/* 获取GPIO对应的中断号 */
	key.irq_num = irq_of_parse_and_map(nd, 0);
	if (!key.irq_num)
    {
        return -EINVAL;
    }

	return 0;
}

static int key_gpio_init(void)
{
	unsigned long irq_flags;
	int ret;

	ret = gpio_request(key.key_gpio, "Key Gpio");
	if (ret)
    {
        return ret;
    }

	gpio_direction_input(key.key_gpio);

	/* 获取设备树中指定的中断触发类型 */
	irq_flags = irq_get_trigger_type(key.irq_num);
	if (IRQF_TRIGGER_NONE == irq_flags)
    {
        irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;
    }

	ret = request_irq(key.irq_num, key_interrupt, irq_flags, "PS Key0 IRQ", NULL);
	if (ret)
    {
		gpio_free(key.key_gpio);
		return ret;
	}

	return 0;
}

static struct file_operations key_fops = {
	.owner		= THIS_MODULE,
	.open		= key_open,
	.read		= key_read,
	.write		= key_write,
	.release	= key_release,
};

static int __init mykey_init(void)
{
	int ret;

	spin_lock_init(&key.spinlock);

	/* 设备树解析 */
	ret = key_parse_dt();
	if (ret)
    {
		return ret;
    }

	/* GPIO、中断初始化 */
	ret = key_gpio_init();
	if (ret)
    {
        return ret;
    }

	/* 初始化cdev */
	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &key_fops);

	/* 添加cdev */
	ret = alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
	if (ret)
		goto out1;

	ret = cdev_add(&key.cdev, key.devid, KEY_CNT);
	if (ret)
		goto out2;

	/* 创建类 */
	key.class = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(key.class)) {
		ret = PTR_ERR(key.class);
		goto out3;
	}

	/* 创建设备 */
	key.device = device_create(key.class, NULL,
				key.devid, NULL, KEY_NAME);
	if (IS_ERR(key.device)) {
		ret = PTR_ERR(key.device);
		goto out4;
	}

	init_timer(&key.timer);
	key.timer.function = key_timer_function;

	return 0;

out4:
	class_destroy(key.class);

out3:
	cdev_del(&key.cdev);

out2:
	unregister_chrdev_region(key.devid, KEY_CNT);

out1:
	free_irq(key.irq_num, NULL);
	gpio_free(key.key_gpio);

	return ret;
}

static void __exit mykey_exit(void)
{
	del_timer_sync(&key.timer);
	device_destroy(key.class, key.devid);
	class_destroy(key.class);
	cdev_del(&key.cdev);
	unregister_chrdev_region(key.devid, KEY_CNT);
	free_irq(key.irq_num, NULL);
	gpio_free(key.key_gpio);
}

module_init(mykey_init);
module_exit(mykey_exit);

MODULE_AUTHOR("panxingyuan1@163.com");
MODULE_DESCRIPTION("Key irq drv.");
MODULE_LICENSE("GPL");
