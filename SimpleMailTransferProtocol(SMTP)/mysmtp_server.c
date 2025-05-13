//===============================================================================
// Assignment-6 submission 
// Name: Darapu Adhithya Shiva Kumar Reddy
// Roll No: 22CS30019
//===============================================================================
// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include<sys/errno.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include<time.h>
#define PORT 2525
#define BUFFER_SIZE 1024
#define MAILBOX_DIR "mailbox/"  // Directory for storing emails
#define CODE200 "200 OK"
#define CODE400 "400 ERR"
#define CODE401 "401 NOT FOUND"
#define CODE403 "403 FORBIDDEN"
#define CODE500 "500 SERVER ERROR"


int server_sock;


void handle_sigchld(int sig) {
    // Wait for all dead processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

void handle_signal(int sig)
{
    printf("\nShutting down server...\n");
    close(server_sock);
    exit(0);
}

void get_current_date(char *date_str, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    strftime(date_str, size, "%d-%m-%Y", tm_info);  // Format: DD-MM-YYYY
}

int is_valid_email(const char *email, const char *key_domain) {
    char *at_ptr = strchr(email, '@');  // Find '@' in email

    if (at_ptr) {
        at_ptr++;  // Move past '@' to check domain
        return strcmp(at_ptr, key_domain) == 0;  // Compare domain
    }
    
    return 0;  // Invalid if '@' is not found
}

void store_email(const char *recipient, const char *message) {
    char filename[BUFFER_SIZE];
    snprintf(filename, BUFFER_SIZE, "%s%s.txt", MAILBOX_DIR, recipient);
    printf("++server(%d):filename :%s\n",getpid(),filename);
    FILE *file = fopen(filename, "a");
    if (file) {
        fprintf(file, "%s\n", message);
        fclose(file);
    }
}

void list_emails(const char *recipient, int client_sock) {
    char filename[BUFFER_SIZE], line[BUFFER_SIZE], sender[BUFFER_SIZE], date[BUFFER_SIZE];
    int email_count = 0;  // Counter for numbering emails

    snprintf(filename, BUFFER_SIZE, "%s%s.txt", MAILBOX_DIR, recipient);
    printf("++server(%d): filename: %s\n", getpid(), filename);

    FILE *file = fopen(filename, "r");
    if (!file) {
        write(client_sock, "401 NOT FOUND\n", 14);
        return;
    }

    write(client_sock, "200 OK\n", 7);

    while (fgets(line, BUFFER_SIZE, file)) {
        if (strncmp(line, "--Beginning-Of-Message--", 24) == 0) {
            memset(sender, 0, BUFFER_SIZE);
            memset(date, 0, BUFFER_SIZE);

            while (fgets(line, BUFFER_SIZE, file)) {
                if (strncmp(line, "Date:", 5) == 0) {
                    sscanf(line, "Date: %999s", date);  // Limit input to avoid overflow
                } else if (strncmp(line, "Sender-Email:", 13) == 0) {
                    sscanf(line, "Sender-Email: %999s", sender);  // Limit input size
                } else if (strncmp(line, "--End-Of-Message--", 18) == 0) {
                    email_count++;
                    char formatted_msg[BUFFER_SIZE];
                    sprintf(formatted_msg,"%d",email_count);
                    strcat(formatted_msg,": Email from ");
                    strcat(formatted_msg,sender);
                    strcat(formatted_msg,"(");
                    strcat(formatted_msg,date);
                    strcat(formatted_msg,")\n");
                    printf("++server(%d):formattted message:%s\n",getpid(),formatted_msg);
                    write(client_sock, formatted_msg, strlen(formatted_msg));
                    break;
                }
            }
        }
    }
    printf("++server(%d):completed making list \n",getpid());
    sleep(1);
    write(client_sock, "EOL",strlen("EOL"));  // End of List Message
    printf("++server(%d):sent EOL \n",getpid());
    fclose(file);
}


void get_email(const char* recipient, int messageid, int client_sock) {
    char filename[BUFFER_SIZE], line[BUFFER_SIZE], sender[BUFFER_SIZE], date[BUFFER_SIZE];
    int email_count = 0;  // Counter for tracking emails

    snprintf(filename, BUFFER_SIZE, "%s%s.txt", MAILBOX_DIR, recipient);
    printf("++server(%d): filename: %s\n", getpid(), filename);

    FILE *file = fopen(filename, "r");
    if (!file) {
        write(client_sock, "401 NOT FOUND\n", 14);
        return;
    }

    while (fgets(line, BUFFER_SIZE, file)) {
        if (strncmp(line, "--Beginning-Of-Message--", 24) == 0) {
            memset(sender, 0, BUFFER_SIZE);
            memset(date, 0, BUFFER_SIZE);

            while (fgets(line, BUFFER_SIZE, file)) {
                if (strncmp(line, "Date:", 5) == 0) {
                    sscanf(line, "Date: %s", date);  // Extract date
                } else if (strncmp(line, "Sender-Email:", 13) == 0) {
                    sscanf(line, "Sender-Email: %s", sender);  // Extract sender email
                } else if (strncmp(line, "Message:", 8) == 0) {
                    email_count++;

                    if (email_count == messageid) {
                        // Send email header to client
                        printf("++server(%d):Date:%s,sendermail:%s\n",getpid(),date,sender);
                        char response[BUFFER_SIZE];
                        strcpy(response,CODE200);
                        write(client_sock,response,BUFFER_SIZE);
                        memset(response,0,BUFFER_SIZE);
                        strcpy(response,"From: ");
                        strcat(response,sender);
                        write(client_sock,response,BUFFER_SIZE);
                        memset(response,0,BUFFER_SIZE);
                        strcpy(response,"Date: ");
                        strcat(response,date);
                        write(client_sock,response,BUFFER_SIZE);  
                        // snprintf(response, BUFFER_SIZE, "200 OK\nFrom: %s\nDate: %s\n", sender, date);
                        // write(client_sock, response, strlen(response));

                        // Send message body to client
                        while (fgets(line, BUFFER_SIZE, file)) {
                            if (strncmp(line, ".", 1) == 0) break;  // End of message
                            write(client_sock, line, strlen(line));
                            printf("++server(%d):message sent\n",getpid());
                        }
                        sleep(1);
                        write(client_sock, "EOM", strlen("EOM"));  // End of Message signal
                        fclose(file);
                        return;
                    }
                }
            }
        }
    }
    // If message ID was not found

    write(client_sock, "401 NOT FOUND\n", 14);
    fclose(file);
}


void handle_client(int client_sock) {
    char sendbuffer[BUFFER_SIZE];
    char recvbuffer[BUFFER_SIZE];
    char recipent_mail[BUFFER_SIZE];
    char sender_mail[BUFFER_SIZE];
    char  domainname[BUFFER_SIZE];
    memset(sendbuffer, 0, BUFFER_SIZE);
    memset(recvbuffer, 0, BUFFER_SIZE);
    memset(domainname, 0, BUFFER_SIZE);
    memset(recipent_mail, 0, BUFFER_SIZE);
    memset(sender_mail, 0, BUFFER_SIZE);
    printf("++server(%d): Waiting for client message...\n", getpid());
    
    while (1) {
        ssize_t bytes_read = read(client_sock, recvbuffer, BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            printf("++server(%d): Client disconnected\n", getpid());
            break;
        }

        recvbuffer[bytes_read] = '\0';
        recvbuffer[strcspn(recvbuffer, "\r\n")] = '\0';  // Remove trailing newline

        printf("++server(%d): Received: %s\n", getpid(), recvbuffer);

        if (strncmp(recvbuffer, "HELO ", 5) == 0) {
            strcpy(domainname,recvbuffer+5);
            if(domainname[0]!='\0')
            {
                printf("++server(%d):client id :%s\n",getpid(),domainname);
                strcpy(sendbuffer, CODE200);

            }else
            {
                strcpy(sendbuffer,CODE400);
            }
        } 
        else if (strncmp(recvbuffer, "MAIL FROM: ", 11) == 0) {
            strcpy(sender_mail,recvbuffer+11);
            if(domainname[0]!='\0')
            {
                if(is_valid_email(sender_mail,domainname))
                {
                    printf("++server(%d):sender mail :%s\n",getpid(),sender_mail);
                    strcpy(sendbuffer, CODE200);
                }else
                {
                    strcpy(sendbuffer,CODE400);
                }
            }else
            {
                memset(sender_mail,0,BUFFER_SIZE);
                strcpy(sendbuffer,CODE400);
            }
            
        } 
        else if (strncmp(recvbuffer, "RCPT TO: ", 9) == 0) {
            strcpy(recipent_mail,recvbuffer+9);
            printf("++server(%d):reciepent mail :%s\n",getpid(),recipent_mail);
            if(domainname[0]!='\0'&&sender_mail[0]!='\0')
            {

                if(is_valid_email(recipent_mail,domainname))
                {
                    char date[20];
                    get_current_date(date,sizeof(date));
                    char datemessage[BUFFER_SIZE];
                    memset(datemessage,0,BUFFER_SIZE);
                    strcpy(datemessage,"--Beginning-Of-Message--");
                    store_email(recipent_mail,datemessage);
                    memset(datemessage,0,BUFFER_SIZE);
                    strcpy(datemessage,"Date: ");
                    strcat(datemessage,date);
                    // strcat(datemessage,"\n");
                    store_email(recipent_mail,datemessage);
                    memset(datemessage,0,BUFFER_SIZE);
                    // memset(datemessage,0,BUFFER_SIZE);
                    strcpy(datemessage,"Sender-Email: ");
                    strcat(datemessage,sender_mail);
                    // strcat(datemessage,"\n");
                    store_email(recipent_mail,datemessage);
                    strcpy(sendbuffer, "200 OK");
                }else
                {
                    memset(recipent_mail,0,BUFFER_SIZE);
                    strcpy(sendbuffer,CODE400);
                }
            }else
            {
                strcpy(sendbuffer,CODE400);
            }
        } 
        else if (strcmp(recvbuffer, "DATA") == 0) {
            if(recipent_mail[0]!='\0')
            {

                write(client_sock, "Enter your message (end with a single dot '.'):", 47);
                
                char message[BUFFER_SIZE];
                memset(message, 0, BUFFER_SIZE);
                strcpy(message,"Message:");
                store_email(recipent_mail,message);
                memset(message,0,BUFFER_SIZE);
                while (1) {
                    read(client_sock, message, BUFFER_SIZE);
                    printf("++server(%d):client message:%s\n",getpid(),message);
                    if (strncmp(message, ".",1) == 0) 
                    {
                        store_email(recipent_mail,message);
                        memset(message,0,BUFFER_SIZE);
                        strcpy(message,"--End-Of-Message--");
                        store_email(recipent_mail,message);
                        break;
                    }
                    store_email(recipent_mail, message);
                    printf("++server(%d):writing completed \n",getpid());
                    memset(message,0,BUFFER_SIZE);
                }
                strcpy(sendbuffer, "200 Message stored successfully");
            }else
            {
                strcpy(sendbuffer,CODE400);
            }
        } 
        else if (strncmp(recvbuffer, "LIST ", 5) == 0) {
            char recipient[BUFFER_SIZE];
            if(domainname[0]!='\0')
            {
                sscanf(recvbuffer + 5, "%s", recipient);
                if(is_valid_email(recipient,domainname))
                {
                    printf("++server(%d):recipent mail:%s\n",getpid(),recipient);
                    list_emails(recipient, client_sock);
                    continue;

                }else
                {
                    strcpy(sendbuffer,CODE400);
                    strcat(sendbuffer,"\n");
                }
            }else
            {
                strcpy(sendbuffer,CODE400);
                strcat(sendbuffer,"\n");
            }
        }
        else if(strncmp(recvbuffer,"GET_MAIL ",9)==0)
        {
            if(domainname[0]!='\0')
            {
                char recipient[BUFFER_SIZE];
                int id;
                sscanf(recvbuffer,"GET_MAIL %s %d",recipient,&id);
                if(is_valid_email(recipient,domainname))
                {
                    printf("++server(%d):recipent email:%s\tmessage-id:%d\n",getpid(),recipient,id);
                    get_email(recipient,id,client_sock);
                }else
                {
                    memset(recipient,0,BUFFER_SIZE);
                    strcpy(sendbuffer,CODE400);
                }

            }else
            {
                strcpy(sendbuffer,CODE400);
            }
        }
        else if (strcmp(recvbuffer, "QUIT") == 0) {
            printf("++server(%d): Client requested QUIT\n", getpid());
            memset(sendbuffer,0,BUFFER_SIZE);
            strcpy(sendbuffer, "200 GoodBye");
            write(client_sock, sendbuffer, strlen(sendbuffer));
            break;
        } 
        else {
            strcpy(sendbuffer, "403 FORBIDDEN");
        }

        write(client_sock, sendbuffer, strlen(sendbuffer));
    }

    close(client_sock);
}

void print_functionality() {
    printf("========================================\n");
    printf("Functionality of server.c\n");
    printf("========================================\n");
    
    printf("1. Shared Memory and Semaphore Setup:\n");
    printf("   - Uses `ftok` to generate a unique key for shared memory and semaphore.\n");
    printf("   - Allocates shared memory to store tasks and their statuses.\n");
    printf("   - Initializes a semaphore for synchronizing access to shared memory.\n\n");
    
    printf("2. Signal Handling:\n");
    printf("   - Sets up signal handlers for SIGCHLD to handle zombie processes.\n");
    printf("   - Sets up signal handlers for SIGTERM and SIGINT to handle graceful shutdown.\n\n");
    
    printf("3. Task Loading:\n");
    printf("   - Loads tasks from a configuration file (`task.txt`) into shared memory.\n");
    printf("   - Initializes task statuses (assigned, completed, result).\n\n");
    
    printf("4. Server Setup:\n");
    printf("   - Creates a socket and binds it to a specified port (8080).\n");
    printf("   - Listens for incoming client connections.\n");
    printf("   - Accepts client connections and spawns child processes to handle them.\n\n");
    
    printf("5. Client Handling:\n");
    printf("   - Child process handles client requests in a loop.\n");
    printf("   - Assigns tasks to clients upon receiving a 'GET_TASK' request.\n");
    printf("   - Receives task results from clients and updates shared memory.\n");
    printf("   - Handles client disconnections and timeouts.\n\n");
    
    printf("6. Task Assignment and Result Processing:\n");
    printf("   - Assigns the next available unassigned task to a client upon request.\n");
    printf("   - Updates the task status to 'completed' and stores the result when a client submits a result.\n\n");
    
    printf("7. Graceful Shutdown:\n");
    printf("   - Closes the server socket and exits the process upon receiving termination signals.\n\n");
    
    printf("========================================\n");
}

int main(int argc,char* argv[]) {
    int port;
    if(argc>1)
    {
        port=atoi(argv[1]);
    }else
    {
        port=PORT;
    }
    signal(SIGTERM,handle_signal);
    signal(SIGINT,handle_signal);
    signal(SIGCHLD, handle_sigchld); // Handle SIGCHLD to prevent zombie processes
    int  client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    struct stat st = {0};
    
    if (stat("mailbox", &st) == -1) 
    {
        if (mkdir("mailbox", 0700) == -1) 
        {
            perror("Failed to create mailbox directory");
            exit(EXIT_FAILURE);
        }
    }

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // print_functionality();

    printf("Server listening on port %d\n", port);
    
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) {
            // fcntl(client_sock,F_SETFL,O_NONBLOCK);
            printf("\n++server Child (%d) created\n\n",getpid());
            handle_client(client_sock);
            printf("++server(%d):client handled succesfully \n",getpid());
            close(server_sock);
            printf("++server(%d):closed success\n\n",getpid());
            exit(0);
        }else{
            
        }
        close(client_sock);
    }
    close(server_sock);
    return 0;
}