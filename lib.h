#define CPU 0
#define STOCKAGE 1
#define NB_ITEMS 2

#define PARTAGE 0
#define EXCLUSIF 1
#define NB_MODES 2

#define MONTPELLIER 0
#define LYON 1
#define PARIS 2
#define NB_PLACES 3

// structure pour envoyer une demande de location
typedef struct {
	int cpu;
	int stockage;
	char nom[20];
} location;

// message reseau
typedef struct {
	location mylocation;
	int mymode;
	int mylocalisation;
} networkmsgloc;

// message de retour de demande
char message[255];

// structure alternative pour un DC
typedef struct {
	int ressources[NB_PLACES][NB_ITEMS];
	int maxressources[NB_PLACES][NB_ITEMS];
} cloudstate;
