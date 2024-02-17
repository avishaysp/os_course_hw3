#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/ioctl.h>


MODULE_LICENSE("GPL");
#define MAJOR_NUM 235

struct chardev_info {
  spinlock_t lock;
};

// Set the message of the device driver
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 80
#define DEVICE_FILE_NAME "simple_char_dev"
#define SUCCESS 0

#endif
