// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"

static msg_channel_t** device_msg_channels;     // pointer to array of pointer to msg_slots
static unsigned int num_of_msg_channels;        // number of msg channels
static msg_channel_t* current_msg_channel;      // the pointer to our current msg slot
                                                // Invariant: if current_msg_channel == NULL => device_msg_channels == NULL

//================== DEVICE FUNCTIONS ===========================

static int device_open( struct inode* inode,
                        struct file*  file )
{
    printk("MSG SLOT: Invoking device_open(%p)\n", file);
    printk("MSG SLOT: current_msg_channel->num_of_used_bytes %u\n", current_msg_channel != NULL ? current_msg_channel->num_of_used_bytes : 0);
    printk("MSG SLOT: num_of_msg_channels%u\n", num_of_msg_channels);
    return SUCCESS;
}


static int device_release( struct inode* inode,
                           struct file*  file)
{
    printk("MSG SLOT: Invoking device_release(%p,%p)\n", inode, file);
    printk("MSG SLOT: current_msg_channel->num_of_used_bytes %u\n", current_msg_channel != NULL ? current_msg_channel->num_of_used_bytes : 0);
    printk("MSG SLOT: num_of_msg_channels%u\n", num_of_msg_channels);
    return SUCCESS;
}


static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    printk("MSG SLOT: Invoking device_read\n");
    if (current_msg_channel == NULL) {
        pr_err("MSG SLOT: current_msg_channel == NULL\n");
        // errno = EINVAL;
        return -1;
    }
    printk("MSG SLOT: current_msg_channel->id %lu", current_msg_channel->id);
    if (current_msg_channel->num_of_used_bytes == 0) {
        pr_err("MSG SLOT: current_msg_channel->num_of_used_bytes == 0\n");
        // errno = EWOULDBLOCK;
        return -1;
    }
    if (current_msg_channel->num_of_used_bytes > length) {
        pr_err("MSG SLOT: current_msg_channel->num_of_used_bytes > length\n");
        // errno = ENOSPC;
        return -1;
    }
    if (copy_to_user(buffer, current_msg_channel->msg, current_msg_channel->num_of_used_bytes)) {
        pr_err("MSG SLOT: Failed to copy from user\n");
        // errno = EFAULT;
        return -1;
    }
    return current_msg_channel->num_of_used_bytes;
}


static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    ssize_t i;
    int ret;
    printk("MSG SLOT: Invoking device_write(%p ,%ld)\n", file, length);
    if (device_msg_channels == NULL) {
        // errno = EINVAL;
        return -1;
    }
    if (!length || length > BUF_LEN) {
        // errno = EMSGSIZE;
        return -1;
    }
    for(i = 0; i < length; i++) {
        ret = get_user(current_msg_channel->msg[i], &buffer[i]);
        if(ret) {
            // errno = -ret;  // Convert to a positive value
            return -1;
        }
    }
    current_msg_channel->num_of_used_bytes = i;
    printk("MSG SLOT: Successfully wrote %u bytes to channel %lu.\n", current_msg_channel->num_of_used_bytes, current_msg_channel->id);
    // return the number of input characters succeeded
    return i;
}

static msg_channel_t* msg_channel_of(unsigned long requested_msg_channel_id) {
    int i;
    printk("MSG SLOT: Number of channels is %u.\n", num_of_msg_channels);
    for (i = 0; i < num_of_msg_channels; i++) {
        if (device_msg_channels[i]->id == requested_msg_channel_id) {
             // the requested msg channel exists, his index is i
            return device_msg_channels[i];
        }
    }
    printk("MSG SLOT: Didn't find the requested channel.\n");
    return NULL;
}

static msg_channel_t* create_channel_if_needed_of(unsigned long requested_msg_channel_id) {
    msg_channel_t* found_msg_channel;
    // First case: it is our first ioctl
    if (device_msg_channels == 0) {
        printk("MSG SLOT: no previous channels.\n");
        // set device_msg_channels to be an array of pointers. device_msg_channels[0] will be our current msg slot
        device_msg_channels = kzalloc(sizeof(msg_channel_t*), GFP_KERNEL);
        if (device_msg_channels == NULL) {
            pr_err("MSG SLOT: kzalloc failed.\n");
            return NULL;
        }
        device_msg_channels[0] = kzalloc(sizeof(msg_channel_t), GFP_KERNEL);
        if (device_msg_channels[0] == NULL) {
            kfree(device_msg_channels);
            pr_err("MSG SLOT: kzalloc failed.\n");
            return NULL;
        }
        device_msg_channels[0]->id = requested_msg_channel_id;
        printk("MSG SLOT: Created one channel.\n");
        return device_msg_channels[0];
    }
    // We know that there where channels in use. Check if msg channel already exists
    printk("MSG SLOT: We used channels in the past. Checking if we had the channel requested.\n");
    found_msg_channel = msg_channel_of(requested_msg_channel_id);
    if (found_msg_channel) {
        printk("MSG SLOT: Found the requested channel.\n");
        return found_msg_channel;
    }

    // New msg channel needed.
    // Let's create one and set a pointer to it at the and of device_msg_channels, which is an array of pointers to channels
    num_of_msg_channels++;
    device_msg_channels = krealloc(device_msg_channels, num_of_msg_channels * sizeof(msg_channel_t*), GFP_KERNEL);
    if (device_msg_channels == NULL) {
        pr_err("MSG SLOT: realloc failed.\n");
        num_of_msg_channels--;
        return NULL;
    }
    device_msg_channels[num_of_msg_channels - 1] = kzalloc(sizeof(msg_channel_t), GFP_KERNEL);
    if (device_msg_channels[num_of_msg_channels - 1] == NULL) {
        num_of_msg_channels--;
        pr_err("MSG SLOT: kzalloc failed.\n");
        return NULL;
    }
    device_msg_channels[num_of_msg_channels - 1]->id = requested_msg_channel_id;
    printk("MSG SLOT: Created the requested channel Number of channels: %u.\n", num_of_msg_channels);
    return device_msg_channels[num_of_msg_channels - 1];
}

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    // Switch according to the ioctl called
    if(ioctl_command_id != MSG_SLOT_CHANNEL || !ioctl_param /* Cannot be 0 */) {
        // errno = EINVAL;
        return -1;
    }
    // Get the parameter given to ioctl by the process
    printk("MSG SLOT: Invoking ioctl(%ld)\n", ioctl_param);
    current_msg_channel = create_channel_if_needed_of(ioctl_param);
    return current_msg_channel != NULL ? SUCCESS : FAIL;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
    .owner	  = THIS_MODULE,
    .read           = device_read,
    .write          = device_write,
    .open           = device_open,
    .unlocked_ioctl = device_ioctl,
    .release        = device_release,
};

// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if( rc < 0 ) {
        printk(KERN_ALERT "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }
    printk("MSG SLOT: registration worked!\n");
    return 0;
}

static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
