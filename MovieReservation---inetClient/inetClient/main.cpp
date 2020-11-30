//	**************************************************
//	*  pcdClient.c - inet pcd client implementation  *
//	**************************************************

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define MAX_MSG 512

#define MAXCHAR 128

using namespace std;

void paginaPrincipala(int clientFd, char * mesaj);
void selectMovie(int clientFd, char * mesaj);
void selectHour(int clientFd, char * mesaj);



int main(int argc, char * argv[])
{

    printf("\nINET - this is the regular pcd client program.\n");
    if (argc < 3)
    {
        printf("\nINET *** specify command line arguments, like - localhost 9091 Hello. so, try again.\n");
        exit(2);
    }

    int clientFd;
    int len, err=0, rc;
    struct sockaddr_in serverAddr;
    struct hostent * host;

    int result;
    char buf[1024];
    int numBytes;
    char alegere[128];
    char mesaj[128];
    char buffer[512];

    // create socket for client.
    clientFd = socket(AF_INET, SOCK_STREAM, 0);

    if (clientFd < 0)
    {
        printf("\nINET *** socket() failed, error = %d. will exit\n", err);
        exit(1);
    }

    if ((host=gethostbyname(argv[1]))==(struct hostent*)NULL)
    {
        printf("\nINET *** gethostbyname error. will exit.\n");
        exit(3);
    }

    // name the socket as agreed with server.
    serverAddr.sin_family = AF_INET;
    memcpy((char*)&serverAddr.sin_addr, (char*)host->h_addr, host->h_length);
    serverAddr.sin_port = htons((u_short)atoi(argv[2]));
    len = sizeof(serverAddr);

    // create the client socket
    result = connect(clientFd, (struct sockaddr *)&serverAddr, len);

    if(result < 0)
    {
        printf("\nINET *** connection error has occurred. will exit.\n");
        exit(0);
    }
    else
        printf("\nINET - connected to the server.\n");


    // afisare menu principal
    while(1)
    {

        printf("Meniu rezervare bilete la film!\n");
        printf("1 - Afiseaza filmele disponibile!\n");//afisare cand ma conectez
        printf("2 - Realizeaza o rezervare!\n"); //selecteaza film + rezervare
        printf("3 - Afiseaza orele disponibile!\n");
        printf("0 - Terminare program.\n");
        printf("Alegere:");
        scanf("%c", alegere);
        if ((alegere[0] < '0' || alegere[0] > '3'))
        {
            printf("\n Alegere incorecta. Incearca din nou\n\n");
        }

        // proceseaza o alegere corecta. in cazurile 1-3 se creeaza
        // un mesaj care e trimis serverului de administrare

        switch(alegere[0])
        {
        case '1':
            strcpy(mesaj, "1 ");
            break;
        case '2':
            strcpy(mesaj,"2");
            printf("\n Realizeaza o rezervare!");
            printf("\n Introducere film:");
            scanf("%s", alegere);
            if(strlen(alegere) == 1)
            {
                strcat(mesaj, "0");
                strcat(mesaj, alegere);
            }
            else
            {
                strcat(mesaj, alegere);
            }
            printf("\n Introducere ora: ");
            scanf("%s",alegere);
            if(strlen(alegere) == 1)
            {
                strcat(mesaj, alegere);
            }
            else
            {
                strcat(mesaj, "0");
            }

            printf("\nIntroducere nume: ");
            scanf("%s", alegere);
            strcat(mesaj, alegere);
            printf("Introducere prenume: ");
            scanf("%s", alegere);
            strcat(mesaj, alegere);
            //printf("%s", mesaj);
            break;
        case '3':
            strcpy(mesaj,"3");
            printf("\n Specifica numar film = ");
            scanf("%s",alegere);
            if(strlen(alegere) == 1)
            {
                strcat(mesaj, "0");
                strcat(mesaj, alegere);

            }
            else
            {
                strcat(mesaj, alegere);
            }
            break;
        case '0':
            printf("\n Terminare program\n");
            exit(0);
            break;
        default:
            break;
        }

        numBytes = send(clientFd, mesaj, strlen(mesaj), 0);
        if (numBytes <= 0)
        {
            perror("send() failed");
        }

        rc = recv(clientFd, buffer, MAX_MSG, 0);
        if (rc < 0)
        {
            perror("recv() failed..");
        }
        else if (rc == 0)
        {
            printf("Deconectat de la serverul UNIX.. \n");
        }
        else
        {
            // display the message received from the admin server
            buffer[rc] = 0;
            printf("\n Mesaj de la server UNIX: %s\n", buffer);
        }

        printf("\n Apasa o tasta pentru a continua \n");
        getchar();
        getchar();
    }

}
