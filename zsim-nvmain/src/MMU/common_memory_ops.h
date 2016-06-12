/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */

#ifndef COMMON_MEMORY_OPS_H
#define COMMON_MEMORY_OPS_H

#include "common/global_const.h"
#include "memory_hierarchy.h"
#include "zone.h"
#include "MMU/page.h"
/****------functions related to buddy system--------*****/
inline Page* get_page_ptr( MemoryNode* mem_node , uint64_t page_id)
{
	return mem_node->get_page_ptr(page_id);
}

/*
 *@function: test whether a page pointed by page_id
 *			 belongs to buddy system
 */
inline bool page_is_buddy(MemoryNode* mem_node ,
								 uint64_t page_id)
{
	return (mem_node->get_page_ptr(page_id)->map_count
			== PAGE_BUDDY_MAPCOUNT_VALUE);
}

inline void set_page_buddy( MemoryNode* mem_node , uint64_t page_id)
{
	mem_node->get_page_ptr(page_id)->map_count = PAGE_BUDDY_MAPCOUNT_VALUE;
}

inline void clear_page_buddy( MemoryNode* mem_node , uint64_t page_id)
{
	assert( page_is_buddy( mem_node , page_id));
	mem_node->get_page_ptr(page_id)->map_count = -1;
}

inline unsigned get_page_private( MemoryNode* mem_node , uint64_t page_id)
{
	return get_page_ptr( mem_node ,page_id)->private_;
}

inline void set_page_private( MemoryNode* mem_node , uint64_t page_id , unsigned order)
{
	mem_node->get_page_ptr(page_id)->private_ = order;
}


inline void set_page_order( MemoryNode* mem_node , uint64_t page_id , unsigned order)
{
	set_page_private(mem_node , page_id, order);
	set_page_buddy(mem_node , page_id );
}

#endif
