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

#ifndef __FINE_NVMAIN_H__
#define __FINE_NVMAIN_H__

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "NVM/nvmain.h"
#include "src/NVMObject.h"
#include "src/Prefetcher.h"
#include "include/NVMainRequest.h"
#include "traceWriter/GenericTraceWriter.h"
#include "include/Exception.h"
#include "Utils/FetcherFactory.h"
#include <map>
#include <iterator>
#include <algorithm>

#include "locks.h"
namespace NVM {

class FineNVMain : public NVMain
{
  public:
    FineNVMain( );
    ~FineNVMain( );
    virtual void SetConfig( Config *conf, std::string memoryName = "MainMemory",
							bool createChildren = true );
    Config *GetConfig( );
	bool IsIssuable( NVMainRequest *request , FailReason* reason);
    bool IssueCommand( NVMainRequest *request );
    bool IssueAtomic( NVMainRequest *request );
	bool RequestComplete( NVMainRequest *request);
	void Cycle( ncycle_t steps = 1 );
    void RegisterStats( );
	void CalculateStats();

	uint64_t GetMemorySize()
	{
		return mem_size;
	}

	unsigned GetWordSize()
	{
		return mem_word_size;
	}

	unsigned GetMemoryWidth()
	{
		return mem_width;
	}
	uint64_t GetBufferSize()
	{
		if(!is_configed)
			NVM::Warning( "haven't set configuration yet,	\
						returned buffer size has non-sense!");
		return cache_size;
	}
	unsigned GetBufferWordSize()
	{
		if(!is_configed)
			NVM::Warning("haven't set configuration yet ,	\
					returned buffer word size has non-sense!");
		return cache_word_size;
	}
	
	AddressTranslator* GetBufferDecoder()
	{
		return cacheTranslator;		
	}
	
	void SetBlockFetcher( NVMObject* block)
	{
		block_fetcher = block;
	}

	NVMObject* GetBlockFetcher()
	{
		return block_fetcher;
	}
	
	void GetDeltaCycles(int &hit, int &clean_miss , int &dirty_miss) 
	{
		hit = delta_hit_t;
		clean_miss = delta_clean_miss_t;
		dirty_miss = delta_dirty_miss_t;
	}

	void GetAdjustThresInfo( uint64_t& hit_time , 
				uint64_t &clean_miss_time , 
				uint64_t& dirty_miss_time ,
				uint64_t &caching_cycles)
	{
		hit_time = rb_hit_time;
		dirty_miss_time = rb_dirty_miss_time;
		clean_miss_time = rb_clean_miss_time;
		caching_cycles = total_caching_t;
	}

	uint64_t GetCachingCycles()
	{
		return total_caching_t; 
	}
	void GetHitRate( double& read_hit, double& write_hit)
	{
		read_hit = drc_read_hit_rate;
		write_hit = drc_write_hit_rate;
	}

	unsigned GetFastReadCycles(){ return t_fast_read; }
	unsigned GetSlowReadCycles(){ return t_slow_read; }
	
	unsigned GetSlowWriteCycle(){ return t_slow_write; }
	uint64_t mem_size;
	uint64_t access_time;
	uint64_t cache_size;

	unsigned mem_word_size;
	unsigned cache_word_size;

	unsigned mem_width;
	unsigned cache_width;
	uint64_t reserved_channels;
	
	typedef std::pair<uint64_t , uint64_t> pair;
	std::map<uint64_t , uint64_t> page_access_map_; 
    static unsigned int numChannels;
  protected:
    Config *config;
    Config **channelConfig;
	Config **reservedConfig;
    MemoryController **memoryControllers;
	MemoryController **reservedControllers;
	
	AddressTranslator *translator;
	AddressTranslator *cacheTranslator;
	
	NVMObject* block_fetcher;
    Prefetcher *prefetcher;

    std::ofstream pretraceOutput;
    GenericTraceWriter *preTracer;
	//statistical
	uint64_t dram_buffer_read;
	uint64_t dram_buffer_write;
	uint64_t pcm_read;
	uint64_t pcm_write;
	uint64_t fetch_time;
	uint64_t dram_access_time;
	uint64_t pcm_access_time;
	uint64_t total_access_time;
	uint64_t drcReadRequests;
	uint64_t drcWriteRequests;
	uint64_t nvmReadRequests;
	uint64_t nvmWriteRequests;
	
	/*****adjust algorithm related statisticals****/
	int delta_clean_miss_t, delta_dirty_miss_t;
	int delta_hit_t;
	uint64_t rb_clean_miss_time, rb_dirty_miss_time;
	uint64_t total_caching_t;
	uint64_t rb_hit_time;
	/*****************************************/
	double hit_rate;

	double drc_read_hit_rate;
	uint64_t drc_read_hits;
	uint64_t drc_write_hits;
	double drc_write_hit_rate;
	
	unsigned t_fast_read, t_slow_read, t_slow_write;
	//end statistical
    void GeneratePrefetches( NVMainRequest *request,
							std::vector<NVMAddress>& prefetchList );
 
  private:
	void SetMemoryControllers( MemoryController** &mem_ctl , Config** &ctl_conf,
							   Config* conf , int num , std::string base_str , 
							   std::string memory_name);
	void SetTranslators( Config* conf , AddressTranslator* &translator ,
						Params* param , unsigned &bit_width , 
						unsigned &word_size , uint64_t &mem_size );

	static int cmp( const pair &x , const pair &y)
	{
		return x.second > y.second;
	}
	inline unsigned GetReadLatency( Params* p);
	inline unsigned GetWriteLatency( Params* p);

	Params* reserve_region_param;
	bool is_configed;
	uint64_t clflush_wb_overhead;
	lock_t issue_lock;
};
};
#endif
