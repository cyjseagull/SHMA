/** $lic$
 * Copyright (C) 2012-2014 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZSIM_H_
#define ZSIM_H_

#include <stdint.h>
#include <sys/time.h>
#include <map>
#include <string>

#include "constants.h"
#include "debug.h"
#include "locks.h"
#include "pad.h"
#include "g_std/g_vector.h"
#include "g_std/g_multimap.h"
#include "common/global_const.h"
#include "src/NVMObject.h"

//forward declaration
class Core;
class MemObject;
class Scheduler;
class AggregateStat;
class StatsBackend;
class ProcessTreeNode;
class ProcessStats;
class EventQueue;
class ContentionSim;
class EventRecorder;
class PinCmd;
class PortVirtualizer;
class VectorCounter;
class BaseDRAMBufferManager;
class BasePageTableWalker;
class BaseDRAMBufferManager;
class BaseTlb;
class BuddyAllocator;
class MemoryNode;
class BasePaging;
struct ClockDomainInfo {
    uint64_t realtimeOffsetNs;
    uint64_t monotonicOffsetNs;
    uint64_t processOffsetNs;
    uint64_t rdtscOffset;
    lock_t lock;
};

class TimeBreakdownStat;
enum ProfileStates {
    PROF_INIT = 0,
    PROF_BOUND = 1,
    PROF_WEAVE = 2,
    PROF_FF = 3,
};

enum ProcExitStatus {
    PROC_RUNNING = 0,
    PROC_EXITED = 1,
    PROC_RESTARTME  = 2
};

struct GlobSimInfo {
    //System configuration values, all read-only, set at initialization
    uint32_t numCores;
    uint32_t lineSize;

    //Cores
    Core** cores;
    PAD();

    EventQueue* eventQueue;
    Scheduler* sched;

    //Contention simulation
    uint32_t numDomains;
    ContentionSim* contentionSim;
    EventRecorder** eventRecorders; //CID->EventRecorder* array

    PAD();

    //World-readable
    uint32_t phaseLength;
    uint32_t statsPhaseInterval;
    uint32_t freqMHz;

    //Maxima/termination conditions
    uint64_t maxPhases; //terminate when this many phases have been reached
    uint64_t maxMinInstrs; //terminate when all threads have reached this many instructions
    uint64_t maxTotalInstrs; //terminate when the aggregate number of instructions reaches this number
    uint64_t maxSimTimeNs; //terminate when the simulation time (bound+weave) exceeds this many ns
    uint64_t maxProcEventualDumps; //term if the number of heartbeat-triggered process dumps reached this (MP/MT)

    bool ignoreHooks;

    bool blockingSyscalls;
    bool perProcessCpuEnum; //if true, cpus are enumerated according to per-process masks (e.g., a 16-core mask in a 64-core sim sees 16 cores)
    bool oooDecode; //if true, Decoder does OOO (instr->uop) decoding
    bool addressRandomization; //if true, randomize address bits for multiprocesses runs

    PAD();

    //Writable, rarely read, unshared in a single phase
    uint64_t numPhases;
    uint64_t globPhaseCycles; //just numPhases*phaseCycles. It behooves us to precompute it, since it is very frequently used in tracing code.

    uint64_t procEventualDumps;

    PAD();

    ClockDomainInfo clockDomainInfo[MAX_CLOCK_DOMAINS];
    PortVirtualizer* portVirt[MAX_PORT_DOMAINS];

    lock_t ffLock; //global, grabbed in all ff entry/exit ops.
    lock_t cidLock; //global, grabbed in all ff entry/exit ops.
	//lock_t pg_walker_lock[];

    volatile uint32_t globalActiveProcs; //used for termination
    //Counters below are used for deadlock detection
    volatile uint32_t globalSyncedFFProcs; //count of processes that are in synced FF
    volatile uint32_t globalFFProcs; //count of processes that are in either synced or unsynced FF

    volatile bool terminationConditionMet;

    const char* outputDir; //all the output files mst be dumped here. Stored because complex workloads often change dir, then spawn...

    AggregateStat* rootStat;
    StatsBackend* periodicStatsBackend;
    StatsBackend* statsBackend; //end-of-sim backend
    StatsBackend* eventualStatsBackend;
    StatsBackend* compactStatsBackend;
    ProcessStats* processStats;

    TimeBreakdownStat* profSimTime;
    VectorCounter* profHeartbeats; //global b/c number of processes cannot be inferred at init time; we just size to max

    uint64_t trigger; //code with what triggered the current stats dump

    ProcessTreeNode* procTree;
    ProcessTreeNode** procArray; //a flat view of the process tree, where each process is indexed by procIdx
    ProcExitStatus* procExited; //starts with all set to PROC_RUNNING, each process sets to PROC_EXITED or PROC_RESTARTME on exit. Used to detect untimely deaths (that don;t go thropugh SimEnd) in the harness and abort.
    uint32_t numProcs;
    uint32_t numProcGroups;

    PinCmd* pinCmd; //enables calls to exec() to modify Pin's calling arguments, see zsim.cpp

    // If true, threads start as shadow and have no effect on simulation until they call the register magic op
    bool registerThreads;

    //If true, do not output vectors in stats -- they're bulky and we barely need them
    bool skipStatsVectors;

    //If true, all the regular aggregate stats are summed before dumped, e.g. getting one thread record with instrs&cycles for all the threads
    bool compactPeriodicStats;

    bool attachDebugger;
    int harnessPid; //used for debugging purposes

    struct LibInfo libzsimAddrs;

    bool ffReinstrument; //true if we should reinstrument on ffwd, works fine with ST apps and it's faster since we run with basically no instrumentation, but it's not precise with MT apps

    //fftoggle stuff
    lock_t ffToggleLocks[256]; //f*ing Pin and its f*ing inability to handle external signals...
    lock_t pauseLocks[256]; //per-process pauses
    volatile bool globalPauseFlag; //if set, pauses simulation on phase end
    volatile bool externalTermPending;

    // NVMain
    bool hasNVMain;
    bool hasDRAMCache;
    uint32_t numMemoryControllers;
    g_vector<MemObject*> memoryControllers;
	//NVM::NVMObject* fetcher;
	/**-----added by YuJieChen , TLB and memory management related-----**/
	uint64_t memory_size;
	uint64_t max_mem_page_no;
	MemoryNode* memory_node;
	uint64_t page_size;
	uint64_t page_shift;
	BuddyAllocator* buddy_allocator;
	int percpu_pagelist_fraction;
	//record max page number of zone
	uint64_t max_zone_pfns[MAX_NR_ZONES];
	int sysctl_lowmem_reserve_ratio[MAX_NR_ZONES-1];
	/*####DRAM buffer related####*/
	uint64_t buffer_size;
	Address high_addr;
	g_vector<unsigned> access_threshold;
	//unsigned access_threshold;	 //threshold to cache pcm block into dram buffer
	uint64_t adjust_interval;
	unsigned block_shift;
	unsigned block_size;
	BaseDRAMBufferManager* dram_manager;
	unsigned read_incre_step;
	unsigned write_incre_step;
	unsigned life_time;
	BasePaging** paging_array;
	BasePageTableWalker** pg_walkers;
	/*####tlb related #####*/
	//tlb["itlb"]:instruction tlb
	//tlb["dtlb"]:data tlb
	bool counter_tlb;
	bool prefetch_set;
	BasePageTableWalker* page_table_walker;
	PagingStyle paging_mode;
	EVICTSTYLE ins_evict_policy;
	EVICTSTYLE data_evict_policy;
	DRAMEVICTSTYLE dram_evict_policy;
	bool dynamic_threshold; 
	bool proc_fairness;
	bool multi_queue;
};


//Process-wide global variables, defined in zsim.cpp
extern Core* cores[MAX_THREADS]; //tid->core array
extern uint32_t procIdx;
extern bool trace_using;
extern uint32_t lineBits; //process-local for performance, but logically global
extern uint64_t procMask;

extern GlobSimInfo* zinfo;
//GlobSimInfo* zinfo=NULL;

//Process-wide functions, defined in zsim.cpp
uint32_t getCid(uint32_t tid);
uint32_t TakeBarrier(uint32_t tid, uint32_t cid);
void SimEnd(); //only call point out of zsim.cpp should be watchdog threads

#endif  // ZSIM_H_
