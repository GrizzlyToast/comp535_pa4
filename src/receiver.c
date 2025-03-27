#include "multicast.h"

int verify_checksum(unsigned char *data, size_t size, unsigned char expected_checksum) {
    unsigned char calculated_checksum = checksum(data, size);
    return (calculated_checksum == expected_checksum) ? 1 : 0;
}

//Receiver setup
int main() {
    const mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
    const int buffer_size = 1024;
    char buffer[buffer_size];

    // Assumes we have to receive the first message to get the header that contains important information
    multicast_setup_recv(m);
    if (multicast_check_receive(m) > 0) {
        multicast_receive(m, buffer, buffer_size);
        // Blocks until the first message is received
    }

    int init_seq = buffer[0];
    int init_file_ID = buffer[1];
    int init_checksum = buffer[2];
    int total_chunks = buffer[3];

    const int chunk_size = (buffer_size - 16);
    char *ordered_msg;
    ordered_msg = (char *)malloc(total_chunks * chunk_size);

    // TODO: Add the initial chunk from the buffer above to the ordered_msg array
    for (int i = 0; i < total_chunks; i++) {
        if (multicast_check_receive(m) > 0) {
            multicast_receive(m, buffer, buffer_size);
            if (verify_checksum(&buffer[5], chunk_size, buffer[2]) == 1) {
                int seq = buffer[0] % total_chunks;
                memcpy(&ordered_msg[seq], &buffer[5], chunk_size);
            }
        }
    }



}

