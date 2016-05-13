/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.
 */
#include "DRAM-buffer/FairAllocator.h"
#include "zsim.h"
FairAllocator::FairAllocator( unsigned process_num , 
				Address memory_size): process_count(process_num),
									  memory_capacity( memory_size ), busy_pages(0)
{
	total_page_count = memory_size >> (zinfo->page_shift);
	//create descriptors for all pages
	for( Address i = 0; i < total_page_count; i++ )
	{
		 DRAMBufferBlock* block = new DRAMBufferBlock(i); 
		 buffer_array.push_back( block );	
		 global_clean_pool.push_back(i);
	}
	global_dirty_pool.clear();

	//for recording process taken space
	for( unsigned i=0 ; i<process_count; i++ )
	{
		g_unordered_set<Address> tmp;
		clean_pools.push_back(tmp);
		dirty_pools.push_back(tmp);
	}
	proc_busy_pages.resize(process_count);
	uint64_t max_pages=((uint64_t)2<<26);
	uint64_t min_pages = ((uint64_t)2<<17);
	//calculate min free pages
	min_free_pages = Min( Max((uint64_t)(4*sqrt(memory_size)),min_pages),max_pages);
	std::cout<<"min free pages:"<<std::hex<<min_free_pages<<std::endl;
	std::cout<<"min pages:"<<std::hex<<min_pages<<std::endl;
	std::cout<<"max pages:"<<std::hex<<max_pages<<std::endl;
	//min free page num
	min_free_pages /=(zinfo->page_size);
}

bool FairAllocator::should_reclaim()
{
	if( total_page_count - busy_pages == 1)
		return true;
	return false;
}

bool FairAllocator::should_cherish()
{
	if( total_page_count - busy_pages <= 0.3*total_page_count)
		return true;
	return false;
}

bool FairAllocator::should_more_cherish()
{
	Address free_pages = total_page_count - busy_pages;
	if( free_pages <=0.05*total_page_count || free_pages <= min_free_pages)
	{
		return true;
	}
	return false;
}

unsigned FairAllocator::Release( unsigned process_id, unsigned evict_size )
{
	//evict clean page first
	unsigned clean_pool_size = clean_pools[process_id].size();
	unsigned clean_evict_size = (evict_size > clean_pool_size)? clean_pool_size:evict_size;
	assert( clean_pool_size + dirty_pools[process_id].size()>= evict_size);
	for( unsigned i=0 ; i < clean_evict_size; i++)
	{
		Address block_id = *clean_pools[process_id].begin();
		clean_pools[process_id].erase( block_id );
		global_clean_pool.push_back(block_id);
		busy_pages--;
	}
	if( clean_evict_size < evict_size)
	{
		for( unsigned i = 0 ; i < (evict_size-clean_evict_size); i++ )
		{
			Address block_id = *dirty_pools[process_id].begin();
			dirty_pools[process_id].erase(block_id);
			global_dirty_pool.push_back(block_id);
			busy_pages--;
		}
	}
	proc_busy_pages[process_id] -= evict_size;
	assert(should_reclaim()==false);
	return evict_size;	
}

double FairAllocator::get_memory_usage() 
{
	return (double)busy_pages/(double)total_page_count;
}

DRAMBufferBlock* FairAllocator::allocate_one_page( unsigned process_id )
{
	assert(!global_clean_pool.empty()|| !global_dirty_pool.empty())
	Address alloc_block_id = INVALID_PAGE_ADDR;
	if( !global_clean_pool.empty())
	{
		alloc_block_id = global_clean_pool.front();
		global_clean_pool.pop_front();
	}
	else if( !global_dirty_pool.empty())
	{
		alloc_block_id = global_dirty_pool.front();
		global_dirty_pool.pop_front();
	}
	if( alloc_block_id != INVALID_PAGE_ADDR  )
	{
		clean_pools[process_id].insert( alloc_block_id);
		busy_pages++;
		proc_busy_pages[process_id]++;
		return buffer_array[alloc_block_id];
	}
	return NULL;
}

/*
 * @function:
 * @param:
 */
void FairAllocator::convert_to_dirty( unsigned process_id , Address block_id )
{
	if( clean_pools[process_id].find(block_id) == clean_pools[process_id].end())
		return;
	else
	{
		clean_pools[process_id].erase(block_id);
		dirty_pools[process_id].insert(block_id);
	}
}

void FairAllocator::evict(DRAMEVICTSTYLE policy)
{
	assert( should_reclaim());
	return equal_evict();
}

/*
 *@function:
 *@param:
 */
void FairAllocator::equal_evict(  )
{
	int reclaim_pages = min_free_pages-(total_page_count-busy_pages);
	//reclaim equally
	double rate;		
	Address proc_busy_size;
	Address release_size;
	for( uint32_t i=0; i< process_count; i++)
	{
		proc_busy_size = clean_pools[i].size()+dirty_pools[i].size();
		rate = (double)proc_busy_size/total_page_count;
		release_size = (int)(rate*reclaim_pages)+1;
		Release( i, release_size);
	}
}

void FairAllocator::fairness_evict()
{
}
