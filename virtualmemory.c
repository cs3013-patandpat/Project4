// Author: Patrick Lebold & Patrick Polley

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "virtualmemory.h"

// Debug flag
int DEBUG;

// Variable definitions
TableEntry pageTable[1000];
int ram[RAM_SIZE];
int ssd[100];
int hd[1000];
int pageCount = 0;

int evictType = 0;
int lastChecked = 0;//used for second chance to get a circular array
int prevVal;

void setupEmptyPage(int i){
	if(DEBUG) printf("Emptying page: %d\n",i);
	pageTable[i].physicalAddress = -1 ;
	pageTable[i].memoryType= -1 ;
	pageTable[i].occupied = 0;
	pageTable[i].r = 0;
	pthread_mutex_init(&pageTable[i].lock,NULL);
	pthread_cond_init(&pageTable[i].condition_variable,NULL);
}

vAddr setupPage(int i, int memoryType, int memoryLoc){
	int finished = 0;
	while(finished == 0){
		if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0 ){
			finished = 1;
		}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
	}
	if(DEBUG) printf("Preparing page: %d\n",i);
	pageTable[i].physicalAddress = memoryLoc;
	pageTable[i].memoryType = memoryType;
	pageTable[i].occupied = 1;
	if(memoryType == RAM){
		usleep(RAM_ACCESS);
		ram[pageTable[i].physicalAddress] = pageTable[i].physicalAddress;
	}
	else if(memoryType == SSD){
		usleep(SSD_ACCESS);
		ssd[pageTable[i].physicalAddress-RAM_SIZE] = pageTable[i].physicalAddress;
	}
	else if(memoryType == HD){
		usleep(HD_ACCESS);
		hd[pageTable[i].physicalAddress-(RAM_SIZE + SSD_SIZE)] = pageTable[i].physicalAddress;
	}
	pthread_mutex_unlock(&(pageTable[i].lock));
	pthread_cond_broadcast(&(pageTable[i].condition_variable));
	return i;
}

//------------------------------

int findFreeMemoryLoc(int memoryType) {
	if(DEBUG) printf("Searching for free memory location in level: %d\n",memoryType);
	int i ;
	if(memoryType == RAM){
		if(DEBUG) printf("Entering RAM\n");
		int freeMemoryLocs[RAM_SIZE];
		for(i=0;i<RAM_SIZE;i++){
			freeMemoryLocs[i]=1;
		}
		for(i=0;i<HD_SIZE;i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0 ){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == RAM){
				prevVal = i;
				freeMemoryLocs[pageTable[i].physicalAddress] = 0;
				i = prevVal;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
		for(i=0;i<RAM_SIZE;i++){
			if(freeMemoryLocs[i] == 1){
				if(DEBUG) printf("Memory found in RAM.\n");
				return i ;
			}
		}
		if(DEBUG) printf("No memory found in RAM.\n");
		return -1;
	}
	else if(memoryType == SSD){
		if(DEBUG) printf("Entering SSD\n");
		int freeMemoryLocs[SSD_SIZE];
		for(i=0;i<SSD_SIZE;i++)
			freeMemoryLocs[i]=1;
		for(i=0;i<HD_SIZE;i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0){
					finished = 1;
				}else 
				pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}

			if(pageTable[i].memoryType == SSD) freeMemoryLocs[pageTable[i].physicalAddress-RAM_SIZE] = 0;
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
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
		for(i=0;i<HD_SIZE;i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == HD)
				freeMemoryLocs[pageTable[i].physicalAddress-(RAM_SIZE + SSD_SIZE)] = 0;
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
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

void evictOne(int memoryType, int freeSpace){
	//find the first slot of memory where you can evict
	int i;
	if(memoryType == RAM){
		for(i = 0; i < 1000; i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == RAM){
				if(DEBUG) printf("RAM eviction successful.\n");
				usleep(SSD_ACCESS + RAM_ACCESS);
				ssd[freeSpace] = ram[pageTable[i].physicalAddress];
				usleep(RAM_ACCESS);
				ram[pageTable[i].physicalAddress] = -1;
				pthread_mutex_unlock(&(pageTable[i].lock));
				setupPage(i,SSD,freeSpace + RAM_SIZE);
				return;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}

	}
	else if (memoryType == SSD){
		for(i = 0; i < 1000; i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == SSD){
				if(DEBUG) printf("SSD eviction successful.\n");
				usleep(HD_ACCESS + SSD_ACCESS);
				hd[freeSpace] = ssd[pageTable[i].physicalAddress-RAM_SIZE];
				usleep(SSD_ACCESS);
				ssd[pageTable[i].physicalAddress-RAM_SIZE] = -1;
				pthread_mutex_unlock(&(pageTable[i].lock));
				setupPage(i,HD,freeSpace + (RAM_SIZE + SSD_SIZE));
				return;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
	}
}

void evictTwo(int memoryType, int freeSpace){
	//find a random slot of memory to evict
	int i;
	if(memoryType == RAM){
		int ram_found = 0;
		TableEntry pagesInRam[RAM_SIZE];
		int tableEntry[RAM_SIZE];
		for(i = 0; i < 1000; i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0 ){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == RAM){
				pagesInRam[ram_found] = pageTable[i];
				tableEntry[ram_found] = i;
				ram_found++;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
		if(ram_found == 0) return;//no ram was found
		int evictThis = rand() %(ram_found);
		if(DEBUG) printf("RAM eviction successful.\n");
		usleep(SSD_ACCESS + RAM_ACCESS);
		ssd[freeSpace] = ram[pagesInRam[evictThis].physicalAddress];
		usleep(RAM_ACCESS);
		ram[pagesInRam[evictThis].physicalAddress] = -1;
		setupPage(tableEntry[evictThis],SSD,freeSpace + RAM_SIZE);
	}
	else if (memoryType == SSD){
		int ssd_found = 0;
		TableEntry pagesInSSD[100];
		int tableEntry[100];
		for(i = 0; i < 1000; i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0 ){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == SSD){
				pagesInSSD[ssd_found] = pageTable[i];
				tableEntry[ssd_found] = i;
				ssd_found++;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
		if(ssd_found == 0) return;//no SSD was found
		int evictThis = rand() %(ssd_found);
		if(DEBUG) printf("SSD eviction successful.\n");
		usleep(SSD_ACCESS + HD_ACCESS);
		hd[freeSpace] = ssd[pagesInSSD[evictThis].physicalAddress - RAM_SIZE];
		usleep(SSD_ACCESS);
		ssd[pagesInSSD[evictThis].physicalAddress - RAM_SIZE] = -1;
		setupPage(tableEntry[evictThis],HD,freeSpace + (RAM_SIZE + SSD_SIZE));
	}
}

void evictThree(int memoryType, int freeSpace){
	//second chance eviction
	int i;
	int j = 0;
	if(memoryType == RAM){
		for(i = lastChecked; j < 1000; j++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0 ){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == RAM){
				if(pageTable[i].r == 0){
					lastChecked = i + 1;
					if(DEBUG) printf("RAM eviction successful.\n");
					usleep(SSD_ACCESS + RAM_ACCESS);
					ssd[freeSpace] = ram[pageTable[i].physicalAddress];
					usleep(RAM_ACCESS);
					ram[pageTable[i].physicalAddress] = -1;
					pthread_mutex_unlock(&(pageTable[i].lock));
					setupPage(i,SSD,freeSpace + RAM_SIZE);
					return;
				}else pageTable[i].r = 0;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
			i++;
			if(i == 1000) i = 0;
		}

	}
	else if (memoryType == SSD){
		for(i = 0; i < 1000; i++){
			int finished = 0;
			while(finished == 0){
				if(pthread_mutex_trylock(&(pageTable[i].lock)) == 0){
					finished = 1;
				}else pthread_cond_wait(&(pageTable[i].condition_variable),&(pageTable[i].lock));
			}
			if(pageTable[i].memoryType == SSD){
				if(DEBUG) printf("SSD eviction successful.\n");
				usleep(HD_ACCESS + SSD_ACCESS);
				hd[freeSpace] = ssd[pageTable[i].physicalAddress-25];
				usleep(SSD_ACCESS);
				ssd[pageTable[i].physicalAddress-25] = -1;
				pthread_mutex_unlock(&(pageTable[i].lock));
				setupPage(i,HD,freeSpace + 125);
				return;
			}
			pthread_mutex_unlock(&(pageTable[i].lock));
			pthread_cond_broadcast(&(pageTable[i].condition_variable));
		}
	}
}

void evict(int memoryType){
	if(DEBUG) printf("Evicting memory in level: %d\n",memoryType);
	int freeSpace;
	if(memoryType == RAM){
		freeSpace = findFreeMemoryLoc(SSD);
		if(freeSpace == -1){
			if(DEBUG) printf("No space available in SSD. Attempting to clear space.\n");
			evict(SSD);
			freeSpace = findFreeMemoryLoc(SSD);
		}
	}
	else if(memoryType == SSD){
		freeSpace = findFreeMemoryLoc(HD);
		if(freeSpace == -1){
			if(DEBUG) printf("Hard drive is full. Lossless eviction not possible.\n");
			return;
		}
	}
	if(evictType == 0) 
		evictOne(memoryType, freeSpace);//first page found is evicted
	else if(evictType == 1) {
		evictTwo(memoryType, freeSpace);//randomly pick a page to evict
}
	else if(evictType == 2) 
		evictThree(memoryType, freeSpace);//use second chance algorithm to pick a page to evict
	else 
		evictThree(memoryType, freeSpace);//default to second chance
}

void handlePageFault(vAddr address){
	int finished = 0;
	while(finished == 0){
		if(pthread_mutex_trylock(&(pageTable[address].lock)) == 0){
			finished = 1;
		}else pthread_cond_wait(&(pageTable[address].condition_variable),&(pageTable[address].lock));
	}
	if(DEBUG) printf("Handling page fault: %d\n",address);
	int physicalAddress = pageTable[address].physicalAddress;
	int memoryType = pageTable[address].memoryType;
	
	if(memoryType == SSD){
		if(DEBUG) printf("Bumping memory cell from SSD -> RAM\n");
		int freeSpace = findFreeMemoryLoc(RAM);
		usleep(SSD_ACCESS);
		int value = ssd[physicalAddress-RAM_SIZE];
		if(freeSpace == -1){
			evict(RAM);
			freeSpace = findFreeMemoryLoc(RAM);
		}
		usleep(RAM_ACCESS);
		ram[freeSpace] = value;
		pageTable[address].physicalAddress = freeSpace;
		pageTable[address].memoryType = RAM;
	}
	else if(memoryType == HD){
		if(DEBUG) printf("Bumping memory cell from HD -> SSD\n");
		int freeSpace = findFreeMemoryLoc(SSD);
		usleep(HD_ACCESS);
		int value = hd[physicalAddress-(RAM_SIZE + SSD_SIZE)];
		if(freeSpace == -1){
			evict(SSD);
			freeSpace = findFreeMemoryLoc(SSD);
			if(freeSpace == -1){
				printf("[SEVERE] Page fault error\n");
				pthread_mutex_unlock(&(pageTable[address].lock));
				pthread_cond_broadcast(&(pageTable[address].condition_variable));
				return;
			}
		}
		usleep(SSD_ACCESS);
		ssd[freeSpace] = value;
		pageTable[address].physicalAddress = freeSpace + RAM_SIZE;
		pageTable[address].memoryType = SSD;
		handlePageFault(address);
	}
	pthread_mutex_unlock(&(pageTable[address].lock));
	pthread_cond_broadcast(&(pageTable[address].condition_variable));
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
		if(pageTable[i].occupied == 0 && pageTable[i].physicalAddress == -1){
			pageCount++;
			return setupPage(i,RAM,freeSpace);
		}
	return -1;
}

int *get_value_safe(vAddr address){
	if(DEBUG) printf("Searching for value at address: %d\n", address);
	if(pageTable[address].occupied == 0){
		if(DEBUG) printf("Page doesn't exist for this address.\n");
		return NULL;
	}
	else{
		int physicalAddress = pageTable[address].physicalAddress;
		int memoryType = pageTable[address].memoryType;
		pageTable[address].r = 1;

		if(memoryType == RAM){
			if(DEBUG) printf("Value found: %d\n",ram[physicalAddress]);
			usleep(RAM_ACCESS);
			return &ram[physicalAddress];
		}
		else{
			handlePageFault(address) ;
			int *value = get_value_safe(address);
			if(DEBUG) printf("Value found: %d\n",*value);
			return value;
		}
	}	
	return NULL;
}

int *get_value(vAddr address){
	if(address < 0 || address >= 1000){
		return NULL;
	}
	int finished = 0;
	int *value;
	while(finished == 0){
		if(pthread_mutex_trylock(&(pageTable[address].lock)) == 0){
			int *value = get_value_safe(address);
			pthread_mutex_unlock(&(pageTable[address].lock));
			pthread_cond_broadcast(&(pageTable[address].condition_variable));
			finished = 1;
			return value;
		}
		else{
			pthread_cond_wait(&(pageTable[address].condition_variable),&(pageTable[address].lock));
		}
	}
}

void store_value_safe(vAddr address, int *value){
	int* valueOfAddress = get_value_safe(address);
	*valueOfAddress = *value;
}

void store_value(vAddr address, int *value){
	if(address < 0 || address >= 1000)
		return;
	int finished = 0;
	while(finished == 0){
		if(pthread_mutex_trylock(&(pageTable[address].lock)) == 0){
			store_value_safe(address, value);
			pthread_mutex_unlock(&(pageTable[address].lock));
			pthread_cond_broadcast(&(pageTable[address].condition_variable));
			finished = 1;
			return;
		}
		else{
			pthread_cond_wait(&(pageTable[address].condition_variable),&(pageTable[address].lock));
		}
	}
}

void free_page(vAddr address){
	int finished = 0;
	while(finished == 0){
		if(pthread_mutex_trylock(&(pageTable[address].lock)) == 0){
			setupEmptyPage(address);
			pthread_mutex_unlock(&(pageTable[address].lock));
			pthread_cond_broadcast(&(pageTable[address].condition_variable));
			finished = 1;
			return;
		}
		else{
			pthread_cond_wait(&(pageTable[address].condition_variable),&(pageTable[address].lock));
		}
	}
}

//------------------------------

void threadCall(int *threadID){
	int i;
	for(i=0; i<300; i++){
		if(DEBUG) printf("Thread %d iteration %d\n",*threadID,i);
		int address = rand()%1000;
		int *value = get_value(address);
		*value = rand();
		store_value(address, value);
		
		if(rand()%25 == 0)
			free_page(rand()%1000); //Free random address
		if(rand()%60 == 0)
			create_page();
	}
}

void multithreadedHavoc(){
	pthread_t threads[20];
	
	int i;
	for (i = 0; i < 1000; ++i) {
		if(DEBUG) printf("Making page # %d.\n",i);
		if(DEBUG) printf("Pagecount is: %d.\n",pageCount);
		if(DEBUG) printf("\n\n----------\n");
		
		vAddr address = create_page();
		if(DEBUG) printf("------------\n");
		int *value = get_value(address);
		if(DEBUG) printf("-------------\n");
		*value = (i * 3);
		store_value(address, value);
	}
	
	for(i=0; i<20; i++){
		if(DEBUG) printf("Spawning literal demon thread\n");
		int* id = malloc(sizeof(int));
		*id = i;
		pthread_create (&(threads[i]),NULL, (void *) &threadCall, id);
	}
}

void memoryMaxer() {
	vAddr indexes[1000];
	int i;
	for (i = 0; i < 1000; ++i) {
		if(DEBUG) printf("Making page # %d.\n",i);
		if(DEBUG) printf("Pagecount is: %d.\n",pageCount);
		if(DEBUG) printf("\n\n----------\n");
		
		indexes[i] = create_page();
		if(DEBUG) printf("------------\n");
		int *value = get_value(indexes[i]);
		if(DEBUG) printf("-------------\n");
		*value = (i * 3);
		store_value(indexes[i], value);
	}
	for (i = 0; i < 1000; ++i) {
		free_page(indexes[i]);
	}
}

int main(int argc, char *argv[] ){
	srand(time(NULL));
	//set debug on or off
	if(argc > 1){
		if(atoi(argv[1]) == 1)
			DEBUG = 1;
		else
			DEBUG = 0;
	}
	//set evict type
	if(argc == 3){
		evictType = atoi(argv[2]);
		if(evictType > 2 || evictType < 0) evictType = rand()%3;
	}else evictType = 0;
	
	printf("Evicting using %d.\n",evictType);
	
	pageCount = 0;
	
	int i;
	for(i = 0 ; i < 1000 ; i++)
		setupEmptyPage(i);
		
	memoryMaxer();
	multithreadedHavoc();
}

