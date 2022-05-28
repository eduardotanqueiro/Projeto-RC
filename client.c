#include "header.h"

void* wait_clients(){
   
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int logged_clients = 0; 
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd_bolsa,&read_set);


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
                    signal(SIGINT,SIG_DFL);
                    handle_client(client_fd);
                    close(client_fd);
                    logged_clients--;
                    exit(0);
                }

                close(client_fd);
            }


        }

    }

}



int client_login(int fd){

    char* token;
    char buffer[BUFFER_SIZE];
    int number_client = -1;  // check if login succeded

    do{
        char username[31];
        char *password;

        memset(buffer,0,BUFFER_SIZE);
        write(fd,"Introduza as credenciais no formato Username/Password: ",56);
        read(fd,buffer,BUFFER_SIZE);
        buffer[strlen(buffer)] = 0;

        token = strtok(buffer,"/");
        strcpy(username,token);

        password = strtok(NULL,"/\n");

        printf("[USER] Attempted user:%s login!\n", username);

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
            write(fd,"Username ou password errado(s)!",33);

    }while(1);

    printf("[USER] Login Bem Sucedido\n");
    write(fd,"Login Bem Sucedido",20);

    return number_client;

}



void handle_client(int fd){

    char buffer[BUFFER_SIZE];
    int nread;
    int client_number = client_login(fd); //LOGIN CLIENT


    if ( client_number != -1){

        usleep(1000);
        //send client available markets
        send_client_markets(client_number,fd);

        char comando[50];
        char* token;

        //MENU

        while( strcmp(comando,"SAIR") != 0){

            //receive messagem from user console
            printf("\n[SERVER-USER] Waiting for user command\n");
            memset(buffer,0,BUFFER_SIZE);

            nread = read(fd,buffer,BUFFER_SIZE);

            if(nread == 0)
                break;

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
    char buffer[BUFFER_SIZE];

    pthread_mutex_lock(&SMV->shm_rdwr);
    for(int i = 0; i<NUMBER_MARKETS;i++)
        if(SMV->users_list[client_number].available_markets[i] == 1)
            nr_markets++;

    //printf("SENDING NUMBER MARKETS FOR CLIENT %d\n",nr_markets);

    memset(buffer,0,BUFFER_SIZE);
    snprintf(buffer,BUFFER_SIZE,"%d",nr_markets);
    //printf("BEFORE SENDING NUM MARKETS %s\n",buffer);
    write(fd,buffer,BUFFER_SIZE);

    //printf("SENDING MARKETS FOR CLIENTS\n");

    pthread_mutex_lock(&SMV->market_access);
    for(int i = 0; i<NUMBER_MARKETS;i++){

        if(SMV->users_list[client_number].available_markets[i] == 1){ //if user has access to this market
            //send market name
            memset(buffer,0,BUFFER_SIZE);
            snprintf(buffer,BUFFER_SIZE,"%s",SMV->market_list[i].name);
            write(fd,buffer,BUFFER_SIZE);

            //send markets' stock info
            for(int j = 0;j<NUMBER_STOCKS_PER_MARKET;j++){
                memset(buffer,0,BUFFER_SIZE);
                snprintf(buffer,BUFFER_SIZE,"--- STOCK: %s | PRICE: %f ---",SMV->market_list[i].stock_list[j].name,SMV->market_list[i].stock_list[j].value);
                write(fd,buffer,BUFFER_SIZE);  
            }

        }
    }
    pthread_mutex_unlock(&SMV->market_access);

    pthread_mutex_unlock(&SMV->shm_rdwr);

}

int buy(char* args,int client_number, int fd){
    char buffer[BUFFER_SIZE];
    int break_flag = 0;
    
    char *tok, *resto, nome_stock[MAX_STRING_SIZES];
    int num_acoes;
    float preco;

    //Get arguments
    tok = strtok_r(args,"/",&resto);
    strcpy(nome_stock,tok);

    tok = strtok_r(NULL,"/",&resto);
    num_acoes = atoi(tok);

    tok = strtok_r(NULL,"/",&resto);
    preco = atof(tok);

    //check saldo
    pthread_mutex_lock(&SMV->shm_rdwr);

    if( ((float)num_acoes)*preco > SMV->users_list[client_number].balance)
        write(fd,"SALDO INSUFICIENTE PARA EFETUAR A COMPRA",41);


    //Check stock price
    pthread_mutex_lock(&SMV->market_access);

    for(int i = 0; i < NUMBER_MARKETS; i++){

        if(break_flag)
            break;
        
        for(int k = 0; k < NUMBER_STOCKS_PER_MARKET; k++){

            if( !strcmp(nome_stock,SMV->market_list[i].stock_list[k].name) ){
                
                break_flag = 1;

                if( SMV->users_list[client_number].available_markets[i] == 0){
                    //User doesnt has access to this market
                    write(fd,"AÇÃO INVÁLIDA OU MERCADO INACESSÍVEL",40);
                    break;
                }

                if( SMV->market_list[i].stock_list[k].value > preco){
                    //User buy price its not enough
                    write(fd,"ORDEM RECUSADA: O PRECO DE MERCADO É SUPERIOR",47);
                    break;
                }

                //Do the purchase
                if( SMV->market_list[i].stock_list[k].num_stocks < num_acoes){

                    SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].num_stocks =  SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].num_stocks + SMV->market_list[i].stock_list[k].num_stocks;
                    snprintf(buffer,BUFFER_SIZE,"COMPRADAS %d AÇÕES",SMV->market_list[i].stock_list[k].num_stocks);

                }else{

                    SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].num_stocks =  SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].num_stocks + num_acoes;
                    snprintf(buffer,BUFFER_SIZE,"AÇÕES COMPRADAS");

                }
                write(fd,buffer,BUFFER_SIZE);

                if( SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].name[0] == 0 )
                    strcpy(SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].name,nome_stock);

                SMV->users_list[client_number].user_stocks[ ( (i+1)*(k+1) ) - 1 ].value = SMV->market_list[i].stock_list[k].value;
                SMV->users_list[client_number].balance -= SMV->market_list[i].stock_list[k].value * num_acoes;

                break;

            }

        }

    }

    pthread_mutex_unlock(&SMV->market_access);

    pthread_mutex_unlock(&SMV->shm_rdwr);

    if(break_flag == 0){
        //Se não encontrou o stock
        write(fd,"AÇÃO INVÁLIDA OU MERCADO INACESSÍVEL",40);
    }

    printf("Bought!\n");
    return 0;
}

int sell(char* args,int client_number, int fd){
    char buffer[BUFFER_SIZE];
    int break_flag = 0;
    int nr_mercado = -1;
    int nr_stocks_no_mercado;

    char *tok, *resto, nome_stock[MAX_STRING_SIZES];
    int num_acoes;
    float preco;

    //Get arguments
    tok = strtok_r(args,"/",&resto);
    strcpy(nome_stock,tok);

    tok = strtok_r(NULL,"/",&resto);
    num_acoes = atoi(tok);

    tok = strtok_r(NULL,"/",&resto);
    preco = atof(tok);

    pthread_mutex_lock(&SMV->shm_rdwr);
    //Check if user has given stock
    for(int i = 0; i < NUMBER_MARKETS*NUMBER_STOCKS_PER_MARKET  ; i++){

        if ( i % NUMBER_STOCKS_PER_MARKET == 0)
            nr_mercado++;

        if(break_flag)
            break;


        if( !strcmp(nome_stock,SMV->users_list[client_number].user_stocks[i].name) ){
            //user has that stock
            break_flag = 1;

            //check wether user has given stock number to sell
            if( SMV->users_list[client_number].user_stocks[i].num_stocks < num_acoes){
                write(fd,"NÃO TENS AÇÕES SUFICIENTES PARA REALIZAR ESTA OPERAÇÃO",60);
                break;
            }

            //check market price and number of stocks on the market
            pthread_mutex_lock(&SMV->market_access);

            nr_stocks_no_mercado = SMV->market_list[nr_mercado].stock_list[ i % NUMBER_STOCKS_PER_MARKET].num_stocks;
            // printf("%d %d %d %d\n",nr_stocks_no_mercado, SMV->market_list[nr_mercado].stock_list[ i % NUMBER_STOCKS_PER_MARKET].num_stocks,nr_mercado,i % NUMBER_STOCKS_PER_MARKET);


            if( preco > SMV->market_list[nr_mercado].stock_list[ i % NUMBER_STOCKS_PER_MARKET].value){
                //
                write(fd,"ORDEM RECUSADA: O PRECO DE COMPRA DO MERCADO É INFERIOR",57);
                pthread_mutex_unlock(&SMV->market_access);
                break;

            }

            pthread_mutex_unlock(&SMV->market_access);


            //do the sale
            if( nr_stocks_no_mercado < num_acoes){
                
                SMV->users_list[client_number].user_stocks[i].num_stocks -= nr_stocks_no_mercado;
                snprintf(buffer,BUFFER_SIZE,"VENDIDAS %d AÇÕES",nr_stocks_no_mercado);
                SMV->users_list[client_number].balance += nr_stocks_no_mercado*preco;

            }
            else{

                SMV->users_list[client_number].user_stocks[i].num_stocks -= num_acoes;
                snprintf(buffer,BUFFER_SIZE,"AÇÕES VENDIDAS");
                SMV->users_list[client_number].balance += num_acoes*preco;

            }

            write(fd,buffer,BUFFER_SIZE);

            if(SMV->users_list[client_number].user_stocks[i].num_stocks == 0)
                memset(SMV->users_list[client_number].user_stocks[i].name,0,MAX_STRING_SIZES);




        }


    }

    pthread_mutex_unlock(&SMV->shm_rdwr);

    if(break_flag == 0)
        write(fd,"AÇÃO INVÁLIDA OU MERCADO INACESSÍVEL",41);


    printf("Sold!\n");
    return 0;
}

void subscribe(char* args,int client_number, int fd){

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

    char buffer[BUFFER_SIZE];

    pthread_mutex_lock(&SMV->shm_rdwr);
    snprintf(buffer,BUFFER_SIZE,"--- SALDO: %f ---",SMV->users_list[client_number].balance);
    write(fd,buffer,BUFFER_SIZE);

    for(int i = 0;i< NUMBER_MARKETS*NUMBER_STOCKS_PER_MARKET;i++){

        if( strcmp(SMV->users_list[client_number].user_stocks[i].name,"\0") != 0){
            snprintf(buffer,BUFFER_SIZE,"--- STOCK: %s | NUMERO STOCKS: %d ---",SMV->users_list[client_number].user_stocks[i].name,SMV->users_list[client_number].user_stocks[i].num_stocks);
            write(fd,buffer,BUFFER_SIZE);
        }

    }
    pthread_mutex_unlock(&SMV->shm_rdwr);

    snprintf(buffer,BUFFER_SIZE,"FIM");
    write(fd,buffer,BUFFER_SIZE);

}

int check_regex(char *text, char *regex){

    regex_t reg;

    regcomp(&reg,regex,REG_EXTENDED);

    int rt = regexec(&reg,text,0,NULL,0);

    regfree(&reg);

    return rt;
}