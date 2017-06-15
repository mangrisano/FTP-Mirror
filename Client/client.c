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

void get(int fd);
void list(int fd);

int main(int argc, char *argv[]) {
    struct sockaddr_in info_server;             /* Connections informations */
    int server, port;                           /* Server params */
    char command[5], *param;
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
    param = (char *) malloc(sizeof(char) * BUFSIZE + 1);
    if (param == NULL) {
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
    strcpy(param, argv[2]);
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
            if (write(server, param, strlen(param)) < strlen(param)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            get(server);
    }
    else if (strcmp(command, "Put") == 0) {
        if (write(server, command, strlen(command)) < strlen(command)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
        if (write(server, param, strlen(param)) < strlen(param)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "List") == 0) {
            if (write(server, command, strlen(command) - 1) < strlen(command) - 1) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            if (write(server, param, strlen(param)) < strlen(param)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            list(server);
    }
    else {
        if (write(STDERR_FILENO, error, strlen(error)) < strlen(error)) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    free(param);
    if (close(server) == -1) {
        perror("Error close server");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void get(int fd) {    
    int f;                                      /* File descriptor for the file that has to be created */
    int found = 0;
    char *f_content;                            /* Content of the file */
    char *filename;
    char type[4];
    char flag = '+';                            /* Flag to set the end of transfer */
    char flag_f = '#';                          /* Flag to say if there are more file regular */
    size_t lenfilename = 0;
    ssize_t nbytes;                             /* Num of bytes read */
    size_t bytewr;                              /* Bytes to write */ 
    /* 1 if the file is found; 0 otherwise */
    nbytes = read(fd, &found, sizeof(found));
    if (nbytes == -1) {
        perror("Error read");
        exit(EXIT_FAILURE);
    }
    if (found == 1) {
        /* Receive the type of the file */
        nbytes = read(fd, type, 3);
        if (nbytes == -1) {
            perror("Error read");
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) {
            exit(EXIT_SUCCESS);
        }
        type[nbytes] = '\0';
        /* If the file is a regular file */
        if (strcmp(type, "REG") == 0) {
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
            nbytes = read(fd, &lenfilename, sizeof(lenfilename));
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                exit(EXIT_SUCCESS);
            }
            nbytes = read(fd, filename, lenfilename);
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                exit(EXIT_SUCCESS);
            }
            filename[nbytes] = '\0';
            /* Create the file if doesn't exist in write-only */
            f = open(filename, O_CREAT | O_WRONLY, 0644);
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
            free(filename);
            if (close(f) == -1) {
                perror("Error closing file");
                exit(EXIT_FAILURE);
            }
        }
        /* If the file is a directory */
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
                /* It's a file */
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
                    filename[nbytes] = '\0';
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
                        /* Check if there are other bytes to receive */
                        if (flag == '-') {
                            /* Read the bytes to write */
                            nbytes = read(fd, &bytewr, sizeof(bytewr));
                            if (nbytes == -1) {
                                perror("Error read");
                                exit(EXIT_FAILURE);
                            }
                            if (nbytes == 0) {
                                exit(EXIT_SUCCESS);
                            }
                            /* Write the content on the file */
                            nbytes = read(fd, f_content, bytewr);
                            if (nbytes == -1) {
                                perror("Error read file content");
                                exit(EXIT_FAILURE);
                            }
                            if (nbytes == 0) {
                                exit(EXIT_SUCCESS);
                            }
                            f_content[nbytes] = '\0';
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
                    /* Close the file */
                    if (close(f) == -1) {
                        perror("Error close file");
                        exit(EXIT_FAILURE);
                    }
                }
                /* It's a directory */
                else if (flag_f == 'd') {
                    continue;
                }
                /* No more files */
                else {
                    if (flag_f == 'e') {
                        break;
                    }
                }
                if (write(STDOUT_FILENO, "File received with success|\n", 28) < 28) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            if (write(STDOUT_FILENO, "The file is not a regular file/directory!\n", 42) < 42) {
                    perror("Error write");
                    exit(EXIT_SUCCESS);
            }
        }
    }
    else {
        if (write(STDOUT_FILENO, "File not found!\n", 16) < 16) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
}

void list(int fd) {
    off_t sizefilename;                       /* Size in byte of the filename */
    ssize_t nbytes;                           /* Number of bytes read */
    size_t lenfilename;                       /* Length of the filename */
    char buf[128];                            /* Message to output */
    char type[4];                             /* Type of the file: REG or DIR */
    char flag_f = '#';                        /* Flag to say if there is a regular file */
    char *filename;
    int found = 0;                            /* 1 if file is found; 0 otherwise */
    nbytes = read(fd, &found, sizeof(found));
    if (nbytes == -1) {
        perror("Error read");
        exit(EXIT_FAILURE);
    }
    /* If file is found */
    if (found == 1) {
        nbytes = read(fd, type, 3);
        if (nbytes == -1) {
            perror("Error read");
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) {
            exit(EXIT_SUCCESS);
        }
        type[nbytes] = '\0';
        /* If the file is a regular file */
        if (strcmp(type, "REG") == 0) {
            nbytes = read(fd, &sizefilename, sizeof(sizefilename));
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                exit(EXIT_SUCCESS);
            }
            sprintf(buf, "The size of the file is %lld\n", sizefilename);
            if (write(STDOUT_FILENO, buf, strlen(buf)) < strlen(buf)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
        }
        /* If the file is a directory */
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
                /* It's a file */
                if (flag_f == 'f') {
                    if (nbytes == -1) {
                        perror("Error read");
                        exit(EXIT_FAILURE);
                    }
                    filename = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                    if (filename == NULL) {
                        perror("Error malloc");
                        exit(EXIT_FAILURE);
                    }
                    /* Read the length of the filename */
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
                        perror("Error read");
                        exit(EXIT_FAILURE);
                    }
                    filename[nbytes] = '\0';
                    /* Read the size of the file in byte */
                    nbytes = read(fd, &sizefilename, sizeof(sizefilename));
                    if (nbytes == -1) {
                        perror("Error read");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(buf, "The file %s has size: %llu\n", filename, sizefilename);
                    if (write(STDOUT_FILENO, buf, strlen(buf)) < strlen(buf)) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    free(filename);
                }
                /* It's a directory */
                else if (flag_f == 'd') {
                    continue;
                }
                /* No more files */
                else {
                    if (flag_f == 'e') {
                        break;
                    }
                }
            }
        }
        else {
            if (write(STDOUT_FILENO, "The file is not a regular file/directory!\n", 42) < 42) {
                    perror("Error write");
                    exit(EXIT_SUCCESS);
            }
        }
    }
    else {
        if (write(STDOUT_FILENO, "File not found!\n", 16) < 16) {
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
}

