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
#define PENDING_QUEUE 5

void get(int fd, char *filedir, char *filename);
void put(int fd, char *filename);
void list(int fd, char *filename);

int main(int argc, char *argv[]) {
    struct sockaddr_in info_server, info_client;
    int server, port, client;
    pid_t child;
    char *command, *param;
    ssize_t nbytes;
    socklen_t address_client_len = sizeof(info_client);
    if (argc < 3) {
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
    if (listen(server, PENDING_QUEUE) == -1) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    for (;;) {
        client = accept(server, (struct sockaddr *) &info_client, &address_client_len);
        if (client == -1) {
            perror("Error accept");
            continue;
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
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            nbytes = read(client, param, BUFSIZE);
            write(STDOUT_FILENO, param, strlen(param));
            write(STDOUT_FILENO, "\n", 1);
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
                get(client, argv[2], param); 

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
        /* Processo padre */
        if (close(client) == -1) {
            perror("Error close");
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}

void get(int fd, char *filedir, char *filename) {
    DIR *dp;
    int found = 0;
    struct dirent *dc;
    struct stat buf;
    int f;
    ssize_t nbytes;
    char *f_content;
    dp = opendir(filedir);
    if (dp == NULL) {
        perror("Error opendir");
        exit(EXIT_FAILURE);
    }
    while ((dc = readdir(dp)) != NULL) {
        if (strcmp(dc->d_name, filename) == 0) {
            found = 1;
            if (write(fd, &found, sizeof(found)) < sizeof(found)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            if (stat(dc->d_name, &buf) == -1) {
                perror("Error stat");
                exit(EXIT_FAILURE);
            }
            if (S_ISREG(buf.st_mode)) {
                if (write(fd, "REG", 3) < 3) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                f = open(dc->d_name, O_RDONLY);
                if (f == -1) {
                    perror("Error open file");
                    exit(EXIT_FAILURE);
                }
                f_content = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                if (f_content == NULL) {
                    perror("Error malloc");
                    exit(EXIT_FAILURE);
                }
                nbytes = read(f, f_content, BUFSIZE);
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    perror("No more bytes\n");
                    exit(EXIT_SUCCESS);
                }
                write(fd, f_content, nbytes);
                if (close(f) == -1) {
                    perror("Error closing file");
                    exit(EXIT_FAILURE);
                }
                free(f_content);
            }
            exit(EXIT_SUCCESS);
        }
    }
    if (closedir(dp) == -1) {
        perror("Error close");
        exit(EXIT_FAILURE);
    }
}
                


