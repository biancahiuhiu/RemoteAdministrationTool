#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVERPORT 12345
#define MAX_SIZE 512
int sessionRunning=1;

void accessMenu(int client_socket)
{
    printf("---Meniu---\n");
    printf("1.Informatii despre sistem.\n");
    printf("2.Trimite o comanda.\n");
    printf("3.Exit.\n");
    printf("Optiune: ");
    int opt;
    scanf("%d",&opt);

    switch(opt)
    {
        case 1:
            printf("Info sistem.\n");
            break;
        case 2:
            printf("Trimite comanda.\n");
            break;
        case 3:
            printf("Delogare...\n");
            sessionRunning=0;
            close(client_socket);
            break;
        default:
            break;
    }
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char server_ip[] = "10.0.2.15";  // Adresa IP a serverului
    
    // Crearea socket-ului
    client_socket = socket(AF_INET, SOCK_STREAM, 0);   //returneaza descriptorul de fisier asociat socketului creat ; AF_INET=familia de prot IPv4, SOCK_STREAM=socket de ttip TCP si ) e protocolul implicit
    if (client_socket == -1) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    // Setarea atributele serverului
    server_address.sin_family = AF_INET;  //indică faptul că se utilizează adrese IPv4, seteaza familia de adrese acolo
    server_address.sin_port = htons(SERVERPORT);
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {   //se transforma adresa IP a serverului
        perror("Eroare la transformarea adresei IP");
        exit(EXIT_FAILURE);
    }

    // Conectarea la server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {   //se conecteaza la client, primul arg este descriptorul de fisier creat mai sus, al doilea adresa si inf despre server(port si adresa IP) si ultimul este lungimea server_address
        perror("Eroare la conectarea la server");
        exit(EXIT_FAILURE);
    }

    printf("Conexiunea la server s-a realizat cu succes.\n");

    // Aici puteți trimite și primi date de la server, folosind `send` și `recv`.
    printf("Introduceti datele:\n");
    char username[MAX_SIZE];
    char password[MAX_SIZE];
    printf("Nume utilizator: ");
    fgets(username,sizeof(username),stdin);
    username[strcspn(username,"\n")]='\0';
    send(client_socket,username,strlen(username),0);

    printf("Parola: ");
    fgets(password,sizeof(password),stdin);
    password[strcspn(password,"\n")]='\0';
    send(client_socket,password,strlen(password),0);
    
    char buff[1024];
    memset(buff,0,sizeof(buff));
    read(client_socket,buff,sizeof(buff));

    if(strcmp(buff,"Autentificare reusita.")==0){
        while(sessionRunning){
             accessMenu(client_socket);
        }
    }
    else if(strcmp(buff,"Autentificare esuata.")==0){
        printf("Utilizatorul sau parola sunt gresite.");
        close(client_socket);
    }

    // Închidem conexiunea
    //close(client_socket);

    return 0;
}