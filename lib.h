#define PORT 4444

// ressources pour un datacenter
typedef struct {
	int cpu;
	int stockage;
} resource;

// resources pour tous les datacenters
typedef struct {
	resource montpellier;
	resource lyon;
	resource paris; 
} resources;
