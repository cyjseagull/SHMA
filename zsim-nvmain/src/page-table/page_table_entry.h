/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef PAGE_TABLE_ENTRY_H
#define PAGE_TABLE_ENTRY_H
#include <vector>
#include "common/global_const.h"
#include "common/common_functions.h"
#include "g_std/g_vector.h"
#include "locks.h"
#include "galloc.h"
#include "pad.h"
/*-----commmon flag of page table entry related----*/
/*----basic page table entry mapping maintaince-----*/
/*
 *@function: simulation of page directory entry,
 *			 such as PDE,PDPE-PAE,PDE-PAE,PML4E-Long Mode
 *			 PDPE-Long Mode,PDE-Long Mode,and PTE
 */
class BasePDTEntry: public GlobAlloc
{
	public:
		BasePDTEntry()
		{ 
			entry_bits = 0;
			next_level_ptr = NULL; 
			set_read_write(); //all page can read and write both
			set_user();	//default user can access all page
			//default is cache write back,cacheable,unaccessed,clear,page_size is 4KB, local page
		}
		BasePDTEntry(void* ppt):next_level_ptr(ppt)
		{ entry_bits = 0;}
		virtual ~BasePDTEntry(){};
	
		void validate( void* next_level_addr , bool is_buffer = false )
		{
			entry_bits |= P;
			set_buffer(is_buffer);
			next_level_ptr = next_level_addr;
		}	
		void invalidate_page()	
		{
			assert(is_present());
			entry_bits=0;
			set_user();
			set_read_write();
			next_level_ptr = NULL;
		}

		template<class T>
		void invalidate_page_table()
		{
			T* next_level_table=(T*)next_level_ptr; 
			if( is_present() )
			{
				assert(next_level_ptr);
				delete next_level_table;
				next_level_ptr = NULL;
			}
			assert(is_present());
			entry_bits=0;
			set_user();
			set_read_write();
		}

		void set_next_level_address(void* ppt)
		{
			next_level_ptr = ppt;
		}

		void* get_next_level_address()
		{
			return next_level_ptr;  
		}

	//---get information through entry_bits ---
	//page present in memory
	bool is_present() 
	{	return (entry_bits)&P;	}

	//page read only 
	bool read_only()
	{	return !(entry_bits&RW);	}

	//can write page
	bool can_write()
	{	return !(entry_bits & RW);	}

	//can access page in user-mode
	bool user_can_access()
	{	return (entry_bits&PERMISSION); }
	
	//cache write through data of pages
	bool page_write_through()
	{	return (entry_bits&PWT);	}
	
	bool page_cache_disable()
	{	return (entry_bits&PCD);	}
	
	//page is accessed?
	bool is_accessed()
	{	return (entry_bits & A);	}

	//page table is dirty
	bool is_dirty()	
	{	return (entry_bits&D);	}
	
	//page size is 4KB?
	bool is_4KB() 
	{	return !(entry_bits &PS);	}

	//page size is 2MB/4MB/1GB
	bool is_largepage()
	{	return (entry_bits&PS);		}
	
	//page is global
	bool is_global()
	{	return (entry_bits&G);	}
	bool point_buffer_page()
	{	return (entry_bits&PBUFFER); }
	
	//----set entry_bits ----
	void set_buffer( bool buffer_block)
	{
		if( buffer_block)
		{
			entry_bits |= PBUFFER;
		}
		else
		{
			entry_bits &= (~PBUFFER);
		}
	}

	//set page read only through setting RW
	void set_read_only()
	{
		entry_bits &=(~RW);		
	}

	void set_read_write()
	{
		entry_bits |= RW;
	}
	
	void set_supervisor()
	{
		entry_bits &= (~PERMISSION);
	}

	void set_user()
	{
		entry_bits |= PERMISSION;
	}
	//cache write through
	void set_pwt()
	{
		entry_bits |= PWT; 
	}
	//cache write back
	void set_cache_writeback()
	{
		entry_bits &=(~PWT);
	}

	//table or physical page is uncacheable
	void set_uncacheable()
	{
		entry_bits |= PCD;	
	}
	//default, cacheable
	void set_cacheable()
	{	entry_bits &=(~PCD);	}

	void set_accessed()
	{	entry_bits |=A;	}
	
	void set_unaccessed()
	{	entry_bits &= (~A);		}
	
	void set_dirty()
	{	entry_bits |=D;		}

	void clear_dirty() 
	{	entry_bits &=(~D); }
	
	//ps is 0,4KB page
	void enable_4KB()
	{	entry_bits &=(~PS);	}
	//ps is 1, large page	
	void enable_large_page() 
	{	entry_bits |= PS;	}
	//G=1,global
	void set_page_global()
	{	entry_bits |= G;	}
	void set_page_local()
	{	entry_bits &= (~G);	}
	
	public:
		void* next_level_ptr;
		unsigned entry_bits;
};

/*---------structure of page table--------*/
class PageTable 
{
public:
	PageTable(uint64_t size )
	{
		assert(size>0);
		//init page table with page table entry
		map_count = size;
		entry_array.resize(map_count,NULL);
		BasePDTEntry* pg_table = gm_memalign<BasePDTEntry>(CACHE_LINE_BYTES, size);
		for( uint64_t i=0 ; i<size; i++)
		{
			entry_array[i] = new (&pg_table[i]) BasePDTEntry();
		}
	}

	~PageTable()
	{
	}

	BasePDTEntry* operator[](unsigned entry_id)
	{
		assert( entry_id < map_count);
		BasePDTEntry* tmp = entry_array[entry_id];
		return tmp; 
	}
	
	bool is_present(unsigned entry_id)
	{
		bool present = entry_array[entry_id]->is_present(); 
		return present;
	}
	
	//template<class U, class S>
	void* get_next_level_address(unsigned entry_id)
	{
		if( entry_id < map_count)
		{
			void* ptr = (entry_array[entry_id])->next_level_ptr;
			return ptr;
		}
		return NULL;
	}

	void enable_large_page()
	{
	   for( unsigned i=0;i < map_count;i++)
			entry_array[i]->enable_large_page();
	}

	public:
		g_vector<BasePDTEntry*> entry_array;
		unsigned map_count;
};
#endif

