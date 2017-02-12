/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * memory access cycles: 117 cycles
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#include <vector>
#include <list>
#include <iterator>
#include "memory_hierarchy.h"
#include "DRAM-buffer/DRAM_buffer_block.h"
#include "page-table/page_table.h"
#include "common/common_functions.h"
#include "zsim.h"
#include "galloc.h"
#include "pad.h"
#include "core.h"
#include "timing_core.h"
/*-------------legacy paging------------*/
lock_t NormalPaging::table_lock;
NormalPaging::NormalPaging(PagingStyle select): mode(select),cur_page_table_num(0)
 {
		debug_printf("create legacy paging");
		assert(zinfo);
		if( select == Legacy_Normal)
		{
			zinfo->page_size = 4*power(2,10);	//4KB
			zinfo->page_shift = 12;	
		}
		else if(select == Legacy_Huge)
		{	
			zinfo->page_size = 4*power(2,20);
			zinfo->page_shift=22;
		}
		PageTable* table= gm_memalign<PageTable>( CACHE_LINE_BYTES, 1);
		page_directory = new(table)PageTable(ENTRY_1024);
		debug_printf("zinfo->block size is: %d",zinfo->block_size);
		buffer_table_entry_num = zinfo->page_size/zinfo->block_size;
		buffer_table_shift = zinfo->page_shift - zinfo->block_shift;
		debug_printf("create legacy paging succeed");
		futex_init(&table_lock);
 }

NormalPaging::~NormalPaging()
{
}

/*****-----functional interface of Legacy-Paging----*****/
int NormalPaging::map_page_table( Address addr, void* pg_ptr, bool is_buffer )
{
	BasePDTEntry* entry;
	return map_page_table(addr, pg_ptr, is_buffer, entry);
}

int NormalPaging::map_page_table(Address addr ,void* pg_ptr , bool is_buffer, 
		BasePDTEntry*& mapped_entry)
{
	mapped_entry = NULL;
	int latency = 0;
	if( is_buffer )
		debug_printf("map address:(%x,%x)",addr);
	//update page table
	unsigned pd_entry_id = get_page_directory_off( addr , mode );
	//unsigned buffer_entry_id = get_buffer_table_off(addr,buffer_table_shift,mode);
	int allocate_time = 0;
	//4KB small pages
	if( mode == Legacy_Normal )
	{
		unsigned pt_entry_id = get_pagetable_off(addr , mode);
		PageTable* table = allocate_one_pagetable( pd_entry_id, allocate_time );
		if( !table )
		{
			return false;
		}
		if( !is_valid(table, pt_entry_id) )
			mapped_entry = (*table)[pt_entry_id];
		validate_entry(table , pt_entry_id , pg_ptr , is_buffer);
		/*
		if( is_buffer)
		{
			//main memory block size is equal to buffer block size
			//extend_one_buffer_map(addr,table,pg_ptr , pt_entry_id ,buffer_entry_id, buffer_table_entry_num, mapped_entry);
			validate_entry(table , pt_entry_id , pg_ptr , true);
		}
		else
		{
			//debug_printf("validate entry %d",pt_entry_id);
			//if( !is_valid(table, pt_entry_id) )
			//	mapped_entry = (*table)[pt_entry_id];
			validate_entry(table , pt_entry_id , pg_ptr , false);
		}*/
	}
	//4MB Huge pages
	else if( mode == Legacy_Huge)
	{
		if( !is_valid(page_directory, pd_entry_id) )
			mapped_entry = (*page_directory)[pd_entry_id];
		validate_entry(page_directory,pd_entry_id, pg_ptr, is_buffer);
		/*
		if( is_buffer)
		{
			//extend_one_buffer_map(addr ,page_directory,pg_ptr,pd_entry_id ,buffer_entry_id, buffer_table_entry_num, mapped_entry);
			validate_entry(page_directory,pd_entry_id, pg_ptr, true);	
		}
		else
		{
			//if( !is_valid(page_directory, pd_entry_id) )
			//	mapped_entry = (*page_directory)[pd_entry_id];
			//std::cout<<"validate "<<pd_entry_id<<std::endl;
			validate_entry(page_directory,pd_entry_id, pg_ptr, false);	
			//std::cout<<"validate "<<pd_entry_id<<" end"<<std::endl;
		}*/
	}
	latency = zinfo->mem_access_time*(1+allocate_time);	//allocate overhead
	return latency;
}


bool NormalPaging::unmap_page_table( Address addr)
{
	unsigned pd_entry_id = get_page_directory_off(addr,mode);
	if( mode == Legacy_Normal )
	{
		unsigned pt_id = get_pagetable_off(addr,mode);
		PageTable* table = NULL;
		if(page_directory)
		{
			get_next_level_address<PageTable>(page_directory , pd_entry_id);
		}
		if( !table )
		{
			debug_printf("entry to be invalidate of TLB isn't valid!");
			return false;
		}
		invalidate_page( table , pt_id );
	}
	else if( mode == Legacy_Huge )
	{
		invalidate_page( page_directory , pd_entry_id);		
	}
	return true;
}

/*
 *@function: access page table
 *@req: memory request issued from TLB 
 */
Address NormalPaging::access(MemReq &req )
{
	Address addr = req.lineAddr;
	bool pbuffer = false;
	unsigned pg_dir_off = get_page_directory_off(addr,mode);
	unsigned pg_id;
	//first access memory get page directory address
	void* ptr = (void*)page_directory;
	PageTable* pgt = NULL;
	if( mode== Legacy_Normal )
	{
		ptr = page_directory->get_next_level_address(pg_dir_off);
		if( !ptr )
			return PAGE_FAULT_SIG;
		unsigned pg_off = get_pagetable_off(addr,mode);
		//second access memory get page directory address
		pbuffer = point_to_buffer_table( (PageTable*)ptr,pg_off);
		pgt = (PageTable*)ptr;
		//point to buffer table or page 
		ptr = get_next_level_address<PageTable>((PageTable*)ptr,pg_off);
		pg_id = pg_off;
		req.cycle += (zinfo->mem_access_time*2);	//assume memory accessing cycle is 117 cycles
		if( !ptr )
			return PAGE_FAULT_SIG;
	}
	else if( mode==Legacy_Huge)
	{
		pgt = (PageTable*)ptr;
		pbuffer = point_to_buffer_table((PageTable*)ptr,pg_dir_off);
		ptr = get_next_level_address<PageTable>((PageTable*)ptr,pg_dir_off);
		pg_id = pg_dir_off;

		req.cycle += (zinfo->mem_access_time*3);
		if( !ptr )
			return PAGE_FAULT_SIG;
	}
	bool write_back = false;  
	if( req.type == WRITEBACK)
		write_back = true;
	uint32_t access_counter = 0;
	if( write_back )
	{
		access_counter = req.childId;
	}
	bool set_dirty = false;
	if(req.type == SETDIRTY)
	{
		set_dirty = true;
	}
	Address result = get_block_id( req ,pgt,ptr,pg_id , mode, pbuffer , set_dirty, 
								write_back,access_counter );
	return result;
}

PageTable* NormalPaging::allocate_one_pagetable(unsigned pd_entry_id, int& allocate_time)
{
	PageTable* table = get_next_level_address<PageTable>(page_directory,pd_entry_id);
	allocate_time = 0;
	if(table == NULL)
	{
		assert( cur_page_table_num<= 1024 && mode==Legacy_Normal);
		table = new PageTable(ENTRY_1024);
		allocate_time = 1;
		validate_entry(page_directory , pd_entry_id , table);		  
		cur_page_table_num++;
	}
	return table;
}


/*
 *@function: allocate page tables
 *@param pd_entry: entry id of page directory 
 */
bool NormalPaging::allocate_page_table(entry_list pd_entry)
{
	int allocate_time;
	for( entry_list_ptr it=pd_entry.begin() ;it!=pd_entry.end(); it++)
	{
		if( !allocate_one_pagetable(*it, allocate_time) )
			return false;
	}
	return true;
}


/*
 *@function: remove page directory, although this is seldom used , but it's provided for complete function
 */
void NormalPaging::remove_root_directory()
{
	cur_page_table_num=0;
}

/*
 *@function: remove page table whose address is recorded in page directory's page_table_idth entry
 *@param page_table_id: entry num of page directory,page table this entry pointed to can be removed if the whole page table is unuseful
 */
bool NormalPaging::remove_page_table(unsigned pd_entry_id)
{
	bool succeed = false;
	assert(mode==Legacy_Normal);
	if( is_present(page_directory , pd_entry_id) )
	{
			invalidate_entry<PageTable>(page_directory, pd_entry_id);
			cur_page_table_num--;
			succeed = true;
	}
	return succeed;
}

/*
 *@function: remove several number of page tables
 *@param pd_entry: list which record entry num of page direcotry , these entries point to page table will be removed 
 */
bool NormalPaging::remove_page_table(entry_list pd_entry)
{
	for( entry_list_ptr it=pd_entry.begin(); it!=pd_entry.end();it++ )
	{
		assert( *it < ENTRY_1024 );
		remove_page_table(*it);
	}
	return true;
}

/*
 *@function:
 *@param addr:
 *@param size:
 */
bool NormalPaging::allocate_page_table(Address addr , Address size)
{
	assert( mode==Legacy_Normal );
	//address must align with 4M
	if(addr & 0x3fffff)
	{
		fatal("address must align with 4M");
		return false;
	}
	unsigned base_entry_id = get_page_directory_off( addr, mode );
	unsigned page_num = (size+0x3fffff)>>22;	//get page size
	bool succeed = true;
	int allocate_time;
	for( unsigned i=0; i<page_num;i++)
	{
		if( !allocate_one_pagetable(base_entry_id+i,allocate_time))
		{
			debug_printf("allocate page table for %d th entry failed",base_entry_id+i);
			succeed = false;
		}
	}
	return succeed;
}

/*
 *@function:
 *@param addr:
 *@param size:
 */
bool NormalPaging::remove_page_table( Address addr , Address size)
{
	assert( mode== Legacy_Normal);
	if( addr & 0x3fffff )
	{
		fatal("address must align wit 4M");
		return false;
	}
	unsigned base_pageNo = get_page_directory_off( addr , mode);
	unsigned page_num = (addr+0x3fffff)>>22;
	bool succeed = true;
	for( unsigned i=0; i<page_num ; i++)
	{
		if( !(remove_page_table(base_pageNo+page_num)))
		{
			debug_printf("remove page table of %d th entry of page directory failed", base_pageNo+page_num);
			succeed = false;
		}
	}
	return succeed;
}

/*--------------PAE Paging------------*/
PAEPaging::PAEPaging(PagingStyle select): mode(select) , cur_pdt_num(0),cur_pt_num(0)
{ 
	//init page directory pointer , its entry num is 4
	page_directory_pointer = new PageTable(ENTRY_4);
	assert(zinfo);
	if(select == PAE_Normal)
	{
		zinfo->page_size = 4*power(2,10);	//4KB
		zinfo->page_shift = 12;
	}
	else if(select==PAE_Huge)
	{
		zinfo->page_size = 2*power(2,20);	//2MB
		zinfo->page_shift = 21;
	}
	buffer_table_entry_num = zinfo->page_size/zinfo->block_size;
	buffer_table_shift = zinfo->page_shift-zinfo->block_shift;
}

//constructor of PAE paging
PAEPaging::~PAEPaging()
{
	remove_root_directory();	//remove whole page table structures
}

/*****-----functional interface of paging----*****/
int PAEPaging::map_page_table(Address addr, void* pg_ptr, bool pbuffer)
{
	BasePDTEntry* entry;
	return map_page_table(addr, pg_ptr, pbuffer, entry);
}

int PAEPaging::map_page_table(Address addr, void* pg_ptr , bool pbuffer, BasePDTEntry*& mapped_entry)
{
	mapped_entry = NULL;
	int latency = 0;
	//page directory pointer offset
	unsigned pdp_id = get_page_directory_pointer_off( addr , mode);
	//page directory offset
	unsigned pd_id = get_page_directory_off(addr,mode);
	//unsigned buffer_entry_id = get_buffer_table_off(addr,buffer_table_shift,mode);
	PageTable* table;
	int allocate_time = 0;
	if( mode == PAE_Normal)
	{
		//page table offset
		unsigned pt_id = get_pagetable_off(addr , mode);
		if( (table = allocate_page_table(pdp_id , pd_id, allocate_time))==NULL )
		{
			debug_printf("allocate page table failed");
			return false;
		}
		if( !is_valid(table, pt_id) )
			mapped_entry = (*table)[pt_id];
		validate_entry(table , pt_id ,pg_ptr, pbuffer);	
		/*
		//if( pbuffer )
		//{
			//extend_one_buffer_map(addr , table , pg_ptr , pt_id , buffer_entry_id , buffer_table_entry_num, mapped_entry);			
			validate_entry(table , pt_id ,pg_ptr, pbuffer);	
		//}
		//else
		//{
		//	if( !is_valid(table, pt_id) )
		//		mapped_entry = (*table)[pt_id];

			validate_entry(table , pt_id ,pg_ptr);	
		}*/
	}
	else if( mode == PAE_Huge)
	{
		//allocate page directory
		if( (table=allocate_pdt(pdp_id, allocate_time))==NULL)
		{
			debug_printf("allocate page directory failed !");
			return false;
		}
		if( !is_valid(table, pd_id) )
			mapped_entry = (*table)[pd_id];

		validate_entry( table , pd_id , pg_ptr, pbuffer);
		/*
		if( pbuffer)
			extend_one_buffer_map(addr,table, pg_ptr , pd_id, buffer_entry_id , buffer_table_entry_num, mapped_entry);
		else
		{
			if( !is_valid(table, pd_id) )
				mapped_entry = (*table)[pd_id];

			validate_entry( table , pd_id , pg_ptr);
		}*/
	}
	latency = zinfo->mem_access_time*(1+allocate_time);
	return latency;
}

bool PAEPaging::unmap_page_table( Address addr)
{
	unsigned pdp_id	= get_page_directory_pointer_off(addr , mode);
	unsigned pd_id = get_page_directory_off(addr , mode);
	PageTable* pd_ptr = get_next_level_address<PageTable>( page_directory_pointer , pdp_id); 
	if( pd_ptr )
	{
		if( mode == PAE_Normal)
		{
			PageTable* pt_ptr = get_next_level_address<PageTable>(pd_ptr , pd_id);
			if( pt_ptr )
			{
			 unsigned pt_id = get_pagetable_off(addr,mode);
			 invalidate_page( pt_ptr , pt_id );
			 return true;
			}
		}
		else if( mode == PAE_Huge)
		{
			invalidate_page(pd_ptr , pd_id);
			return true;
		}
	}
	return false;
}

Address PAEPaging::access(MemReq &req)
{
	Address addr = req.lineAddr;
	bool pbuffer = false;
	unsigned pdp_id = get_page_directory_pointer_off(addr , mode);
	unsigned pd_id = get_page_directory_off(addr,mode);
	unsigned pg_id;
	//first memory access,get page directory address
	void* ptr = get_next_level_address<void>(page_directory_pointer,pdp_id);
	PageTable* pgt = NULL;
	if( !ptr )
		return PAGE_FAULT_SIG;
	if( mode==PAE_Huge )
	{
		pgt = (PageTable*)page_directory_pointer;
		pbuffer = point_to_buffer_table((PageTable*)ptr,pd_id);
		pg_id = pd_id;
		req.cycle += (zinfo->mem_access_time*3);
	}
	//second memory access,get page table address  
	ptr = get_next_level_address<void>((PageTable*)ptr,pd_id);
	if(!ptr)
		return PAGE_FAULT_SIG;
	if( mode == PAE_Normal )
	{
		pgt = (PageTable*)ptr;
		unsigned pg_id = get_pagetable_off(addr,mode);
		pbuffer = point_to_buffer_table( (PageTable*)ptr,pg_id);
		//page or buffer table
		ptr = get_next_level_address<void>((PageTable*)ptr,pg_id);
		req.cycle += (zinfo->mem_access_time*4);
	}
	bool set_dirty = false;
	if(req.type==SETDIRTY)
		set_dirty = true;
	uint32_t access_counter = 0;
	bool write_back = false;
	if( req.type == WRITEBACK)
	{
		write_back = true;
		access_counter = req.childId;
	}
	return get_block_id(req, pgt, ptr , pg_id, mode,pbuffer , 
						set_dirty, write_back, access_counter);
}

/****----basic call back of Legacy-Paging----****/
PageTable* PAEPaging::allocate_page_table( unsigned pdpt_entry_id , unsigned pd_entry_id , int &allocate_time)
{
	allocate_time = 0;
	//only on PAE-NORMAL mode,can allocate page table
	assert( mode == PAE_Normal && cur_pt_num< ENTRY_512);
	//page directory
	PageTable* table = get_next_level_address<PageTable>( page_directory_pointer , pdpt_entry_id);
	PageTable* page=NULL;
	//allocate page directory first
	if( !table)
	{
		//page directory
		table = new PageTable(ENTRY_512);
		//connect page directory pointer with page directory
		validate_entry( page_directory_pointer , pdpt_entry_id , table);
		//page table 
		page = new PageTable(ENTRY_512);
		allocate_time += 2;
		//connect page directory with page table
		validate_entry( table, pd_entry_id ,page);
		cur_pt_num++;	
		cur_pdt_num++;
		return page; //return page table
	}
	else
	{
		page = get_next_level_address<PageTable>(table,pd_entry_id);
		if( !page)
		{
			//new page table 
			page = new PageTable(ENTRY_512);
			allocate_time += 1;
			validate_entry(table , pd_entry_id, page);
			cur_pt_num++;
		}
		//page table already exist
		return page;
	}
	return page;
}

/*
 *@function: allocate page table according to input
 *%attention: information of current page tables must can be contained within page directory entries before or when allocating page table
 *@param pdt_entry: list of (page directory pointer entry id , page direcotry entry id)
 */
bool PAEPaging::allocate_page_table(pair_list pdt_entry)
{
	bool succeed = true;
	int time = 0;
	for( pair_list_ptr it= pdt_entry.begin();
		it!=pdt_entry.end();it++)
	{
		if( !(allocate_page_table((*it).first , (*it).second), time) )
		{
			succeed= false;
			debug_printf("allocate page directory for entry %d for page directory pointer failed");
		}
	}
	return succeed;
}

PageTable* PAEPaging::allocate_pdt( unsigned pdpt_entry , int & alloc_time)
{
	alloc_time = 0;
	assert( cur_pdt_num < 4 && pdpt_entry < 4 );
	PageTable* page_dir = NULL;
	//pdpt_entry of page_direcotry_pointer has already pointed to page directory 
	if( (page_dir = get_next_level_address<PageTable>(page_directory_pointer, pdpt_entry))!=NULL )
		return page_dir;
	else
	{
		//page directory
		page_dir=new PageTable(ENTRY_512);
		alloc_time += 1;
		validate_entry( page_directory_pointer , pdpt_entry ,page_dir);
		cur_pt_num++;
	}
	return page_dir;
}

/*
 *@function: allocate page directory table according to input
 *%attention: number of page directory must no more than 4
 *@param pdpt_entry: list of page directory pointer entry id
 */
bool PAEPaging::allocate_pdt(entry_list pdpt_entry)
{
	bool succeed = true;
	int time;
	for( entry_list_ptr it=pdpt_entry.begin() ; it!=pdpt_entry.end() ; it++)
	{
		if( !(allocate_pdt(*it, time)))
		{
			succeed = false;
			debug_printf("allocate page directory for %d entry of page directory pointer failed",*it);
		}
	 }
	return succeed;
}


bool PAEPaging::allocate_page_table(Address addr , Address size)
{
	bool succeed = true;
	int time;
	if ( addr & 0x1fffff )
	{
		fatal("base address must align with 1MB");
		return false;
	}
	unsigned pd_offset =  get_page_directory_off( addr , PAE_Normal);
	unsigned pdp_offset = get_page_directory_pointer_off( addr , PAE_Normal);
	unsigned page_num = (size+0x1fffff) >>21;
	assert( page_num + pd_offset < 512);
	for( unsigned i=0 ; i<page_num ; i++)
	{
		if (allocate_page_table( pdp_offset ,i+pd_offset, time)==false)
		{
			debug_printf("allocate %d page table for 0x%x failed",page_num , addr);
			succeed = false;
		}
	}
	return succeed;
}

bool PAEPaging::allocate_pdt(Address addr , Address size)
{
	bool succeed = true;
	int time;
	if( addr&0x7fffffff )
	{
		fatal("base address must align with 1GB when allocate page directory in PAE mode");
		succeed = false;
	}
	unsigned base_addr = get_page_directory_pointer_off(addr , mode);
	unsigned page_dir_num = (size+0x7fffffff)>>30;
	for( unsigned i = 0; i<page_dir_num ; i++)
	{
		if( allocate_pdt(base_addr+i, time)==false)
		{
			debug_printf("allocate number of %d page directory for address 0x%x failed",page_dir_num , addr);
			succeed = false;
		}
	}
	return succeed;
}


//delete whole structures of page tables
void PAEPaging::remove_root_directory()
{
	assert(page_directory_pointer);
	for(unsigned i=0 ; i<ENTRY_4 ; i++)
	{
		if( is_present(page_directory_pointer , i))
		{
			if( remove_pdt(i)==false )
			{
				debug_printf("remove page directory entry %d of page direcotry pointer pointed to failed", i);
			}
		}
	}
	delete page_directory_pointer;
	page_directory_pointer = NULL;
}

bool PAEPaging::remove_pdt( Address addr , Address size)
{
	bool succeed = true;
	if( addr &0x7fffffff )
	{
		fatal("addr must align with 1GB");
		succeed = false;
	}
	unsigned page_dir_ptr = get_page_directory_pointer_off(addr , PAE_Normal );
	unsigned dir_num = (size+0x7fffffff)>>30;
	for( unsigned i=0; i<dir_num;i++ )
	{
		if( remove_pdt(page_dir_ptr+i) )
		{
			debug_printf("remove page directory whose arrange is (%ld,%ld) failed",addr , addr+size);
			succeed = false;
		}
	}
	return succeed;
}


bool PAEPaging::remove_pdt( unsigned pdp_entry_id)
{
	bool succeed = false;
	PageTable* page_dir = get_next_level_address<PageTable>(page_directory_pointer , pdp_entry_id);
	if(page_dir)
	{
		if( mode == PAE_Huge)
		{
			invalidate_entry<PageTable>( page_directory_pointer,pdp_entry_id);
			succeed = true;
			cur_pdt_num--;
		}
		else if( mode == PAE_Normal)
		{
			unsigned i=0;
			//delete presented page table
			for( i=0 ; i<ENTRY_512; i++)
			{
				if( is_present( page_dir ,i))
				{
					if(remove_page_table(pdp_entry_id ,i)==false)
					{
						debug_printf("remove %d entry of page table failed!",i);
						succeed = false;
					}
				}
			}
			if( i== ENTRY_512 )
			{	
				cur_pdt_num--;
				succeed = true;
			}
		}
	}
	else
	{
		debug_printf("entry %d of page direcotry pointer pointed to nothing", pdp_entry_id);
	}
	return succeed;
}

/*
 *@function:
 *@param pdpt_entry:
 */
bool PAEPaging::remove_pdt( entry_list pdpt_entry)
{
	bool succeed = true;
	for( entry_list_ptr it=pdpt_entry.begin(); 
		 it!= pdpt_entry.end();it++ )
	{
		if( remove_pdt(*it)==false)
		{
			debug_printf("remove page directory pointed by entry %d of page directory pointer failed", *it);
			succeed = false;
		}
	}
	return succeed;
}

bool PAEPaging::remove_page_table( Address addr, Address size)
{
	bool succeed = true;
	if( addr & 0x1fffff)
	{
		fatal("base address must be align with 1MB");
		succeed = false;
	}
	else
	{
		unsigned page_dir_ptr = get_page_directory_pointer_off( addr , PAE_Normal);
		unsigned page_dir = get_page_directory_off(addr , PAE_Normal);
		unsigned page_table_num = (size+0x1fffff)>>21;
		for( unsigned i=0 ; i<page_table_num;i++)
		{
			if(remove_page_table(page_dir_ptr , page_dir+i)==false)
			{
				succeed = false;
				debug_printf("remove (page directory pointer,page directory)-(%d,%d) pointed page table failed",page_dir_ptr,page_dir+i);
			}
		}
	}
	return succeed;
}

/*
 *@function: delete several pages referred by element of pd_entry
 *@param pdp_entry: list of (page_directory_pointer entry id , page_directory entry id)
 *@return: remove succeed , return true; failed return false
 */
bool PAEPaging::remove_page_table(pair_list pdp_entry)
{
	bool succeed = true;
	assert( mode == PAE_Normal);
	for( pair_list_ptr it=pdp_entry.begin(); it!=pdp_entry.end();it++)
	{
		if(remove_page_table( (*it).first , (*it).second) == false)
		{
			debug_printf("remove page table for (page direcotor pointer entry id: %d , page directory id:%d) failed",(*it).first , (*it).second);
			succeed = false;
		}
	}
	return succeed;
}

/*
 *@function: delete page table referred by (pdpt_num[entry id of page directory pointer],pd_num[entry id of page directory])
 *@param pdpt_num: entry id of page directory pointer table
 *@param pd_num: entry id of page directory table
 */
bool PAEPaging::remove_page_table( unsigned pdpt_num , unsigned pd_num )
{
	bool succeed = false;
	assert(pdpt_num<ENTRY_4 && pd_num< ENTRY_512 && mode==PAE_Normal);
	PageTable *page_dir = get_next_level_address<PageTable>( page_directory_pointer , pdpt_num);
	if(page_dir)
	{
		if( is_present(page_dir , pd_num))
		{
			invalidate_entry<PageTable>( page_dir , pd_num );
			cur_pt_num--;
			succeed = true;
		}
		else
			debug_printf("entry %d of page directory is invalid", pd_num);
	}
	else
	{
		debug_printf("entry %d of page directory pointer is invalid",pdpt_num);
	}
	return succeed;
}

/*-----------LongMode Paging--------------*/
//PageTable* LongModePaging::pml4;
lock_t LongModePaging::table_lock;
LongModePaging::LongModePaging(PagingStyle select): mode(select),cur_pdp_num(0),cur_pd_num(0),cur_pt_num(0)
{
  PageTable* table = gm_memalign<PageTable>(CACHE_LINE_BYTES,1);
  pml4=new (table) PageTable(512);
  assert(zinfo);
  if(select == LongMode_Normal)			//4KB
  {
	zinfo->page_size = 4*power(2,10);
	zinfo->page_shift = 12;
  }
  else if(select == LongMode_Middle )			//2MB
  {
	zinfo->page_size = 2*power(2,20);
	zinfo->page_shift=21;
  }
  else if(select == LongMode_Huge )			//1GB
  {
	zinfo->page_size=power(2,30);	
	zinfo->page_shift = 30;
  }
  buffer_table_entry_num = zinfo->page_size/zinfo->block_size;
  buffer_table_shift = zinfo->page_shift - zinfo->block_shift;
  futex_init(&table_lock);
}

LongModePaging::~LongModePaging()
{
	remove_root_directory();
}

/*****-----functional interface of LongMode-Paging----*****/
int LongModePaging::map_page_table(Address addr, void* pg_ptr, bool pbuffer)
{
	BasePDTEntry* entry;
	return map_page_table(addr, pg_ptr, pbuffer, entry);
}


int LongModePaging::map_page_table( Address addr, void* pg_ptr , bool pbuffer, BasePDTEntry*& mapped_entry)
{
	mapped_entry = NULL;
	int latency = 0;
	//std::cout<<"map:"<<std::hex<<addr<<std::endl;
	unsigned pml4,pdp,pd,pt;
	get_domains(addr , pml4 , pdp , pd , pt , mode);
	//unsigned buffer_entry_id = get_buffer_table_off(addr,buffer_table_shift,mode);
	assert( (pml4!=(unsigned)(-1)) && (pdp!=(unsigned)(-1)));
	PageTable* table;
	int alloc_time = 0;
	if( mode == LongMode_Normal)
	{
		assert( (pd!=(unsigned)(-1)) &&(pt!=(unsigned)(-1)));
		table = allocate_page_table(pml4,pdp,pd, alloc_time);
		if( !table )
		{
			debug_printf("allocate page table for LongMode_Normal failed!");
			return false;
		}
		if( !is_valid(table, pt) )	
			mapped_entry = (*table)[pt];
		validate_entry(table , pt , pg_ptr, pbuffer);
		/*
		if( pbuffer)
		{
			//std::cout<<"extend one buffer map"<<std::endl;
			//extend_one_buffer_map(addr,table,pg_ptr , pt, buffer_entry_id, buffer_table_entry_num, mapped_entry);
			validate_entry(table , pt , pg_ptr, true);
		}
		else
		{
			//if( !is_valid(table, pt) )	
			//	mapped_entry = (*table)[pt];
			validate_entry(table , pt , pg_ptr);
		}*/
	}
	else if( mode == LongMode_Middle )
	{
		table = allocate_page_directory( pml4 , pdp, alloc_time);
		if(!table)
		{
			debug_printf("allocate page directory for LongMode_Middle failed!");
			return false;
		}
		if( !is_valid(table, pd) )
			mapped_entry = (*table)[pd];
		validate_entry( table,pd, pg_ptr, pbuffer);
		/*if( pbuffer )
			extend_one_buffer_map(addr , table , pg_ptr , pd , buffer_entry_id , buffer_table_entry_num, mapped_entry);
		else
		{
			if( !is_valid(table, pd) )
				mapped_entry = (*table)[pd];
			validate_entry( table,pd, pg_ptr);
		}*/
	}
	else if( mode == LongMode_Huge )
	{
		table = allocate_page_directory_pointer(pml4, alloc_time);
		if(!table)
		{
			debug_printf("allocate page directory pointer for LongMode_Huge failed!");
			return false;
		}
		if( !is_valid(table, pdp) )
			mapped_entry = (*table)[pdp];

		validate_entry(table,pdp,pg_ptr, pbuffer);
		/*if(pbuffer)
			extend_one_buffer_map(addr , table ,pg_ptr, pdp,buffer_entry_id , buffer_table_entry_num, mapped_entry);
		else
		{
			if( !is_valid(table, pdp) )
				mapped_entry = (*table)[pdp];

			validate_entry(table,pdp,pg_ptr);
		}*/
	}
	latency = zinfo->mem_access_time*(1+alloc_time);
	return latency;
}


inline PageTable* LongModePaging::get_tables(unsigned level , std::vector<unsigned> entry_id_list)
{
	assert( level >= 1 );
	PageTable* table;
	unsigned i = 0;
	table = get_next_level_address<PageTable>( pml4 , entry_id_list[i]);
	level--;	
	i++;
	while( level>0 )
	{
		table = get_next_level_address<PageTable>(table,entry_id_list[i]); 
		if( !table )
			return NULL;
		if( table )
		level--;
		i++;
	}
	return table;
}

bool LongModePaging::unmap_page_table( Address addr)
{
	unsigned pml4_id,pdp_id,pd_id,pt_id;
	std::vector<unsigned> entry_id_vec(4);
	
	get_domains(addr , pml4_id , pdp_id , pd_id , pt_id , mode);
	entry_id_vec[0]=pml4_id;
	entry_id_vec[1]=pdp_id;
	entry_id_vec[2]=pd_id;
	PageTable* table = NULL;
	//point to page directory pointer table
	if( mode == LongMode_Normal)
	{
		table = get_tables(3, entry_id_vec);
		if( !table )
		{
			debug_printf("didn't find entry indexed with %ld !",addr);
			return false;
		}
		invalidate_page(table,pt_id);
	}
	else if( mode == LongMode_Middle )
	{
		table = get_tables(2,entry_id_vec);
		if( !table)
		{
			debug_printf("didn't find entry indexed with %ld !",addr);
			return false;
		}
		invalidate_page( table,pd_id);
	}
	else if( mode == LongMode_Huge)
	{
		table = get_tables(1,entry_id_vec);
		if(!table)
		{
			debug_printf("didn't find entry indexed with %ld !",addr);
			return false;
		}
		invalidate_page(table,pdp_id);
	}
	return false;
}

Address LongModePaging::access(MemReq &req)
{
	Address addr = req.lineAddr;
	unsigned pml4_id,pdp_id,pd_id,pt_id;
	bool pbuffer = false;
	get_domains(addr,pml4_id,pdp_id,pd_id,pt_id,mode);
	//point to page table pointer table 
	PageTable* pdp_ptr = get_next_level_address<PageTable>( pml4,pml4_id );
	PageTable* pgt = NULL;
	if( !pdp_ptr)
	{
		return PAGE_FAULT_SIG;
	}
	if( mode == LongMode_Huge)
	{
	    pgt = (PageTable*)pdp_ptr;
		pbuffer = point_to_buffer_table( (PageTable*)pdp_ptr,pdp_id);
		pt_id = pdp_id;
		//std::cout<<req.lineAddr<<"point to dram buffer:"<<pbuffer<<std::endl;
		req.cycle += (zinfo->mem_access_time*4);
	}
	//point to page or page directory
	void* ptr = get_next_level_address<void>(pdp_ptr,pdp_id );
	if( !ptr )
	{
		return PAGE_FAULT_SIG;
	}
	if( mode == LongMode_Middle)
	{
		pgt = (PageTable*)ptr;
		assert( pd_id != (unsigned)(-1) );
		pbuffer = point_to_buffer_table((PageTable*)ptr,pd_id);
		//std::cout<<req.lineAddr<<"point to dram buffer:"<<pbuffer<<std::endl;
		debug_printf("point to dram buffer: %d",pbuffer);
		//point to page
		ptr = get_next_level_address<void>( (PageTable*)ptr,pd_id);
		pt_id = pd_id;
		req.cycle += (zinfo->mem_access_time*5);
		if( !ptr )
		{
			return PAGE_FAULT_SIG;
		}
	}
	else if( mode == LongMode_Normal)
	{
		assert( pd_id != (unsigned)(-1));
		assert( pt_id != (unsigned)(-1));
		//point to page table
		ptr = get_next_level_address<void>((PageTable*)ptr,pd_id);
		req.cycle += (zinfo->mem_access_time*6);
		if( ptr )
		{
			pbuffer = point_to_buffer_table((PageTable*)ptr,pt_id);
			//std::cout<<req.lineAddr<<" point to dram buffer:"<<pbuffer<<std::endl;
		}
		if( !ptr )
		{
			return PAGE_FAULT_SIG;
		}
		pgt = (PageTable*)ptr;
		//point to page or buffer table 
		ptr = get_next_level_address<void>((PageTable*)ptr,pt_id);
		if( !ptr )
		{
			return PAGE_FAULT_SIG;
		}
	}
	bool write_back = false;
	uint32_t access_counter = 0;
	bool set_dirty = false;
	if( req.type == SETDIRTY)
		set_dirty = true;
	if( req.type==WRITEBACK)
	{
		access_counter = req.childId;
		write_back = true;
	}
	return get_block_id(req ,pgt,ptr, pt_id,mode,pbuffer ,
				set_dirty, write_back ,access_counter);
}


//allocate
PageTable* LongModePaging::allocate_page_directory_pointer( unsigned pml4_entry_id, int& allocate_time)
{
	allocate_time = 0;
	assert( pml4_entry_id<512 && cur_pdp_num<ENTRY_512);
	if( !is_present(pml4, pml4_entry_id))
	{
		PageTable* table_tmp= gm_memalign<PageTable>( CACHE_LINE_BYTES, 1);
		PageTable* table = new (table_tmp) PageTable(ENTRY_512);
		allocate_time++;
		validate_entry(pml4 , pml4_entry_id , table);
		cur_pdp_num++;
		return table;
	}
	PageTable* pg_dir_p = get_next_level_address<PageTable>( pml4,pml4_entry_id);
	return pg_dir_p;
}

bool LongModePaging::allocate_page_directory_pointer(entry_list pml4_entry)
{
	bool succeed = true;
	int allocate_time;
	for( entry_list_ptr it=pml4_entry.begin(); it!=pml4_entry.end();it++)
	{
		if( allocate_page_directory_pointer(*it, allocate_time)==false)
		{
			succeed = false;
			debug_printf("allocate page directory pointer for entry %d of pml4 table failed",*it);
		}
	}
	return succeed;
}

bool LongModePaging::allocate_page_directory(pair_list high_level_entry)
{
	bool succeed=true;
	int allocate_time;
	for( pair_list_ptr it=high_level_entry.begin(); it!=high_level_entry.end() ; it++)
	{
		if( allocate_page_directory((*it).first,(*it).second, allocate_time)==false)
		{
			debug_printf("allocate (pml4_entry_id , page directory pointer entry id)---(%d,%d) failed",(*it).first , (*it).second );
			succeed = false;
		}
	}
	return succeed;
}

PageTable* LongModePaging::allocate_page_directory( unsigned pml4_entry_id , unsigned pdpt_entry_id, int& allocate_time )
{
	allocate_time = 0;
	PageTable* pdp_table = get_next_level_address<PageTable>( pml4 , pml4_entry_id);
	if( pdp_table )
	{
		if(!is_present(pdp_table , pdpt_entry_id))
		{
			PageTable* table_tmp= gm_memalign<PageTable>( CACHE_LINE_BYTES, 1);
			PageTable* pd_table = new (table_tmp) PageTable(ENTRY_512);
			validate_entry(pdp_table , pdpt_entry_id , pd_table);
			cur_pd_num++;
			allocate_time = 1;
			return pd_table;
		}
		else
		{
			PageTable* table = get_next_level_address<PageTable>( pdp_table, pdpt_entry_id);
			return table;
		}
	}
	else
	{
		if(allocate_page_directory_pointer(pml4_entry_id, allocate_time))
		{
			PageTable* pdpt_table = get_next_level_address<PageTable>(pml4,pml4_entry_id);
			PageTable* table_tmp= gm_memalign<PageTable>( CACHE_LINE_BYTES, 1);
			PageTable* pd_table=new (table_tmp)PageTable(ENTRY_512);
			allocate_time++;
			validate_entry(pdpt_table , pdpt_entry_id , pd_table);
			cur_pd_num++;
			return pd_table;
		}
	}
	return NULL;
}


bool LongModePaging::allocate_page_table(triple_list high_level_entry)
{
	bool succeed =true;
	int time;
	for( triple_list_ptr it=high_level_entry.begin() ; it!=high_level_entry.end() ; it++)
	{
		if( !allocate_page_table((*it).first , (*it).second,(*it).third, time))
			succeed = false;
	}
	return succeed;
}


PageTable* LongModePaging::allocate_page_table(unsigned pml4_entry_id , 
		unsigned pdpt_entry_id , unsigned pdt_entry_id , int& alloc_time)
{
	alloc_time = 0;
	assert( mode == LongMode_Normal);
	PageTable* pdp_table=get_next_level_address<PageTable>(pml4 , pml4_entry_id);
	if( pdp_table )
	{
		PageTable* pd_table=get_next_level_address<PageTable>(pdp_table , pdpt_entry_id);
		if(pd_table)
		{
			if(is_present(pd_table , pdt_entry_id))
			{
				PageTable* table = get_next_level_address<PageTable>(pd_table,pdt_entry_id);
				return table;
			}
			else
			{
				PageTable* table_tmp= gm_memalign<PageTable>( CACHE_LINE_BYTES, 1);
				PageTable* table = new (table_tmp)PageTable(ENTRY_512);
				validate_entry(pd_table , pdt_entry_id ,table );
				cur_pt_num++;
				alloc_time++;
				return table;
			}
		}
		//page_direcory doesn't exist allocate
		else
		{
			if( allocate_page_directory(pml4_entry_id,pdpt_entry_id, alloc_time))
			{
				//get page directory
				PageTable* page_dir = get_next_level_address<PageTable>( pdp_table , pdpt_entry_id);
				PageTable* table= gm_memalign<PageTable>( CACHE_LINE_BYTES, 1);
				PageTable* pg_table=new (table)PageTable(ENTRY_512);
				validate_entry(page_dir , pdt_entry_id , pg_table );
				cur_pt_num++;
				alloc_time++;
				return pg_table;
			}
		}
	}
	else
	{
		PageTable* g_tables = gm_memalign<PageTable>(CACHE_LINE_BYTES,3);
		PageTable* pdp_table=new (&g_tables[0])PageTable(ENTRY_512);
		//std::cout<<"validate pg dir pointer in pml4"<<std::dec<<pml4_entry_id<<std::endl;
		validate_entry(pml4,pml4_entry_id,pdp_table);
		cur_pdp_num++;
		PageTable* pd_table=new (&g_tables[1])PageTable(ENTRY_512);
		//std::cout<<"validate page directory table in pg dir pointer"<<std::dec<<pdpt_entry_id<<std::endl;
		validate_entry(pdp_table,pdpt_entry_id , pd_table);
		cur_pd_num++;
		PageTable* pg_table=new (&g_tables[2])PageTable(ENTRY_512);
		//std::cout<<"new page  table"<<std::endl;
		validate_entry(pd_table , pdt_entry_id , pg_table);
		//std::cout<<"validate: "<<std::dec<<pml4_entry_id<<","<<std::dec<<pdpt_entry_id<<","<<std::dec<<pdt_entry_id<<std::endl;
		cur_pt_num++;
		alloc_time += 3;
		return pg_table;
	}
	return NULL;
}


bool LongModePaging::allocate_page_table(Address addr , Address size)
{
	bool succeed = true;
	assert(mode==LongMode_Normal);
	if( addr & 0x1fffff)
	{
		fatal("must align with 2MB");
		succeed = false;
	}
	unsigned pml4_entry = get_pml4_off(addr , mode);
	unsigned pdp_entry = get_page_directory_pointer_off(addr , mode);
	unsigned pd_entry = get_page_directory_off(addr , mode);
	unsigned page_table_num = (size+0x1fffff)>>21;
	int time;
	for( unsigned i =0 ;i<page_table_num ; i++)
	{
		if( allocate_page_table(pml4_entry , pdp_entry , pd_entry+i, time)==false)
		succeed = false;
	}
	return succeed;
}


//remove
void LongModePaging::remove_root_directory()
{
	if( pml4)
	{
		for( unsigned i=0 ; i<ENTRY_512;i++ )
		{
			if(is_present(pml4,i))
			{
				remove_page_directory_pointer(i);
			}
		}
		delete pml4;
		pml4=NULL;
	}
}

bool LongModePaging::remove_page_directory_pointer(unsigned pml4_entry_id)
{
	bool succeed = true;
	PageTable* pdp_table= get_next_level_address<PageTable>(pml4,pml4_entry_id);
	if(pdp_table)
	{
		if( mode!=LongMode_Huge)
		{
			for(unsigned i=0;i<ENTRY_512;i++)
			{
				if( is_present(pdp_table,i))
				{
					if( remove_page_directory(pml4_entry_id , i)==false)
						succeed = false;
				}
			}
		}
			invalidate_entry<PageTable>(pml4,pml4_entry_id);
			cur_pdp_num--;
	}
	return succeed;
}

bool LongModePaging::remove_page_directory_pointer(entry_list pml4_entry )
{
	bool succeed = true;
	for( entry_list_ptr it=pml4_entry.begin();it!=pml4_entry.end();it++)
	{
		if( remove_page_directory_pointer(*it))
			succeed = false;
	}
	return succeed;
}

bool LongModePaging::remove_page_directory(unsigned pml4_entry_id , unsigned pdp_entry_id)
{
	assert(mode!=LongMode_Huge && pml4_entry_id<512 && pdp_entry_id<512);
	PageTable* pdp_table = NULL;
	if( (pdp_table= get_next_level_address<PageTable>(pml4, pml4_entry_id)) )
	{
		PageTable* pd_table = NULL;
		if( (pd_table=get_next_level_address<PageTable>(pdp_table , pdp_entry_id)) )
		{
			if(mode==LongMode_Normal)
			{
				for(unsigned i=0 ; i<ENTRY_512;i++)
					if( is_present(pd_table , i))
						remove_page_table( pml4_entry_id , pdp_entry_id , i);
			}
			invalidate_entry<PageTable>(pdp_table,pdp_entry_id);
			cur_pd_num--;
		}
	}
	return true;
}

bool LongModePaging::remove_page_directory(pair_list high_level_entry)
{
	for( pair_list_ptr it=high_level_entry.begin(); it!=high_level_entry.end();it++)
	{
		remove_page_directory((*it).first,(*it).second );
	}
	return true;
}

bool LongModePaging::remove_page_table(unsigned pml4_entry_id , unsigned pdp_entry_id , unsigned pd_entry_id)
{
	assert(mode==LongMode_Normal);
	PageTable* pdp_table = NULL;
	if( (pdp_table = get_next_level_address<PageTable>(pml4,pml4_entry_id)))
	{
		PageTable* pd_table=NULL;
		if( (pd_table=get_next_level_address<PageTable>(pdp_table,pdp_entry_id)) )
		{
			PageTable* pg_table = NULL;
			if( (pg_table=get_next_level_address<PageTable>(pd_table, pd_entry_id)) )
			{
				invalidate_entry<PageTable>(pd_table , pd_entry_id);
				cur_pt_num--;
			}
		}
	}
	return true;
}

bool LongModePaging::remove_page_table(Address addr , Address size)
{
	assert(mode==LongMode_Normal);
	if( addr&0x1fffff )
	{
		debug_printf("address must align with 2MB");
		return false;
	}
	unsigned pml4_entry , pdp_entry , pd_entry , pt_entry;
	get_domains(addr , pml4_entry , pdp_entry ,pd_entry, pt_entry , mode);
	unsigned page_table_num = (size+0x1fffff)>>21;
	for( unsigned i=0; i<page_table_num ;i++)
		remove_page_table( pml4_entry , pdp_entry , pd_entry+i);
	return true;
}

bool LongModePaging::remove_page_table( triple_list high_level_entry)
{
	for( triple_list_ptr it = high_level_entry.begin(); it!=high_level_entry.end();it++)
		remove_page_table((*it).first,(*it).second,(*it).third);
	return true;
}
