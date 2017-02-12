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

#ifndef __MULTI_QUEUE_NVMAIN_H__
#define __MULTI_QUEUE_NVMAIN_H__

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "src/Params.h"
#include "src/NVMObject.h"
#include "src/Prefetcher.h"
#include "include/NVMainRequest.h"
#include "traceWriter/GenericTraceWriter.h"
#include "MemControl/DRAMCache/DRAMCache.h"
#include "NVM/RBLA_NVMain/StatsStore.hh"
#include "NVM/Flat_NVMain/FlatNVMain.h"
#include "NVM/nvmain.h"
#include <memory>
#include "Decoders/FlatDecoder/FlatDecoder.h"
#include "Utils/FlatMigrator/FlatMigrator.h"
#include "include/global_vars.h"

#define QUEUE_SIZE 15
#define PAGE_SHIFT 12
namespace NVM {
enum PageType
{
	DRAM,
	PCM
};
struct PageDes
{
	uint64_t page_num;
	uint64_t ref_count;
	uint64_t expire_time;
	PageType page_type;	//PCM,DRAM
	PageDes(uint64_t pageNo, uint64_t refCount,
			uint64_t expire, PageType type)
		: page_num(pageNo), ref_count(refCount),
		expire_time(expire), page_type(type)
	{}
};
class MultiQueueNVMain : public NVMain
{
  public:
    MultiQueueNVMain( );
    ~MultiQueueNVMain( )
	{
		if( flat_memory )
			delete flat_memory;
	}

    void SetConfig( Config *conf, std::string memoryName = "defaultMemory", bool createChildren = true );

    bool IssueCommand( NVMainRequest *request );

    bool RequestComplete( NVMainRequest *request );

    void RegisterStats( );
    void CalculateStats( );
	void Cycle( ncycle_t steps );
	void SetMigrator( NVMObject* migra){
		migrator = dynamic_cast<FlatMigrator*>(migra);
		migrator->SetParent(this);
	}

	bool IsIssuable( NVMainRequest* req, FailReason* reason = NULL)
	{
		return flat_memory->IsIssuable(req, reason);
	}

	uint64_t GetMemorySize()
	{
		return flat_memory->GetMemorySize();
	}
	FlatNVMain* GetFlatMemory()
	{
		return flat_memory;
	}
  private:
	FlatNVMain* flat_memory;
	FlatMigrator* migrator;
	std::vector<std::list<PageDes*> > ranking_queue;
	std::list<PageDes*> dram_list;
	uint64_t dram_page_num;
};
};
#endif
