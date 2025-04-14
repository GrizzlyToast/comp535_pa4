#include "multicast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>


#define CHUNK_SIZE 1024
#define MAX_FILENAME_LEN 32
#define HEADER_SIZE (MAX_FILENAME_LEN+20)
#define MAX_DATA_SIZE (CHUNK_SIZE - HEADER_SIZE - sizeof(uint32_t))

typedef struct FileReconstructor {
    char file_id[MAX_FILENAME_LEN];
    uint32_t total_chunks;
    uint32_t received_chunks;
    uint32_t received_bytes;
    char *file_buffer;
    struct FileReconstructor *next;
    bool is_done;
} FileReconstructor;

FileReconstructor *reconstructor_list = NULL;

FileReconstructor* create_reconstructor (const char *filename, uint32_t total_chunks) {
    FileReconstructor *new_rec = malloc(sizeof(FileReconstructor));
    if (!new_rec) return NULL;
    strncpy(new_rec->file_id, filename, MAX_FILENAME_LEN - 1);
    new_rec->file_id[MAX_FILENAME_LEN - 1] = '\0';

    new_rec->total_chunks = total_chunks;
    new_rec->received_chunks = 0;
    new_rec->received_bytes = 0;
    new_rec->is_done = false;

    new_rec->file_buffer = malloc(total_chunks * CHUNK_SIZE);
    if (!new_rec->file_buffer) {
        free(new_rec);
        return NULL;
    }
    memset(new_rec->file_buffer, 0, total_chunks * CHUNK_SIZE);

    new_rec->next = reconstructor_list;
    reconstructor_list = new_rec;

    return new_rec;
}

void process_chunk(FileReconstructor *rec, uint32_t seq_num, const char *chunk_data, uint32_t data_size, uint32_t files_left, uint32_t total_files) {
    if (seq_num >= rec->total_chunks*CHUNK_SIZE) {
        fprintf(stderr, "Invalid sequence %u for file %s\n", seq_num, rec->file_id);
        return;
    }

    if (rec->file_buffer[seq_num] == '\0') {
        memcpy(&rec->file_buffer[seq_num], 
               chunk_data, 
               data_size);
        rec->received_chunks++;
        rec->received_bytes += data_size;
        
        printf("File %s: %u/%u chunks (%.1f%%), Files downloaded: %u, Files left: %u\n", 
               rec->file_id,
               rec->received_chunks,
               rec->total_chunks,
               (float)rec->received_chunks/rec->total_chunks*100,
                total_files - files_left,
                files_left);
    }
}

FileReconstructor* find_reconstructor(char *file_id) {
    FileReconstructor *current = reconstructor_list;
    while (current != NULL) {
        if (strcmp(current->file_id, file_id) == 0) { 
            return current;
        }
        current = current->next;
    }
    return NULL;
}

const char *get_basename(const char *path) {
    const char *base = strrchr(path, '/');
    return base ? base + 1 : path;
}

void save_and_remove(FileReconstructor *rec, uint32_t *files_left) {
    const char *filename = get_basename(rec->file_id);
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
    }
    if (file) {
        fwrite(rec->file_buffer, 1, rec->received_bytes, file);
        fclose(file);
        printf("Saved %s (%u chunks)\n", rec->file_id, rec->total_chunks);
        (*files_left)--;
    }

    rec->is_done = true;
    
    // Remove from list
    // if (reconstructor_list == rec) {
    //     reconstructor_list = rec->next;
    // } else {
    //     FileReconstructor *prev = reconstructor_list;
    //     while (prev && prev->next != rec) prev = prev->next;
    //     if (prev) prev->next = rec->next;
    // }
    
    free(rec->file_buffer);
    //free(rec);
}

uint32_t calculate_checksum(const char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += (unsigned char)data[i];
    }
    return checksum;
}

void* send_connection(void* arg) {
    uint32_t id = *(uint32_t*)arg;
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
    // send join msg to sender
    char *chunk = malloc(CHUNK_SIZE);
        
    uint32_t type = 1;
    memcpy(chunk, &type, 4);
    uint32_t join = 1;
    memcpy(chunk + 4, &join, 4);
    memcpy(chunk + 8, &id, 4);

    multicast_send(m, chunk, CHUNK_SIZE);
    free(chunk);

    printf("sent connection\n");

    return NULL;
}

void* send_disconnection(void* arg) {
    uint32_t id = *(uint32_t*)arg;
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
    // send join msg to sender
    char *chunk = malloc(CHUNK_SIZE);
        
    uint32_t type = 1;
    memcpy(chunk, &type, 4);
    uint32_t join = 0;
    memcpy(chunk + 4, &join, 4);
    memcpy(chunk + 8, &id, 4);

    multicast_send(m, chunk, CHUNK_SIZE);
    free(chunk);

    printf("sent disconnection\n");

    return NULL;
}

void* send_update(void* arg) {
    uint32_t id = *(uint32_t*)arg;
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
    // send join msg to sender
    char *chunk = malloc(CHUNK_SIZE);
        
    uint32_t type = 1;
    memcpy(chunk, &type, 4);
    uint32_t join = 2;
    memcpy(chunk + 4, &join, 4);
    memcpy(chunk + 8, &id, 4);

    multicast_send(m, chunk, CHUNK_SIZE);
    free(chunk);

    printf("sent update\n");

    return NULL;
}

int main(int argc, char *argv[]) {
    // process args
    if (argc != 2) {
        printf("Usage: %s <id>\n", argv[0]);
        return 1;
    }

    uint32_t recv_id = (uint32_t) strtoul(argv[1], NULL, 10);

    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);

    unsigned char buffer[CHUNK_SIZE];
    uint32_t files_left = -1;
    uint32_t total_files = -1;

    // start thread to send connection
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, send_connection, &recv_id) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    multicast_setup_recv(m);
    printf("Receiver ready. Processing multiple files...\n");

    while (1) {
        if (multicast_check_receive(m) > 0) {
            int bytes_received = multicast_receive(m, buffer, CHUNK_SIZE);
            if (bytes_received <= 0) continue;

            // Parse header
            uint32_t type = *(uint32_t*)&buffer[0];
            // check if type is data or control
            if (type == 1) {
                continue;
            }
            uint32_t seq_num = *(uint32_t*)&buffer[4];

            char file_id[MAX_FILENAME_LEN + 1];
            strncpy(file_id, (const char*)&buffer[8], MAX_FILENAME_LEN);
            uint32_t total_chunks = *(uint32_t*)&buffer[40];
            uint32_t num_files = *(uint32_t*)&buffer[44];
            uint32_t data_size = *(uint32_t*)&buffer[48];
            uint32_t checksum = *(uint32_t*)&buffer[CHUNK_SIZE-4];

            // Verify checksum
            if (calculate_checksum((const char *)buffer, CHUNK_SIZE - sizeof(uint32_t)) != checksum) {
                fprintf(stderr, "Bad checksum for file %s chunk %u\n", file_id, seq_num);
                continue;
            }

            if (files_left == -1) {
                // init num files
                files_left = num_files;
                total_files = num_files;
            }

            if (files_left == 0) {
                // finished reading files, stop
                printf("finished reading files.\n");
                break;
            }

            // Find or create reconstructor
            FileReconstructor *rec = find_reconstructor(file_id);
            if (!rec) {
                rec = create_reconstructor(file_id, total_chunks);
                printf("New file detected: ID=%s (%u chunks)\n", file_id, total_chunks);
            }

            // check if file is done
            if (rec->is_done) {
                continue;
            }

            // Process chunk
            process_chunk(rec, seq_num, (char*)&buffer[HEADER_SIZE], data_size, files_left, total_files);

            // Check completion
            if (rec->received_chunks == rec->total_chunks) {
                save_and_remove(rec, &files_left);

                // send update packet
                pthread_t thread_id2;
                if (pthread_create(&thread_id2, NULL, send_update, &recv_id) != 0) {
                    perror("Failed to create thread");
                    return 1;
                }
            }

        }
    }

    // start thread to send disconnection
    pthread_t thread_id3;
    if (pthread_create(&thread_id3, NULL, send_disconnection, &recv_id) != 0) {
        perror("Failed to create thread");
        return 1;
    }
    sleep(1);
    return 0;
}