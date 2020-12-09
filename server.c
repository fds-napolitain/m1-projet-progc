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
#include <signal.h>
#include <sys/sem.h>

#include "lib.h"

// RAPPEL: structure d'operation sur les semaphores
//struct sembuf {
//	unsigned short sem_num; /* Numero du semaphore */
//	short sem_op ; /* Operation sur le semaphore */
//	short sem_flg ; /* Options par exemple SEM UNDO */
// };


// verrou mise à jour du système
pthread_mutex_t cloud_mutex = PTHREAD_MUTEX_INITIALIZER;

// fonction de notification
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t_cond = PTHREAD_COND_INITIALIZER;

// construction d'une chaine de notification
void buildNotif(char* strnotif, cloudstate *lecloud){
	char tmpnotif[LNG_NOTIF];
	// construction de la notification
	sprintf(tmpnotif,"\n*** ETAT DU SYSTEME (actuel /max) ***\n");
	strcpy(strnotif, tmpnotif);
	sprintf(tmpnotif,"0.Montpellier:\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- cpu: %d /%d\n", lecloud->ressources[MONTPELLIER][CPU],lecloud->maxressources[MONTPELLIER][CPU]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- stockage: %d /%d\n", lecloud->ressources[MONTPELLIER][STOCKAGE],lecloud->maxressources[MONTPELLIER][STOCKAGE]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"1.Lyon:\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- cpu: %d /%d\n", lecloud->ressources[LYON][CPU], lecloud->maxressources[LYON][CPU]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- stockage: %d /%d\n", lecloud->ressources[LYON][STOCKAGE],lecloud->maxressources[LYON][STOCKAGE] );
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"2.Paris:\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- cpu: %d /%d\n", lecloud->ressources[PARIS][CPU],lecloud->maxressources[PARIS][CPU] );
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- stockage: %d /%d\n", lecloud->ressources[PARIS][STOCKAGE],lecloud->maxressources[PARIS][STOCKAGE] );
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"3.Partagé\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- Montpellier:\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|  |- cpu: %d\n", lecloud->ressources_partagees[MONTPELLIER][CPU]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|  |- stockage: %d\n", lecloud->ressources_partagees[MONTPELLIER][STOCKAGE]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- Lyon:\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|  |- cpu: %d\n", lecloud->ressources_partagees[LYON][CPU]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|  |- stockage: %d\n", lecloud->ressources_partagees[LYON][STOCKAGE]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|- Paris:\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|  |- cpu: %d\n", lecloud->ressources_partagees[PARIS][CPU]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"|  |- stockage: %d\n", lecloud->ressources_partagees[PARIS][STOCKAGE]);
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif,"4.Par personnes\n");
	strcat(strnotif, tmpnotif);
	sprintf(tmpnotif, "|- Exclusif\n");
	strcat(strnotif, tmpnotif);
	for (int i = 0; i < NB_PLACES; i++) {	
		for (int j = 0; j < NB_PERSONNES; j++) {
			if (lecloud->exclusif[i][j].cpu == 0 && lecloud->exclusif[i][j].stockage == 0) {
				break;
			}
			sprintf(tmpnotif,"|  |- %s\n", lecloud->exclusif[i][j].nom);
			strcat(strnotif, tmpnotif);
			sprintf(tmpnotif,"|  |- %d\n", lecloud->exclusif[i][j].cpu);
			strcat(strnotif, tmpnotif);
			sprintf(tmpnotif,"|  |- %d\n", lecloud->exclusif[i][j].stockage);
			strcat(strnotif, tmpnotif);
		}
	}
	sprintf(tmpnotif, "|- Partagé\n");
	strcat(strnotif, tmpnotif);
	for (int i = 0; i < NB_PLACES; i++) {
		for (int j = 0; j < NB_PERSONNES; j++) {
			if (lecloud->partage[i][j].cpu == 0 && lecloud->partage[i][j].stockage == 0) {
				break;
			}
			sprintf(tmpnotif,"|  |- %s\n", lecloud->partage[i][j].nom);
			strcat(strnotif, tmpnotif);
			sprintf(tmpnotif,"|  |- %d\n", lecloud->partage[i][j].cpu);
			strcat(strnotif, tmpnotif);
			sprintf(tmpnotif,"|  |- %d\n", lecloud->partage[i][j].stockage);
			strcat(strnotif, tmpnotif);
		}
	}
	sprintf(tmpnotif,"*** FIN DE NOTIFICATION ***\n");
	strcat(strnotif, tmpnotif);
}

// fonction du thread de notification, pour notifier le thread de notif du client via le socket de notif
void notification(int my_socket) {
	cloudstate *lecloud;
	char strnotif[LNG_NOTIF];

	key_t key = ftok("server", 2);

	int sh_id = shmget(key, sizeof(cloudstate), 0666);
	if (sh_id == -1) {
		perror("notif, shmget");
		exit(-1);
	}

	lecloud = (cloudstate *) shmat(sh_id, 0, 0);
	if ((uintptr_t) lecloud == -1) {
		perror("notif, shmat");
		exit(-1);
	}

	while (1) {

		pthread_mutex_lock(&t_mutex);

		pthread_cond_wait(&t_cond, &t_mutex);

		// construction de la notification
		buildNotif(strnotif, lecloud);

		printf("Je pousse la notification au socket %d !\n",my_socket);

		if (send(my_socket, strnotif, sizeof(strnotif), 0) < 0) {
			perror("send client");
			break ;
		}

		// Affichage côté serveur pour information
		printf(strnotif);

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

// positionnement de la valeur du verrou et de la ressource correspondant à la ressource ress sur le DC loc
void setRessource(int semid, cloudstate* mycloud, int loc, int ress, int value, int mode, char* nom) {
	// index correspondant à PLACE sur 3, ITEMS sur 2
	int index = loc * NB_ITEMS + ress ;
	struct sembuf sb;
	sb.sem_num = index;
	sb.sem_flg = 0;
	if (mode == PARTAGE) {
		if (value > 0 && value - mycloud->ressources_partagees[loc][ress] >= 0) {
			value -= mycloud->ressources_partagees[loc][ress]; // utiliser les res partagees
		} else if (value < 0) {
			int v = 0;
			printf("Mode partagé, parcours de liste... ");
			for (int i = 0; i < sizeof(*mycloud->partage[loc])/sizeof(location); i++) {
				printf("%d, ",i);
				if (ress == CPU && strcmp(mycloud->partage[loc][i].nom, nom) != 0) {
					printf("v=%d, ",v);
					v += mycloud->partage[loc][i].cpu;
				}
			}
			value = - (mycloud->ressources_partagees[loc][ress] - v);
			printf("value=%d \n",value);
		}
	}
	if (value != 0) {
		sb.sem_op = -value; // si on en veut +N, il faut décrementer de N. 
		//printf("#DEBUG: ajout dans le semaphore %d à la position %d de la valeur %d\n", semid, index, -value );
		if (semop(semid, &sb, 1) == -1) {
			perror("semop setRessource");
			exit(1); //BADABOUM, tant pis pour les mutex et sem...
		} else {
			// on a mis à jour le sem, alors on met à jour la variable partagée
			pthread_mutex_lock(&cloud_mutex);
			mycloud->ressources[loc][ress] -= value;
			pthread_mutex_unlock(&cloud_mutex);
		}
	}

	int existe = -1; // écrire à un emplacement existant (lié au nom)
	int place_libre = -1; // écrire à un emplacement libre (suite ou emplacement libéré)
	int i = 0;
	if (mode == EXCLUSIF) {
		for (i = 0; i < NB_PERSONNES; i++) {
			if (strcmp(mycloud->exclusif[loc][i].nom, nom) == 0) {
				existe = i;
			}
			if (mycloud->exclusif[loc][i].cpu == 0 && mycloud->exclusif[loc][i].stockage == 0) {
				place_libre = i;
			}
		}
		printf("existe placelibre %d %d\n", existe, place_libre);
		strcpy(mycloud->exclusif[loc][place_libre].nom, nom);
		if (ress == CPU) {
			mycloud->exclusif[loc][place_libre].cpu += value;
		} else {
			mycloud->exclusif[loc][place_libre].stockage += value;
		}
	} else {
		for (i = 0; i < NB_PERSONNES; i++) {
			if (strcmp(mycloud->partage[loc][i].nom, nom) == 0) {
				existe = i;
			}
			if (mycloud->partage[loc][i].cpu == 0 && mycloud->partage[loc][i].stockage == 0) {
				place_libre = i;
			}
		}
		printf("existe placelibre %d %d\n", existe, place_libre);
		strcpy(mycloud->partage[loc][place_libre].nom, nom);
		if (ress == CPU) {
			mycloud->partage[loc][place_libre].cpu += value;
		} else {
			mycloud->partage[loc][place_libre].stockage += value;
		}
		mycloud->ressources_partagees[loc][ress] += value;
	}
}

// init de la valeur du verrou 
void initSem(int semid, int loc, int ress, int value ) {
	// index correspondant à PLACE sur 3, ITEMS sur 2
	int index = loc * NB_ITEMS + ress ;
	//printf("#DEBUG: INIT du semaphore %d à la position %d à la valeur %d\n", semid, index, value+1 );
 	if (semctl(semid, index, SETVAL, (value) )==-1) {
		perror("semctl initSem");
		exit(1);
	}
}

int main(int argc, char** argv) {

	if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\nNote: le serveur utilisera un 2nd port consécutif\n", argv[0]);
        exit(1);
    }
	if ( atoi(argv[1])<1 || atoi(argv[1]) > 65534) {
        fprintf(stderr, "port incorrect\n");
        exit(1);
	}
	int port1 = atoi(argv[1]);
	int port2 = atoi(argv[1])+1;
	printf("Le serveur va exposer les ports :%d et :%d\n", port1, port2);

	// gestion du cloud
	networkmsgloc messageloc;
	// synthese etat du cloud
	cloudstate * mycloud;

	printf("Création de la clé d'accès IPC\n");
	key_t key = ftok("server", 2);

	printf("Création du segment de mémoire partagée\n");
	int sh_id2 = shmget(key, sizeof(cloudstate), IPC_CREAT | 0666);
	if (sh_id2 == -1) {
		perror("shmget");
		exit(-1);
	}

	// attachement du segment partagé à la variable
	mycloud = shmat(sh_id2, 0, 0);
	if ((uintptr_t) mycloud == -1) {
		perror("shmat");
		exit(-1);
	}
	printf("L'adresse de la variable mycloud pere est : %p\n", mycloud);

	printf("Création des sémaphores.\n");
	int semid = semget(key, NB_PLACES*NB_ITEMS, 0666 | IPC_CREAT);
	if (semid == -1) {
		perror("semget");
		exit(-1);
	}
	printf("L'id de la liste de semaphores est: %d\n", semid);

	// init des limites
	mycloud->maxressources[MONTPELLIER][CPU] = 		60;
	mycloud->maxressources[MONTPELLIER][STOCKAGE] = 700;
	mycloud->maxressources[LYON][CPU] = 			60;
	mycloud->maxressources[LYON][STOCKAGE] = 		2000;
	mycloud->maxressources[PARIS][CPU] = 			10;
	mycloud->maxressources[PARIS][STOCKAGE] = 		100;

	// init de l'état

	// init des états par personnes
	for (int i = 0; i < NB_PLACES; i++) {
		for (int j = 0; j < NB_PERSONNES; j++) {
			mycloud->exclusif[i][j].cpu = 0;
			mycloud->exclusif[i][j].stockage = 0;
			mycloud->partage[i][j].cpu = 0;
			mycloud->partage[i][j].stockage = 0;
		}
	}

	// init des ressources partagées
	for (int i = 0; i < NB_PLACES; i++) {
		for (int j = 0; j < NB_ITEMS; j++) {
			mycloud->ressources_partagees[i][j] = 0;
			mycloud->ressources_partagees[i][j] = 0;
			mycloud->ressources[i][j] = mycloud->maxressources[i][j];
			mycloud->ressources[i][j] = mycloud->maxressources[i][j];
		}
	}

	// init des verrous
	initSem(semid, MONTPELLIER, CPU, 		mycloud->maxressources[MONTPELLIER][CPU]);
	initSem(semid, MONTPELLIER, STOCKAGE, 	mycloud->maxressources[MONTPELLIER][STOCKAGE]);
	initSem(semid, LYON, CPU, 				mycloud->maxressources[LYON][CPU]);
	initSem(semid, LYON, STOCKAGE, 			mycloud->maxressources[LYON][STOCKAGE]);
	initSem(semid, PARIS, CPU, 				mycloud->maxressources[PARIS][CPU]);
	initSem(semid, PARIS, STOCKAGE, 		mycloud->maxressources[PARIS][STOCKAGE]);

	location buffer;
	pid_t childpid;

	// socket de dialogue
	int sockfd, ret;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("[-]Erreur de connexion.");
		exit(1);
	}
	printf("[+]Socket serveur créé.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port1);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = bind(sockfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		perror("[-]Erreur de bind.");
		exit(1);
	}
	printf("[+]Bind du port %d\n",atoi(argv[1]));

	if (listen(sockfd, 10) == 0) {
		printf("[+]En écoute....\n");
 	} else {
		perror("[-]Erreur de bind.");
		exit(1);
	}

	// socket de notif
	int sockfdN;
	int newSocketN;
	struct sockaddr_in newAddrN;

	sockfdN = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfdN < 0) {
		perror("[-]Erreur de connexion.");
		exit(1);
	}
	printf("[+]Socket notification serveur créé.\n");

	serverAddr.sin_port = htons(port2);

	ret = bind(sockfdN, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		perror("[-]Erreur de bind (notif).\n");
		exit(1);
	}
	printf("[+]Bind du port de notif %d\n",atoi(argv[1])+1);

	if (listen(sockfdN, 10) == 0) {
		printf("[+]Notif, en écoute....\n");
 	} else {
		perror("[-]Notif, Erreur de bind.");
		exit(1);
	}

	// le père va réagir au signal SIGUSR2 en appelant la fonction reveil() qui elle même réveillera les threads de notification
	signal(SIGUSR2, reveil);

	// boucle d'écoute des clients
	printf("\nEn attente de connexion de client...\n");
	while (1) {
		char strnotif[LNG_NOTIF];

		// attente d'un client
		newSocket = accept(sockfd, (struct sockaddr*) &newAddr, &addr_size);
		if(newSocket < 0){
			exit(1);
		}
		printf("Connection acceptée de %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
		
		// attente de la connexion du client à la socket de notif
		newSocketN = accept(sockfdN, (struct sockaddr*) &newAddrN, &addr_size);
		if(newSocketN < 0){
			exit(1);
		}
		printf("Connection 'Notif' acceptée de %s:%d\n", inet_ntoa(newAddrN.sin_addr), ntohs(newAddrN.sin_port));

		// HANDSHAKE
		strcpy(message, "BONJOUR");
		printf("Envoi d'un message de confirmation de connexion au socket %d:%s\n",newSocketN, message);
		if (send(newSocketN, message, sizeof(message), 0) < 0) {
			perror("echec de send client confirmation de notif");
			break ;
		}
		
		// première notif
		printf("Je pousse l'état du système au socket %d !\n",newSocketN);
		// construction de la notification
		buildNotif(strnotif, mycloud);
		if (send(newSocketN, strnotif, sizeof(strnotif), 0) < 0) {
			perror("send client");
			break ;
		}

		// creation du thread de notification à chaque connection de client
		pthread_t threadId;
		int err = pthread_create(&threadId, NULL, &notification, newSocketN);
		if (err) {
			perror("pthread_create");
			exit(1);
		} 
		printf("thread de notification créé pour le socket %d\n", newSocketN);
	
		switch (fork())	{
			case -1: // erreur
				printf("Erreur de fork\n");
				exit(-1);
			case 0: // fils
				printf("L'adresse de la variable fils est : %p\n", mycloud);
				close(sockfd);

				int i = 0;
				// traitement de la requete du client
				while (1) {
					i++;
					printf("itération %d-%d\n",getpid(),i);
					int mode;
					int localisation;

					printf("Système en attente de demande...\n");
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
					printf("Client %d - DEMANDE EN COURS...\n", getpid());

					// check localisation
					if (localisation == MONTPELLIER || localisation == LYON || localisation == PARIS) {
						// check limites
						if ((abs(buffer.cpu) > 0 ) && (abs(buffer.stockage) > 0) &&
						   (abs(buffer.cpu) <= mycloud->maxressources[localisation][CPU]) && 
						   (abs(buffer.stockage) <= mycloud->maxressources[localisation][STOCKAGE])) {
							// demande de ressources
							// l'une ou l'autre peut nous mettre en attente
							setRessource(semid, mycloud, localisation, CPU, buffer.cpu, mode, buffer.nom);
							setRessource(semid, mycloud, localisation, STOCKAGE, buffer.stockage, mode, buffer.nom);
							strcpy(message, "OK");
						} else {
							strcpy(message, "DEMANDE HORS LIMITES !");
						}
					} else {
						strcpy(message, "ERREUR DE LOCALISATION !");
					}
					printf("Client %d - DEMANDE TRAITEE.\n", getpid());
					printf("Retour de traitement au socket %d:%s\n",newSocket, message);
					if (send(newSocket, message, sizeof(message), 0) < 0) {
						perror("echec de send client confirmation");
						break ;
					}

					// reveil des threads de notification de tous les fils
					// on réveille d'abord le père
					if (kill(getppid(), SIGUSR2) == -1) {
						/* En cas d'erreur... */
						perror("kill(2)");
						exit(1);
					}
				}
				break;

		}
	}
	close(newSocket);
	close(newSocketN);
}
