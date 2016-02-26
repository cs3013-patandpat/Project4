// Author: Patrick Lebold & Patrick Polley

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "virtualmemory.h"

// Debug flag
int DEBUG;

// Variable definitions
TableEntry pageTable[1000];
int ram[25];
int ssd[100];
int hd[1000];
int pageCount;

void setupEmptyPage(int i){
	if(DEBUG) printf("Emptying page: %d\n",i);
	pageTable[i].virtualAddress = i;
	pageTable[i].physicalAddress = -1 ;
	pageTable[i].memoryType= -1 ;
	pageTable[i].occupied = 0 ;
}

vAddr setupPage(int i, int memoryType, int memoryLoc){
	if(DEBUG) printf("Preparing page: %d\n",i);
	pageTable[i].physicalAddress = memoryLoc;
	pageTable[i].memoryType = memoryType;
	pageTable[i].occupied = 1;
	
	if(memoryType == RAM)
		ram[pageTable[i].physicalAddress] = pageTable[i].physicalAddress;
	else if(memoryType == SSD)
		ssd[pageTable[i].physicalAddress-25] = pageTable[i].physicalAddress;
	else if(memoryType == HD)
		hd[pageTable[i].physicalAddress-125] = pageTable[i].physicalAddress;
	
	pageCount++;
	return i;
}

//------------------------------

int findFreeMemoryLoc(int memoryType) {
	if(DEBUG) printf("Searching for free memory location in level: %d\n",memoryType);
	int i ;
	if(memoryType == RAM){
		if(DEBUG) printf("Entering RAM\n");
		int freeMemoryLocs[RAM_SIZE];
		for(i=0;i<RAM_SIZE;i++)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;i++)
			if(pageTable[i].memoryType == RAM)
				freeMemoryLocs[pageTable[i].physicalAddress] = 0;
		for(i=0;i<RAM_SIZE;i++)
			if(freeMemoryLocs[i] == 1){
				if(DEBUG) printf("Memory found in RAM.\n");
				return i ;
			}
		if(DEBUG) printf("No memory found in RAM.\n");
		return -1;
	}
	else if(memoryType == SSD){
		if(DEBUG) printf("Entering SSD\n");
		int freeMemoryLocs[SSD_SIZE];
		for(i=0;i<SSD_SIZE;i++)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;i++)
			if(pageTable[i].memoryType == SSD)
				freeMemoryLocs[pageTable[i].physicalAddress-25] = 0;
		for(i=0;i<SSD_SIZE;i++)
			if(freeMemoryLocs[i] == 1){
				if(DEBUG) printf("Memory found in SSD.\n");
				return i ;
			}
		if(DEBUG) printf("No memory found in SSD.\n");
		return -1 ;
	}
	else if(memoryType == HD){
		if(DEBUG) printf("Entering HD\n");
		int freeMemoryLocs[HD_SIZE];
		for(i=0;i<HD_SIZE;i++)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;i++)
			if(pageTable[i].memoryType == HD)
				freeMemoryLocs[pageTable[i].physicalAddress-125] = 0;
		for(i=0;i<HD_SIZE;i++)
			if(freeMemoryLocs[i] == 1) {
				if(DEBUG) printf("Memory found in HD.\n");
				return i ;
			}
		if(DEBUG) printf("No memory found in HD.\n");
		return -1 ;
	}
	else{
		if(DEBUG) printf("Undefined memory type: %d\n",memoryType);
	}
	return -1;
}

void evict(int memoryType){
	if(DEBUG) printf("Evicting memory in level: %d\n",memoryType);
	int freeSpace;
	if(memoryType == RAM){
		freeSpace = findFreeMemoryLoc(SSD);
		if(freeSpace == -1){
			if(DEBUG) printf("No space available in level: %d\n",memoryType+1);
			evict(SSD);
		}
	}
	else if(memoryType == SSD){
		freeSpace = findFreeMemoryLoc(HD);
		if(freeSpace == -1){
			printf("Hard drive is full. Lossless eviction not possible.\n");
			return;
		}
	}
	
	int i;
	for(i = 0; i < 1000; i++){
		if(pageTable[i].memoryType == RAM && memoryType == RAM){
			if(DEBUG) printf("Eviction successful.\n");
			ssd[freeSpace] = ram[pageTable[i].physicalAddress];
			ram[pageTable[i].physicalAddress] = -1;
			setupPage(i,SSD,freeSpace + 25);
			usleep(RAM_ACCESS);
			return;
		}
		else if(pageTable[i].memoryType == SSD && memoryType == SSD){
			if(DEBUG) printf("Eviction successful.\n");
			hd[freeSpace] = ssd[pageTable[i].physicalAddress-25];
			ssd[pageTable[i].physicalAddress-25] = -1;
			setupPage(i,HD,freeSpace + 125);
			usleep(SSD_ACCESS);
			return;
		}
	}
	
	//Probably not necessary
	if(memoryType == RAM){
		if(DEBUG) printf("No space available in level: %d\n",memoryType);
		evict(SSD);
	}
	else
		printf("Could not evict");
}

void handlePageFault(vAddr address){
	if(DEBUG) printf("Handling page fault: %d\n",address);
	int physicalAddress = pageTable[address].physicalAddress;
	int memoryType = pageTable[address].memoryType;
	
	if(memoryType == SSD){
		if(DEBUG) printf("Bumping memory cell from SSD -> RAM\n");
		int freeSpace = findFreeMemoryLoc(RAM);
		int value = ssd[physicalAddress-25];
		if(freeSpace == -1){
			evict(RAM);
			freeSpace = findFreeMemoryLoc(RAM);
		}
		ram[freeSpace] = value;
		pageTable[address].physicalAddress = freeSpace;
		pageTable[address].memoryType = RAM;
	}
	else if(memoryType == HD){
		if(DEBUG) printf("Bumping memory cell from HD -> SSD\n");
		int freeSpace = findFreeMemoryLoc(SSD);
		int value = hd[physicalAddress-125];
		if(freeSpace == -1){
			evict(SSD);
			freeSpace = findFreeMemoryLoc(SSD);
			if(freeSpace == -1){
				printf("[SEVERE] Page fault error\n");
				return;
			}
		}
		ssd[freeSpace] = value;
		pageTable[address].physicalAddress = freeSpace + 25;
		pageTable[address].memoryType = SSD;
		handlePageFault(address);
	}
}

//------------------------------
vAddr create_page(){	
	if(DEBUG) printf("Creating page.\n");
	
	int freeSpace = findFreeMemoryLoc(RAM);

	if(pageCount >= 1000){
		printf("Hard drive is full.\n") ;
		return -1;
	}
	else if(freeSpace == -1){
		if(DEBUG) printf("There is no free space in RAM for this page.\n");
		evict(RAM);
		freeSpace = findFreeMemoryLoc(RAM);
	}
	int i ;
	for(i = 0 ; i < 1000 ; i++)
		if(pageTable[i].occupied == 0 && pageTable[i].physicalAddress == -1)
			return setupPage(i,RAM,freeSpace);
	return -1;
}

int *get_value_safe(vAddr address){	
	if(DEBUG) printf("Searching for value at address: %d\n", address);
	if(pageTable[address].occupied == 0){
		printf("Page doesn't exist for this address.\n");
		return;
	}
	else{
		int physicalAddress = pageTable[address].physicalAddress;
		int memoryType = pageTable[address].memoryType;
		pageTable[address].lock = 1;

		if(memoryType == RAM){
			if(DEBUG) printf("Value found: %d\n",ram[physicalAddress]);
			return &ram[physicalAddress];
		}
		else{
			handlePageFault(address) ;
			int *value = get_value(address);
			if(DEBUG) printf("Value found: %d\n",*value);
			return value;
		}
	}	
	return NULL;
}

int *get_value(vAddr address){
	sem_wait(&(pageTable[address].lock));
	int *value = get_value_safe(address);
	sem_post(&(pageTable[address].lock));
	return tmp;
}

void store_value_safe(vAddr address, int *value){
	int* valueOfAddress = get_value(address);
	*valueOfAddress = *value;
}

void store_value(vAddr address, int *value){
	sem_wait(&(pageTable[address].lock));
	store_value_safe(address, value);
	sem_wait(&(pageTable[address].lock));
}

void free_page(vAddr address){
	setupEmptyPage(address);
}

//------------------------------

void memoryMaxer() {
	vAddr indexes[1000];
	int i;
	for (i = 0; i < 1000; ++i) {
		if(DEBUG) printf("\n\n----------\n");
		
		indexes[i] = create_page();
		if(DEBUG) printf("----------\n");
		int *value = get_value(indexes[i]);
		if(DEBUG) printf("----------\n");
		*value = (i * 3);
		store_value(indexes[i], value);
	}
	for (i = 0; i < 1000; ++i) {
		free_page(indexes[i]);
	}
}

int main(int argc, char *argv[] ){
	if(argc == 2){
		if(atoi(argv[1]) == 1)
			DEBUG = 1;
		else
			DEBUG = 0;
	}
	
	pageCount = 0;
	
	int i;
	for(i = 0 ; i < 1000 ; i++)
		setupEmptyPage(i);
		
	memoryMaxer();
}

