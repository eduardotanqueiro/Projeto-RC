#include "header.h"


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

    printf("Admin: %s--%s\n",admin.username,admin.password);
    //LOGIN (TODO -> FUNCAO PARA ISTO)
    do{
        char username[31];
        char *password;
        
        memset(buffer,0,BUFSIZ);
        sendto(fd_config,"Introduza as credenciais no formato Username/Password: ",56,0,(struct sockaddr*)&admin_connect,(socklen_t )slen);
        recvfrom(fd_config, buffer, BUFSIZ, 0, (struct sockaddr *) &admin_connect, (socklen_t *)&slen );
        buffer[strlen(buffer) - 1] = 0;

        token = strtok(buffer,"/\n");
        strcpy(username,token);

        password = strtok(NULL,"/\n");

        if( password != NULL){
            password[strlen(password)] = '\0';
            printf("[SERVER] Attempted admin login!\n"); 
            printf("[SERVER] %d--%d\n",strcmp(username,admin.username),strcmp(password,admin.password)); //DEBUG
        }

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

        //Get Command -- ERRO AQUI, VERIFICAR SE O INPUT ESTÁ CORRETO, PODE DAR SEGFAULT!

        char *resto = "";
        memset(comando,0,sizeof(comando));
        
        token = strtok(buffer, " \n");
        strcpy(comando,token);

        resto = strtok(NULL,"\n");



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
    socklen_t slen = sizeof(addr);

    char *tok,username[31],password[31];
    int balance;
    //TODO ACABAR


    if(args != NULL){

        tok = strtok(args," ");
        strcpy(username,tok);

        tok = strtok(NULL, " ");
        strcpy(password,tok);

        return 0;
    }
    else{
        sendto(fd_config,"Invalid User Command Format!\n",30,0,(struct sockaddr*)&addr,(socklen_t) slen);
        return 1;
    }

}

int delete_user(char* args, struct sockaddr addr){

    printf("[ADMIN] deleting user %s\n",args);
    socklen_t slen = sizeof(addr);
    
    if(args != NULL){
        for(int i = 0;i<number_users;i++){

            if( (users_list[i].username[0] != 0) && (!strcmp(users_list[i].username,args)) ){
                
                memset(users_list[i].username,0,31);
                memset(users_list[i].password,0,31);
                users_list[i].balance = 0;

                printf("[ADMIN] User deleted!\n");
                sendto(fd_config,"User deleted!\n",15,0,(struct sockaddr*)&addr,(socklen_t) slen);

                number_users--;
                return 0;
            }

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

        if( users_list[i].username[0] != 0){
            memset(buffer,0,BUFSIZ);
            snprintf(buffer,BUFSIZ,"Username: %s\n",users_list[i].username);
            sendto(fd_config,buffer,BUFSIZ,0,(struct sockaddr*)&addr,(socklen_t) slen);

        }

    }

    return 0;
}

int refresh(char* args, struct sockaddr addr){
    printf("[ADMIN] refreshing time %s\n",args);
    
    if( args != NULL){
        refresh_time = atoi(args);
    }
    
    return 0;
}
