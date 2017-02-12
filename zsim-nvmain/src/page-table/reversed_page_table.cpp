/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#include <iterator>
#include "page-table/reversed_page_table.h"
#include "common/common_functions.h"

#include "DRAM-buffer/DRAM_buffer_block.h"
#include "tlb/common_func.h"
#include "MMU/zone.h"
//addr: virtual address
//pg_ptr: point to page table
int ReversedPaging::map_page_table( Address addr, void* pg_ptr, bool pbuffer)
{
	BasePDTEntry* mapped_entry;
	futex_lock(&(reversed_pgt_lock));
	int latency = paging->map_page_table(addr, pg_ptr, pbuffer,mapped_entry);
	Address vpn = addr >>(zinfo->page_shift);
	Address ppn;
	if( pbuffer )
		ppn = block_id_to_addr( ((DRAMBufferBlock*)pg_ptr)->block_id)>>(zinfo->block_shift);
	else
		ppn = ((Page*)pg_ptr)->pageNo;  
	//std::cout<<"map page table, ppn:"<<ppn<<" vpn:"<<vpn<<std::endl;
	if( mapped_entry )
	{
		Content* content = new Content( mapped_entry, vpn);
		assert( !reversed_pgt.count(ppn) );
		if( !(reversed_pgt).count(ppn))
		{
			reversed_pgt[ppn] = content;
		}
	}
	futex_unlock(&(reversed_pgt_lock));
	return latency;
}

uint64_t ReversedPaging::remap_page_table( Address ppn, Address dst_ppn, bool src_dram, bool dst_dram )
{

	void* page_ptr = NULL;
	void* dst_ptr = NULL;
	if( !src_dram )
	{
		page_ptr = (void*)zinfo->memory_node->get_page_ptr(ppn);
	}
	else
		page_ptr = (void*)zinfo->dram_manager->get_page_ptr(
				dram_addr_to_block_id(ppn<<zinfo->block_shift));
	
	if( !dst_dram )
	{
		dst_ptr = zinfo->memory_node->get_page_ptr(dst_ppn);
	}
	else
		dst_ptr = (void*)zinfo->dram_manager->get_page_ptr(
				dram_addr_to_block_id( dst_ppn<<zinfo->block_shift));
	assert(page_ptr);
	assert(dst_ptr);
	uint64_t latency = 0;
	futex_lock(&reversed_pgt_lock);
	if( reversed_pgt.count(ppn) )
	{
		//std::cout<<"remap:"<<ppn<<"("<<src_dram<<")"<<" -> "<<dst_ppn<<"("<<dst_dram<<")"<<" vpn:"<<get_vpn(ppn)<<std::endl;
		latency += zinfo->mem_access_time;
		(reversed_pgt[ppn])->get_pgt_entry()->set_next_level_address(dst_ptr);
		//(reversed_pgt[ppn])->get_pgt_entry()->clear_dirty();
		if( dst_dram)
		{
			//std::cout<<"remap "<<ppn<<" to dram"<<std::endl;
			(reversed_pgt[ppn])->get_pgt_entry()->set_buffer(true);
		}
		else
		{
			 //std::cout<<"remap "<<ppn<<" to pcm"<<std::endl;
			(reversed_pgt[ppn])->get_pgt_entry()->set_buffer(false);
		}
	}
	if( (reversed_pgt).count(dst_ppn) )
	{
		//std::cout<<"remap:"<<dst_ppn<<"("<<ppn<<")"<<" -> "<<dst_ppn<<"("<<dst_dram<<")"<<" vpn:"<<get_vpn(dst_ppn)<<std::endl;
		latency += zinfo->mem_access_time;
		(reversed_pgt[dst_ppn])->get_pgt_entry()->set_next_level_address(page_ptr);
		//(reversed_pgt[dst_ppn])->get_pgt_entry()->clear_dirty();
		if( src_dram )
		{
			 //std::cout<<"remap "<<dst_ppn<<" to dram"<<std::endl;
			(reversed_pgt[dst_ppn])->get_pgt_entry()->set_buffer(true);
		}
		else
		{
			 //std::cout<<"remap "<<dst_ppn<<" to pcm"<<std::endl;
			(reversed_pgt[dst_ppn])->get_pgt_entry()->set_buffer(false);
		}
	}
	futex_unlock(&reversed_pgt_lock);
	return latency;
}

void ReversedPaging::calculate_stats()
{

}
