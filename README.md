# Cache Simulator Project

## Table of Contents
  * [Introduction](#introduction)
  * [Traces](#traces)
  * [Acknowledgment](#acknowledgment)
  * [Running Cache Simulator](#running-cache-simulator)
  * [Implementation Detail](#implementation-detail)
    - [Configuration](#configuration)
    - [Inclusion](#inclusion)
    - [Uninstantiated Caches](#uninstantiated-caches)
    - [Replacement Policy](#replacement-policy)
    - [Statistics](#statistics)


## Introduction

This is a cache simulator project of graduate computer architecture course at UCSD. If you are currently taking graduate computer architecture at UCSD and doing this project, for the academic integrity consideration, please don't check and use codes in this repository.

This project simulates the perfomance of L1, L2 cache hierachy. 

## Acknowledgment

Traces and start code are provided by TA of this courses: Prannoy Pilligundla.

## Traces

Traces of real programs are included to test the implementation of cache simulator.
Each line in the trace file contains the address of a memory access in hex as well as where the access should be directed, either to the I$ (I) or D$ (D).

There are one full real-program trace (~200M memory references), and one smaller (20M references).

```
<Address> <I or D>

Sample Trace from tsman.bz2:
0x648 I
0x64c I
0x650 I
0x654 I
0x658 I
0x40868 D
0x65c I
0x660 I
0x664 I
0x668 I
```

Correct outputs of each trace in five different memory configurations are also included here for debugging and test the implementation.

## Running Cache Simulator

To build the simulator, simply run `make` in the src/
directory of the project.  To run the program on an uncompressed trace, use following command:

`./cache <options> [<trace>]`

Some of the traces are rather large when uncompressed, to run the program on an compressed trace, use following command:

`bunzip2 -kc trace.bz2 | ./cache <options>`

If no trace file is provided then the simulator will read in input from STDIN.

In either case the options that can be used to change the configurations of
the memory hierarchy are as follows:

```
  --help                     Print this message
  --icache=sets:assoc:hit    I-cache Parameters
  --dcache=sets:assoc:hit    D-cache Parameters
  --l2cache=sets:assoc:hit   L2-cache Parameters
  --inclusive                Makes L2-cache be inclusive
  --blocksize=size           Block/Line size
  --memspeed=latency         Latency to Main Memory
```

## Implementation Detail

There are 4 methods implemented in the cache.c file: **init_cache**, **icache_access**, **dcache_access**, **l2cache_access**.

`void init_cache();`

All data structures needed for the implementaion are initialized here. This will be run before any memory accesses are provided to the simulator.


```
uint32_t icache_access(uint32_t addr);
uint32_t dcache_access(uint32_t addr);
uint32_t l2cache_access(uint32_t addr);
```

These 3 functions are the interface to the instruction, data, and l2 caches respectively.  Input is the address of a memory access to perform and expected to return the time it took to perform the access in cycles. Only icache_access and dcache_access will be called from main.c, any calls to l2cache_access are done in the instruction and data cache access when there is a miss.

### Configuration

```
  [cache]Sets       // Number of sets in the cache
  [cache]Assoc      // Associativity of the cache
  [cache]HitTime    // Hit Time of the cache in cycles
  blocksize         // The Block or Line size
  inclusive         // Indicates if the L2 is inclusive
  memspeed          // Latency to Main Memory
```

Each cache can be configured to have a different number of Sets, Associativity and Hit Time.  Additionally the block size of the memory system can be configured.  The **I$**, **D$**, and **L2$** all have the same block size.  The L2 cache can be configured to be inclusive. You are also able to set the latency of main memory.

### Inclusion

If the L2 is configured to be inclusive, then all valid lines in the D$ and I$ must be present in the L2.  This means if a line is evicted from the L2 then an invalidation must be sent to both the data cache as well as the instruction cache. If the L2 is not inclusive then this restriction doesn't hold and the D$ and I$ could potentially hold valid lines not present in the L2.  Generally you should observe worse cache performance when the L2 is configured to be inclusive.  Why do you think that is?

### Uninstantiated Caches

If the number of sets of a cache is set to 0 then that cache won't be instantiated for the simulation. This means that all accesses which are directed to it should just be passed through to the next level in the memory hierarchy. If a cache is uninstantiated then no statistics need to be
collected for that cache.

### Replacement Policy

When evicting lines from a cache, in order to free space for a new line, you
should select a victim using the **LRU replacement policy**.

### Statistics

```
  [cache]Refs        // cache references (total # of accesses)
  [cache]Misses      // cache misses
  [cache]Penalties   // cache penalties
```

In addition to modeling the memory hierarchy, a number of statistics for each cache in the hierarchy are also maintained in this simulator. These statistics will be
used to calculate the miss rate and average access time for each cache. Keeping track of references and misses is self explanatory.  The Penalties statistic will keep track of the total penalty (in cycles) for a simulation.  A
penalty is defined as any **extra** overhead incurred by accessing the cache beyond the hit time.  This means that the penalty does not take into account the latency of an access due to a cache hit.  

As an example, let's say I have an I$ with a 2 cycle hit latency.  If I have an access which hits in the cache, no penalty is observed and I will return a 2 cycle access time.  On the other hand if I have an access which misses the I$ and is passed up to the L2 then the penalty which the I$ observes is the L2 access time.  For this access I will return the 2 cycle hit time plus the additional penalty as the I$ access time.

This means that the access time that cache access functions return are the Hit Time plus any additional penalty observed.

### Test Cases

Memory hierachy configurations of several real hardwares are tested here, along with their technical reference manuals wherever applicable.

1. **Intel Pentium III** - [Reference Manual](http://download.intel.com/design/PentiumIII/datashts/24526408.pdf):
   * I$: 16KB, direct-mapped, 2 cycles hit latency
   * D$: 16KB, direct-mapped, 2 cycles hit latency
   * L2: 256KB, 8-way, on-chip, 10 cycles hit latency, inclusive
   * 64B block size
   * `./cache --icache=256:1:2 --dcache=256:1:2 --l2cache=512:8:10 --blocksize=64 --memspeed=100 --inclusive`

2. **ARM Cortex-A32** - [Reference Manual](https://static.docs.arm.com/100241/0001/cortex_a32_trm_100241_0001_00_en.pdf):
   * I$: 16KB, 2-way, 2 cycles hit latency
   * D$: 32KB, 4-way, 2 cycles hit latency
   * L2: 128KB, 8-way, on-chip, 10 cycles hit latency, non-inclusive
   * 64B block size
   * `./cache --icache=128:2:2 --dcache=128:4:2 --l2cache=256:8:10 --blocksize=64 --memspeed=100`

3. **MIPS R10K** - [Slides](https://web.stanford.edu/class/ee282h/handouts/Handout36.pdf):
   * I$: 32KB, 2-way, 2 cycles hit latency
   * D$: 32KB, 4-way, 2 cycles hit latency
   * L2: 128KB, 8-way, off-chip, 50 cycles hit latency, inclusive
   * 128B block size
   * `./cache --icache=128:2:2 --dcache=64:4:2 --l2cache=128:8:50 --blocksize=128 --memspeed=100 --inclusive`

4. **Alpha A21264** - [Reference Manual](http://www.ece.cmu.edu/~ece447/s13/lib/exe/fetch.php?media=21264hrm.pdf):
   * I$: 64KB, 2-way, 2 cycles hit latency
   * D$: 64KB, 4-way, 2 cycles hit latency
   * L2: 8MB, direct-mapped, off-chip, 50 cycles hit latency, inclusive
   * 64B block size
   * `./cache --icache=512:2:2 --dcache=256:4:2 --l2cache=16384:8:50 --blocksize=64 --memspeed=100 --inclusive`

5. **Specialized hardware:** Open-source FPGA design of a Bitcoin Miner (suspected memory hierarchy)
   * [Verilog Source Code - Github] (https://github.com/progranism/Open-Source-FPGA-Bitcoin-Miner)
   * Bitcoin mining is almost entirely compute-bound and makes very little use of memory. This is what makes it a great algorithm to map on an FPGA/ASIC. As a result, there are no caches, just a tiny main memory, which we will model as a L2 off-chip cache.
   * BTC miners are definitely not designed for general-purpose computing like the other entries in this list, however we will evaluate its memory structure as if it was part of a general-purpose processor.
   * I$: Uninstantiated
   * D$: Uninstantiated
   * L2: 1KB, direct-mapped, off-chip, 50 cycles hit latency
   * 128B block size
   * `./cache --icache=0:0:0 --dcache=0:0:0 --l2cache=8:1:50 --blocksize=128 --memspeed=100`
