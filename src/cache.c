//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <stdio.h>

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

int blockOffset_bits;     //number of bits for blockoffset in each block

//number of bits for index of set in each block
int i_setIndex_bits;      
int d_setIndex_bits;
int l2_setIndex_bits;

//the mask used to obtain index of set from address
uint32_t i_setIndex_mask; 
uint32_t d_setIndex_mask;
uint32_t l2_setIndex_mask;

//block structure of a block
struct cacheBlock{
	_Bool valid;          //whether this block is valid
	uint32_t LRU_val;     //LRU value used to perform the LRU replacement
	uint32_t tag;         //tag of each block
};

//pointer of each cache
struct cacheBlock** icache;
struct cacheBlock** dcache;
struct cacheBlock** l2cache;

//pointer of available table of each set of each cache
uint32_t* icache_set_avail;
uint32_t* dcache_set_avail;
uint32_t* l2cache_set_avail;


//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void init_cache(){
	// Initialize cache stats
	icacheRefs        = 0;
	icacheMisses      = 0;
	icachePenalties   = 0;
	dcacheRefs        = 0;
	dcacheMisses      = 0;
	dcachePenalties   = 0;
	l2cacheRefs       = 0;
	l2cacheMisses     = 0;
	l2cachePenalties  = 0;
	
	//Compute number of bits for blockoffset
	blockOffset_bits=INTlog2(blocksize);

	//Initiate icache
	if(icacheSets!=0){
		i_setIndex_bits=INTlog2(icacheSets);
		i_setIndex_mask=icacheSets-1;

		icache=(struct cacheBlock**)malloc(icacheSets*sizeof(struct cacheBlock*));    //allocate memory for all sets
		icache_set_avail=(uint32_t*)malloc(icacheSets*sizeof(uint32_t));    //allocate memory for available table

		for(int i=0;i<icacheSets;i++){
			icache[i]=(struct cacheBlock*)malloc(icacheAssoc*sizeof(struct cacheBlock));  //allocate memory for all blocks in a set
			icache_set_avail[i]=icacheAssoc;    //Initiate the available blocks in a set
			for(int j=0;j<icacheAssoc;j++){
				struct cacheBlock tmp={tag:0,valid:0,LRU_val:0};  //Initiate each block
				icache[i][j]=tmp;
			}
		}
	}
	//Initiate dcache
	if(dcacheSets!=0){
		d_setIndex_bits=INTlog2(dcacheSets);
		d_setIndex_mask=dcacheSets-1;

		dcache=(struct cacheBlock**)malloc(dcacheSets*sizeof(struct cacheBlock*));
		dcache_set_avail=(uint32_t*)malloc(dcacheSets*sizeof(uint32_t));

		for(int i=0;i<dcacheSets;i++){
			dcache[i]=(struct cacheBlock*)malloc(dcacheAssoc*sizeof(struct cacheBlock));
			dcache_set_avail[i]=dcacheAssoc;
			for(int j=0;j<dcacheAssoc;j++){
				struct cacheBlock tmp={tag:0,valid:0,LRU_val:0};
				dcache[i][j]=tmp;
			}
		}
	}
	//Initiate l2cache
	if(l2cacheSets!=0){
		l2_setIndex_bits=INTlog2(l2cacheSets);
		l2_setIndex_mask=l2cacheSets-1;

		l2cache=(struct cacheBlock**)malloc(l2cacheSets*sizeof(struct cacheBlock*));
		l2cache_set_avail=(uint32_t*)malloc(l2cacheSets*sizeof(uint32_t));

		for(int i=0;i<l2cacheSets;i++){
			l2cache[i]=(struct cacheBlock*)malloc(l2cacheAssoc*sizeof(struct cacheBlock));
			l2cache_set_avail[i]=l2cacheAssoc;
			for(int j=0;j<l2cacheAssoc;j++){
				struct cacheBlock tmp={tag:0,valid:0,LRU_val:0};
				l2cache[i][j]=tmp;
			}
		}
	}
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t icache_access(uint32_t addr){
	if(0==icacheSets){    //pass to next level if number of sets is 0
		return l2cache_access(addr);
	}
	icacheRefs+=1;
	uint32_t i_setIndex=(addr>>blockOffset_bits)&i_setIndex_mask;
	uint32_t tag=addr>>(i_setIndex_bits+blockOffset_bits);

	//Search corresponding set to check whether addr is in it
	for(int j=0;j<icacheAssoc;j++){
		if(icache[i_setIndex][j].tag==tag && 1==icache[i_setIndex][j].valid){ //If cache hit
			uint32_t old_LRU_val=icache[i_setIndex][j].LRU_val;  //save the LRU val before update all LRU vals
			for(int k=0;k<icacheAssoc;k++){
				if(icache[i_setIndex][k].LRU_val<old_LRU_val)
					icache[i_setIndex][k].LRU_val+=1; //Update those LRU_val smaller than hit one
				else if(old_LRU_val==icache[i_setIndex][k].LRU_val)
					icache[i_setIndex][k].LRU_val=0;  //Reset the hit one to 0
			}
			return icacheHitTime;
		}
	}

	//If the program doesn't return in loop above, it means this access doen't hit
	icacheMisses+=1;
	uint32_t missPenalty=l2cache_access(addr);
	icachePenalties+=missPenalty;
	//the addr is not in the cache, try to allocate a block for it
	if(icache_set_avail[i_setIndex]>0){
		//If corresponding set is not full
		_Bool allocated=FALSE;
		for(int j=0;j<icacheAssoc;j++){
			if(0==icache[i_setIndex][j].valid && FALSE==allocated){   //allocate the first block that is not valid 
				icache[i_setIndex][j].tag=tag;
				icache[i_setIndex][j].valid=1;
				icache[i_setIndex][j].LRU_val=0;  //It's youngest, so LRU val is 0
				allocated=TRUE;   //to avoid duplicate allocation
				icache_set_avail[i_setIndex]-=1;  //decrease the available number
			}else{
				icache[i_setIndex][j].LRU_val+=1; //Increase the LRU val of existing blocks
			}
		}
	}else{
		//If corresponding set is full
		for(int j=0;j<icacheAssoc;j++){
			if((icacheAssoc-1)!=icache[i_setIndex][j].LRU_val)  //if the block is not the oldest, just increase the LRU val
				icache[i_setIndex][j].LRU_val+=1;
			else{   //replace the block with largest LRU val (the oldest block)
				icache[i_setIndex][j].tag=tag;
				icache[i_setIndex][j].valid=1;
				icache[i_setIndex][j].LRU_val=0;
			}
		}
	}
	return icacheHitTime+missPenalty;  
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t dcache_access(uint32_t addr){
	if(0==dcacheSets){
		return l2cache_access(addr);
	}
	dcacheRefs+=1;
	uint32_t d_setIndex=(addr>>blockOffset_bits)&d_setIndex_mask;
	uint32_t tag=addr>>(d_setIndex_bits+blockOffset_bits);

	//Search cache to check whether addr is in it
	for(int j=0;j<dcacheAssoc;j++){
		if(dcache[d_setIndex][j].tag==tag && 1==dcache[d_setIndex][j].valid){ //If cache hit
			uint32_t old_LRU_val=dcache[d_setIndex][j].LRU_val;
			for(int k=0;k<dcacheAssoc;k++){
				if(dcache[d_setIndex][k].LRU_val<old_LRU_val)
					dcache[d_setIndex][k].LRU_val+=1; //Update those LRU_val smaller than hit one
				else if(old_LRU_val==dcache[d_setIndex][k].LRU_val)
					dcache[d_setIndex][k].LRU_val=0;  //Reset the hit one to 0
			}
			return dcacheHitTime;
		}
	}

	dcacheMisses+=1;
	uint32_t missPenalty=l2cache_access(addr);
	dcachePenalties+=missPenalty;
	//If the addr is not in the cache, try to allocate
	if(dcache_set_avail[d_setIndex]>0){
		//If corresponding set is not full
		_Bool allocated=FALSE;
		for(int j=0;j<dcacheAssoc;j++){
			if(0==dcache[d_setIndex][j].valid && FALSE==allocated){
				dcache[d_setIndex][j].tag=tag;
				dcache[d_setIndex][j].valid=1;
				dcache[d_setIndex][j].LRU_val=0;
				allocated=TRUE;
				dcache_set_avail[d_setIndex]-=1;
			}else{
				dcache[d_setIndex][j].LRU_val+=1; //Increase the LRU val of existing blocks
			}
		}
	}else{
		//If corresponding set is full
		for(int j=0;j<dcacheAssoc;j++){
			if((dcacheAssoc-1)!=dcache[d_setIndex][j].LRU_val)
				dcache[d_setIndex][j].LRU_val+=1;
			else{
				dcache[d_setIndex][j].tag=tag;
				dcache[d_setIndex][j].valid=1;
				dcache[d_setIndex][j].LRU_val=0;
			}
		}
	}
	return dcacheHitTime+missPenalty;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t l2cache_access(uint32_t addr){
	if(0==l2cacheSets){
		return memspeed;
	}
	l2cacheRefs+=1;
	uint32_t l2_setIndex=(addr>>blockOffset_bits)&l2_setIndex_mask;
	uint32_t tag=addr>>(l2_setIndex_bits+blockOffset_bits);

	//Search cache to check whether addr is in it
	for(int j=0;j<l2cacheAssoc;j++){
		if(l2cache[l2_setIndex][j].tag==tag && 1==l2cache[l2_setIndex][j].valid){ //If cache hit
			uint32_t old_LRU_val=l2cache[l2_setIndex][j].LRU_val;
			for(int k=0;k<l2cacheAssoc;k++){
				if(l2cache[l2_setIndex][k].LRU_val<old_LRU_val)
					l2cache[l2_setIndex][k].LRU_val+=1; //Update those LRU_val smaller than hit one
				else if(old_LRU_val==l2cache[l2_setIndex][k].LRU_val)
					l2cache[l2_setIndex][k].LRU_val=0;  //Reset the hit one to 0
			}
			return l2cacheHitTime;
		}
	}

	l2cacheMisses+=1;
	uint32_t missPenalty=memspeed;
	l2cachePenalties+=missPenalty;
	//If the addr is not in the cache, try to allocate
	if(l2cache_set_avail[l2_setIndex]>0){
		//If corresponding set is not full
		_Bool allocated=FALSE;
		for(int j=0;j<l2cacheAssoc;j++){
			if(0==l2cache[l2_setIndex][j].valid && FALSE==allocated){
				l2cache[l2_setIndex][j].tag=tag;
				l2cache[l2_setIndex][j].valid=1;
				l2cache[l2_setIndex][j].LRU_val=0;
				allocated=TRUE;
				l2cache_set_avail[l2_setIndex]-=1;
			}else{
				l2cache[l2_setIndex][j].LRU_val+=1; //Increase the LRU val of existing blocks
			}
		}
	}else{
		//If corresponding set is full
		for(int j=0;j<l2cacheAssoc;j++){
			if((l2cacheAssoc-1)!=l2cache[l2_setIndex][j].LRU_val)
				l2cache[l2_setIndex][j].LRU_val+=1;
			else{
				if(TRUE==inclusive){
					evict_L1(l2cache[l2_setIndex][j].tag,l2_setIndex);
				}
				l2cache[l2_setIndex][j].tag=tag;
				l2cache[l2_setIndex][j].valid=1;
				l2cache[l2_setIndex][j].LRU_val=0;
			}
		}
	}
	return l2cacheHitTime+missPenalty;
}

void evict_L1(uint32_t l2_tag,uint32_t l2_setIndex){
	//Rebuild the original addr
	uint32_t origin_tagANDindex=(l2_tag<<l2_setIndex_bits)^l2_setIndex;
	//Evict L1 i cache
	if(icacheSets>0){
		uint32_t i_setIndex=origin_tagANDindex&i_setIndex_mask;
		uint32_t i_tag=origin_tagANDindex>>i_setIndex_bits;
		for(int j=0;j<icacheAssoc;j++){
			if(i_tag==icache[i_setIndex][j].tag){
				icache[i_setIndex][j].valid=0;
				for(int k=0;k<icacheAssoc;k++){
					if(icache[i_setIndex][k].LRU_val>icache[i_setIndex][j].LRU_val)
						icache[i_setIndex][k].LRU_val-=1;
				}
				icache[i_setIndex][j].LRU_val=icacheAssoc-1;  //Theoratically no need to do this, may cause problem?
				icache_set_avail[i_setIndex]+=1;
			}
		}
	}
	//Evict L1 d cache
	if(dcacheSets>0){
		uint32_t d_setIndex=origin_tagANDindex&d_setIndex_mask;
		uint32_t d_tag=origin_tagANDindex>>d_setIndex_bits;
		for(int j=0;j<dcacheAssoc;j++){
			if(d_tag==dcache[d_setIndex][j].tag){
				dcache[d_setIndex][j].valid=0;
				for(int k=0;k<dcacheAssoc;k++){
					if(dcache[d_setIndex][k].LRU_val>dcache[d_setIndex][j].LRU_val)
						dcache[d_setIndex][k].LRU_val-=1;
				}
				dcache[d_setIndex][j].LRU_val=dcacheAssoc-1;  //Theoratically no need to do this, may cause problem?
				dcache_set_avail[d_setIndex]+=1;
			}
		}
	}

}

int INTlog2(uint32_t num){
	int cnt=0;
	while(num!=1){
		num/=2;
		cnt+=1;
	}
	return cnt;
}
