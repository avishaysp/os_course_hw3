#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/ioctl.h>
#include <linux/errno.h>  // For error codes
#include <linux/err.h>    // For PTR_ERR and IS_ERR
#include <linux/slab.h>   // Include this for kmalloc, kzalloc, and krealloc
#include <linux/types.h>  // For kernel mode types


MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "simple_char_dev"
#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define SUCCESS 0
#define FAIL 0

typedef struct msg_channel_t {
  u64 id;  // we will go with 64 bits beacuse ioctl get a losg as a param. Doesn't cost much relative to the 128 chars of the msg
  u8 num_of_used_bytes;
  char msg[BUF_LEN];
}msg_channel_t;


#endif
