make
sudo insmod message_slot.ko
sudo mknod /dev/msg_slot c 235 0
sudo chmod 666 /dev/msg_slot
gcc -O3 -Wall -std=c11 message_sender.c -o message_sender
gcc -O3 -Wall -std=c11 message_reader.c -o message_reader