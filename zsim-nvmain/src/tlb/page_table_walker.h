/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef PAGE_TABLE_WALKER_H
#define PAGE_TABLE_WALKER_H

#include <map>
#include <fstream>
#include "locks.h"
#include "g_std/g_string.h"
#include "memory_hierarchy.h"
#include "page-table/comm_page_table_op.h"
#include "common/global_const.h"
#include "page-table/page_table.h"

#include "MMU/memory_management.h"
#include "tlb/common_func.h"
#include "tlb/hot_monitor_tlb.h"
#include "DRAM-buffer/DRAM_buffer_block.h"
#include "tlb/tlb_entry.h"

#include "src/nvmain_mem_ctrl.h"
#include "include/NVMainRequest.h"
#include "core.h"
#include "ooo_core.h"
class PageTableWalker: public BasePageTableWalker
{
	public:
		//access memory
		PageTableWalker(const g_string& name , PagingStyle style): pg_walker_name(name),procIdx((uint32_t)(-1))
		{
			mode = style;
			period = 0;
			dirty_evict = 0;
			total_evict = 0;
			futex_init(&walker_lock);
		}
		~PageTableWalker();
		/*------tlb hierarchy related-------*/
		//bool add_child(const char* child_name ,  BaseTlb* tlb);
		/*------simulation timing and state related----*/
	
		uint64_t access( MemReq& req);		
		void write_through( MemReq& req);
		BasePaging* GetPaging()
		{ return paging;}
		void SetPaging( uint32_t proc_id , BasePaging* copied_paging);
		void convert_to_dirty( Address block_id)
		{
			zinfo->dram_manager->convert_to_dirty( procIdx , block_id );
		}
		const char* getName()
		{  return pg_walker_name.c_str(); }

		void calculate_stats()
		{
			info("%s evict time:%lu \t dirty evict time: %lu \n",getName(),total_evict,dirty_evict);
		}

		inline Address do_page_fault(MemReq& req, PAGE_FAULT fault_type)
		{
		    //allocate one page from Zone_Normal area
		    debug_printf("page falut, allocate free page through buddy allocator");
		    Page* page = NULL;
			if( zinfo->buddy_allocator)
			{
				page = zinfo->buddy_allocator->allocate_pages(0);
				if(page)
				{
					paging->map_page_table( req.lineAddr,(void*)page);
					return page->pageNo;
				}
			}
			//update page table
			return (req.lineAddr>>zinfo->page_shift);
		}
		
		DRAMBufferBlock* allocate_page( )
		{
			if( zinfo->dram_manager->should_reclaim() )
			{
				zinfo->dram_manager->evict( zinfo->dram_evict_policy);
			}
			return (zinfo->dram_manager)->allocate_one_page( procIdx);
		}

		template <typename T>
		void reset_tlb( T* tlb_entry)
		{
			tlb_entry->set_in_dram(false);
			tlb_entry->clear_counter();
		}

		template <typename T>
		Address do_dram_page_fault(MemReq& req, Address vpn ,uint32_t coreId, PAGE_FAULT fault_type , T* entry , bool is_itlb , bool &evict)
		{
			debug_printf("fetch pcm page %d to DRAM",(req.lineAddr>>zinfo->page_shift));
			//allocate dram buffer block
			DRAMBufferBlock* dram_block = allocate_page();
			if( dram_block)
			{
				Address dram_addr = block_id_to_addr( dram_block->block_id);
				//evict dram_block if it's dirty
				if( dram_block->is_occupied())
				{
					total_evict++;
					Address origin_ppn = dram_block->get_src_addr();
					if( dram_block->is_dirty())
					{
						evict = true;
						dirty_evict++;
						Address dst_addr = origin_ppn<<(zinfo->page_shift);
						if( NVMainMemory::fetcher)
						{
							NVM::NVMainRequest* nvmain_req = new NVM::NVMainRequest();
							//from nvm to dram
							//is nvm address
							nvmain_req->address.SetPhysicalAddress(dram_addr,true);
							//buffer address
							nvmain_req->address.SetDestAddress(dst_addr, false);
							nvmain_req->burstCount = zinfo->block_size;
							nvmain_req->type = NVM::FETCH; 
							(NVMainMemory::fetcher)->IssueCommand( nvmain_req );
						}
						else 
						{
							req.cycle +=200;
						}
					}
					//remove relations between tlb and invalidate dram
					Address vaddr = dram_block->get_vaddr();	
					T* tlb_entry = NULL;
					HotMonitorTlb<T>* recover_tlb = NULL;
					for( uint32_t i=0; i<zinfo->numCores; i++)
					{
						recover_tlb = dynamic_cast<HotMonitorTlb<T>* >
									  (zinfo->cores[i]->getInsTlb());;
						tlb_entry = recover_tlb->look_up( vaddr );
						if( tlb_entry)
						{
							reset_tlb( tlb_entry);
							tlb_entry = NULL;
						}
						recover_tlb = dynamic_cast<HotMonitorTlb<T>* >
									  (zinfo->cores[i]->getDataTlb());;
						tlb_entry = recover_tlb->look_up( vaddr );
						if( tlb_entry )
						{
							reset_tlb(tlb_entry);
							tlb_entry = NULL;
						}
					}
					Page* page_ptr = zinfo->memory_node->get_page_ptr(origin_ppn); 
					paging->map_page_table((vaddr<<zinfo->page_shift),(void*)page_ptr,false);
					dram_block->invalidate();
				}
				//call memory controller interface to cache page
				 if( NVMainMemory::fetcher)
				{
					NVM::NVMainRequest* nvm_req = new NVM::NVMainRequest();
					nvm_req->address.SetPhysicalAddress(req.lineAddr ,false );
					nvm_req->address.SetDestAddress(dram_addr , true);
					nvm_req->burstCount = zinfo->block_size;
					NVMainMemory::fetcher->IssueCommand( nvm_req );
				}
				else	//add fix latency express data buffering 
				{
					req.cycle += 200;
				}
				dram_block->validate(req.lineAddr>>(zinfo->block_shift));
				HotMonitorTlb<T>* tlb = NULL;		
				if( is_itlb )
					tlb = dynamic_cast<HotMonitorTlb<T>* >(zinfo->cores[coreId]->getInsTlb());
				else
					tlb = dynamic_cast<HotMonitorTlb<T>* >(zinfo->cores[coreId]->getDataTlb());
				tlb->remap_to_dram((dram_addr>>(zinfo->block_shift)) , entry);
			    dram_block->set_src_addr( entry->p_page_no );
				dram_block->set_vaddr( entry->v_page_no);
				debug_printf("after remap , vpn:%llx , ppn:%llx",entry->v_page_no , entry->p_page_no);
				//update extended page table
				paging->map_page_table((vpn<<zinfo->page_shift),(void*)dram_block,true);
				return dram_addr;
			}
			return INVALID_PAGE_ADDR;
		}
public:
		PagingStyle mode;
		g_string pg_walker_name;
	    BasePaging* paging;
		uint64_t period; 
	private:
		uint32_t procIdx;
		Address total_evict;
		Address dirty_evict;
		lock_t walker_lock;
		std::list<NVM::NVMainRequest*> failed_cache_request;
};
#endif

