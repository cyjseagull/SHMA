/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#include "page_table_walker.h"
#include "tlb/common_tlb.h"
void PageTableWalker::SetPaging( uint32_t proc_id , BasePaging* copied_paging)
{
	futex_lock(&walker_lock);
	procIdx = proc_id;
	paging = copied_paging;
	futex_unlock(&walker_lock);
}

PageTableWalker::~PageTableWalker()
{
}


/***------------simulation timing and state related----------***/
void PageTableWalker::write_through( MemReq& req)
{
	assert(paging);
	paging->access(req);
}

uint64_t PageTableWalker::access( MemReq& req)
{
	assert(paging);
	period++;
	Address addr = PAGE_FAULT_SIG;
	addr = paging->access(req);
	//page fault
	if( addr == PAGE_FAULT_SIG )	
	{
		addr = do_page_fault(req , PCM_PAGE_FAULT);
	}
	//suppose page table walking time when tlb miss is 20 cycles
	return addr;	//find address
}

