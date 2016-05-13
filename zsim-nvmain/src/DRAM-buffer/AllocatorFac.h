/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.
 */
#ifndef _DRAMBUFFER_FAC_
#define _DRAMBUFFER_FAC_
#include "memory_hierarchy.h"
#include "DRAM-buffer/FairAllocator.h"

class AllocatorFac
{
	public:
		static BaseDRAMBufferManager* CreateAllocator(Address buffer_size, unsigned process_queue, std::string type = "")
		{
			if( type == "FairAlloctor")
			{
				//std::cout<<"new fair allocator"<<std::endl;
				return new FairAllocator( process_queue , buffer_size);
			}
			else{
				//std::cout<<"new DRAM buffer manager"<<std::endl;
				return new FairAllocator( process_queue , buffer_size );
			}
			
		}
};
#endif
