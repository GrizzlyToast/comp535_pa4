#include "multicast.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

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


    multicast_setup_recv(m);

    printf("Receiver ready. Waiting for data...\n");
    
    while (1) {
        if (multicast_check_receive(m) > 0) {
            int bytes_received = multicast_receive(m, buffer, buffer_size);
            if (bytes_received <= 0) {
                perror("Receive error");
                continue;
            }

            // Parse and print structured information if available
            if (bytes_received >= 12) {  // Minimum header size
                printf("Structured Data:\n");
                printf("Sequence: %u\n", *(uint32_t*)&buffer[0]);
                printf("File ID: %u\n", *(uint32_t*)&buffer[4]);
                printf("Total Chunks: %u\n", *(uint32_t*)&buffer[8]);
                
                if (bytes_received > 12) {
                    printf("Payload Size: %d bytes\n", bytes_received - 12);
                }
            }
            printf("----------------------------\n");
        } else {
            //usleep(100000);  // Sleep 100ms to prevent CPU spin
        }
    }

    //multicast_close(m);
    return 0;
}

