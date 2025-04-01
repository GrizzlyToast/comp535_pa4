#include "multicast.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define SEG_SIZE 1024

uint32_t calculate_checksum(char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += (unsigned char)data[i];
    }
    return checksum;
}

int verify_checksum(char *data, size_t size, uint32_t expected_checksum) {
    uint32_t calculated_checksum = calculate_checksum(data, size);
    return (calculated_checksum == expected_checksum) ? 1 : 0;
}

void write_to_file(const char *filename, const char *data, size_t size) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }
    size_t written = fwrite(data, 1, size, file);
    if (written != size) {
        perror("Failed to write complete data to file");
    }
    fclose(file);
}

//Receiver setup
int main() {
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
    const int buffer_size = 1024;
    char buffer[buffer_size];
    printf("before block");
    // Assumes we have to receive the first message to get the header that contains important information
    multicast_setup_recv(m);
    if (multicast_check_receive(m) > 0) {
        multicast_receive(m, buffer, buffer_size);
        // Blocks until the first message is received
    }
    printf("after block");

    uint32_t init_seq = *(uint32_t *)&buffer[0];
    uint32_t init_file_ID = *(uint32_t *)&buffer[4];
    uint32_t total_chunks = *(uint32_t *)&buffer[8];

    const int chunk_size = (buffer_size - 12);
    char *ordered_msg;
    ordered_msg = (char *)malloc(total_chunks * chunk_size);

    // TODO: Add the initial chunk from the buffer above to the ordered_msg array
    for (int i = 1; i < total_chunks; i++) {
        if (multicast_check_receive(m) > 0) {
            multicast_receive(m, buffer, buffer_size);
            char *chunk = &buffer[5];
            uint32_t expected_checksum = *(uint32_t *)&buffer[buffer_size - 4];
            if (verify_checksum(chunk, chunk_size, expected_checksum) == 1) {
                uint32_t seq = *(uint32_t *)&buffer[0] % total_chunks;
                memcpy(&ordered_msg[seq], chunk, chunk_size);
            }
        }
    }
    // Call the function to write ordered_msg to a file
    write_to_file("output_file", ordered_msg, total_chunks * chunk_size);
}

