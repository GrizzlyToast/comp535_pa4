// Sender initiailization
#include "multicast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define CHUNK_SIZE 1024
#define MAX_FILENAME_LEN 32
#define HEADER_SIZE (MAX_FILENAME_LEN+20)
#define MAX_DATA_SIZE (CHUNK_SIZE - HEADER_SIZE - sizeof(uint32_t))

typedef struct Client {
    uint32_t id;
    uint32_t files_downloaded;
    struct Client *next;
} Client;

Client *client_list = NULL;
volatile int count = 30;
volatile int clients = 0;

void add_client(uint32_t id) {
    // Create a new client
    Client *new_client = malloc(sizeof(Client));
    if (new_client == NULL) {
        perror("Failed to allocate memory for new client");
        return;
    }

    // Initialize the new client
    new_client->id = id;
    new_client->files_downloaded = 0;
    new_client->next = client_list;
    client_list = new_client;
}

void update_client(uint32_t id) {
    Client *current = client_list;
    while (current != NULL) {
        if (current->id == id) {
            current->files_downloaded += 1;
        }
        current = current->next;
    }
}

void print_clients(int num_clients, int time) {
    printf("Ending session in %ds\n", time);
    printf("Active Clients: %d\n", num_clients);

    printf("ID   files_downloaded\n");
    int c = 0;
    Client *current = client_list;
    while (current != NULL) {
        c++;
        printf("%d    %d\n", current->id, current->files_downloaded);
        current = current->next;
    }
    printf("\033[%dA", c + 3);
}

// function to calc the checksum by summing all the bytes of the data
uint32_t calculate_checksum(const char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += (unsigned char)data[i];
    }
    return checksum;
}



void* listen_for_clients() {
    //printf("Starting file distribution service\n\n");
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
    multicast_setup_recv(m);
    unsigned char buffer[CHUNK_SIZE];

    while (1) {
        if (multicast_check_receive(m) > 0) {
            int bytes_received = multicast_receive(m, buffer, CHUNK_SIZE);
            if (bytes_received <= 0) continue;

            uint32_t type = *(uint32_t*)&buffer[0];
            uint32_t join = *(uint32_t*)&buffer[4];
            uint32_t id = *(uint32_t*)&buffer[8];

            if (type == 1) {
                if (join == 1) {
                    clients++;
                    add_client(id);
                    //printf("\r\033[KActive Clients: %d", clients);
                    print_clients(clients, count);
                    fflush(stdout);
                    count = 30;
                }
                else if (join == 2) {
                    update_client(id);
                    print_clients(clients, count);
                }
                else {
                    clients--;
                    fflush(stdout);
                }
            }
        }
    }
    return NULL;
}

void* countdown () {
    while (count >= 0) {
        if (clients == 0) {
            //printf("\rEnding session in %ds ", count);
            //fflush(stdout);
            print_clients(clients, count);
            count--;
        }
        sleep(1);
    }
    printf("\r\033[Kquitting...\n");
    exit(1);
    return NULL;
}

int main(int argc, char *argv[]) {
    mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);

    // check that there is at least 1 file
    if (argc < 2) {
        printf("Usage: %s <filename1> <filename2> <filename3> ...\n", argv[0]);
        return 1;
    }

    // start thread to listen for clients
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, listen_for_clients, NULL) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    // start thread to listen for clients
    pthread_t thread_id2;
    if (pthread_create(&thread_id2, NULL, countdown, NULL) != 0) {
        perror("Failed to create thread");
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
                uint32_t type = 0;
                memcpy(chunk, &type, 4);

                memcpy(chunk + 4, &sequence_number, 4);

                strncpy(chunk + 8, argv[i], MAX_FILENAME_LEN - 1);
                chunk[8 + MAX_FILENAME_LEN - 1] = '\0';

                memcpy(chunk+8+MAX_FILENAME_LEN, &total_chunks, 4);

                memcpy(chunk+MAX_FILENAME_LEN+12, &num_files, 4);

                memcpy(chunk+MAX_FILENAME_LEN+16, &bytesRead, 4);

                memcpy(chunk+MAX_FILENAME_LEN+20, buffer, bytesRead);


                // calc the checksum value
                uint32_t checksum = calculate_checksum(chunk, CHUNK_SIZE - sizeof(uint32_t));
                // inset the checksum value at the end of the segment
                memcpy(chunk+CHUNK_SIZE-4, &checksum, 4);

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