#define SHM_KEY 1234
//===============================================================================
// Assignment-5 submission 
// Name: Darapu Adhithya Shiva Kumar Reddy
// Roll No: 22CS30019
//===============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to compute the arithmetic result
int compute_result(char *task) {
    int num1, num2, result;
    char op;
    if (sscanf(task, "%d %c %d", &num1, &op, &num2) == 3) {
        switch (op) {
            case '+': result = num1 + num2; break;
            case '-': result = num1 - num2; break;
            case '*': result = num1 * num2; break;
            case '/': result = (num2 != 0) ? num1 / num2 : 0; break;
            default: return -1; // Invalid operator
        }
        return result;
    }
    return -1; // Parsing error
}

int main(int argc ,char* argv[]) {
    int no_of_req;
    if(argc==1)
    {
        no_of_req=100;
    }else{
        no_of_req=atoi(argv[1]);
    }
    printf(" requi->%d\n",no_of_req);
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // fcntl(sock,F_SETFL,O_NONBLOCK);
    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    while(no_of_req)
    {
        memset(buffer,0,BUFFER_SIZE);
        strcpy(buffer,"GET_TASK");
        printf("++client : sending buffer :%s\n",buffer);
        // Send request for a task
        write(sock, buffer, BUFFER_SIZE);
        sleep(1);
        memset(buffer,0,BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);
        // printf("++client:reponse before :%s\n",response);
        read(sock, response, BUFFER_SIZE);
        // printf("++client:reponse from Server  :%s\n",response);
        
        printf("++Server:%s\n", response);
        if(strcmp(response,"NO TASK AVAILABLE")==0)
        {
            memset(buffer,0,BUFFER_SIZE);
            strcpy(buffer,"exit");
            write(sock,buffer,BUFFER_SIZE);
            printf("++client:No Task Leaving");
            break;
        }
        // Check if response starts with "TASK :"
        if (strncmp(response, "TASK :", 6) == 0) {
            char task[BUFFER_SIZE];
            strcpy(task, response + 7);  // Extract the arithmetic expression

            int result = compute_result(task);
            
            if (result != -1) {
                memset(buffer,0,BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "RESULT : %d", result);
                printf("++client:Sending result to server: %s\n", buffer);
                sleep(1);
                write(sock, buffer, BUFFER_SIZE); // Send result back
            } else {
                printf("Error: Invalid task format\n");
            }
        }
        memset(buffer,0,BUFFER_SIZE);
        no_of_req--;
    }
    if(no_of_req==0)
    {
        memset(buffer,0,BUFFER_SIZE);
        strcpy(buffer,"exit");
        printf("++client:Task Completed Leaving\n");
        write(sock,buffer,BUFFER_SIZE);
    }
    // Close socket
    close(sock);
    return 0;
}
