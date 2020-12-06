#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "lib.h"

// pour mémoriser et rappeler le dernier message
char lastmessage[80];

// fonction pour gérer l'affichage de lastmessage
void myprintf(char* msg) {
	strcpy(lastmessage, msg);
	printf("%s", lastmessage);
}
// remet à blanc le dernier message
void resetprintf() {
	memset( lastmessage, 0, sizeof(lastmessage));
}

// fonction de notification 
void notification(int my_socket) {
	char strnotif[LNG_NOTIF];

	int ret =0;
	printf("en attente de notif sur le socket %d\n",my_socket);
	while (1) {
		// je reçois l'état du système
		ret=recv(my_socket, strnotif, sizeof(strnotif), 0);
		if (ret < 1) {
			printf("error de receive de notif !!! %d\n", ret);
			perror("error notif");
			break;	
		}

		// affichage
		printf(strnotif);
		// rappel du dernier message
		printf("\n%s\n", lastmessage);
	}
}

int main(int argc, char** argv) {

	resetprintf();

	if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }
	if ( atoi(argv[2])<1 || atoi(argv[2]) > 65534) {
        fprintf(stderr, "port incorrect\n");
        exit(1);
	}
	int port1 = atoi(argv[2]);
	int port2 = atoi(argv[2])+1;

	printf("Préparation de la connexion à %s:%d,%d\n", argv[1], port1, port2);

	int clientSocket, clientSocketN, ret;
	struct sockaddr_in serverAddr;
	location buffer;
	int localisation;
	int mode;
	networkmsgloc messageloc;

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0) {
		perror("[-]Erreur de connexion.\n");
		exit(1);
	}
	printf("[+]Socket client créée.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port1);
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

	printf("[+]Tentative de connexion du client...\n");
	ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		perror("[-]Erreur de connexion.\n");
		exit(1);
	}
	printf("[+]Connecté au serveur.\n");

	clientSocketN = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocketN < 0) {
		perror("[-]Notif: Erreur de connexion.\n");
		exit(1);
	}
	printf("[+]Socket de notification du client créée.\n");

	printf("[+]Tentative de connexion du client au serveur de notifications...\n");
	serverAddr.sin_port = htons(port2);
	ret = connect(clientSocketN, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		perror("[-]Notif: Erreur de connexion.\n");
		exit(1);
	}
	printf("[+]Connecté au serveur de notifications.\n");

	// attente confirmation HANDSHAKE
	memset(message, 0, sizeof(message));
	if (recv(clientSocketN, message, sizeof(message), 0) < 1) {
		perror("receive confirmation from notif");
	}
	printf("[+]Confirmation de notification reçue: %s.\n", message);


	// MESSAGE D'ACCEUIL
	printf("\n**********************************************************\n");
	printf("Prêt à recevoir les demandes.\n");
	printf("Saisir des demandes positives pour prendre des ressources.\n");
	printf("Saisir des demandes négatives pour libérer des ressources.\n");
	printf("Les demandes nulles ne sont pas permises par le serveur.\n");
//TODO
	printf("NOTE:\n");
	printf(" -  pour l'instant le mode partagé ou dédié n'est pas géré.)\n");
	printf(" -  le nom de client est réservé à un usage future.\n");
//END TODO
	printf("**********************************************************\n");
	printf("\n");

	// creation du thread de notification
	pthread_t threadId;
	int err = pthread_create(&threadId, NULL, &notification, clientSocketN);
	if (err) {
		perror("pthread_create");
		exit(1);
	} 
	printf("thread de notification créé.\n");
	printf("\n");

	while (1) {
		printf("\n");
		myprintf("Client: ");
		scanf("%[^\n]s", buffer.nom);

		myprintf("Cpu: ");
		scanf("%d", &buffer.cpu);

		myprintf("Stockage: ");
		scanf("%d", &buffer.stockage);

		myprintf("Mode (0=SHR, 1=DED): ");
		scanf("%d", &mode);

		myprintf("Localisation (O=MPL,1=LYO,2=PAR): ");
		scanf("%d", &localisation);

		resetprintf();
		printf("Votre demande:\n");
		printf("* Nom: %s\n", buffer.nom);
		printf("* CPU: %d\n", buffer.cpu);
		printf("* Stockage: %d\n", buffer.stockage);
		printf("* mode (0=SHR, 1=DED):%d\n", mode);
		printf("* localisation (O=MPL,1=LYO,2=PAR):%d\n", localisation);

		// corriger scanf (vide le buffer d'entrée)
		int c; while ((c = getchar()) != '\n' && c != EOF) { }

		messageloc.mylocalisation = localisation;
		messageloc.mymode = mode;
		messageloc.mylocation.cpu = buffer.cpu;
		strcpy(messageloc.mylocation.nom , buffer.nom);
		messageloc.mylocation.stockage = buffer.stockage;
	
		// envoi de la demande
		if(send(clientSocket, &messageloc, sizeof(messageloc),0) < 0) {
			perror("send client");
			break ;
		}
		myprintf("Demande envoyée, en attente de confirmation du serveur...\n");

		// attente confirmation
		// ce qui permet d'être bloqué si les ressources sont manquantes
		memset(message, 0, sizeof(message));
		if (recv(clientSocket, message, sizeof(message), 0) < 1) {
			perror("receive confirmation");
			break;	
		}
		if ( strcmp(message, "OK") != 0 ) {
			myprintf("\n**** Une erreur dans votre demande !!! ****\n");
			resetprintf();
			printf("**** Erreur:%s\n\n", message);
		} else {
			myprintf("\nConfirmation reçue du serveur.\n");
			resetprintf();
		}
	}
 
	return 0;
}
