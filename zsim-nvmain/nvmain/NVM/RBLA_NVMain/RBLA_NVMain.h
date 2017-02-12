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

#ifndef __RBLA_NVMAIN_H__
#define __RBLA_NVMAIN_H__

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "src/Params.h"
#include "src/NVMObject.h"
#include "src/Prefetcher.h"
#include "include/NVMainRequest.h"
#include "include/global_vars.h"
#include "traceWriter/GenericTraceWriter.h"
#include "MemControl/DRAMCache/DRAMCache.h"
#include "NVM/RBLA_NVMain/StatsStore.hh"
#include "NVM/nvmain.h"
#include <memory>
#include "Decoders/Migrator/Migrator.h"
#include "Utils/CommonMigrator/CommonMigrator.h"

namespace NVM {
//action
/*enum Action
{
	INIT,
	DECREMENT,
	INCREMENT
};*/


class RBLA_NVMain : public NVMain
{
  public:
    RBLA_NVMain( );
    ~RBLA_NVMain( );

    void SetConfig( Config *conf, std::string memoryName = "defaultMemory", bool createChildren = true );

    bool IssueCommand( NVMainRequest *request );

    bool RequestComplete( NVMainRequest *request );

    void RegisterStats( );
    void CalculateStats( );
	void Cycle( ncycle_t steps );
	void SetMigrator( NVMObject* migra){
		migrator = dynamic_cast<CommonMigrator*>(migra); 
		migrator->SetParent(this);
	}

  protected:
	bool UpdateStatsTable ( uint64_t row_num , uint64_t incre_num);
	void AdjustMigrateThres();
	uint64_t GetRowNum(NVMainRequest* req);
	double CalculateBenefit();
  private:
	std::auto_ptr<StatsStore> statsTable;
	ncounter_t statsHit , statsMiss;
	uint64_t last_cycle;
	double statsHitRate;
	//name identify migrator
	std::string migrator_name_;
	CommonMigrator* migrator;
	////////////////////////////////////////
	
	//added on 2015/5/4
	uint64_t stats_table_entry;
	uint64_t write_incre , read_incre;
	uint64_t update_interval;
	
	static uint64_t migra_thres;	//migration threshold	
	static NVM::Action pre_action;
    static uint64_t pre_net_benefit;	
	
	uint64_t src_channel;
	uint64_t dst_channel;
	//statistic data
	uint64_t migrated_pages_;
	uint64_t dirty_rb_miss_ , clean_rb_miss_;
	double rb_hitrate;
	uint64_t row_buffer_hits , row_buffer_miss;
	//const data
	int tdram_hit_ , tdram_miss_ ;
	int tpcm_hit_ , tpcm_clean_miss_ , tpcm_miss_;
	int tdiff_dirty_ , tdiff_clean_;
	int tmigration_;

};
};

#endif
