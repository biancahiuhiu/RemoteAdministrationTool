#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define SERVERPORT 12345
#define MAX_SIZE 512
int sessionRunning=1;

char* getFileNameFromPath(char *path) 
{
    char *filename = strrchr(path, '/');
    if (filename == NULL) {
        return path;
    } else {
        return filename + 1;
    }
}

void getSysInfo(int client_socket)
{
    char buffer[MAX_SIZE];
    int len=0;

    recv(client_socket,&len,sizeof(int),0);
    recv(client_socket,buffer,len,0);
    buffer[strcspn(buffer,"\n")]='\0';
    int fileSize=atoi(buffer);
    //printf("%d\n",fileSize);

    
    //FILE *received_file = fopen("/home/bianca/PSO_Proiect/RecievedDocumentsClient/sistem.txt", "wb");

    //if (received_file == NULL) {
        //perror("Error opening file");
        //exit(EXIT_FAILURE);
    //}

    int remainData=fileSize;
    //printf("\n%d\n",remainData);
    int bytesReceived=0;
    memset(buffer,0,sizeof(buffer));
    while((remainData>0) && ((bytesReceived=recv(client_socket,buffer,MAX_SIZE,0))>0))
    {
        printf("%s",buffer);
        //fwrite(buffer,sizeof(char),bytesReceived,received_file);
        remainData-=bytesReceived;
    }

    //fclose(received_file);
}

void accessMenu(int client_socket)
{
    printf("\n----MENIU----\n");
    printf("1.Informatii despre sistem.\n");
    printf("2.Trimite o comanda.\n");
    printf("3.Transfera fisiere.\n");
    printf("4.Gestioneaza procesele.\n");
    printf("5.Trimite o comanda personalizata.\n");
    printf("6.Delogare.\n");
    printf("Introduceti optiune: ");
    int opt;
  
    scanf("%d",&opt);
    char opt_char[2];
    sprintf(opt_char,"%d",opt);
    char* msj=NULL; // Define msj to hold messages

    if (opt == 1) {
        msj="infosistem";
    } else if (opt == 2) {
        msj="comanda";
    } else if (opt == 3) {
        msj="transfer";
    } else if (opt == 4) {
        msj="procese";
    } else if (opt == 5) {
        msj="comandapers";
    } else if (opt == 6) {
        msj="delogare";
        sessionRunning = 0;
    }


    send(client_socket, msj, strlen(msj), 0);
    printf("COD TRIMIS: %s\n",msj);

    char mesaj_server[9024];

    switch(opt)
    {
        case 1:
            getSysInfo(client_socket);
            break;
        case 2:
            char* comanda=(char*)malloc(1024*sizeof(char));
            int i=0;
            //printf("Tastati aici comanda pe care doriti sa o introduceti: ");
            write(STDOUT_FILENO,"---COMENZI DIN TERMINAL---",27);
            write(STDOUT_FILENO,"Tastati aici comanda pe care doriti sa o introduceti: ",55);
            read(STDIN_FILENO,comanda,1024);
            send(client_socket,comanda,strlen(comanda),0);
            memset(comanda,0,sizeof(comanda));
            memset(mesaj_server, 0, sizeof(mesaj_server));
            recv(client_socket, mesaj_server, sizeof(mesaj_server), 0);
            printf("Raspunsul serverului:\n%s\n", mesaj_server);
            break;
        case 3:
            printf("\n---TRANSFER DE FISIERE---\n");
            printf("Tasteaza calea catre fisierul pe care doresti sa il trimiti.\n");
    
            char file_to_send[MAX_SIZE];
            
            int readPath = read(0, file_to_send, sizeof(file_to_send));
            if (readPath == -1) {
            perror("Eroare la citirea caii.");
            exit(EXIT_FAILURE);
            }
            file_to_send[strcspn(file_to_send, "\n")] = '\0';
            
        
            char* filename=getFileNameFromPath(file_to_send);
            int len=strlen(filename);
            printf("%d\n",len);
            send(client_socket,&len,sizeof(int),0);
            send(client_socket,filename,len,0);
            
            FILE *file_to_send_ptr = fopen(file_to_send, "rb");

            if (file_to_send_ptr == NULL) {
            perror("Eroare la deschiderea fisierului de trimis.");
            exit(EXIT_FAILURE);
            }

            fseek(file_to_send_ptr,0,SEEK_END);
            int fs=ftell(file_to_send_ptr);
            fseek(file_to_send_ptr,0,SEEK_SET);
            char fileSize[256];
            memset(fileSize,0,sizeof(fileSize));
            sprintf(fileSize,"%d",fs);
            printf("%s\n",fileSize);
            len=strlen(fileSize);
            send(client_socket,&len,sizeof(int),0);
            send(client_socket,fileSize,len,0);

            int sentBytes=0;
            int remainData=fs;
            char buffer[MAX_SIZE];
            while(((sentBytes=fread(buffer,1,BUFFER_SIZE,file_to_send_ptr))>0)&&(remainData>0)){
                //printf("%s",buffer);
                //l=strlen(buffer);
                //printf("%d\n",l);
                send(client_socket,buffer,strlen(buffer),0);
                remainData-=sentBytes;
                //printf("%d\n",remainData);
            }
            fclose(file_to_send_ptr);
            printf("Fisierul %s a fost primit cu succes.",filename);
            break;
        case 4:
            printf("\n---PROCESE---\n");
            while(1)
            {
                int optiune;
                printf("\n");
                printf("1. Listeaza procesele.\n");
                printf("2. Opreste proces.\n");
                printf("3. Exit\n");

                printf("Tastati optiunea aici: ");
                scanf("%d",&optiune);
                while(optiune!=1 && optiune!=2 && optiune!=3)
                {
                    printf("\nOptiunea nu este valida, introduceti o optiune valida: ");
                    scanf("%d",&optiune);
                }
                char optiune_str[2];
                sprintf(optiune_str,"%d",optiune);
                printf("OPTIUNE TRIMISA: %s\n",optiune_str);
                send(client_socket,optiune_str,strlen(optiune_str),0);
                switch(optiune)
                {
                    case 1:
                        char buffer[100000];
                        char piduri[1024];
                        char nr_procese_char[10];
                        strcpy(buffer,"listeazaprocese");
                    
                        send(client_socket,buffer,strlen(buffer),0);
                        memset(buffer,0,sizeof(buffer));

                        recv(client_socket,nr_procese_char,3,0);
                        printf("Numar de procese: %s\n",nr_procese_char);


                        int nr_procese=atoi(nr_procese_char);
                        
                        printf("Raspuns server:\n");
                        for(int i=0;i<nr_procese;i++)
                        {
                            recv(client_socket,buffer,10000,0);
                            recv(client_socket,piduri,1024,0);
                            printf("Nume proces: %s ----> pid-ul %s\n",buffer,piduri);
                            memset(buffer,0,sizeof(buffer));
                            memset(piduri,0,sizeof(piduri));

                        }
                    break;
                    case 2: 
                        char pid_str[100];
                        printf("Tastati aici pidul procesului pe care vreti sa-l opriti: ");
                        scanf("%s",pid_str);
                        send(client_socket,pid_str,strlen(pid_str),0);
                        memset(pid_str,0,sizeof(pid_str));

                        char mesaj_ser[100];
                        recv(client_socket,mesaj_ser,100,0);

                        printf("Mesajul de la server: %s\n",mesaj_ser);
                        memset(mesaj_ser,0,sizeof(mesaj_ser));
                        break;
                    default:
                        break;
                }
                if(optiune==3)
                {
                    break;
                }
            }
            break;
        case 5:
            printf("\n---COMENZI PERSONALIZATE---\n");
            char comanda_pers[1024];
            printf("Comenzile sunt:\n");
            printf("1 ---> afiseaza utilizatori\n");
            printf("2 ---> afiseaza grupurile\n");
            printf("3 ---> afiseaza utilizatorul curent\n");
            printf("4 ---> afiseaza numarul utilizatorilor\n");
            printf("5 ---> cautare utilizator [nume]\n");
            printf("6 ---> afisare informatii [nume utilizator]\n");
            
            write(STDOUT_FILENO,"Tastati aici comanda personalizata: ",37);
            read(STDIN_FILENO,comanda_pers,1024);
            char* comm=strtok(comanda_pers,"\n");
            send(client_socket,comanda_pers,strlen(comanda_pers),0);

            
            char output[4096];
            recv(client_socket,output,4096,0);

            printf("Raspunsul serverului: \n");
            printf("%s\n",output);

            memset(output,0,sizeof(output));

            break;
        case 6:
            printf("Delogare...\n");
            sessionRunning=0;
            close(client_socket);
            break;
        default:
            printf("Tasta incorecta.Mai incearca.\n");
            break;
    }

    opt=0;
    memset(opt_char,0,sizeof(opt_char));
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

    // primire citire date
    printf("---AUTENTIFICARE---\n");
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
        while(sessionRunning==1){
            accessMenu(client_socket); 
        }  
    }
    else if(strcmp(buff,"Autentificare esuata.")==0){
        printf("Utilizatorul sau parola sunt gresite.\n");
        close(client_socket);
    }

    return 0;
}