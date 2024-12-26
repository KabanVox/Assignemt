#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>

#define GPIO_21 (21)

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static struct gpio_desc *gpio_desc;

/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t * off);
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t * off);
/******************************************************/

// File operation structure
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};

/*
** This function will be called when we open the Device file
*/ 
static int etx_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

/*
** This function will be called when we read the Device file
*/ 
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;

  // Reading GPIO value
  gpio_state = gpiod_get_value(gpio_desc);

  // Write to user
  len = 1;
  if (copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }

  pr_info("Read function: GPIO_21 = %d \n", gpio_state);

  return 0;
}

/*
** This function will be called when we write to the Device file
*/ 
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[2] = {0};  

  if (copy_from_user(rec_buf, buf, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
    return -EFAULT;  
  }

  pr_info("Write Function: GPIO_21 Set = %c\n", rec_buf[0]);

  if (rec_buf[0] == '1') {
    gpiod_set_value(gpio_desc, 1);  
    pr_info("GPIO_21 is ON\n");
  } else if (rec_buf[0] == '0') {
    gpiod_set_value(gpio_desc, 0);  
    pr_info("GPIO_21 is OFF\n");
  } else {
    pr_err("Unknown command: Please provide either 1 or 0\n");
    return -EINVAL;  
  }

  return len;  
}

/*
** Module Init function
*/ 
static int __init etx_driver_init(void)
{
  if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0) {
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

  cdev_init(&etx_cdev, &fops);

  if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }

  if (IS_ERR(dev_class = class_create("etx_class"))) {
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }

  if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "etx_device"))) {
    pr_err("Cannot create the Device \n");
    goto r_device;
  }

  if (gpio_is_valid(GPIO_21) == false) {
    pr_err("GPIO %d is not valid\n", GPIO_21);
    goto r_device;
  }

  gpio_desc = gpio_to_desc(GPIO_21);  
  if (IS_ERR(gpio_desc)) {
    pr_err("ERROR: GPIO %d request\n", GPIO_21);
    goto r_gpio;
  }

  gpiod_direction_output(gpio_desc, 0);

  pr_info("Device Driver Insert...Done!!!\n");
  return 0;

r_gpio:
  gpio_free(GPIO_21);
r_device:
  device_destroy(dev_class, dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&etx_cdev);
r_unreg:
  unregister_chrdev_region(dev, 1);

  return -1;
}

/*
** Module exit function
*/ 
static void __exit etx_driver_exit(void)
{
  gpiod_unexport(gpio_desc);
  gpio_free(GPIO_21);
  device_destroy(dev_class, dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Minh Anh");
MODULE_DESCRIPTION("Control LED - GPIO Driver");
MODULE_VERSION("1");
