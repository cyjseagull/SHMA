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

#include "NVM/Flat_RBLA_NVMain/FlatRBLANVMain.h"
#include "src/Config.h"
#include "src/AddressTranslator.h"
#include "src/Interconnect.h"
#include "src/SimInterface.h"
#include "src/EventQueue.h"
#include "Interconnect/InterconnectFactory.h"
#include "MemControl/MemoryControllerFactory.h"
#include "traceWriter/TraceWriterFactory.h"
#include "Decoders/DecoderFactory.h"
#include "Utils/HookFactory.h"
#include "include/NVMainRequest.h"
#include "include/NVMHelpers.h"
#include "Prefetchers/PrefetcherFactory.h"
#include "include/Exception.h"
#include "nvmain_mem_ctrl.h"
#include <sstream>
#include <cassert>

#include "Utils/FlatMigrator/FlatMigrator.h"

#include "zsim.h"
#include "core.h"
#include "timing_core.h"
using namespace NVM;


uint64_t FlatRBLANVMain::migra_thres = 0;
Action FlatRBLANVMain::pre_action = INIT;
double FlatRBLANVMain::pre_net_benefit = 0;
FlatRBLANVMain::FlatRBLANVMain( )
{
	stats_table_entry = 16;
	write_incre = 2;
	read_incre = 1;
	update_interval = 10000000;
	migrator_name_ = "FlatRBLANVMain";
	migrated_pages_ = 0;

	last_cycle = 0;
	statsHit = 0;
	statsMiss = 0;
	rb_hitrate = 0.0;
	clean_rb_miss_ = dirty_rb_miss_ = 0;
	nvm_clean_rb_miss_ = nvm_dirty_rb_miss_ = 0;
	tdram_hit_ = tdram_miss_ = -1;
	tpcm_hit_ = tpcm_clean_miss_ = tpcm_miss_ = -1;
	row_buffer_miss = 0;
	row_buffer_hits = 0;

	flat_memory = NULL;
	total_access_time = 0;
	total_fast_access_time = 0;
	total_slow_access_time = 0;
	fast_memory_usage = 0.0;
	per_fast_page_access = 0;
	accessed_fast_page_count = 0;
	accessed_slow_page_count = 0;
	std::cout<<"create flat RBLA NVMain"<<std::endl;
}


void FlatRBLANVMain::SetConfig( Config *conf, std::string memoryName, bool createChildren )
{
	std::cout<<"set config for Flat RBLA nvmain"<<std::endl;
	if( conf->KeyExists("StatsTableEntry"))
		stats_table_entry = static_cast<uint64_t>( conf->GetValue("StatsTableEntry") );
	if(conf->KeyExists("WriteIncrement"))
		write_incre = static_cast<uint64_t>(conf->GetValue("WriteIncrement"));
	if(conf->KeyExists("ReadIncrement"))
		read_incre = static_cast<uint64_t>(conf->GetValue("ReadIncrement"));
	if(conf->KeyExists("UpdateInterval"))
		update_interval = static_cast<uint64_t>(conf->GetValue("UpdateInterval"));
	if(conf->KeyExists("MigrationThres"))
		migra_thres = static_cast<uint64_t>(conf->GetValue("MigrationThres"));
	if( conf->KeyExists("MigratorName"))
		migrator_name_ = conf->GetString("MigratorName");
	std::cout<<"create stats table"<<std::endl;
	//create stats table
	statsTable =  new StatsStore(stats_table_entry);
	std::cout<<"create flat nvmain"<<std::endl;
	flat_memory = new FlatNVMain();
	EventQueue* queue=new EventQueue();
	
	flat_memory->SetParent(this);
	flat_memory->SetEventQueue(queue);
	GetGlobalEventQueue()->AddSystem( flat_memory,conf );
	flat_memory->SetConfig( conf, memoryName, createChildren);
	AddChild(flat_memory);
	flat_memory->RegisterStats();
	if( conf->KeyExists("Decoder"))
	{
		std::string decoder = conf->GetString("Decoder");
		if( decoder == "FlatDecoder" )
		{
			FlatDecoder* migrator = new FlatDecoder();
			migrator->Init( flat_memory->GetFastMemoryConfig(),
					flat_memory->GetSlowMemoryConfig());
			migrator->SetFastMemory( flat_memory->GetFastMemory() );
			migrator->SetSlowMemory( flat_memory->GetSlowMemory());
			migrator->SetFastMemoryBits(flat_memory->GetFastMemoryBits());
			SetDecoder(migrator);
		}
	}
	tpcm_clean_miss_ = flat_memory->GetSlowMemory()->memoryControllers[0]->GetMissCycles();
	tpcm_miss_ = flat_memory->GetSlowMemory()->memoryControllers[0]->GetDirtyMissCycles();
	tdram_miss_ = flat_memory->GetFastMemory()->memoryControllers[0]->GetMissCycles();
	
	tdiff_dirty_ = tpcm_miss_ - tdram_miss_;
	tdiff_clean_ = tpcm_clean_miss_ - tdram_miss_;
	std::cout<<"tdiff_dirty_:"<<tdiff_dirty_<<std::endl;
	std::cout<<"tdiff_clean_:"<<tdiff_clean_<<std::endl;
	migrated_access = 0;
	RegisterStats();
}


bool FlatRBLANVMain::IsIssuable( NVMainRequest* request, FailReason* reason)
{
	
	FlatDecoder* migrator_trans = dynamic_cast< FlatDecoder*>(this->GetDecoder());
	//migrator decoder must be Migrator object
	assert( migrator_trans);
	if ( migrator_trans->IsMigrated( request->address) )
	{
		request->is_migrated = true;
		uint64_t row, col, bank,rank,channel,subarray;
		uint64_t paddr=request->address.GetPhysicalAddress();
		request->address.SetVirtualAddress( paddr);
		migrator_trans->Translate2(paddr, &row,&col,&bank,
							  &rank,&channel,&subarray);
		request->address.SetPhysicalAddress(paddr);
		request->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray);
		//std::cout<<"is migrated, org:"<<(request->address.GetVirtualAddress()>>6)<<
			//" real:"<<(paddr>>6)<<std::endl;
	}
	return flat_memory->IsIssuable(request);
}

bool FlatRBLANVMain::IssueCommand( NVMainRequest *request)
{
	uint64_t cur_cycle = GetEventQueue()->GetCurrentCycle();
	//every update_interval ,call function "AdjustMigrateThres" adjust migra_thres
	if( cur_cycle - last_cycle > update_interval)
	{
		AdjustMigrateThres();
		statsTable->Clear(); //refresh row buffer miss times of all entries
		//clean_rb_miss_=0;
		//dirty_rb_miss_ = 0;
		//nvm_clean_rb_miss_=0;
		//nvm_dirty_rb_miss_=0;
		//pre_migration_cycles = FlatMigrator::migration_cycles;

		//FlatMigrator::migration_cycles = 0;
		last_cycle = cur_cycle;
		//std::cout<<"instrs:"<<zinfo->cores[0]->getInstrs()<<" cycles:"<<cur_cycle<<" migrated access:"<<migrated_access<<std::endl;
	}
   return flat_memory->IssueCommand(request); 
}

bool FlatRBLANVMain::RequestComplete( NVMainRequest *request )
{
    bool rv = false;
    if( request->owner == this )
    {
        if( request->isPrefetch )
        {
            /* Place in prefetch buffer. */
            if( prefetchBuffer.size() >= p->PrefetchBufferSize )
            {
                unsuccessfulPrefetches++;
                delete prefetchBuffer.front();
                prefetchBuffer.pop_front();
            }
            prefetchBuffer.push_back( request );
            rv = true;
        }
        else if(request->isMigration)
        {
			migrator->RequestComplete(request);
            delete request;
            rv = true;
        }
		else
		{
			delete request;
			rv = true;
		}
    }
    else
    {
		//if row buffer miss , modify stats table and decide whether to migrate
		if(request->rbHit == false )
		{
			row_buffer_miss++;
			if( request->media == SLOW_MEM )
			{
				bool ret = false;
				uint64_t row_num = GetRowNum( request );
				if( request->type==READ || request->type==READ_PRECHARGE) 
					ret = UpdateStatsTable( row_num , read_incre );
				if(request->type==WRITE || request->type==WRITE_PRECHARGE)
					ret = UpdateStatsTable( row_num , write_incre);
				//can migration
				if( ret )
				 {
					if( migrator)
					{
						//std::cout<<"can migration :"<<(request->address.GetPhysicalAddress())<<" migra thres:"<<migra_thres<<std::endl;
					   //create new request,issue to Migrator
					   NVMainRequest *req = new NVMainRequest();
					   *req = *request;
					   req->type = MIGRATION;
					   req->owner = this;
					   migrator->IssueCommand(req);
					   migrated_pages_++;
				   }
				}
			}
		}
		else
			row_buffer_hits++;
	
		if( (request->is_migrated))
		{
			if( request->media == FAST_MEM)
			{
				migrated_access++;	
				if( request->rbHit == false )
				{
					if( request->dirty_miss )
						dirty_rb_miss_++;	
					else
						clean_rb_miss_++;
				}
			}
			else if( request->media == SLOW_MEM)
			{
				if( request->rbHit == false )
				{
					if( request->dirty_miss)
						nvm_dirty_rb_miss_++;
					else
						nvm_clean_rb_miss_++;
				}
			}
		}
		rv = GetParent( )->RequestComplete( request );
	}
    return rv;
}

void FlatRBLANVMain::Cycle( ncycle_t steps )
{
	  flat_memory->Cycle(steps);
	  //every update_interval ,call function "AdjustMigrateThres" adjust migra_thres
}

void FlatRBLANVMain::RegisterStats( )
{
	AddStat(migrated_pages_);
	AddStat( statsHit );
	AddStat( statsMiss);
	AddStat( statsHitRate );
	AddStat( row_buffer_hits);
	AddStat( row_buffer_miss);
	AddStat( rb_hitrate);
	AddStat( dirty_rb_miss_);
	AddStat( clean_rb_miss_);
	AddStat( nvm_dirty_rb_miss_);
	AddStat( nvm_clean_rb_miss_);
	AddStat( total_access_time);
	AddStat( total_fast_access_time);
	AddStat( total_slow_access_time);
	AddStat( accessed_fast_page_count);
	AddStat( accessed_slow_page_count);
	AddStat( fast_memory_usage );
	AddStat( per_fast_page_access );
	AddStat( migrated_access);
	NVMain::RegisterStats();
}

void FlatRBLANVMain::CalculateStats( )
{
	statsHitRate = double(statsHit)/double(statsHit+statsMiss);
	if( row_buffer_hits + row_buffer_miss >0)
		rb_hitrate = (double)row_buffer_hits/(row_buffer_hits + row_buffer_miss);
	for( unsigned int i =0; i< numChannels ; i++)
	    memoryControllers[i]->CalculateStats( );
	NVMain::CalculateStats();
	flat_memory->CalculateStats();
	totalReadRequests = flat_memory->totalReadRequests;
	totalWriteRequests = flat_memory->totalWriteRequests;
	total_access_time = totalReadRequests + totalWriteRequests;

	total_fast_access_time = flat_memory->GetFastMemory()->totalReadRequests
		+ flat_memory->GetSlowMemory()->totalWriteRequests;
	total_slow_access_time = flat_memory->GetSlowMemory()->totalReadRequests
		+ flat_memory->GetFastMemory()->totalWriteRequests;
	accessed_fast_page_count = flat_memory->GetFastPageCount();
	accessed_slow_page_count = flat_memory->GetSlowPageCount();
	fast_memory_usage = (double)accessed_fast_page_count/(double)flat_memory->GetFastMemory()->GetMemorySize()*(1<<12);
	if( accessed_fast_page_count >0 )
		per_fast_page_access = total_fast_access_time/accessed_fast_page_count;
}

/////////////////////////////////////////////////////////added on 2015/5/4
/*
 * function : update stats table when row buffer miss 
 * @row_num : row address (key of stats table)
 * @incre_num : when row buffer miss , increment num of miss_time
 *
 */
bool FlatRBLANVMain::UpdateStatsTable ( uint64_t row_num , uint64_t incre_num)
{
	bool can_migration = false;
	uint64_t entry_id;
	StatsStoreBlock* stat_blk;
	//stats table hit
	if( (stat_blk = statsTable->FindEntry(row_num)))
	{
		statsHit++;
	}
	//stats table miss
	else
	{
		stat_blk = statsTable->FindVictim(entry_id);
		statsTable->Install(stat_blk , row_num);
		statsMiss++;
	}
	if(stat_blk)
	{
		statsTable->IncreMissTime( stat_blk , incre_num );
		if(stat_blk->miss_time >= migra_thres)
		{
			stat_blk->miss_time = 0;
			can_migration = true;
		}
	}
	return can_migration;
}


void FlatRBLANVMain::AdjustMigrateThres()
{
	double net_benefit = CalculateBenefit();
	double delta = net_benefit - pre_net_benefit;
	if( pre_action == INIT)
	{
		if( net_benefit > 0)
		{
			pre_action = DECREMENT;
			pre_net_benefit = net_benefit;
			if( migra_thres > 0)
				migra_thres--;
		}
		else
		{
			pre_action = INCREMENT;
			migra_thres += 1;
			pre_net_benefit = net_benefit;
		}
		return;
	}

	if( net_benefit < 0)
	{
		migra_thres +=3;
		pre_action = INCREMENT;
	}
	if( delta > 0)
	{
		if( pre_action == INCREMENT)
		{
			migra_thres++;
			pre_action = INCREMENT;
		}
		else if( pre_action == DECREMENT && migra_thres>0)
		{
			migra_thres--;
			pre_action = DECREMENT;
		}
	}
	else if(delta <= 0 )
	{
		if(pre_action == INCREMENT && migra_thres>0)
		{
			migra_thres--;
			pre_action = DECREMENT;
		}
		else if( pre_action == DECREMENT)
		{
			migra_thres++;
			pre_action = INCREMENT;
		}
	}
	//std::cout<<"netbenefit:"<<net_benefit<<" delta:"<<delta<<" migrate threshod is:"<<migra_thres<<std::endl;
	pre_net_benefit = net_benefit;
}


double FlatRBLANVMain::CalculateBenefit()
{
	
	double benefit = 0;
	benefit = ((int64_t)(dirty_rb_miss_-nvm_dirty_rb_miss_))*tdiff_dirty_ +
		(int64_t)((clean_rb_miss_-nvm_clean_rb_miss_))* tdiff_clean_
		- (int64_t)(FlatMigrator::migration_cycles);
	//std::cout<<"migration cycles:"<<FlatMigrator::migration_cycles<<std::endl;
	//std::cout<<"dram dirty rb miss:"<<dirty_rb_miss_<<" , dram clean rb miss:"<<clean_rb_miss_<<" nvm dirty rb miss:"<<nvm_dirty_rb_miss_<<" , nvm clean rb miss:"<<nvm_clean_rb_miss_<<std::endl;
	return benefit;
}

uint64_t FlatRBLANVMain::GetRowNum(NVMainRequest* req)
{
	uint64_t row , col , rank , bank , channel , subarray;
	flat_memory->GetSlowMemory()->GetDecoder()->Translate(req->address.GetPhysicalAddress(), &row, &col,&bank,&rank, &channel, &subarray);
	return flat_memory->GetSlowMemory()->GetDecoder()->ReverseTranslate(row , 0 , bank , rank , channel , subarray);
}

void FlatRBLANVMain::SetMigrator( NVMObject* migra){
		migrator = dynamic_cast<FlatMigrator*>(migra);
		migrator->SetParent(this);
		AddChild(migrator);
}
