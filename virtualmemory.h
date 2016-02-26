// Authors: Patrick Lebold & Patrick Polley

#ifndef VIRTUAL_MEMORY
#define VIRTUAL_MEMORY

#include <semaphore.h>

#define RAM 1
#define SSD 2
#define HD 3
#define RAM_SIZE 	25
#define SSD_SIZE	100
#define HD_SIZE 	1000
//#define RAM_ACCESS  (10 * 1000)
//#define SSD_ACCESS  (100 * 1000)
//#define HD_ACCESS   (2500 * 1000)

#define RAM_ACCESS  1
#define SSD_ACCESS  5
#define HD_ACCESS   50

typedef signed short vAddr;
typedef struct TableEntry {
	vAddr virtualAddress;
	int physicalAddress;
	int memoryType;
	int occupied;
	int r;
	pthread_mutex_t lock;
	pthread_cond_t condition_variable;
} TableEntry;

vAddr create_page();
int *get_value(vAddr address);
void store_value(vAddr address, int *value);
void free_page(vAddr address);

#endif
