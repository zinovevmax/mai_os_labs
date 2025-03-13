#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

#define MAX_MSG_SIZE 128
#define HEARTBEAT_INTERVAL 2000

typedef struct {
    long mtype;
    char mtext[MAX_MSG_SIZE];
} message_t;

typedef struct {
    int id;
    pid_t pid;
    int parent_id;
    int is_alive;
    time_t last_heartbeat;
} node_t;

node_t nodes[100];
int node_count = 0;

int msgqid;

void send_message(int id, const char* text) {
    message_t msg;
    msg.mtype = id;
    strncpy(msg.mtext, text, MAX_MSG_SIZE);
    msgsnd(msgqid, &msg, sizeof(msg.mtext), 0);
}

void create_node(int id, int parent_id) {
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].id == id) {
            printf("Error: Already exists\n");
            return;
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        while (1) {
            message_t msg;
            msgrcv(msgqid, &msg, sizeof(msg.mtext), id, 0);
            printf("Node %d received: %s\n", id, msg.mtext);
        }
    } else {
        // Parent process
        nodes[node_count].id = id;
        nodes[node_count].pid = pid;
        nodes[node_count].parent_id = parent_id;
        nodes[node_count].is_alive = 1;
        nodes[node_count].last_heartbeat = time(NULL);
        node_count++;
        printf("Ok: %d\n", pid);
    }
}

void exec_command(int id, const char* name, int value) {
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].id == id) {
            if (nodes[i].is_alive) {
                char cmd[MAX_MSG_SIZE];
                snprintf(cmd, MAX_MSG_SIZE, "exec %d %s %d", id, name, value);
                send_message(id, cmd);
                printf("Ok:%d\n", id);
            } else {
                printf("Error:%d: Node is unavailable\n", id);
            }
            return;
        }
    }
    printf("Error:%d: Not found\n", id);
}

void heartbit(int interval) {
    while (1) {
        sleep(interval / 1000);  // Ждем interval миллисекунд
        for (int i = 0; i < node_count; i++) {
            if (nodes[i].is_alive && time(NULL) - nodes[i].last_heartbeat > 4 * interval / 1000) {
                printf("Heartbit: node %d is unavailable now\n", nodes[i].id);
                nodes[i].is_alive = 0;
            }
        }
    }
}

int main() {
    msgqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    char command[128];
    while (1) {
        printf("> ");
        fgets(command, 128, stdin);
        command[strcspn(command, "\n")] = 0;

        if (strncmp(command, "create", 6) == 0) {
            int id, parent_id;
            sscanf(command, "create %d %d", &id, &parent_id);
            create_node(id, parent_id);
        } else if (strncmp(command, "exec", 4) == 0) {
            int id, value;
            char name[128];
            sscanf(command, "exec %d %s %d", &id, name, &value);
            exec_command(id, name, value);
        } else if (strncmp(command, "heartbit", 8) == 0) {
            int interval;
            sscanf(command, "heartbit %d", &interval);
            heartbit(interval);
        } else {
            printf("Unknown command\n");
        }
    }

    msgctl(msgqid, IPC_RMID, NULL);
    return 0;
}