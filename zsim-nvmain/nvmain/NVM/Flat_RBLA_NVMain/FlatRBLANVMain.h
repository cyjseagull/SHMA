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

#ifndef __FLAT_RBLA_NVMAIN_H__
#define __FLAT_RBLA_NVMAIN_H__

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "src/Params.h"
#include "src/NVMObject.h"
#include "src/Prefetcher.h"
#include "include/NVMainRequest.h"
#include "traceWriter/GenericTraceWriter.h"
#include "MemControl/DRAMCache/DRAMCache.h"
#include "NVM/RBLA_NVMain/StatsStore.hh"
#include "NVM/Flat_NVMain/FlatNVMain.h"
#include "NVM/nvmain.h"
#include <memory>
#include "Decoders/FlatDecoder/FlatDecoder.h"
#include "include/global_vars.h"
namespace NVM {
class FlatMigrator;
class FlatRBLANVMain : public NVMain
{
  public:
    FlatRBLANVMain( );
    ~FlatRBLANVMain( )
	{
		if( flat_memory )
			delete flat_memory;
	}

    void SetConfig( Config *conf, std::string memoryName = "defaultMemory", bool createChildren = true );

    bool IssueCommand( NVMainRequest *request );
	bool IsIssuable(NVMainRequest* req, FailReason* reason=NULL);

    bool RequestComplete( NVMainRequest *request );

    void RegisterStats( );
    void CalculateStats( );
	void Cycle( ncycle_t steps );
	void SetMigrator( NVMObject* migra);
	
	uint64_t GetMemorySize()
	{
		return flat_memory->GetMemorySize();
	}
	FlatNVMain* GetFlatMemory()
	{
		return flat_memory;
	}
  protected:
	bool UpdateStatsTable ( uint64_t row_num , uint64_t incre_num);
	void AdjustMigrateThres();
	uint64_t GetRowNum(NVMainRequest* req);
	double CalculateBenefit();
  private:
	FlatNVMain* flat_memory;
	StatsStore* statsTable;
	ncounter_t statsHit , statsMiss;
	uint64_t last_cycle;
	double statsHitRate;
	//name identify migrator
	std::string migrator_name_;
	FlatMigrator* migrator;
	////////////////////////////////////////
	
	//added on 2015/5/4
	uint64_t stats_table_entry;
	uint64_t write_incre , read_incre;
	uint64_t update_interval;
	
	static uint64_t migra_thres;	//migration threshold	
	static NVM::Action pre_action;
    static double pre_net_benefit;	
	
	//statistic data
	uint64_t migrated_pages_;
	int64_t pre_migration_cycles;
	uint64_t dirty_rb_miss_ , clean_rb_miss_;
	uint64_t nvm_dirty_rb_miss_ , nvm_clean_rb_miss_;
	double rb_hitrate;
	uint64_t row_buffer_hits , row_buffer_miss;
	//const data
	int tdram_hit_ , tdram_miss_ ;
	int tpcm_hit_ , tpcm_clean_miss_ , tpcm_miss_;
	int tdiff_dirty_ , tdiff_clean_;
	int tmigration_;
	uint64_t total_access_time;
	uint64_t migrated_access;
	uint64_t total_fast_access_time;
	uint64_t total_slow_access_time;
	uint64_t per_fast_page_access, accessed_fast_page_count, accessed_slow_page_count;
	double fast_memory_usage;
};
};

#endif
