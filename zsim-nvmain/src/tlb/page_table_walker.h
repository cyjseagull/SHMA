/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef PAGE_TABLE_WALKER_H
#define PAGE_TABLE_WALKER_H

#include <map>
#include <fstream>
//#include "glog/logging.h"

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

template <class T>
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
			allocated_page = 0;
			mmap_cached = 0;
			tlb_shootdown_overhead = 0;
			hscc_tlb_shootdown = 0;
			pcm_map_overhead = 0;
			hscc_map_overhead = 0;
			tlb_miss_exclude_shootdown = 0;
			tlb_miss_overhead = 0;
			clflush_overhead = 0;
			extra_write = 0;
			futex_init(&walker_lock);
		}
		~PageTableWalker(){}
		/*------tlb hierarchy related-------*/
		//bool add_child(const char* child_name ,  BaseTlb* tlb);
		/*------simulation timing and state related----*/
		uint64_t access( MemReq& req)
		{
			assert(paging);
			period++;
			Address addr = PAGE_FAULT_SIG;
			Address init_cycle = req.cycle;
			addr = paging->access(req);

			tlb_miss_exclude_shootdown += (req.cycle - init_cycle);
			//page fault
			if( addr == PAGE_FAULT_SIG )	
			{
				addr = do_page_fault(req , PCM_PAGE_FAULT);
				//std::cout<<"allocate page:"<<addr<<std::endl;
				req.childId = 0;
			}
			tlb_miss_overhead += (req.cycle - init_cycle);
			//suppose page table walking time when tlb miss is 20 cycles
			return addr;	//find address
		}

		void write_through( MemReq& req)
		{
			assert(paging);
			paging->access(req);
		}

		BasePaging* GetPaging()
		{ return paging;}
		void SetPaging( uint32_t proc_id , BasePaging* copied_paging)
		{
			futex_lock(&walker_lock);
			procIdx = proc_id;
			paging = copied_paging;
			futex_unlock(&walker_lock);
		}

		void convert_to_dirty( Address block_id)
		{
			zinfo->dram_manager->convert_to_dirty( procIdx , block_id );
		}

		const char* getName()
		{  return pg_walker_name.c_str(); }

		void calculate_stats()
		{
			info("%s evict time:%lu \t dirty evict time: %lu \n",getName(),total_evict,dirty_evict);
			info("%s allocated pages:%lu \n", getName(),allocated_page);
			info("%s TLB shootdown overhead:%llu \n", getName(), tlb_shootdown_overhead);
			info("%s HSCC TLB shootdown overhead:%llu \n", getName(), hscc_tlb_shootdown);
			info("%s PCM page mapping overhead:%llu \n", getName(), pcm_map_overhead);
			info("%s DRAM page mapping overhead:%llu \n", getName(), hscc_map_overhead);
			info("%s TLB miss overhead(exclude TLB shootdown and page fault): %llu", getName(),tlb_miss_exclude_shootdown);
			info("%s TLB miss overhead (include TLB shootdown and page fault): %llu",getName(), tlb_miss_overhead);
			info("%s clflush overhead caused by caching:%llu",getName(), clflush_overhead);
			info("%s clflush overhead caused extra write:%llu",getName(), extra_write);
		}
		
		Address do_page_fault(MemReq& req, PAGE_FAULT fault_type)
		{
		    //allocate one page from Zone_Normal area
		    debug_printf("page falut, allocate free page through buddy allocator");
		    Page* page = NULL;
			if( zinfo->buddy_allocator)
			{
				page = zinfo->buddy_allocator->allocate_pages(0);
				if(page)
				{
					//TLB shootdown
					Address vpn = req.lineAddr>>(zinfo->page_shift);
					tlb_shootdown(req, vpn, tlb_shootdown_overhead );
					if( zinfo->enable_shared_memory)
					{
						if( !map_shared_region(req, page) )
						{
							//std::cout<<"map "<< pg_walker_name<<":"<<req.lineAddr<<"->"<<page->pageNo<<std::endl;
							Address overhead = paging->map_page_table( req.lineAddr,(void*)page);
							pcm_map_overhead += overhead;
							req.cycle += overhead;
						}
					}
					else
					{
						Address overhead = paging->map_page_table( req.lineAddr,(void*)page);
						pcm_map_overhead += overhead;
						req.cycle += overhead;
					}
					allocated_page++;
					return page->pageNo;
				}
			}
			//update page table
			return (req.lineAddr>>zinfo->page_shift);
		}
		
		Address do_dram_page_fault(MemReq& req, Address vpn ,uint32_t coreId, PAGE_FAULT fault_type , T* entry , bool is_itlb , bool &evict)
		{
			debug_printf("fetch pcm page %d to DRAM",(req.lineAddr>>zinfo->page_shift));
			
			//allocate dram buffer block
			DRAMBufferBlock* dram_block = allocate_page();
			if( dram_block)
			{
				//*clflush nvm pages from on-chip caches
				bool req_write_back = true;
				uint64_t write_backs = 0;
				uint64_t cycle = clflush(req.lineAddr, &req_write_back, req.cycle, coreId, write_backs);
				if( write_backs)
					extra_write += write_backs;
				clflush_overhead += (cycle - req.cycle);
				req.cycle = cycle;
				//*1.clflush DRAM pages from on-chip caches
				//*2.evict dirty DRAM page
				//*3.tlb shootdown
				//*4.map page table
				evict_DRAM_page( req, dram_block,entry, coreId, evict);
				
				 return block_id_to_addr( dram_block->block_id);
			}
			return INVALID_PAGE_ADDR;
		}

	private:
		bool inline map_shared_region( MemReq& req , void* page)
		{
			Address vaddr = req.lineAddr;
			//std::cout<<"find out shared region"<<std::endl;
			if( !zinfo->shared_region[procIdx].empty())
			{
				int vm_size = zinfo->shared_region[procIdx].size();
				//std::cout<<"mmap_cached:"<<std::dec<<mmap_cached
				//	<<" vm size:"<<std::dec<<vm_size<<std::endl;
				Address vm_start = zinfo->shared_region[procIdx][mmap_cached].start;
				Address vm_end = zinfo->shared_region[procIdx][mmap_cached].end;
				Address vpn = vaddr>>(zinfo->page_shift);
				//belong to the shared memory region
				//examine whether in mmap_cached region (examine mmap_cached firstly)
				if( find_shared_vm(vaddr,
						zinfo->shared_region[procIdx][mmap_cached]) )
				{
					Address overhead = map_all_shared_memory( vaddr, (void*)page);
					req.cycle += overhead;
					pcm_map_overhead += overhead;
					return true;
				}
				//after mmap_cached
				else if( vpn > vm_end && mmap_cached < vm_size-1 )
				{
					mmap_cached++;
					for( ;mmap_cached < vm_size; mmap_cached++)
					{
						if( find_shared_vm(vaddr, 
							zinfo->shared_region[procIdx][mmap_cached]))
						{
							Address overhead = map_all_shared_memory(vaddr, (void*)page);
							req.cycle += overhead;
							pcm_map_overhead += overhead;
							return true;
						}
					}
					mmap_cached--;
				}
				//before mmap_cached
				else if( vpn < vm_start && mmap_cached > 0 )
				{
					mmap_cached--;
					for( ;mmap_cached >= 0; mmap_cached--)
					{
						if( find_shared_vm(vaddr, 
							zinfo->shared_region[procIdx][mmap_cached]))
						{
							Address overhead = map_all_shared_memory(vaddr, (void*)page);
							req.cycle += overhead;
							pcm_map_overhead += overhead;
							return true;
						}
					}
					mmap_cached++;
				}
			}
			return false;
		}

		bool inline find_shared_vm(Address vaddr, Section vm_sec)
		{
			Address vpn = vaddr >>(zinfo->page_shift);
			if( vpn >= vm_sec.start && vpn < vm_sec.end )
				return true;
			return false;
		}

		int inline map_all_shared_memory( Address va, void* page)
		{
			int latency = 0;
			//std::cout<<"map all shared memory:"<<va<<std::endl;
			for( uint32_t i=0; i<zinfo->numProcs; i++)
			{
				assert(zinfo->paging_array[i]);
				//std::cout<<"map "<<i<<std::endl;
				//std::cout<<"map "<<i<<" "<<va<<"->"<<(((Page*)page)->pageNo)<<std::endl;
				latency += zinfo->paging_array[i]->map_page_table(va,page);
			}
			return latency;
		}

		DRAMBufferBlock* allocate_page( )
		{
			if( zinfo->dram_manager->should_reclaim() )
			{
				zinfo->dram_manager->evict( zinfo->dram_evict_policy);
			}
			//std::cout<<"allocate page table"<<std::endl;
			return (zinfo->dram_manager)->allocate_one_page( procIdx);
		}
		
		//#####clflush simulation
		inline uint64_t clflush( Address dram_addr, bool* req_write_back, uint64_t issue_cycle, int core_id, uint64_t &is_dirty)
		{
			Address inv_addr_init = dram_addr>>(zinfo->page_shift)<<lineBits;
			uint64_t latency = zinfo->cores[core_id]->clflush( inv_addr_init, issue_cycle, is_dirty);
			return latency;
		}

		inline void store_page( MemReq & req, Address src_addr, Address dst_addr,
				 bool src_is_dram, NVM::OpType type )
		{
			if( NVMainMemory::fetcher)
			{
				NVM::NVMainRequest* nvmain_req = new NVM::NVMainRequest();
				//is DRAM address
				nvmain_req->address.SetPhysicalAddress(src_addr, src_is_dram);
				//nvm address
				nvmain_req->address.SetDestAddress(dst_addr, !src_is_dram);
				nvmain_req->burstCount = zinfo->lineNum;
				nvmain_req->type = type; 
				(NVMainMemory::fetcher)->IssueCommand( nvmain_req );
			 }
			else 
			{
				req.cycle +=200;
			}
		}

		inline void evict_and_fetch( MemReq& req, DRAMBufferBlock* dram_block,T* entry, bool& evict)
		{
			Address origin_ppn = dram_block->get_src_addr();
			Address dram_addr = block_id_to_addr( dram_block->block_id);
			//evict
			if( dram_block->is_occupied() && dram_block->is_dirty())
			{
				evict = true;
				dirty_evict++;
				Address dst_addr = origin_ppn<<(zinfo->page_shift);
				//write back dirty DRAM pages to NVM
				store_page( req, dram_addr, dst_addr, true, NVM::FETCH);
			}
			if( dram_block->is_occupied())
				total_evict++;
			//call memory controller interface to cache page
			store_page( req, req.lineAddr, dram_addr, false, NVM::READ);
			Address overhead = 0;			
			//invalidate dram
			if( dram_block->is_occupied())
			{
				//std::cout<<"evict:"<<dram_addr<<std::endl;
				if( zinfo->enable_shared_memory)
				{
					overhead = remap_page_table( dram_addr>>(zinfo->page_shift),origin_ppn, true, false);
				}
				else
				{
					Address vpn = dram_block->get_vaddr();
					Page* page_ptr = zinfo->memory_node->get_page_ptr(origin_ppn); 
					overhead = paging->map_page_table((vpn<<zinfo->page_shift),(void*)page_ptr,false);
				}
				req.cycle += overhead;
				hscc_map_overhead += overhead;
				dram_block->invalidate();
			}
			dram_block->validate(req.lineAddr>>(zinfo->block_shift));
			dram_block->set_src_addr( entry->p_page_no );
			dram_block->set_vaddr( entry->v_page_no);
			debug_printf("after remap , vpn:%llx , ppn:%llx",entry->v_page_no , entry->p_page_no);
			//update extended page table
			if( zinfo->enable_shared_memory )
			{
				//std::cout<<"remap page table "<<entry->p_page_no<<"->"<<(dram_addr>>zinfo->page_shift)<<std::endl;
				overhead = remap_page_table( entry->p_page_no,dram_addr>>(zinfo->page_shift), false , true);
			}
			else
			{
				//std::cout<<"remap page table "<<entry->v_page_no<<": "<<entry->p_page_no<<"->"<<(dram_addr>>zinfo->page_shift)<<std::endl;
				overhead= paging->map_page_table(( (entry->v_page_no)<<zinfo->page_shift),(void*)dram_block,true);
			}
			
			req.cycle += overhead;
			hscc_map_overhead += overhead;	
		}
		
		inline void tlb_shootdown( MemReq& req, BaseTlb* tlb, 
						Address vpn, unsigned long long shootdown_counter)
		{
			T* entry = NULL;
			union
			{	
				CommonTlb<T>* com_tlb;
				HotMonitorTlb<T>* hot_tlb;
			};
			if( zinfo->tlb_type == COMMONTLB )
			{
				com_tlb = dynamic_cast<CommonTlb<T>* >(tlb); 
				entry = com_tlb->look_up(vpn);
			}
			else if( zinfo->tlb_type == HOTTLB)
			{
				hot_tlb = dynamic_cast<HotMonitorTlb<T>* >(tlb); 
				entry = hot_tlb->look_up(vpn);
			}
			//instruction TLB IPI
			if( entry )
			{
				entry->set_invalid();
				shootdown_counter += zinfo-> tlb_hit_lat; 
				req.cycle += zinfo->tlb_hit_lat;
			}
		}

		inline void tlb_shootdown( MemReq& req, Address vpn, unsigned long long shootdown_counter)
		{
			BaseTlb* tmp_tlb = NULL;
			for( uint64_t i = 0; i<zinfo->numCores; i++)
			{
				//instruction TLB shootdown
				tmp_tlb = zinfo->cores[i]->getInsTlb();
				tlb_shootdown(req, tmp_tlb, vpn, shootdown_counter);
				//data TLB shootdown
				tmp_tlb = zinfo->cores[i]->getDataTlb();
				tlb_shootdown(req, tmp_tlb, vpn, shootdown_counter);
			}
		}

		void tlb_shootdown_hscc(MemReq& req, Address vaddr ,Address ppn,
				unsigned long long& shootdown_counter, bool in_dram = false)
		{
			T* tlb_entry = NULL;
			HotMonitorTlb<T>* recover_tlb = NULL;
			for( uint32_t i=0; i<zinfo->numCores; i++)
			{
				recover_tlb = dynamic_cast<HotMonitorTlb<T>* >
					  (zinfo->cores[i]->getInsTlb());;
				tlb_entry = recover_tlb->look_up( vaddr );
				//instruction TLB shootdown
				//assume IPI is equal to TLB hit cycle
				if( tlb_entry)
				{
					recover_tlb->remap_ppn( ppn, tlb_entry, in_dram );
					shootdown_counter += zinfo->tlb_hit_lat;
					req.cycle += zinfo->tlb_hit_lat;	//IPI latency 
					tlb_entry = NULL;
				}
				//data TLB shootdown( for PCM pages)
				recover_tlb = dynamic_cast<HotMonitorTlb<T>* >
							  (zinfo->cores[i]->getDataTlb());;
				tlb_entry = recover_tlb->look_up( vaddr );
				if( tlb_entry )
				{
					recover_tlb->remap_ppn(ppn,tlb_entry,in_dram);
					shootdown_counter += zinfo->tlb_hit_lat;
					req.cycle += zinfo->tlb_hit_lat; //IPI latency
					tlb_entry = NULL;
				}
			}
		}


		void evict_DRAM_page(MemReq& req, DRAMBufferBlock* dram_block,T* entry, uint32_t core_id, bool &evict)
		{
			if( dram_block)
			{
				Address dram_addr = block_id_to_addr( dram_block->block_id);
				bool req_write_back = true;
				//evict dram_block if it's dirty
				if( dram_block->is_occupied())
				{
					//#cache clflush: DRAM cache block
					//std::cout<<std::hex<<dram_addr<<" is occupied, clflush cache first"<<std::endl;
					uint64_t wbs = 0;
					uint64_t cycle = clflush(dram_addr, &req_write_back, req.cycle, core_id, wbs);
					if( wbs )
					{
						extra_write += wbs;
						dram_block->set_dirty();
					}
					clflush_overhead += cycle - req.cycle;
					req.cycle = cycle;
					//TLB shootdown1: shootdown evicted pages
					Address vpn = dram_block->get_vaddr();
					tlb_shootdown_hscc(req,vpn,dram_block->get_src_addr(), hscc_tlb_shootdown, false);
				}
				//if victim DRAM block is dirty, evict to main memory
				evict_and_fetch(req, dram_block,entry ,evict);
				//std::cout<<"migrate from:"<<std::hex<<req.lineAddr<<" to:"<<std::hex<<dram_addr<<std::endl;
				//TLB shootdown2: related to installed pages
				Address vpage_installed = entry->v_page_no;
				tlb_shootdown_hscc(req, vpage_installed,(dram_addr>>zinfo->page_shift), hscc_tlb_shootdown, true); 
		 }
	 }

public:
		PagingStyle mode;
		g_string pg_walker_name;
	    BasePaging* paging;
		uint64_t period;
		unsigned long long tlb_shootdown_overhead;
		unsigned long long hscc_tlb_shootdown;
		unsigned long long pcm_map_overhead;
		unsigned long long hscc_map_overhead;

		unsigned long long tlb_miss_exclude_shootdown;
		unsigned long long tlb_miss_overhead;

		unsigned long long clflush_overhead;
		unsigned long long extra_write;
	private:
		uint32_t procIdx;
		int mmap_cached;
		Address allocated_page;
		Address total_evict;
		Address dirty_evict;
		lock_t walker_lock;
};
#endif

