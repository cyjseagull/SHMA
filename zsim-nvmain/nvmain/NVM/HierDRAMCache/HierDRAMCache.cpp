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

#include "NVM/HierDRAMCache/HierDRAMCache.h"
#include "MemControl/MemoryControllerFactory.h"
#include "Interconnect/InterconnectFactory.h"
#include "include/NVMHelpers.h"
#include "NVM/NVMainFactory.h"

#include "Decoders/DRCDecoder/DRCDecoder.h"
#include "src/EventQueue.h"

#include <iostream>
#include <sstream>
#include <cassert>
#include <cstdlib>

using namespace NVM;

HierDRAMCache::HierDRAMCache( ):NVMain(),
					drc_evicts(0), drc_fetches(0),
					ranks(0),banks(0),rows(0),cols(0),
						offset_word(0),assoc(0),lineSize(0),
						lineSizeOff(0),set_num(0),
						total_drc_size(0),
						drc_hits(0),drc_miss(0),
						write_hits(0),write_miss(0),
						read_hits(0),read_miss(0),
						rb_hits(0),rb_miss(0),
						caching_cycles(0),
						drc_hit_rate(0.0),drc_miss_rate(0.0),
						drc_read_hit_rate(0.0),drc_read_miss_rate(0.0),
						drc_write_hit_rate(0.0),drc_write_miss_rate(0.0)
{
    drcChannels = NULL;
	drcConfig = NULL;
    numChannels = 0;
	drc_dirty_evicts = 0;
	cache_line_size = 64;
	total_access_time = 0;
	mem_type = "NVMain";
	mainMemory = NULL;
	memory_size = 0;
}

HierDRAMCache::~HierDRAMCache( )
{
	if(dram_cache)
		delete dram_cache;
}

void HierDRAMCache::SetConfig(Config *conf,std::string mem_name,  bool createChildren )
{
	std::cout<<"set config for HierDRAMCache"<<std::endl;
    Params *params = new Params( );
    params->SetParams( conf );
    SetParams( params );
    StatName( "hierarch_dram" );
    config = conf;
    if( config->GetSimInterface( ) != NULL )
        config->GetSimInterface( )->SetConfig( conf, createChildren );
    /* Initialize DRAM Cache channels */
    numChannels = static_cast<ncounter_t>( conf->GetValue( "CHANNELS" ) );
	if(conf->KeyExists("CacheLineSize"))
		cache_line_size = NVM::TransToBytes(conf->GetString("CacheLineSize"));
	std::cout<<"cache line size is "<<cache_line_size<<std::endl;
    if( createChildren )
    {
        /* Initialize off-chip memory */
        std::string configFile;
        Config *mainMemoryConfig;

        configFile  = NVM::GetFilePath( conf->GetFileName( ) );
        configFile += conf->GetString( "MM_CONFIG" );
		//load main memory config , create main memory event queue , set main memory parent to this object , add main memory to system
        mainMemoryConfig = new Config( );
        mainMemoryConfig->Read( configFile );
		if(mainMemoryConfig->KeyExists("CMemType"))
			mem_type = mainMemoryConfig->GetString("CMemType");
		mainMemory =  NVMainFactory::CreateNewNVMain(mem_type);
        EventQueue *mainMemoryEventQueue = new EventQueue( );
        mainMemory->SetParent( this ); 
        mainMemory->SetEventQueue( mainMemoryEventQueue );
		//set main memory to system run at subsystem frequency
        GetGlobalEventQueue( )->AddSystem( mainMemory, mainMemoryConfig);
		//main memory name is "offChipMemory"
        mainMemory->SetConfig( mainMemoryConfig, "MainMemory", createChildren );
		memory_size = mainMemory->GetMemorySize();
        /* Orphan the interconnect created by NVMain */
        std::vector<NVMObject_hook *>& childNodes = GetChildren( );
        childNodes.clear();

		std::cout<<"drcChannels is:"<<numChannels<<std::endl;
        drcChannels = new MemoryController*[numChannels];
        int subarrays;
        if( conf->KeyExists( "MATHeight" ) )
        {
             rows = conf->GetValue( "MATHeight" );
             subarrays = conf->GetValue( "ROWS" ) / conf->GetValue( "MATHeight" );
        }
        else
        {
            rows = conf->GetValue( "ROWS" );
            subarrays = 1;
        }

        cols = conf->GetValue( "COLS" );
        banks = conf->GetValue( "BANKS" );
        ranks = conf->GetValue( "RANKS" );
		ncounter_t tburst = static_cast<uint64_t>(conf->GetValue("tBURST"));
		ncounter_t buswidth = static_cast<uint64_t>(conf->GetValue("BusWidth"));
		offset_word = (tburst*buswidth)/8;
		total_drc_size = ranks*banks*rows*cols*offset_word;	 
		//get set and cachelinesize config
		//default associate is 256, cache line size is 4KB(page size)
		assoc = 256;
		lineSize=4096;
		if( conf->KeyExists("ASSOC"))
		assoc = conf->GetValue("ASSOC");
		if( conf->KeyExists("BufferLineSize"))
		lineSize = conf->GetValue("BufferLineSize");
		lineSizeOff=NVM::Log2(lineSize); 
		ncounter_t assoc_bit = NVM::Log2(assoc);
		set_off_bit = lineSizeOff + assoc_bit;
		//get total set num
		set_num = total_drc_size/(assoc*lineSize);
		//create cache
		dram_cache = new CacheBank(set_num,assoc,lineSize);
		//calculate burst_count
		burst_count = lineSize/offset_word;
        TranslationMethod *drcMethod = new TranslationMethod();
        drcMethod->SetBitWidths( NVM::mlog2( rows ),
                                 NVM::mlog2( cols ),
                                 NVM::mlog2( banks ),
                                 NVM::mlog2( ranks ),
								 NVM::mlog2( numChannels ),
								 NVM::mlog2( subarrays ) );
        drcMethod->SetCount( rows, cols, banks, ranks, numChannels, subarrays );
		drcMethod->SetAddressMappingScheme(conf->GetString("AddressMappingScheme"));
        /* When selecting a child, use the channel field from a DRC decoder. */
		std::string decoder_str="";
		if( conf->KeyExists("Decoder"))
			decoder_str = config->GetString("Decoder");
		AddressTranslator* decoder = DecoderFactory::CreateNewDecoder(decoder_str);
		decoder->SetConfig(conf,createChildren);
		decoder->SetTranslationMethod(drcMethod);
		decoder->SetDefaultField(CHANNEL_FIELD);
		SetDecoder(decoder);

        /* Initialize a DRAM cache channel. */
        std::stringstream formatter;
		drcConfig = new Config* [numChannels];
        for( ncounter_t i = 0; i < numChannels; i++ )
        {
			std::stringstream configString;
			std::string drc_config_file;
			drcConfig[i] = new Config( *conf );
			configString<<"DRC_CHANNEL"<<i;
			if( conf->GetString(configString.str())!="")
			{
				drc_config_file = conf->GetString( configString.str());
				if( drc_config_file[0]!='/')
				{
					drc_config_file = NVM::GetFilePath( conf->GetFileName());
					drc_config_file += conf->GetString( configString.str() );
				}
			   drcConfig[i]->Read(drc_config_file);
			}
            drcChannels[i] =  MemoryControllerFactory::CreateNewController(drcConfig[i]->GetString( "MEM_CTL" ));
            formatter.str( "" );
            formatter << StatName( ) << ".L" << i<<"_cache."<<drcConfig[i]->GetString("MEM_CTL");
            drcChannels[i]->SetID( static_cast<int>(i) );
            drcChannels[i]->StatName( formatter.str() ); 
            AddChild( drcChannels[i] );
			drcChannels[i]->SetParent( this );
			drcChannels[i]->SetConfig( drcConfig[i],createChildren );
            drcChannels[i]->RegisterStats( );
        }
		   RegisterStats();
		   AddChild( mainMemory );
    }
    /* DRC Variant will call base SetConfig */
	dram_miss_t_ = conf->GetValue("tRP") + conf->GetValue("tRCD")
		                      + conf->GetValue("tCAS") + conf->GetValue("tRTP");
    SetDebugName( "DRAMCache", conf );
}


uint64_t HierDRAMCache::GetCacheLineSize()
{
	return cache_line_size;
}

void HierDRAMCache::RegisterStats( )
{
	NVMain::RegisterStats();
	AddStat(drc_hits);
	AddStat(drc_miss);
	AddStat(read_hits);
	AddStat(read_miss);
	AddStat(write_hits);
	AddStat(write_miss);
	AddStat(drc_evicts);
	AddStat(drc_dirty_evicts);
	AddStat(drc_fetches);
	AddStat(total_drc_size);
	AddStat(assoc);
	AddStat(lineSize);
	AddStat(set_num);
	AddStat(drc_hit_rate);
	AddStat(drc_miss_rate);
	AddStat(drc_read_hit_rate);
	AddStat(drc_read_miss_rate);
	AddStat(drc_write_hit_rate);
	AddStat(drc_write_miss_rate);
	AddStat(caching_cycles);
}

inline void HierDRAMCache::Retranslate(NVMainRequest* req, uint64_t address )
{
    uint64_t col, row, bank, rank, chan, subarray;

    GetDecoder()->Translate( address , &row, &col,
							&bank, &rank, &chan, &subarray );
    req->address.SetTranslatedAddress( row, col, bank, rank, chan, subarray );
}

//TODO: IssueAtomic is not implemented yet
bool HierDRAMCache::IssueAtomic( NVMainRequest *req )
{
	return true;
}

bool HierDRAMCache::IsIssuable( NVMainRequest *req, FailReason *reason )
{
	ncounter_t row, col, rank, bank, channel, subarray;
	uint64_t set_id, assoc_id;
	assert( req != NULL );
	bool hit = dram_cache->Present(req->address , set_id , assoc_id);
	ncounter_t offset_addr = req->address.GetPhysicalAddress()%lineSize;
	ncounter_t cache_addr;
	bool ret = false;
	if( hit )
	{
		ConvertToCacheAddress( set_id , assoc_id , offset_addr , cache_addr);
		req->address.SetCacheAddress( cache_addr );
		Retranslate(req , cache_addr);
		uint64_t drc_channel;
		req->address.GetTranslatedAddress(NULL,NULL,NULL,NULL,&drc_channel, NULL);
		assert( drc_channel < numChannels);
		assert(GetChild(req)->GetTrampoline() == drcChannels[drc_channel]);
		ret = drcChannels[drc_channel]->IsIssuable(req , reason);
	}
	if( hit )
		return ret;
	else
	{
		mainMemory->GetDecoder( )->Translate( req->address.GetPhysicalAddress( ),&row, &col, &rank, &bank, &channel, &subarray );
		bool mem_issuable = mainMemory->GetChild(channel)->IsIssuable(req, reason);
		return (mem_issuable);
	}
}

bool HierDRAMCache::IssueCommand( NVMainRequest *req )
{
	total_access_time++;
	req->arrivalCycle = GetEventQueue()->GetCurrentCycle();
	if( req->type==READ || req->type==READ_PRECHARGE)
		totalReadRequests++;
	else
		totalWriteRequests++;
	uint64_t set_id , assoc_id;
	bool hit = false;
	bool set_dirty = false;
	if( req->type==WRITE || req->type == WRITE_PRECHARGE)
		set_dirty = true;
	hit = dram_cache->Present(req->address , set_id , assoc_id , set_dirty);
	//dram buffer hit
	if( hit)
	{
			uint64_t channel;
			req->address.SetPhysicalAddress( req->address.GetCacheAddress());
			req->address.GetTranslatedAddress(NULL,NULL,NULL,NULL,&channel, NULL);
			assert( channel < numChannels);
			assert(GetChild(req)->GetTrampoline() == drcChannels[channel]);
			drc_hits++;
			if(req->type==READ || req->type==READ_PRECHARGE)
				read_hits++;
			else if( req->type ==WRITE || req->type==WRITE_PRECHARGE)
			{
				write_hits++;
			}
			req->bulkCmd = CMD_NOP;
			//issue command to channel
			if( drcChannels[channel]->IsIssuable(req,NULL))
				drcChannels[channel]->IssueCommand(req);
		}
		//dram buffer miss
		else	
		{
			bool dirty = false;
			NVMAddress victim;
			//evict data block when set is full,issue evict request
			if( dram_cache->SetFull(req->address))
			{
				dirty = dram_cache->ChooseVictim( req->address, &victim);
				drc_evicts++;
				if(dirty)
				{
					NVMainRequest* evict_req = new NVMainRequest();
					evict_req->address = victim; 
					uint64_t set_id , assoc_id;
					dram_cache->Evict(victim , set_id , assoc_id);	//evict entry
					evict_req->tag = HierDRC_EVICT;
					evict_req->type = WRITE;
					evict_req->burstCount = burst_count;
					evict_req->owner = this;
					drc_dirty_evicts++;
					assert(outstandingEvicts.count(req)==0);
					outstandingEvicts[evict_req] = req;
					//issue evict request to main memory
					mainMemory->IssueCommand(evict_req);
				}
			}
			NVMainRequest *memReq = new NVMainRequest();
			*memReq = *req;
			memReq->owner = this;
			memReq->burstCount = burst_count;
			memReq->tag = HierDRC_MEMREAD;
			memReq->type = READ;
			assert(outstandingMemReads.count(memReq)==0);
			outstandingMemReads[memReq] = req; //record original request
			if( mainMemory->IsIssuable( memReq , NULL) )
				mainMemory->IssueCommand(memReq);
			//issue Fill request to dram cache
			NVMainRequest* fillReq = new NVMainRequest();
			*fillReq = *req;
			fillReq->address.SetPhysicalAddress(req->address.GetCacheAddress() );
			fillReq->owner = this;
			fillReq->tag = HierDRC_FILL;
			fillReq->type = WRITE;
			fillReq->burstCount = burst_count;
			assert( cachingMap.count(req)==0);
			cachingMap[req].first_bool = false;
			cachingMap[req].second_bool = false;
			outstandingFills[fillReq] = req;
			uint64_t channel,bank, rank, col, row, subarray;

			//fillReq->address.GetTranslatedAddress(NULL,NULL,NULL,NULL,&channel,NULL);
			GetDecoder()->Translate( req->address.GetCacheAddress() , &row, 
					&col,&bank, &rank, &channel, &subarray );
		    req->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray );

			assert(channel< numChannels);
			assert(GetChild(fillReq)->GetTrampoline()==drcChannels[channel]);
			if( drcChannels[channel]->IsIssuable(fillReq, NULL))
				drcChannels[channel]->IssueCommand(fillReq);
		drc_miss++;
		if( req->type==WRITE || req->type==WRITE_PRECHARGE)
			write_miss++;
		else if( req->type==READ || req->type==READ_PRECHARGE)
			read_miss++;
	}
	return true;
}

bool HierDRAMCache::IssueFunctional( NVMainRequest *req )
{
	return true;
}

bool HierDRAMCache::RequestComplete( NVMainRequest *req )
{
	bool ret = false;
	//request is issued from cache
	if( req->owner == this)
	{
		//memory read operation
		if( req->tag == HierDRC_MEMREAD)
		{
			drc_fetches++;
			/*if( drc_fetches%100 == 0)
			{
			}*/
			//erase request from fill queues
			assert(outstandingMemReads.count(req));
			NVMainRequest* origin_req = outstandingMemReads[req];
			outstandingMemReads.erase(req);
			cachingMap[origin_req].first_bool = true;
			if( cachingMap[origin_req].second_bool)
			{
				dram_cache->Install(origin_req->address);
				caching_cycles += GetEventQueue()->GetCurrentCycle()-origin_req->arrivalCycle;
				cachingMap.erase(origin_req);
				GetParent()->RequestComplete(origin_req);
			}
		}
		else if( req->tag == HierDRC_FILL)
		{
			assert(outstandingFills.count(req));
			NVMainRequest* origin_req = outstandingFills[req];
			outstandingFills.erase(req);
			cachingMap[origin_req].second_bool = true;
			if( cachingMap[origin_req].first_bool)
			{
				caching_cycles += GetEventQueue()->GetCurrentCycle()-origin_req->arrivalCycle;
				dram_cache->Install(origin_req->address);
				cachingMap.erase(origin_req);
				GetParent()->RequestComplete(origin_req);
			}
		}
		else if( req->tag == HierDRC_EVICT )
		{
			assert(outstandingEvicts.count(req));
			outstandingEvicts.erase(req);
		}
		ret = true;
		delete req;
	}
	else
	{
		ret = GetParent()->RequestComplete(req);
	}
	return ret;
}

void HierDRAMCache::Cycle( ncycle_t steps )
{
	assert(!p->EventDriven);
    uint64_t i;
    for( i = 0; i < numChannels; i++ )
    {
        drcChannels[i]->Cycle( steps );
    }
    mainMemory->Cycle( steps );
	GetEventQueue()->Loop(steps);
}

void HierDRAMCache::CalculateStats( )
{
    uint64_t i;
	NVMain::CalculateStats();
    for( i = 0; i < numChannels; i++ )
    {
        drcChannels[i]->CalculateStats( );
    }
    mainMemory->CalculateStats( );
	mainMemory->totalReadRequests += drc_fetches;
	mainMemory->totalWriteRequests += drc_evicts;
	totalWriteRequests += drc_fetches;
	totalReadRequests += drc_evicts;
	ncounter_t total_access_time = drc_hits + drc_miss;
	drc_hit_rate = (double)drc_hits/(double)total_access_time;
	drc_miss_rate = 1 - drc_hit_rate;
	drc_read_hit_rate = (double)read_hits/(double)total_access_time;
	drc_read_miss_rate = (double)read_miss/(double)total_access_time;
	drc_write_hit_rate = (double)write_hits/(double)total_access_time;
	drc_write_miss_rate = (double)write_miss/(double)total_access_time;
}

NVMain* HierDRAMCache::GetMainMemory( )
{
    return mainMemory;
}
