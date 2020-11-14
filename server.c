#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "lib.h"

int main(int argc, char** argv) {
	// gestion du cloud
	resource montpellier;
	resource lyon;
	resource paris;
	resources* cloud;

	printf("Création de la clé d'accès IPC\n");
	key_t key = ftok("server", 1);

	printf("Création du segment de mémoire partagée\n");
	int sh_id = shmget(key, sizeof(resources), IPC_CREAT | 0666);
	if (sh_id == -1) {
		perror("shmget");
		exit(-1);
	}

	printf("Initialisation de la variable partagée\n");
	cloud = shmat(sh_id, 0, 0);
	if ((int) cloud == -1) {
		perror("shmat");
	}

	// gestion des sockets
	int sockfd, ret;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	char buffer[1024];
	pid_t childpid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Server Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret < 0){
		printf("[-]Error in binding.\n");
		exit(1);
	}
	printf("[+]Bind to port %d\n", 4444);

	if(listen(sockfd, 10) == 0){
		printf("[+]Listening....\n");
	}else{
		printf("[-]Error in binding.\n");
	}

	// boucle d'écoute des clients
	while(1){
		newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
		if(newSocket < 0){
			exit(1);
		}
		printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
	
		switch (fork())	{
			case -1: // erreur
				printf("Erreur de fork\n");
				exit(-1);
			case 0: // fils
				printf("L'adresse de la variable fils est : %p\n", cloud);
				close(sockfd);

				// boucle temporaire de test
				while(1){
					recv(newSocket, buffer, 1024, 0);
					if(strcmp(buffer, ":exit") == 0){
						printf("Disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
						break;
					}else{
						printf("Client: %s\n", buffer);
						send(newSocket, buffer, strlen(buffer), 0);
						bzero(buffer, sizeof(buffer));
					}
				}
				break;

			default: // pere
				printf("L'adresse de la variable pere est : %p\n", cloud);
				montpellier.cpu = 60;
				montpellier.stockage = 700;
				lyon.cpu = 64;
				lyon.stockage = 2000;
				paris.cpu = 10;
				paris.stockage = 100;
				cloud->montpellier = montpellier;
				cloud->lyon = lyon;
				cloud->paris = paris;

				printf("Détachement de la variable partagée\n");
				shmdt(cloud);

				wait(0);
				break;
		}
	}
	close(newSocket);
}