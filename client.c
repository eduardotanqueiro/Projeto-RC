#include "header.h"

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

            //printf("[MAIN] Looping\n");

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