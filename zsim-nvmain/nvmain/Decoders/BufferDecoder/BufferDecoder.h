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

#ifndef _BUFFER_DECODER_H__
#define _BUFFER_DECODER_H__

#include <map>
#include "src/AddressTranslator.h"
#include "include/Exception.h"
#include "include/CommonEnums.h"

namespace NVM
{
	class BufferDecoder : public AddressTranslator
	{
		public:
			BufferDecoder();
			~BufferDecoder();
			/**---------address translate related---------**/
			void Translate( uint64_t address, uint64_t *row, uint64_t *col,
					  uint64_t *bank,uint64_t *rank, uint64_t *channel,
					  uint64_t *subarray );

			void Translate( NVMainRequest *request, uint64_t *row, 
							uint64_t *col, uint64_t *bank, 
							uint64_t *rank, uint64_t *channel , uint64_t *subarray);
			uint64_t ReverseTranslate( const uint64_t& row, const uint64_t& col,
				const uint64_t& bank, const uint64_t& rank,
				const uint64_t& channel,const uint64_t& subarray);
		   
			
			virtual void SetAddrWidth( unsigned buffer_width , unsigned main_mem_width);

			void SetBufferShift( unsigned shift )	
			{	buffer_shift = shift;}

			void SetMemColBytes( unsigned bytes)
			{ mem_col_bytes = bytes; }

			unsigned GetMemColBytes()
			{	return mem_col_bytes;}
			
			void SetBufferColBytes( unsigned bytes)
			{	buffer_col_bytes = bytes; }

			unsigned GetBufferColBytes()
			{	return buffer_col_bytes;}

			/**-----------fetch block related-------------**/
			void StartBuffer( NVMAddress& srcAddr, NVMAddress& dstAddr);
			void StartBuffer(NVMAddress &srcAddr , NVMAddress &dstAddr , unsigned offset_shift );
			bool SetOpComplete(NVMAddress &addr);
			bool SetOpComplete(NVMAddress &addr , unsigned offset_shift);
			void RegisterStats();

			bool isCaching()
			{		return caching; }
			using AddressTranslator::Translate;

		private:
			bool inline is_buffer(uint64_t addr);
			unsigned buffer_width_;
			unsigned main_mem_width_;
			uint64_t buffer_mask_;
			uint64_t base_buffer_addr;

			unsigned buffer_shift;
			uint64_t src_key , dst_key;
			std::map<uint64_t , bool> op_state;
			bool caching;
			uint64_t cache_time;
			unsigned mem_col_bytes;
			unsigned buffer_col_bytes;
	};
};
#endif
