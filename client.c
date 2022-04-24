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

int client_login(int fd){

    char* token;
    char buffer[BUFSIZ];


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
            return 1;
        }

        bool flag = false; // check if login succeded
        for(int i = 0; i < 10; i++){
            if(strcmp(username, users_list[i].username) == 0 && strcmp(password, users_list[i].password) == 0){
                flag = true;
                break;
            }

        }

        if( flag == true ) //Login Succeded 
            break;

        else
            write(fd,"Username ou password errado(s)!\n",33);

    }while(1);

    printf("[USER] Login Bem Sucedido\n");
    write(fd,"Login Bem Sucedido\n",20);

    return 0;

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

    if ( client_login(fd) == 0){


        char comando[50];
        char* token;

        //MENU
        while( strcmp(comando,"QUIT") != 0){

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

            if(!strcmp(comando, "BUY"))
                buy(resto, fd);

            else if(!strcmp(comando, "SELL"))
                sell(resto, fd);

            // TODO resto do menu
        }

        printf("[USER] User left\n");
    }

}