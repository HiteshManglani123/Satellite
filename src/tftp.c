#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "tftp.h"

// Returns amount of bytes saved in buf
size_t pack_str(uint8_t *buf, const char *str, size_t str_len)
{
    memcpy(buf, str, str_len);
    buf[str_len] = '\0';

    return str_len + 1;
}

// Returns amount of bytes read from src_buf
size_t unpack_str(uint8_t *src_buf, char *dest_buf, size_t max_str_len)
{
    strncpy(dest_buf, (char *) src_buf, max_str_len);
    dest_buf[max_str_len - 1] = '\0';

    size_t str_len = strnlen(dest_buf, max_str_len);
    return str_len + 1;
}

// Saved in Big Endian
void serialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, size_t filename_len, size_t mode_len)
{
    size_t offset = 0;

    // Save opcode
    buf[offset++] = (rrq_pkt->opcode >> 8) & 0xFF;
    buf[offset++] = (rrq_pkt->opcode) & 0xFF;

    // Save filename
    offset += pack_str(buf + offset, rrq_pkt->filename, filename_len);

    // Save mode
    pack_str(buf + offset, rrq_pkt->mode, mode_len);
}

// Saved in Big Endian
void serialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len)
{
    int offset = 0;

    // Save opcode
    buf[offset++] = (data_pkt->opcode >> 8) & 0xFF;
    buf[offset++] = data_pkt->opcode & 0xFF;
    // Save block number
    buf[offset++] = (data_pkt->block >> 8) & 0xFF;
    buf[offset++] = data_pkt->block & 0xFF;
    /*
        Save Data buf
        a lenth under 512 means that this is the last data packet to be sent.
    */
    // size_t actual_data_length = strlen()
    // size_t data_len = strnlen(data_pkt->data, MAX_DATA_LEN);
    memcpy(buf + offset, data_pkt->data, data_len);
    offset += data_len;
}

// Saved in Big Endian
void serialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt)
{
    size_t offset = 0;

    // Save opcode
    buf[offset++] = (ack_pkt->opcode >> 8) & 0xFF;
    buf[offset++] = ack_pkt->opcode & 0xFF;
    // Save block
    buf[offset++] = (ack_pkt->block >> 8) & 0xFF;
    buf[offset++] = ack_pkt->block & 0xFF;
}


// Received buf is stored in Big Endian
void deserialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt)
{
    ack_pkt->opcode = ((buf[0] << 8) | buf[1]) & 0xFFFF;
    ack_pkt->block = ((buf[2] << 8) | buf[3]) & 0xFFFF;
}

//  Received buf is stored in Big Endian, assumes RRQ has enough space
void deserialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, int buf_len) {
    size_t offset = 0;

    // Opcode
    rrq_pkt->opcode = ((buf[offset] << 8) | buf[offset + 1]) & 0xFFFF;
    offset += 2;
    // Filename
    offset += unpack_str(buf + offset, rrq_pkt->filename, MAX_FILENAME_LEN);
    // Block
    unpack_str(buf + offset, rrq_pkt->mode, MAX_MODE_LEN);
}

//  Received buf is stored in Big Endian
void deserialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len)
{
    size_t offset = 0;

    // Opcode

    data_pkt->opcode = ((buf[offset] << 8) | buf[offset + 1]) & 0xFFFF;
    offset += 2;
    // Block
    uint16_t msb_half = (buf[offset] << 8) & 0xFF00;
    uint16_t lsb_half = (buf[offset + 1]) & 0xFF;
    data_pkt->block = ((buf[offset] << 8) | buf[offset + 1]) & 0xFFFF;
    offset += 2;
    // Data
    // printf("offset is : %lu!\n", offset);
    // printf("going to serialize now!\n");
    // for (size_t i = 0; i < data_len; i += 4) {
    //     printf("i: %lu - %.2x%.2x %.2x%.2x\n", i, buf[offset + i], buf[offset + i + 1], buf[offset + i + 2], buf[offset + i + 3]);
    // }

    memcpy(data_pkt->data, buf + offset, data_len);

    // printf("after memcpy!\n");
    // for (size_t i = 0; i < data_len; i += 4) {
    //     printf("i: %lu - %.2x%.2x %.2x%.2x\n", i, data_pkt->data[i], data_pkt->data[i + 1], data_pkt->data[i + 2], data_pkt->data[i + 3]);
    // }
}

/*
    The Ground Station Receiving Images from a Satellite.

    Retrieve a file using a custom (simplified) TFTP proctol.
    The flow of the protocol is as follows:
        - Send a Read Request to the specified socket
        - Specified socket keeps on sending Data pakcets untill the length of the
            data packet is < 512
    Returns 0 on success, -1 on failure.
*/
int tftp_retrieve_file(int sfd, struct sockaddr_un dest_addr, uint8_t *buf, size_t len)
{
    // Create RRQ
    char *filename = "temp_file";
    char *mode = tftp_mode_str[MODE_OCTET];

    struct tftp_request rrq = { .opcode = TFTP_RRQ };
    strcpy(rrq.filename, filename);
    strcpy(rrq.mode, mode);

    size_t filename_len = strlen(filename);
    size_t mode_len = strlen(mode);

    // Serialize RRQ packet
    int serial_buf_len = sizeof(rrq.opcode) + filename_len + 1 + mode_len + 1;

    uint8_t serial_buf[serial_buf_len];

    serialize_rrq_pkt(serial_buf, &rrq, filename_len, mode_len);

    // Send a RRQ to destination address
    if (sendto(sfd, serial_buf, serial_buf_len, 0, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Unable to send request packet to destination address");
        return -1;
    }

    // Open new image
    FILE *fp_image= fopen("received-images/1.bmp", "wb");

    if (fp_image == NULL) {
        perror("unable to allocate space for new image");
        return -1;
    }

    /*
        This setups a loop to receive data packets and send acknowledgement pakcets.
        The end of a transmission is determined by the size of the data packet received.
        If the data in the received data packet is < 512, it will stop transmission.
    */
    uint8_t recv_buf[MAX_BUF_SIZE];
    size_t recv_len;

    while (1) {
        // Receive Data Packet
        if ((recv_len = recvfrom(sfd, recv_buf, MAX_BUF_SIZE, 0, NULL, NULL)) == -1) {
            perror("error while receiving");
            fclose(fp_image);
            return -1;
        }
        struct tftp_data data_pkt;
        size_t data_len = recv_len - sizeof(data_pkt.opcode) - sizeof(data_pkt.block);

        printf("just received: %lu\n", recv_len);
        uint8_t *recv_buf_p = recv_buf + sizeof(data_pkt.opcode) + sizeof(data_pkt.block);
        // for (size_t i = 0; i < data_len; i += 4) {
        //     printf("i: %lu - %.2x%.2x %.2x%.2x\n", i, recv_buf_p[i], recv_buf_p[i + 1], recv_buf_p[i + 2], recv_buf_p[i + 3]);
        // }


        deserialize_data_pkt(recv_buf, &data_pkt, data_len);

        printf("Received Data packet. Opcode: %d, Block: %d\n", data_pkt.opcode, data_pkt.block);

        // Write buf to new image file
        if (fwrite(data_pkt.data, 1, data_len, fp_image) != data_len) {
            fprintf(stderr, "Unable to write header file of %lu bytes\n", data_len);
            fclose(fp_image);
            return -1;
        }

        // Send ACK Packet
        struct tftp_ack ack_pkt = {
            .opcode = TFTP_ACK,
            .block = data_pkt.block
        };

        size_t ack_pkt_len = sizeof(ack_pkt.opcode) + sizeof(ack_pkt.block);
        uint8_t ack_pkt_buf[ack_pkt_len];

        serialize_ack_pkt(ack_pkt_buf, &ack_pkt);

        // Send an ACK to destination address
        if (sendto(sfd, ack_pkt_buf, ack_pkt_len, 0, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr_un)) == -1) {
            perror("Unable to send request packet to destination address");
            fclose(fp_image);
            return -1;
        }

        printf("Sent Ack packet with block: %d!\n", ack_pkt.block);
        if (data_len < MAX_DATA_LEN) {
            break;
        }

    }

    // split buff into chucks
    fclose(fp_image);
    return 0;
}

/*
    The satellite sending data (images) packets
    Returns 0 on success, -1 on failure.
*/
int tftp_send_file(int sfd, uint8_t *buf, size_t  buf_len)
{
    if (buf == NULL) {
        fprintf(stderr, "Could not send file, Buf is NULL");
        return -1;
    }

    struct sockaddr_un client_addr;
    socklen_t sock_len = sizeof(struct sockaddr_un);

    uint8_t recv_buf[MAX_BUF_SIZE];
    size_t recv_len;

    /*
        Receive a RRQ Packet from Ground Station.
        This will fill in the client_addr struct.
    */
    if ((recv_len = recvfrom(sfd, recv_buf, MAX_BUF_SIZE, 0, (struct sockaddr *) &client_addr, &sock_len)) == 1) {
        perror("unable to establish connection with client addr");
        exit(1);
    }

    struct tftp_request request_pkt;
    deserialize_rrq_pkt(recv_buf, &request_pkt, recv_len);

    printf("Received rrq pakcet. Opcode: %d filename: %s, mode: %s\n", request_pkt.opcode, request_pkt.filename, request_pkt.mode);
    /*
        This setups a loop to send data packets and receive acknowledgement pakcets.
        The end of a transmission is determined by the size of the data packet received.
        If the data in the received data packet is < 512, it will stop transmission after receiving the final data packry.
    */

    uint16_t current_block = 1;
    // uint8_t data[MAX_DATA_LEN];
    size_t index = 0;

    struct tftp_data data_pkt = {
        .opcode = TFTP_DATA,
        .data = {0}
    };


    while (1) {
        // Create data packet
        /*
            1. Check if there is room for MAX_DATA_LEN
            2. If there is: memcpy MAX_DATA_LEN into data
            3. If there is not: memcpy the remaining ones
        */
        data_pkt.block = current_block;

        int final_chunk = (buf_len - index) < MAX_DATA_LEN;
        size_t data_len =  final_chunk ? (buf_len - index) : MAX_DATA_LEN;

        memcpy(data_pkt.data, buf + index, data_len);

        index += data_len;

        // Serialize first data packet
        size_t data_pkt_len = sizeof(data_pkt.opcode) + sizeof(data_pkt.block) + data_len;
        uint8_t data_pkt_buf[data_pkt_len];

        printf("Serializing block: %d\n", current_block);

        serialize_data_pkt(data_pkt_buf, &data_pkt, data_len);

        // printf("after serialize data_pkt_len: %lu\n", data_pkt_len);
        // for (size_t i = 0; i < data_len; i += 4) {
        //     uint8_t *data_pkt_buf_p = data_pkt_buf + sizeof(data_pkt.opcode) + sizeof(data_pkt.block);
        //     printf("i: %lu - %.2x%.2x %.2x%.2x\n", i, data_pkt_buf_p[i], data_pkt_buf_p[i + 1], data_pkt_buf_p[i + 2],data_pkt_buf_p[i + 3]);
        // }

        // Send first data packet
        printf("Sending data packet. Opcode %d - Block: %d\n", data_pkt.opcode, data_pkt.block);

        if (sendto(sfd, data_pkt_buf, data_pkt_len, 0, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_un)) == -1) {
            perror("Unable to send first data packet");
            exit(1);
        }

        // Wait for ACK pakcet - We don't care about the client address now
        if ((recv_len = recvfrom(sfd, recv_buf, MAX_BUF_SIZE, 0, NULL, NULL)) == 1) {
            perror("unable to establish connection with client addr");
            exit(1);
        }

        struct tftp_ack ack_pkt;

        deserialize_ack_pkt(recv_buf, &ack_pkt);
        printf("Received ack. Opcode %d - Block: %d\n", ack_pkt.opcode, ack_pkt.block);

        if (ack_pkt.block != data_pkt.block) {
            printf("Wrong packet wtf!\n");
            return -1;
        }

        if (final_chunk) {
            printf("Done sending!\n");
            return 0;
        }

        current_block++;
    }

    return 0;
}
