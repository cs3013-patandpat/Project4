// Author: Patrick Lebold & Patrick Polley

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "virtualmemory.h"

#define DEBUG 1 //True

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

int findFreeMemoryLoc(int memoryType) {
	if(DEBUG) printf("Searching for free memory location in level: %d\n",memoryType);
	int i ;
	if(memoryType == RAM){
		int freeMemoryLocs[RAM_SIZE];
		for(i=0;i<RAM_SIZE;i++)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;i++)
			if(pageTable[i].memoryType == RAM)
				freeMemoryLocs[pageTable[i].physicalAddress] = 0;
		for(i=0;i<RAM_SIZE;i++)
			if(freeMemoryLocs[i] == 1){
				if(DEBUG) printf("Memory found in RAM.");
				return i ;
			}

		return -1;
	}
	else if(memoryType == SSD){
		int freeMemoryLocs[SSD_SIZE];
		for(i=0;i<SSD_SIZE;++i)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;++i)
			if(pageTable[i].memoryType == SSD)
				freeMemoryLocs[pageTable[i].physicalAddress-25]=0;
		for(i=0;i<SSD_SIZE;++i)
			if(freeMemoryLocs[i] == 1){
				if(DEBUG) printf("Memory found in SSD.");
				return i ;
			}
				
		return -1 ;
	}
	else if(memoryType == HD){
		int freeMemoryLocs[HD_SIZE];
		for(i=0;i<HD_SIZE;++i)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;++i)
			if(pageTable[i].memoryType == HD)
				freeMemoryLocs[pageTable[i].physicalAddress-125] = 0;
		for(i=0;i<HD_SIZE;++i)
			if(freeMemoryLocs[i] == 1) {
				if(DEBUG) printf("Memory found in HD.");
				return i ;
			}
				
		return -1 ;
	}
}

void evict(int memoryType){
	if(DEBUG) printf("Evicting memory in level: %d\n",memoryType);
	int freeSpace;
	if(memoryType == RAM){
		freeSpace = findFreeMemoryLoc(SSD);
		if(freeSpace == -1){
			if(DEBUG) printf("No space available in level: %d\n",memoryType);
			evict(SSD);
		}
	}
	else if(memoryType == SSD){
		freeSpace = findFreeMemoryLoc(HD);
		if(freeSpace == -1){
			printf("Hard drive is full. Lossless eviction not possible.");
			return;
		}
	}
	
	int i;
	for(i = 0; i < 1000; i++){
		if(pageTable[i].memoryType == RAM && memoryType == RAM){
			if(DEBUG) printf("Eviction successful.");
			ssd[freeSpace] = ram[pageTable[i].physicalAddress];
			setupPage(i,SSD,freeSpace + 25);
			usleep(RAM_ACCESS);
			ram[pageTable[i].physicalAddress] = -1;
			return;
		}
		else if(pageTable[i].memoryType == SSD && memoryType == SSD){
			if(DEBUG) printf("Eviction successful.");
			hd[freeSpace] = ssd[pageTable[i].physicalAddress-25];
			setupPage(i,HD,freeSpace + 125);
			usleep(SSD_ACCESS);
			ssd[pageTable[i].physicalAddress-25] = -1;
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

vAddr create_page(){
	if(DEBUG) printf("Creating page.\n");
	int freeSpace = findFreeMemoryLoc(RAM);

	if(pageCount >= 1000){
		printf("Hard drive is full.\n") ;
		return;
	}
	else if(freeSpace == -1){
		evict(RAM);
		freeSpace = findFreeMemoryLoc(RAM);
	}
	int i ;
	for(i = 0 ; i < 1000 ; i++)
		if(pageTable[i].occupied == 0 && pageTable[i].physicalAddress == -1)
			return setupPage(i,RAM,freeSpace);
	return -1;
}

void handlePageFault(vAddr address){
	int physicalAddress = pageTable[address].physicalAddress;
	int memoryType = pageTable[address].memoryType;
	
	if(memoryType == SSD){
		int freeSpace = findFreeMemoryLoc(RAM);
		int value = ssd[physicalAddress-25];
		if(freeSpace == -1){
			evict(RAM);
			freeSpace = findFreeMemoryLoc(RAM);
		}
		ram[freeSpace] = value;
	}
	else if(memoryType == HD){
		int freeSpace = findFreeMemoryLoc(SSD);
		int value = hd[physicalAddress-125];
		if(freeSpace == -1){
			printf("Critical Page Fault Error");
			return;
		}
		ssd[freeSpace] = value;
		handlePageFault(address);
	}
}

int *get_value(vAddr address){	
	if(DEBUG) printf("Searching for value at address: %d\n", address);
	if(pageTable[address].occupied == 0){
		printf("Page doesn't exist for this address.\n");
		return NULL;
	}
	else{
		int physicalAddress = pageTable[address].physicalAddress;
		int memoryType = pageTable[address].memoryType;

		if(memoryType == RAM){
			printf("Value found: %d\n",ram[physicalAddress]);
			return &ram[physicalAddress];
		}
		else{
			handlePageFault(address) ;
			int *value = get_value(address);
			printf("Value found: %d\n",*value);
			return value;
		}
	}	
	return NULL;
}

void store_value(vAddr address, int *value){
	int* valueOfAddress = get_value(address);
	*valueOfAddress = *value;
}

void free_page(vAddr address){
	setupEmptyPage(address);
}

void memoryMaxer() {
	vAddr indexes[1000];
	int i;
	for (i = 0; i < 1000; ++i) {
		indexes[i] = create_page();
		int *value = get_value(indexes[i]);
		*value = (i * 3);
		store_value(indexes[i], value);
	}
	for (i = 0; i < 1000; ++i) {
		free_page(indexes[i]);
	}
}

int main(){
	pageCount = 0;
	
	int i;
	for(i = 0 ; i < 1000 ; i++)
		setupEmptyPage(i);
		
	memoryMaxer();
}

