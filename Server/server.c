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
    struct sockaddr_in info_server, info_client;        /* Connections informations */
    int server, port, client;
    pid_t child;
    char *command, *param;                              /* Command and param received from the client */
    ssize_t nbytes;                                     /* Num of bytes read */
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
    /* Bind the address to the service */
    if (bind(server, (struct sockaddr *) &info_server, sizeof(struct sockaddr_in)) == -1) {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }
    /* Keep the socket in listening */
    if (listen(server, PENDING_QUEUE) == -1) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    for (;;) {
        /* Accept for incoming connections */
        client = accept(server, (struct sockaddr *) &info_client, &address_client_len);
        if (client == -1) {
            perror("Error accept");
            continue;
        }            
        if ((child = fork()) == -1) {
            perror("Error fork");
            exit(EXIT_FAILURE);
        }
        /* Child process */
        if (child == 0) { 
            if (close(server) == -1) { /* Closing unused file descriptor */ 
                perror("Error close");
                exit(EXIT_FAILURE);
            }
            command = (char *) malloc(sizeof(char) * 3);
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
            if (nbytes == -1) {
                perror("Error read");
                exit(EXIT_FAILURE);
            }
            if (nbytes == 0) {
                if (write(STDERR_FILENO, "No bytes read!\n", 15) < 15) {
                    perror("Error write");
                }
                exit(EXIT_FAILURE);
            }
            if (strcmp(command, "Get") == 0) {
                get(client, argv[2], param);
            }
            else if (strcmp(command, "Put") == 0) {
                if (write(STDOUT_FILENO, "Put received...\n", 16) < 16) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                /* Do something */
            }
            else if (strcmp(command, "Lis") == 0) {
                if (write(STDOUT_FILENO, "List received...\n", 17) < 17) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                /* Do something */
            }
            free(command);
            free(param);
            _exit(EXIT_SUCCESS);
        } 
        /* Parent process */
        if (close(client) == -1) {
            perror("Error close");
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}

void get(int fd, char *filedir, char *filename) {
    DIR *dp;                                        /* dp is for directory parent */
    DIR *dc;                                        /* dc is for directoy child */
    struct dirent *de;                              /* de is for directory entry */
    struct stat buf;
    int found = 0;                                  /* True if filename is found; 1 otherwise */
    size_t lenfname = 0;                            /* Length of the filename */
    int f;
    ssize_t nbytes;                                 /* Num of bytes read */
    char *f_content;                                /* Content of the file */
    char *fname;                                    /* Name of the file */
    dp = opendir(filedir);
    if (dp == NULL) {
        perror("Error opendir");
        exit(EXIT_FAILURE);
    }
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, filename) == 0) {
            found = 1;
            if (write(fd, &found, sizeof(found)) < sizeof(found)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            if (stat(de->d_name, &buf) == -1) {
                perror("Error stat");
                exit(EXIT_FAILURE);
            }
            /* If the param of the client is a regular file */
            if (S_ISREG(buf.st_mode)) {
                if (write(fd, "REG", 3) < 3) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                /* Open the file with READONLY permissions */
                f = open(de->d_name, O_RDONLY);
                if (f == -1) {
                    perror("Error open file");
                    exit(EXIT_FAILURE);
                }
                /* Malloc the space for the content of the file */
                f_content = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                if (f_content == NULL) {
                    perror("Error malloc");
                    exit(EXIT_FAILURE);
                }
                /* Read the content of the file */
                nbytes = read(f, f_content, BUFSIZE);
                if (nbytes == -1) {
                    perror("Error read");
                    exit(EXIT_FAILURE);
                }
                if (nbytes == 0) {
                    perror("No more bytes\n");
                    exit(EXIT_SUCCESS);
                }
                /* Send the content to the fd */
                write(fd, f_content, nbytes);
                if (close(f) == -1) {
                    perror("Error closing file");
                    exit(EXIT_FAILURE);
                }
                /* Free the space */
                free(f_content);
            }
            /* If the param of the client is a directory */
            else {
                if (S_ISDIR(buf.st_mode)) {
                    if (write(fd, "DIR", 3) < 3) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    /* Change the directory */
                    chdir(filename);
                    dc = opendir(".");
                    /* Read the directory */
                    while ((de = readdir(dc)) != NULL) {
                        if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
                            continue;
                        }
                        /* Malloc the space for the filename and check */
                        fname = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                        if (fname == NULL) {
                            perror("Error malloc");
                            exit(EXIT_FAILURE);
                        }
                        /* Copy the name of the file in fname */
                        strcpy(fname, de->d_name);
                        lenfname = strlen(fname);
                        /* Send to the client the length of the filename */
                        write(fd, &lenfname, sizeof(lenfname));
                        /* Send to the client the filename */
                        if (write(fd, fname, lenfname) < lenfname) {
                            perror("Error write");
                            exit(EXIT_FAILURE);
                        }
                        /* Free the space */
                        free(fname);
                    }
                    /* Close the directory child */
                    if (closedir(dc) == -1) {
                        perror("Error closedir");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    /* Close the directory parent */
    if (closedir(dp) == -1) {
        perror("Error close");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
