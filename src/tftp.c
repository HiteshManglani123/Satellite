#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "tftp.h"

#define MAX_BUF_SIZE 8192

/*
    Retrieve a file using a custom (simplified) TFTP proctol.
    The flow of the protocol is as follows:
        - Send a Read Request to the specified socket
        - Specified socket keeps on sending Data pakcets untill the length of the
            data packet is < 512
    Returns 0 on success, -1 on failure.
*/
int tftp_retrieve_file(int sfd, struct sockaddr_un dest_addr, char *buf, size_t len)
{
    // Send a RRQ request
    const char *filename = "temp_file";
    const char *mode = tftp_mode_str[MODE_OCTET];

    struct tftp_request rrq = {
        .opcode = TFTP_RRQ,
        .filename = "temp_file",
        .zero1 = '\0',
        .mode = tftp_mode_str[MODE_OCTET],
        .zero2 = '\0'
    };

    // Serialize RRQ packet
    int filename_len = strlen(rrq.filename);
    int mode_len = strlen(rrq.mode);

    int serial_buf_len = 2 + filename_len + 1 + mode_len + 1;
    char serial_buf[serial_buf_len];


    int offset = 0;
    // Save opcode - save in Big Endian
    serial_buf[offset++] = (rrq.opcode >> 8) & 0xFF;
    serial_buf[offset++] = (rrq.opcode) & 0xFF;
    // Save filename
    strncpy(serial_buf[offset], rrq.filename, filename_len);
    offset += filename_len;
    // Save reserved1
    serial_buf[offset++] = 0;
    // Save mode
    strncpy(serial_buf[offset], rrq.mode, mode_len);
    offset += mode_len;
    // Save reserved2
    serial_buf[offset] = 0;

    // Send a read request packet to destination address
    if (sendto(sfd, serial_buf, serial_buf_len, 0, (struct sockaddr *) dest_addr, sizeof(dest_addr))) {
        perror("Unable to send request packet to destination address");
        exit(1);
    }

    // this sets up a data -> ack chain untill the data block has data len of < 511
    char recv_buf[MAX_BUF_SIZE];
    size_t recv_len;
    while (1) {
        // wait for the first data block
        if ((recv_len = recvfrom(sfd, recv_buf, MAX_BUF_SIZE, 0, NULL, NULL)) == -1) {
            perror("error while receiving");
            exit(1);
        }

        printf("Received buf - sending ack: %s\n", recv_buf);

        // send ACK request
    }

    // split buff into chucks

    return 0;
}

int tftp_send_file(int sfd, char *buf, size_t len)
{
    struct sockaddr_un client_addr;

    char *recv_buf[MAX_BUF_SIZE];

    // Wait for connection
    size_t recv_len;
    socklen_t sock_len = sizeof(struct sockaddr_un);

    if ((recv_len = recvfrom(sfd, recv_buf, MAX_BUF_SIZE, 0, (struct sockaddr *) &client_addr, &sock_len)) == 1) {
        perror("unable to establish connection with client addr");
        exit(1);
    }

    // Send first data packet
    char *data = "first data packet";

    if (sendto(sfd, data, strlen(data), 0, (struct sockaddr *) &client_addr, sizeof(client_addr)) == -1) {
        perror("Unable to send first data packet");
        exit(1);
    }
}
