#ifndef __SERIAL_H__
#define __SERIAL_H__

int serial_init(char *devpath);

int serial_read_data(int fd, unsigned char *data_buf, int data_size);

int serial_send_data(int fd, unsigned char *data_buf, int data_size);
#endif
