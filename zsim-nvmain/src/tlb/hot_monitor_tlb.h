/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef HOT_MONITOR_TLB_H
#define HOT_MONITOR_TLB_H

#include "g_std/g_string.h"
#include "common/trie.h"
#include "tlb/common_tlb.h"
#include "tlb/common_func.h"
#include "DRAM-buffer/DRAM_buffer_block.h"
//#include "tlb/page_table_walker.h"

template<class T>
class HotMonitorTlb: public BaseTlb 
{
	public:
		HotMonitorTlb(const g_string name, unsigned tlb_size, unsigned hit_lat, unsigned res_lat,EVICTSTYLE policy , unsigned write_incre=2 ,
				       unsigned read_incre=1): write_incre_step(write_incre),
					   read_incre_step(read_incre)
		{
		    CommonTlb<T>* tmp = gm_memalign<CommonTlb<T> >( CACHE_LINE_BYTES,1);
			ordinary_tlb = new (tmp) CommonTlb<T>( name , tlb_size , hit_lat , res_lat , policy);
			dram_tlb_hit = 0;
			dram_tlb_miss = 0;
			pcm_tlb_hit = 0;
			pcm_tlb_miss =0;
		}
		~HotMonitorTlb()
		{
		}
		/*******TLB access related functions************/		
		uint64_t access(MemReq& req)
		{
			ordinary_tlb->tlb_access_time++;
			Address virt_addr = req.lineAddr;
			Address offset = virt_addr &(zinfo->page_size-1);
			Address vpn = virt_addr >>(zinfo->page_shift);
			Address ppn;
			T* entry = ordinary_tlb->look_up(vpn);
			uint32_t access_counter = 0;
			uint32_t origin_child_id = req.childId;
			//tlb miss
			if( !entry )
			{
				ppn = ordinary_tlb->page_table_walker->access(req);
				if( zinfo->multi_queue)
				{
					access_counter = req.childId; 
				}
				debug_printf("hot monitor insert (%llx,%llx)",vpn,ppn);
				insert(vpn,ppn, access_counter);
			}
			//tlb hit
			else
			{
				req.cycle += ordinary_tlb->hit_latency;
				ordinary_tlb->tlb_hit_time++;
				ppn = entry->p_page_no; //get physical address(DRAM or PCM)
				//write through to page table to set dirty bits
				if( req.type == PUTX && entry->is_dirty()==false && entry->is_in_dram())
				{	
					entry->set_dirty();
					MemReq wt_req;
					wt_req.lineAddr = entry->v_page_no<<(zinfo->page_shift);
					wt_req.cycle = req.cycle;
					wt_req.type = SETDIRTY;
					ordinary_tlb->page_table_walker->write_through(wt_req);
					Address block_id = dram_addr_to_block_id( (entry->p_page_no)<<(zinfo->page_shift));
					ordinary_tlb->page_table_walker->convert_to_dirty( block_id );
				}
				if( entry->is_in_dram() )
				{
					dram_tlb_hit++;
				}
				else
					pcm_tlb_hit++;
				debug_printf("hot monitor access %llx,%llx tlb hit",vpn,ppn);
			}
			req.childId = origin_child_id;
			req.cycle += ordinary_tlb->response_latency;
			return  ((ppn<<zinfo->page_shift)|offset);
		}

		const char* getName()
		{
			return ordinary_tlb->getName();
		}
		void clear_counter()
		{
			ordinary_tlb->clear_counter();
		}
		void set_parent( BasePageTableWalker* pg_table_walker)
		{
			ordinary_tlb->set_parent(pg_table_walker);
		}

		T* look_up( Address vpage_no , bool update_lru = true)
		{
		    return ordinary_tlb->look_up(vpage_no , update_lru);
		}

		bool flush_all()
		{
			return ordinary_tlb->flush_all();
		}
		
		void update_life_time(Address vpn)
		{
			T* entry = NULL;
			for(unsigned i=0; i< ordinary_tlb->tlb_entry_num; i++)
			{
			  entry = ordinary_tlb->tlb[i];
			  if(!entry->is_in_dram() && entry->is_valid() && entry->v_page_no!=vpn)
				   ordinary_tlb->tlb[i]->decre_lifetime();
			}
		}

		bool delete_entry(Address vpage_no)
		{
			return ordinary_tlb->delete_entry(vpage_no);
		}

		void flush_all_noglobal()
		{
			return ordinary_tlb->flush_all_nogloabl();
		}

		uint64_t calculate_stats()
		{
			double dram_tlb_hit_part = (double)dram_tlb_hit/(double)(ordinary_tlb->tlb_access_time); 
			double pcm_tlb_hit_part = (double)pcm_tlb_hit/(double)(ordinary_tlb->tlb_access_time);
			double dram_tlb_hit_rate = (double)dram_tlb_hit/(double)(dram_tlb_hit + dram_tlb_miss);
			double pcm_tlb_hit_rate = (double)pcm_tlb_hit/(double)(pcm_tlb_hit+pcm_tlb_miss);
			info("dram_tlb_hit:%lu, dram_tlb_miss:%lu , pcm_tlb_hit:%lu , pcm_tlb_miss:%lu",dram_tlb_hit, dram_tlb_miss, pcm_tlb_hit,pcm_tlb_miss);	
			info("dram tlb hit part(+pcm tlb hit part=total hit part):%.5f",dram_tlb_hit_part);
			info("pcm tlb hit part(+dram tlb hit part=total hit part):%.5f",pcm_tlb_hit_part);
			info("dram tlb hit rate(dram TLB hit to all dram TLB access):%.5f",dram_tlb_hit_rate);
			info("pcm tlb hit rate(PCM TLB hit to all PCM TLB access):%.5f",pcm_tlb_hit_rate);
			return ordinary_tlb->calculate_stats();
		}
		
		T* look_up_va( Address p_page_no , bool is_write ,uint32_t proc_id, TLBSearchResult &over_thres)
		{
			//typename CommonTlbEntryTrie::TrieHandle result_node = (ordinary_tlb->tlb_trie_pa).search(p_page_no);
			T* result_node = NULL;
			if( ordinary_tlb->tlb_trie_pa.count(p_page_no))
				result_node =(ordinary_tlb->tlb_trie_pa)[p_page_no];
			over_thres = unOverThres;
			if( result_node )
			{
				unsigned thres = 0;	
				if( zinfo->proc_fairness)
					thres = zinfo->access_threshold[proc_id];
				else
					thres = zinfo->access_threshold[0];
				//std::cout<<"threshold is:"<<thres<<std::endl;
				over_thres = result_node->incre_counter(is_write ,write_incre_step ,
														read_incre_step,thres);
				//decrease life time of all other tlb entries 
				if( zinfo->multi_queue)
				{
					update_life_time( result_node->v_page_no );								
				}
				return result_node;
			}
			return NULL;
		}

		T* insert( Address vpage_no ,Address ppn , uint32_t access_counter)
		{
			//DRAM buffer TLB 
			if( (ppn << zinfo->page_shift) >= zinfo->high_addr )
			{
				T entry(vpage_no, ppn , true);
				entry.remap( ppn,true);
				if( access_counter == 1)
				{
					//std::cout<<"set dirty in tlb"<<std::endl;
					entry.set_dirty();
				}
				dram_tlb_miss++;
				return ordinary_tlb->insert( vpage_no , entry);
			}
			//PCM main memory Tlb
			else
			{
				T entry(vpage_no,ppn , false);
				if( zinfo->multi_queue)
					entry.set_counter( access_counter);
				pcm_tlb_miss++;
				return ordinary_tlb->insert( vpage_no , entry);
			}
			return NULL;
		}

		bool remap_ppn(Address page_no , T* entry, bool is_dram=true )
		{
			debug_printf("remap (%llx , %llx , %llx)",page_no , entry->v_page_no , entry->p_page_no);
			if(entry)
			{
				ordinary_tlb->tlb_trie_pa.erase(entry->p_page_no);
				ordinary_tlb->tlb_trie_pa[page_no]=entry;
				entry->remap(page_no , is_dram);
				return true;
			}
		  return false;
		}
		
		/*****modified  12.21*/
		//dram buffer policy: Multi-Queue
		virtual T* evict( )
		{
			return ordinary_tlb->evict( );
		}

		void moveToDRAMList( T* entry)
		{
			ordinary_tlb->moveToDRAMList(entry);
		}

	private:
		unsigned write_incre_step;
		unsigned read_incre_step;
		uint64_t pcm_tlb_hit;
		uint64_t pcm_tlb_miss;
		uint64_t dram_tlb_hit;
		uint64_t dram_tlb_miss;
		CommonTlb<T>* ordinary_tlb;
};
#endif
