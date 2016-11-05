/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
* 
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: Yujie Chen (YuJieChen_hust@163.com)
*******************************************************************************/

#ifndef __MEMCONTROL_HIERARCHY_DRAMCACHE_H__
#define __MEMCONTROL_HIERARCHY_DRAMCACHE_H__
#include <set>
#include <fstream>
#include "src/MemoryController.h"
#include "Utils/Caches/CacheBank.h"
#include "MemControl/DRAMCache/AbstractDRAMCache.h"
#include "NVM/nvmain.h"
namespace NVM {
class NVMainFactory;
struct bool_pair
{
	bool first_bool;
	bool second_bool;
};  
#define HierDRC_MEMREAD tagGen->CreateTag("HierDRC_MEMREAD")
#define HierDRC_FILL tagGen->CreateTag("HierDRC_FILL")
#define HierDRC_EVICT tagGen->CreateTag("HierDRC_EVICT")
class HierDRAMCache : public NVMain
{
  public:
    HierDRAMCache( );
    ~HierDRAMCache( );
    virtual void SetConfig( Config *conf, std::string memName="", bool createChildren = true );
    bool IssueAtomic( NVMainRequest *req );
    bool IssueCommand( NVMainRequest *req );
    bool IssueFunctional( NVMainRequest *req );
    bool RequestComplete( NVMainRequest *req );
	bool IsIssuable( NVMainRequest *req, FailReason *reason);
	int GetMissCycles()
	{
		return dram_miss_t_;
	}

	void GetHitRate( double& read_hit, double& write_hit)
	{
		read_hit = drc_read_hit_rate;
		write_hit = drc_write_hit_rate;
	}

	virtual uint64_t GetMemorySize()
	{
		return memory_size;
	}

    void Cycle( ncycle_t );

    void RegisterStats( );
    void CalculateStats( );

	NVMain* GetMainMemory( );
	uint64_t GetCacheLineSize();
	ncounter_t drc_evicts,drc_dirty_evicts, drc_fetches;
 private:
	inline void Retranslate(NVMainRequest* req, uint64_t address);
	inline void ConvertToCacheAddress(ncounter_t set ,ncounter_t assoc,ncounter_t addr_offset,
									ncounter_t &cache_addr)
		{
		    cache_addr = (set<<set_off_bit) + (assoc<<lineSizeOff) + addr_offset;	
		}
 private:
    NVMain *mainMemory;
	std::string mem_type;
	Config** drcConfig;
    MemoryController **drcChannels;
    ncounter_t numChannels;
	uint64_t dram_miss_t_;	
	uint64_t cache_line_size;
	CacheBank *dram_cache;
	//(memread request ptr, origin request ptr)
	std::map<NVMainRequest* , NVMainRequest *>outstandingMemReads;
	std::map<NVMainRequest* , NVMainRequest *>outstandingEvicts;
	std::map<NVMainRequest* , NVMainRequest *>outstandingFills;
	std::map<NVMainRequest* , bool_pair>cachingMap;

	uint64_t total_access_time;

	ncounter_t  ranks, banks,rows,cols;
	ncounter_t offset_word;
	//statistical data of DRAM cache 
	ncounter_t assoc;
	ncounter_t lineSize;
	unsigned lineSizeOff;
	//memory controller burst read for fetch data into dram 
	unsigned burst_count;  
	ncounter_t set_num;
	ncounter_t set_off_bit;
	//statistical data
	ncounter_t total_drc_size;
	ncounter_t drc_hits, drc_miss;
	ncounter_t write_hits, write_miss;
	ncounter_t read_hits, read_miss;
	ncounter_t rb_hits, rb_miss;
	ncounter_t caching_cycles;
	double drc_hit_rate;
	double drc_miss_rate;
	double drc_read_hit_rate;
	double drc_read_miss_rate;
	double drc_write_hit_rate;
	double drc_write_miss_rate;
	uint64_t memory_size;
};
};

#endif
