#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>

#define NETASCII "netascii"

enum tftp_opcode {
    MODE_NETASCII,
    MODE_OCTET,
    MODE_MAIL, // deprecated
};

static const char *const tftp_mode_str[] = {
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
    const char *filename;
    char zero1; // should be '\0'
    const char *mode;
    char zero2; // should be '\0'
};

struct tftp_ack {
    uint16_t opcode;
    uint16_t block;
};

struct tftp_data {
    uint16_t opcode;
    uint16_t block;
    uint8_t data[]; // flexible array for variable length data
};

int tftp_send_file(int sfd, uint8_t *buf, size_t len);

#endif
