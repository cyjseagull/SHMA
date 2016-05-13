/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef MEMORY_MANAGEMENT_H_
#define MEMORY_MANAGEMENT_H_
#include <fstream>
#include <vector>
#include <list>
#include <iterator>
#include "locks.h"
#include "memory_hierarchy.h"
#include "MMU/zone.h"
#include "page-table/page_table_entry.h"
#include "common/common_functions.h"
#include "MMU/common_memory_ops.h"

/*-----manage memory by buddy allocating algorithm----*/
class BuddyAllocator
{
	public:
		BuddyAllocator( MemoryNode* node);
		~BuddyAllocator();
		/***allocate pages according to gfp_mask***/
		Page* allocate_pages( Zone* zone , unsigned order);
		Page* allocate_pages( unsigned int gfp_mask , unsigned order = 0 );
		Address get_free_pages( unsigned int gfp_mask , unsigned order=0);
		Address get_dma_pages( unsigned int gfp_mask , unsigned order=0);

		unsigned allocate_bulk( Zone* zone , unsigned int order , uint64_t count , std::list<Page*>& list);
		/***free pages from specified zone***/
		void free_one_page( Zone* zone , uint64_t page_no , unsigned order);
		
		void free_pcppages_bulk(Zone* zone, unsigned int count , PerCpuPages* pcp);
		/***per cpu pageset allocate***/
		Page* buffered_rmqueue( unsigned int gfp_mask , Zone* zone , unsigned order , unsigned cpu_id);
		/***--per cpu pageset free--***/
		
		/***--memory system status related--***/
		uint64_t get_total_memory_size()
		{	return total_memsize;		}
		
		inline uint64_t get_free_memory_size();
		static void InitMemoryNode( MemoryNode* node);
	private:
		uint64_t find_buddy_index(uint64_t page_no , unsigned order );
		bool page_is_buddy( uint64_t page_id , uint64_t buddy_id , unsigned order);

		Page* rmqueue_page_smallest(MemoryNode* mem_node , Zone* zone , unsigned order);
		void expand(MemoryNode* mem_node,Zone* zone , uint64_t page_id , unsigned low_order , unsigned high_order );

		void free_hot_cold_page(Page* page , unsigned cpu_id , bool cold);
		inline Zone* gfp_zone( unsigned int flags);
	private:
		uint64_t total_memsize;
		PagingStyle mode;
		uint64_t free_page_num;
		MemoryNode* mem_node;
		lock_t buddy_lock;
};
#endif
