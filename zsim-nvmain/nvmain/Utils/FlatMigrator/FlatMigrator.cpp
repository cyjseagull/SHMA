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
*   Yujie Chen (YuJieChen_hust@163.com)
*******************************************************************************/

#include "Utils/FlatMigrator/FlatMigrator.h"
#include "src/SubArray.h"
#include "src/EventQueue.h"
#include "include/NVMHelpers.h"

using namespace NVM;

uint64_t FlatMigrator::migration_cycles;

FlatMigrator::FlatMigrator( ): tbuffer_read(2), total_pages(0),
	current_page(0), channels(0), ranks(0), banks(0), rows(0),
	fast_memory(NULL), slow_memory(NULL), local_buffered(false),
	remote_buffered(false), migration_count(0), queue_waits(0), buffered_reads(0), local_req(NULL), remote_req(NULL)
{
}


FlatMigrator::~FlatMigrator( )
{
}

void FlatMigrator::Init( Config *config )
{
    /* If we want to simulate additional latency serving buffered requests. */
	std::cout<<"init migrator"<<std::endl;
	if( config->KeyExists("MigrationReadLatency"))
		tbuffer_read = config->GetValue("MigrationReadLatency");
	//std::cout<<"fast_memory of hook:"<<fast_memory<<std::endl;
	Params* p = fast_memory->GetParams();
	ranks = p->RANKS;
	banks = p->BANKS;
	channels = p->CHANNELS;
	rows = p->ROWS;
	cols = p->COLS;
	total_pages = p->RANKS * p->BANKS* p->ROWS * p-> CHANNELS;
	std::cout<<"total pages is:"<<total_pages<<std::endl;
	migration_cycles = 0;
	
	AddStat( migration_count);
	AddStat( migration_cycles);
	AddStat(queue_waits);
}


bool FlatMigrator::IssueAtomic( NVMainRequest *request )
{
	return true;
}


bool FlatMigrator::IssueCommand( NVMainRequest *request )
{
	if (request->type == MIGRATION)
	{
		if( !request->tag == RETRY_MIGRATION_TAG)
			request->arrivalCycle = GetEventQueue()->GetCurrentCycle();
		if( !migratorTranslator->Migrating())
			return TryMigration( request, false );
		else
		{
			request->tag = DELAYED_MIGRATION_TAG;
			GetEventQueue()->InsertEvent(EventResponse, this, 
					request, GetEventQueue()->GetCurrentCycle()+500);
		}
	}
	return true;
}


bool FlatMigrator::RequestComplete( NVMainRequest *request )
{
	if( request->tag == DELAYED_MIGRATION_TAG)
	{
		request->tag = RETRY_MIGRATION_TAG;
		this->IssueCommand(request);	
	}
	else
	{
    /* Ensure the Migrator translator is used. */
    assert( migratorTranslator != NULL );
    if( request->owner == memory && 
		request->tag == MIG_READ_TAG )
    {
        /* A migration read completed, update state. */
		//std::cout<<"Migration Buffered"<<std::endl;
        
		migratorTranslator->SetMigrationState( request->address,FLATMIGRATION_BUFFERED ); 
        /* If both requests are buffered, we can attempt to write. */
        bool bufferComplete = false;

        if( (request == local_req 
        && migratorTranslator->IsBuffered( remote_addr ))
         || (request == remote_req
        && migratorTranslator->IsBuffered( local_addr )) )
        {
                bufferComplete = true;
        }
     /* Make a new request to issue for write. Parent will delete current pointer. */
        if( request == local_req )
         {
            local_req = new NVMainRequest( );
			*local_req = *request;
		 }
         else if( request == remote_req )
        {
          remote_req = new NVMainRequest( );
          *remote_req = *request;
        }

      /* Swap the address and set type to write. */
       if( bufferComplete )
	   {
			//std::cout<<"buffer complete"<<std::endl;
			//std::cout<<"local address:"<<local_req->address.GetPhysicalAddress()<<std::endl;
			//std::cout<<"remote address:"<<remote_req->address.GetPhysicalAddress()<<std::endl;
            NVMAddress tmp = local_req->address;
			local_req->address = remote_req->address;
			remote_req->address = tmp;
			remote_req->type = WRITE;
			local_req->type = WRITE;
			remote_req->tag = MIG_WRITE_TAG;
			local_req->tag = MIG_WRITE_TAG;
			local_req->owner = memory;
			remote_req->owner = memory;

            /* Try to issue these now, otherwise we can try later. */
            bool local_issued, remote_issued;
				
			//write fast memory
			local_issued = fast_memory->IssueCommand( local_req );
			remote_issued = slow_memory->IssueCommand( remote_req );
			if( local_issued )
			{
				//std::cout<<"Migration writing local req:"<<local_req->address.GetPhysicalAddress()<<std::endl;
				migratorTranslator->SetMigrationState( local_req->address, FLATMIGRATION_WRITING);
			}
			if( remote_issued )
			{
				//std::cout<<"Migration writing remote req:"<<remote_req->address.GetPhysicalAddress()<<std::endl;
				migratorTranslator->SetMigrationState( remote_req->address,FLATMIGRATION_WRITING);
			}
			
			local_buffered = !local_issued;
			remote_buffered = !remote_issued;
          }
        }
        /* A write completed. */
        else if( request->owner == memory && request->tag == MIG_WRITE_TAG )
        {
			//std::cout<<"migration done"<<std::endl;
			// Note: request should be deleted by parent
            migratorTranslator->SetMigrationState( request->address, FLATMIGRATION_DONE );
            migration_count++;
			if( migration_count%1000==0)
				std::cout<<"migrate time:"<<migration_count<<" migration cycle:"<<migration_cycles<<std::endl;
			migration_cycles += GetEventQueue()->GetCurrentCycle() - request->arrivalCycle;
			if( migration_count%1000 == 0)
				std::cout<<"migration count:"<<migration_count<<", cycle:"<<migration_cycles<<std::endl;
        }
        /* Some other request completed, see if we can ninja issue some migration writes that did not queue. */
        else if( local_buffered || remote_buffered )
        {
			//std::cout<<"local buffered or remote buffered"<<std::endl;
            if( local_buffered )
            {
				bool local_issued = fast_memory->IssueCommand( local_req); 
                local_buffered = !local_issued;
            }
            if( remote_buffered )
            {
				bool remote_issued = slow_memory->IssueCommand(remote_req);
                remote_buffered = !remote_issued;
            }
        }
	}
    return true;
}

bool FlatMigrator::TryMigration( NVMainRequest *request, bool atomic )
{
	//std::cout<<"try migration:"<<request->address.GetPhysicalAddress()<<std::endl;
	//begin_cycle = GetEventQueue()->GetCurrentCycle();
    bool rv = true;
    /* Ensure the Migrator translator is used. */
	//parent module is who issue migration??
    /* Migrations in progress must be served from the buffers during migration. */
	if( migratorTranslator->IsBuffered( request->address ) )
    {
            /* Short circuit this request so it is not queued. */
            rv = false;
            /* Complete the request, adding some buffer read latency. */
            GetEventQueue( )->InsertEvent( EventResponse, 
					memory, request,
					GetEventQueue()->GetCurrentCycle()+ tbuffer_read );
            buffered_reads++;
			//std::cout<<"read to buffer"<<std::endl;
            return rv;
    }
    /* See if any migration is possible (i.e., no migration is in progress) */
     bool migrationPossible = false;
	//std::cout<<"migrating:"<<migratorTranslator->Migrating()<<std::endl;
     if( !migratorTranslator->Migrating( ) 
            /*&& !migratorTranslator->IsMigrated( request->address )*/
            && request->media == SLOW_MEM )
    {
            migrationPossible = true;
    }
    if( migrationPossible )
     {
            assert( !local_buffered && !remote_buffered);
            /* Discard the unused column address. */
            uint64_t row, bank, rank, channel, subarray;
            request->address.GetTranslatedAddress( &row, NULL, &bank, &rank, &channel, &subarray );
            uint64_t promoteeAddress = migratorTranslator->ReverseTranslate2( row, 0, bank, rank, channel, subarray, false );
			
			//std::cout<<"set local addr from:"<<(request->address.GetPhysicalAddress()>>6)<<" to:"<<(promoteeAddress>>6)<<std::endl;
			local_addr.SetPhysicalAddress( promoteeAddress );
            local_addr.SetTranslatedAddress( row, 0, bank, rank, channel, subarray );
            /* Pick a victim to replace. */
            ChooseVictim( migratorTranslator, local_addr, remote_addr );
             /* Lastly, make sure we can queue the migration requests. */
             if( CheckIssuable( local_addr, READ ) &&
                         CheckIssuable( remote_addr, READ ) )
             {
				    //clflush first
					uint64_t start_cycle = GetEventQueue()->GetCurrentCycle();
					uint64_t cycle = migratorTranslator->StartMigration( local_addr, 
															remote_addr, start_cycle);
					//std::cout<<"clflush cycle:"<<cycle-start_cycle<<std::endl;
					
                    local_req = new NVMainRequest( ); 
                    remote_req = new NVMainRequest( );
				
					local_req->address = local_addr;
					local_req->type = READ;
					local_req->tag = MIG_READ_TAG;
					local_req->burstCount = cols;
					local_req->owner = memory;
					local_req->isMigration = true;
					
					remote_req->address = remote_addr;
					remote_req->type = READ;
					remote_req->tag = MIG_READ_TAG;
					remote_req->burstCount = cols;
					remote_req->owner = memory;
					remote_req->isMigration = true;
					//std::cout<<"local addr:"<<local_addr.GetPhysicalAddress()<<std::endl;
					//std::cout<<"remote addr:"<<remote_addr.GetPhysicalAddress()<<std::endl;
                    memory->IssueCommand( remote_req);
					memory->IssueCommand( local_req);
             }
             else
            {
                queue_waits++;
            }
        }
    return rv;
}


bool FlatMigrator::CheckIssuable( NVMAddress address, OpType type, bool is_fast )
{
    NVMainRequest request;
    request.address = address;
    request.type = type;
	if( is_fast )
		return fast_memory->IsIssuable(&request);
	return slow_memory->IsIssuable(&request);
}

//victim address in the fast memory
void FlatMigrator::ChooseVictim( FlatDecoder *at, NVMAddress& promotee, NVMAddress& victim )
{
	uint64_t migra_page = current_page;
	int victim_channel = migra_page % channels;
	migra_page >>= NVM::mlog2(channels);

	int victim_rank = migra_page % ranks;
	migra_page >>= NVM::mlog2(ranks);

	int victim_bank = migra_page % banks;
	migra_page >>= NVM::mlog2(banks);

	int victim_row = migra_page%rows;

	victim.SetTranslatedAddress( victim_row, 0,victim_bank, 
			victim_rank, victim_channel, 0);

	uint64_t addr = at->ReverseTranslate2(victim_row, 0, 
			victim_bank, victim_rank, victim_channel, 0,true);

	victim.SetPhysicalAddress( addr );
	if( current_page <= total_pages )
		current_page = (current_page+1)%total_pages;
	else
	{
		current_page = (current_page -total_pages)%total_pages;
		std::cout<<"current page is:"<<current_page<<std::endl;
	}
}

