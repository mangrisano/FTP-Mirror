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
#include <signal.h>

#define BUFSIZE 4096
#define PENDING_QUEUE 5

void signal_handler(int sig);
void get(int fd, char *filedir, char *filename);
void list(int fd, char *dirname, char *filename);
void put(int fd);

int main(int argc, char *argv[]) {
    struct sockaddr_in info_server, info_client;          /* Connections informations */
    int server, port, client;
    pid_t child;
    char command[4], *param;                              /* Command and param received from the client */
    ssize_t nbytes;                                       /* Num of bytes read */
    ssize_t lenparam;
    socklen_t address_client_len = sizeof(info_client);
    if (argc < 3) {
        perror("Error arguments");
        exit(EXIT_FAILURE);
    }
    sscanf(argv[1], "%d", &port);
    /* Set server's params */
    info_server.sin_family = AF_INET;
    info_server.sin_port = htons(port);
    info_server.sin_addr.s_addr = htonl(INADDR_ANY);
    server = socket(PF_INET, SOCK_STREAM, 0);
    signal(SIGINT, signal_handler);
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
        /* signal(SIGINT, signal_handler); */
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
            /* Command received from the client */
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
            command[nbytes] = '\0';
            /* Length of the param received from the client */
            nbytes = read(client, &lenparam, sizeof(lenparam));
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
            param = (char *) malloc(sizeof(char) * lenparam + 1);
            if (param == NULL) {
                perror("Error malloc");
                exit(EXIT_FAILURE);
            }
            /* Param received from the client */
            nbytes = read(client, param, lenparam);
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
            param[nbytes] = '\0';
            if (strcmp(command, "Get") == 0) {
                get(client, argv[2], param);
            }
            else if (strcmp(command, "Put") == 0) {
                put(client);
            }
            else if (strcmp(command, "Lis") == 0) {
                list(client, argv[2], param);
            }
            else {
                if (write(client, "Command not found!\n", 19) < 19) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
            /* Free memory */
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

void get(int fd, char *dirname, char *filename) {
    DIR *dp;                                        /* dp is for directory parent */
    DIR *dc;                                        /* dc is for directoy child */
    struct dirent *de;                              /* de is for directory entry */
    struct stat buf;                                /* buf contains the file's informations */
    int found = 0;                                  /* 1 if filename is found; 0 otherwise */
    int f;                                          /* Filedescriptor for the file that has to be read */
    size_t lenfilename = 0;                         /* Length of the filename */
    ssize_t nbytes;                                 /* Num of bytes read */
    char *f_content;                                /* Content of the file */
    char *fname;                                    /* Name of the file */
    char flag = '-';                                /* Flag to close the transfer */
    char flag_f = 'f';                              /* Flag to say if there are more file regular */
    dp = opendir(dirname);
    if (dp == NULL) {
        perror("Error opendir");
        exit(EXIT_FAILURE);
    }
    /* Read the directory */
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
                lenfilename = strlen(filename);
                if (write(fd, &lenfilename, sizeof(lenfilename)) < sizeof(lenfilename)) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                if (write(fd, filename, lenfilename) < lenfilename) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
                /* Open the file with READONLY permissions */
                f = open(filename, O_RDONLY);
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
                while ((nbytes = read(f, f_content, 100)) != 0) {
                    if (nbytes == -1) {
                        perror("Error read");
                        exit(EXIT_FAILURE);
                    }
                    if (write(fd, &flag, sizeof(flag)) < sizeof(flag)) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    if (write(fd, &nbytes, sizeof(nbytes)) < sizeof(nbytes)) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    /* Send the content to the fd */
                    if (write(fd, f_content, nbytes) < nbytes) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    sleep(1);
                }
                flag = '+';
                if (write(fd, &flag, sizeof(flag)) < sizeof(flag)) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
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
                    if (dc == NULL) {
                        perror("Error opendir");
                        exit(EXIT_FAILURE);
                    }
                    /* Read the directory */
                    while ((de = readdir(dc)) != NULL) {
                        if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
                            continue;
                        }
                        if (stat(de->d_name, &buf) == -1) {
                            perror("Error stat");
                            exit(EXIT_FAILURE);
                        }
                        if (S_ISREG(buf.st_mode)) {
                            /* It's a file */
                            flag_f = 'f';
                            if (write(fd, &flag_f, sizeof(flag_f)) < sizeof(flag_f)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            /* Malloc the space for the filename and check */
                            fname = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                            if (fname == NULL) {
                                perror("Error malloc");
                                exit(EXIT_FAILURE);
                            }                            
                            f_content = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                            if (f_content == NULL) {
                                perror("Error malloc");
                                exit(EXIT_FAILURE);
                            }
                            if (strlen(de->d_name) >= BUFSIZE) {
                                perror("Error: the length of the filename is too big. Memory leak");
                                exit(EXIT_FAILURE);
                            }
                            /* Copy the name of the file in fname */
                            strcpy(fname, de->d_name);
                            lenfilename = strlen(fname);
                            /* Send the length of the filename to the client */
                            if (write(fd, &lenfilename, sizeof(lenfilename)) < sizeof(lenfilename)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            /* Send the filename to the client */
                            if (write(fd, fname, lenfilename) < lenfilename) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }                            
                            /* Open the file */
                            f = open(fname, O_RDONLY);
                            if (f == -1) {
                                perror("Error open file");
                                exit(EXIT_FAILURE);
                            }
                            /* Read the file content, 100 byte at a time */
                            while ((nbytes = read(f, f_content, 100)) != 0) {
                                /* There are some bytes to write */
                                flag = '-';
                                if (write(fd, &flag, sizeof(flag)) < sizeof(flag)) {
                                    perror("Error write");
                                    exit(EXIT_FAILURE);
                                }
                                if (write(fd, &nbytes, sizeof(nbytes)) < sizeof(nbytes)) {
                                    perror("Error write");
                                    exit(EXIT_FAILURE);
                                }
                                if (write(fd, f_content, nbytes) < nbytes) {
                                    perror("Error write");
                                    exit(EXIT_FAILURE);
                                }
                                sleep(1);
                            }
                            /* No more bytes */
                            flag = '+';
                            if (write(fd, &flag, sizeof(flag)) < sizeof(flag)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            /* Close the file */
                            if (close(f) == -1) {
                                perror("Error close file");
                                exit(EXIT_FAILURE);
                            }
                            /* Free memory */
                            free(f_content);
                            free(fname);
                        }
                        else {
                            /* It's a directory */
                            flag_f = 'd';
                            if (write(fd, &flag_f, sizeof(flag_f)) < sizeof(flag_f)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    /* No more files */
                    flag_f = 'e';
                    if (write(fd, &flag_f, sizeof(flag_f)) < sizeof(flag_f)) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
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

void list(int fd, char *dirname, char *filename) {
    DIR *dp;                            /* Parent directory */
    DIR *dc;                            /* Child directory */
    struct dirent *de;                  /* Directory entry */
    struct stat buf;                    /* File's informations */
    int found = 0;                      /* 1 if the file is found; 0 otherwise */
    char *fname;                        /* Name of the file */
    char flag_f = '#';                  /* Flag to say if the file is a regular file or a dir */
    size_t lenfname = 0;                /* Length of the filename */
    /* Open the directory */
    dp = opendir(dirname);
    if (dp == NULL) {
        perror("Error opendir");
        exit(EXIT_FAILURE);
    }
    /* Read inside the directory */
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, filename) == 0) {
            /* The file is found */
            found = 1;
            if (write(fd, &found, sizeof(found)) < sizeof(found)) {
                perror("Error write");
                exit(EXIT_FAILURE);
            }
            if (stat(de->d_name, &buf) == -1) {
                perror("Error stat");
                exit(EXIT_FAILURE);
            }
            /* It's a file */
            if (S_ISREG(buf.st_mode)) {
                if (write(fd, "REG", 3) < 3) {
                    perror("Error write");
                    exit(EXIT_SUCCESS);
                }
                if (write(fd, &buf.st_size, sizeof(off_t)) < sizeof(off_t)) {
                    perror("Error write");
                    exit(EXIT_FAILURE);
                }
            }
            /* It's a directory */
            else {
                if (S_ISDIR(buf.st_mode)) {
                    if (write(fd, "DIR", 3) < 3) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    /* Change directory */
                    chdir(filename);
                    dc = opendir(".");
                    if (dc == NULL) {
                        perror("Error opendir");
                        exit(EXIT_FAILURE);
                    }
                    while ((de = readdir(dc)) != NULL) {
                        if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
                            continue;
                        }
                        if (stat(de->d_name, &buf) == -1) {
                            perror("Error stat");
                            exit(EXIT_FAILURE);
                        }
                        /* It's a file */
                        if (S_ISREG(buf.st_mode)) {
                            flag_f = 'f';
                            if (write(fd, &flag_f, sizeof(flag_f)) < sizeof(flag_f)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            fname = (char *) malloc(sizeof(char) * BUFSIZE + 1);
                            if (fname == NULL) {
                                perror("Error malloc");
                                exit(EXIT_FAILURE);
                            }
                            if (strlen(de->d_name) >= BUFSIZE) {
                                perror("Error: The length of the filename is too big. Memory leak");
                                exit(EXIT_FAILURE);
                            }
                            strcpy(fname, de->d_name);
                            lenfname = strlen(fname);
                            /* Write to the client the length of the filename */
                            if (write(fd, &lenfname, sizeof(lenfname)) < sizeof(lenfname)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            /* Write to the client the filename */
                            if (write(fd, fname, lenfname) < lenfname) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            /* Write to the client the size of the file in byte */
                            if (write(fd, &buf.st_size, sizeof(off_t)) < sizeof(off_t)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                            /* Free memory */
                            free(fname);
                        }
                        else {
                            /* It's a directory */
                            flag_f = 'd';
                            if (write(fd, &flag_f, sizeof(flag_f)) < sizeof(flag_f)) {
                                perror("Error write");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    /* No more files */
                    flag_f = 'e';
                    if (write(fd, &flag_f, sizeof(flag_f)) < sizeof(flag_f)) {
                        perror("Error write");
                        exit(EXIT_FAILURE);
                    }
                    /* Close the child directory */
                    if (closedir(dc) == -1) {
                        perror("Error closedir");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    /* Close the parent directory */
    if (closedir(dp) == -1) {
        perror("Error closedir");
        exit(EXIT_FAILURE);
    }
}

void put(int fd) {    
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
            if (write(STDOUT_FILENO, "The file is not a regular file/directory!\n", 45) < 45) {
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

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("Segnale ricevuto: %d\n", sig);
    }
}
