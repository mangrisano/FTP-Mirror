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

void get(int fd, char *params);

int main(int argc, char *argv[]) {
    struct sockaddr_in info_server;             /* Connections informations */
    int server, port;                           /* Server params */
    char command[5], *params;
    const char *error = "Command not found!\n";
    if (argc < 4) {
        perror("Error arguments");
        exit(EXIT_FAILURE);
    }
    sscanf(argv[3], "%d", &port);
    /* Set fd's params */
    info_server.sin_family = AF_INET;
    info_server.sin_port = htons(port);
    inet_aton("127.0.0.1", &info_server.sin_addr);
    server = socket(PF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        perror("Error socket");
        exit(EXIT_FAILURE);
    }
    params = (char *) malloc(sizeof(char) * BUFSIZE + 1);
    if (params == NULL) {
        perror("Error malloc");
        exit(EXIT_FAILURE);
    }
    if (strlen(argv[1]) >= sizeof(command)) {
        perror("Error: the length of the command is too big. Memory leak");
        exit(EXIT_FAILURE);
    }
    if (strlen(argv[2]) >= BUFSIZE) {
        perror("Error: the length of the param is too big. Memory leak");
        exit(EXIT_FAILURE);
    }
    strcpy(command, argv[1]);
    strcpy(params, argv[2]);
    /* Connection to the fd */
    if (connect(server, (struct sockaddr *) &info_server, sizeof(struct sockaddr_in)) == -1) {
        perror("Error connect");
        exit(EXIT_FAILURE);
    }
    if (strcmp(command, "Get") == 0) {
            if (write(server, command, strlen(command)) < strlen(command)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            if (write(server, params, strlen(params)) < strlen(params)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            get(server, params);
    }
    else if (strcmp(command, "Put") == 0) {
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
    free(params);
    if (close(server) == -1) {
        perror("Error close server");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void get(int fd, char *params) {    
    int f;                                      /* Filedescritor for the file that has to be creat */
    int found = 0;
    char *f_content;                            /* Content of the file */
    char *filename;
    char type[3];
    char flag = '+';                            /* Flag to set the end of transfer */
    char flag_f = '#';                          /* Flag to say if there are more file regular */
    size_t lenfilename = 0;
    ssize_t nbytes;                             /* Num of bytes read */
    size_t bytewr;  
    /* 1 if the file is found; 0 otherwise */
    read(fd, &found, sizeof(found));
    if (found == 1) {
        /* Receive the type of the file */
        nbytes = read(fd, type, 3);
        printf("type: %s\n", type);
        if (nbytes == -1) {
            perror("Error read");
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) {
            exit(EXIT_SUCCESS);
        }
        /* Check the type of the file */
        if (strcmp(type, "REG") == 0) {
            printf("type is reg\n");
            f_content = (char *) malloc(sizeof(char) * BUFSIZE + 1);
            if (f_content == NULL) {
                perror("Error malloc");
                exit(EXIT_FAILURE);
            }
            /* Create the file if doesn't exist in write-only */
            f = open(params, O_CREAT | O_WRONLY, 0644);
            if (f == -1) {
                perror("Error open");
                exit(EXIT_FAILURE);
            }
            /* Read the content of the file */
            while ((nbytes = read(fd, &flag, sizeof(flag))) != 0) {
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (flag == '-') {                    
                    nbytes = read(fd, &bytewr, sizeof(bytewr));
                    if (nbytes == -1) {
                        perror("Error read");
                        exit(EXIT_SUCCESS);
                    }
                    if (nbytes == 0) {
                        exit(EXIT_FAILURE);
                    }
                    nbytes = read(fd, f_content, bytewr);
                    if (nbytes == -1) {
                        perror("Error read file content");
                        exit(EXIT_FAILURE);
                    }
                    if (nbytes == 0) {
                        exit(EXIT_SUCCESS);
                    }
                    if (write(f, f_content, bytewr) < bytewr) {

                        perror("Error write");
                        exit(EXIT_SUCCESS);
                    }
                }
                else {
                    break;
                }
            }
            if (write(STDOUT_FILENO, "File received with success!\n", 28) < 28) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            /* Free memory */
            free(f_content);
            if (close(f) == -1) {
                perror("Error closing file");
                exit(EXIT_FAILURE);
            }
        }
        /* Check the type of the file */
        else if (strcmp(type, "DIR") == 0) {
            for (;;) {
                nbytes = read(fd, &flag_f, sizeof(flag_f));
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    exit(EXIT_SUCCESS);
                }
                if (flag_f == 'f') {
                    filename = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                    if (filename == NULL) {
                        perror("Error malloc");
                        exit(EXIT_FAILURE);
                    }                
                    f_content = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                    if (f_content == NULL) {
                        perror("Error malloc");
                        exit(EXIT_FAILURE);
                    }
                    /* Read the right length of the filename */
                    nbytes = read(fd, &lenfilename, sizeof(lenfilename));
                    if (nbytes == -1) {
                        perror("Error read");
                        exit(EXIT_FAILURE);
                    }
                    if (nbytes == 0) {
                        exit(EXIT_SUCCESS);
                    }
                    /* Read the filename */
                    nbytes = read(fd, filename, lenfilename);
                    if (nbytes == -1) {
                        perror("Error read filename");
                        exit(EXIT_FAILURE);
                    }
                    if (nbytes == 0) {
                        exit(EXIT_SUCCESS);
                    }
                    /* Open the file, create if it doesn't exists */
                    f = open(filename, O_CREAT | O_WRONLY, 0644);
                    if (f == -1) {
                        perror("Error open file");
                        exit(EXIT_FAILURE);
                    }
                    while ((nbytes = read(fd, &flag, sizeof(flag))) != 0) {
                        if (nbytes == -1) {
                            perror("Error write");
                            exit(EXIT_FAILURE);
                        }
                        /* Checking if there are other byte to receive */
                        if (flag == '-') {
                            nbytes = read(fd, &bytewr, sizeof(bytewr));
                            if (nbytes == -1) {
                                perror("Error read");
                                exit(EXIT_FAILURE);
                            }
                            if (nbytes == 0) {
                                exit(EXIT_SUCCESS);
                            }
                            nbytes = read(fd, f_content, bytewr);
                            if (nbytes == -1) {
                                perror("Error read file content");
                                exit(EXIT_FAILURE);
                            }
                            if (nbytes == 0) {
                                exit(EXIT_SUCCESS);
                            }
                            /* Write the content on the file */
                            if (write(f, f_content, bytewr) < bytewr) {
                                perror("Error write file content");
                                exit(EXIT_FAILURE);
                            }
                        }
                        /* No more bytes */
                        else {
                            break;
                        }
                    }
                    /* Free memory */
                    free(f_content);
                    free(filename);
                    if (close(f) == -1) {
                        perror("Error close file");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (flag_f == 'd') {
                    continue;
                }
                else {
                    break;
                }
                if (write(STDOUT_FILENO, "File received with success|\n", 28) < 28) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            printf("Warning\n");
        }
    }
    else {
        write(STDOUT_FILENO, "File not found!\n", 16);
        exit(EXIT_FAILURE);
    }
}

