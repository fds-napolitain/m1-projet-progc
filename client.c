#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lib.h"

int main(int argc, char** argv) {

	int clientSocket, ret;
	struct sockaddr_in serverAddr;
	location buffer;
	int localisation;
	int mode;

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0) {
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Client Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");

	while (1) {
		printf("Client: ");
		scanf("%s\n", buffer.nom);
		printf("Cpu: ");
		scanf("%i\n", buffer.cpu);
		printf("Stockage: ");
		scanf("%i\n", buffer.stockage);
		printf("Mode: ");
		scanf("%i\n", mode);
		printf("Localisation: ");
		scanf("%i\n", localisation);

		send(clientSocket, buffer.nom, sizeof(buffer.nom), 0);
		send(clientSocket, buffer.cpu, sizeof(buffer.cpu), 0);
		send(clientSocket, buffer.stockage, sizeof(buffer.stockage), 0);
		send(clientSocket, mode, sizeof(mode), 0);
		send(clientSocket, localisation, sizeof(localisation), 0);

		/*if(strcmp(buffer, ":exit") == s0){
			close(clientSocket);
			printf("[-]Disconnected from server.\n");
			exit(1);
		}*/

		if (recv(clientSocket, buffer, 1024, 0) < 0) {
			printf("[-]Error in receiving data.\n");
		} else {
			printf("Server: \t%s\n", buffer);
		}
	}

	return 0;
}