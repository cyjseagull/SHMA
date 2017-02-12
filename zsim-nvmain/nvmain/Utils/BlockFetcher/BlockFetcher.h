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
/*
 * added by YuJieChen
 * time : 2015.12.03
 */
#ifndef __BLOCK_FETCHER_H__
#define __BLOCK_FETCHER_H__

#include "src/NVMObject.h"
#include "src/Params.h"
#include "include/NVMainRequest.h"
#include "include/NVMAddress.h"
#include "Decoders/BufferDecoder/BufferDecoder.h"

namespace NVM {
#define BUFFER_FETCH GetTagGenerator()->CreateTag("BUFFER_FETCH") 
#define DELAYED_BUFFER GetTagGenerator()->CreateTag("DelayedBuffer") 
class BlockFetcher : public NVMObject
{
  public:
    BlockFetcher();
    virtual ~BlockFetcher( );
    void Init( Config* conf);
    bool IssueAtomic( NVMainRequest *req );
    bool IssueCommand( NVMainRequest *req );
    bool RequestComplete( NVMainRequest *req );
	void RegisterStats( );
	void CalculateStats( );
    void Cycle( ncycle_t steps );
  private:
    bool CheckIssuable( NVMAddress &address, OpType type );
	bool Fetch( NVMainRequest* request , uint64_t data_size , bool src_is_buffer = false , bool dst_is_buffer = true );
  private:
	NVMAddress srcNVMAddress;
	NVMAddress dstNVMAddress;
	NVMainRequest *srcReq; //read
	NVMainRequest *dstReq;  //write
	unsigned offset_shift;
	uint64_t succeed_cache_time;
	uint64_t drc_evicts;
	uint64_t caching_cycles;
	BufferDecoder* buffer_translator;
	unsigned mem_col_size;
	unsigned buffer_col_size;
};
};
#endif

