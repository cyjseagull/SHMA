/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef ZONE_H_
#define ZONE_H_
#include "g_std/g_multimap.h"
#include "pad.h"
#include "common/common_functions.h"
#include "common/common_structures.h"
#include "common/global_const.h"
#include "MMU/page.h"
#include "memory_hierarchy.h"
#include "math.h"
/***-------free area for buddy allocator-------***/
struct FreeArea
{
	//store start page description of 2^nr_free pages block , lru point to next block 
	FlexiList<Page>* block_list;
	unsigned long nr_free;
};

/***-#--------cpu page cache -------#-***/
typedef std::list<Page*> PageList;
struct PerCpuPages
{
	unsigned int count;	//number of page in page cache
	unsigned int low;	//low bound,need complement pages to page cache
	unsigned int high;	//high bound, page cache has no pages
	unsigned int batch;	//number of pages need to be deleted from or added to page cache
	PageList page_list;	//page list in page cache	
};

struct PerCpuPageset
{
	struct PerCpuPages pcp[2];	//0,hot;1,cold
};

//forward declarition
class MemoryNode;
/***-#---related to memory zone---#-***/
class Zone
{
	public:
	Zone( ZoneType type , Address start_addr , Address end_addr );
	//very important: free allocated object
	~Zone();

	void set_pages_thres( uint64_t min , double low_rate=1.25, double high_rate=1.5)
	{
		pages_min = min;
		pages_low = (uint64_t)(low_rate*pages_min);
		pages_high = (uint64_t)(high_rate*pages_min);
	}
	/***---init per_cpu_pageset for zone---***/
	void setup_zone_pageset();
	PerCpuPages* get_cpu_cold_pages( unsigned cpu_id )
	{
		return &(page_set[cpu_id]->pcp[1]);
	}

	PerCpuPages* get_cpu_hot_pages( unsigned cpu_id )
	{
		return &(page_set[cpu_id]->pcp[0]);
	}

	void free_area_init_zone( MemoryNode* mem_node , uint64_t page_size);
	/*#########------------------##########*/
	//number of free pages of zone
	uint64_t free_pages;
	//reserved pages for zone  
	uint64_t pages_min;
	//low bound for reclaiming pages 
	uint64_t pages_low;
	//high bound for reclaiming pages
	uint64_t pages_high;

	ZoneType zone_type;	//DMA,NROMAL,HighMem
	//first page number of zone   
	uint64_t zone_start_pfn;
	//buddy allocator related
	FreeArea free_area[MAXORDER+1];
	//active &&inactive page list
	Page* active_page_head;
	Page* inactive_page_head;
	//number of active pages && inactive pages
	uint64_t active_page_num;
	uint64_t inactive_page_num;
	//two cpu page cache, one for hot page allocate, another for cold page allocate
	std::vector<PerCpuPageset*> page_set;

	MemoryNode *node_ptr; //point to node which contained this zone
		void memmap_init_zone( MemoryNode* mem_node , Zone* zone, uint64_t start_pfn , uint64_t size );
	private:
		unsigned int zone_batchsize();
		void zone_pageset_init(PerCpuPageset* ps);
};

/***-#-------memory node-----------#-***/
typedef std::map<ZoneType , Zone*> NodeZone;
typedef std::map<ZoneType , Zone*>::iterator NodeZonePtr;
//memory node , corresponding to pd_data_t(pglist_data)
//UMA memory system , there only has a node(contig_page_data)
class MemoryNode
{
	public:
	MemoryNode( unsigned id , Address start_addr );
	~MemoryNode();
	bool zone_exists( std::string zone_name)
	{
		if( node_zones[string_to_zonetype(zone_name)] )
			return true;
		return false;
	}
	
	bool zone_exists( ZoneType type)
	{
		if( node_zones[type])
			return true;
		return false;
	}
	
	Page* get_page_ptr( uint64_t page_id )
	{
		if( page_id >= node_page_num)
			std::cout<<"page_id:"<<page_id<<" page_num:"<<node_page_num<<std::endl;
		assert(page_id<node_page_num);
		if( node_mem_map.count(page_id) )
			return node_mem_map[page_id];
		else
		{
			Page* tmp_page = gm_memalign<Page>(CACHE_LINE_BYTES,1);
			node_mem_map[page_id] = new (tmp_page) Page(page_id);
			return tmp_page;
		}
	}

	private:
		uint64_t calculate_total_pages();
		void init_zones();
		void setup_per_zone_wmarks();
	private:
		g_map<Address, Page*> node_mem_map;
	public:
		uint64_t zone_lowest_possible[MAX_NR_ZONES];
		uint64_t zone_highest_possible[MAX_NR_ZONES];
		unsigned nr_zones;
		//total number of physical pages 
		uint64_t present_pages;

		unsigned node_id;
		//start page number of node
		uint64_t node_start_pfn;
		uint64_t node_page_num;

		uint64_t lowmem_kbytes;
		//get size of page reserved pool 
		uint64_t min_free_kbytes;
		//map between zone name and zone struct
		//NodeZone node_zones;
		Zone** node_zones;
};
#endif
