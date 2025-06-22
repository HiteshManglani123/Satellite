#include <stdio.h>
#include <string.h>
#include "tftp_test_interface.h"

static int tests_run = 0;
static int tests_passed = 0;
#define TEST_ASSERT(expr) do { tests_run++; if (expr) { tests_passed++; } else { \
    printf("\033[0;31mTest failed: %s at %s:%d\033[0m\n", #expr, __FILE__, __LINE__); \
    return; } } while(0)

// Test helper functions
void assert_memory_equal(const void* a, const void* b, size_t size) {
    TEST_ASSERT(memcmp(a, b, size) == 0);
}

// Test pack_str and unpack_str
void test_string_packing() {
    printf("Testing string packing/unpacking...\n");
    
    // Test Case 1: Normal string (Happy Path)
    {
        const char* test_str = "hello";
        uint8_t buf[100] = {0};
        char unpacked[100] = {0};
        
        size_t packed_len = pack_str(buf, test_str, strlen(test_str));
        size_t unpacked_len = unpack_str(buf, unpacked, sizeof(unpacked));
        
        TEST_ASSERT(packed_len == strlen(test_str) + 1);
        TEST_ASSERT(unpacked_len == strlen(test_str) + 1);
        TEST_ASSERT(strcmp(test_str, unpacked) == 0);
    }
    
    // Test Case 2: Empty string (Edge Case)
    {
        const char* test_str = "";
        uint8_t buf[100] = {0};
        char unpacked[100] = {0};
        
        size_t packed_len = pack_str(buf, test_str, strlen(test_str));
        size_t unpacked_len = unpack_str(buf, unpacked, sizeof(unpacked));
        
        TEST_ASSERT(packed_len == 1);
        TEST_ASSERT(unpacked_len == 1);
        TEST_ASSERT(strcmp(test_str, unpacked) == 0);
    }
    
    // Test Case 3: String with special characters (Edge Case)
    {
        const char* test_str = "hello\n\t\r";
        uint8_t buf[100] = {0};
        char unpacked[100] = {0};
        
        size_t packed_len = pack_str(buf, test_str, strlen(test_str));
        size_t unpacked_len = unpack_str(buf, unpacked, sizeof(unpacked));
        
        TEST_ASSERT(packed_len == strlen(test_str) + 1);
        TEST_ASSERT(unpacked_len == strlen(test_str) + 1);
        TEST_ASSERT(strcmp(test_str, unpacked) == 0);
    }
    
    // Test Case 4: Buffer overflow protection (Error Case)
    {
        const char* test_str = "hello";
        uint8_t buf[3] = {0};  // Too small buffer
        char unpacked[3] = {0}; // Too small buffer
        
        size_t packed_len = pack_str(buf, test_str, 2);  // Only pack 2 chars
        size_t unpacked_len = unpack_str(buf, unpacked, sizeof(unpacked));
        
        TEST_ASSERT(packed_len == 3);  // 2 chars + null terminator
        TEST_ASSERT(unpacked_len == 3);  // 2 chars + null terminator
        TEST_ASSERT(strncmp("he", unpacked, 2) == 0);
    }
}

// Test packet serialization/deserialization
void test_packet_serialization() {
    printf("Testing packet serialization/deserialization...\n");
    
    // Test Case 1: RRQ Packet (Happy Path)
    {
        struct tftp_request rrq = {
            .opcode = TFTP_RRQ,
            .filename = "test.txt",
            .mode = "octet"
        };
        
        uint8_t serialized[256] = {0};
        struct tftp_request deserialized = {0};
        serialize_rrq_pkt(serialized, &rrq, strlen(rrq.filename), strlen(rrq.mode));
        deserialize_rrq_pkt(serialized, &deserialized, sizeof(serialized));
        
        TEST_ASSERT(deserialized.opcode == TFTP_RRQ);
        TEST_ASSERT(strcmp(deserialized.filename, "test.txt") == 0);
        TEST_ASSERT(strcmp(deserialized.mode, "octet") == 0);
    }
    
    // Test Case 2: DATA Packet (Happy Path)
    {
        struct tftp_data data = {
            .opcode = TFTP_DATA,
            .block = 1234,
            .data = "test data"
        };
        
        uint8_t serialized[256] = {0};
        struct tftp_data deserialized = {0};
        
        serialize_data_pkt(serialized, &data, strlen((char*)data.data));
        deserialize_data_pkt(serialized, &deserialized, strlen((char*)data.data));
        
        TEST_ASSERT(deserialized.opcode == TFTP_DATA);
        TEST_ASSERT(deserialized.block == 1234);
        TEST_ASSERT(memcmp(deserialized.data, "test data", strlen("test data")) == 0);
    }
    
    // Test Case 3: ACK Packet (Happy Path)
    {
        struct tftp_ack ack = {
            .opcode = TFTP_ACK,
            .block = 5678
        };
        
        uint8_t serialized[256] = {0};
        struct tftp_ack deserialized = {0};
        
        serialize_ack_pkt(serialized, &ack);
        deserialize_ack_pkt(serialized, &deserialized);
        
        TEST_ASSERT(deserialized.opcode == TFTP_ACK);
        TEST_ASSERT(deserialized.block == 5678);
    }
    
    // Test Case 4: Invalid Packet (Error Case)
    {
        uint8_t invalid_packet[] = {0xFF, 0xFF, 0xFF, 0xFF};  // Invalid opcode
        struct tftp_ack ack = {0};
        
        deserialize_ack_pkt(invalid_packet, &ack);
        TEST_ASSERT(ack.opcode == 0xFFFF);  // Should be all 1's
        TEST_ASSERT(ack.block == 0xFFFF);   // Should be all 1's
    }
    
    // Test Case 5: Edge Case - Maximum Block Number
    {
        struct tftp_ack ack = {
            .opcode = TFTP_ACK,
            .block = 0xFFFF  // Maximum block number
        };
        
        uint8_t serialized[256] = {0};
        struct tftp_ack deserialized = {0};
        
        serialize_ack_pkt(serialized, &ack);
        deserialize_ack_pkt(serialized, &deserialized);
        
        TEST_ASSERT(deserialized.opcode == TFTP_ACK);
        TEST_ASSERT(deserialized.block == 0xFFFF);
    }
}

int main() {
    printf("Starting TFTP protocol tests...\n");
    
    test_string_packing();
    test_packet_serialization();
    
    if (tests_run == tests_passed) {
        printf("\033[0;32m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    } else {
        printf("\033[0;31m%d/%d tests passed\033[0m\n", tests_passed, tests_run);
    }
    return 0;
} 