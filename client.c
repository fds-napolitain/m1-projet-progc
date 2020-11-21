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

// fonction de notification
void notification(int my_socket) {
	char mon_buffer[128];
	while (1) {
		// je reçois l'état du système
		if (recv(my_socket, mon_buffer, sizeof(mon_buffer), 0) < 1) {
			break;	
		} 

		printf("Etat du système:'%s'\n", mon_buffer);
	}
}

int main(int argc, char** argv) {

	if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }
	if ( atoi(argv[2])<1 || atoi(argv[2]) > 65535) {
        fprintf(stderr, "port incorrect\n");
        exit(1);
	}
	printf("Prepare to connect to %s:%d\n", argv[1],atoi(argv[2]));

	int clientSocket, ret;
	struct sockaddr_in serverAddr;
	location buffer;
	int localisation;
	int mode;
	networkmsgloc messageloc;

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0) {
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Client Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

	printf("[+]Client trying to connect...\n");
	ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");

	// creation du thread de notification
	pthread_t threadId;
	int err = pthread_create(&threadId, NULL, &notification, clientSocket);
	if (err) {
		perror("pthread_create");
		exit(1);
	} 
	printf("thread de notification créé\n");


	while (1) {

		printf("\nClient: ");
		scanf("%[^\n]s", buffer.nom);

		printf("Cpu: ");
		scanf("%d", &buffer.cpu);

		printf("Stockage: ");
		scanf("%d", &buffer.stockage);

		printf("Mode: ");
		scanf("%d", &mode);

		printf("Localisation: ");
		scanf("%d", &localisation);

		printf("Votre nom: %s\n", buffer.nom);
		printf("Votre demande:\n");
		printf("* CPU: %d\n", buffer.cpu);
		printf("* Stockage: %d\n",buffer.stockage);
		printf("* mode:%d\n",mode);
		printf("* localisation:%d\n",localisation);

		// corriger scanf
		int c; while ((c = getchar()) != '\n' && c != EOF) { }

		messageloc.mylocalisation = localisation;
		messageloc.mymode = mode;
		messageloc.mylocation.cpu = buffer.cpu;
		strcpy(messageloc.mylocation.nom , buffer.nom);
		messageloc.mylocation.stockage = buffer.stockage;
	
		if(send(clientSocket, &messageloc, sizeof(messageloc),0) < 0) {
			perror("send client");
			break ;
		}

	}

	return 0;
}
