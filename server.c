#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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


int main(int argc, char** argv){

    if(argc == 4){
        init( atoi(argv[1]), atoi(argv[2]) ,argv[3] );
    }
    else{
        fprintf(stderr,"Wrong command format\n");
        exit(1);
    }

    pid_t wait_clients_pid; 
    if( (wait_clients_pid = fork()) == 0){
        wait_clients();
        exit(0);
    }

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd_config,&read_set);
    int rt_status = -1;

    //HANDLE ADMIN
    while(1){

        printf("[MAIN] SERVER: Waiting for admin connection\n");

        if( select(fd_config+1,&read_set,NULL,NULL,NULL) > 0){
            printf("[MAIN] SERVER: Admin Connected\n");

            //Conection from the admin console
            if( FD_ISSET(fd_config,&read_set) ){

                printf("[MAIN] SERVER: Admin accepted\n");
                rt_status = handle_admin();

                if(rt_status == 10){
                    kill(wait_clients_pid,SIGINT);
                    break;
                }

                printf("[MAIN] a sair do fd isset config\n");
            }


            printf("[MAIN] Looping\n");

        }


    }

    printf("[MAIN] Closing server...\n");

    //cleanup
    close(fd_bolsa);
    close(fd_config);
    shmdt(market_list);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

void wait_clients(){
   
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int client_number = 0;

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd_bolsa,&read_set);

    signal(SIGINT,sigint_client);

    while(1){

        while(waitpid(-1,NULL,WNOHANG)>0);

        printf("[CLIENT] SERVER: Waiting for clients\n");
        if( select(fd_bolsa+1,&read_set,NULL,NULL,NULL) > 0){
            printf("[CLIENT] SERVER: Client Connected\n");

            //Connection from some user
            if( FD_ISSET(fd_bolsa,&read_set)){
                
                printf("[CLIENT] SERVER: A entrar no client\n");
                client_addr_size = sizeof(client_addr_size);
                
                //Accept the TCP connection
                client_fd = accept(fd_bolsa,(struct sockaddr *)&client_addr,&client_addr_size);
                printf("[CLIENT] SERVER: Client aceite\n");

                if(client_number == 10){
                    printf("[CLIENT] SERVER: Reached maximum number of clients!\n");
                }
                else if( (childs_pids[client_number++] = fork()) == 0){
                    close(fd_bolsa);
                    close(fd_config);
                    handle_client(client_fd);
                    close(client_fd);
                    exit(0);
                }

                close(client_fd); //???
            }

            printf("[MAIN] Looping\n");

        }

    }

}

void sigint_client(){

    //clear client processes
    for(int i = 0;i<10;i++){
        kill(SIGINT,childs_pids[i]);
    }
    //clear client socket ??


}


void init(int porto_bolsa, int porto_config, char* cfg){

    //Read Config File
    FILE* initFile;
    if ( (initFile = fopen(cfg,"r")) == NULL)
    {
        printf("Non-existent config file!!");
        exit(1);
    }

    refresh_time = INITIAL_REFRESH_TIME;

    //Read Admin Auth
    char buf[BUFSIZ],*tok;

    fscanf(initFile,"%[^\n]",&buf[0]);
    
    tok = strtok(buf,"/");
    strcpy(admin.username,tok);
    tok = strtok(NULL, "\n");
    strcpy(admin.password,tok);
    admin.password[ strlen(admin.password) - 1] = 0;

    //Read Number Users
    fscanf(initFile,"%d\n",&number_users);

    if( number_users > 5){
        printf("Maximum number of initial users exceeded!\n");
        exit(1);
    }

    //Read Users Auth
    for(int i = 0;i< number_users;i++){

        fscanf(initFile,"%30[^;];%30[^;];%d\n",&users_list[i].username[0],&users_list[i].password[0],&users_list[i].balance);
    }

    //Clean the empty spots for users in the userlist
    for(int i = number_users;i<10;i++){
        memset(users_list[i].username,0,31);
        memset(users_list[i].password,0,31);
        users_list[i].balance = 0;
    }


    //Read Markets/Stocks
    shmid = shmget(IPC_PRIVATE, sizeof(market_list)*2, IPC_CREAT | IPC_EXCL | 0700);
    if (shmid < 1){
		printf("Error creating shm memory!");
		exit(1);
	}

    market_list = (market*)shmat(shmid,NULL,0);
    if (market_list < (market*) 1){
		printf("Error attaching memory!");
        //cleanup(); TODO
		exit(1);
	}

    for(int i = 0;i<3;i++){
        fscanf(initFile,"%30[^;];%30[^;];%d\n",&market_list[0].name[0],&market_list[0].stock_list[i].name[0],&market_list[0].stock_list[i].value);
    }
    for(int i = 0;i<3;i++){
        fscanf(initFile,"%30[^;];%30[^;];%d\n",&market_list[1].name[0],&market_list[1].stock_list[i].name[0],&market_list[1].stock_list[i].value);
    }

    fclose(initFile);

    //----------------------------

    //Startup server sockets
    bzero((void *) &addr_bolsa, sizeof(addr_bolsa));
    addr_bolsa.sin_family = AF_INET;
    addr_bolsa.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_bolsa.sin_port = htons(porto_bolsa);

    bzero((void *) &addr_config, sizeof(addr_config));
    addr_config.sin_family = AF_INET;
    addr_config.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_config.sin_port = htons(porto_config);


    // char ipStr[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET,&addr_bolsa,&ipStr[0],INET_ADDRSTRLEN);
    // printf("[MAIN] SERVER IP BOLSA %s\n",ipStr);

    // inet_ntop(AF_INET,&addr_bolsa,&ipStr[0],INET_ADDRSTRLEN);
    // printf("[MAIN] SERVER IP CONFIG %s\n", ipStr);

    //Socket Bolsa
    if ( (fd_bolsa = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	  erro("na funcao socket bolsa");
    if ( bind(fd_bolsa,(struct sockaddr*)&addr_bolsa,sizeof(addr_bolsa)) < 0)
	  erro("na funcao bind bolsa");
    if( listen(fd_bolsa, 2) < 0)
	  erro("na funcao listen bolsa");

    //Socket Config
    if ( (fd_config = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	  erro("na funcao socket config");
    if ( bind(fd_config,(struct sockaddr*)&addr_config,sizeof(addr_config)) < 0)
	  erro("na funcao bind config");



}


int handle_admin(){

    char buffer[BUFSIZ];
    int recv_len;
    char comando[50];
    char* token;

    struct sockaddr admin_connect;
    socklen_t slen = sizeof(admin_connect);

    //NETCAT CONNECTING STRINGS (TO IGNORE)
    for(int k = 0;k<5;k++){
        recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &admin_connect, (socklen_t *)&slen);
    }


    //LOGIN (TODO -> FUNCAO PARA ISTO)
    do{
        char username[31];
        char *password;
        
        memset(buffer,0,BUFSIZ);
        sendto(fd_config,"Introduza as credenciais no formato Username/Password: ",56,0,(struct sockaddr*)&admin_connect,(socklen_t )slen);
        recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &admin_connect, (socklen_t *)&slen );
        buffer[strlen(buffer) - 1] = 0;

        token = strtok(buffer,"/");
        strcpy(username,token);

        password = strtok(NULL,"/\n");

        printf("[SERVER] Attempted admin login!\n");


        if( (strcmp(username,admin.username) == 0) && (strcmp(password,admin.password) == 0 ) ){
            sendto(fd_config,"Login bem sucedido!\n",20,0,(struct sockaddr*)&admin_connect,(socklen_t )slen);
            break;
        }
        else if( !strcmp(username,"QUIT") ){
            printf("[ADMIN] A fechar processo do admin\n");
            return 0;
        }
        else
            sendto(fd_config,"Username e/ou password errado(s)!\n",34,0,(struct sockaddr*)&admin_connect,(socklen_t) slen);


    }while(1);

    printf("[ADMIN] Login Bem Sucedido\n");


    while( strcmp(comando,"QUIT") != 0){

        //receive messagem from admin console
        printf("\n[ADMIN] Waiting for admin command\n");
        memset(buffer,0,BUFSIZ);

        if( (recv_len = recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &admin_connect, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom\n");
            return 1;
        }

        //Get Command

        char *resto;
        memset(comando,0,strlen(comando));
        
        token = strtok(buffer, " \n");
        strcpy(comando,token);

        resto = strtok(NULL,"");


        if( !strcmp(comando,"QUIT_SERVER"))
            return 10; //QUIT SERVER CODE
        else if( !strcmp(comando,"ADD_USER") )
            add_user(resto,admin_connect);
        
        else if( !strcmp(comando,"DEL") )
            delete_user(resto,admin_connect);
        
        else if ( !strcmp(comando,"LIST") )
            list(admin_connect);
        
        else if( !strcmp(comando,"REFRESH") )
            refresh(resto,admin_connect);

        else if( strcmp(comando,"QUIT") != 0 )
            sendto(fd_config,"Comando Inválido!\n",20,0,(struct sockaddr*)&admin_connect,(socklen_t) slen);
                
    }

    printf("[ADMIN] A fechar processo do admin\n");
    return 0;
}

int add_user(char* args, struct sockaddr addr){
    printf("[ADMIN] adicionar user %s\n",args);
    return 0;
}

int delete_user(char* args, struct sockaddr addr){

    printf("[ADMIN] deleting user %s\n",args);
    socklen_t slen = sizeof(addr);
    
    for(int i = 0;i<number_users;i++){

        if( (strcmp(users_list[i].username,"\0") != 0) && (!strcmp(users_list[i].username,args)) ){
            
            memset(users_list[i].username,0,31);
            memset(users_list[i].password,0,31);
            users_list[i].balance = 0;

            printf("[ADMIN] User deleted!\n");
            sendto(fd_config,"User deleted!\n",15,0,(struct sockaddr*)&addr,(socklen_t) slen);

            number_users--;
            return 0;
        }

    }

    printf("[ADMIN] Username not found!\n");
    sendto(fd_config,"Error! Username not found!\n",28,0,(struct sockaddr*)&addr,(socklen_t) slen);
    return 1;
}

int list(struct sockaddr addr){
    printf("[ADMIN] Listing users\n");
    char buffer[BUFSIZ];
    socklen_t slen = sizeof(addr);

    for(int i = 0;i<10;i++){

        if( strcmp(users_list[i].username,"\0") != 0){
            snprintf(buffer,BUFSIZ,"Username: %s\n",users_list[i].username);
            sendto(fd_config,buffer,BUFSIZ,0,(struct sockaddr*)&addr,(socklen_t) slen);

        }

    }

    return 0;
}

int refresh(char* args, struct sockaddr addr){
    printf("[ADMIN] refreshing time %s\n",args);
    
    refresh_time = atoi(args);
    
    return 0;
}

void handle_client(int fd){

    char buffer[BUFSIZ];
    int n = -1;

    //  TODO
    //  -LOGIN
    //  -MENU

    do{
        memset(buffer,0,BUFSIZ);
        n = read(fd,buffer,BUFSIZ);
        
        if(n == 0)
            break;

        buffer[strlen(buffer)-1] = 0;
        printf("[CLIENT] USER %d: %s\n",fd,buffer);
    }while( strcmp( buffer, "SAIR") != 0);

    printf("[CLIENT] Closing client\n");
}