#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void trim_newline(char *str) {
    str[strcspn(str, "\r\n")] = '\0';
}

int main(int argc,char* argv[]) {
    int serverport;
    char serverip[BUFFER_SIZE];
    if(argc>1)
    {
        serverport=atoi(argv[2]);
        strcpy(serverip,argv[1]);
    }
    printf("++client : server port :%d\tserver ip-%s\n",serverport,serverip);
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    memset(response, 0, BUFFER_SIZE);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverport);
    server_addr.sin_addr.s_addr = inet_addr(serverip);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to My_SMTP server.\n");
    fflush(stdout);

    while (1) {
        printf("> ");
        fflush(stdout);
        fgets(buffer, sizeof(buffer), stdin);
        trim_newline(buffer);

        write(sock, buffer, strlen(buffer));
        if(strncmp(buffer,"DATA",4)==0){
            memset(response,0,BUFFER_SIZE);
            read(sock,response,BUFFER_SIZE);
            printf("%s\n",response);
            if(strncmp(response,"400 ERR",7)==0)
            {
                memset(response,0,BUFFER_SIZE);
                continue;
            }
            while (1)
            {
                fgets(buffer, sizeof(buffer), stdin);
                trim_newline(buffer);
                write(sock, buffer, strlen(buffer));
                if(strcmp(buffer,".")==0) break;
            }  
        }else if (strncmp(buffer,"LIST ",5)==0)
        {
            memset(response,0,BUFFER_SIZE);
            read(sock,response,BUFFER_SIZE);
            printf("%s",response);
            fflush(stdout);
            if(strncmp(response,"400 ERR",7)==0)
            {
                memset(response,0,BUFFER_SIZE);
                continue;
            }
            if(strncmp(response,"401 NOT FOUND",13)==0) 
            {
               memset(response,0,BUFFER_SIZE);
               continue;
            }
            while(1)
            {
                memset(response,0,BUFFER_SIZE);
                // printf("++client i am list waiting here \n");
                read(sock,response,BUFFER_SIZE);
                if(strncmp(response,"EOL",3)==0) 
                {
                    // printf("++server Response:%s,%ld\n",response,strlen(response));
                    break;
                }
                printf("%s",response);
                fflush(stdout);
            }
            memset(response,0,BUFFER_SIZE);
            continue;
        }else if(strncmp(buffer,"GET_MAIL ",9)==0)
        {
            memset(response,0,BUFFER_SIZE);
            read(sock,response,BUFFER_SIZE);
            printf("%s\n",response);
            fflush(stdout);
            if(strncmp(response,"400 ERR",7)==0)
            {
                memset(response,0,BUFFER_SIZE);
                continue;
            }
            if(strncmp(response,"401 NOT FOUND",13)==0) 
            {
                memset(response,0,BUFFER_SIZE);
                continue;
            }
            memset(response,0,BUFFER_SIZE);
            read(sock,response,BUFFER_SIZE);
            printf("%s\n",response);
            fflush(stdout);
            memset(response,0,BUFFER_SIZE);
            read(sock,response,BUFFER_SIZE);
            printf("%s\n",response);
            fflush(stdout);
            while (1)
            {
                memset(response,0,BUFFER_SIZE);
                // printf("++client i am list waiting here \n");
                read(sock,response,BUFFER_SIZE);
                if(strncmp(response,"EOM",3)==0)
                {
                    // printf("++server Response:%s,%ld\n",response,strlen(response));
                    break;
                }
                printf("%s",response);
                fflush(stdout);
            }
            continue;
        }
        if (strcmp(buffer, "QUIT") == 0) {
            memset(response,0,BUFFER_SIZE);
            read(sock, response, BUFFER_SIZE);
            printf("%s\n", response);
            fflush(stdout);
            break;
        }

        memset(response, 0, BUFFER_SIZE);
        // printf("++client i am waiting here \n");
        read(sock, response, BUFFER_SIZE);
        printf("%s\n", response);
        fflush(stdout);
    }

    close(sock);
    return 0;
}
