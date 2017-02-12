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

#ifndef __NVMAIN_UTILS_FlatMigrator_H__
#define __NVMAIN_UTILS_FlatMigrator_H__

//#include "NVM/Flat_RBLA_NVMain/FlatRBLANVMain.h"
#include "Decoders/FlatDecoder/FlatDecoder.h"
#include "NVM/Flat_RBLA_NVMain/FlatRBLANVMain.h"
#include "src/NVMObject.h"
#include "src/Params.h"
#include "include/NVMainRequest.h"
#include "NVM/nvmain.h"

namespace NVM {
#define MIG_READ_TAG GetTagGenerator( )->CreateTag("MIGREAD")
#define MIG_WRITE_TAG GetTagGenerator( )->CreateTag("MIGWRITE")
#define DELAYED_MIGRATION_TAG GetTagGenerator()->CreateTag("DelayedMigration") 
#define RETRY_MIGRATION_TAG GetTagGenerator()->CreateTag("RetryMigration") 

class FlatRBLANVMain;
class FlatMigrator : public NVMObject
{
  public:
    FlatMigrator( );
    virtual ~FlatMigrator( );

    void Init( Config *config );

    bool IssueAtomic( NVMainRequest *request );
    bool IssueCommand( NVMainRequest *request );
    bool RequestComplete( NVMainRequest *request );
	void SetFastMemory( NVMain* fast_mem)
	{
		fast_memory = fast_mem;
	}

	void SetSlowMemory( NVMain* slow_mem)
	{
		slow_memory = slow_mem;
	}
	
	void SetMemory( FlatRBLANVMain* mem)
	{
		memory = mem;
		migratorTranslator = dynamic_cast<FlatDecoder*>(memory->GetDecoder( ));
		assert( migratorTranslator);
	}
  protected:
    virtual bool TryMigration( NVMainRequest *request, bool atomic );
    virtual void ChooseVictim( FlatDecoder *at, NVMAddress& promo, NVMAddress& victim );
  public:
	static uint64_t migration_cycles;
  private:
	bool CheckIssuable( NVMAddress address, OpType type, bool is_fast=false );
  private:
	uint64_t tbuffer_read;
	uint64_t total_pages; 
	uint64_t current_page;
	uint64_t channels, ranks, banks,rows, cols;
	NVMain* fast_memory;
	NVMain* slow_memory;
	FlatRBLANVMain* memory;
	FlatDecoder *migratorTranslator;

	bool local_buffered;
	bool remote_buffered;
	uint64_t migration_count;
	uint64_t queue_waits;
	uint64_t buffered_reads;

	NVMainRequest* local_req;
	NVMainRequest* remote_req;
	NVMAddress local_addr;
	uint64_t begin_cycle;
	NVMAddress remote_addr;
};
};
#endif

