/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */

#include "MMU/zone.h"
#include "zsim.h"
//init zone
Zone::Zone(ZoneType type , uint64_t start_pfn ,
		   uint64_t end_pfn):zone_type(type),
	       zone_start_pfn(start_pfn)
{
	assert(start_pfn < end_pfn);
	free_pages = end_pfn - start_pfn;
	//init per cpu pageset
	setup_zone_pageset();
}

Zone::~Zone()
{
}

void Zone::memmap_init_zone(MemoryNode* mem_node,
			Zone* zone , uint64_t start_pfn , uint64_t size )
{
	debug_printf("init zone memmap");
	free_area_init_zone(mem_node,size);
}

/*
 *@function: init free area  
 */
void Zone::free_area_init_zone( MemoryNode* mem_node, uint64_t page_number)
{
	debug_printf("init free area");
	FlexiList<Page>* tmp_block_list = gm_memalign<FlexiList<Page> >(CACHE_LINE_BYTES, MAXORDER+1);
	for( unsigned i=0 ; i<=MAXORDER ; i++)
	{
		free_area[i].block_list = new (&tmp_block_list[i]) FlexiList<Page>();
		free_area[i].nr_free = 0;
	}
	unsigned long max_block_num = 0;
	uint64_t page_index = 0;
	//std::cout<<"page_number:"<<page_number<<std::endl;
	for( int i= MAXORDER; i>0 && page_number >0 ; i--)
	{
	   max_block_num = (page_number)/(1<<i);
	   //std::cout<<"order "<<i<<" block num is:"<<max_block_num<<std::endl;
	   if( max_block_num >0 )
	   {
		  free_area[i].nr_free = max_block_num;
		  page_number -= max_block_num*(1<<i);
		  for( unsigned long j=0; j< max_block_num ; j++)
		  {
		 	 free_area[i].block_list->push_block_back(mem_node->get_page_ptr(page_index));
			 page_index += (1<<i);
		  }
	   }
	}
	std::cout<<"maxorder list size:"<<free_area[MAXORDER].block_list->get_size()<<std::endl;
	std::cout<<"free area init finished"<<std::endl;
}
//######init per_cpu_pageset for zone
unsigned int Zone::zone_batchsize()
{
	int batch;
	//one batch is comprised of 1024 pages 
	batch = free_pages >>10;
	
	if( batch*(zinfo->page_size)>512*1024)
		batch = (512*1024)/(zinfo->page_size);
	else
		batch /= 4;
	if( batch <1 )
		batch = 1;
	batch = log2((uint16_t)(batch + batch/2))-1;
	return batch;
}

void Zone::setup_zone_pageset()
{
	//allocate zone per_cpu_pageset
	for( unsigned i=0 ; i< (zinfo->numCores); i++)
	{
		PerCpuPageset* per_cpu = gm_memalign<PerCpuPageset>(CACHE_LINE_BYTES);
		PerCpuPageset* ps = new (per_cpu) PerCpuPageset;
		page_set.push_back( ps );
		zone_pageset_init( ps );
	}
}

void Zone::zone_pageset_init(PerCpuPageset* ps)
{
	PerCpuPages* hot_pps = &ps->pcp[0];
	PerCpuPages* cold_pps = &ps->pcp[1];
	hot_pps->count = 0;
	cold_pps->count = 0;
	hot_pps->page_list.clear();
	cold_pps->page_list.clear();
	
	if( zinfo->percpu_pagelist_fraction)
	{
		unsigned int high = (unsigned int)(free_pages/zinfo->percpu_pagelist_fraction);
		unsigned int batch = Max((unsigned)1,(unsigned)(high/4));
		if( (high/4)>(zinfo->page_shift)*8)
			batch = zinfo->page_shift*8;
		 hot_pps->batch = batch;
		 hot_pps->high = high;
		 cold_pps->batch = batch;
		 cold_pps->high = 2*batch;
	}
	else
	{
		unsigned int batch = zone_batchsize();
		unsigned int high = 6*batch;
		batch = Max((unsigned)1,(unsigned)batch);
		hot_pps->batch = batch;
		hot_pps->high = high;
		cold_pps->batch = Max((unsigned)1,(unsigned)(batch/2) );
		cold_pps->high = 2*batch;
	}
}

/***--------Memory node related----------***/
MemoryNode::MemoryNode( unsigned id , Address start_addr=0):
	node_id(id) , node_start_pfn( start_addr >> zinfo->page_shift),lowmem_kbytes(0),min_free_kbytes(0)
{
	debug_printf("create memory node");
	memset(zone_lowest_possible , 0 , sizeof(zone_lowest_possible));
	memset( zone_highest_possible , 0 , sizeof(zone_highest_possible));

	zone_lowest_possible[0]= node_start_pfn;
	zone_highest_possible[0]= zinfo->max_zone_pfns[Zone_DMA];
	for( unsigned i=1 ; i< MAX_NR_ZONES; i++)
	{
		zone_lowest_possible[i] = zone_highest_possible[i-1];
		zone_highest_possible[i] = Max(zinfo->max_zone_pfns[i] ,
									    zone_lowest_possible[i]);
		debug_printf("zone lowest possible %d:%lld , zone highest possible %d: %lld",i,zone_lowest_possible[i],i,zone_highest_possible[i]);
	}
	//calculate total page number of a node
	node_page_num = calculate_total_pages();
	present_pages = node_page_num;
	//allocate node_mem_map
	debug_printf("memory node: allocate node mem map , page num: %lld",node_page_num);
	//allocate_node_mem_map(0,node_page_num);
	init_zones();
	//init water value for per zone
	setup_per_zone_wmarks();
}

//you are very important for delete some allocated objects
//such as zones
MemoryNode::~MemoryNode()
{
}

uint64_t MemoryNode::calculate_total_pages()
{
	uint64_t total_page = 0;
	for( unsigned i=0; i<MAX_NR_ZONES;i++)
	{
		assert( zone_lowest_possible[i]<= zone_highest_possible[i]);
		total_page += zone_highest_possible[i]-zone_lowest_possible[i];
	}
	return total_page;
}


//init zones
void MemoryNode::init_zones()
{
	debug_printf("init zones");
	node_zones = gm_memalign<Zone*>(CACHE_LINE_BYTES , 4);
	for( unsigned i=0 ; i< MAX_NR_ZONES ; i++)
	{
	   if( zone_highest_possible[i]!=zone_lowest_possible[i] )
		{
			debug_printf("create zone %d",i);
			//allocate and init zone
			Zone* tmp_zone = gm_memalign<Zone>( CACHE_LINE_BYTES , 1);	
			node_zones[i] = new (tmp_zone) Zone((ZoneType)i , 
				zone_lowest_possible[i] , zone_highest_possible[i]);

			node_zones[i]->node_ptr = this;
			node_zones[i]->memmap_init_zone(this,node_zones[(ZoneType)i] , zone_lowest_possible[i],
					zone_highest_possible[i]);
			nr_zones++;
		}
	   else{
			node_zones[i] = NULL;
	   }
	   debug_printf("number of zones are: %d",nr_zones);
	}
}

void MemoryNode::setup_per_zone_wmarks()
{
	debug_printf("set up per zone watermarks");
	//must have DMA and Normal zones
	assert(node_zones[Zone_Normal]);
	assert( zinfo->page_shift >= 10);
	uint64_t normal_pages = 0 , dma_pages = 0;
	//NodeZonePtr p_zones = node_zones.begin(); 
	Zone* p_zones = NULL;
	std::cout<<"set low mem kbytes"<<std::endl;
	for( int i=0;i< MAX_NR_ZONES; i++)
	{
		p_zones = node_zones[i];
		debug_printf("get information of node_zones");
		if( p_zones )
		{
			uint64_t pages = p_zones->free_pages;
			if( i==Zone_DMA)
				dma_pages = pages;
			else if( i==Zone_Normal)
				normal_pages = pages;
			lowmem_kbytes += pages;
		}
	}
	lowmem_kbytes <<= (zinfo->page_shift-10);
	min_free_kbytes = sqrt(16*lowmem_kbytes);
	debug_printf("low memory size:%lld, min_free_kbytes:%lld",lowmem_kbytes,min_free_kbytes);
	uint64_t min_free_pages = min_free_kbytes>>(zinfo->page_shift-10);
	double rate = (double)normal_pages/(double)(dma_pages+normal_pages); 
	uint64_t min_dma = (uint64_t)((1-rate)*min_free_pages);
	uint64_t min_normal =(uint64_t)(rate*min_free_pages);
	//set pages_min , pages_high , pages_low for DMA and Normal zones
	if( node_zones[Zone_DMA])
		node_zones[Zone_DMA]->set_pages_thres(min_dma);
	if(  node_zones[Zone_Normal])
		node_zones[Zone_Normal]->set_pages_thres(min_normal);
	//setup watermarks(pages_min,pages_low,pages_high) for high memory zone
	if(  node_zones[Zone_HighMem])
	{
		uint64_t min_highmem = node_zones[Zone_HighMem]->free_pages/1024;
		min_highmem = Min( Max(min_highmem,(uint64_t)SWAP_CLUSTER_MAX),(uint64_t)128);
		node_zones[Zone_HighMem]->set_pages_thres(min_highmem);	
	}
}
