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

#include "Decoders/FlatDecoder/FlatDecoder.h"

#include <iostream>
#include <sstream>
#include <cassert>

using namespace NVM;

FlatDecoder::FlatDecoder( ): total_channels(0), total_banks(0),
	total_ranks(0),total_subarrays(0),slow_channels(0),
	slow_banks(0),slow_ranks(0),slow_subarrays(0),migrating(false),
	inputPage(0), outputPage(0),fastMemory(NULL),
	slowMemory(NULL), memory_slice(0),fast_mem_bits(0),fast_mem_size(0)
{
    //migrationMap.clear( );
    migrationState.clear( );
}

void FlatDecoder::Init(Config *fast_conf, Config* slow_conf )
{
    slow_channels = slow_conf->GetValue( "CHANNELS" );
    slow_banks = slow_conf->GetValue( "BANKS" );
    slow_ranks = slow_conf->GetValue( "RANKS" );
    slow_subarrays = slow_conf->GetValue( "ROWS" ) / slow_conf->GetValue( "MATHeight" );
	slow_rows = slow_conf->GetValue("ROWS");

    total_channels = fast_conf->GetValue( "CHANNELS" )+slow_channels;
    total_banks = fast_conf->GetValue( "BANKS" )+slow_banks;
    total_ranks = fast_conf->GetValue( "RANKS" )+slow_ranks;
    total_subarrays = fast_conf->GetValue( "ROWS" ) / slow_conf->GetValue( "MATHeight" ) + slow_subarrays;
	total_rows = fast_conf->GetValue("ROWS") + slow_rows;
}


void FlatDecoder::RegisterStats( )
{
}


uint64_t FlatDecoder::GetAddressKey( uint64_t &paddr , bool& is_fast)
{
    uint64_t col, row, bank, rank, subarray, channel;
	uint64_t dram_addr = (paddr >> (uint64_t)fast_mem_bits);
	is_fast = false;
	//std::cout<<"*paddr:"<<paddr<<" fast mem bits:"<<fast_mem_bits<<std::endl;
	//std::cout<<"slow memory decoder:"<<slowMemory->GetDecoder()<<std::endl;
	if( !dram_addr )
	{
		uint64_t unit_num = paddr/memory_slice;
		//std::cout<<"paddr:"<<paddr<<" unit_num:"<<unit_num<<std::endl;
		//access DRAM
		if( unit_num%2==1 )
		{
			paddr -= (((unit_num-1)/2+1)*memory_slice);
			//std::cout<<"after paddr:"<<paddr<<std::endl;
			fastMemory->GetDecoder()->Translate( paddr, &row, &col, &bank,
					&rank, &channel, &subarray);
			is_fast = true;
		}
		else
		{
			paddr -= ((unit_num/2)*memory_slice);
			//std::cout<<"after paddr:"<<paddr<<std::endl;
			slowMemory->GetDecoder()->Translate(paddr, &row, &col, &bank,
					&rank, &channel, &subarray);
		}
	}
	else
			
	{
		paddr -= (2*fast_mem_size);
		//std::cout<<"after paddr:"<<paddr<<std::endl;
		//std::cout<<"translate through slow memory"<<std::endl;
		slowMemory->GetDecoder()->Translate(paddr, &row, &col, &bank,
					&rank, &channel, &subarray);
		//std::cout<<"translate ended"<<std::endl;
	}
	//std::cout<<"address key of "<<paddr<<":"<<row<<" "<<bank<<" "<<rank<<" "<<channel<<" "<<subarray<<std::endl;
	if( is_fast)
	{
		channel += slow_channels;
		subarray += slow_subarrays;
		rank += slow_ranks;
		bank += slow_banks;
		row += slow_rows;
		//std::cout<<"is fast"<<std::endl;
	}
	return (channel*total_ranks*total_banks*total_rows*total_subarrays
			+ rank*total_banks*total_rows*total_subarrays
			+ bank*total_rows*total_subarrays
			+ row*total_subarrays
			+ subarray);
}
/*
 *  Calculates a unique key for each possible unit of memory that can be
 *  migrated. In this case, we are migrating single rows of a bank.
 */
uint64_t FlatDecoder::GetAddressKey( NVMAddress& address, bool& is_fast)
{
	uint64_t paddr = address.GetPhysicalAddress();
	uint64_t ret = GetAddressKey( paddr, is_fast);
	address.SetVirtualAddress( paddr );
	return ret;
    /*uint64_t col, row, bank, rank, subarray, channel;
	uint64_t paddr = address.GetPhysicalAddress();
	uint64_t dram_addr = (paddr >> (uint64_t)fast_mem_bits);
	is_fast = false;
	std::cout<<"*paddr:"<<paddr<<" fast mem bits:"<<fast_mem_bits<<std::endl;
	//std::cout<<"slow memory decoder:"<<slowMemory->GetDecoder()<<std::endl;
	if( !dram_addr )
	{
		uint64_t unit_num = paddr/memory_slice;
		std::cout<<"paddr:"<<paddr<<" unit_num:"<<unit_num<<std::endl;
		//access DRAM
		if( unit_num%2==1 )
		{
			paddr -= (((unit_num-1)/2+1)*memory_slice);
			std::cout<<"after paddr:"<<paddr<<std::endl;
			address.SetVirtualAddress(paddr);
			fastMemory->GetDecoder()->Translate( paddr, &row, &col, &bank,
					&rank, &channel, &subarray);
			is_fast = true;
		}
		else
		{
			paddr -= ((unit_num/2)*memory_slice);
			std::cout<<"after paddr:"<<paddr<<std::endl;

			address.SetVirtualAddress(paddr); //real physical address
			//std::cout<<"translate through slow memory"<<std::endl;
			slowMemory->GetDecoder()->Translate(paddr, &row, &col, &bank,
					&rank, &channel, &subarray);
		}
	}
	else
			
	{
		paddr -= (2*fast_mem_size);
		std::cout<<"after paddr:"<<paddr<<std::endl;
		address.SetVirtualAddress(paddr);
		//std::cout<<"translate through slow memory"<<std::endl;
		slowMemory->GetDecoder()->Translate(paddr, &row, &col, &bank,
					&rank, &channel, &subarray);
		//std::cout<<"translate ended"<<std::endl;
	}
	//std::cout<<"address key of "<<paddr<<":"<<row<<" "<<bank<<" "<<rank<<" "<<channel<<" "<<subarray<<std::endl;
	if( is_fast)
	{
		channel += slow_channels;
		subarray += slow_subarrays;
		rank += slow_ranks;
		bank += slow_banks;
		row += slow_rows;
	}
	std::cout<<"is fast"<<std::endl;
	return (channel*total_ranks*total_banks*total_rows*total_subarrays
			+ rank*total_banks*total_rows*total_subarrays
			+ bank*total_rows*total_subarrays
			+ row*total_subarrays
			+ subarray);*/
}

void FlatDecoder::StartMigration( NVMAddress& promotee, NVMAddress& demotee )
{
    /* 
     *  The address being demoted is assumed to be in the "fast" memory and
     *  the address being promoted in the slow memory, therefore we define
     *  the promotion channel as the demotion address' value and similarly
     *  for demotion channel.
     */
    /* Get unique keys for each page to migrate. */
    //uint64_t promokey = GetAddressKey( promotee,is_fast );
    //uint64_t demokey = GetAddressKey( demotee , is_fast);
	bool is_fast;
	uint64_t promo_addr = promotee.GetPhysicalAddress();
	uint64_t demo_addr = demotee.GetPhysicalAddress();
    uint64_t promokey = GetAddressKey( promo_addr, is_fast );
    uint64_t demokey = GetAddressKey( demo_addr, is_fast);

	//std::cout<<"key of "<<promotee.GetPhysicalAddress()<<":"<<promokey<<std::endl;
	//std::cout<<"key of "<<demotee.GetPhysicalAddress()<<":"<<demokey<<std::endl;
    /* Ensure we are not already migrating a page. */
    assert( migrating == false );

    /*
     *  Set the new channel decodings immediately, but mark the migration
     *  as being in progress.
     */
	/*PAddr tmp;
	tmp.channel = demotee.GetChannel();
	tmp.rank = demotee.GetRank();
	tmp.bank = demotee.GetBank();
	tmp.row = demotee.GetRow();
	tmp.subarray = demotee.GetSubArray();
    migrationMap[promokey] = tmp;*/
	//std::cout<<"*promokey:"<<promokey<<std::endl;
	//std::cout<<tmp.row<<" "<<tmp.bank<<" "<<tmp.rank<<" "<<tmp.channel<<" "<<tmp.subarray<<std::endl;

	/*tmp.channel = promotee.GetChannel();
	tmp.rank = promotee.GetRank();
	tmp.bank = promotee.GetBank();
	tmp.row = promotee.GetRow();
	tmp.subarray = promotee.GetSubArray();
    migrationMap[demokey] = tmp;*/

    migrationState[promokey] = FLATMIGRATION_READING;
    migrationState[demokey] = FLATMIGRATION_READING;
	//std::cout<<"*demokey:"<<demokey<<std::endl;
	//std::cout<<tmp.row<<" "<<tmp.bank<<" "<<tmp.rank<<" "<<tmp.channel<<" "<<tmp.subarray<<std::endl;
	//std::cout<<"set migration state for promokey "<<promokey<<std::endl;
	//std::cout<<"set migration state for Demokey "<<demokey<<std::endl;
    /*
     *  Only one migration is allowed at a time; These values hold the
     *  key values for each migration page and marks a migration in
     *  progress.
     */
    migrating = true;
    inputPage = promokey;
    outputPage = demokey;
}

/*
 * set migrating state to newState,if migrating is done , set migrating false
 *
 */
void FlatDecoder::SetMigrationState( NVMAddress& address, FlatDecoderState newState)
{
    //Get the key and set the new state; Ensure the state is really new.
    //uint64_t key = GetAddressKey( address,is_fast );
	bool is_fast;
	uint64_t addr = address.GetVirtualAddress();
    uint64_t key = GetAddressKey( addr, is_fast);
	//std::cout<<key<<" state is:"<<migrationState[key]<<" "<<newState<<std::endl; 
	assert( migrationState.count( key ) != 0 );
    assert( migrationState[key] != newState );

    migrationState[key] = newState;

    /* If migration is done we can handle another migration */
    if( migrationState[inputPage] == FLATMIGRATION_DONE &&
        migrationState[outputPage] == FLATMIGRATION_DONE )
    {
        migrating = false;
    }
}


bool FlatDecoder::Migrating( )
{
    return migrating;
}


/*
 *  If the page was migrated, we should read from the new channel it which
 *  it was placed, since the buffer may no longer be valid.
 */
/*bool FlatDecoder::IsMigrated( NVMAddress &address )
{
	uint64_t addr = address.GetPhysicalAddress();
	bool is_fast;
    uint64_t key = GetAddressKey( addr, is_fast);
	//std::cout<<"key of "<<address.GetPhysicalAddress()<<":"<<key<<endl;
    bool rv = false;
    if( migrationMap.count( key ) != 0 && migrationState.count( key ) != 0
        && migrationState[key] == FLATMIGRATION_DONE )
    {
		//std::cout<<"request "<<address.GetPhysicalAddress()<<" already migrated"<<std::endl;
        rv = true;
    }
    return rv;
}*/


/*
 *  If a request has been read from one channel and placed in our temporary
 *  swap buffer, the data in the bank may no longer be valid. Therefore, we
 *  need to read the data from the temporary swap buffer instead.
 */
bool FlatDecoder::IsBuffered( NVMAddress& address )
{
	bool is_fast;
    uint64_t addr = address.GetPhysicalAddress();
	uint64_t key = GetAddressKey( addr, is_fast );
    bool rv = false;

    if (migrationState[key] == FLATMIGRATION_BUFFERED ||
			migrationState[key] == FLATMIGRATION_WRITING ) 
    {
        rv = true;
    }
    return rv;
}

/*
void FlatDecoder::Translate2( uint64_t &address, uint64_t *row, uint64_t *col, uint64_t *bank,
                          uint64_t *rank, uint64_t *channel, uint64_t *subarray )
{
    NVMAddress keyAddress;
    keyAddress.SetTranslatedAddress( *row, NULL, *bank, *rank, *channel, *subarray );
    keyAddress.SetPhysicalAddress( address );
	bool is_fast = false;
    uint64_t key = GetAddressKey( keyAddress, is_fast );
	address = keyAddress.GetVirtualAddress(); //get real address issued to corresponding decoder
	//std::cout<<"get col"<<std::endl;
	if( is_fast )
	{
		fastMemory->GetDecoder()->Translate(address,row,col,bank,rank,channel,subarray);	
	}
	else
		slowMemory->GetDecoder()->Translate(address,row,col,bank,rank,channel,subarray);	

    // Check if the page was migrated and migration is complete. 
	//only change channel
    if( migrationMap.count( key ) != 0 )
    {
        if( migrationState[key] == FLATMIGRATION_DONE )
        {
			PAddr tmp_addr = migrationMap[key];
			*row = tmp_addr.row;
			*bank = tmp_addr.bank;
			*rank = tmp_addr.rank;
			*channel = tmp_addr.channel;
			*subarray = tmp_addr.subarray;
        }
    }
	//std::cout<<"*key:"<<key<<std::endl;
	//std::cout<<*col<<" "<<*row<<" "<<*bank<<" "<<*rank<<" "<<*channel<<" "<<*subarray<<std::endl;
	address=ReverseTranslate2(*row, *col, *bank, *rank,*channel, *subarray,!is_fast);
}
*/

uint64_t FlatDecoder::ReverseTranslate2( const uint64_t& row, 
         const uint64_t& col, const uint64_t& bank,const uint64_t& rank, 
         const uint64_t& channel, const uint64_t& subarray, bool is_fast )
{
	//std::cout<<"reverse translator:"<<row<<" "<<bank<<" "<<rank<<" "<<channel
	//	<<" "<<subarray<<std::endl;
	uint64_t address;
	if( is_fast )
	{
		address = fastMemory->GetDecoder()->
			ReverseTranslate(row, col, bank,rank,channel,subarray);
		uint64_t unit_num = (address/memory_slice)+1;
		return (address += (unit_num*memory_slice));
	}
	address = slowMemory->GetDecoder()->
		ReverseTranslate(row,col,bank,rank,channel,subarray);
	//NVM
	if( !(address>>fast_mem_bits) )
	{
		uint64_t unit_num = address/memory_slice;
		return (address	+= (unit_num*memory_slice));
	}
	else
		return(address += (2*fast_mem_size));
} 
