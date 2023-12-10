#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>


#define SERVERPORT 12345
#define BUFFER_SIZE 1024
#define MAX_SIZE 1024
#define CMDLINE_MAX_SIZE 256
#define RECEIVED_DOCS_DIR "/home/bianca/PSO_Proiect/RecievedDocumentsServer/"

int sessionRunning=1;
char procesele_curente[1024][1024];
char pid_procese[1024][1024];
int nr_proc;

#ifndef DT_DIR
#define DT_DIR 4
#endif

struct Users{
    char* username;
    char* password;
};
struct Users utilizatori[MAX_SIZE]; //stocam toti utilizatorii permisi intr-o structura

//fisier log
void modifyLogger(const char* logFile,char* user,char* status)
{
    FILE* f=fopen(logFile,"a");
    if(f==NULL){
        perror("Eroare la deschiderea fisierului de log.");
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

//utilizatori
int numberOfUsers(const char* filename)
{
    int count=0;
    FILE* f=fopen(filename,"r");
    if(f==NULL){
        perror("Eroare la deschiderea fisierului cu utilizatori.");
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
        perror("Eroare la deschiderea fisierului cu utilizatori.");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_SIZE];
    int i=0;

    while(fgets(buffer,MAX_SIZE,f)!=NULL && i<nbAcc){
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
        perror("Eroare citire username.");
        exit(EXIT_FAILURE);
    }
    username[bytesReadUser]='\0';

    int bytesReadPasswd=read(client_socket,password,sizeof(password));
    if(bytesReadPasswd==-1){
        perror("Eroare citire parola.");
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

//executie comenzi
void getCommand(int client_socket)
{
    char buffer[9024];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            perror("Error receiving data");
            exit(EXIT_FAILURE);
        }
        buffer[bytes_received]='\0';

        //  char *command = strtok(buffer, " \n");
        // char *arguments = strtok(NULL, "\n");

        printf("Comanda primita: %s\n",buffer);

        // executarea comenzii folosind popen
        FILE *command_output = popen(buffer, "r");
        if (command_output == NULL) {
            perror("Error executing command");
           exit(EXIT_FAILURE);
        }

        // trimiterea rezultatului catre client
        char line_buffer[9024];
        memset(buffer,0,9024);
            if(fgets(line_buffer, sizeof(line_buffer), command_output) != NULL)
            {
                while(fgets(line_buffer, sizeof(line_buffer), command_output) != NULL)
                {
                    strcat(buffer, line_buffer);
                }
                printf("buffer: %s\n",buffer);
                send(client_socket, buffer, strlen(buffer), 0);
            }
            else 
            {
            char* msg="Comanda executata cu succes.";
                send(client_socket,msg, strlen(msg), 0);
            }
        
        
        pclose(command_output);
}

//informatii sistem
char* checkEndian()
{
    unsigned int num=1;
    char* ptr=(char*)&num;
    char little[]="Little Endian";
    char big[]="Big Endian";
    char* s;

    if(*ptr==1){
        s=(char*)malloc(sizeof(char)*strlen(little));
        strcpy(s,little);

        return s;
    }
    if(*ptr==0){
        s=(char*)malloc(sizeof(char)*strlen(big));
        strcpy(s,big);
        
        return s;
    }

    return NULL;
}

void CPUInfo(FILE* f)
{
    char* byteOrder=checkEndian();
    if(byteOrder!=NULL){
        fprintf(f,"Byte order: %s\n",byteOrder);
    }

    FILE* fcpu=fopen("/proc/cpuinfo","r");
    if(fcpu==NULL){
        perror("Eroare la deschidere /proc/cpu.");
        exit(-1);
    }

    char line[MAX_SIZE];
    int i=0;
    while(fgets(line,sizeof(line),fcpu)!=NULL){
        if(i==1 || i==4 || i==8 || i==12 ){
            fprintf(f,"%s",line);
        }
        i++;
    }
    fprintf(f,"\n");

    fclose(fcpu);
}

void RAMInfo(FILE* f)
{
    struct sysinfo mem_info;
    if (sysinfo(&mem_info) != 0) {
        perror("Eroare la obținerea informațiilor despre memorie");
        exit(-1);
    }

    fprintf(f,"--Memory--");
    fprintf(f, "\nTotal RAM: %lu MB\n", mem_info.totalram / 1024 / 1024);
    fprintf(f, "Free RAM: %lu MB\n", mem_info.freeram / 1024 / 1024);
    fprintf(f, "Used RAM: %lu MB\n\n", (mem_info.totalram - mem_info.freeram) / 1024 / 1024);
}

void BIOSInfo(FILE* f)
{
    FILE *bios_vendor_file = fopen("/sys/class/dmi/id/bios_vendor", "r");
    FILE *bios_version_file = fopen("/sys/class/dmi/id/bios_version", "r");
    FILE *bios_date_file = fopen("/sys/class/dmi/id/bios_date", "r");

    if (bios_vendor_file == NULL || bios_version_file == NULL || bios_date_file == NULL) {
        perror("Eroare la deschiderea fișierelor de BIOS");
        return;
    }

    char vendor[100], version[100], date[100];

    fgets(vendor, sizeof(vendor), bios_vendor_file);
    fgets(version, sizeof(version), bios_version_file);
    fgets(date, sizeof(date), bios_date_file);

    fprintf(f,"--BIOS--\n");
    fprintf(f,"Vendor: %s", vendor);
    fprintf(f,"Version: %s", version);
    fprintf(f,"Realease date: %s", date);

    fclose(bios_vendor_file);
    fclose(bios_version_file);
    fclose(bios_date_file);
}

void getSysInfo(int client_socket)
{
    FILE *file_to_send = fopen("/home/bianca/PSO_Proiect/RecievedDocumentsServer/sistem.txt", "rb");

    if (file_to_send == NULL) {
    perror("Eroare la deschiderea fisierului de trimis.");
    exit(EXIT_FAILURE);
    }
    int len=0;
    char buffer[BUFFER_SIZE];
    fseek(file_to_send,0,SEEK_END);
    int fs=ftell(file_to_send);
    fseek(file_to_send,0,SEEK_SET);
    char fileSize[256];
    memset(fileSize,0,sizeof(fileSize));
    sprintf(fileSize,"%d",fs);
    printf("%s\n",fileSize);
    len=strlen(fileSize);
    send(client_socket,&len,sizeof(int),0);
    send(client_socket,fileSize,len,0);

    int sentBytes=0;
    int remainData=fs;
    memset(buffer,0,sizeof(buffer));
    while(((sentBytes=fread(buffer,1,BUFFER_SIZE,file_to_send))>0)&&(remainData>0)){
        //printf("%s",buffer);
        //l=strlen(buffer);
        //printf("%d\n",l);
        send(client_socket,buffer,strlen(buffer),0);
        remainData-=sentBytes;
        //printf("%d\n",remainData);
    }
    fclose(file_to_send);
    printf("Datele au fost trimise.\n");
}

//procese
void get_procese()
{
     DIR *dir;
    struct dirent *entry;

    // Deschide directorul /proc
    dir = opendir("/proc");

    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }


   // Parcurge directorul
   long pid;
   int j=0;
   int counter_procese=0;
   char PID[100];
    while ((entry = readdir(dir)) != NULL) {

          int valid_pid = 1;
        for (int i = 0; entry->d_name[i] != '\0'; i++) {
            if (!isdigit(entry->d_name[i])) {
                valid_pid = 0;
                break;
            }
        }
        if(valid_pid)
        {
            counter_procese++;
            char filename[1024];
            sprintf(filename, "/proc/%s/comm", entry->d_name);

            int fd_cmdline_file = open(filename, O_RDONLY);

            if (fd_cmdline_file ==-1)
            {
                perror("Eroare la deschiderea fisierului");
                exit(EXIT_FAILURE);
            } 
            else{
                char cmdline[256];
                //fgets(cmdline, strlen(cmdline), cmdline_file);

                char buffer_citit[1024];
                int bytes=read(fd_cmdline_file,buffer_citit,sizeof(buffer_citit)-1);
                
                buffer_citit[bytes]='\0';
                if(bytes==-1)
                {
                    perror("Eroare la citirea din fisier");
                    exit(EXIT_FAILURE);
                }
                // procesele_curente[j]=(char*)malloc(strlen(buffer_citit)*sizeof(char));
                strcpy(pid_procese[j],entry->d_name);
                printf("PID: %s\n",pid_procese[j]);
                strcpy(procesele_curente[j],buffer_citit);
                printf("PROCESUL CITIT: %s",procesele_curente[j]);
                j++;
                
                close(fd_cmdline_file);
                }
        }            
    }
      int i=0;

    nr_proc=counter_procese;
    char counter_str[3];
    sprintf(counter_str,"%d",counter_procese);
    
    printf("Numar procese: %s\n",counter_str);

}

void listeaza_procese(int client_socket)
{
    char mesaj_client[100];
    recv(client_socket,mesaj_client,sizeof(mesaj_client),0);

    //iau procesele curente din server
    get_procese();

    char nr_proc_str[10];
    sprintf(nr_proc_str,"%d",nr_proc);

    send(client_socket,nr_proc_str,strlen(nr_proc_str),0);
    //printf("MESAJUL: %s\n",procesele_curente);
    if(strcmp(mesaj_client,"listeazaprocese")==0)
    {
        int i=0;
        char* nume=strtok(procesele_curente[i],"\n");
        char* pid_str=strtok(pid_procese[i],"\n");
        i++;
       while(nume)
       {
        printf("NUME PROCES: %s CU PID %s\n",nume,pid_str);
        send(client_socket,nume,strlen(nume),0);
        send(client_socket,pid_str,strlen(pid_str),0);
        nume=strtok(procesele_curente[i],"\n");
        pid_str=strtok(pid_procese[i],"\n");
        i++;
       }
    }
    memset(procesele_curente,0,sizeof(procesele_curente));
}

void opreste_proces(int client_socket)
{
    char pid_str[5];
    recv(client_socket,pid_str,5,0);  // primeste numele prosului pe care vrea sa-l opreasca

    get_procese();

    printf("\n");
    printf("\n");

    char*pid_mod=strtok(pid_str,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n");
    printf("PROCESUL DAT LA TASTATURA: %s\n",pid_mod);

        for(int i=0;i<nr_proc;i++)
        {
            char*pid_proc_salvat=strtok(pid_procese[i],"\n");
           // printf("PID VERIFICAT: %s\n",pid_proc_salvat);
            if(strcmp(pid_mod,pid_proc_salvat)==0)
            {
                printf("PROCESUL PE CARE VREAU SA-L OPRESC: %s\n",pid_proc_salvat);
                int pid=atoi(pid_proc_salvat);
                if(kill(pid,SIGTERM)==0)
                {
                    printf("Procesul cu pid-ul %d a fost oprit cu succes.\n",pid);
                    char msj[100]="Procesul cu pid-ul ";
                    strcat(msj,pid_proc_salvat);
                    strcat(msj," a fost oprit cu succes.");
                    send(client_socket,msj,strlen(msj),0);
                }
                else
                {
                    perror("Eroare la oprirea procesului");
                    exit(EXIT_FAILURE);
                }
                
            }
        }

}

void makeSysFile()
{
    FILE* f=fopen("/home/bianca/PSO_Proiect/RecievedDocumentsServer/sistem.txt","w");
    if(f==NULL){
        perror("Eroare la deschiderea fisierului de sistem.");
        exit(-1);
    }

    struct utsname system_info;

    if (uname(&system_info) != 0) {
        perror("Eroare la obținerea informațiilor despre sistem");
        exit(-1);
    }

    fprintf(f,"\n\n----INFORMATII DESPRE SISTEM----\n");
    fprintf(f,"Sys name: %s\n", system_info.sysname);
    fprintf(f,"Node name: %s\n", system_info.nodename);
    fprintf(f,"Version: %s\n", system_info.version);
    fprintf(f,"Realease: %s\n\n",system_info.release);

    fprintf(f,"--CPU--\n");
    fprintf(f,"Arhitecture: %s\n", system_info.machine);
    CPUInfo(f);
    RAMInfo(f);
    BIOSInfo(f);

    fclose(f);
}

void recieveFileFromCLient(int client_socket) {
    
    char buffer[MAX_SIZE];
    char filename[MAX_SIZE];
    int len=0;

    recv(client_socket,&len,sizeof(int),0);
    recv(client_socket,filename,len,0);
    filename[strcspn(filename, "\n")] = '\0';
    //printf("%s\n\n",filename);
    len=0;
    recv(client_socket,&len,sizeof(int),0);
    recv(client_socket,buffer,len,0);
    buffer[strcspn(buffer,"\n")]='\0';
    int fileSize=atoi(buffer);
    //printf("%d\n",fileSize);

    char* newPath=(char*)malloc(sizeof(char)*(strlen(RECEIVED_DOCS_DIR)+strlen(filename)));
    strcpy(newPath,RECEIVED_DOCS_DIR);
    strcat(newPath, filename);
    //printf("%s\n",newPath);
    
    FILE *received_file = fopen(newPath, "wb");

    if (received_file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int remainData=fileSize;
    //printf("\n%d\n",remainData);
    int bytesReceived=0;
    memset(buffer,0,sizeof(buffer));
    while((remainData>0) && ((bytesReceived=recv(client_socket,buffer,MAX_SIZE,0))>0))
    {
        //printf("%s",buffer);
        fwrite(buffer,sizeof(char),bytesReceived,received_file);
        remainData-=bytesReceived;
    }

    fclose(received_file);
    printf("Primit.\n");
    free(newPath);
}

//comanda personalizata
void comanda_personalizata(int client_socket)
{
    char comanda[1024];
    char user[100];
    recv(client_socket,comanda,1024,0);

    char* comm=strtok(comanda,"\n");

    printf("COMANDA PERSONALIZATA: %s\n",comm);

    FILE* output;
    char continut[4096];

    if(strcmp(comm,"afiseaza utilizatori")==0)  //utilizatorii din sistem
    {
        output=popen("cat /etc/passwd | cut -d':' -f1","r");
        if(output==NULL)
        {
            perror("Eroare la output");
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp(comm,"afiseaza grupurile")==0) //grupurile din sistem
    {
        output=popen("cat /etc/group | cut -d':' -f1","r");
        if(output==NULL)
        {
            perror("Eroare la output");
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp(comm,"afiseaza utilizatorul curent")==0) //utilizatorul curent
    {
        output=popen("whoami","r");
        if(output==NULL)
        {
            perror("Eroare la output");
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp(comm,"afiseaza numarul utilizatorilor")==0) //numarul de utilizatori
    {
        output=popen("cat /etc/passwd | wc -l","r");
        if(output==NULL)
        {
            perror("Eroare la output");
            exit(EXIT_FAILURE);
        }
    }
    else if(strstr(comm,"cautare utilizator")!=NULL) //cautare user
    {
        // extrag user-ul din comanda personalizata
        char prop[100];
        strcpy(prop,"echo ");
        strcat(prop,comm);
        strcat(prop," | cut -d' ' -f3");
        FILE* obtine_user=popen(prop,"r");
        char user[100];
        fgets(user,sizeof(user),obtine_user);

        char*usr=strtok(user,"\n");

        printf("USER: %s\n",usr);

        //aici formez comanda pt linux
        char command[100];
        strcpy(command,"egrep ");
        strcat(command,usr);
        strcat(command," /etc/passwd | cut -d':' -f1");
        output=popen(command,"r");
        if(output==NULL)
        {
            perror("Eroare la output");
            exit(EXIT_FAILURE);
        }
        strcpy(continut,"Utilizatorul a fost gasit!\n");
        memset(user,0,sizeof(user));
    }
     else if(strstr(comm,"afisare informatii")!=NULL) //detalii user
    {
         char prop[100];
        strcpy(prop,"echo ");
        strcat(prop,comm);
        strcat(prop," | cut -d' ' -f3");
        FILE* obtine_user=popen(prop,"r");
        char user[100];
        fgets(user,sizeof(user),obtine_user);
        char* usr=strtok(user,"\n");

        char command[100];
        strcpy(command,"id ");
        strcat(command,usr);
        strcat(command," ");
        output=popen(command,"r");
        if(output==NULL)
        {
            perror("Eroare la output");
            exit(EXIT_FAILURE);
        }
        memset(user,0,sizeof(user));
    }

    memset(comm,0,sizeof(comm));

    
    char line[1024];
    
    while(fgets(line,sizeof(line),output)!=NULL)
    {
        if(continut[0]=='\0')
        {
            strcpy(continut,line);
        }
        else
        {
            strcat(continut,line);
        }
    }
    if(fgets(line,sizeof(line),output)==NULL)
    {
        strcat(continut,"Comanda executata cu succes.");
    }

    send(client_socket,continut,strlen(continut),0);

    memset(continut,0,sizeof(continut));
    memset(line,0,sizeof(line));


    pclose(output);
}

void accessMenu(int server_socket,int client_socket)
{
    char msj[20];
    char optiune_proc[2];

    recv(client_socket,msj,20,0);
    printf("S-A PRIMIT CODUL: %s\n",msj);

    if(strstr(msj,"delogare")!=NULL)
    {   
        sessionRunning=0;
        close(server_socket);
        close(client_socket);
        exit(EXIT_SUCCESS);
    }

    if(strstr(msj,"infosistem")!=NULL)
    {
        getSysInfo(client_socket);
        memset(msj,0,sizeof(msj));
    }
    else if(strcmp(msj,"comanda")==0)
    {
        //primire comanda
        getCommand(client_socket);
        memset(msj,0,sizeof(msj));
    }
    else if(strstr(msj,"transfer")!=NULL)
    {
        recieveFileFromCLient(client_socket);
        memset(msj,0,sizeof(msj));
    }
    else if(strstr(msj,"procese")!=NULL)
   {
        while(1)
            {
                recv(client_socket,optiune_proc,1,0);
                printf("OPTIUNE PRIMITA: %s\n",optiune_proc);
                if(strcmp(optiune_proc,"1")==0)
                {
                    //pentru listare procese
                    listeaza_procese(client_socket);
                }
                else if(strcmp(optiune_proc,"2")==0)
                {
                    //oprire proces
                    opreste_proces(client_socket);
                }
                else if(strcmp(optiune_proc,"3")==0)
                {
                    break;
                }
            }
            memset(msj,0,sizeof(msj));
    }
    else if(strstr(msj,"comandapers")!=NULL)
    {
        //comanda personalizata
        comanda_personalizata(client_socket);
        memset(msj,0,sizeof(msj));
    }
    else{
        printf("NU EXISTA CODUL.\n");
        memset(msj,0,sizeof(msj));
        //exit(EXIT_FAILURE);
    }

}



int main() {
    //Gestiunea utilizatorilor
    int nbUsers=numberOfUsers("utilizatori.txt");
    addAllAccounts("utilizatori.txt",nbUsers);
    
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;

    // Crearea socket-ului
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    // Setarea atributele serverului
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVERPORT);
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

    printf("Serverul ascultă pe portul %d...\n", SERVERPORT);

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
        makeSysFile();
        while(sessionRunning==1){
            accessMenu(server_socket,client_socket);
        }
    }
    else{       
        modifyLogger("logFile.txt","-","FAILED");
        char* msg="Autentificare esuata.";
        send(client_socket,msg,strlen(msg),0);
        close(server_socket);
    }

    return 0;
}