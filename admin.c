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
    
    //LOGIN
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

    char *tok,*resto,username[31],password[31];
    int balance;
    int markets;


    if(args != NULL){

        tok = strtok_r(args," ",&resto);
        strcpy(username,tok);

        tok = strtok_r(NULL, " ", &resto);
        strcpy(password,tok);

        tok = strtok_r(NULL," ", &resto);
        markets = atoi(tok);

        tok = strtok_r(NULL," ",&resto);
        balance = atoi(tok);


        pthread_mutex_lock(&SMV->shm_rdwr);
        //check number of users
        if(SMV->number_users == MAX_CLIENTS){
            sendto(fd_config,"Maximum number of clients reached!\n",36,0,(struct sockaddr*)&addr,(socklen_t) slen);
            pthread_mutex_unlock(&SMV->shm_rdwr);

            return 1;
        }

        for(int i = 0;i<MAX_CLIENTS;i++){
            if(SMV->users_list[i].username[0] == 0){
                //empty spot
                strcpy(SMV->users_list[i].username,username);
                strcpy(SMV->users_list[i].password,password);
                SMV->users_list[i].balance = balance;

                //adicionar às bolsas a que tem acesso
                SMV->users_list[i].available_markets[0] = (int)(markets * 0.1) % 10;
                SMV->users_list[i].available_markets[1] = markets % 10;

                pthread_mutex_unlock(&SMV->shm_rdwr);

                sendto(fd_config,"New user created!\n",19,0,(struct sockaddr*)&addr,(socklen_t) slen);

                return 0;
            }
        }

        return 1;
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
        
        pthread_mutex_lock(&SMV->shm_rdwr);
        for(int i = 0;i<MAX_CLIENTS;i++){

            if( (SMV->users_list[i].username[0] != 0) && (!strcmp(SMV->users_list[i].username,args)) ){
                
                memset(SMV->users_list[i].username,0,31);
                memset(SMV->users_list[i].password,0,31);
                SMV->users_list[i].balance = 0;

                printf("[ADMIN] User deleted!\n");
                sendto(fd_config,"User deleted!\n",15,0,(struct sockaddr*)&addr,(socklen_t) slen);

                SMV->number_users--;
                pthread_mutex_unlock(&SMV->shm_rdwr);

                return 0;
            }

        }

        pthread_mutex_unlock(&SMV->shm_rdwr);
    }
    printf("[ADMIN] Username not found!\n");
    sendto(fd_config,"Error! Username not found!\n",28,0,(struct sockaddr*)&addr,(socklen_t) slen);
    return 1;
}

int list(struct sockaddr addr){
    printf("[ADMIN] Listing users\n");
    char buffer[BUFSIZ];
    socklen_t slen = sizeof(addr);

    pthread_mutex_lock(&SMV->shm_rdwr);
    for(int i = 0;i<MAX_CLIENTS;i++){

        if( SMV->users_list[i].username[0] != 0){
            memset(buffer,0,BUFSIZ);
            snprintf(buffer,BUFSIZ,"Username: %s\n",SMV->users_list[i].username);
            sendto(fd_config,buffer,BUFSIZ,0,(struct sockaddr*)&addr,(socklen_t) slen);

        }

    }
    pthread_mutex_unlock(&SMV->shm_rdwr);

    return 0;
}

int refresh(char* args, struct sockaddr addr){
    printf("[ADMIN] refreshing time %s\n",args);
    
    if( args != NULL){
        pthread_mutex_lock(&SMV->shm_rdwr);
        SMV->refresh_time = atoi(args);
        pthread_mutex_unlock(&SMV->shm_rdwr);
    }
    //TODO enviar mensagens pro user e debug se vier NULL
    
    return 0;
}
