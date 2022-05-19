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
#include <semaphore.h>

#include <sys/fcntl.h>
#include <pthread.h>

#include <math.h>

#include <regex.h>

#include <netdb.h>

//TUDO RELATIVO AO SERVER
void init(int porto_bolsa, int porto_config, char* config_file);
void handle_client(int fd);
int handle_admin();
void cleanup();
void erro(char *s);

//Admin
int add_user(char* args,struct sockaddr addr);
int delete_user(char* args,struct sockaddr addr);
int list(struct sockaddr addr);
int refresh(char* args,struct sockaddr addr);
void* wait_clients();
void sigint_client();

//User
int client_login(int fd);
void send_client_markets(int client_number, int fd);
int buy(char* args, int client_number, int fd);
int sell(char* args, int client_number, int fd);
void subscribe(char* args, int client_number, int fd);
void wallet(int client_number, int fd);

//Bolsa
void* ManageBolsa();

// 
int check_regex(char *text, char *regex);


#define INITIAL_REFRESH_TIME 2
#define MAX_CLIENTS 10
#define NUMBER_MARKETS 2
#define NUMBER_STOCKS_PER_MARKET 3
#define MAX_SIMULTANEOUS_USERS 5
#define MAX_STRING_SIZES 50

#define MULTICAST_MARKET1 "239.0.0.1"
#define MULTICAST_MARKET2 "239.0.0.2"


int fd_bolsa,fd_config;
int fd_multicast_markets[NUMBER_MARKETS];
struct sockaddr_in addr_bolsa;
struct sockaddr_in addr_config;
struct sockaddr_in addr_multicast_markets[NUMBER_MARKETS];

pid_t childs_pids[10];
pid_t admin_pid;


typedef struct{
    char name[MAX_STRING_SIZES];
    float value;
    int num_stocks;
}stock;

typedef struct{
    char username[MAX_STRING_SIZES];
    char password[MAX_STRING_SIZES];
    float balance;
    stock user_stocks[NUMBER_MARKETS*NUMBER_STOCKS_PER_MARKET];
    short available_markets[NUMBER_MARKETS];
}user;

typedef struct{
    char name[MAX_STRING_SIZES];
    stock stock_list[NUMBER_STOCKS_PER_MARKET];

}market;

typedef struct{

    //TODO ACABAR A SHARED MEMORY
    pthread_mutex_t shm_rdwr;
    pthread_mutexattr_t attr_mutex;

    int refresh_time;
    int number_users;
    user users_list[MAX_CLIENTS];

    //markets
    pthread_mutex_t market_access;
    market market_list[NUMBER_MARKETS];

} shm_vars;


// int refresh_time;
// int number_users;
// user users_list[10];
shm_vars *SMV; //shared memory
user admin;
// market* market_list;
int shmid;