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

#include "Utils/SelfDefinedMigrator/StaticMigrator.h"
#include "NVM/nvmain.h"
#include "src/SubArray.h"
#include "src/EventQueue.h"
#include "include/NVMHelpers.h"

using namespace NVM;

StaticMigrator::StaticMigrator( )
{
    /*
     *  We will eventually be injecting requests to perform migration, so we
     *  would like IssueCommand to be called on the original request first so
     *  that we do not unintentially fill up the transaction queue causing 
     *  the original request triggering migration to fail.
     */
    SetHookType( NVMHOOK_BOTHISSUE ); //call hook before and after
	
    promoRequest = NULL;
    demoRequest = NULL;
    promoBuffered = false;
    demoBuffered = false;

    migrationCount = 0;
    queueWaits = 0;
    bufferedReads = 0;

    queriedMemory = false;
    promotionChannelParams = NULL;
    currentPromotionPage = 0;
	//	
	migraAddress = static_cast<uint64_t>(-1);
	CopyChars(signal_file ,"signal");
	CopyChars(migration_done , "migration done");
	migration_done_len = strlen(migration_done);
	cmd = new command;
	can_migrate = false;
	start_addr = 0;
	channels = 0;
	nvm_channel.clear();

	nvm_num = 0;
	dram_num = 0;
	cur_dram = 0;
	cur_nvm = 0;
}


StaticMigrator::~StaticMigrator( )
{
	if(signal_file)
	{
		delete signal_file;
		signal_file = NULL;
	}
	if(cmd)
	{
		delete cmd;
		cmd = NULL;
	}
	if(migration_done)
	{
		delete migration_done;
		migration_done = NULL;
	}
	std::cout<<"delete static migrator"<<std::endl;
}


bool StaticMigrator::CopyChars(char* &dest_chars , const char* source_chars)
{
	bool ret = false;
	int len = strlen(source_chars);
	if(!dest_chars)
	{
		dest_chars = new char[len+1];
		//allocate space failed
		if( !dest_chars )
		   return ret;
	 }
	else
	{
		int dest_len = strlen(dest_chars);
		if(dest_len < len )
		{
			delete dest_chars;
			dest_chars = new char[len];
		}
	}

		if(dest_chars)
		{
			memcpy( dest_chars , source_chars , len);
			dest_chars[len] = '\0';
			ret = true;
		}

	 return true;
}



void StaticMigrator::Init( Config *config )
{
    /* Specifies with channel is the "fast" memory. */
   promotionChannel = 0;
   //config->GetValueUL( "CoinMigratorPromotionChannel", promotionChannel );

    /* If we want to simulate additional latency serving buffered requests. */
    bufferReadLatency = 4;
    config->GetValueUL( "MigrationBufferReadLatency", bufferReadLatency );
    /* 
     *  We migrate entire rows between banks, so the column count needs to
     *  match across all channels for valid results.
     */
    numCols = config->GetValue( "COLS" );
	
	//get channels
	if(config->KeyExists("CHANNELS"))
	{
		channels = static_cast<uint64_t>(config->GetValue("CHANNELS"));
	    std::stringstream confString;
	    std::string channelConfigFile;	
		std::string tmp_str;
		std::cout<<"channels are "<<channels<<std::endl;
		for( uint64_t i=0 ; i<channels;i++)
		{
			confString<< "CONFIG_CHANNEL" << i;
			if( config->GetString( confString.str( ) ) != "" )
			{
				channelConfigFile  = config->GetString( confString.str( ) );
				//transform to capital
				//tmp_str.resize(channelConfigFile.size());
				transform( channelConfigFile.begin(),channelConfigFile.end(),channelConfigFile.begin() , ::toupper);
				std::cout<<"channelConfigFile str is"<<channelConfigFile<<std::endl;
				if( channelConfigFile.find( "DRAM" )!= std::string::npos )
				{

					dram_channel.push_back(i);
					std::cout<<"add dram channel "<<dram_channel[dram_num]<<std::endl;
					dram_num++;
				}
				else if( channelConfigFile.find("NVM") != std::string::npos)
				{
					nvm_channel.push_back(i);
					std::cout<<"add nvm channel "<<nvm_channel[nvm_num]<<std::endl;
					nvm_num++;
				}

			}
		  confString.str("");
		}
	}
    AddStat(migrationCount);
    AddStat(queueWaits);
    AddStat(bufferedReads);
}


bool StaticMigrator::IssueAtomic( NVMainRequest *request )
{
	bool ret = true;
    /* For atomic mode, we just swap the pages instantly. */
	if(ParseRequest(request))
			ret =  TryMigration( request, true );
	return ret;
}


bool StaticMigrator::IssueCommand( NVMainRequest *request )
{
    /* 
     *  In cycle-accurate mode, we must read each page, buffer it, enqueue a
     *  write request, and wait for write completion.
     */
	bool ret = true;
    if( ParseRequest(request) )
	{
		ret = TryMigration( request, false );
		WriteDoneFlag();
		//can_migrate = false;
	}
    return ret;
}


bool StaticMigrator::RequestComplete( NVMainRequest *request )
{
    if( NVMTypeMatches(NVMain) && GetCurrentHookType( ) == NVMHOOK_PREISSUE )
    {
        /* Ensure the Migrator translator is used. */
        Migrator *migratorTranslator = dynamic_cast<Migrator *>(parent->GetTrampoline( )->GetDecoder( ));
        assert( migratorTranslator != NULL );

        if( request->owner == parent->GetTrampoline( ) && request->tag == MIG_READ_TAG )
        {
            /* A migration read completed, update state. */
            migratorTranslator->SetMigrationState( request->address, MIGRATION_BUFFERED ); 

            /* If both requests are buffered, we can attempt to write. */
            bool bufferComplete = false;

            if( (request == promoRequest 
                 && migratorTranslator->IsBuffered( demotee ))
                || (request == demoRequest
                 && migratorTranslator->IsBuffered( promotee )) )
            {
                bufferComplete = true;
            }

            /* Make a new request to issue for write. Parent will delete current pointer. */
            if( request == promoRequest )
            {
                promoRequest = new NVMainRequest( );
                *promoRequest = *request;
            }
            else if( request == demoRequest )
            {
                demoRequest = new NVMainRequest( );
                *demoRequest = *request;
            }
            else
            {
                assert( false );
            }

            /* Swap the address and set type to write. */
            if( bufferComplete )
            {
                /* 
                 *  Note: once IssueCommand is called, this hook may receive
                 *  a different parent, but fail the NVMTypeMatch check. As a
                 *  result we need to save a pointer to the NVMain class we
                 *  are issuing requests to.
                 */
				
				//std::cout<<"buffer complete"<<std::endl;
                NVMObject *savedParent = parent->GetTrampoline( );

                NVMAddress tempAddress = promoRequest->address;
                promoRequest->address = demoRequest->address;
                demoRequest->address = tempAddress;

                demoRequest->type = WRITE;
                promoRequest->type = WRITE;

                demoRequest->tag = MIG_WRITE_TAG;
                promoRequest->tag = MIG_WRITE_TAG;

                /* Try to issue these now, otherwise we can try later. */
                bool demoIssued, promoIssued;

                demoIssued = savedParent->GetChild( demoRequest )->IssueCommand( demoRequest );
                promoIssued = savedParent->GetChild( promoRequest )->IssueCommand( promoRequest );

                if( demoIssued )
                {
                    migratorTranslator->SetMigrationState( demoRequest->address, MIGRATION_WRITING );
                }
                if( promoIssued )
                {
                    migratorTranslator->SetMigrationState( promoRequest->address, MIGRATION_WRITING );
                }

                promoBuffered = !promoIssued;
                demoBuffered = !demoIssued;
				
            }
        }
        /* A write completed. */
        else if( request->owner == parent->GetTrampoline( ) && request->tag == MIG_WRITE_TAG )
        {
            // Note: request should be deleted by parent
            migratorTranslator->SetMigrationState( request->address, MIGRATION_DONE );
			std::cout<<"migrating pa : "<<request->address.GetPhysicalAddress()<<" complete"<<std::endl;
            migrationCount++;
        }
        /* Some other request completed, see if we can ninja issue some migration writes that did not queue. */
        else if( promoBuffered || demoBuffered )
        {
            bool demoIssued, promoIssued;

            if( promoBuffered )
            {
                promoIssued = parent->GetTrampoline( )->GetChild( promoRequest )->IssueCommand( promoRequest );
                promoBuffered = !promoIssued;
            }

            if( demoBuffered )
            {
                demoIssued = parent->GetTrampoline( )->GetChild( demoRequest )->IssueCommand( demoRequest );
                demoBuffered = !demoIssued;
            }
        }
    }
	
    return true;
}


bool StaticMigrator::CheckIssuable( NVMAddress address, OpType type )
{
    NVMainRequest request;

    request.address = address;
    request.type = type;

    return parent->GetTrampoline( )->GetChild( &request )->IsIssuable( &request );
}


bool StaticMigrator::TryMigration( NVMainRequest *request, bool atomic )
{
    bool rv = true;
	//std::cout<<"try migration"<<std::endl;
	//translate this object's parent to NVMain type
    if( NVMTypeMatches(NVMain) )
    {
		//std::cout<<"NVMTypeMatches true"<<std::endl;
        /* Ensure the Migrator translator is used. */
		//parent module is who issue migration??
        Migrator *migratorTranslator = dynamic_cast<Migrator *>(parent->GetTrampoline( )->GetDecoder( ));
        assert( migratorTranslator != NULL );

        /* Migrations in progress must be served from the buffers during migration. */
        if( GetCurrentHookType( ) == NVMHOOK_PREISSUE && migratorTranslator->IsBuffered( request->address ) )
        {
            /* Short circuit this request so it is not queued. */
            rv = false;
            /* Complete the request, adding some buffer read latency. */
			//call back parent objects function "RequestComplete"
            GetEventQueue( )->InsertEvent( EventResponse, parent->GetTrampoline( ), request,
                              GetEventQueue()->GetCurrentCycle()+bufferReadLatency );

            bufferedReads++;

            return rv;
        }

        /* Don't inject results before the original is issued to prevent deadlock */
        if( GetCurrentHookType( ) != NVMHOOK_POSTISSUE )
        {
            return rv;
        }

        /* See if any migration is possible (i.e., no migration is in progress) */
        bool migrationPossible = false;
		
		if(request->address.GetChannel()==promotionChannel && !migratorTranslator->IsMigrated(request->address) )
		{
			std::cout<<"va :"<<request->address.GetVirtualAddress()<<"  pa:"<<request->address.GetPhysicalAddress()<<" already in the specified memory area , no need to migration !"<<std::endl;
		}
        if( !migratorTranslator->Migrating( ) 
            && !migratorTranslator->IsMigrated( request->address ) 
            && request->address.GetChannel( ) != promotionChannel )
        {
                migrationPossible = true;
		}

        if( migrationPossible )
        {
            assert( !demoBuffered && !promoBuffered );
                /* 
                 *  Note: once IssueCommand is called, this hook may receive
                 *  a different parent, but fail the NVMTypeMatch check. As a
                 *  result we need to save a pointer to the NVMain class we
                 *  are issuing requests to.
                 */
                NVMObject *savedParent = parent->GetTrampoline( );

                /* Discard the unused column address. */
                uint64_t row, bank, rank, channel, subarray , col;
                request->address.GetTranslatedAddress( &row, &col, &bank, &rank, &channel, &subarray );
                uint64_t promoteeAddress = migratorTranslator->ReverseTranslate( row, 0, bank, rank, channel, subarray );
				std::cout<<"request channel: "<<request->address.GetChannel()<<" promotion channel: "
					<<promotionChannel<<std::endl;
				
					//std::cout<<"begin migration"<<std::endl;
					promotee.SetPhysicalAddress( promoteeAddress );
					promotee.SetTranslatedAddress( row, 0, bank, rank, channel, subarray );
					//std::cout<<"migrate from "<<promoteeAddress<<"->"<<demoRequest->address.GetPhysicalAddress()<<std::endl;
					/* Pick a victim to replace. */
					ChooseVictim( migratorTranslator, promotee, demotee );

					assert( migratorTranslator->IsMigrated( demotee ) == false );
					assert( migratorTranslator->IsMigrated( promotee ) == false );

					if( atomic )
					 {
						migratorTranslator->StartMigration( request->address, demotee );
						migratorTranslator->SetMigrationState( promotee, MIGRATION_DONE );
						migratorTranslator->SetMigrationState( demotee, MIGRATION_DONE );
					 }
					 /* Lastly, make sure we can queue the migration requests. */
					else if( CheckIssuable( promotee, READ ) &&
                         CheckIssuable( demotee, READ ) )
					 {
						migratorTranslator->StartMigration( request->address, demotee );

						promoRequest = new NVMainRequest( ); 
						demoRequest = new NVMainRequest( );

						promoRequest->address = promotee;
						promoRequest->type = READ;
	                    promoRequest->tag = MIG_READ_TAG;
		                promoRequest->burstCount = numCols;
	
		                demoRequest->address = demotee;
			            demoRequest->type = READ;
				        demoRequest->tag = MIG_READ_TAG;
					    demoRequest->burstCount = numCols;
	
		                promoRequest->owner = savedParent;
			            demoRequest->owner = savedParent;

						std::cout<<std::endl<<"va : "<<request->address.GetVirtualAddress()<<" migrate from "<<promoteeAddress<<"->"<<demoRequest->address.GetPhysicalAddress()<<std::endl;

				        savedParent->IssueCommand( promoRequest );
					    savedParent->IssueCommand( demoRequest );
					 }
					 else
					 {
						std::cout<<"queue wait"<<std::endl;
						queueWaits++;
					  }
            
		}
    }
    return rv;
}


void StaticMigrator::ChooseVictim( Migrator *at, NVMAddress& /*promotee*/, NVMAddress& victim )
{
    /*
     *  Since this is no method called after every module in the system is 
     *  initialized, we check here to see if we have queried the memory system
     *  about the information we need.
     */
    if( !queriedMemory )
    {
        /*
         *  Our naive replacement policy will simply circle through all the pages
         *  in the fast memory. In order to count the pages we need to count the
         *  number of rows in the fast memory channel. We do this by creating a
         *  dummy request which would route to the fast memory channel. From this
         *  we can grab it's config pointer and calculate the page count.
         */
        NVMainRequest queryRequest;
		//set query request's channel to promotionChannel
        queryRequest.address.SetTranslatedAddress( 0, 0, 0, 0, promotionChannel, 0 );
        queryRequest.address.SetPhysicalAddress( 0 );
        queryRequest.type = READ;
        queryRequest.owner = this;

        NVMObject *curObject = NULL;
		//search all children of parent , only if find the child node that can cast to SubArray safely , and assign it to curObject(find Subarray Object ) 
        FindModuleChildType( &queryRequest, SubArray, curObject, parent->GetTrampoline( ) );

        SubArray *promotionChannelSubarray = NULL;
        promotionChannelSubarray = dynamic_cast<SubArray *>( curObject );

        assert( promotionChannelSubarray != NULL );
        Params *p = promotionChannelSubarray->GetParams( );
        promotionChannelParams = p;

        totalPromotionPages = p->RANKS * p->BANKS * p->ROWS;
        currentPromotionPage = 0;

        if( p->COLS != numCols )
        {
            std::cout << "Warning: Page size of fast and slow memory differs." << std::endl;
        }

        queriedMemory = true;
    }

    /*
     *  From the current promotion page, simply craft some translated address together
     *  as the victim address.
     */
    uint64_t victimRank, victimBank, victimRow, victimSubarray, subarrayCount;
    ncounter_t promoPage = currentPromotionPage;

    victimRank = promoPage % promotionChannelParams->RANKS;
    promoPage >>= NVM::mlog2( promotionChannelParams->RANKS );

    victimBank = promoPage % promotionChannelParams->BANKS;
    promoPage >>= NVM::mlog2( promotionChannelParams->BANKS );

    subarrayCount = promotionChannelParams->ROWS / promotionChannelParams->MATHeight;
    victimSubarray = promoPage % subarrayCount;
    promoPage >>= NVM::mlog2( subarrayCount );

    victimRow = promoPage;

    victim.SetTranslatedAddress( victimRow, 0, victimBank, victimRank, promotionChannel, victimSubarray );
    uint64_t victimAddress = at->ReverseTranslate( victimRow, 0, victimBank, victimRank, promotionChannel, victimSubarray );
    victim.SetPhysicalAddress( victimAddress );

    currentPromotionPage = (currentPromotionPage + 1) % totalPromotionPages;
}


void StaticMigrator::Cycle( ncycle_t /*steps*/ )
{

}

bool StaticMigrator::ParseRequest(NVMainRequest* req) 
{
	bool ret = false;		
	//std::cout<<" request va is:"<<(req->address.GetVirtualAddress())<<" pa:"<<req->address.GetPhysicalAddress()<<std::endl;

	if(!signal_inited)
	{
		GetSignal();
	}
	else{
		//std::cout<<"signal is"<<std::hex<<signal<<std::endl;
		if(((req->address.GetVirtualAddress()>>12)== signal>>12 )&&(req->address.GetVirtualAddress()>0) )
		{
			//std::cout<<"enter parse request"<<std::endl;
			if( ReadCommand(cmd))
			{
				 can_migrate = true;
				//WriteDoneFlag();
			    start_addr = cmd->start_addr;
				op = cmd->optype;
			}
		}

		if( ((req->address.GetVirtualAddress()>>12)==(start_addr>>12))&&(req->address.GetVirtualAddress()>0) )
		{	
			 
			std::vector<uint64_t>::iterator it;
			uint64_t tmp_channel = req->address.GetChannel();
			//std::cout<<"request channel:"<<req->address.GetChannel()<<std::endl;
			if( op==DRAM_MEM )
			{
					//std::cout<<"optype DRAM_MEM , current channel 1 is "<<req->address.GetChannel()<<" address is:"<<req->address.GetPhysicalAddress()<<std::endl;
					//current channel is not DRAM CHANNEL, set promotion channel to DRAM channels in turn; else set promotion channel to current channel
					//original channel is DRAM
					for( it = dram_channel.begin(); it!=dram_channel.end();it++)
					{
						if( (*it)==tmp_channel)
						{
							promotionChannel = tmp_channel;
							break;
						}
					}
					if( it == dram_channel.end())
					{
						promotionChannel = dram_channel[cur_dram];
					//	UpdateCurChannel( cur_dram , dram_num );
					}
			}
			else if( op==PCM_MEM )
			{
					//original channel is PCM
					for( it = nvm_channel.begin(); it!=nvm_channel.end();it++)
					{
						if( (*it)==tmp_channel)
							promotionChannel = tmp_channel;
					}
					
					if( it == nvm_channel.end())
					{
						//std::cout<<"migrating from PCM to DRAM"<<std::endl;
						promotionChannel = nvm_channel[cur_nvm];
					//	UpdateCurChannel( cur_dram , dram_num );
					}
			}
				//std::cout<<"promotion channel is"<<promotionChannel<<std::endl;		
				ret = true;
		}
	}	
	return ret;
}

void StaticMigrator::UpdateCurChannel( uint64_t &cur_channel , uint64_t channel_num)
{
	if(cur_channel == channel_num-1)
		cur_channel =0;
	else
		cur_channel++;
}


bool StaticMigrator::GetSignal()
{
	bool ret = false;
	int fd;
//	int read_len;
	uint64_t* signal_ptr = new uint64_t;
	if( (fd = open( "signal" , O_RDWR , 00777)) != -1 )
	{
		if (  read(fd, signal_ptr , sizeof(uint64_t)) == sizeof(uint64_t) ) 
		{
			ret = true;
			std::cout<<"getsignal: "<<std::hex<<*signal_ptr<<std::endl;	
			signal = *signal_ptr;
			signal_inited = true;
		}
		close(fd);
	}

	if(signal_ptr)
	{
		delete signal_ptr;
		signal_ptr = NULL;
	}
	return ret;
}


bool  StaticMigrator::ReadCommand( command* cmd )
{

	int fd;
	bool ret = false;
	if(	(fd = open("command" , O_CREAT|O_RDWR,00777)) != -1)
	{
		//std::cout<<"sizeof(command) : "<<sizeof(command)<<std::endl;
		if( read(fd , cmd ,sizeof(command))==sizeof(command))
		{
		//	std::cout<<"read address"<<cmd->start_addr<<"to command"<<std::endl;
			ret = true;
		}
	 close(fd);
	}
	return ret;
}

/*
 * once a migration complete,write "migration done" to file command
 * representing next migration can start now
 *
 */
void StaticMigrator::WriteDoneFlag()
{
	int fd;
	char migra_done_str[255] = "migration done";
		
	//std::cout<<"write done flag"<<std::endl;
	if( (fd = open("command",O_CREAT|O_RDWR|O_TRUNC,00777)) != -1 )
	{
		size_t len = write(fd , migra_done_str , strlen(migra_done_str));
		assert(len == strlen(migra_done_str));
	}
	close(fd);
}
