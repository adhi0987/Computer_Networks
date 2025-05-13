#define SHM_KEY 1234
//===============================================================================
// Assignment-5 submission 
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
#define PORT 8080
#define MAX_TASKS 100
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
// Semaphore operations
void sem_lock(int semid) {
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

void sem_unlock(int semid) {
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

// Task queue structursleep(1);e
typedef struct {
    char task[BUFFER_SIZE];
    int assigned;
    int completed;
    int result;
} Task;

typedef struct {
    Task task_queue[MAX_TASKS];
    int task_count;
} SharedMemory;

int semid,server_sock;

// Function to handle zombie processes
void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void handle_signal(int sig)
{
    printf("\nShutting down server...\n");
    close(server_sock);
    exit(0);
}

// Function to load tasks from config file
void load_tasks(const char *filename, SharedMemory* shared) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open task file");
        exit(EXIT_FAILURE);
    }
    printf("++load_tsk:file open success \n");

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        // printf("++load_task:%s",line);
        line[strcspn(line, "\n")] = 0;  // Remove newline
        // printf("++load_task:%s",line);
        if (strlen(line) == 0) continue; // Skip empty lines

        strncpy(shared->task_queue[shared->task_count].task, line, BUFFER_SIZE);
        // printf("Loaded Task %d: %s\n", shared->task_count, shared->task_queue[shared->task_count].task);

        shared->task_queue[shared->task_count].assigned = 0;
        shared->task_queue[shared->task_count].completed = 0;
        shared->task_count++;
    }
    printf("+--------------------------------------------------------------+\n");
    printf("| %-3s | %-15s | %-10s | %-10s | %-10s|\n", "S.no", "Task", "Assignment", "Completed", "Result");
    printf("+--------------------------------------------------------------+\n");
    for(int i = 0; i < shared->task_count; i++)
    {
        printf("| %-3d | %-15s | %-10d | %-10d | %-10d |\n", 
               i, 
               shared->task_queue[i].task, 
               shared->task_queue[i].assigned, 
               shared->task_queue[i].completed, 
               shared->task_queue[i].result);
    }
    printf("+--------------------------------------------------------------+\n");
    fclose(file);
}

// Function to assign a task to a client
// char* assign_task(SharedMemory* shared) {
//     for (int i = 0; i < shared->task_count; i++) {
//         printf("Checking Task %d: %s | Assigned: %d | Completed: %d | Result: %d \n", i, shared->task_queue[i].task, shared->task_queue[i].assigned, shared->task_queue[i].completed,shared->task_queue[i].result);
        
//         if (!shared->task_queue[i].assigned && !shared->task_queue[i].completed) {
//             shared->task_queue[i].assigned = 1;
//             printf("Assigning Task %d: %s\n", i, shared->task_queue[i].task);
//             shared->recent_task=i;
//             shared->task_queue[i].index=i;
//             return shared->task_queue[i].task;
//         }
//     }
//     return NULL;
// }




// Function to remove a completed task
void remove_task(const char *task,SharedMemory* shared) {
    for (int i = 0; i < shared->task_count; i++) {
        if (strcmp(shared->task_queue[i].task, task) == 0) {
            shared->task_queue[i].completed = 1;
            shared->task_queue[i].assigned=1;
            break;
        }
    }
}

int extract_result_number(const char *str) {
    const char *prefix = "RESULT : ";
    int prefix_len = strlen(prefix);

    // Check if the input string starts with "RESULT : "
    if (strncmp(str, prefix, prefix_len) == 0) {
        // Convert the number part of the string to an integer
        return atoi(str + prefix_len);
    }
    
    return -1; // Return -1 if the format doesn't match
}

void handle_client(int client_sock,SharedMemory* shared) {
    char sendbuffer[BUFFER_SIZE];
    char recvbuffer[BUFFER_SIZE];
    memset(sendbuffer, 0, BUFFER_SIZE);
    memset(recvbuffer, 0, BUFFER_SIZE);
    int sleep_count=0;
    int is_alloted_task=0;
    int result_obtained=0;
    int task_id=-1;
    // read(client_sock, buffer, BUFFER_SIZE);
    printf("++server(%d):initial recv-buffer:%s\n",getpid(),recvbuffer);
    while(1)
    {
        // sem_lock(semid);
        // printf("+---------------------------------------------------------------------------------------+\n");
        // printf("|\tTask\t\t|\tAssignment\t|\tCompleted\t|\tResult\t|\n");
        // printf("+---------------------------------------------------------------------------------------+\n");
        // sem_lock(semid);
        // for(int i=0;i<shared->task_count;i++)
        // {
        //     printf("|\t%s\t\t|\t%3d\t\t|\t%3d\t\t|\t%3d\t|\n",shared->task_queue[i].task,shared->task_queue[i].assigned,shared->task_queue[i].completed,shared->task_queue[i].completed);
        // }
        // printf("+---------------------------------------------------------------------------------------+\n");
        sem_unlock(semid);
        read(client_sock, recvbuffer, BUFFER_SIZE);
        // printf("++server(%d):buffer---->%s\n",getpid(),recvbuffer);
        if(sleep_count==5)
        {
            printf("++server(%d):Waited Enough Time No response From Client disconnecting\n",getpid());
            // printf("++server(%d):waiting for shm \n",getpid());
            sem_lock(semid);
            if(task_id!=-1)
            {
                // printf("server locked mem  recent_task\n");
                if(!result_obtained)
                {
                    printf("++server(%d):Task %d :%s - Not completed\n",getpid(),task_id,shared->task_queue[task_id].task);
                    shared->task_queue[task_id].assigned=0;
                }
            }
            sem_unlock(semid);
            result_obtained=0;
            break;
        }
       
        if (strcmp(recvbuffer, "GET_TASK") == 0) {
            if(!is_alloted_task)
            {
                sem_lock(semid);
                // printf("server locked mem \n");
                memset(sendbuffer,0,BUFFER_SIZE);
                // char* task=assign_task(shared);
                char *task=NULL;
                for (int i = 0; i < shared->task_count; i++) {
                    // printf("Checking Task %d: %s | Assigned: %d | Completed: %d | Result: %d \n", i, shared->task_queue[i].task, shared->task_queue[i].assigned, shared->task_queue[i].completed,shared->task_queue[i].result);
                    if (!shared->task_queue[i].assigned && !shared->task_queue[i].completed) {
                        shared->task_queue[i].assigned = 1;
                        printf("++server(%d):Assigning...\t\t Task %d: %s\n",getpid(), i, shared->task_queue[i].task);
                        // shared->recent_task=i;
                        task_id=i;
                        task =shared->task_queue[i].task;
                        printf("++Server(%d):Sleep Count-Timer for Task %d Started\t\t%3d\n",getpid(),task_id,sleep_count);
                        break;
                    }
                }
               
                if(task)
                {
                    snprintf(sendbuffer, BUFFER_SIZE, "TASK : %s", task);
                    sendbuffer[strcspn(sendbuffer, "\n")] = 0;  // Remove newline from task if it exists
                }else
                {
                    strcpy(sendbuffer,"NO TASK AVAILABLE");
                }
                sem_unlock(semid);
                write(client_sock, sendbuffer, BUFFER_SIZE);
                if(strcmp(sendbuffer,"NO TASK AVAILABLE")==0) break;
                memset(sendbuffer,0,BUFFER_SIZE);
                is_alloted_task=1;
                result_obtained=0;
            }else
            {
                printf("++server(%d):Complete Alloted Task %d  Time Remaining = %d\n",getpid(),task_id,5-sleep_count);
                sleep(1);
                sleep_count++;
            }
        }else if(strncmp(recvbuffer,"RESULT : ",9)==0)
        {
            if(!result_obtained)
            {
                int result =extract_result_number(recvbuffer);
                printf("++server(%d):result from the client :%d\n++server(%d):writing result to Task %d\n",getpid(),result,getpid(),task_id);
                sem_lock(semid);
                // printf("server locked mem \n");
                    shared->task_queue[task_id].result=result;
                    shared->task_queue[task_id].completed=1;
                sem_unlock(semid);
                is_alloted_task=0;
                result_obtained=1;
                sleep_count=0;
            }else{
                printf("++Server(%d):Result Already obtained Waiting For New GET_TASK Request From Client Time Remaining To Respond =%d \n",getpid(),5-sleep_count);
                sleep(1);
                sleep_count++;
            }
            
            // memset(buffer,0,BUFFER_SIZE);
            // break;
        }else if(strcmp(recvbuffer,"exit")==0)
        {
            printf("\n++server(%d):client wants to close \n",getpid());
            break;
        }
        else
        {
            printf(
                "++server(%d):No Response From Client \n",getpid()
            );
            sleep(1);
            sleep_count++;
            // memset(sendbuffer,0,BUFFER_SIZE);
            printf("++server(%d):waked up sleep_count = %d\n",getpid(),sleep_count);
        } 
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

int main() {
    key_t shmkey=ftok(".",66);
    if(shmkey==-1)
    {
        perror("shmkey");
        exit(1);
    }
    int shmid = shmget(shmkey, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    
    SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    // Initialize shared memory
    shm->task_count = 0;
    // shm->recent_task=-1;
    for (int i = 0; i < MAX_TASKS; i++) {
        shm->task_queue[i].assigned = 0;
        memset(shm->task_queue[i].task,0,BUFFER_SIZE);
        shm->task_queue[i].completed=0;
        shm->task_queue[i].result=-1;
    }
    // Load tasks from config file
    load_tasks("config.txt",shm);

    // Create semaphore
    key_t key = ftok(".", 1);
    semid = semget(key, 1, 0666 | IPC_CREAT);
    semctl(semid,0,SETVAL,1); //intialize semaphore to 1 
    // sem_unlock(semid); // Initialize semaphore to 1

    // Set up SIGCHLD handler
    signal(SIGCHLD, handle_sigchld);
    signal(SIGTERM,handle_signal);
    signal(SIGINT,handle_signal);
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

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    print_functionality();

    printf("Server listening on port %d\n", PORT);
    
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) {
            fcntl(client_sock,F_SETFL,O_NONBLOCK);
            printf("\n++server Child (%d) created\n\n",getpid());
            handle_client(client_sock,shm);
            printf("++server(%d):client handled succesfully \n",getpid());
            close(server_sock);
            printf("++server(%d):closed success\n\n",getpid());
            printf("+----------------Printing Task Status------------------+\n");
            printf("+--------------------------------------------------------------+\n");
            printf("| %-3s | %-15s | %-10s | %-10s | %-10s|\n", "S.no", "Task", "Assignment", "Completed", "Result");
            printf("+--------------------------------------------------------------+\n");
            sem_lock(semid);
            for(int i = 0; i < shm->task_count; i++)
            {
                printf("| %-3d | %-15s | %-10d | %-10d | %-10d |\n", 
                       i, 
                       shm->task_queue[i].task, 
                       shm->task_queue[i].assigned, 
                       shm->task_queue[i].completed, 
                       shm->task_queue[i].result);
            }
            printf("+--------------------------------------------------------------+\n");
            sem_unlock(semid);
            exit(0);
        }else{
            
        }
        close(client_sock);
    }
    close(server_sock);
    return 0;
}