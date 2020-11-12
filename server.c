#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "lib.h"

int main(int argc, char** argv) {
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
	
	switch (fork())	{
		case -1: // erreur
			printf("Erreur de fork\n");
			exit(-1);
		case 0: // fils
			printf("Le fiston\n");
			break;
		default: // pere
			printf("Création de la clé d'accès IPC\n");
			key_t key = ftok("server", 1);

			printf("Création du segment de mémoire partagée\n");
			int sh_id = shmget(key, sizeof(resources), IPC_CREAT | 0666);
			if (sh_id == -1) {
				perror("shmget");
				exit(-1);
			}

			printf("Initialisation de la variable partagée\n");
			resources* cloud = shmat(sh_id, 0, 0);
			if ((int) cloud == -1) {
				perror("shmat");
			}
			cloud->montpellier = montpellier;
			cloud->lyon = lyon;
			cloud->paris = paris;

			printf("Détachement de la variable partagée\n");
			shmdt(cloud);

			wait(0);
			break;
	}
}