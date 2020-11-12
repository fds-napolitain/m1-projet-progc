#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MONTPELLIER 0
#define LYON 1
#define PARIS 2

int main(int argc, char** argv) {
	pid_t pid;

	typedef struct resources {
		int location;
	} resources;
	typedef struct resource	{
		int cpu;
		int ram;
	} resource;

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
	
	switch (pid = fork())	{
		case -1: // erreur
			printf("Erreur de fork\n");
			return -1;
		case 0: // fils

			break;
		default: // pere
			printf("Création de la clé d'accès IPC\n");
			key_t key = ftok("server", 1);

			int sh_id = shmget(key, sizeof(resource), IPC_CREAT | 0666);

			wait(0);
			break;
	}
}