#include "header.h"
 
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