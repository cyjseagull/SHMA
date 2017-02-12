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

#include "NVM/Multi_Queue_NVMain/MultiQueueNVMain.h"
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

using namespace NVM;


uint64_t MultiQueueNVMain::migra_thres = 0;
Action MultiQueueNVMain::pre_action = INIT;
uint64_t MultiQueueNVMain::pre_net_benefit = 0;
MultiQueueNVMain::MultiQueueNVMain( )
{
	ranking_queue.resize(QUEUE_SIZE);
}

void MultiQueueNVMain::SetConfig( Config *conf, std::string memoryName, bool createChildren )
{
	flat_memory = new FlatNVMain();
	EventQueue* queue=new EventQueue();
	flat_memory->SetParent(this);
	flat_memory->SetEventQueue(queue);
	GetGlobalEventQueue()->AddSystem( flat_memory,conf );
	flat_memory->SetConfig( conf, memoryName, createChildren);
	flat_memory->RegisterStats();
	AddChild(flat_memory);

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
			SetDecoder(migrator);
		}
	}
	dram_page_num = (flat_memory->GetFastMemory()->GetMemorySize())>>PAGE_SHIFT;
}

bool MultiQueueNVMain::IssueCommand( NVMainRequest *request)
{
	std::cout<<"Issue command through MultiQueueNVMain:"<<request->address.GetPhysicalAddress()<<", is migration:"<<request->isMigration<<std::endl;
	
	FlatDecoder* migrator_trans = dynamic_cast< FlatDecoder*>(this->GetDecoder());
	//migrator decoder must be Migrator object
	assert( migrator_trans);
	if ( migrator_trans->IsMigrated( request->address) )
	{
		std::cout<<"is migrated"<<std::endl;
		request->is_migrated = true;
		uint64_t row, col, bank,rank,channel,subarray;
		uint64_t paddr=request->address.GetPhysicalAddress();
		migrator_trans->Translate2(paddr,
				&row,&col,&bank,&rank,&channel,&subarray);

		std::cout<<"translate result,paddr:"<<paddr<<" "<<row<<" "<<bank<<" "<<rank<<" "<<channel<<" "<<subarray<<std::endl;
		request->address.SetTranslatedAddress(row,col,bank,rank,channel,subarray);
		request->address.SetPhysicalAddress( paddr);
	}
   return flat_memory->IssueCommand(request); 
}


bool MultiQueueNVMain::RequestComplete( NVMainRequest *request )
{
	std::cout<<"request complete"<<request->address.GetPhysicalAddress()<<std::endl;
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
			//std::cout<<"migration ended"<<std::endl;
			migrator->RequestComplete(request);
            delete request;
			migrated_pages_++;
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
		rv = GetParent( )->RequestComplete( request );
	}
    return rv;
}

void MultiQueueNVMain::Cycle( ncycle_t steps )
{
	  flat_memory->Cycle(steps);
}

void MultiQueueNVMain::RegisterStats( )
{
}
