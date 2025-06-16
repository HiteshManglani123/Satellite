#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <stdlib.h>

#define NETASCII "netascii"

#define MAX_BUF_SIZE 8192 // Maximum size buf i can send and receive
#define MAX_FILENAME_LEN 128
#define MAX_DATA_LEN 512
#define MAX_MODE_LEN 20

enum tftp_opcode {
    MODE_NETASCII,
    MODE_OCTET,
    MODE_MAIL, // deprecated
};


static char *tftp_mode_str[] = {
    [MODE_NETASCII] = "netascii",
    [MODE_OCTET] = "octet",
    [MODE_MAIL] = "mail"
};

enum tftp_mode {
    TFTP_RRQ,
    TFTP_WRQ,
    TFTP_DATA,
    TFTP_ACK,
    TFTP_ERROR
};

struct tftp_request {
    uint16_t opcode;
    char filename[MAX_FILENAME_LEN];
    char mode[MAX_MODE_LEN];
};

struct tftp_ack {
    uint16_t opcode;
    uint16_t block;
};

struct tftp_data {
    uint16_t opcode;
    uint16_t block;
    uint8_t data[MAX_DATA_LEN]; // flexible array for variable length data
};

int tftp_send_file(int sfd, uint8_t *buf, size_t buf_len);
int tftp_retrieve_file(int sfd, struct sockaddr_un dest_addr, uint8_t *buf, size_t len);

#endif
