#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "../src/tftp.h"

static int tests_run = 0;
static int tests_passed = 0;
#define TEST_ASSERT(expr) do { tests_run++; if (expr) { tests_passed++; } else { \
    printf("\033[0;31mTest failed: %s at %s:%d\033[0m\n", #expr, __FILE__, __LINE__); \
    return 1; } } while(0)

#define SATELLITE_SOCKET_PATH "temp/server-socket"
#define GROUND_STATION_SOCKET_PATH "temp/client-socket"
#define TEST_FILE_PATH "images/test.bmp"

// Forward declarations
int tftp_send_file(int sfd, uint8_t *buf, size_t buf_len, const char *log_prefix);
void serialize_rrq_pkt(uint8_t *buf, struct tftp_request *rrq_pkt, size_t filename_len, size_t mode_len);
void deserialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len);
void serialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt);

// Function to act as a minimal ground-station/client
void run_mock_ground_station() {
    printf("[GROUND STATION] Starting mock ground station...\n");
    // Clean up any existing socket
    unlink(GROUND_STATION_SOCKET_PATH);
    int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd < 0) {
        perror("unable to open socket stream");
        exit(1);
    }
    printf("[GROUND STATION] Socket created successfully\n");
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_un));
    client_addr.sun_family = AF_UNIX;
    strncpy(client_addr.sun_path, GROUND_STATION_SOCKET_PATH, sizeof(client_addr.sun_path) - 1);
    if (bind(sfd, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("unable to bind client socket");
        exit(1);
    }
    printf("[GROUND STATION] Socket bound successfully\n");
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    // Send a TFTP read request
    struct tftp_request rrq = { .opcode = TFTP_RRQ };
    strcpy(rrq.filename, "test.bmp");
    strcpy(rrq.mode, "octet");
    size_t filename_len = strlen(rrq.filename);
    size_t mode_len = strlen(rrq.mode);
    int serial_buf_len = sizeof(rrq.opcode) + filename_len + 1 + mode_len + 1;
    uint8_t serial_buf[serial_buf_len];
    serialize_rrq_pkt(serial_buf, &rrq, filename_len, mode_len);
    printf("[GROUND STATION] Sending read request...\n");
    if (sendto(sfd, serial_buf, serial_buf_len, 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Unable to send request packet");
        exit(1);
    }
    printf("[GROUND STATION] Read request sent successfully\n");
    // Receive data packets and send ACKs
    uint8_t recv_buf[1024];
    while (1) {
        printf("[GROUND STATION] Waiting for data packet...\n");
        ssize_t recv_len = recvfrom(sfd, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
        if (recv_len < 0) {
            perror("error receiving data");
            exit(1);
        }
        printf("[GROUND STATION] Received data packet of size %zd\n", recv_len);
        struct tftp_data data_pkt;
        size_t data_len = recv_len - sizeof(data_pkt.opcode) - sizeof(data_pkt.block);
        deserialize_data_pkt(recv_buf, &data_pkt, data_len);
        printf("[GROUND STATION] Received block %d with %zu bytes of data\n", data_pkt.block, data_len);
        // Send ACK
        struct tftp_ack ack_pkt = { .opcode = TFTP_ACK, .block = data_pkt.block };
        size_t ack_pkt_len = sizeof(ack_pkt.opcode) + sizeof(ack_pkt.block);
        uint8_t ack_pkt_buf[ack_pkt_len];
        serialize_ack_pkt(ack_pkt_buf, &ack_pkt);
        printf("[GROUND STATION] Sending ACK for block %d\n", ack_pkt.block);
        if (sendto(sfd, ack_pkt_buf, ack_pkt_len, 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1) {
            perror("Unable to send ACK");
            exit(1);
        }
        printf("[GROUND STATION] ACK sent successfully\n");
        if (data_len < MAX_DATA_LEN) {
            printf("[GROUND STATION] Received final packet, exiting...\n");
            break;
        }
    }
    printf("[GROUND STATION] Closing socket and cleaning up...\n");
    close(sfd);
    unlink(GROUND_STATION_SOCKET_PATH);
    printf("[GROUND STATION] Exiting successfully\n");
    exit(0);
}

int main() {
    printf("[TEST] Starting satellite file sending test...\n");
    printf("[TEST] Creating directories...\n");
    mkdir("temp", 0777);
    mkdir("images", 0777);
    mkdir("received-images", 0777);
    printf("[TEST] Cleaning up existing sockets...\n");
    unlink(SATELLITE_SOCKET_PATH);
    unlink(GROUND_STATION_SOCKET_PATH);
    printf("[TEST] Creating test file...\n");
    FILE *fp = fopen(TEST_FILE_PATH, "wb");
    TEST_ASSERT(fp != NULL);
    uint8_t test_data[100];
    for (int i = 0; i < 100; i++) {
        test_data[i] = i;
    }
    TEST_ASSERT(fwrite(test_data, 1, 100, fp) == 100);
    fclose(fp);
    printf("[TEST] Test file created successfully\n");
    printf("[TEST] Forking ground station process...\n");
    pid_t pid = fork();
    TEST_ASSERT(pid >= 0);
    if (pid == 0) {
        run_mock_ground_station();
    } else {
        printf("[TEST] Parent process: Setting up satellite...\n");
        int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        TEST_ASSERT(sfd >= 0);
        printf("[TEST] Satellite socket created\n");
        struct sockaddr_un server_addr;
        memset(&server_addr, 0, sizeof(struct sockaddr_un));
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
        TEST_ASSERT(bind(sfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) != -1);
        printf("[TEST] Satellite socket bound\n");
        printf("[TEST] Opening test file...\n");
        fp = fopen(TEST_FILE_PATH, "rb");
        TEST_ASSERT(fp != NULL);
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        printf("[TEST] Test file size: %ld bytes\n", file_size);
        uint8_t *buf = malloc(file_size);
        TEST_ASSERT(buf != NULL);
        TEST_ASSERT(fread(buf, 1, file_size, fp) == file_size);
        printf("[TEST] Test file read successfully\n");
        fclose(fp);
        printf("[TEST] Starting file transfer...\n");
        int result = tftp_send_file(sfd, buf, file_size, "[SATELLITE]");
        printf("[TEST] File transfer completed with result: %d\n", result);
        TEST_ASSERT(result == 0);
        printf("[TEST] Cleaning up...\n");
        free(buf);
        close(sfd);
        unlink(SATELLITE_SOCKET_PATH);
        printf("[TEST] Waiting for ground station to finish...\n");
        wait(NULL);
        printf("[TEST] Ground station finished\n");
    }
    printf("[TEST] Testing non-happy path...\n");
    int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    TEST_ASSERT(sfd >= 0);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    TEST_ASSERT(setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0);
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    TEST_ASSERT(bind(sfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) != -1);
    fp = fopen(TEST_FILE_PATH, "rb");
    TEST_ASSERT(fp != NULL);
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *buf = malloc(file_size);
    TEST_ASSERT(buf != NULL);
    TEST_ASSERT(fread(buf, 1, file_size, fp) == file_size);
    fclose(fp);
    printf("[TEST] Attempting file transfer with no client (should timeout)...\n");
    int result = tftp_send_file(sfd, buf, file_size, "[SATELLITE]");
    printf("[TEST] File transfer result: %d (expected -1)\n", result);
    TEST_ASSERT(result == -1);
    free(buf);
    close(sfd);
    unlink(SATELLITE_SOCKET_PATH);
    if (tests_run == tests_passed) {
        printf("\033[0;32m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    } else {
        printf("\033[0;31m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    }
    return 0;
} 