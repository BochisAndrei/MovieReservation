#include <iostream>
#include <sys/un.h>
#include <sys/socket.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>


//Fisierul prin care serverul si clientul de administrare vor comunica:
#define UNIX_FILE "/tmp/unixFile"

//Clientii normali se pot conecta la server pe portul 4444:
#define INET_PORT 4444

//Clientii soap se pot conecta la server la portul 5555:
#define SOAP_PORT 5555

//numar maxim de filme intr-o lista(filme+zile+ore)
#define MAXMOVIES 256

//numarul maxim de ip-uri blocate
#define MAXBLOCKED 128
//numar maxim de clienti
#define NUM_MAX_CLIENTS 256

#define MAXCHAR 256

typedef struct
{
    int fd; // identificator de socket din interiorul programului
    char IP[16];
    __uint16_t port;
} inetClient;

inetClient clientList[512];
int maxClient; //ultimul index din lista clientilor

//fisierul cu filme
FILE * fileMovies;
char movieList[100][MAXMOVIES];
int nr_movies = 0;
typedef struct movie
{
    char title[30];
    char firstHour[6];
    int nr_loc_first;
    char secondHour[6];//de pus un 0 la final pentru terminator de sir
    int nr_loc_second;
    char lastHour[6];
    int nr_loc_last;
};
struct movie movieArray[20];
void initMovieList();
void initMovieArray();

//fisierul cu ip-uri blocate:
FILE* fileBlocked;
char blockedIps[NUM_MAX_CLIENTS][16];  //lista de clienti blocati
int numBlocked = 0;

//fisierul cu rezervari
typedef struct rezervari
{
    char name[30];
    int movie;
    int hour;
};
int nr_rezervari=0;
struct rezervari rezervariArray[20];
void initRezervationArray();

//functii:
int processUnixClientRequest(int bytes, char client[512], char mesaj[512]);
int processClientRequest(int cliFd, char mesaj[512], char * msg);
void validateMovie(char user[512], int movie, int hour, char * temp);
void saveMovie(char movie[128], char hour1[128], char hour2[128], char hour3[128], char * msg);

void *unix_main(void *args)
{

    printf("[+]UNIX server is running\n");

    int serverFd = -1, clientFd = -2;
    int error, nBytes, numSent;
    char cmdClient[512]; // comanda de la client
    char mesaj[512]; // mesaj catre client
    struct sockaddr_un serverAddr;

    // creeaza socket cu protocolul TCP
    serverFd = socket(AF_UNIX, SOCK_STREAM, 0);

    if(serverFd < 0)
    {
        perror("UNIX socket() error..");
    }

    memset(&serverAddr, 0, sizeof(serverAddr)); //initialize serverAddr
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, UNIX_FILE);

    //cu bind facem legatura dintre serverAddr si socket(serverFd)
    error = bind(serverFd, (struct sockaddr *) &serverAddr, SUN_LEN(&serverAddr));

    if(error < 0)
    {
        perror("UNIX bind() error..");
    }

    error = listen(serverFd, 10); // convertim socket-ul in modul ascultare, pot sa aibe maximum 10 conexiuni

    if(error < 0)
    {
        perror("UNIX listen() error..");
    }
    int tries = 10;
    while(tries)
    {
        //ascultam cererile de conectare doar daca clientFd < 0
        if(clientFd < 0)
        {
            if((clientFd = accept(serverFd, NULL, NULL)) == -1)  // accept returneaza urmatoarea conexiune existenta
            {
                perror("UNIX accept() error..\n");
                break;
            }
        }
        else   //primeste comenzi de la administrator
        {
            while((nBytes = read(clientFd, cmdClient, sizeof(cmdClient))) > 0)  // citeste mesajul de la client si il pune in buffer-ul cmd client
            {
                cmdClient[nBytes] = 0;
                printf("UNIX = read %d bytes: %s\n", nBytes, cmdClient);  // buffer-ul cmdClient continte acum mesajul de la admin

                int len = processUnixClientRequest(nBytes, cmdClient, mesaj);

                numSent = send(clientFd, mesaj, len, 0); // trimite mesajul inapoi la client
                if( numSent < 0)
                {
                    perror("UNIX send() error..");
                    break;
                }
            }
            if(nBytes < 0)
            {
                perror("UNIX client read error, the connection will be closed.\n");
                close(clientFd);
                clientFd = -2;
                continue;
            }
            else if( nBytes == 0)
            {
                printf("UNIX ** The client closed the connection.\n");
                close(clientFd);
                clientFd = -2;
                continue;
            }
        }
        --tries;
    }

    if(serverFd > 0)
        close(serverFd);
    if(clientFd > 0)
        close(clientFd);

    unlink(UNIX_FILE); //fisierul de comunicare este inchis
}
//------------------------------------------------------------------
// functie procesare comanda client administrare UNIX

int processUnixClientRequest(int numChars, char * buffer, char * msg)
{
    int cmdType = buffer[0];
    int numConnected = 0;
    printf("\n UNIX = comanda primita = %s, type: %c.\n", buffer, cmdType);
    bzero(msg, strlen(msg));  //msg este umplut cu strlen(msg) de zero
    char temp[8], movie[128], ora1[128], ora2[128], ora3[128];
    int i, theFd, found;

    switch(cmdType)
    {
    case '1': //lista fd-uri clientilor
        strcpy(msg,"1");//seteaza tipul mesajului
        for (i = 0; i < maxClient; i++)
        {
            if(clientList[i].fd > 0)
            {
                strcat(msg, ":");
                snprintf(temp, 3, "%d", clientList[i].fd);
                strcat(msg, temp);
                numConnected++;
            }
        }
        if(numConnected == 0)
            strcat(msg, " - No INET client is connected.");
        break;
    case '2': //afisare informatii despre client
        strcpy(msg,"2");
        //gaseste clientul cu fd-ul cerut, afiseaza info
        theFd = atoi(buffer + 2);
        for(i=0; i<maxClient; i++)
        {
            if(clientList[i].fd == theFd)
            {
                strcat(msg, clientList[i].IP);
                strcat(msg, ":");
                snprintf(temp, 5, "%d", clientList[i].port);
                strcat(msg, temp);
                break;
            }
        }
        break;
    case '3': //deconectare client
        strcpy(msg, "3");
        theFd = atoi(buffer + 2);
        found = 0;
        for (i = 0; i<maxClient ; i++)
        {
            if (clientList[i].fd == theFd)
            {
                close(clientList[i].fd);
                found = 1;
                break;
            }
        }
        if(found)
            strcat(msg, ": client disconnected.");
        else
            strcat(msg, ": client not found.");
        break;
    case '4':
        strcpy(msg, "4");

        sprintf(ora1, "%c%c%c%c%c", buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
        sprintf(ora2, "%c%c%c%c%c", buffer[6], buffer[7], buffer[8], buffer[9], buffer[10]);
        sprintf(ora3, "%c%c%c%c%c", buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);
        strncpy(movie, buffer+16, strlen(buffer));
        printf("Ore: %s %s %s %s ", ora1, ora2, ora3, movie);
        saveMovie(movie, ora1, ora2, ora3, msg);
        break;
    default:
        //afisare mesaj eroare
        printf("Command is invalid, it will be ignored..\n");
        break;
    }
    printf("\nMessage for UNIX client: %s", msg);
    return strlen(msg);

    return 0;
}


void *inet_main(void *args)
{

    int serverFd, clientFd, error, i, maxFd, numReady, numBytes, numSent;
    fd_set readSet;

    //declaram doua structuri pentru server si client pentru a salva datele celor doi
    struct sockaddr_in
        serverAddr,
        clientAddr;

    socklen_t addr_size;

    char mesaj[512]; // mesaj catre client
    char clientMsg[512]; // comanda de la client

    pid_t childpid;

    //creeaza socket-ul pentru server
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverFd < 0)
    {
        printf("[-]Error in connection..\n");
        exit(1);
    }
    printf("[+]Server Socket is created.\n");

    //configura serverul pentru clienti
    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(INET_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    error = bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if(error < 0)
    {
        printf("[-]Error in binding.\n");
        exit(1);
    }
    else
    {
        printf("[+]Bind to port %d\n", INET_PORT);
    }

    ///server ul trece in mod ascultare
    if(listen(serverFd, 10) == 0)
    {
        printf("[+]Listening....\n");
    }
    else
    {
        printf("[-]Error in binding.\n");
    }

    //initializeaza lista de clienti
    for(i = 0; i<maxClient; i++)
    {
        clientList[i].fd = 0;
    }



    //deschide fisierul de ip-uri blocate pentru a citi si a adauga

    char* blockedSpec = "../theServer/blockedIPs.txt";
    fileBlocked = fopen(blockedSpec,"a+");
    if(!fileBlocked)
    {
        printf("INET * could not open the blocked ip's file\n");
    }
    else
    {
        printf("INET - blocked ip's:\n\n");
        while (fgets(blockedIps[numBlocked], MAXBLOCKED, fileBlocked))
        {
            printf("%s", blockedIps[numBlocked]);
            numBlocked++;
        }
    }

    FD_ZERO(&readSet); // seteaza bit-ul pe 0
    FD_SET(serverFd, &readSet); //seteaza serverFd cu readSet care are bitii pe 0(initializare)
    maxFd = serverFd + 1;
    addr_size = sizeof(clientAddr);


    while(1)
    {

        printf("INET - ready for ordinary clients to connect().\n");
        FD_SET(serverFd, &readSet); //probabil nu este necesar dar fara da eroare
        numReady = select(maxFd + 1, &readSet, NULL, NULL, NULL);

        if(FD_ISSET(serverFd, &readSet))  //conexiune client
        {
            clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addr_size);
            char * clientIP = inet_ntoa(clientAddr.sin_addr);
            printf("INET - accepted connection. fd = %d, IP = %s, will check if it is blocked.\n", clientFd, clientIP);

            int isBlocked = 0;

            //parseaza lista de ip-uri blocate si verifica daca e blocat
            for(i = 0; i<NUM_MAX_CLIENTS; i++)
            {
                if(!strcmp(clientIP, blockedIps[i]))
                {
                    isBlocked = 1;
                    close(clientFd);
                    printf("INET - this IP: %s is blocked. connection request denied.\n", clientIP);
                    break;
                }
            }

            if(!isBlocked)  // adauga la lista clientilor
            {
                printf("INET - the IP: %s is not blocked.\n", clientIP);

                //adauga la lista clientilor
                for(i = 0; i< NUM_MAX_CLIENTS; i++)
                {
                    if(clientList[i].fd == 0)
                    {
                        clientList[i].fd = clientFd; //salveaza fd
                        strcpy(clientList[i].IP, clientIP);
                        clientList[i].port = clientAddr.sin_port;
                        if(i>=maxClient)
                            maxClient = i+1;
                        break;
                    }
                }

                if(i == NUM_MAX_CLIENTS)
                {
                    printf("INET - * too many clients, will exit..\n");
                    exit(1);
                }

                //pune noul clientfd in lista de readSet
                FD_SET(clientFd, &readSet);

                if(clientFd > maxFd)
                    maxFd = clientFd;

            }
            if(--numReady <=0)
                continue; //intoarce te la select

        }

        //daca client-ul s-a conectat atunci realizam rezervarea
        for(i = 0; i<NUM_MAX_CLIENTS; i++)
        {
            if(clientList[i].fd <=0)
                continue;

            if(FD_ISSET(clientList[i].fd, &readSet))
            {
                if((numBytes = recv(clientList[i].fd, clientMsg, MAXCHAR,0)) <= 0)
                {
                    FD_CLR(clientList[i].fd, &readSet);
                    close(clientList[i].fd);
                    printf("INET ** conection with client at IP: %s, fd = %d - closed.\n", clientList[i].IP, clientList[i].fd);
                    clientList[i].fd = 0;
                }
                else
                {
                    printf("INET - received message: %s from client with IP: %s, fd = %d.\n", clientMsg, clientList[i].IP, clientList[i].fd);
                    int len = processClientRequest(clientList[i].fd, clientMsg, mesaj);

                    numSent = send(clientList[i].fd, mesaj, len, 0); // trimite mesajul inapoi la client
                    if( numSent < 0)
                    {
                        perror("UNIX send() error..");
                        break;
                    }
                }
            }

        }

    }
}

int processClientRequest(int cliFd, char * buffer, char * msg)
{

    initMovieArray();
    initRezervationArray();
    int cmdType = buffer[0], movieChoice, hourChoice;
    int numConnected = 0;
    char temp[512], user[512];

    printf("\n UNIX = comanda primita = %s, type: %c.\n", buffer, cmdType);
    bzero(msg, strlen(msg));  //msg este umplut cu strlen(msg) de zero
    strcat(msg, "\n");

    switch(cmdType)
    {
    case '1':
        for(int i=0; i< nr_movies; i++)
        {
            sprintf(temp, "%d - %s\n", i+1, movieArray[i].title);
            strcat(msg, temp);
        }
        break;
    case '2':
        strncpy(user,buffer+4,strlen(buffer)-4);
        sprintf(temp,"%c",buffer[3]);
        hourChoice = atoi(temp);
        sprintf(temp, "%c%c",buffer[1], buffer[2]);
        movieChoice = atoi(temp);
        if(movieChoice>0 && movieChoice<=nr_movies)
        {
            validateMovie(user, movieChoice, hourChoice, temp);
            strcpy(msg,temp);
        }
        else
        {
            strcpy(msg, "\nFilmul nu este disponibil, selecteaza un film disponibil!");
        }

        break;
    case '3':
        sprintf(temp, "%c%c", buffer[1],buffer[2]);
        movieChoice = atoi(temp);
        for(int i=0; i< nr_movies; i++)
        {
            if(movieChoice==i+1)
            {
                sprintf(temp, "\nFilm: %s\n 1.%s\n 2.%s\n 3.%s\n", movieArray[i].title, movieArray[i].firstHour, movieArray[i].secondHour, movieArray[i].lastHour);
                strcat(msg, temp);
            }
        }
        break;
    case '5':
        strcat(msg,(char*)(sizeof(movieArray) / sizeof(movieArray[0])));
        break;
    default:
        //afisare mesaj eroare
        printf("Command is invalid, it will be ignored..\n");
        break;
    }

    return strlen(msg);

}

void initRezervationArray()
{
    FILE *fptr;
    int ch, nrLinii = 0;
    if ((fptr = fopen("../theServer/rezervare.txt","r")) == NULL)
    {
        printf("Eroare la file");
        return;
    }
    do
    {
        ch = fgetc(fptr);
        if(ch == '\n')
            nrLinii++;
    }
    while (ch != EOF);
    nr_rezervari = nrLinii;

    fclose(fptr);

    if ((fptr = fopen("../theServer/rezervare.txt","r")) == NULL)
    {
        printf("Eroare la file");
        return;
    }

    for(int i = 0; i <= nrLinii; i++)
    {
        fscanf(fptr,"%s %d %d", rezervariArray[i].name, &rezervariArray[i].movie, &rezervariArray[i].hour);
    }

    fclose(fptr);
}

void initMovieArray()
{
    FILE *fptr;
    int ch, nrLinii = 0;

    if ((fptr = fopen("../theServer/filme.txt","r")) == NULL)
    {
        printf("Eroare la file");
        return;
    }
    do
    {
        ch = fgetc(fptr);
        if(ch == '\n')
            nrLinii++;
    }
    while (ch != EOF);
    nr_movies = nrLinii;
    printf("%d", nr_movies);

    fclose(fptr);

    if ((fptr = fopen("../theServer/filme.txt","r")) == NULL)
    {
        printf("Eroare la file");
        return;
    }

    for(int i = 0; i <= nrLinii; i++)
    {
        fscanf(fptr,"%s %s %d %s %d %s %d", movieArray[i].title, movieArray[i].firstHour, &movieArray[i].nr_loc_first, movieArray[i].secondHour, &movieArray[i].nr_loc_second, movieArray[i].lastHour, &movieArray[i].nr_loc_last);
    }
    fclose(fptr);
}

void initMovieList()
{
    //deschidere fisier filme
    char* filename = "../theServer/filme.txt";
    char str[MAXMOVIES];
    fileMovies = fopen(filename, "r");
    if (fileMovies == NULL)
    {
        printf("Could not open file %s",filename);
    }

    int i=0;
    while (fgets(str, MAXMOVIES-1, fileMovies) != NULL)
    {
        strcpy(movieList[i],str);
        i++;
    }
    nr_movies = i;

    fclose(fileMovies);
}

void validateMovie(char user[512], int movie, int hour, char * msg)
{

    int locuri, acc=0, cnt=0;
    FILE *fptr;
    char temp[512];
    for(int i=0; i<nr_rezervari; i++)
    {
        if(rezervariArray[i].movie == movie && rezervariArray[i].hour==hour)
            cnt++;
    }

    for(int i=0; i< nr_movies; i++)
    {
        if(movie==i+1)
        {
            if(hour==1)
            {
                locuri =  movieArray[i].nr_loc_first;
                if(locuri > cnt)
                {
                    strcpy(msg, "\n----Rezervare reusita!\n");
                    acc=1;
                }
                else
                {
                    strcpy(msg,"\nRezervarea a fost respinsa!");
                }

            }
            else if(hour==2)
            {
                locuri =  movieArray[i].nr_loc_second;
                if(locuri > cnt)
                {
                    strcpy(msg, "\n----Rezervare reusita!\n");
                    acc=1;
                }
                else
                {
                    strcpy(msg,"\nRezervarea a fost respinsa!");
                }
            }
            else if(hour==3)
            {
                locuri =  movieArray[i].nr_loc_last;
                if(locuri > cnt)
                {
                    strcpy(msg, "\n----Rezervare reusita!\n");
                    acc=1;
                }
                else
                {
                    strcpy(msg,"\nRezervarea a fost respinsa!");
                }
            }
            else
            {
                strcpy(msg, "\nOra indisponibila, rezervare respinsa!\n");
            }

        }
    }

    if(acc==1)
    {
        fptr = fopen("../theServer/rezervare.txt", "a+");

        if(fptr !=NULL)
        {
            sprintf(temp, "%s %d %d\n",user, movie, hour);
            fprintf(fptr,temp);
            fclose(fptr);
        }
        else
        {
            printf("Could not open the file!\n");
        }
    }

}
void saveMovie(char movie[128], char hour1[128], char hour2[128], char hour3[128], char * msg){

    int acc=1;
    FILE *fptr;
    char temp[512];
    if(acc==1)
    {
        fptr = fopen("../theServer/filme.txt", "a+");

        if(fptr !=NULL)
        {
            sprintf(temp, "%s %s %d %s %d %s %d\n",movie, hour1, 20, hour2, 20, hour3, 20);
            fprintf(fptr,temp);
            fclose(fptr);
        }
        else
        {
            printf("Could not open the file!\n");
        }
    }
}

int main()
{
    int inetPort, soapPort;
    pthread_t
    unixThread,  // UNIX Thread: componenta UNIX
    inetThread,  // INET Thread: componenta INET
    soapThread;  // SOAP Thread: componenta SOAP

    unlink(UNIX_FILE);
    pthread_create(&unixThread, NULL, unix_main, nullptr);

    inetPort = INET_PORT; //portul utilizat pentru inet
    pthread_create(&inetThread, NULL, inet_main, &inetPort);

    soapPort = SOAP_PORT ; //portul utilizat pentru soap
//	pthread_create (&soapThread, NULL, soap_main, &soapPort);

    pthread_join(unixThread, NULL);
    pthread_join(inetThread, NULL);
//  pthread_join(soapThread, NULL);
    unlink(UNIX_FILE);
    return 0;
}
