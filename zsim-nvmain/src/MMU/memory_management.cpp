/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#include "MMU/memory_management.h"
#include "tlb/page_table_walker.h"
#include "zsim.h"
//lock_t BuddyAllocator::buddy_lock;
//MemoryNode* BuddyAllocator::mem_node;
BuddyAllocator::BuddyAllocator(MemoryNode* node)
{
	mode = zinfo->paging_mode;
	total_memsize = zinfo->memory_size;
	assert(total_memsize>0);
	free_page_num = total_memsize>>(zinfo->page_shift);
	mem_node = node;
	futex_init(&buddy_lock);
}


void BuddyAllocator::InitMemoryNode( MemoryNode* node)
{
}

BuddyAllocator::~BuddyAllocator()
{
}

/***-----allocate pages----------***/
Page* BuddyAllocator::allocate_pages( unsigned int gfp_mask , unsigned order )
{
	if( order>=MAXORDER )
	{
		return NULL;
	}
	futex_lock(&buddy_lock);
	Zone* zone = gfp_zone( gfp_mask );
	if( zone )
	{
		Page* page = allocate_pages(zone , order);
		if(page==NULL)
			std::cout<<"allocate failed inner"<<std::endl;
		//else
			//std::cout<<"page no:"<<std::hex<<page->pageNo<<std::endl;
		futex_unlock(&buddy_lock);
		return page;
	}
	else
	{
		futex_unlock(&buddy_lock);
		return NULL;
		panic("system has no zone specified by gfp_mask");
	}
}


Address BuddyAllocator::get_free_pages(unsigned int gfp_mask , unsigned order)
{
	return (allocate_pages(gfp_mask,order)->pageNo)>>(zinfo->page_shift);
}


Address BuddyAllocator::get_dma_pages(unsigned gfp_mask , unsigned order)
{
	return get_free_pages( gfp_mask | GFP_DMA,order);
}


/***---------free pages----------***/
uint64_t BuddyAllocator::find_buddy_index( uint64_t page_no , unsigned order)
{
	return page_no^(1<<order);
}

/*
 * @function:
 * page can coalesce with its buddy in below situations:
 *(1)the buddy is in the buddy system
 *(2)a page and its buddy have the same order
 *(3)a page and its buddy are in the same zone
 *@param page_id: page number 
 *@param buddy_id: founded buddy page number of page_id
 *@order: order of page whose page number is page_id
 */
bool BuddyAllocator::page_is_buddy(uint64_t page_id , uint64_t buddy_id,
								   unsigned order)
{
	bool is_buddy = false;
	assert( mem_node );
	if( get_page_ptr(mem_node , buddy_id)->private_== order )
	{
		if( mem_node->get_page_ptr(page_id)->node == 
			mem_node->get_page_ptr(buddy_id)->node && 
			mem_node->get_page_ptr(page_id)->zone ==
			mem_node->get_page_ptr(buddy_id)->zone )
			is_buddy = true;
	}
	return is_buddy;
}


void BuddyAllocator::free_one_page( Zone* zone ,
							uint64_t page_no , unsigned order)
{
	uint64_t page_id = page_no &((1<<MAXORDER)-1);
	while( order < MAXORDER-1 )
	{
		uint64_t buddy_id = find_buddy_index( page_id , order);
		if( !page_is_buddy(page_no, buddy_id , order) )
			break;
		zone->free_area[order].nr_free--;
		clear_page_buddy( mem_node , buddy_id );
		set_page_private(mem_node , buddy_id ,0);
		//page number after combined
		uint64_t combined_id = buddy_id & page_no;
		page_id = combined_id;
		order++;
	}
	//set bigger order for bigger block 
	set_page_order( mem_node , page_id , order);
	zone->free_area[order].nr_free++;
}

/*
 *@function: free a number of pages from pcp lists
 *@count: number of pages to be freed from pcp list
 *@pcp: PerCpuPages struct,
 */
void BuddyAllocator::free_pcppages_bulk(Zone* zone , unsigned int count , PerCpuPages* pcp)
{
	Page* page;
	for( unsigned i=0 ; (i<count)&&(pcp->page_list.size()>0);i++)
	{
		page = pcp->page_list.front();
		pcp->page_list.pop_front();	//delete page
		pcp->count++;
		free_one_page(zone,(page->pageNo),0);  
	}
}

unsigned BuddyAllocator::allocate_bulk(Zone* zone , unsigned int order , uint64_t count , std::list<Page*>& list )
{
	Page* page;
	uint64_t i=0;
	for( ; i<count; i++)
	{
		page = allocate_pages( zone , order);
		//page allocate failed
		if( unlikely(page==NULL) )
			break;
		list.push_back(page);
	}
	return i;	//return allocated page
}

/*
 *@function:
 *@param zone:
 *@param order:
 */
Page* BuddyAllocator::allocate_pages(Zone* zone , unsigned order)
{
	assert(mem_node);
	//get the page can be allocated
	Page* page = rmqueue_page_smallest( mem_node,zone,order);	
	return page;
}

/*
 *@function:
 *@param mem_node:
 *@param free_area:
 *@param zone:
 *@param order:
 */
Page* BuddyAllocator::rmqueue_page_smallest(MemoryNode* mem_node , Zone* zone , unsigned order)
{
	Page* page=NULL;
	for( unsigned current_order=order ; current_order<=MAXORDER ; current_order++ )
	{
		FreeArea* area = &(zone->free_area[current_order]);
		//no free block whose order is current_order
		//continue find page block
		if(area->block_list->get_size()==0 )
			continue;
		//get page descriptor
		page = area->block_list->fetch_head();
		uint64_t page_id = page->pageNo;
		page->map_count = -1;
		page->private_=0;
		page->count = 1;	//set reference count to 1
		area->nr_free--;
		//update free page of zone
		zone->free_pages -= 1UL>>order;
		free_page_num -= 1UL<<order;
		//std::cout<<"current_order:"<<current_order<<std::endl;
		//expand
		expand( mem_node ,zone , page_id ,order , current_order);
		break;
	}
	return page;
}

/*
 *@function: when there is no block which size is 2^low_order ; 
 *we have to allocate from larger block whose size is 2^high_order;
 *when allocated continous 2^low_order pages to program,
 *we should adjust free_area,expand 2^high_order-2^low_order pages to appropriate free area list,
 *and this is what the function are doing
 *@param area:
 *@param low_order:
 *@param high_order:
 */
void BuddyAllocator::expand(MemoryNode* mem_node,
		Zone* zone , uint64_t page_id,
		unsigned low_order , unsigned high_order)
{
	FreeArea* area; 
	Page* page;
	//get low page number
	uint64_t size=1<<high_order;
	uint64_t new_page_id;
	//std::cout<<"high order:"<<high_order<<std::endl;
	//if(zone->free_area[high_order].block_list->get_size()==0)
	//	return;
	while( low_order < high_order)
	{
		high_order--;
		area = &(zone->free_area[high_order]);
	    size >>= 1;	
		new_page_id = page_id+size;
		//std::cout<<"new_page_id:"<<new_page_id<<std::endl;
		page = mem_node->get_page_ptr(new_page_id);
		//debug_printf("realloc page %lld,order: %d",new_page_id,high_order);
		//add page to area to list head
		area->block_list->push_block_back(page);
		area->nr_free++;
		set_page_order(mem_node,new_page_id,high_order);
	}
}

Zone* BuddyAllocator::gfp_zone( unsigned int flag)
{
	//futex_lock(&buddy_lock);
	//allocate pages from Zone DMA
	if( mem_node )
	{
		if( mem_node->node_zones[Zone_DMA] && (flag&GFP_DMA) )
		{
			return mem_node->node_zones[Zone_DMA];
		}
		//allocate pages from DMA32 Zone
		if( mem_node->node_zones[Zone_DMA32]&& (flag&GFP_DMA32))
		{	
			return mem_node->node_zones[Zone_DMA32];
		}
		if( mem_node->node_zones[Zone_HighMem]&& (flag&GFP_HighMem))
		{
			return mem_node->node_zones[Zone_HighMem];
		}
		//default allocate pages from Normal Zone
		assert( mem_node->node_zones[Zone_Normal]);
		//debug_printf("allocate pages from normal zone");
		return mem_node->node_zones[Zone_Normal];
	}
	return NULL;
}

/****------per cpu pageset allocation--------****/
Page* BuddyAllocator::buffered_rmqueue( unsigned int gfp_mask , Zone* zone , unsigned order , unsigned cpu_id )
{
	PerCpuPages* pps;
	Page* page = NULL;
	bool cold = ((gfp_mask & GFP_COLD)!=0);
	//allocate pages from hot per_cpu_pages 
	if( likely(order==0) )
	{
		//allocate from cold page list
		if( !cold )
			pps = zone->get_cpu_hot_pages(cpu_id);
		else
			pps = zone->get_cpu_cold_pages(cpu_id);
		//has no pages in hot page list 
		//allocate pages through buddy allocator
		if( pps->page_list.empty())
		{
			pps->count += allocate_bulk(zone, order , pps->batch , pps->page_list);	
		}
		//need complement page
		else if( pps->count <= pps->low )
		{
			uint64_t page_need = pps->batch - pps->count;
			pps->count += allocate_bulk(zone , order,page_need,pps->page_list );
		}
		//allocate succeed
		if( !(pps->page_list.empty()) )
		{
			page = *(pps->page_list.begin());				 pps->count--;
		}
	}
    if( !page || pps->page_list.empty() || unlikely(order!=0))
	{
		page = allocate_pages( zone , order); 
	}
	return page;
}

/****------per cpu pageset free--------****/
/*
 *@function: free page to cpu cache list;
 *			 if cpu cache list has too much pages , free batch pages to buddy system
 *@page: page need to be freed to cpu cache list
 *@cpu_id: tag cpu this page should be freed to
 *@cold: should be freed to cold page list or not? 
 */
void BuddyAllocator::free_hot_cold_page(Page* page , unsigned cpu_id , bool cold)
{
	//get zone
	Zone* zone = page->zone;
	PerCpuPages* cpu_pages;

	if( cold )		//free cold page
		cpu_pages = zone->get_cpu_cold_pages(cpu_id);
	else if( !cold )	//free hot page
		cpu_pages = zone->get_cpu_hot_pages(cpu_id);
	//add page to cpu page list
	cpu_pages->page_list.push_back(page);
	cpu_pages->count++;
	//check whether need to clear cache
	if(cpu_pages->count > cpu_pages->high )
	{
		free_pcppages_bulk( zone ,cpu_pages->batch , cpu_pages);
		cpu_pages->count -= cpu_pages->batch;
	}
}

uint64_t BuddyAllocator::get_free_memory_size()
{
	return free_page_num>>(zinfo->page_shift);
}
