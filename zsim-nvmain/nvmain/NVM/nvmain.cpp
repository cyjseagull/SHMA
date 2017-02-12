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
* Author list: 
*   Matt Poremba    ( Email: mrp5060 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/

#include "nvmain.h"
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
#include "Decoders/Migrator/Migrator.h"
#include <sstream>
#include <cassert>

using namespace NVM;
//uint64_t NVMain::memory_size = 0;
NVMain::NVMain( )
{
    config = NULL;
    translator = NULL;
    memoryControllers = NULL;
    channelConfig = NULL;
    syncValue = 0.0f;
    preTracer = NULL;

    totalReadRequests = 0;
    totalWriteRequests = 0;

    prefetcher = NULL;
    successfulPrefetches = 0;
    unsuccessfulPrefetches = 0;
	mem_width = 0;
	memory_size = 0;
	std::cout<<"************create nvmain---"<<std::endl;
}

NVMain::~NVMain( )
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

    if( translator )
        delete translator;

    if( channelConfig )
    {
        for( unsigned int i = 0; i < numChannels; i++ )
        {
            delete channelConfig[i];
        }

        delete [] channelConfig;
    }
}

Config *NVMain::GetConfig( )
{
    return config;
}

void NVMain::SetConfig( Config *conf, std::string memoryName, bool createChildren )
{
    TranslationMethod *method;
    int channels, ranks, banks, rows, cols, subarrays;

    Params *params = new Params( );
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
        if( conf->KeyExists( "MATHeight" ) )
        {
            rows = static_cast<int>(p->MATHeight);
            subarrays = static_cast<int>( p->ROWS / p->MATHeight );
        }
        else
        {
            rows = static_cast<int>(p->ROWS);
            subarrays = 1;
        }
        if( config->KeyExists( "Decoder" ) )
			translator = DecoderFactory::CreateNewDecoder( config->GetString( "Decoder" ) );
        else
            translator = new AddressTranslator( );

        cols = static_cast<int>(p->COLS);
        banks = static_cast<int>(p->BANKS);
        ranks = static_cast<int>(p->RANKS);
        channels = static_cast<int>(p->CHANNELS);
		std::cout<<"cols is"<<cols<<std::endl;
		std::cout<<"rows is"<<rows<<std::endl;
		std::cout<<"ranks is"<<ranks<<std::endl;
		std::cout<<"banks is"<<banks<<std::endl;
		std::cout<<"ranks is"<<ranks<<std::endl;
		std::cout<<"channels is"<<channels<<std::endl;
		std::cout<<"subarrays is"<<subarrays<<std::endl;

        method = new TranslationMethod( );

        method->SetBitWidths( NVM::mlog2( rows ), 
                    NVM::mlog2( cols ), 
                    NVM::mlog2( banks ), 
                    NVM::mlog2( ranks ), 
                    NVM::mlog2( channels ), 
                    NVM::mlog2( subarrays )
                    );
        method->SetCount( rows, cols, banks, ranks, channels, subarrays );
        method->SetAddressMappingScheme( p->AddressMappingScheme );
        translator->SetConfig( config, createChildren );
        translator->SetTranslationMethod( method );
        translator->SetDefaultField( CHANNEL_FIELD );

        SetDecoder( translator );
        memoryControllers = new MemoryController* [channels];
        channelConfig = new Config* [channels];
        for( int i = 0; i < channels; i++ )
        {
            std::stringstream confString;
            std::string channelConfigFile;

            channelConfig[i] = new Config();

            channelConfig[i]->SetSimInterface( config->GetSimInterface( ) );

            confString << "CONFIG_CHANNEL" << i;

            if( config->GetString( confString.str( ) ) != "" )
            {
                channelConfigFile  = config->GetString( confString.str( ) );

                if( channelConfigFile[0] != '/' )
                {
                    channelConfigFile  = NVM::GetFilePath( config->GetFileName( ) );
                    channelConfigFile += config->GetString( confString.str( ) );
                }
                
                channelConfig[i]->Read( channelConfigFile );
            }
			else
				channelConfig[i] = config;
            /* Initialize memory controller */
            memoryControllers[i] = 
                MemoryControllerFactory::CreateNewController( channelConfig[i]->GetString( "MEM_CTL" ) );

            /* When selecting a MC child, use no field from the decoder (only-child). */

            confString.str( "" );
			//memoryControllers[i]'s statName is memoryName.channeli.FCFS(etc)
            confString << memoryName << ".channel" << i << "." 
                << channelConfig[i]->GetString( "MEM_CTL" );
            memoryControllers[i]->StatName( confString.str( ) );
            memoryControllers[i]->SetID( i );

            AddChild( memoryControllers[i] );
            memoryControllers[i]->SetParent( this );
            /* Set Config recursively. */
            memoryControllers[i]->SetConfig( channelConfig[i], createChildren );
            /* Register statistics. */
            memoryControllers[i]->RegisterStats( );
        }
    }
	Params *channel_p = new Params( );
	channel_p->SetParams( channelConfig[0] );
	word_size = static_cast<uint64_t>( channel_p->tBURST * channel_p->BusWidth/8);

	std::cout<<"word size is "<<word_size<<std::endl;
	if( !config->KeyExists("DRCVariant") )
	{
		memory_size = ((uint64_t)word_size *(uint64_t)cols *(uint64_t)rows *(uint64_t)banks * (uint64_t)ranks * (uint64_t)channels *(uint64_t)subarrays);
	}

    if( p->MemoryPrefetcher != "none" )
    {
        prefetcher = PrefetcherFactory::CreateNewPrefetcher( p->MemoryPrefetcher );
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


        if( config->GetString( "PreTraceWriter" ) == "" )
            preTracer = TraceWriterFactory::CreateNewTraceWriter( "NVMainTrace" );
        else
            preTracer = TraceWriterFactory::CreateNewTraceWriter( config->GetString( "PreTraceWriter" ) );

        if( p->PrintPreTrace )
            preTracer->SetTraceFile( pretraceFile );
        if( p->EchoPreTrace )
            preTracer->SetEcho( true );
    }
    RegisterStats( );
}


bool NVMain::IsIssuable( NVMainRequest *request, FailReason *reason )
{
    uint64_t channel, rank, bank, row, col, subarray;
    bool rv;
    assert( request != NULL );

    GetDecoder( )->Translate( request->address.GetPhysicalAddress( ), 
                           &row, &col, &rank, &bank, &channel, &subarray );
	
    rv = memoryControllers[channel]->IsIssuable( request, reason );
    return rv;
}

void NVMain::GeneratePrefetches( NVMainRequest *request, std::vector<NVMAddress>& prefetchList )
{
    std::vector<NVMAddress>::iterator iter;
    ncounter_t channel, rank, bank, row, col, subarray;

    for( iter = prefetchList.begin(); iter != prefetchList.end(); iter++ )
    {
        /* Make a request from the prefetch address. */
        NVMainRequest *pfRequest = new NVMainRequest( );
        *pfRequest = *request;
        pfRequest->address = (*iter);
        pfRequest->isPrefetch = true;
        pfRequest->owner = this;
        
        /* Translate the address, then copy to the address struct, and copy to request. */
        GetDecoder( )->Translate( request->address.GetPhysicalAddress( ), 
                               &row, &col, &bank, &rank, &channel, &subarray );
        request->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray );
        request->bulkCmd = CMD_NOP;

        /* Just type to issue; If the queue is full it simply won't be enqueued. */
        GetChild( pfRequest )->IssueCommand( pfRequest );
    }
}

void NVMain::IssuePrefetch( NVMainRequest *request )
{
    /* 
     *  Generate prefetches here. It makes the most sense to prefetch in this class
     *  since we may prefetch across channels. However, this may be applicable to any
     *  class in the hierarchy as long as we filter out the prefetch addresses that
     *  do not belong to a child of the current module.
     */
    /* TODO: We are assuming this is the master root! */
    std::vector<NVMAddress> prefetchList;
    if( prefetcher && request->type == READ && request->isPrefetch == false 
        && prefetcher->DoPrefetch(request, prefetchList) )
    {
        GeneratePrefetches( request, prefetchList );
    }
}

bool NVMain::CheckPrefetch( NVMainRequest *request )
{
    bool rv = false;
    NVMainRequest *pfRequest = NULL;
    std::list<NVMainRequest *>::const_iterator iter;
    std::vector<NVMAddress> prefetchList;

    for( iter = prefetchBuffer.begin(); iter!= prefetchBuffer.end(); iter++ )
    {
        if( (*iter)->address.GetPhysicalAddress() == request->address.GetPhysicalAddress() )
        {
            if( prefetcher->NotifyAccess(request, prefetchList) )
            {
                GeneratePrefetches( request, prefetchList );
            }

            successfulPrefetches++;
            rv = true;
            pfRequest = (*iter);
            delete pfRequest;
            break;
        }
    }

    if( pfRequest != NULL )
        prefetchBuffer.remove( pfRequest );

    return rv;
}

void NVMain::PrintPreTrace( NVMainRequest *request )
{
    /*
     *  Here we can generate a data trace to use with trace-based testing later.
     */
    if( p->PrintPreTrace || p->EchoPreTrace )
    {
        TraceLine tl;

        tl.SetLine( request->address,
                    request->type,
                    GetEventQueue( )->GetCurrentCycle( ),
                    request->data,
                    request->threadId 
                  );

        preTracer->SetNextAccess( &tl );
    }
}

bool NVMain::IssueCommand( NVMainRequest *request )
{
	//common memory access
    ncounter_t channel, rank, bank, row, col, subarray;
    bool mc_rv;
    if( !config )
        return false;
    
	GetDecoder()->Translate( request->address.GetPhysicalAddress( ), 
                           &row, &col, &bank, &rank, &channel, &subarray );
	//std::cout<<"translate by nvmain, row:"<<row<<",col:"<<col<<", bank"<<bank<<" , rank:"<<rank<<" , channel:"<<channel<<" , subarray:"<<subarray<<std::endl;	
    request->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray );
    request->bulkCmd = CMD_NOP;
    /* Check for any successful prefetches. */
    if( CheckPrefetch( request ) )
    {
        GetEventQueue()->InsertEvent( EventResponse, this, request, 
                                      GetEventQueue()->GetCurrentCycle() + 1 );

        return true;
    }
    assert( GetChild( request )->GetTrampoline( ) == memoryControllers[channel] );
    mc_rv = GetChild( request )->IssueCommand( request );
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

bool NVMain::IssueAtomic( NVMainRequest *request )
{
    ncounter_t channel, rank, bank, row, col, subarray;
    bool mc_rv;

    if( !config )
    {
        std::cout << "NVMain: Received request before configuration!\n";
        return false;
    }

    /* Translate the address, then copy to the address struct, and copy to request. */
    GetDecoder( )->Translate( request->address.GetPhysicalAddress( ), 
                           &row, &col, &bank, &rank, &channel, &subarray );
    request->address.SetTranslatedAddress( row, col, bank, rank, channel, subarray );
    request->bulkCmd = CMD_NOP;

    /* Check for any successful prefetches. */
    if( CheckPrefetch( request ) )
    {
        return true;
    }

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

bool NVMain::RequestComplete( NVMainRequest *request )
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
        else
        {
            delete request;
            rv = true;
        }
    }
    else
    {
        rv = GetParent()->RequestComplete( request );
    }
    return rv;
}

void NVMain::Cycle( ncycle_t steps )
{
    assert( !p->EventDriven );
    
	/*
     *  Previous errors can prevent config from being set. Likewise, if the first memoryController is
     *  NULL, so are all the others, so return here instead of seg faulting.
     */
    if( !config || !memoryControllers )
      return;

	if( GetSyncValue()==-1)
			return;

	CycleMemCtl( memoryControllers ,numChannels );
    GetEventQueue()->Loop( steps );
}

double NVMain::GetSyncValue( )
{

    /* Sync the memory clock with the cpu clock. */
    double cpuFreq = static_cast<double>(p->CPUFreq);
    double busFreq = static_cast<double>(p->CLK);

    syncValue += static_cast<double>( busFreq / cpuFreq );
    if( syncValue >= 1.0f )
    {
        syncValue -= 1.0f;
		return syncValue;
    }
    else
    {
        return -1;
    }
}


void NVMain::CycleMemCtl( MemoryController** &mem_ctl , uint64_t num_channels)
{
	if(num_channels)
	{
		for( unsigned int i = 0; i < numChannels; i++ )
		{
			mem_ctl[i]->Cycle( 1 );
		}
	}	
}

void NVMain::RegisterStats( )
{
    AddStat(totalReadRequests);
    AddStat(totalWriteRequests);
    AddStat(successfulPrefetches);
    AddStat(unsuccessfulPrefetches);
}

void NVMain::CalculateStats( )
{
    for( unsigned int i = 0; i < numChannels; i++ )
	{
		memoryControllers[i]->CalculateStats( );
	}
}

