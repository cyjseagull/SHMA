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


#include <map>
#include <string>
#include <math.h>
#include "galloc.h"
#include "pad.h"
#include "event_recorder.h"
#include "tick_event.h"
#include "timing_event.h"
#include "zsim.h"
#include "nvmain_mem_ctrl.h"
#include "ooo_core.h"

#include "common/common_functions.h"
#include "common/global_const.h"
#include "tlb/hot_monitor_tlb.h"
#include "tlb/page_table_walker.h"
#include "tlb/common_func.h"
#ifdef _WITH_NVMAIN_ //was compiled with nvmain
#include "SimInterface/NullInterface/NullInterface.h"
#include "Utils/HookFactory.h"

#include "Utils/FetcherFactory.h"
#include "NVM/NVMainFactory.h"
#include "zsim.h"

NVM::NVMObject* NVMainMemory::fetcher;

class NVMainAccEvent : public TimingEvent {
    private:
        NVMainMemory* nvram;
        bool write;
        Address addr;
		bool is_buffer_addr;

    public:
        uint64_t sCycle;

        NVMainAccEvent(NVMainMemory* _nvram, bool _write, Address _addr, int32_t domain) :
			TimingEvent(0, 0, domain), nvram(_nvram), write(_write), addr(_addr),
			is_buffer_addr(false) {}
		virtual ~NVMainAccEvent(){}

        bool isWrite() const {
            return write;
        }

        Address getAddr() const {
            return addr;
        }
	
		void setAddr( Address new_addr)
		{
			addr = new_addr;
		}
		void setBufferAddr()
		{ 
			is_buffer_addr = true;
		}

		bool isBufferAddr()
		{
			return is_buffer_addr;
		}

        void simulate(uint64_t startCycle) {
            sCycle = startCycle;
            nvram->enqueue(this, startCycle);
        }
};

/* Globally allocated event for scheduling
 *
 * NOTE: Reusing the same interface used in DDRMemory.
 */
class SchedEventNVMain : public TimingEvent, public GlobAlloc {
    private:
        NVMainMemory* const mem;
        enum State { IDLE, QUEUED, RUNNING, ANNULLED };
        State state;

    public:
        SchedEventNVMain* next;  // for event freelist

        SchedEventNVMain(NVMainMemory* _mem, int32_t domain) : TimingEvent(0, 0, domain), mem(_mem)          {
            setMinStartCycle(0);
            setRunning();
            hold();
            state = IDLE;
            next = NULL;
        }

        void parentDone(uint64_t startCycle) {
            panic("This is queued directly");
        }

        void simulate(uint64_t startCycle) {
            if (state == QUEUED) {
                state = RUNNING;
                uint64_t nextCycle = mem->tick(startCycle);
                if (nextCycle) {
                    requeue(nextCycle);
                    state = QUEUED;
                } else {
                    state = IDLE;
                    hold();
                    mem->recycleEvent(this);
                }
            } else {
                assert(state == ANNULLED);
                state = IDLE;
                hold();
                mem->recycleEvent(this);
            }
        }

        void enqueue(uint64_t cycle) {
            assert(state == IDLE);
            state = QUEUED;
            requeue(cycle);
        }

        void annul() {
            assert_msg(state == QUEUED, "sched state %d", state);
            state = ANNULLED;
        }

        // Use glob mem
        using GlobAlloc::operator new;
        using GlobAlloc::operator delete;
};


NVMainMemory::NVMainMemory(std::string& nvmainTechIni, std::string& outputFile, std::string& traceName, uint32_t capacityMB, uint64_t _minLatency, uint32_t _domain, const g_string& _name , std::string fetcher_name) {
    nvmainConfig = new NVM::Config();
	mm = NULL;
    nvmainConfig->Read(nvmainTechIni);
    info("NVMainControl: Reading NVMain config file: %s", nvmainTechIni.c_str());
	std::string mem_type = "NVMain";
	if( nvmainConfig->KeyExists("CMemType"))
		mem_type = nvmainConfig->GetString("CMemType");
	nvmainPtr = NVM::NVMainFactory::CreateNewNVMain(mem_type);
    nvmainStatsPtr = new NVM::Stats();
    nvmainSimInterface = new NVM::NullInterface();
    nvmainEventQueue = new NVM::EventQueue();
    nvmainGlobalEventQueue = new NVM::GlobalEventQueue();
    nvmainTagGenerator = new NVM::TagGenerator(1000);

    nvmainConfig->SetSimInterface(nvmainSimInterface);

    SetEventQueue(nvmainEventQueue);
    SetStats(nvmainStatsPtr);
    SetTagGenerator(nvmainTagGenerator);
    nvmainGlobalEventQueue->SetFrequency(nvmainConfig->GetEnergy("CPUFreq") * 1000000.0);
    SetGlobalEventQueue(nvmainGlobalEventQueue);

    /*  Add any specified hooks */
    std::vector<std::string>& hookList = nvmainConfig->GetHooks();
	previous_caching = 0;
	bool migrator_setted = false;
    for(size_t i = 0; i < hookList.size(); i++) {

        NVMObject *hook = NVM::HookFactory::CreateHook(hookList[i]);
        if( hook != NULL ) {
			AddHook(hook);
            hook->SetParent( this );
			std::cout<<"this:"<<this<<std::endl;
			std::cout<<"nvmainPtr:"<<nvmainPtr<<std::endl;
            hook->Init( nvmainConfig );
        } else {
            warn("Could not create a hook");
        }
		if( mem_type == "RBLANVMain" &&
			!migrator_setted && 
			hookList[i].find("Migrator")!=std::string::npos)
		{
			nvmainPtr->SetMigrator(hook);
			migrator_setted = true;
			//add migrator to nvmain
		}
	}
    //Setup child and parent modules
    AddChild(nvmainPtr);
    nvmainPtr->SetParent(this);
    nvmainGlobalEventQueue->AddSystem(nvmainPtr, nvmainConfig);
    nvmainPtr->SetConfig(nvmainConfig);
	if( mem_type == "FineNVMain"  )
	{
		mm = dynamic_cast<NVM::FineNVMain*>(nvmainPtr);
		//DRAM buffer fetcher related
		if( (mm->reserved_channels) > 0 )
		{
			fetcher = NVM::FetcherFactory::CreateFetcher(fetcher_name);
			debug_printf("set fetcher parent");
			fetcher->SetParent( nvmainPtr );
			nvmainPtr->AddChild(fetcher);
			fetcher->Init( nvmainConfig);
			mm->SetBlockFetcher(fetcher);
			//basic information about dram buffer and main memory
			zinfo->buffer_size = nvmainPtr->GetBufferSize();
			unsigned mem_width = nvmainPtr->GetMemoryWidth(); 
			zinfo->high_addr = (Address)1<<mem_width;
			//get delta time information for dynamically threshold adjustment
			mm->GetDeltaCycles( delta_hit_t,
								delta_clean_miss_t , delta_dirty_miss_t);
			debug_printf("nvmain_buffer size is : %llx",zinfo->buffer_size);
			debug_printf("base addr of nvmain dram buffer: %llx",zinfo->high_addr);
			debug_printf("width of main memory: %d",nvmainPtr->GetMemoryWidth());
		}
	}
	else
	{
		fetcher=NULL;
		zinfo->buffer_size = 0;
		zinfo->high_addr = 0;
		delta_hit_t = delta_clean_miss_t = delta_dirty_miss_t = 0;
	}
	/***-----get memory size------***/
	zinfo->memory_size = nvmainPtr->GetMemorySize();
    curCycle = 0;
    updateCycle = 0;
	nvmain_access_count = 0;
	nvmain_read_access_count = 0;
	nvmain_write_access_count = 0;
	prefetch_time = 0;
	last_memory_access = 0;
	last_memory_access_cycle=0;
    double cpuFreq = static_cast<double>(nvmainConfig->GetEnergy("CPUFreq"));
    double busFreq = static_cast<double>(nvmainConfig->GetEnergy("CLK"));
    eventDriven = static_cast<bool>(nvmainConfig->GetBool("EventDriven"));
    info("NVMain: with %f cpuFreq, %f busFreq", cpuFreq, busFreq);
    minLatency = _minLatency;
    domain = _domain;

    // No longer necessary, now we do not tick every cycle, we use SchedEvent

    name = _name;
    // Data
    if( nvmainConfig->KeyExists( "IgnoreData" ) && nvmainConfig->GetString( "IgnoreData" ) == "true" ) {
        ignoreData = true;
    } else {
        ignoreData = false;
    }

    // NVMain stats output file
    std::string path = zinfo->outputDir;
    path += "/";
    nvmainStatsFile = gm_strdup((path + name.c_str() + "-" + outputFile).c_str());
    std::ofstream out(nvmainStatsFile, std::ios_base::out);
    out << "# nvmain stats for " << name << std::endl;
    out << "===" << std::endl;

    // Wave phase handling
    nextSchedRequest = NULL;
    nextSchedEvent = NULL;
    eventFreelist = NULL;
	e = 2.718;
	T = 2000;
	N = 999;
	srand((unsigned)time(NULL));
	lastCycle = 0;
	previous_action = InitZero;
	previous_benefit = 0;

	if( zinfo->proc_fairness == false)
	{
		period_touch_vec.resize(1);
		period_nvm_touch.resize(1);
		period_access_vec.resize(1,0);
		period_nvm_access.resize(1,0);
		last_period_hotness.resize(1,0.0);
		last_action.resize(1,-1);
	}
	else
	{
		period_touch_vec.resize(zinfo->numProcs);
		period_nvm_touch.resize(zinfo->numProcs);
		period_access_vec.resize(zinfo->numProcs,0);	
		period_nvm_access.resize(zinfo->numProcs,0);
		last_period_hotness.resize(zinfo->numProcs,0.0);
		last_action.resize(zinfo->numProcs,-1);
	}
	for( unsigned i=0; i< period_touch_vec.size(); i++ )
	{
		g_map<Address, Address> tmp;
		g_map<Address, Address> tmp2;
		period_touch_vec[i] = tmp;
		period_nvm_touch[i] = tmp2;
	}
	fdrc.open("dram.log");
	fnvm.open("nvm.log");
	futex_init(&access_lock);
}

void NVMainMemory::initStats(AggregateStat* parentStat) {
    AggregateStat* memStats = new AggregateStat();
    memStats->init(name.c_str(), "Memory controller stats");
    profIssued.init("issued", "Issued requests"); memStats->append(&profIssued);
    profReads.init("rd", "Read requests"); memStats->append(&profReads);
    profWrites.init("wr", "Write requests"); memStats->append(&profWrites);
    profPUTS.init("PUTS", "Clean Evictions (from lower level)"); memStats->append(&profPUTS);
    profPUTX.init("PUTX", "Dirty Evictions (from lower level)"); memStats->append(&profPUTX);
    profTotalRdLat.init("rdlat", "Total latency experienced by read requests"); memStats->append(&profTotalRdLat);
    profTotalWrLat.init("wrlat", "Total latency experienced by write requests"); memStats->append(&profTotalWrLat);
    profMemoryFootprint.init("footprint", "Total memory footprint in bytes"); memStats->append(&profMemoryFootprint);
    profMemoryAddresses.init("addresses", "Total number of distinct memory addresses"); memStats->append(&profMemoryAddresses);
    latencyHist.init("mlh", "latency histogram for memory requests", NUMBINS); memStats->append(&latencyHist);
    addressReuseHist.init("addressReuse", "address reuse histogram for memory requests", NUMBINS); memStats->append(&addressReuseHist);
    parentStat->append(memStats);
}

uint64_t NVMainMemory::access(MemReq& req) {
	futex_lock(&access_lock);
    switch (req.type) {
        case PUTS:
            profPUTS.inc();
            *req.state = I;
            break;
        case PUTX:
            profPUTX.inc();
            *req.state = I;
            break;
        case GETS:
            *req.state = req.is(MemReq::NOEXCL)? S : E;
            break;
        case GETX:
            *req.state = M;
            break;
        default: panic("!?");
    }
	uint64_t respCycle = req.cycle + minLatency;
    assert(respCycle > req.cycle);
    if ((zinfo->hasDRAMCache || (req.type != PUTS) /*discard clean writebacks going to mainMemory*/) && zinfo->eventRecorders[req.srcId]) {
		Address addr = req.lineAddr << lineBits;
        bool isWrite = ((req.type == PUTX) || (req.type == PUTS));
		nvmain_access_count++;
		if(isWrite)
			nvmain_write_access_count++;
		else
			nvmain_read_access_count++;
		uint32_t core_id = req.srcId;
	    //access PCM main memory && LLC miss
		 NVMainAccEvent* memEv = new (zinfo->eventRecorders[req.srcId]) NVMainAccEvent(this, isWrite, addr, domain);
		 //##########counter tlb ##############/
	    if( zinfo->counter_tlb && req.type!=PUTX)
	    {
			bool is_itlb;
			ExtendTlbEntry* entry;
			TLBSearchResult over_thres;
			LookupTlb(core_id, addr,isWrite, entry, is_itlb , over_thres);
			if(!entry)
			{
				std::cout<<"core "<<req.srcId<<" error, no entry found, ppn:"<<std::hex<<(req.lineAddr)<<" type:"<<req.type<<std::endl;
			}
			else
			{
				if( over_thres == InDRAM)
				{
					Address dram_addr = (entry->get_counter()) << (zinfo->page_shift);
					Address offset = addr & (zinfo->page_size -1);
					dram_addr |= offset;
					//request type is write, get block id and set corresponding block dirty 
					debug_printf("already in DRAM,reset to %llx ",dram_addr);
					memEv->setBufferAddr();
					memEv->setAddr( dram_addr );
				}
				else{
					//in PCM,over threshold
					if( over_thres == OverThres )
					{
						debug_printf("over thres");
						MemReq request;
						//update page table and TLB
						Address vpn = entry->v_page_no;
						request.lineAddr = addr;
						request.cycle = req.cycle;
						Address dram_addr = dynamic_cast<PageTableWalker*>(zinfo->pg_walkers[core_id])->do_dram_page_fault(request,vpn ,core_id , DRAM_BUFFER_FAULT , entry , is_itlb , evict);
						memEv->setBufferAddr();
						memEv->setAddr( dram_addr );
						if( zinfo->prefetch_set && zinfo->dram_manager->get_memory_usage()<= 0.3 )
						{
							Prefetch(core_id, vpn , request.cycle);
						}
					}
				}
		}
	}
	//###############counter tlb ended#################/
	/**********************************************/
	if( zinfo->counter_tlb)
	{
		//###########****Begin Dynamic***#################
		if( zinfo->dynamic_threshold )
		{
			//####access information profiling####################
			uint32_t proc_id = 0;
			if( zinfo->proc_fairness)
			{
				proc_id = zinfo->cores[req.srcId]->GetProcIdx();	
			}
			if( memEv->isBufferAddr() )
			{
				period_access_vec[proc_id]++;
				Address ppn = memEv->getAddr()>>(zinfo->page_shift); 
				if( period_touch_vec[proc_id].count(ppn))
					period_touch_vec[proc_id][ppn]++;
				else
					period_touch_vec[proc_id][ppn] = 1;
			}
			else
			{
				Address ppn = memEv->getAddr()>>(zinfo->page_shift);
				if( period_nvm_touch[proc_id].count(ppn))
					period_nvm_touch[proc_id][ppn]++;
				else
					period_nvm_touch[proc_id][ppn]=1;
				period_nvm_access[proc_id]++;
			}
			//################end access informatin profiling##############
			//################period threshold adjustment#####################
			 if( curCycle - lastCycle >= zinfo->adjust_interval )
			 {
				//#####adjust threshold of every process####################
				for( uint32_t i=0; i< period_touch_vec.size(); i++)
				{
					double drc_average = (double)period_access_vec[i]/(double)period_touch_vec[i].size();
					if( last_period_hotness[i]==0.0)
					{	
						last_period_hotness[i] = drc_average;
					}
					else
					{
						//#########adjust threshold with hill-climbing algorithm
						if( zinfo->dram_manager->should_cherish() == false)
						{
							Climbing( zinfo->access_threshold[i] , drc_average , last_period_hotness[i],
									last_action[i] );
						}
						//#########end hill-climbing algorithm
						//#########should use DRAM buffer cherishly
						else
						{
							assert( mm );
							Address caching_cycle = mm->GetCachingCycles();
							double prefetch_benefit = (double)period_access_vec[i]*15+previous_caching-caching_cycle; 	
							if( previous_caching == 0)
							{
								previous_caching = caching_cycle;
								last_period_hotness[i] = prefetch_benefit;	
							}
							else
							{
								if( prefetch_benefit < 0 || zinfo->dram_manager->should_more_cherish())
								{	
									unsigned v1 =(unsigned)((double)period_access_vec[i]/
												(double)(period_touch_vec[i].size())*29/44);
									unsigned v2 = 2*zinfo->access_threshold[i];
									if( !evict && (zinfo->dram_manager->should_more_cherish() == false) )
									{
										if( v2>0 )
											zinfo->access_threshold[i] = Min(v1, v2);
										else
											zinfo->access_threshold[i] = v1;
									}
									else
									{
										zinfo->access_threshold[i] = v1;
									}
								}
								else
									Climbing( zinfo->access_threshold[i],prefetch_benefit,last_period_hotness[i],last_action[i]);
								previous_caching = caching_cycle;
							}
						}
						//###########>usage_thres: threshold adjustment end##############
					}
						//################dynamic threshold adjustment end######### 
				}
						//################dynamic threshold adjustment for every process end#####
				//reset counter of PCM TLB entry periodly
				if( zinfo->multi_queue == false)
				{
					for(uint32_t i=0; i<zinfo->numCores; i++ )
					{
						zinfo->cores[i]->getInsTlb()->clear_counter();
						zinfo->cores[i]->getDataTlb()->clear_counter();
					}
				}
				for( unsigned i=0; i<period_touch_vec.size(); i++)
				{
					period_touch_vec[i].clear();
					period_nvm_touch[i].clear();
					period_access_vec[i] = 0;
					period_nvm_access[i] = 0;
				}
				lastCycle = curCycle;
			}
			//##########end period threshold adjustment############
		  }
			//#####****End Dynamic Threshold adjustment***#########
		  else if( zinfo->multi_queue == false && curCycle - lastCycle >= zinfo->adjust_interval )
		  {
			  //reset counter of PCM TLB entry periodly
			  for(uint32_t i=0; i<zinfo->numCores; i++ )
			  {
				  zinfo->cores[i]->getInsTlb()->clear_counter();
				  zinfo->cores[i]->getDataTlb()->clear_counter();
			  }
		  }
	  }
	
		//########end counter tlb#######
    memEv->setMinStartCycle(req.cycle);
	TimingRecord tr = {addr, req.cycle, respCycle, req.type, memEv, memEv};
	zinfo->eventRecorders[req.srcId]->pushRecord(tr);
	}
	futex_unlock(&access_lock);
    return respCycle;
}

inline void NVMainMemory::Climbing( unsigned& threshold, double benefit, double &previous_benefit, 
									int &last_action)
{
	double delta = benefit - previous_benefit;
	if( benefit < 0)
	{
		threshold += 2;
		last_action = 1;
	}

	else if( delta >= 0)
	{
		if(last_action==-1 && threshold>0)
		{
			threshold--;
			last_action = -1;
		}
		else if( last_action == 1)
		{
			threshold++;
			last_action = 1;
		}
	}
	else if( delta<0)
	{
		if( last_action == 1 && threshold>0)
		{
			threshold--;
			last_action = -1;
		}
		else if( last_action == -1)
		{
			threshold++;
			last_action = 1;
		}
	}
	previous_benefit = benefit; 
}

inline bool NVMainMemory::Prefetch( unsigned core_id, Address vpn , Address cycle )
{
	uint32_t proc_id = 0;
	if(zinfo->proc_fairness) 
		proc_id = zinfo->cores[core_id]->GetProcIdx();

	Address near_vpn = vpn;
	ExtendTlbEntry* near_entry;
	bool is_itlb = false;
	if( vpn%2 )
		near_vpn = vpn-1;
	else
		near_vpn = vpn+1;
	HotMonitorTlb<ExtendTlbEntry>* itlb = dynamic_cast<HotMonitorTlb<ExtendTlbEntry>*>(zinfo->cores[core_id]->getInsTlb());
	HotMonitorTlb<ExtendTlbEntry>* dtlb = dynamic_cast<HotMonitorTlb<ExtendTlbEntry>*>(zinfo->cores[core_id]->getDataTlb());
	near_entry = dtlb->look_up( near_vpn );
	if( !near_entry )
	{
		is_itlb = true;
		itlb->look_up( near_vpn );
	}
	if( near_entry )
	{
		if( near_entry->is_in_dram())
		{
			std::cout<<"already in dram"<<std::endl;
			return false;
		}
		//prefetch
		else if( near_entry->access_counter >= 0.5*zinfo->access_threshold[proc_id] )
		{
				std::cout<<"prefetch ("<<std::hex<<vpn<<","<<std::hex<<near_entry->p_page_no<<")"<<std::endl;
				MemReq request;
				//update page table and TLB
				Address vpn = near_entry->v_page_no;
				request.lineAddr = near_entry->p_page_no<<(zinfo->page_shift);
				request.cycle = cycle;
				dynamic_cast<PageTableWalker*>(zinfo->pg_walkers[core_id])->do_dram_page_fault(request,vpn ,core_id , DRAM_BUFFER_FAULT , near_entry , is_itlb , evict);
				prefetch_time++;
				return true;
		}
				
	}
	return false;
}


inline void NVMainMemory::LookupTlb(uint32_t coreId, Address addr , bool isWrite, ExtendTlbEntry* &entry, bool& is_itlb , TLBSearchResult& over_thres)
{
	uint32_t proc_id = zinfo->cores[coreId]->GetProcIdx();
	Address ppn = addr>>(zinfo->page_shift);
	over_thres = unOverThres;
	is_itlb = true;
	entry= NULL;
	HotMonitorTlb<ExtendTlbEntry>* itlb = dynamic_cast<HotMonitorTlb<ExtendTlbEntry>*>(zinfo->cores[coreId]->getInsTlb());
	HotMonitorTlb<ExtendTlbEntry>* dtlb = dynamic_cast<HotMonitorTlb<ExtendTlbEntry>*>(zinfo->cores[coreId]->getDataTlb());
	if( itlb )
	   entry = itlb->look_up_va(ppn,isWrite,proc_id , over_thres);
	if(entry)
	{	
		 debug_printf("itlb : entry (%llx,%llx)",entry->v_page_no , entry->p_page_no);
		 debug_printf("addr is %llx, page should be increased is %llx",addr , ppn);
	}
	else if( dtlb )
	{
		debug_printf("look up data tlb");
		entry = dtlb->look_up_va(ppn,isWrite,proc_id , over_thres);
		is_itlb = false;
	}
}


//adjust access threshold of TLB dynamically
inline void NVMainMemory::AdjustThreshold( int &delta_thres, uint64_t hit_time,
						uint64_t clean_miss_time , uint64_t dirty_miss_time,
						uint64_t caching_cycles)
{
	int64_t benefit = hit_time*delta_hit_t + clean_miss_time*delta_clean_miss_t + dirty_miss_time*delta_dirty_miss_t - caching_cycles;
	int64_t delta = benefit - previous_benefit;
	std::cout<<"hit:"<<std::dec<<hit_time<<
			"  clean miss:"<<std::dec<<clean_miss_time<<
			" dirty miss:"<<std::dec<<dirty_miss_time<<
			"  caching_cycles:"<<std::dec<<caching_cycles<<std::endl;
	std::cout<<"benefit is:"<<std::dec<<benefit<<"  delta:"<<std::dec<<delta<<std::endl;
	if(previous_action==InitZero)
	{
		if( benefit >=0)
			previous_action = Decre;
		else
			previous_action = Incre;
	}
	//caching is performance benefit
	if( delta >=0)
	{
		if( previous_action == Incre )
			delta_thres = 1;
		else
			delta_thres = -1;
	}
	else
	{
		if( previous_action == Incre )
			delta_thres = -1;
		else
			delta_thres = 1;
	}
	previous_benefit = benefit;
	if( delta_thres < 0)
		previous_action = Decre;
	else
		previous_action = Incre;
}

uint64_t NVMainMemory::tick(uint64_t cycle) {

    // Advance NVMain to current cycle
    //info("[%s] [Tick] Update NVMain %lu cycles", getName(), (cycle+1) - updateCycle);
    nvmainGlobalEventQueue->Cycle((cycle+1) - updateCycle);
    updateCycle = cycle + 1;
    curCycle = updateCycle;

    // Check if nextSchedEvent has been serviced by RequestComplete
    // TODO: Maybe I need to save the Address in a nextSchedAddress varaible to know this
    // If the event has not been serviced do 'return 1', which reschedules for next cycle.
    // Else check multiset/multimap for next event if any.
    assert(nextSchedEvent);
    if (nextSchedRequest) { // Not serviced yet, cycle by cycle until it's served
        return cycle + 1;
    } else { // has been serviced, check for inflight requests
        if (inflightRequests.empty()) {
            nextSchedEvent = NULL;
            nextSchedRequest = NULL;
            return 0; //this will recycle the SchedEvent
        } else {
            nextSchedRequest = inflightRequests.front().first;
            if (cycle >= inflightRequests.front().second.second) {
                return cycle + 1; // we are past min finish cycle for next request
            } else {
                return inflightRequests.front().second.second; //get min finish cycle for next request
            }
        }
    }
}

void NVMainMemory::recycleEvent(SchedEventNVMain* ev) {
    assert(ev != nextSchedEvent);
    assert(ev->next == NULL);
    ev->next = eventFreelist;
    eventFreelist = ev;
}

// Use this for debugging
/*
template< class T >
std::ostream & operator << ( std::ostream & os, const std::multiset< T > & v ) {
    for ( const auto & i : v ) {
        os << i << std::endl;
    }
    return os;
}

template< class K, class V >
std::ostream & operator << ( std::ostream & os, const std::map< K, V > & m ) {
    for ( const auto & i : m ) {
        os << i.first << " : " << i.second << std::endl;
    }
    return os;
}

template< class T >
std::ostream & operator << ( std::ostream & os, const std::vector< T > & m ) {
    for ( const auto & i : m ) {
        os << std::hex << i.first->address.GetPhysicalAddress() << std::dec << " : " << i.second.first << ", " << i.second.second << std::endl;
    }
    return os;
}
*/
void NVMainMemory::enqueue(NVMainAccEvent* ev, uint64_t cycle) {
    profIssued.inc();
    // Build NVMainRequest and send it to NVMain
    NVM::NVMainRequest *request = new NVM::NVMainRequest();
    // No data in memory for now
    if (!ignoreData) {
        int transfer_size = zinfo->lineSize;
        request->data.SetSize(transfer_size);
        for(int i = 0; i < transfer_size; i++) {
            request->data.SetByte(i, 0);
        }
    }
	request->startCycle = curCycle;
	request->arrivalCycle = curCycle;
    request->access = NVM::UNKNOWN_ACCESS;
    request->address.SetPhysicalAddress(ev->getAddr());
    request->status = NVM::MEM_REQUEST_INCOMPLETE;
    request->type = (ev->isWrite()) ? NVM::WRITE : NVM::READ;
    request->owner = (NVMObject *)this;

    // Sync NVMain state to curCycle
    // NVMain can only issue command in the current cycle
    curCycle = cycle + 1;
    //info("[%s] [Enqueue] Update NVMain %lu cycles", getName(), curCycle - updateCycle);
    nvmainGlobalEventQueue->Cycle(curCycle - updateCycle);
    updateCycle = curCycle;

    // If command cannot be issued due to contention, retry next cycle.
    if (!nvmainPtr->IsIssuable(request, NULL)) {
        //info("[%s] %s access to %lx requeued. Curent cycle %ld, requeue cycle %ld", getName(), ev->isWrite()? "Write" : "Read", ev->getAddr(), cycle, cycle+1);
        ev->requeue(cycle+1);
        delete request;
        return;
    }
	//issue to pre hook firstly
	std::vector<NVMObject*>& preHooks = GetHooks(NVM::NVMHOOK_PREISSUE);
	std::vector<NVMObject*>::iterator it;
	for( it = preHooks.begin(); it!=preHooks.end(); it++)
	{
		(*it)->SetParent(nvmainPtr);
		(*it)->IssueCommand(request);
	}
    //info("[%s] [Enqueue] Address %lx curCycle %lu, cycle %lu, updateCycle %lu, inflight requests %ld", getName(), ev->getAddr(), curCycle, cycle, updateCycle, inflightRequests.size());
    bool enqueued = nvmainPtr->IssueCommand(request);
    //assert(enqueued);
	if(enqueued)
	{
    // Update stats
    const auto it = memoryHistogram.find(ev->getAddr());
    if (it == memoryHistogram.end()){
        memoryHistogram.insert(std::make_pair<uint64_t, uint64_t>(ev->getAddr(), 1));
        profMemoryAddresses.inc(1);
        profMemoryFootprint.inc(zinfo->lineSize);
        addressReuseHist.inc(1);
    } else {
        addressReuseHist.dec(std::min(NUMBINS-1, it->second));
        it->second++;
        addressReuseHist.inc(std::min(NUMBINS-1, it->second));
    }
	}
    // Add this request to list of inflight requests
    inflightRequests.push_back(std::make_pair(request, std::make_pair(ev, cycle + minLatency)));
    ev->hold();

    // Event handling
    if (!nextSchedEvent) {
        assert(inflightRequests.size() == 1);
        if (eventFreelist) {
            nextSchedEvent = eventFreelist;
            eventFreelist = eventFreelist->next;
            nextSchedEvent->next = NULL;
        } else {
            nextSchedEvent = new SchedEventNVMain(this, domain);
        }
        nextSchedEvent->enqueue(cycle + minLatency);
        nextSchedRequest = request;
    }
	
    return;
}

bool NVMainMemory::RequestComplete(NVM::NVMainRequest *creq) {
    assert(inflightRequests.size() > 0);
    auto it = inflightRequests.begin();
    for (; it != inflightRequests.end(); ++it) {
        if (it->first == creq) break;
    }
    assert(it != inflightRequests.end());
    NVMainAccEvent* ev = it->second.first;

    // Note that curCycle is up to date because we are advancing cycle by cycle in tick while
    // we are waiting for a request completion.
    //uint64_t lat = curCycle+1 - ev->sCycle;
    uint64_t lat = curCycle+1 - creq->startCycle;
    if (ev->isWrite()) {
        profWrites.inc();
        profTotalWrLat.inc(lat);
    } else {
        profReads.inc();
        profTotalRdLat.inc(lat);
        uint32_t bucket = std::min(NUMBINS-1, lat/BINSIZE);
        latencyHist.inc(bucket, 1);
    }

    ev->release();
    ev->done(curCycle+1);

    if (creq == nextSchedRequest)
        nextSchedRequest = NULL;

    inflightRequests.erase(it);

    //info("[%s] [RequestComplete] %s access to %lx DONE at %ld (%ld cycles), %ld inflight reqs", getName(), ev->isWrite()? "W" : "R", ev->getAddr(), curCycle, lat, inflightRequests.size());

    delete creq;
    return true;
}

void NVMainMemory::printStats() {
    //info("Print NVMain stats for %s, curCycle %ld, updateCycle %ld", getName(), dynamic_cast<OOOCore*>(zinfo->cores[0])->getCycles(), updateCycle);
    std::ofstream out(nvmainStatsFile, std::ios_base::app);
    nvmainPtr->CalculateStats();
    nvmainPtr->GetStats()->PrintAll(out);

    out << "===" << std::endl;
}
#else //no nvmain, have the class fail when constructed
NVMainMemory::NVMainMemory(std::string& nvmainTechIni, std::string& outputFile, std::string& traceName,
        uint32_t capacityMB, uint64_t _minLatency, uint32_t _domain, const g_string& _name)
{
    panic("Cannot use NVMainMemory, zsim was not compiled with NVMain");
}

void NVMainMemory::initStats(AggregateStat* parentStat) { panic("???"); }
uint64_t NVMainMemory::access(MemReq& req) { panic("???"); return 0; }
uint64_t NVMainMemory::tick(uint64_t cycle) { panic("???"); return 0; }
void NVMainMemory::enqueue(NVMainAccEvent* ev, uint64_t cycle) { panic("???"); }
void NVMainMemory::recycleEvent(SchedEventNVMain* ev) { panic("???"); }
bool NVMainMemory::RequestComplete(NVM::NVMainRequest *creq) { panic("???"); }
void NVMainMemory::printStats() { panic("???"); }

#endif

