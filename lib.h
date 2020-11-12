// ressources pour un datacenter
typedef struct	{
	int cpu;
	int ram;
} resource;

// resources pour tous les datacenters
typedef struct {
	resource montpellier;
	resource lyon;
	resource paris; 
} resources;