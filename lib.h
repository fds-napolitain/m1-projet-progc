#define PORT 4444

// ressources pour un datacenter
typedef struct {
	int cpu;
	int stockage;
	char** exclusif;
	char** partage;
} datacenter;

// ressources pour tous les datacenters
typedef struct {
	datacenter montpellier;
	datacenter lyon;
	datacenter paris; 
} datacenters;

// structure pour envoyer une demande de location
typedef struct {
	int cpu;
	int stockage;
	char* nom;
	char* mode;
} location;