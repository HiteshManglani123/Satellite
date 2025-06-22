#ifndef TFTP_TEST_INTERFACE_H
#define TFTP_TEST_INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include "../src/tftp.h"

// Internal functions exposed for testing only
size_t pack_str(uint8_t *buf, const char *str, size_t str_len);
size_t unpack_str(uint8_t *src_buf, char *dest_buf, size_t max_str_len);

void serialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, size_t filename_len, size_t mode_len);
void serialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len);
void serialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt);

void deserialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, int buf_len);
void deserialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len);
void deserialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt);

#endif // TFTP_TEST_INTERFACE_H 