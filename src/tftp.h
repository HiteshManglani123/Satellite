#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <sys/socket.h>
#include <stdlib.h>

// Constants
#define MAX_BUF_SIZE 8192
#define MAX_FILENAME_LEN 128
#define MAX_DATA_LEN 512
#define MAX_MODE_LEN 20

// TFTP opcodes
#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5

// TFTP modes
#define MODE_NETASCII 0
#define MODE_OCTET    1
#define MODE_MAIL     2  // deprecated

// Mode strings
extern char *tftp_mode_str[];

// TFTP packet structures
struct tftp_request {
    uint16_t opcode;
    char filename[MAX_FILENAME_LEN];
    char mode[MAX_MODE_LEN];
};

struct tftp_data {
    uint16_t opcode;
    uint16_t block;
    uint8_t data[MAX_DATA_LEN];
};

struct tftp_ack {
    uint16_t opcode;
    uint16_t block;
};

struct tftp_error {
    uint16_t opcode;
    uint16_t error_code;
    char error_msg[512];
};

// String packing/unpacking functions
size_t pack_str(uint8_t *buf, const char *str, size_t str_len);
size_t unpack_str(uint8_t *src_buf, char *dest_buf, size_t max_str_len);

// Packet serialization/deserialization functions
void serialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, size_t filename_len, size_t mode_len);
void serialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len);
void serialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt);
void deserialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, int buf_len);
void deserialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len);
void deserialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt);

// Public interface
int tftp_send_file(int sfd, uint8_t *buf, size_t buf_len, const char *log_prefix);
int tftp_retrieve_file(int sfd, struct sockaddr_un dest_addr, uint8_t *buf, size_t len, const char *log_prefix);

#endif // TFTP_H
