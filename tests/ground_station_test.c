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
#define RECEIVED_FILE_PATH "received-images/test.bmp"

// Forward declarations
int tftp_retrieve_file(int sfd, struct sockaddr_un dest_addr, uint8_t *buf, size_t len, const char *log_prefix);
void serialize_data_pkt(uint8_t *buf, struct tftp_data *data_pkt, size_t data_len);
void deserialize_ack_pkt(uint8_t *buf, struct tftp_ack *ack_pkt);

// Simple mock satellite that sends a small test file
static void run_mock_satellite(void) {
    int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd < 0) {
        perror("unable to open socket stream");
        exit(1);
    }

    // Clean up any existing socket
    unlink(SATELLITE_SOCKET_PATH);

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("unable to bind server socket");
        exit(1);
    }

    // Wait for client connection
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t recv_buf[MAX_BUF_SIZE];
    
    // Receive RRQ
    if (recvfrom(sfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &client_addr, &client_len) < 0) {
        perror("error receiving RRQ");
        exit(1);
    }

    // Send a simple test file (just 100 bytes of data)
    uint8_t test_data[100];
    for (int i = 0; i < 100; i++) {
        test_data[i] = i % 256;
    }

    // Send data in one packet
    struct tftp_data data_pkt = {
        .opcode = TFTP_DATA,
        .block = 1
    };
    memcpy(data_pkt.data, test_data, 100);

    size_t pkt_len = sizeof(data_pkt.opcode) + sizeof(data_pkt.block) + 100;
    uint8_t pkt_buf[pkt_len];
    serialize_data_pkt(pkt_buf, &data_pkt, 100);

    if (sendto(sfd, pkt_buf, pkt_len, 0, (struct sockaddr *) &client_addr, client_len) == -1) {
        perror("error sending data packet");
        exit(1);
    }

    // Wait for ACK
    if (recvfrom(sfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &client_addr, &client_len) < 0) {
        perror("error receiving ACK");
        exit(1);
    }

    close(sfd);
    unlink(SATELLITE_SOCKET_PATH);
}

int main() {
    printf("Testing ground-station file retrieval...\n");

    // Create required directories
    mkdir("temp", 0755);
    mkdir("received-images", 0755);

    // Clean up any existing socket files
    unlink(SATELLITE_SOCKET_PATH);
    unlink(GROUND_STATION_SOCKET_PATH);

    // Fork a child to act as the satellite/server
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process: Run the mock satellite
        run_mock_satellite();
        return 0;
    }

    // Parent process: Run the ground-station logic
    int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd < 0) {
        perror("unable to open socket stream");
        return 1;
    }

    struct sockaddr_un satellite_addr, ground_station_addr;
    memset(&ground_station_addr, 0, sizeof(struct sockaddr_un));
    ground_station_addr.sun_family = AF_UNIX;
    strncpy(ground_station_addr.sun_path, GROUND_STATION_SOCKET_PATH, sizeof(ground_station_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &ground_station_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("unable to bind client socket");
        return 1;
    }

    memset(&satellite_addr, 0, sizeof(struct sockaddr_un));
    satellite_addr.sun_family = AF_UNIX;
    strncpy(satellite_addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(satellite_addr.sun_path) - 1);

    // Wait for server socket to exist
    int wait_count = 0;
    while (access(SATELLITE_SOCKET_PATH, F_OK) == -1 && wait_count < 100) {
        usleep(1000); // 1ms
        wait_count++;
    }

    // Retrieve the file
    int result = tftp_retrieve_file(sfd, satellite_addr, NULL, 1, "[GROUND STATION]");
    TEST_ASSERT(result == 0);

    // Verify the file was created and contains the expected data
    FILE *fp = fopen(RECEIVED_FILE_PATH, "rb");
    TEST_ASSERT(fp != NULL);
    
    uint8_t received_data[100];
    size_t bytes_read = fread(received_data, 1, 100, fp);
    fclose(fp);
    
    TEST_ASSERT(bytes_read == 100);
    
    // Verify the data matches what we sent
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT(received_data[i] == (i % 256));
    }

    close(sfd);
    unlink(GROUND_STATION_SOCKET_PATH);
    wait(NULL); // Wait for the child to finish

    if (tests_run == tests_passed) {
        printf("\033[0;32m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    } else {
        printf("\033[0;31m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    }
    return 0;
} 