#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define N_CONNECTIONS 3

void get(int fd, char *filedir, char *filename);
void put(int fd, char *filename);
void list(int fd, char *filename);

int main(int argc, char *argv[]) {
    struct sockaddr_in info_server, info_client;
    int server, port, client;
    pid_t child;
    char *address_client, *command, *param;
    ssize_t nbytes;
    socklen_t address_client_len = sizeof(info_client);
    int i = 0;
    if (argc < 2) {
        perror("Error arguments");
        exit(EXIT_FAILURE);
    }
    sscanf(argv[1], "%d", &port);
    info_server.sin_family = AF_INET;
    info_server.sin_port = htons(port);
    info_server.sin_addr.s_addr = htonl(INADDR_ANY);
    server = socket(PF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    if (bind(server, (struct sockaddr *) &info_server, sizeof(struct sockaddr_in)) == -1) {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server, N_CONNECTIONS) == -1) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    for (;;) {
        client = accept(server, (struct sockaddr *) &info_client, &address_client_len);
        if (client == -1) {
            perror("Error accept");
            continue;
        }            
        address_client = (char *) malloc(sizeof(char) * BUFSIZE + 1);
        if (address_client == NULL) {
            perror("Error malloc");
            exit(EXIT_FAILURE);
        }
        sprintf(address_client, "New connection n° %d from: %s\n", ++i, inet_ntoa(info_client.sin_addr));
        if (write(STDOUT_FILENO, address_client, strlen(address_client)) < strlen(address_client)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
        if ((child = fork()) == -1) {
            perror("Error fork");
            exit(EXIT_FAILURE);
        }
        /* Processo figlio */
        if (child == 0) { 
            if (close(server) == -1) { /* Chiudo il file descriptor del socket */ 
                perror("Error close"); /* in listening non più utilizzato */
                exit(EXIT_FAILURE);
            }
            command = (char *) malloc(sizeof(char) * 4);
            if (command == NULL) {
                perror("Error malloc");
                exit(EXIT_FAILURE);
            }
            param = (char *) malloc(sizeof(char) * BUFSIZE + 1);
            if (param == NULL) {
                perror("Error malloc");
                exit(EXIT_FAILURE);
            }
            nbytes = read(client, command, 3);
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                if (write(STDERR_FILENO, "Client has sent an invalid command!\n", 36) < 36) {
                    perror("Error write");
                }
                exit(EXIT_FAILURE);
            }
            nbytes = read(client, param, BUFSIZE);
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                if (write(STDERR_FILENO, "No bytes read!\n", 16) < 16) {
                    perror("Error write");
                }
                exit(EXIT_FAILURE);
            }
            if (strcmp(command, "Get") == 0) {
                if (write(STDOUT_FILENO, "Get received...\n", 16) < 16) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(command, "Put") == 0) {
                if (write(STDOUT_FILENO, "Put received...\n", 16) < 16) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(command, "Lis") == 0) {
                if (write(STDOUT_FILENO, "List received...\n", 17) < 17) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
            free(command);
            free(param);
            _exit(EXIT_SUCCESS);
        } 
        free(address_client);
        /* Processo padre */
        if (close(client) == -1) {
            perror("Error close");
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}
