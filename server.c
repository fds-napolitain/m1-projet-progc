#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "lib.h"

int main(int argc, char** argv) {
	pid_t pid;
	
	switch (pid = fork())	{
		case -1: // erreur
			printf("Erreur de fork\n");
			exit(-1);
		case 0: // fils

			break;
		default: // pere
			resource montpellier = {
				.cpu = 60,
				.ram = 150
			};
			resource lyon = {
				.cpu = 90,
				.ram = 120
			};
			resource paris = {
				.cpu = 30,
				.ram = 20
			};
			resources cloud = {
				.montpellier = montpellier,
				.lyon = lyon,
				.paris = paris
			};
			printf("Création de la clé d'accès IPC\n");
			key_t key = ftok("server", 1);

			printf("Création du segment de mémoire partagée\n");
			int sh_id = shmget(key, sizeof(resources), IPC_CREAT | 0666);

			if (sh_id == -1) {
				perror("shmget");
				exit(-1);
			}

			wait(0);
			break;
	}
}