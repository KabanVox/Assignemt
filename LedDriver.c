#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio/consumer.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define DEVICE_NAME "led_driver"
#define CLASS_NAME "led"

static struct gpio_desc *led_gpio;
static struct timer_list blink_timer;
static int blink_rate = 1000; // Default blink rate in ms
static struct class *led_class;
static struct cdev led_cdev;
static dev_t dev_number;

static void timer_callback(struct timer_list *t)
{
    static bool led_state = false;

    led_state = !led_state; // Toggle LED state
    gpiod_set_value(led_gpio, led_state);

    mod_timer(&blink_timer, jiffies + msecs_to_jiffies(blink_rate));
}

static ssize_t blink_rate_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    char kbuf[16];
    if (len > 15)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';
    if (kstrtoint(kbuf, 10, &blink_rate))
        return -EINVAL;

    if (blink_rate < 100)
        blink_rate = 100;

    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = blink_rate_write,
};

static int __init led_driver_init(void)
{
    int ret;

    // Allocate a device number
    if (alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME) < 0)
        return -1;

    // Create device class
    led_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(led_class)) {
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(led_class);
    }

    // Initialize and add character device
    cdev_init(&led_cdev, &fops);
    if (cdev_add(&led_cdev, dev_number, 1) < 0) {
        class_destroy(led_class);
        unregister_chrdev_region(dev_number, 1);
        return -1;
    }

    device_create(led_class, NULL, dev_number, NULL, DEVICE_NAME);

    // Initialize GPIO
    led_gpio = gpiod_get(NULL, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led_gpio)) {
        cdev_del(&led_cdev);
        class_destroy(led_class);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(led_gpio);
    }

    // Initialize timer
    timer_setup(&blink_timer, timer_callback, 0);
    mod_timer(&blink_timer, jiffies + msecs_to_jiffies(blink_rate));

    pr_info("LED driver initialized.\n");
    return 0;
}

static void __exit led_driver_exit(void)
{
    del_timer(&blink_timer);
    gpiod_put(led_gpio);
    device_destroy(led_class, dev_number);
    class_destroy(led_class);
    cdev_del(&led_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("LED driver removed.\n");
}

module_init(led_driver_init);
module_exit(led_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("LED Driver using descriptor-based GPIO interface");
MODULE_VERSION("1.0");
