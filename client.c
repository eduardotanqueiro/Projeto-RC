#include "header.h"

void* wait_clients(){
   
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int logged_clients = 0; //TODO TEM DE ESTAR PARTILHADO PARA OS PROCESSOS FILHOS ACEDEREM E MUDAREM QUANTOS TAO LOGADOS

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

                if(logged_clients == MAX_SIMULTANEOUS_USERS){
                    printf("[CLIENT] SERVER: Reached maximum number of clients!\n");
                }
                else if( (childs_pids[logged_clients++] = fork()) == 0){
                    close(fd_bolsa);
                    close(fd_config);
                    handle_client(client_fd);
                    close(client_fd);
                    exit(0);
                }

                close(client_fd); //???
            }

            //printf("[MAIN] Looping\n");
            //CHECK SE ALGUM PROCESSO FECHOU, E DIMINUIR O NUMERO CLIENTES LOGADOS  

        }

    }

}

void sigint_client(){

    //clear client processes
    //TODO fazer outra estrategia, fazer um array com os processos abertos! (GLOBAL)
    for(int i = 0;i<MAX_CLIENTS;i++){
        kill(SIGINT,childs_pids[i]);
    }
    //clear client socket ??
}

int client_login(int fd){

    char* token;
    char buffer[BUFSIZ];
    int number_client = -1;  // check if login succeded

    do{
        char username[31];
        char *password;

        memset(buffer,0,BUFSIZ);
        write(fd,"Introduza as credenciais no formato Username/Password: ",56);
        read(fd,buffer,BUFSIZ);
        buffer[strlen(buffer) - 1] = 0;

        token = strtok(buffer,"/");
        strcpy(username,token);

        password = strtok(NULL,"/\n");

        printf("[SERVER] Attempted user:%s login!\n", username);

        if(!strcmp(username, "QUIT")){
            printf("[USER] A fechar processo do user sem login: %d\n", fd);
            return -1;
        }

        
        pthread_mutex_lock(&SMV->shm_rdwr);
        for(int i = 0; i < MAX_CLIENTS; i++){
            if(strcmp(username, SMV->users_list[i].username) == 0 && strcmp(password, SMV->users_list[i].password) == 0){
                number_client = i;
                break;
            }
        }
        pthread_mutex_unlock(&SMV->shm_rdwr);

        if( number_client != -1 ) //Login Succeded 
            break;

        else
            write(fd,"Username ou password errado(s)!\n",33);

    }while(1);

    printf("[USER] Login Bem Sucedido\n");
    write(fd,"Login Bem Sucedido\n",20);

    return number_client;

}

int buy(char* args, int fd){
    printf("Bought!\n");
    return 0;
}

int sell(char* args, int fd){
    printf("Sold!\n");
    return 0;
}

void handle_client(int fd){

    char buffer[BUFSIZ];
    int client_number = client_login(fd);

    if ( client_number != -1){

        //TODO send client available markets and respective multicast groups ips
        send_client_markets(client_number,fd);

        char comando[50];
        char* token;

        //MENU
        while( strcmp(comando,"SAIR") != 0){

            //receive messagem from user console/ SEND MENU TO CLIENT
            printf("\n[USER] Waiting for user command\n");
            memset(buffer,0,BUFSIZ);

            read(fd,buffer,BUFSIZ);

            //Get Command

            char *resto;
            memset(comando,0,strlen(comando));
            
            token = strtok(buffer, " \n");
            strcpy(comando,token);

            resto = strtok(NULL,"");

            if(!strcmp(comando, "COMPRAR"))
                buy(resto, fd);

            else if(!strcmp(comando, "VENDER"))
                sell(resto, fd);

            else if(!strcmp(comando,"SUBSCREVER"))
                subscribe(resto,fd);

            else if(!strcmp(comando,"CARTEIRA"))
                wallet(fd);
            
            else if( strcmp(comando,"SAIR") != 0)
                write(fd,"COMANDO INVALIDO!",18);

            // TODO resto do menu
        }

        printf("[USER] User left\n");
    }

}


void send_client_markets(int client_number,int fd){





}