#include "header.h" // TODO fazer header files separados e 1 comum só com estruturas e imports
#include "client_program.h"

int main(int argc, char* argv){

    char endServer[100];
    struct hostent *hostPtr;

    //argv arguments
    if (argc != 3) {
        printf("cliente <host> <port>\n");
        exit(1);
    }

    porto = atoi(argv[2]);
    feed_on = 1;
    pthread_mutex_init(&check_feed,PTHREAD_MUTEX_INITIALIZER);


    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Host Address not Reachable");


    //Set TCP socket and connect
    bzero((void *) &addr_connect, sizeof(addr_connect));
    addr_connect.sin_family = AF_INET;
    addr_connect.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr_connect.sin_port = htons((short) porto);

    if ( (fd_server = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	    erro("na funcao socket server");
        exit(1);
    }


    if ( connect(fd_server,(struct sockaddr*)&addr_connect,sizeof(addr_connect)) < 0){
        erro("na funcao connect server");
        exit(1);
    }

    //Login
    if( login() == 1){
        //CLOSE PROGRAM
        //cleanup
    }

    //TODO Receive Markets Info
    int number_markets = ReceiveMarketsInfo();
    

    //thread for receiving UDP Multicast Updates
    // pthread_t updates_handler;
    // pthread_create(&updates_handler,NULL,HandleStocksUpdates,0);
    // pthread_detach(updates_handler); //??

    //
    TalkWithServer();


    //cleanup

    return 0;
}

int login(){

    char buffer[BUFSIZ] = "\0";
    int nread;

    while( strcmp(buffer,"Login Bem Sucedido\n") != 0){

        read(fd_server,buffer,BUFSIZ);
        printf("--- %s ---",buffer);

        memset(buffer,0,BUFSIZ);
        scanf("%s",buffer);

        write(fd_server,buffer,BUFSIZ);

        if(!strcmp(buffer,"QUIT\n")) //sair sem login
            return 1;

        memset(buffer,0,BUFSIZ);
        read(fd_server,buffer,BUFSIZ);
        printf("%s",buffer);

    }

    return 0;
}

int ReceiveMarketsInfo(){

    int num_markets_access, nread;
    char buffer[BUFSIZ];

    //number of markets user has access
    read(fd_server,buffer,BUFSIZ);
    num_markets_access = atoi(buffer);


    for(int i = 0;i<num_markets_access;i++){
        //receive market name
        memset(buffer,0,BUFSIZ);
        nread = read(fd_server,buffer,BUFSIZ);
        buffer[nread] = '\0';

        strcpy(available_markets[i],buffer);

    }

    for(int i = num_markets_access;i<NUMBER_MARKETS;i++){
        //empty left slots
        memset(available_markets[i],0,MAX_STRING_SIZES);
    }


    return num_markets_access;
}

void TalkWithServer(){

    char buffer[BUFSIZ];
    int nread,check_input,number_instruction;

    printf("--- MENU ---\n");
    printf("--- 1: SUBSCREVER COTAÇÕES DE MERCADOS ---\n");
    printf("--- 2: COMPRAR AÇÕES ---\n");
    printf("--- 3: VENDER AÇÕES ---\n");
    printf("--- 4: VERIFICAR ESTADO DA CARTEIRA ---\n");
    printf("--- 5: DESATIVAR/ATIVAR FEED DE ATUALIZAÇÃO DE AÇÕES ---\n");
    printf("--- 6: SAIR ---\n");


    while( number_instruction != 6 ){

        check_input = 0;
        number_instruction = -1;

        while( !check_input ){

            printf("--- INTRODUZA UMA INSTRUÇÃO --- \n");

            memset(buffer,0,BUFSIZ);
            scanf("%s",buffer);

            number_instruction = atoi(buffer);

            switch (number_instruction){
                case 1:

                    TrySubscribeMarket();
                    check_input = 1;
                    break;
                case 2:

                    BuyOrSellStock(1);
                    check_input = 1;
                    break;
                case 3:

                    BuyOrSellStock(2);
                    check_input = 1;
                    break;
                case 4:

                    AskWalletStatus();
                    check_input = 1;
                    break;
                case 5:

                    ChangeStateFeed();
                    check_input = 1;
                    break;
                case 6:

                    strcpy(buffer,"SAIR");
                    check_input = 2;
                    break;

                default:
                    printf("--- INSTRUÇÃO INVÁLIDA! ---\n");
            }

        }



    }

}

void BuyOrSellStock(int mode){

    int check_input = 0;
    char aux[BUFSIZ-10];
    char buffer[BUFSIZ];

    if (mode == 1)
        strcpy(buffer,"COMPRAR ");
    else
        strcpy(buffer,"VENDER ");

    while( check_input == 0){

        printf("--- INTRODUZA O COMANDO NO FORMATO: NOME_DO_STOCK NUMERO_ACOES PRECO ---\n");
        scanf("%s",aux);

        if( check_regex(aux,"([a-zA-Z0-9]+) ([0-9]+) ([0-9]+(\r\n|\r|\n)?)") == 0)
            check_input = 1;

    }

    strcat(buffer,aux);

    //Send command to server
    write(fd_server,buffer,BUFSIZ);

    //Receive Response from server
    memset(buffer,0,BUFSIZ);
    read(fd_server,buffer,BUFSIZ);
    printf("%s\n",buffer);

}


void TrySubscribeMarket(){

    int check_string = 0;
    char aux[BUFSIZ - 11];
    char buffer[BUFSIZ];
    strcpy(buffer,"SUBSCREVER ");

    while( !check_string){
        
        printf("--- INTRODUZA O COMANDO NO FORMATO: MARKET_NAME ---\n");
        scanf("%s",aux);

        if( check_regex(aux,"[a-zA-Z0-9_-]+") == 0)
            
            //TODO checkar se o market introduzido está na lista de markets 
            //permitidos ao user (available_markets)
            check_string = 1;

    }

    strcat(buffer,aux);

    //Send command to server
    write(fd_server,buffer,BUFSIZ);

    //Receive response from server
    memset(buffer,0,BUFSIZ);
    read(fd_server,buffer,BUFSIZ);

    //Create thread that receives updates from ip received in buffer
    pthread_t market;
    pthread_create(&market,NULL,HandleStocksUpdates,(void*)buffer);
    pthread_detach(market);

}

void AskWalletStatus(){


    char buffer[BUFSIZ];
    strcpy(buffer,"CARTEIRA");

    //Send command to server
    write(fd_server,buffer,BUFSIZ);

    memset(buffer,0,BUFSIZ);

    //Receive response
    while( strcmp(buffer,"FIM") != 0 ){
        
        read(fd_server,buffer,BUFSIZ);
        printf("%s\n",buffer);

    }

}


void ChangeStateFeed(){

    pthread_mutex_lock(&check_feed);

    if(feed_on)
        feed_on = 0;
    else
        feed_on = 1;

    pthread_mutex_unlock(&check_feed);

}

void* HandleStocksUpdates(void* ip_multicast){

    char* ip_str = (char*) ip_multicast;
    char buffer[BUFSIZ];

    struct sockaddr_in addr_multicast;
    int addrlen, socket_multicast, nread,  multicastTTL = 255;
    struct ip_mreq mreq;

    /* set up socket */ 
    if ((socket_multicast = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket error");
        pthread_exit(NULL);
    }

    // change socket behavior for multicast connections
    if (setsockopt(socket_multicast, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){ 
        perror("set socket error");
        pthread_exit(NULL);
    }

    bzero((char *)&addr_multicast, sizeof(addr_multicast)); 
    addr_multicast.sin_family = AF_INET; 
    addr_multicast.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr_multicast.sin_port = htons(porto); 
    addrlen = sizeof(addr_multicast);

    if (bind(socket_multicast, (struct sockaddr *) &addr_multicast, sizeof(addr_multicast)) < 0) { 
        perror("bind error");
        pthread_exit(NULL);
    }

    mreq.imr_multiaddr.s_addr = inet_addr(ip_str); 
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socket_multicast, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) { 
        perror("setsockopt mreq"); 
        pthread_exit(NULL);
    }

    while(1){
        
        nread = recvfrom(socket_multicast,buffer,BUFSIZ,0,(struct sockaddr*) &addr_multicast,(socklen_t *)&addrlen);

        if(nread < 0){
            perror("recvfrom");
            pthread_exit(NULL);
        }
        else if(nread == 0)
            break;


        pthread_mutex_lock(&check_feed);
        if(feed_on)
            printf("%s\n",buffer);
        pthread_mutex_lock(&check_feed);

    }

    //close socket
    close(socket_multicast);

    pthread_exit(NULL);

}

int check_regex(char *text, char *regex){

    regex_t reg;

    regcomp(&reg,regex,REG_EXTENDED);

    int rt = regexec(&reg,text,0,NULL,0);

    regfree(&reg);

    return rt;
}