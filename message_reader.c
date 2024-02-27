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
    if (argc != 3) {
        printf("Invalid Number of arguments for message_sender.c: %d instead of 2", argc);
        exit(1);
    }
    // Parse args
    char* msg_slot_path = argv[1];
    uint64_t target_channel_id = strtoull(argv[2], NULL, 10);  // convert str to unsigned long long

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
    char buffer[BUF_LEN];
    ret = read(fd, buffer, BUF_LEN);
        if (ret == -1) {
        perror("device read failure");
        exit(1);
    }
    printf("%s", buffer);
    close(fd);
    exit(0);
}