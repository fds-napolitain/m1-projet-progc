#define PORT 4444

#define EXCLUSIF 1
#define PARTAGE 0

#define MONTPELLIER 0
#define LYON 1
#define PARIS 2

// structure pour envoyer une demande de location
typedef struct {
	int cpu;
	int stockage;
	char nom[20];
} location;

// ressources pour un datacenter
typedef struct {
	int cpu;
	int stockage;
	location* exclusif;
	location* partage;
} datacenter;

// ressources pour tous les datacenters
typedef struct {
	datacenter montpellier;
	datacenter lyon;
	datacenter paris; 
} datacenters;
