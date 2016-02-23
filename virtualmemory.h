// Authors: Patrick Lebold & Patrick Polley

#ifndef VIRTUAL_MEMORY
#define VIRTUAL_MEMORY

#define RAM 1
#define SSD 2
#define HD 3
#define RAM_SIZE 	25
#define SSD_SIZE	100
#define HD_SIZE 	1000
#define RAM_ACCESS  (10 * 1000)
#define SSD_ACCESS  (100 * 1000)
#define HD_ACCESS   (2500 * 1000)

vAddr create_page();
u_int32_t *get_value(vAddr address);
void store_value(vAddr address, u_int32_t *value);
void free_page(vAddr address);

typedef struct TableEntry {
	vAddr virtualAddress;
	int physicalAddress;
	int memoryType;]
	int occupied;
} table_entry;

#endif
