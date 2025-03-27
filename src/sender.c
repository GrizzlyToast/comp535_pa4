// Sender initiailization
#include "multicast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEG_SIZE 1024
#define HEADER_SIZE sizeof(struct Header)
#define MSS (SEG_SIZE - HEADER_SIZE)

struct Header {
    uint32_t sequence_number;
    uint32_t file_id;
    uint32_t total_chunks;
};

// function to calc the checksum by summing all the bytes of the data
uint32_t calculate_checksum(const char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += (unsigned char)data[i];
    }
    return checksum;
}

int main(int argc, char *argv[]) {
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);

    // check that there is at least 1 file
    if (argc != 2) {
        printf("Usage: %s <filename1> <filename2> <filename3> ...\n", argv[0]);
        return 1;
    }

    uint32_t file_id = 0;

    for (int i = 1; i < argc; i++) {
        // open file
        FILE *file = fopen(argv[i], "rb");
        if (file == NULL) {
            perror("Error opening file");
            return 1;
        }

        // define variables
        long file_size = ftell(file);
        uint32_t total_chunks = (file_size + MSS - 1) / MSS;
        char buffer[MSS];
        size_t bytesRead;
        uint32_t sequence_number = 0;

        // read bytes from file into chunks
        while ((bytesRead = fread(buffer, 1, MSS, file)) > 0) {
            // create header info 
            // TODO: checksum
            struct Header header;
            header.sequence_number = sequence_number;
            header.file_id = file_id;
            header.total_chunks = total_chunks;

            // combine the header with the chunk data
            char *chunk_data = malloc(SEG_SIZE);
            memcpy(chunk_data, &header, HEADER_SIZE);
            memcpy(chunk_data + HEADER_SIZE, buffer, MSS);

            // calc the checksum value
            uint32_t checksum = calculate_checksum(chunk_data, SEG_SIZE - sizeof(uint32_t));
            // inset the checksum value at the end of the segment
            memcpy(chunk_data + SEG_SIZE - sizeof(uint32_t), &checksum, sizeof(uint32_t));
    
            // multicast the segment
            multicast_send(m, chunk_data, SEG_SIZE);

            free(chunk_data);

            // increase seq num
            sequence_number++;
        }
        // close file
        fclose(file);

        // inc file id
        file_id++;
    }
    return 0;
}