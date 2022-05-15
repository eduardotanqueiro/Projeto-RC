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



void handle_client(int fd){

    char buffer[BUFSIZ];
    int client_number = client_login(fd);

    if ( client_number != -1){

        //send client available markets
        send_client_markets(client_number,fd);

        char comando[50];
        char* token;

        //MENU
        while( strcmp(comando,"SAIR") != 0){

            //receive messagem from user console
            printf("\n[SERVER-USER] Waiting for user command\n");
            memset(buffer,0,BUFSIZ);

            read(fd,buffer,BUFSIZ);

            printf("[DEBUG][SERVER-USER] Received command %s\n",buffer);

            //Get Command

            char *resto;
            memset(comando,0,strlen(comando));
            
            token = strtok(buffer, " \n");
            strcpy(comando,token);

            resto = strtok(NULL,"");

            if(!strcmp(comando, "COMPRAR"))
                buy(resto, client_number, fd);

            else if(!strcmp(comando, "VENDER"))
                sell(resto, client_number, fd);

            else if(!strcmp(comando,"SUBSCREVER"))
                subscribe(resto,client_number,fd);

            else if(!strcmp(comando,"CARTEIRA"))
                wallet(client_number,fd);
            
            else if( strcmp(comando,"SAIR") != 0)
                write(fd,"COMANDO INVALIDO!",18);

        }

        printf("[USER] User left\n");
    }

}


void send_client_markets(int client_number,int fd){

    int nr_markets = 0;
    char buffer[BUFSIZ];

    pthread_mutex_lock(&SMV->shm_rdwr);
    for(int i = 0; i<NUMBER_MARKETS;i++)
        if(SMV->users_list[client_number].available_markets[i] == 1)
            nr_markets++;

    snprintf(buffer,BUFSIZ,"%d",nr_markets);
    write(fd,buffer,BUFSIZ);

    pthread_mutex_lock(&SMV->market_access);
    for(int i = 0; i<NUMBER_MARKETS;i++){

        if(SMV->users_list[client_number].available_markets[i] == 1){ //if user has access to this market
            //send market name
            memset(buffer,0,BUFSIZ);
            snprintf(buffer,BUFSIZ,"%s",SMV->market_list[i].name);
            write(fd,buffer,BUFSIZ);

            //send markets' stock info
            for(int j = 0;j<NUMBER_STOCKS_PER_MARKET;j++){
                memset(buffer,0,BUFSIZ);
                snprintf(buffer,BUFSIZ,"-- STOCK: %s | PRICE: %f ---",SMV->market_list[i].stock_list[j].name,SMV->market_list[i].stock_list[j].value);
                write(fd,buffer,BUFSIZ);  
            }

        }
    }
    pthread_mutex_unlock(&SMV->market_access);

    pthread_mutex_unlock(&SMV->shm_rdwr);

}

int buy(char* args,int client_number, int fd){
    printf("Bought!\n");
    return 0;
}

int sell(char* args,int client_number, int fd){
    printf("Sold!\n");
    return 0;
}

void subscribe(char* args,int client_number, int fd){

    char buffer[BUFSIZ];
    int market_nr;

    pthread_mutex_lock(&SMV->shm_rdwr);
    pthread_mutex_lock(&SMV->market_access);
    if( check_regex(args,"[a-zA-Z0-9_-]+") == 0){
            
        //checkar se o market introduzido está na lista de markets 
        //permitidos ao user
        for(int i = 0;i<NUMBER_MARKETS;i++){

            if( !strcmp(args, SMV->market_list[i].name) && (SMV->users_list[client_number].available_markets[i] == 1)){
                market_nr = i;
                break;
            }

            if(i == NUMBER_MARKETS-1){
                write(fd,"--- MERCADO INVÁLIDO OU NÃO ACESSÍVEL ---",45);
                pthread_mutex_unlock(&SMV->market_access);
                pthread_mutex_unlock(&SMV->shm_rdwr);
                return;
            }

        }
    
    }

    pthread_mutex_unlock(&SMV->market_access);
    pthread_mutex_unlock(&SMV->shm_rdwr);


    //send multicast group ip to the user
    if(market_nr == 0)
        write(fd,MULTICAST_MARKET1,sizeof(MULTICAST_MARKET1));
    else if(market_nr == 1)
        write(fd,MULTICAST_MARKET2,sizeof(MULTICAST_MARKET2));

    

}

void wallet(int client_number, int fd){

    char buffer[BUFSIZ];

    pthread_mutex_lock(&SMV->shm_rdwr);
    snprintf(buffer,BUFSIZ,"--- SALDO: %d ---",SMV->users_list[client_number].balance);
    write(fd,buffer,BUFSIZ);

    for(int i = 0;i< NUMBER_MARKETS*NUMBER_STOCKS_PER_MARKET;i++){

        if( strcmp(SMV->users_list[client_number].user_stocks[i].name,"\0") != 0){
            snprintf(buffer,BUFSIZ,"--- STOCK: %d | NUMERO STOCKS: %d ---",SMV->users_list[client_number].user_stocks[i].name,SMV->users_list[client_number].user_stocks[i].num_stocks);
            write(fd,buffer,BUFSIZ);
        }

    }
    pthread_mutex_lock(&SMV->shm_rdwr);

    write(fd,"FIM",BUFSIZ);

}