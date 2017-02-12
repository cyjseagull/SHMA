/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef __REVERSED_PGT__
#define __REVERSED_PGT__
#include "g_std/g_unordered_map.h"
#include "g_std/g_list.h"
#include "page-table/page_table_entry.h"
#include "page-table/page_table.h"
#include "common/global_const.h"
#include "memory_hierarchy.h"
#include "locks.h"
#include "zsim.h"
#include "MMU/page.h"
class ReversedPaging: public BasePaging
{
	public:
		ReversedPaging(std::string mode_str , PagingStyle mode):BasePaging()
		{
			paging = gm_memalign<BasePaging>(CACHE_LINE_BYTES, 1);
			LongModePaging* longmode_paging;
			/*union
			{
				NormalPaging* normal_paging;
				PAEPaging* pae_paging;
				LongModePaging* longmode_paging;
			};
			std::cout<<"mode_str"<<std::endl;
			std::cout<<"mode_str:"<<mode_str<<std::endl;
			if( mode_str == "Legacy")
			{
				normal_paging = gm_memalign<NormalPaging>(CACHE_LINE_BYTES, 1);	
				paging = new (&normal_paging) NormalPaging(mode);
			}
			else if( mode_str == "PAE")
			{
				pae_paging = gm_memalign<PAEPaging>(CACHE_LINE_BYTES, 1);	
				paging = new (&pae_paging) PAEPaging(mode);
			}
			else if( mode_str == "LongMode")
			{*/
				longmode_paging = gm_memalign<LongModePaging>(CACHE_LINE_BYTES, 1);	
				paging = new (longmode_paging) LongModePaging(mode);
			//}
			assert(paging);
			std::cout<<"create reversed paging"<<std::endl;
		}
		virtual uint64_t access(MemReq& req)
		{
			//std::cout<<"reversed paging access"<<std::endl;
			return paging->access(req);
		}
		/****important: find vpn according to ppn****/
		Address get_vpn( Address ppn )
		{
			assert( (reversed_pgt).count(ppn));
			Address vpn = (reversed_pgt)[ppn]->get_vpn();
			return vpn;
		}
		/****important: find vpn according to ppn end****/
		virtual PagingStyle get_paging_style()
		{
			return paging->get_paging_style();
		}
		
		virtual PageTable* get_root_directory()
		{
			return paging->get_root_directory();
		}
		
		virtual bool unmap_page_table( Address addr)
		{
			return paging->unmap_page_table(addr);
		}

		virtual int map_page_table( Address addr, void* pg_ptr, bool pbuffer=false);
		
		virtual uint64_t remap_page_table( Address ppn, Address dst_ppn, bool src_dram, bool dst_dram);

		virtual bool allocate_page_table(Address addr, Address size)
		{
			return paging->allocate_page_table(addr, size);
		}

		virtual void remove_root_directory()
		{
			paging->remove_root_directory();
		}

		virtual bool remove_page_table( Address addr, Address size)
		{
			return remove_page_table( addr, size);
		}
		
		virtual void calculate_stats();

	private:
		BasePaging* paging;
		/*****for inverted page table*****/
		//g_unordered_map<uint32_t, Content* > reversed_pgt;
		lock_t reversed_pgt_lock;
		g_unordered_map<uint32_t, Content* > reversed_pgt;
};
#endif
