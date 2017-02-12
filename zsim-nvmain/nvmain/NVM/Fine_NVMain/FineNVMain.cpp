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
#include "NVM/Fine_NVMain/FineNVMain.h"
#include "src/EventQueue.h"
#include "MemControl/MemoryControllerFactory.h"
#include "traceWriter/TraceWriterFactory.h"
#include "Decoders/DecoderFactory.h"
#include "Decoders/BufferDecoder/BufferDecoder.h"
#include "include/NVMainRequest.h"
#include "include/NVMHelpers.h"
#include "Prefetchers/PrefetcherFactory.h"
#include <vector>

#include <sstream>
#include <cassert>

#include "galloc.h"
#include "pad.h"
using namespace std;
using namespace NVM;

unsigned int FineNVMain::numChannels;
FineNVMain::FineNVMain( )
{
    config = NULL;
	mem_size = 0;
	mem_word_size = 0;
	cache_size = 0;
	cache_word_size = 0;

	reserved_channels = 0;
	reservedControllers = NULL;
	cacheTranslator = NULL;
	translator = NULL;

	channelConfig = NULL;
	reservedConfig = NULL;
	memoryControllers = NULL;
	prefetcher = NULL;
	preTracer = NULL;
	reserve_region_param = NULL;
	block_fetcher = NULL;
	syncValue = 0.0;
	is_configed = false;
	access_time = 0;
	dram_buffer_read = 0;
	dram_buffer_write = 0;
	pcm_read = 0;
	pcm_write = 0;
	drc_read_hits = 0;
	drc_write_hits = 0;
	drcReadRequests = 0;
	drcWriteRequests = 0;
	nvmReadRequests =0;
	nvmWriteRequests =0;
	hit_rate = 0.0;
	/*******threshold adjustment related information*****/
	rb_clean_miss_time = rb_dirty_miss_time = rb_hit_time = 0;
	total_caching_t =0;
	dram_access_time = 0;
	pcm_access_time = 0;
	total_access_time = 0;
	t_fast_read = 0;
	t_slow_read = 0;
	t_slow_write = 0;
	clflush_wb_overhead = 0;
	futex_init( &issue_lock );
}


FineNVMain::~FineNVMain( )
{
    if( config ) 
        delete config;
    
    if( memoryControllers )
    {
        for( unsigned int i = 0; i < numChannels; i++ )
        {
            if( memoryControllers[i] )
                delete memoryControllers[i];
        }

        delete [] memoryControllers;
    }

	if( reservedControllers)
	{
		for( uint64_t i=0 ; i < reserved_channels;i++)
			delete reservedControllers[i];
		delete []reservedControllers;
	}

    if( channelConfig )
    {
        for( unsigned int i = 0; i < numChannels; i++ )
        {
            delete channelConfig[i];
        }

        delete [] channelConfig;
    }

	if( reservedConfig)
	{
		for( uint64_t i=0 ; i<reserved_channels ; i++)
			delete reservedConfig[i];
		delete [] reservedConfig;
	}
	if( reserve_region_param )
	{
		delete reserve_region_param;
	}
	if(cacheTranslator)
		delete cacheTranslator;
}


Config *FineNVMain::GetConfig( )
{
    return config;
}


void FineNVMain::SetConfig( Config *conf, std::string memoryName, bool createChildren )
{
	Params* params = new Params( );
    params->SetParams( conf );
    SetParams( params );

    StatName( memoryName );
    config = conf;
    
	if( config->GetSimInterface( ) != NULL )
        config->GetSimInterface( )->SetConfig( conf, createChildren );
    else
      std::cout << "Warning: Sim Interface should be allocated before configuration!" << std::endl;
    if( createChildren )
    {
		if( conf->KeyExists( "Decoder" ) )
		{
		    translator = DecoderFactory::CreateNewDecoder( conf->GetString("Decoder"));
			//BufferDecoder* decoder = gm_memalign<NVM::BufferDecoder>(CACHE_LINE_BYTES, 1);
		   //translator = new (decoder) BufferDecoder();	
		}
		else
		{
			std::cout<<"create address translator"<<std::endl;
			AddressTranslator* decoder = gm_memalign<NVM::AddressTranslator>(CACHE_LINE_BYTES, 1);
			translator = new (decoder) AddressTranslator();
		}
		SetDecoder( translator);
        uint64_t channels = static_cast<int>(p->CHANNELS);
		reserved_channels = static_cast<int>( p->ReservedChannels);
		
		std::cout<<"reserved_channel num:"<<reserved_channels<<std::endl;
		std::cout<<"channels:"<<channels<<std::endl;
        memoryControllers = new MemoryController* [channels];
        channelConfig = new Config* [channels];	

		SetMemoryControllers( memoryControllers ,channelConfig,
							  conf , channels ,
							  "CONFIG_CHANNEL" ,"MainMemoryController" );
		p->SetParams(channelConfig[0]);
		t_slow_read = GetReadLatency(p);
		t_slow_write = GetWriteLatency(p);
		SetTranslators( channelConfig[0] ,translator , p , mem_width ,mem_word_size, mem_size);

		if( reserved_channels )
		{
			std::string buffer_decoder = "BufferDecoder";
			if( conf->KeyExists("DRAMBufferDecoder"))
			{
				buffer_decoder = conf->GetString("DRAMBufferDecoder");
				//cacheTranslator = DecoderFactory::CreateNewDecoder( buffer_decoder);
				std::cout<<"create buffer decoder"<<std::endl;
				BufferDecoder* decoder = gm_memalign<NVM::BufferDecoder>(CACHE_LINE_BYTES, 1);
				cacheTranslator = new (decoder) BufferDecoder();	
				SetBufferDecoder(cacheTranslator);
			}
			reservedControllers = new MemoryController* [reserved_channels];
			reservedConfig = new Config* [reserved_channels];
			reserve_region_param = new Params;
			SetMemoryControllers(reservedControllers , reservedConfig , 
								 conf , reserved_channels , 
								 "CONFIG_DRAM_CHANNEL" , "DRAMBufferControllers");

			reserve_region_param->SetParams(reservedConfig[0]);	//set reserve region param
			t_fast_read = GetReadLatency( reserve_region_param);

			for( unsigned i=0; i<reserved_channels;i++)
				reservedControllers[i]->SetParams(reserve_region_param);
			SetTranslators(	reservedConfig[0] ,cacheTranslator , 
							reserve_region_param , cache_width,
							cache_word_size , cache_size);
			std::cout<<"cache buffer size is "<<std::hex<<cache_size<<std::endl;
			cacheTranslator->SetAddrWidth(cache_width,mem_width );
			std::cout<<"mem col size: "<<mem_word_size<<"Bytes"<<std::endl;
			std::cout<<"cache col size: "<<cache_word_size<<"Bytes"<<std::endl;

			(dynamic_cast<BufferDecoder*>(cacheTranslator))->SetMemColBytes( mem_word_size);
			(dynamic_cast<BufferDecoder*>(cacheTranslator))->SetBufferColBytes( cache_word_size);
			for( unsigned i=0; i<reserved_channels; i++)
			{
				BufferDecoder* cache_dec = dynamic_cast<BufferDecoder*>(reservedControllers[i]->GetDecoder());
				assert(cache_dec);
				cache_dec->SetAddrWidth(cache_width, mem_width);
				cache_dec->SetMemColBytes(mem_word_size);
				cache_dec->SetBufferColBytes(cache_word_size);
			}
		}
	}

	if( p->MemoryPrefetcher != "none" )
	{
		prefetcher = PrefetcherFactory::CreateNewPrefetcher( p->MemoryPrefetcher );
		std::cout << "Made a " << p->MemoryPrefetcher << " prefetcher." << std::endl;
	}

    numChannels = static_cast<unsigned int>(p->CHANNELS);
    std::string pretraceFile;
    if( p->PrintPreTrace || p->EchoPreTrace )
    {
        if( config->GetString( "PreTraceFile" ) == "" )
            pretraceFile = "trace.nvt";
        else
            pretraceFile = config->GetString( "PreTraceFile" );

        if( pretraceFile[0] != '/' )
        {
            pretraceFile  = NVM::GetFilePath( config->GetFileName( ) );
            pretraceFile += config->GetString( "PreTraceFile" );
        }

        std::cout << "Using trace file " << pretraceFile << std::endl;

        if( config->GetString( "PreTraceWriter" ) == "" )
            preTracer = TraceWriterFactory::CreateNewTraceWriter( "NVMainTrace" );
        else
            preTracer = TraceWriterFactory::CreateNewTraceWriter( config->GetString( "PreTraceWriter" ) );

        if( p->PrintPreTrace )
            preTracer->SetTraceFile( pretraceFile );
        if( p->EchoPreTrace )
            preTracer->SetEcho( true );
    }
	/***********get time params of main memory and dram buffer*******/
	if( reserved_channels )
	{
		assert( memoryControllers[0]&& reservedControllers[0]);
		delta_clean_miss_t = memoryControllers[0]->GetMissCycles() - 
								  reservedControllers[0]->GetMissCycles();
		delta_dirty_miss_t = memoryControllers[0]->GetDirtyMissCycles()-
								  reservedControllers[0]->GetDirtyMissCycles();
		delta_hit_t = memoryControllers[0]->GetHitCycles()-
						   reservedControllers[0]->GetHitCycles();
	}
	/************************************************/
    RegisterStats();
	is_configed = true;
}

inline unsigned FineNVMain::GetReadLatency( Params* p)
{
	//tAL: append latency
	//tCAS: col activate
	//tRAS: row activate
	//tRDPDEN: wait for read to complete
	std::cout<<"AL:"<<(p->tAL)<<" CAS:"<<(p->tCAS)<<
		" tRDPDEN:"<<(p->tRDPDEN)<<std::endl;

	return (p->tAL + p->tRDPDEN);
}

inline unsigned FineNVMain::GetWriteLatency( Params* p)
{
	std::cout<<"get write latency, AL:"<<p->tAL<<" CAS:"
			 <<p->tCAS<<" WRPDEN:"<<
			 p->tWRPDEN<<std::endl;
	return (p->tAL + p->tCAS + p->tWRPDEN);
}

void FineNVMain::SetTranslators( Config* conf , AddressTranslator* &translator,
								 Params* param , unsigned &bit_width ,
								 unsigned &word_size , uint64_t &mem_size )
{
	uint64_t cols , rows , banks , ranks , channels , subarrays;
	unsigned int col_bits, row_bits , bank_bits , rank_bits , channel_bits , subarray_bits;
	unsigned tburst, buswidth;
	//rate;
	if( !param )
		return;
	if( !translator )
	{
		if( conf->KeyExists( "Decoder" ) )
			translator = DecoderFactory::CreateNewDecoder( conf->GetString("Decoder"));
		else
			translator = new AddressTranslator();
	}
	
	cols = static_cast<uint64_t>(param->COLS);
	rows = static_cast<uint64_t>(param->ROWS);
	banks = static_cast<uint64_t>(param->BANKS);
	ranks = static_cast<uint64_t>(param->RANKS);
	channels = static_cast<uint64_t>(param->CHANNELS);
	if( conf->KeyExists("MATHeight"))
	{
		uint64_t mat_height = static_cast<uint64_t>( param->MATHeight);
		subarrays =	static_cast<uint64_t>(mat_height/rows); 
	}
	else
		subarrays = 1;
	tburst = param->tBURST;
	if( conf->KeyExists("tBURST"))
	{
		tburst = conf->GetValue("tBURST");
		param->tBURST = tburst;
	}
	buswidth = param->BusWidth;
	if( conf->KeyExists("BusWidth"))
	{
		buswidth = conf->GetValue("BusWidth");
		param->BusWidth = buswidth;
	}
	col_bits = NVM::mlog2(cols);
	row_bits = NVM::mlog2(rows);
	bank_bits = NVM::mlog2(banks);
	rank_bits = NVM::mlog2(ranks);
	channel_bits = NVM::mlog2(channels);
	subarray_bits = NVM::mlog2(subarrays);
	TranslationMethod* tmp_method = gm_memalign<TranslationMethod>(CACHE_LINE_BYTES,1);
	TranslationMethod *method = new (tmp_method) TranslationMethod();
	bit_width = static_cast<uint64_t>(col_bits + row_bits + bank_bits
									  + rank_bits + channel_bits + subarray_bits );
	method->SetBitWidths( row_bits , col_bits ,
						  bank_bits , rank_bits,
						  channel_bits , subarray_bits );
	method->SetCount( rows , cols , banks , ranks , channels , subarrays);
	method->SetAddressMappingScheme( param->AddressMappingScheme);
	std::cout<<"address mapping scheme is :"<<param->AddressMappingScheme<<std::endl;
	translator->SetTranslationMethod(method);
	translator->SetDefaultField(CHANNEL_FIELD);
	//may be should not include param->RATE
	//word_size =	static_cast<uint64_t> (tburst * rate * buswidth/8);
	word_size =	static_cast<uint64_t> (tburst * buswidth/8);
	unsigned word_bit = NVM::mlog2(word_size);
	bit_width += word_bit;
	mem_size = static_cast<uint64_t>( word_size*cols*rows*banks*ranks*channels*subarrays );
}

void FineNVMain::SetMemoryControllers( MemoryController** &mem_ctl ,
									   Config** &ctl_conf , Config* conf ,
									   int num , std::string base_str ,
									   std::string memory_name )
{
	if( !num )
		return;
	std::stringstream confStr;
	std::string channelConfigFile;
	int i=0;
	while( i <  num)
	{
		ctl_conf[i] = new Config( *config );
		ctl_conf[i]->SetSimInterface(conf->GetSimInterface());
		confStr<< base_str << i ;
		if( conf->GetString(confStr.str()) !="" )
		{
			//relative path,get config file path
			if( channelConfigFile[0]!='/')
			{
				channelConfigFile = NVM::GetFilePath( conf->GetFileName() );
				channelConfigFile += conf->GetString( confStr.str()) ;
			}
			//read channel config 
			ctl_conf[i]->Read(channelConfigFile);
		}
		//create memory controller
		mem_ctl[ i ] = MemoryControllerFactory::CreateNewController( ctl_conf[i ]->GetString("MEM_CTL"));

		confStr.str("");
		confStr << memory_name << ".channel" <<i <<"."<<ctl_conf[i]->GetString("MEM_CTL");
		mem_ctl[i]->StatName(confStr.str());
		mem_ctl[i]->SetID(i);
		AddChild( mem_ctl[i]);
		mem_ctl[i]->SetParent(this);
		mem_ctl[i]->SetConfig( ctl_conf[i] , true);
		mem_ctl[i]->RegisterStats();
		i++;
		confStr.str("");
	}
}

bool FineNVMain::IsIssuable( NVMainRequest *request, FailReason *reason )
{
	uint64_t row, col , rank , bank , channel, subarray;
	uint64_t pa = request->address.GetPhysicalAddress();
	//main memory
	if( pa < mem_size)
	{
		GetDecoder()->Translate( pa , &row , &col , &rank , &bank, &channel , &subarray);
		return memoryControllers[channel]->IsIssuable(request , reason);
	}
	//dram buffer
	else
	{
		GetBufferDecoder()->Translate( pa , &row , &col , &rank , &bank , &channel , &subarray);
		return reservedControllers[channel]->IsIssuable(request,reason);
	}
}

bool FineNVMain::IssueCommand( NVMainRequest *request )
{
	futex_lock(&issue_lock);
	access_time++;
    ncounter_t channel, rank, bank, row, col, subarray;
	uint64_t pa = request->address.GetPhysicalAddress();
    bool mc_rv;
	bool access_cache = false;
	//std::cout<<"nvmain access"<<std::endl;
	if( !config )
    {
        std::cout << "NVMain: Received request before configuration!\n";
        return false;
    }
	if( request->isFetch == true )
	{
		fetch_time++;
	}
	else{
			if((request->type==READ) || (request->type==READ_PRECHARGE))
			{
				if( pa < mem_size)
					pcm_read++;
				else
					drc_read_hits++;
			}
			else if( (request->type==WRITE)||(request->type==WRITE_PRECHARGE))
			{
				if( pa< mem_size)
					pcm_write++;
				else
					drc_write_hits++;
			}
	}
    /* Translate the address, then copy to the address struct, and copy to request. */
	//access main memory 
	if( pa < mem_size )
	{
		GetDecoder()->Translate( pa, &row, &col, &bank, &rank, &channel, &subarray );
	}
	else
	{
		//translator should be modified 
		GetBufferDecoder()->Translate( request , &row , &col , &bank , &rank , &channel , &subarray);
		access_cache = true;
	}
		request->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray );
	    request->bulkCmd = CMD_NOP;

		/* Check for any successful prefetches. */
	    if( CheckPrefetch( request ) )
		{
			//insert event into event queue , type is EventResponse ,
			//when process evnet , call this->RequestComplete
	        GetEventQueue()->InsertEvent( EventResponse, this, request, 
                                      GetEventQueue()->GetCurrentCycle() + 1 );
			futex_unlock(&issue_lock);
		    return true;
	    }

		if( access_cache )
		{
			mc_rv = GetChild( channel + numChannels)->IssueCommand(request);
		}
		else
		{
			assert( GetChild( request )->GetTrampoline( ) == memoryControllers[channel] );
			mc_rv = GetChild( request )->IssueCommand( request );
		}
		if( mc_rv == true )
		{
			IssuePrefetch( request );
	        if( request->type == READ ) 
			    totalReadRequests++;
			else
				totalWriteRequests++;
		    PrintPreTrace( request );
		}
	futex_unlock(&issue_lock);
    return mc_rv;
}

bool FineNVMain::IssueAtomic( NVMainRequest *request )
{
    ncounter_t channel, rank, bank, row, col, subarray;
    bool mc_rv;
	bool access_cache = false;

    if( !config )
    {
        std::cout << "NVMain: Received request before configuration!\n";
        return false;
    }
	
	uint64_t pa = request->address.GetPhysicalAddress();
    /* Translate the address, then copy to the address struct, and copy to request. */
	if ( pa < mem_size )
	{
		GetDecoder( )->Translate( pa, 
                           &row, &col, &bank, &rank, &channel, &subarray );
	}
	else
	{
		//translator should be modified
		std::cout<<"access dram buffer memory"<<std::endl;
		GetBufferDecoder()->Translate( request , &row , &col , &bank , &rank , &channel , &subarray);
		access_cache = true;
	}
    request->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray );
    request->bulkCmd = CMD_NOP;
    /* Check for any successful prefetches. */
    if( CheckPrefetch( request ) )
    {
        return true;
    }
	
	if( access_cache )
		mc_rv = memoryControllers[reserved_channels+ numChannels ]->IssueAtomic(request);
	else
		mc_rv = memoryControllers[channel]->IssueAtomic( request );

    if( mc_rv == true )
    {
        IssuePrefetch( request );

        if( request->type == READ ) 
        {
            totalReadRequests++;
        }
        else
        {
            totalWriteRequests++;
        }

        PrintPreTrace( request );
    }
    
    return mc_rv;
}


void FineNVMain::RegisterStats( )
{
	NVMain::RegisterStats();
	AddStat( reserved_channels );
	AddStat( mem_size );
	AddStat( cache_size );
    AddStat(access_time);
	AddStat( delta_hit_t);
	AddStat( delta_clean_miss_t);
	AddStat( delta_dirty_miss_t);

	AddStat( rb_hit_time );
	AddStat( rb_dirty_miss_time);
	AddStat( rb_clean_miss_time);
	
	AddStat(drcReadRequests);
	AddStat(drcWriteRequests);	//include fetch write
	AddStat(nvmReadRequests);	//include fetch read
	AddStat(nvmWriteRequests);
	AddStat( dram_access_time);	//without fetch write
	AddStat( pcm_access_time);	//without fetch read
	AddStat( total_access_time);
	AddStat( fetch_time);
	AddStat(hit_rate);
	AddStat( drc_read_hit_rate);
	AddStat( drc_write_hit_rate );
	AddStat( clflush_wb_overhead );
}

void FineNVMain::CalculateStats( )
{
    for( unsigned int i = 0; i < numChannels; i++ )
        memoryControllers[i]->CalculateStats( );
	for( uint64_t i=0 ; i<reserved_channels; i++)
		reservedControllers[i]->CalculateStats();
	
    dram_access_time = drc_read_hits + drc_write_hits;
	pcm_access_time = pcm_read + pcm_write;
	total_access_time = dram_access_time + pcm_access_time;
	drcReadRequests = drc_read_hits + fetch_time/2;
	drcWriteRequests = drc_write_hits + fetch_time/2;
	nvmReadRequests = pcm_read + fetch_time/2;
	nvmWriteRequests = pcm_write;
	totalReadRequests = drcReadRequests + nvmReadRequests;
	totalWriteRequests = drcWriteRequests + nvmWriteRequests;
	drc_read_hit_rate = (double)drc_read_hits/(double)(dram_access_time + pcm_access_time);
	drc_write_hit_rate = (double)drc_write_hits/(double)(dram_access_time + pcm_access_time);
	hit_rate = drc_read_hit_rate + drc_write_hit_rate;
}

void FineNVMain::Cycle( ncycle_t steps )
{
	assert( !p->EventDriven );
	if( (!channelConfig || !memoryControllers)&&( !reservedConfig || !reservedControllers ) )
	{
		return;
	}
	double cpuFreq = static_cast<double>(p->CPUFreq);
	double busFreq = static_cast<double>(p->CLK);

	syncValue += static_cast<double>( busFreq / cpuFreq );
	if( syncValue >= 1.0f )
	{   
		syncValue -= 1.0f;
    }
	else 
		return;
	
     if(numChannels)
	    {   
			for( unsigned int i = 0; i < numChannels; i++ )
				{   
				
					memoryControllers[i]->Cycle( 1 );
				}   
		}
	 if(reserved_channels)
	 {
		 for( uint64_t i=0 ;i <reserved_channels ; i++)
		 {
			 reservedControllers[i]->Cycle(1);
		 }
	 }
	GetEventQueue()->Loop( steps );
	std::map<uint64_t , uint64_t>::iterator it;
	uint64_t current_cycle = GetEventQueue()->GetCurrentCycle();
	std::vector< std::pair<uint64_t , uint64_t> > vec;
	std::vector< std::pair<uint64_t , uint64_t> >::iterator vec_it;
	//output counter every 10000cycles
	if( !(current_cycle%100000))
	{
		std::cout<<"current cycle:"<<current_cycle<<std::endl;
		for( it=page_access_map_.begin();it!=page_access_map_.end();it++)
		{
			vec.push_back( std::make_pair( it->first , it->second) );
		}
		sort( vec.begin() , vec.end() ,cmp);
		for( vec_it = vec.begin() ; vec_it!=vec.end();vec_it++)
		{
			std::cout<<vec_it->first<<" "<<vec_it->second<<std::endl;
		}
		std::cout<<std::endl<<std::endl;
	}
}

bool FineNVMain::RequestComplete( NVMainRequest *request )
{
	bool rv = false;
	if(request->isFetch)
	{
	  total_caching_t += GetEventQueue()->GetCurrentCycle() 
						 - request->arrivalCycle;	
	}
	
	if( request->owner == this )
    {
        if( request->isPrefetch )
        {
            if( prefetchBuffer.size() >= p->PrefetchBufferSize )
            {
                unsuccessfulPrefetches++;
                delete prefetchBuffer.front();
                prefetchBuffer.pop_front();
            }
            prefetchBuffer.push_back( request );
            rv = true;
        }
        else if( request->isFetch )
        {
			assert( block_fetcher);
			block_fetcher->RequestComplete(request);
            rv = true;
        }
		else if( request->isClflush)
		{
			//std::cout<<"curCycle:"<<GetEventQueue()->GetCurrentCycle()<<" arrival cycle:"<<request->arrivalCycle<<std::endl;
			clflush_wb_overhead += GetEventQueue()->GetCurrentCycle()-(request->arrivalCycle);
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
		/***update statisticals for threshold adjustment alogrithm***/ 
		if( request->address.GetPhysicalAddress() > mem_size)
		{
			if( request->rbHit)
				rb_hit_time++;
			else
			{	
				if( request->dirty_miss )
					rb_dirty_miss_time++;	
				else
					rb_clean_miss_time++;
			}
		}
	   /*****************************************************/
        rv = GetParent()->RequestComplete( request );
    }
    return rv;
}	

