#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    struct sockaddr_in info_server;
    int server;
    char *command, *params, type[4];
    int found;
    ssize_t nbytes;
    const char *error = "Command not found!\n";
    if (argc < 3) {
        perror("Error arguments");
        exit(EXIT_FAILURE);
    }
    info_server.sin_family = AF_INET;
    info_server.sin_port = htons(5200);
    inet_aton("127.0.0.1", &info_server.sin_addr);
    server = socket(PF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    command = (char *) malloc(sizeof(char) * BUFSIZE + 1);
    if (command == NULL) {
        perror("Error malloc");
        exit(EXIT_FAILURE);
    }
    params = (char *) malloc(sizeof(char) * BUFSIZE + 1);
    if (params == NULL) {
        perror("Error malloc");
        exit(EXIT_FAILURE);
    }
    strcpy(command, argv[1]);
    strcpy(params, argv[2]);
    if (connect(server, (struct sockaddr *) &info_server, sizeof(struct sockaddr_in)) == -1) {
        perror("Error connect");
        exit(EXIT_FAILURE);
    }
    if ((strcmp(command, "Get") == 0) || (strcmp(command, "Put") == 0)) {         
        if (write(server, command, strlen(command)) < strlen(command)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
        if (write(server, params, strlen(params)) < strlen(params)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "List") == 0) {
            if (write(server, command, strlen(command) - 1) < strlen(command) - 1) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            if (write(server, params, strlen(params)) < strlen(params)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
    }
    else {
        if (write(STDERR_FILENO, error, strlen(error)) < strlen(error)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    read(server, &found, sizeof(found));
    if (found == 1) {
        if (write(STDOUT_FILENO, "Found\n", 6) < 6) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
        nbytes = read(server, type, 4);
        if (nbytes == -1) {
            perror("Error read");
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) {
            exit(EXIT_SUCCESS);
        }
        if (write(STDOUT_FILENO, type, strlen(type)) < strlen(type)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
    else {
        write(STDOUT_FILENO, "no\n", 3);
    }
    free(command);
    free(params);
    exit(EXIT_SUCCESS);
}

