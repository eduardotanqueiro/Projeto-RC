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

void client_login(){
    char buffer[BUFSIZ];
    int recv_len;
    char comando[50];
    char* token;

    struct sockaddr user_connect;
    socklen_t slen = sizeof(user_connect);

    //NETCAT CONNECTING STRINGS (TO IGNORE)
    for(int k = 0;k<5;k++){
        recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &user_connect, (socklen_t *)&slen);
    }


    do{
        char username[31];
        char *password;
        
        memset(buffer,0,BUFSIZ);
        sendto(fd_config,"Introduza as credenciais no formato Username/Password: ",56,0,(struct sockaddr*)&user_connect,(socklen_t )slen);
        recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &user_connect, (socklen_t *)&slen );
        buffer[strlen(buffer) - 1] = 0;

        token = strtok(buffer,"/");
        strcpy(username,token);

        password = strtok(NULL,"/\n");

        printf("[SERVER] Attempted user:%s login!\n", username);

        if(!strcmp(username, "QUIT")){
            printf("[USER] A fechar processo do user:%s\n", username);
            return 0;
        }

        bool flag = false; // check if user exists (true == yes, false == no)
        for(int i = 0; i < 10; i++){
            if(strcmp(username, users_list[i].username) == 0 && strcmp(password, users_list[i].password) == 0){
                sendto(fd_config, "Login bem sucedido!\n", 20, 0, (struct sockaddr*)&user_connect,(socklen_t) slen);
                flag = true;
                break;
            }
            else if(strcmp(username, users_list[i].username) == 0 && strcmp(password, users_list[i].password) != 0){
                sendto(fd_config, "Password errada!\n", 34, 0, (struct sockaddr*)&user_connect,(socklen_t) slen);
                flag = true;
            }
        }

        if(!flag){
            sendto(fd_config, "User não existe!\n", 34, 0, (struct sockaddr*)&user_connect,(socklen_t) slen);
        }

    }while(1);

    printf("[USER] Login Bem Sucedido\n");

    //MENU
    while( strcmp(comando,"QUIT") != 0){

        //receive messagem from user console
        printf("\n[USER] Waiting for user command\n");
        memset(buffer,0,BUFSIZ);

        if( (recv_len = recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &user_connect, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom\n");
            return 1;
        }

        //Get Command

        char *resto;
        memset(comando,0,strlen(comando));
        
        token = strtok(buffer, " \n");
        strcpy(comando,token);

        resto = strtok(NULL,"");

        if(!strcmp(comando, "QUIT_SERVER"))
            return 10;

        else if(!strcmp(comando, "BUY"))
            buy(resto, user_connect);

        else if(!strcmp(comando, "SELL"))
            sell(resto, user_connect);

        // TODO menu
}

int buy(char* args, struct sockaddr addr){
    printf("Bought!\n");
    return 0;
}

int sell(char* args, struct sockaddr addr){
    printf("Sold!\n");
    return 0;
}

void handle_client(int fd){

    char buffer[BUFSIZ];
    int n = -1;

    //  TODO
    //  -LOGIN
    //  -MENU

    client_login();

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