#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/shm.h>
#include <sys/ipc.h>

void init(int porto_bolsa, int porto_config, char* config_file);
void handle_client(int fd);
int handle_admin();

int add_user(char* args,struct sockaddr addr);
int delete_user(char* args,struct sockaddr addr);
int list(struct sockaddr addr);
int refresh(char* args,struct sockaddr addr);
void wait_clients();
void sigint_client();


void erro(char *s) {
	perror(s);
	exit(1);
}

#define INITIAL_REFRESH_TIME 2

int fd_bolsa,fd_config;
struct sockaddr_in addr_bolsa;
struct sockaddr_in addr_config;

pid_t childs_pids[10];
pid_t admin_pid;

typedef struct{
    char username[31];
    char password[31];
    int balance;
}user;


typedef struct{
    char name[31];
    int value;
}stock;

typedef struct{
    char name[31];
    stock stock_list[3];

}market;


int refresh_time;
int number_users;
user users_list[10];
user admin;
market* market_list;
int shmid;