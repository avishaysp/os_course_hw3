#include "message_slot.h"

static msg_channel_t** device_msg_channels;     // pointer to array of pointer to msg_slots
static u32 num_of_msg_channels;            // number of msg channels
static msg_channel_t* current_msg_channel;      // the pointer to our current msg slot
                                                // Invariant: if current_msg_channel == NULL => device_msg_channels == NULL

//================== DEVICE FUNCTIONS ===========================

static int device_open( struct inode* inode,
                        struct file*  file )
{
     printk("MSG SLOT: Invoking device_open(%p)\n", file);
    device_msg_channels = NULL;
    num_of_msg_channels = 0;
    current_msg_channel = NULL;
    return SUCCESS;
}


static int device_release( struct inode* inode,
                           struct file*  file)
{
    printk("Invoking device_release(%p,%p)\n", inode, file);
    if (device_msg_channels == NULL) {
        return SUCCESS;
    }
    for (u32 i = 0; i < num_of_msg_channels; i++) {
        kfree(device_msg_channels[i]);
    }
    kfree(device_msg_channels);

    return SUCCESS;
}


static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    if (current_msg_channel == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (!current_msg_channel->num_of_used_bytes) {
        errno = EWOULDBLOCK;
        return -1;
    }
    if (current_msg_channel->num_of_used_bytes > length) {
        errno = ENOSPC;
        return -1;
    }
    if (copy_to_user(buffer, current_msg_channel->msg, current_msg_channel->num_of_used_bytes)) {
        errno = EFAULT;
        return -1;
    }
    return current_msg_channel->num_of_used_bytes;
}


static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    printk("Invoking device_write(%p,%ld)\n", file, length);

    if (device_msg_channels == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (!length || length > BUF_LEN) {
        errno = EMSGSIZE;
        return -1;
    }
    ssize_t i;
    for(i = 0; i < length; ++i) {
        int ret;
        if(ret = get_user(current_msg_channel->msg[i], &buffer[i])) {
            errno = -ret;  // Convert to a positive value
            return -1;
        }
    }
    // return the number of input characters succeeded
    return i;
}

static msg_channel_t* msg_channel_of(u64 requested_msg_channel_id) {
    for (int i = 0; i < num_of_msg_channels; i++) {
        if (device_msg_channels[i]->id == requested_msg_channel_id) {
             // the requested msg channel exists, his index is i
            return device_msg_channels[i];
        }
    }
    return NULL;
}

static msg_channel_t* create_channel_if_needed_of(u64 requested_msg_channel_id) {

    // First case: it is our first ioctl
    if (device_msg_channels == NULL) {
        // set device_msg_channels to be an array of pointers. device_msg_channels[0] will be our current msg slot
        device_msg_channels = kzalloc(sizeof(msg_channel_t*), GFP_KERNEL);
        if (device_msg_channels == NULL) {
            pr_err("kzalloc failed.\n");
            return NULL;
        }
        device_msg_channels[0] = kzalloc(sizeof(msg_channel_t), GFP_KERNEL);
        if (device_msg_channels[0] == NULL) {
            kfree(device_msg_channels);
            pr_err("kzalloc failed.\n");
            return NULL;
        }
        device_msg_channels[0]->id = requested_msg_channel_id;
        return device_msg_channels[0];
    }
    // We know that there where channels in use. Check if msg channel already exists
    msg_channel_t* found_msg_channel = msg_channel_of(requested_msg_channel_id);
    if (found_msg_channel) {
        return found_msg_channel;
    }

    // New msg channel needed.
    // Let's create one and set a pointer to it at the and of device_msg_channels, which is an array of pointers to channels
    num_of_msg_channels++;
    device_msg_channels = krealloc(device_msg_channels, num_of_msg_channels * sizeof(msg_channel_t*), GFP_KERNEL);
    if (device_msg_channels == NULL) {
        pr_err("realloc failed.\n");
        num_of_msg_channels--;
        return NULL;
    }
    device_msg_channels[num_of_msg_channels - 1] = kzalloc(sizeof(msg_channel_t), GFP_KERNEL);
    if (device_msg_channels[num_of_msg_channels - 1] == NULL) {
        num_of_msg_channels--;
        pr_err("kzalloc failed.\n");
        return NULL;
    }
    device_msg_channels[num_of_msg_channels - 1]->id = requested_msg_channel_id;
    return device_msg_channels[num_of_msg_channels - 1];
}

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    // Switch according to the ioctl called
    if(ioctl_command_id != MSG_SLOT_CHANNEL || !ioctl_param /* Cannot be 0 */) {
        errno = EINVAL;
        return -1;
    }
    // Get the parameter given to ioctl by the process
    printk("Invoking ioctl: %ld\n", ioctl_param);
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
    // init dev struct
    memset( &device_info, 0, sizeof(struct chardev_info) );
    spin_lock_init(&device_info.lock);

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if( rc < 0 ) {
        printk(KERN_ALERT "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

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