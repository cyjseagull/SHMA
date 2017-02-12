/** $lic$
 *
 * Copyright (C) 2014 by Adria Armejach <adria.armejach@bsc.es>
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
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NVMAIN_MEM_CTRL_H_
#define NVMAIN_MEM_CTRL_H_
#include <fstream>
#include <set>
#include <string>
#include <cmath>
#include <time.h>
#include <unordered_map>
#include "g_std/g_string.h"
#include "g_std/g_multimap.h"
#include "memory_hierarchy.h"
#include "pad.h"
#include "stats.h"
#include "common/common_functions.h"
#include "locks.h"

#include "tlb/tlb_entry.h"
// NVMain defines MAX and MIN as functions.
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#include "NVM/nvmain.h"
#include "src/Config.h"
#include "src/EventQueue.h"
#include "src/NVMObject.h"
#include "src/SimInterface.h"
#include "NVM/Fine_NVMain/FineNVMain.h"
#include "NVM/Flat_NVMain/FlatNVMain.h"
#include "g_std/g_vector.h"
#include "g_std/g_unordered_set.h"
#define MIN(x, y) ({ __typeof__(x) xx = (x); __typeof__(y) yy = (y); (xx < yy)? xx : yy;})
#define MAX(x, y) ({ __typeof__(x) xx = (x); __typeof__(y) yy = (y); (xx > yy)? xx : yy;})

//using namespace NVM;

class NVMainAccEvent;
class SchedEventNVMain;

typedef std::pair<Address,Address> PAIR;
struct CmpByValue
{
	bool operator()( const PAIR& lhs, const PAIR& rhs)
	{
		return lhs.second > rhs.second;
	}
};

class NVMainMemory : public MemObject, public NVM::NVMObject { //one NVMain controller
    private:
        g_string name;
        uint64_t minLatency;
        uint64_t domain;

        NVM::NVMainRequest *nvmainRetryRequest;
        NVM::NVMain *nvmainPtr;
        NVM::SimInterface *nvmainSimInterface;
        NVM::Config *nvmainConfig;
        NVM::EventQueue *nvmainEventQueue;
        NVM::Stats *nvmainStatsPtr;
        NVM::GlobalEventQueue *nvmainGlobalEventQueue;
        NVM::TagGenerator *nvmainTagGenerator;

        std::vector<std::pair<NVM::NVMainRequest*, std::pair<NVMainAccEvent*, uint64_t>>> inflightRequests;
        std::unordered_map<uint64_t,uint64_t> memoryHistogram;
		g_vector< g_map<Address,Address > > period_touch_vec;
		g_vector<Address> period_access_vec;
		g_vector<double> last_period_hotness;
		g_vector<int> last_action;

		g_vector< g_map<Address, Address> > period_nvm_touch;
		g_vector<Address> period_nvm_access;
        uint64_t curCycle; //processor cycle, used in callbacks
		uint64_t lastCycle;
		uint64_t adjust_period;
        uint64_t updateCycle; //latest cycle where nvmain was updated
        bool eventDriven;
        bool ignoreData;
		bool evict;

        // R/W stats
        PAD();
        Counter profIssued;
        Counter profReads;
        Counter profWrites;
        Counter profPUTS;
        Counter profPUTX;
        Counter profTotalRdLat;
        Counter profTotalWrLat;
        Counter profMemoryFootprint;
        Counter profMemoryAddresses;
        VectorCounter latencyHist;
        VectorCounter addressReuseHist;
        static const uint64_t BINSIZE = 10, NUMBINS = 100;
        PAD();

        // Stats file name
        const char* nvmainStatsFile;

        // Wave phase information
        SchedEventNVMain* nextSchedEvent;
        NVM::NVMainRequest* nextSchedRequest;
        SchedEventNVMain* eventFreelist;
		uint64_t last_memory_access;
		uint64_t last_memory_access_cycle;
		uint64_t previous_caching;
		std::ofstream fdrc;
		std::ofstream fnvm;
	public:
		uint64_t nvmain_access_count,nvmain_read_access_count,nvmain_write_access_count;
		uint64_t request_complete;
		uint64_t recorder_num;
		uint64_t issued_num;
    public:
        //NVMainMemory(std::string& nvmainTechIni, std::string& outputFile, std::string& traceName, uint32_t capacityMB, uint64_t _minLatency, uint32_t _domain, const g_string& _name);
        NVMainMemory(std::string& nvmainTechIni, std::string& outputFile, std::string& traceName, uint32_t capacityMB, uint64_t _minLatency, uint32_t _domain, const g_string& _name , std::string fetcher_name="BlockFetcher");	
			
        const char* getName() {return name.c_str();}
		virtual NVM::NVMain* GetNVMainPtr(){ return nvmainPtr; }
		
        void initStats(AggregateStat* parentStat);

        // Record accesses
        uint64_t access(MemReq& req);

        // Event-driven simulation (phase 2)
        uint64_t tick(uint64_t cycle);
        void enqueue(NVMainAccEvent* ev, uint64_t cycle);
        void recycleEvent(SchedEventNVMain* ev);

        bool RequestComplete(NVM::NVMainRequest *creq);
        void Cycle(NVM::ncycle_t){};
        void printStats();
		NVM::NVMObject* GetNVMainMemory()
		{
			return nvmainPtr;
		}
		inline NVM::NVMObject* GetFetcher()
		{
			return fetcher;
		}
		static NVM::NVMObject* fetcher;
	private:
		lock_t access_lock;
		NVM::FineNVMain* mm;
		//threshold adjustment algorithm related
		int delta_hit_t, delta_clean_miss_t , delta_dirty_miss_t;
		int64_t previous_benefit;
		ThresAdAction previous_action;
		uint64_t prefetch_time;
		uint64_t adjust_time;
		unsigned t_fast_read;
		unsigned t_slow_read;
		unsigned t_slow_write;
		std::ofstream out;
	private:
		inline void LookupTlb(uint32_t coreId, Address addr , bool isWrite,ExtendTlbEntry* &entry, bool& is_tlb , TLBSearchResult& over_thres);
		inline bool Prefetch( unsigned coreId , Address vpn , Address cycle);

		inline void Climbing( unsigned& threshold, double benefit, 
				double &previous_benefit, int &last_action);

		inline void AdjustThreshold( int &delta_thres, uint64_t hit_time,
						uint64_t clean_miss_time , uint64_t dirty_miss_time,
						uint64_t caching_cycles);

		inline void filter_based_cache( MemReq& req,
		NVMainAccEvent* memEv,uint64_t paddr, bool is_write);
		inline NVMainAccEvent* push_to_main_memory( MemReq& req, uint64_t respCycle);
	
		inline void profiling(NVMainAccEvent* memEv, uint32_t core_id);
		inline void adjust_single_process( uint32_t proc_id ,unsigned &threshold);
		inline void adjust_threshold();
};
#endif  // NVMAIN_MEM_CTRL_H_
