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

#include "Decoders/BufferDecoder/BufferDecoder.h"
#include <cassert>
//#include "include/common/common_functions.h"
using namespace NVM;
BufferDecoder::BufferDecoder():buffer_width_(0),main_mem_width_(0),
			buffer_mask_(0),base_buffer_addr(0),buffer_shift(0),
			src_key(0),dst_key(0),caching(false),cache_time(0),
			mem_col_bytes(0),buffer_col_bytes(0)
{
	op_state.clear();
}


BufferDecoder::~BufferDecoder()
{
}

void BufferDecoder::Translate(uint64_t address, uint64_t *row, uint64_t *col, uint64_t *bank,uint64_t *rank, uint64_t *channel, uint64_t *subarray )
{
	assert(buffer_width_>0 && main_mem_width_>0 && buffer_mask_>0);
	//access cache
	address &= buffer_mask_;
	AddressTranslator::Translate( address , row , col , bank, rank , channel , subarray);
}


void BufferDecoder::Translate( NVMainRequest *request, uint64_t *row, uint64_t *col, uint64_t *bank,
				   uint64_t *rank, uint64_t *channel, uint64_t *subarray )
{
	uint64_t addr = request->address.GetPhysicalAddress();
	addr &= buffer_mask_;
	//std::cout<<"addr is:"<<addr<<std::endl;
	AddressTranslator::Translate( request , row , col , bank, rank , channel , subarray);
	//std::cout<<"translated address, row:"<<*row<<" channel:"<<*channel<<std::endl;
}


uint64_t BufferDecoder::ReverseTranslate( const uint64_t& row, const uint64_t& col,const uint64_t& bank,
							const uint64_t& rank, const uint64_t& channel,
							const uint64_t& subarray )
{
	uint64_t tmp_pa = AddressTranslator::ReverseTranslate( row , col , bank , rank , channel , subarray );
	return tmp_pa | base_buffer_addr;
}


void BufferDecoder::SetAddrWidth(unsigned buffer_width , unsigned main_mem_width)
{
	buffer_width_ =  buffer_width;
	main_mem_width_ = main_mem_width;
	buffer_mask_ = (((uint64_t)1<<buffer_width)-1);
	base_buffer_addr = (uint64_t)1<<main_mem_width;
}

/***----------fetch block related--------***/
void BufferDecoder::StartBuffer(NVMAddress &srcAddr , NVMAddress &dstAddr)
{
	assert( buffer_shift);
	StartBuffer( srcAddr , dstAddr , buffer_shift);
}

void BufferDecoder::StartBuffer(NVMAddress &srcAddr , NVMAddress &dstAddr , unsigned offset_shift )
{
	assert (caching == false);
	src_key = (srcAddr.GetPhysicalAddress())>>offset_shift;
	dst_key = (dstAddr.GetPhysicalAddress())>>offset_shift;
	assert( op_state.count(src_key)==0);
	assert( op_state.count(dst_key)==0);
	//begin buffer
	op_state[src_key] = false;
	op_state[dst_key] = false;
	caching = true;
}

bool BufferDecoder::SetOpComplete(NVMAddress &addr)
{
	assert( buffer_shift );
	return SetOpComplete(addr , buffer_shift);
}

bool BufferDecoder::SetOpComplete(NVMAddress &addr , unsigned offset_shift)
{
	uint64_t key=(addr.GetPhysicalAddress())>>offset_shift;
	assert( op_state.count(key));
	op_state[key] = true;
	//fetch data block complete
	if( op_state[src_key] && op_state[dst_key])
	{
		cache_time++;
		caching = false;
		op_state.clear();
		return true;
	}
	return false;
}


void BufferDecoder::RegisterStats()
{
	AddStat(cache_time);
}
