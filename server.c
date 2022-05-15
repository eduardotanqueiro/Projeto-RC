#include "header.h"
 
int main(int argc, char** argv){

    if(argc == 4){
        init( atoi(argv[1]), atoi(argv[2]) ,argv[3] );
    }
    else{
        fprintf(stderr,"Wrong command format\n");
        exit(1);
    }

    //TODO CTRL+C


    pthread_t wait_clients_thread;
    pthread_create(&wait_clients_thread,NULL,wait_clients,NULL);
    pthread_detach(wait_clients_thread); //TIRAR!!!!

    /*
    pid_t wait_clients_pid; //TROCAR PARA UMA THREAD?????
    if( (wait_clients_pid = fork()) == 0){
        wait_clients();
        exit(0);
    }
    */

    //Thread que dá manage aos valores da bolsa
    pthread_t thread_bolsa;
    pthread_create(&thread_bolsa,NULL,ManageBolsa,NULL);
    pthread_detach(thread_bolsa); // sim ou não??? mais vale no ctrl+C esperar pela thread

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
                    //kill(wait_clients_pid,SIGINT); TODO MANDAR SINAL PARA AS THREADS ACABAREM!!!   
                    break;
                }

                printf("[MAIN] a sair do fd isset config\n");
            }


            printf("[MAIN] Looping\n");

        }


    }

    printf("[MAIN] Closing server...\n");

    //cleanup
    //cleanup();
    close(fd_bolsa);
    close(fd_config);
    
    for(int i = 0; i< NUMBER_MARKETS; i++)
        close(fd_multicast_markets[i]);

    pthread_mutexattr_destroy(&SMV->attr_mutex);
    pthread_mutex_destroy(&SMV->shm_rdwr);
    pthread_mutex_destroy(&SMV->market_access);

    shmdt(SMV);
    shmctl(shmid, IPC_RMID, NULL);

    exit(0);
}


void* ManageBolsa(){

    //TODO
    //Criar grupos multicast
    //Gerar valores aleatórios para as ações
    int sent_value,addrlen = sizeof(addr_multicast_markets[0]),refresh;
    char buffer[BUFSIZ];
    

    while(1){
        

        //GERAR NOVOS VALORES PARA AS BOLSAS
        pthread_mutex_lock(&SMV->market_access);

        for(int i= 0;i<NUMBER_MARKETS;i++){

            for(int k = 0; k < NUMBER_STOCKS_PER_MARKET; k++){

                //PRICE
                if(SMV->market_list[i].stock_list[k].value == 0.01)
                    SMV->market_list[i].stock_list[k].value += 0.01;

                else if( rand()%2 == 0)
                    SMV->market_list[i].stock_list[k].value += 0.01;
                
                else
                    SMV->market_list[i].stock_list[k].value -= 0.01;


                //STOCK NUMBER
                if(SMV->market_list[i].stock_list[k].num_stocks == 10)
                    SMV->market_list[i].stock_list[k].num_stocks += 10;

                else if (SMV->market_list[i].stock_list[k].num_stocks == 100)
                    SMV->market_list[i].stock_list[k].num_stocks -= 10;

                else if( rand()%2 == 0)
                    SMV->market_list[i].stock_list[k].num_stocks += 10;
                
                else
                    SMV->market_list[i].stock_list[k].num_stocks -= 10;


                //CAST NEW VALUES FOR USERS IN MULTICAST GROUP
                memset(buffer,0,BUFSIZ);
                snprintf(buffer,BUFSIZ,"--- MARKET: %s | STOCK: %s | PRICE: %f | STOCKS AVAILABLE: %d ---\n",SMV->market_list[i].name,SMV->market_list[i].stock_list[k].name,SMV->market_list[i].stock_list[k].value,SMV->market_list[i].stock_list[k].num_stocks);
                printf("%s",buffer); //debug

                sent_value = sendto(fd_multicast_markets[i], buffer, BUFSIZ, 0, (struct sockaddr *) &addr_multicast_markets[i], addrlen);

                if(sent_value < 0)
                    perror("SERVER ERROR, SENDTO");
                
            }
        }
        pthread_mutex_unlock(&SMV->market_access);

        //REFRESH TIME
        pthread_mutex_lock(&SMV->shm_rdwr);
        refresh = SMV->refresh_time;
        pthread_mutex_unlock(&SMV->shm_rdwr);

        sleep(refresh);

    }


    pthread_exit(NULL);
}

void cleanup(){

    //parar a thread da bolsa

    close(fd_bolsa);
    close(fd_config);

    pthread_mutexattr_destroy(&SMV->attr_mutex);
    pthread_mutex_destroy(&SMV->shm_rdwr);
    pthread_mutex_destroy(&SMV->market_access);

    shmdt(SMV);
    shmctl(shmid, IPC_RMID, NULL);

}

void init(int porto_bolsa, int porto_config, char* cfg){

    //Open Config File
    FILE* initFile;
    if ( (initFile = fopen(cfg,"r")) == NULL)
    {
        printf("Non-existent config file!!");
        exit(1);
    }

    //TODO
    //CHECK FILE WITH REGEX EXPRESSION BEFORE READING ANYTHING FOR VARIABLES

    //Create Shared Memory
    shmid = shmget(IPC_PRIVATE, sizeof(shm_vars), IPC_CREAT | IPC_EXCL | 0700);
    if (shmid < 1){
		printf("Error creating shm memory!");
		exit(1);
	}

    SMV = (shm_vars*)shmat(shmid,NULL,0);
    if (SMV < (shm_vars*) 1){
		printf("Error attaching memory!");
        //cleanup(); TODO
		exit(1);
	}

    //Open Semaphores
    pthread_mutexattr_init(&SMV->attr_mutex);
    pthread_mutexattr_setpshared(&SMV->attr_mutex,PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&SMV->shm_rdwr,&SMV->attr_mutex);
    pthread_mutex_init(&SMV->market_access,&SMV->attr_mutex);
    // sem_unlink("SHM_RDWR");
    // SMV->shm_rdwr = sem_open("SHM_RDWR", O_CREAT | O_EXCL,0700,1);

    SMV->refresh_time = INITIAL_REFRESH_TIME;

    //Read Admin Auth
    char buf[BUFSIZ],*tok;

    fscanf(initFile,"%[^\n]",&buf[0]);
    
    tok = strtok(buf,"/");
    strcpy(admin.username,tok);

    tok = strtok(NULL, "\n");
    strcpy(admin.password,tok);
    

    //Read Number Users
    fscanf(initFile,"%d\n",&SMV->number_users);

    if( SMV->number_users > 5){
        printf("Maximum number of initial users exceeded!\n");
        exit(1);
    }

    //Read Users Auth
    for(int i = 0;i< SMV->number_users;i++){

        fscanf(initFile,"%30[^;];%30[^;];%f\n",&SMV->users_list[i].username[0],&SMV->users_list[i].password[0],&SMV->users_list[i].balance);

        for(int j = 0;j< NUMBER_MARKETS*NUMBER_STOCKS_PER_MARKET;j++)
            memset(&SMV->users_list[i].user_stocks[j].name,0,MAX_STRING_SIZES);

        //TODO METER OS USERS NOS GRUPOS MULTICAST (SECALHAR NÃO, SÓ METE QUANDO O USER PEDE)
        for(int j = 0;j< NUMBER_MARKETS;j++)
            SMV->users_list[i].available_markets[j] = 1;
    }


    //Clean the empty spots for users in the userlist
    for(int i = SMV->number_users;i<10;i++){
        memset(SMV->users_list[i].username,0,31);
        memset(SMV->users_list[i].password,0,31);
        SMV->users_list[i].balance = 0;

        for(int j = 0;j< NUMBER_MARKETS*NUMBER_STOCKS_PER_MARKET;j++)
            memset(&SMV->users_list[i].user_stocks[j].name,0,MAX_STRING_SIZES);

    }


    //Read Markets/Stocks
    for(int i = 0;i< NUMBER_STOCKS_PER_MARKET;i++){
        fscanf(initFile,"%30[^;];%30[^;];%f\n",&SMV->market_list[0].name[0],&SMV->market_list[0].stock_list[i].name[0],&SMV->market_list[0].stock_list[i].value);
        SMV->market_list[0].stock_list[i].num_stocks = 50;
    }
    for(int i = 0;i<NUMBER_STOCKS_PER_MARKET;i++){
        fscanf(initFile,"%30[^;];%30[^;];%f\n",&SMV->market_list[1].name[0],&SMV->market_list[1].stock_list[i].name[0],&SMV->market_list[1].stock_list[i].value);
        SMV->market_list[0].stock_list[i].num_stocks = 50;
    }

    //DEBUG
    for(int i = 0;i<2;i++){
        for(int k = 0;k<3;k++){
            printf("DEUBG %s %s %f\n",&SMV->market_list[i].name[0],&SMV->market_list[i].stock_list[k].name[0],SMV->market_list[i].stock_list[k].value);
        }
    }

    //

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

    bzero((void *) &addr_multicast_markets[0], sizeof(addr_multicast_markets[0]) );
    addr_multicast_markets[0].sin_family = AF_INET;
    addr_multicast_markets[0].sin_addr.s_addr = inet_addr(MULTICAST_MARKET1);
    addr_multicast_markets[0].sin_port = htons(porto_bolsa);

    bzero((void *) &addr_multicast_markets[1], sizeof(addr_multicast_markets[1]) );
    addr_multicast_markets[1].sin_family = AF_INET;
    addr_multicast_markets[1].sin_addr.s_addr = inet_addr(MULTICAST_MARKET2);
    addr_multicast_markets[1].sin_port = htons(porto_bolsa);


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

    //Multicast Market1
    int multicastTTL = 255;\

    fd_multicast_markets[0] = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (setsockopt(fd_multicast_markets[0], IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){ 
        perror("socket opt"); 
        exit(1);
    }

    //Multicast Market2
    fd_multicast_markets[1] = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(fd_multicast_markets[1], IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){ 
        perror("socket opt"); 
        exit(1);
    }


    //Socket Config
    if ( (fd_config = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	  erro("na funcao socket config");
    if ( bind(fd_config,(struct sockaddr*)&addr_config,sizeof(addr_config)) < 0)
	  erro("na funcao bind config");

}

void erro(char *s) {
	perror(s);
	exit(1);
}