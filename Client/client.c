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
    struct sockaddr_in info_server;             /* Connections informations */
    int server, port;                           /* Server params */
    char command[5], *params, type[3];
    int found = 0;                              /* 1 if filename is found; 0 otherwise */
    int f;                                      /* Filedescritor for the file that has to be creat */
    int i;
    char *f_content;                            /* Content of the file */
    char *filename;
    size_t lenfilename = 0;
    size_t lenfcontent = 0;
    ssize_t nbytes;                             /* Num of bytes read */
    const char *error = "Command not found!\n";
    if (argc < 4) {
        perror("Error arguments");
        exit(EXIT_FAILURE);
    }
    sscanf(argv[3], "%d", &port);
    /* Set server's params */
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
    /* Connection to the server */
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
    /* 1 if the file is found; 0 otherwise */
    read(server, &found, sizeof(found));
    if (found == 1) {
        /* Receive the type of the file */
        nbytes = read(server, type, 3);
        if (nbytes == -1) {
            perror("Error read");
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) {
            exit(EXIT_SUCCESS);
        }
        /* Check the type of the file */
        if (strcmp(type, "REG") == 0) {
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
            nbytes = read(server, &lenfcontent, sizeof(lenfcontent));
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                exit(EXIT_SUCCESS);
            }
            /* Read the content of the file */
            nbytes = read(server, f_content, lenfcontent);
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                perror("No more bytes");
                exit(EXIT_SUCCESS);
            }
            if (write(f, f_content, lenfcontent) < lenfcontent) {
                perror("Error write");
                exit(EXIT_SUCCESS);
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
            for (i = 0; i < 3; i++) {
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
                nbytes = read(server, &lenfilename, sizeof(lenfilename));
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    exit(EXIT_SUCCESS);
                }
                /* Read the filename */
                nbytes = read(server, filename, lenfilename);
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
                nbytes = read(server, &lenfcontent, sizeof(lenfcontent));
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    exit(EXIT_SUCCESS);
                }
                /* Read the file content from the server */
                nbytes = read(server, f_content, lenfcontent);
                if (nbytes == -1) {
                    perror("Error read file content");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    exit(EXIT_SUCCESS);
                }
                /* Write the content on the file */
                if (write(f, f_content, nbytes) < nbytes) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                /* while ((nbytes = read(server, f_content, 100)) != 0) {
                    if (nbytes == -1) {
                        perror("Error read file content");
                        exit(EXIT_FAILURE);
                    }
                    if (write(f, f_content, 100) < 100) {
                        perror("Error write file content");
                        exit(EXIT_FAILURE);
                    }
                } */
                write(STDOUT_FILENO, filename, strlen(filename));
                write(STDOUT_FILENO, "\n", 1);
                /* Free memory */
                free(f_content);
                free(filename);
                if (close(f) == -1) {
                    perror("Error close file");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    else {
        write(STDOUT_FILENO, "File not found!\n", 16);
    }
    /* Free the memory */
    free(params);
    exit(EXIT_SUCCESS);
}

