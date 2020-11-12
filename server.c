#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char** argv) {
	pid_t pid;

	// ressources pour un datacenter
	typedef struct resource	{
		int cpu;
		int ram;
	} resource;

	// resources pour tous les datacenters
	typedef struct resources {
		resource montpellier;
		resource lyon;
		resource paris; 
	} resources;
	
	switch (pid = fork())	{
		case -1: // erreur
			printf("Erreur de fork\n");
			return -1;
		case 0: // fils

			break;
		default: // pere
			resource montpellier = {
				.cpu = 60,
				.ram = 150,
			};
			resource lyon = {
				.cpu = 90,
				.ram = 120,
			};
			resource paris = {
				.cpu = 30,
				.ram = 20,
			};
			resources cloud = {
				.montpellier = montpellier,
				.lyon = lyon,
				.paris = paris,
			};
			printf("Création de la clé d'accès IPC\n");
			key_t key = ftok("server", 1);

			int sh_id = shmget(key, sizeof(resources), IPC_CREAT | 0666);

			wait(0);
			break;
	}
}