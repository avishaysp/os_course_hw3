#include "message_slot.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>



int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Invalid Number of arguments for message_sender.c: %d instead of 3", argc);
        exit(1);
    }
    // Parse args
    char* msg_slot_path = argv[1];
    uint64_t target_channel_id = strtoull(argv[2], NULL, 10);  // convert str to unsigned long long
    char* curr_msg = argv[3];

    int fd = open(msg_slot_path, O_RDWR);
    if (fd < 0) {
        perror("open device file failure");
        exit(1);
    }
    int ret = ioctl(fd, MSG_SLOT_CHANNEL, target_channel_id);
    if (ret == -1) {
        perror("device ioctl failure");
        exit(1);
    }
    ret = write(fd, curr_msg, strlen(curr_msg) - 1);
        if (ret == -1) {
        perror("device write failure");
        exit(1);
    }
    close(fd);
    exit(0);
}