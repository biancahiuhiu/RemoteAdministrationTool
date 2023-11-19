#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAX_SIZE 1024

struct Users{
    char* username;
    char* password;
};

struct Users utilizatori[MAX_SIZE];

void modifyLogger(const char* logFile,char* user,char* status)
{
    FILE* f=fopen(logFile,"a");
    if(f==NULL){
        perror("Error in opening logFile");
        exit(EXIT_FAILURE);
    }
    // cpluscplus.com/reference/ctime/strftime/
    time_t t=time(NULL);
    struct tm *tm=localtime(&t);
    char date[64];
    strftime(date,sizeof(date),"%c",tm);

    fprintf(f,"Date: %s\n",date);
    if(strcmp(user,"-")!=0){
    fprintf(f,"Nume utilizator: %s\n",user);
    }
    fprintf(f,"Status: %s\n",status);
    fprintf(f,"----------------------\n");

    fclose(f);
}

int numberOfUsers(const char* filename)
{
    int count=0;
    FILE* f=fopen(filename,"r");
    if(f==NULL){
        perror("Error in opening file.");
        exit(EXIT_FAILURE);
    }
    char buff[MAX_SIZE];
    while(fgets(buff,1024,f)!=NULL){
        count++;
    }

    fclose(f);
    return count;
}

void addAllAccounts(const char* filename,int nbAcc)
{
    FILE* f=fopen(filename,"r");
    if(f==NULL){
        perror("Error in opening file.");
        exit(-1);
    }

    char buffer[1024];
    int i=0;

    while(fgets(buffer,1024,f)!=NULL && i<nbAcc){
        char* p=strtok(buffer," \t");
        utilizatori[i].username=(char*)malloc(sizeof(char)*strlen(p));
        strcpy(utilizatori[i].username,p);
        
        p=strtok(NULL,"\n");
        utilizatori[i].password=(char*)malloc(sizeof(char)*strlen(p));
        strcpy(utilizatori[i].password,p);
        i++;
    }
}

bool verifyAccount(int client_socket,int users)
{
    char username[MAX_SIZE];
    char password[MAX_SIZE];

    int bytesReadUser=read(client_socket,username,sizeof(username));
    if(bytesReadUser==-1){
        perror("Username error.");
        exit(EXIT_FAILURE);
    }
    username[bytesReadUser]='\0';

    int bytesReadPasswd=read(client_socket,password,sizeof(password));
    if(bytesReadPasswd==-1){
        perror("Password error.");
        exit(EXIT_FAILURE);
    }
    password[bytesReadPasswd]='\0';
    
    for(int i=0;i<users;i++){
        if((strcmp(password,utilizatori[i].password)==0) && (strcmp(username,utilizatori[i].username)==0)){
            modifyLogger("logFile.txt",username,"SUCCESS");
            return true;
        }
    }
    return false;
}



int main() {
    //Gestiunea utilizatorilor
    int nbUsers=numberOfUsers("utilizatori.txt");
    addAllAccounts("utilizatori.txt",nbUsers);
    
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    int server_port = 12345;  // Portul la care serverul ascultă

    // Crearea socket-ului
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    // Setarea atributele serverului
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;    //constanta reprezintă o constantă specială care indică faptul că serverul va asculta la oricare adresă IP disponibilă pe sistem

    // Legăm socket-ul la adresa și portul specificate
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {   // efectuează legarea socket-ului server_socket la adresa specificată în structura server_address și portul specificat în aceeași structură
        perror("Eroare la legarea socket-ului");
        exit(EXIT_FAILURE);
    }

    // Ascultăm pentru conexiuni de la clienți
    if (listen(server_socket, 5) == -1) {   //se referă la procesul de ascultare (listening) a conexiunilor de la clienți pe socket-ul server.
    ///serverul va putea aștepta până la 5 conexiuni în așteptare înainte de a refuza oricare conexiuni ulterioare
        perror("Eroare la ascultarea conexiunilor");
        exit(EXIT_FAILURE);
    }

    printf("Serverul ascultă pe portul %d...\n", server_port);

    // Acceptăm o conexiune de la un client
    socklen_t client_address_len = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
    if (client_socket == -1) {
        perror("Eroare la acceptarea conexiunii de la client");
        exit(EXIT_FAILURE);
    }

    printf("Conexiunea cu clientul s-a realizat cu succes.\n");
    if(verifyAccount(client_socket,nbUsers)==true){
        char* msg="Autentificare reusita.";
        send(client_socket,msg,strlen(msg),0);
    }
    else{
        
        modifyLogger("logFile.txt","-","FAILED");
        char* msg="Autentificare esuata.";
        send(client_socket,msg,strlen(msg),0);
    }


    // Închidem conexiunile
    close(client_socket);
    close(server_socket);

    return 0;
}