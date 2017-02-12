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

#ifndef _FlatDecoder_H__
#define _FlatDecoder_H__

#include "src/AddressTranslator.h"
#include "src/Config.h"
#include "include/NVMAddress.h"
#include "NVM/nvmain.h"
#include <map>
#include <set>
namespace NVM
{
enum FlatDecoderState
{      
    FLATMIGRATION_UNKNOWN = 0, // Error state
    FLATMIGRATION_READING,     // Read in progress for this page
    FLATMIGRATION_BUFFERED,    // Read is done, waiting for writes to be queued
    FLATMIGRATION_WRITING,     // Writes queued, waiting for request complete
    FLATMIGRATION_DONE         // Migration successfully completed
};
struct PAddr
{
	uint64_t channel;
	uint64_t rank;
	uint64_t bank;
	uint64_t subarray;
	uint64_t row;
	PAddr( uint64_t channel_, uint64_t rank_, uint64_t bank_, 
		   uint64_t subarray_, uint64_t row_):channel(channel_),
		   rank(rank_), bank(bank_), subarray(subarray_),row(row_)
		   {}
	PAddr(){}
};

struct VAddr
{
	uint64_t key;
	PAddr concrete_addr;
	bool is_fast;
	VAddr( uint64_t value, PAddr &addrs, bool is_fast_ = false): 
		   key(value),concrete_addr(addrs), is_fast(is_fast_){}
	VAddr(){}
	uint64_t get_key()
	{	return key;   }
	void set_key( uint64_t value )
	{   value = key;  }
};

class FlatDecoder : public AddressTranslator
{
  public:
    FlatDecoder();

    void Init( Config *total_conf, Config* slow_conf);

    void Translate2( uint64_t& address, uint64_t *row, uint64_t *col, uint64_t *bank, 
                            uint64_t *rank, uint64_t *channel, uint64_t *subarray );
	
	uint64_t ReverseTranslate2( const uint64_t& row, const uint64_t& col,
			const uint64_t& bank,const uint64_t& rank, const uint64_t& channel,
			const uint64_t& subarray, bool is_fast);

    using AddressTranslator::Translate;

    uint64_t StartMigration( NVMAddress& promotee, NVMAddress& demotee , uint64_t start_cycle = 0);
    void SetMigrationState( NVMAddress& address, FlatDecoderState newState );
    bool Migrating( );
    bool IsBuffered( NVMAddress& address);
    bool IsMigrated( NVMAddress& address );
    void RegisterStats( );

	void SetFastMemory( NVMain* mem)
	{
		fastMemory = mem;
		fast_mem_size = fastMemory->GetMemorySize();
		memory_slice = fast_mem_size/total_channels;	
	}

	void SetSlowMemory( NVMain* mem)
	{   slowMemory = mem;  }

	void SetFastMemoryBits( unsigned bits)
	{
		fast_mem_bits = bits;
	}
  private:
    std::map<uint64_t, PAddr > migrationMap;
	std::map<uint64_t, VAddr> pa_to_va;
    std::map<uint64_t, FlatDecoderState> migrationState;
	
	uint64_t total_channels, total_banks, total_ranks, total_subarrays, total_rows;
	uint64_t slow_channels, slow_banks, slow_ranks, slow_subarrays, slow_rows;

    /* Pages being swapped in and swapped out. */
    bool migrating;
    uint64_t inputPage, outputPage;

	NVMain* fastMemory;
	NVMain* slowMemory;
	uint64_t memory_slice;
	unsigned fast_mem_bits;
	uint64_t fast_mem_size;
    uint64_t GetAddressKey( NVMAddress& address, bool& is_fast);
	uint64_t GetAddressKey( uint64_t &paddr, bool& is_fast );

	uint64_t clflush( uint64_t row, uint64_t bank,uint64_t rank,  uint64_t channel,
					uint64_t subarray , bool is_fast, uint64_t start_cycle);
};
};
#endif

