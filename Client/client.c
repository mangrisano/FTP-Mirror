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
    int server, port;
    char *command, *params, type[3];
    int found, f;
    char *f_content, *filename;
    ssize_t nbytes;
    int i;
    const char *error = "Command not found!\n";
    if (argc < 4) {
        perror("Error arguments");
        exit(EXIT_FAILURE);
    }
    sscanf(argv[3], "%d", &port);
    info_server.sin_family = AF_INET;
    info_server.sin_port = htons(port);
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
        nbytes = read(server, type, 3);
        if (nbytes == -1) {
            perror("Error read");
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) {
            exit(EXIT_SUCCESS);
        }
        if (strcmp(type, "REG") == 0) {
            f_content = (char *) malloc(sizeof(char) * BUFSIZE + 1);
            if (f_content == NULL) {
                perror("Error malloc");
                exit(EXIT_FAILURE);
            }
            f = open(params, O_CREAT | O_WRONLY, 0644);
            if (f == -1) {
                perror("Error open");
                exit(EXIT_FAILURE);
            }
            nbytes = read(server, f_content, BUFSIZE);
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                perror("No more bytes");
                exit(EXIT_SUCCESS);
            }
            if (write(f, f_content, nbytes) < nbytes) {
                perror("Error write");
                exit(EXIT_SUCCESS);
            }
            if (write(STDOUT_FILENO, "File received with success!\n", 28) < 28) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            free(f_content);
            if (close(f) == -1) {
                perror("Error closing file");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(type, "DIR") == 0) {
            for (i = 0; i < 3; i++) {
                filename = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                if (filename == NULL) {
                    perror("Error malloc");
                    exit(EXIT_FAILURE);
                }
                nbytes = read(server, filename, BUFSIZE);
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    exit(EXIT_SUCCESS);
                }
                write(STDOUT_FILENO, filename, nbytes);
                write(STDOUT_FILENO, "\n", 1);
                free(filename);
            }
        }
    }
    else {
        write(STDOUT_FILENO, "File not found!\n", 16);
    }
    free(command);
    free(params);
    exit(EXIT_SUCCESS);
}

