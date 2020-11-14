<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdint.h>
#include <inttypes.h>
//#include <uintptr_t>

#include "lib.h"

int main(int argc, char** argv) {
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
	if ((uintptr_t) cloud == -1) {
		perror("shmat");
	}
	
	switch (fork())	{
		case -1: // erreur
			printf("Erreur de fork\n");
			exit(-1);
		case 0: // fils
			printf("L'adresse de la variable fils est : %p\n", cloud);
			
			printf("Détachement de la variable partagée\n");
			shmdt(cloud);

			break;
		default: // pere
			printf("L'adresse de la variable pere est : %p\n", cloud);

			cloud->montpellier = montpellier;
			cloud->lyon = lyon;
			cloud->paris = paris;

			printf("Détachement de la variable partagée\n");
			shmdt(cloud);

			wait(0);
			break;
	}
=======
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "lib.h"

int main(int argc, char** argv) {
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
	
	switch (fork())	{
		case -1: // erreur
			printf("Erreur de fork\n");
			exit(-1);
		case 0: // fils
			printf("L'adresse de la variable fils est : %p\n", cloud);
			
			printf("Détachement de la variable partagée\n");
			shmdt(cloud);

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
>>>>>>> ea1409214224025ed337cc136d89cafb8a2cfc8d
}