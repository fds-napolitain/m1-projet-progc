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
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "lib.h"

// fonction de notification
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t_cond = PTHREAD_COND_INITIALIZER;

void notification(int my_socket) {
	datacenters* cloud;
	key_t key = ftok("server", 1);

	int sh_id = shmget(key, sizeof(datacenters), 0666);
	if (sh_id == -1) {
		perror("notif, shmget");
		exit(-1);
	}

	cloud = shmat(sh_id, 0, 0);
	if ((uintptr_t) cloud == -1) {
		perror("notif, shmat");
		exit(-1);
	}

	while (1) {

		pthread_mutex_lock(&t_mutex);

		pthread_cond_wait(&t_cond, &t_mutex);

		printf("Je pousse la notification au socket %d !\n",my_socket);

		if (send(my_socket, cloud, sizeof(datacenters), 0) < 0) {
			perror("send client");
			break ;
		}

		pthread_mutex_unlock(&t_mutex);

	}
}

// fonction de reveil du pere
void reveil(int signum)
{
	// le pere est reveillé, il reveille tout les threads de notifications
	pthread_mutex_lock(&t_mutex);
	pthread_cond_broadcast(&t_cond);
	pthread_mutex_unlock(&t_mutex);
}

/**
 * renvoit le total de cpu / stockage partagée contenu dans la liste des locations
 */
int fnlocation(location* d, int MODE) {
	int n = 0;
	switch (MODE) {
		case CPU:
			for (int i = 0; i < sizeof(d)/sizeof(location); i++) {
				n += d[i].cpu;
			}
			break;
		default:
			for (int i = 0; i < sizeof(d)/sizeof(location); i++) {
				n += d[i].stockage;
			}
			break;
	}
	return n;
}

int main(int argc, char** argv) {

	if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
	if ( atoi(argv[1])<1 || atoi(argv[1]) > 65535) {
        fprintf(stderr, "port incorrect\n");
        exit(1);
	}
	printf("Le serveur va exposer le port 127.0.0.1:%d\n", atoi(argv[1]));

	// gestion du cloud
	datacenter montpellier;
	datacenter lyon;
	datacenter paris;
	datacenters* cloud;
	networkmsgloc messageloc;

	printf("Création de la clé d'accès IPC\n");
	key_t key = ftok("server", 1);

	printf("Création du segment de mémoire partagée\n");
	int sh_id = shmget(key, sizeof(datacenters), IPC_CREAT | 0666);
	if (sh_id == -1) {
		perror("shmget");
		exit(-1);
	}

	printf("Initialisation de la variable partagée\n");
	cloud = shmat(sh_id, 0, 0);
	if ((uintptr_t) cloud == -1) {
		perror("shmat");
		exit(-1);
	}
	printf("L'adresse de la variable pere est : %p\n", cloud);
	montpellier.cpu = 60;
	montpellier.stockage = 700;
	montpellier.exclusif = malloc(sizeof(location));
	montpellier.partage = malloc(sizeof(location));
	lyon.cpu = 64;
	lyon.stockage = 2000;
	lyon.exclusif = malloc(sizeof(location));
	lyon.partage = malloc(sizeof(location));
	paris.cpu = 10;
	paris.stockage = 100;
	paris.exclusif = malloc(sizeof(location));
	paris.partage = malloc(sizeof(location));
	cloud->montpellier = montpellier;
	cloud->lyon = lyon;
	cloud->paris = paris;
	
	// gestion des sockets
	int sockfd, ret;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	location buffer;
	pid_t childpid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("[-]Erreur de connexion.\n");
		exit(1);
	}
	printf("[+]Socket serveur créé.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = bind(sockfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		perror("[-]Erreur de bind.\
		n");
		exit(1);
	}
	printf("[+]Bind du port %d\n",atoi(argv[1]));

	if (listen(sockfd, 10) == 0) {
		printf("[+]En écoute....\n");
 	} else {
		perror("[-]Erreur de bind.\n");
		exit(1);
	}

	// le père va réagir au signal SIGUSR2 en appelant la fonction reveil() qui elle même réveillera les threads de notification
	signal(SIGUSR2, reveil);

	// boucle d'écoute des clients
	while (1) {
		newSocket = accept(sockfd, (struct sockaddr*) &newAddr, &addr_size);
		if(newSocket < 0){
			exit(1);
		}
		printf("Connection acceptée de %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

		// creation du thread de notification à chaque connection de client
		pthread_t threadId;
		int err = pthread_create(&threadId, NULL, &notification, newSocket);
		if (err) {
			perror("pthread_create");
			exit(1);
		} 
		printf("thread de notification créé pour le socket %d\n", newSocket);
	
		switch (fork())	{
			case -1: // erreur
				printf("Erreur de fork\n");
				exit(-1);
			case 0: // fils
				printf("L'adresse de la variable fils est : %p\n", cloud);
				close(sockfd);

				int i = 0;
				// traitement de la requete du client
				while (1) {
					i++;
					printf("itération %d-%d\n",getpid(),i);
					int mode;
					int localisation;

					if (recv(newSocket, &messageloc, sizeof(messageloc), 0) < 1 ) 
						break ;

					strcpy( buffer.nom, messageloc.mylocation.nom);
					buffer.cpu = messageloc.mylocation.cpu;
					buffer.stockage = messageloc.mylocation.stockage;
					mode = messageloc.mymode;
					localisation = messageloc.mylocalisation;

					printf("Client %d - Votre nom: %s\n", getpid(),buffer.nom);
					printf("Client %d - Votre demande:\n", getpid());
					printf("Client %d - * CPU: %d\n", getpid(), buffer.cpu);
					printf("Client %d - * Stockage: %d\n", getpid(),buffer.stockage);
					printf("Client %d - * mode:%d\n", getpid(),mode);
					printf("Client %d - * localisation:%d\n", getpid(),localisation);

					// reveil des threads de notification de tous les fils
					// on réveille d'abord le père
					if (kill(getppid(), SIGUSR2) == -1) {
						/* En cas d'erreur... */
						perror("kill(2)\n");
						exit(1);
					}

					/**
					 * On choisit le datacenter puis
					 * - si demande de ressources exclusives, comparaison avec ce qui est disponible dans le datacenter
					 * - si demande de ressources partagées, comparaison avec ce qui est disponible ainsi que ce qui est partagé
					 *   (si il y a 40 cpu partage et 10 libres, une demande de 50 en partagé marche et retire les 10 cpus libres restants)
					 */
					switch (localisation) {
						case MONTPELLIER:
							if (montpellier.cpu >= buffer.cpu && montpellier.stockage >= buffer.stockage && mode == EXCLUSIF) {
								int index = (sizeof(*montpellier.exclusif) / sizeof(location)) - 1;
								montpellier.exclusif[index].cpu = buffer.cpu;
								montpellier.exclusif[index].stockage = buffer.stockage;
								*montpellier.exclusif[index].nom = buffer.nom;
								montpellier.cpu -= buffer.cpu;
								montpellier.stockage -= buffer.stockage;
								montpellier.exclusif = realloc(montpellier.exclusif, sizeof(montpellier.exclusif) + sizeof(location));
							} else if (fnlocation(montpellier.partage, CPU) + montpellier.cpu >= buffer.cpu && mode == PARTAGE) {
								int index = (sizeof(*montpellier.partage) / sizeof(location)) - 1;
								montpellier.partage[index].cpu = fnlocation(montpellier.partage, CPU) - buffer.cpu;
								montpellier.partage[index].stockage = fnlocation(montpellier.partage, STOCKAGE) - buffer.stockage;
								*montpellier.partage[index].nom = buffer.nom;
								montpellier.partage = realloc(montpellier.partage, sizeof(montpellier.partage) + sizeof(location));
							}
							break;
						case LYON:
							if (lyon.cpu >= buffer.cpu && lyon.stockage >= buffer.stockage && mode == EXCLUSIF) {
								int index = (sizeof(*lyon.exclusif) / sizeof(location)) - 1;
								lyon.exclusif[index].cpu = buffer.cpu;
								lyon.exclusif[index].stockage = buffer.stockage;
								*lyon.exclusif[index].nom = buffer.nom;
								lyon.cpu -= buffer.cpu;
								lyon.stockage -= buffer.stockage;
								lyon.exclusif = realloc(lyon.exclusif, sizeof(lyon.exclusif) + sizeof(location));
							} else if (fnlocation(lyon.partage, CPU) + lyon.cpu >= buffer.cpu && mode == PARTAGE) {
								int index = (sizeof(*lyon.partage) / sizeof(location)) - 1;
								lyon.partage[index].cpu = fnlocation(lyon.partage, CPU) - buffer.cpu;
								lyon.partage[index].stockage = fnlocation(lyon.partage, STOCKAGE) - buffer.stockage;
								*lyon.partage[index].nom = buffer.nom;
								lyon.partage = realloc(lyon.partage, sizeof(lyon.partage) + sizeof(location));
							}
							break;
						case PARIS:
							if (paris.cpu >= buffer.cpu && paris.stockage >= buffer.stockage && mode == EXCLUSIF) {
								int index = (sizeof(*paris.exclusif) / sizeof(location)) - 1;
								paris.exclusif[index].cpu = buffer.cpu;
								paris.exclusif[index].stockage = buffer.stockage;
								*paris.exclusif[index].nom = buffer.nom;
								paris.cpu -= buffer.cpu;
								paris.stockage -= buffer.stockage;
								paris.exclusif = realloc(paris.exclusif, sizeof(paris.exclusif) + sizeof(location));
							} else if (fnlocation(paris.partage, CPU) + paris.cpu >= buffer.cpu && mode == PARTAGE) {
								int index = (sizeof(*paris.partage) / sizeof(location)) - 1;
								paris.partage[index].cpu = fnlocation(paris.partage, CPU) - buffer.cpu;
								paris.partage[index].stockage = fnlocation(paris.partage, STOCKAGE) - buffer.stockage;
								*paris.partage[index].nom = buffer.nom;
								paris.partage = realloc(paris.partage, sizeof(paris.partage) + sizeof(location));
							}
							break;
						default:
							break;
					}
					printf("Requête traitée par le serveur.\n");
				}
				break;

		}
	}
	close(newSocket);
}
