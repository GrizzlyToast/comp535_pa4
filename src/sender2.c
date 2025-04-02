// Sender initiailization
#include "multicast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CHUNK_SIZE 1024
#define MAX_FILENAME_LEN 32
#define HEADER_SIZE (MAX_FILENAME_LEN+16)
#define MAX_DATA_SIZE (CHUNK_SIZE - HEADER_SIZE - sizeof(uint32_t))

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
    if (argc < 2) {
        printf("Usage: %s <filename1> <filename2> <filename3> ...\n", argv[0]);
        return 1;
    }

    while(1) {

        uint32_t file_id = 0;

        for (int i = 1; i < argc; i++) {
            // open file
            FILE *file = fopen(argv[i], "rb");
            if (file == NULL) {
                perror("Error opening file");
                return 1;
            }

            // define variables
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);        
            uint32_t total_chunks = (file_size + MAX_DATA_SIZE - 1) / MAX_DATA_SIZE;
            if (total_chunks == 0 && file_size > 0) {
                total_chunks = 1;
            }
            char buffer[MAX_DATA_SIZE];
            size_t bytesRead;
            uint32_t sequence_number = 0;
            uint32_t num_files = argc - 1;

            // read bytes from file into chunks
            while ((bytesRead = fread(buffer, 1, MAX_DATA_SIZE, file)) > 0) {              
                // create chunk and add data
                char *chunk = malloc(CHUNK_SIZE);

                // add header data
                memcpy(chunk, &sequence_number, 4);

                strncpy(chunk + 4, argv[i], MAX_FILENAME_LEN - 1);
                chunk[4 + MAX_FILENAME_LEN - 1] = '\0';

                memcpy(chunk+4+MAX_FILENAME_LEN, &total_chunks, 4);

                memcpy(chunk+MAX_FILENAME_LEN+8, &num_files, 4);

                memcpy(chunk+MAX_FILENAME_LEN+12, &bytesRead, 4);

                memcpy(chunk+MAX_FILENAME_LEN+16, buffer, bytesRead);


                // calc the checksum value
                uint32_t checksum = calculate_checksum(chunk, CHUNK_SIZE - sizeof(uint32_t));
                // inset the checksum value at the end of the segment
                memcpy(chunk+CHUNK_SIZE-4, &checksum, 4);
        
                printf("Sequence: %u\n", *(uint32_t*)&chunk[0]);
                printf("Filename: %s\n", &chunk[4]); 
                printf("Total Chunks: %u\n", *(uint32_t*)&chunk[36]);
                printf("num files: %u\n", *(uint32_t*)&chunk[40]);
                printf("data size: %u\n", *(uint32_t*)&chunk[44]);
                printf("checksum: %u\n", *(uint32_t*)&chunk[CHUNK_SIZE - 4]);


                printf("----------------------------\n");

                // multicast the segment
                multicast_send(m, chunk, CHUNK_SIZE);
                free(chunk);

                // increase seq num
                sequence_number += bytesRead;
            }
            // close file
            fclose(file);

            // inc file id
            file_id++;
        }
    }
    return 0;
}