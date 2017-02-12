/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef _PAGE_TABLE_H
#define _PAGE_TABLE_H
#include <map>
#include <iterator>
#include "common/global_const.h"
#include "common/common_functions.h"
#include "page-table/comm_page_table_op.h"
#include "page-table/page_table_entry.h"
#include "memory_hierarchy.h"
#include "locks.h"
/*#------legacy paging(supports 4KB&&4MB)--------#*/
class NormalPaging: public BasePaging
{
	public:
		NormalPaging(PagingStyle select);
		~NormalPaging();

		int map_page_table( Address addr , void* pg_ptr , bool pbuffer,BasePDTEntry*& mapped_entry);
		int map_page_table( Address addr , void* pg_ptr , bool pbuffer=false);
		bool unmap_page_table( Address addr );	
		//access
		Address access( MemReq& req );
		void remove_root_directory();
	   //allocate page table for [addr , addr+size]
		bool allocate_page_table(Address addr , Address size );

		bool remove_page_table(Address addr,Address size);

		PagingStyle get_paging_style()
		{
			return mode;
		}
		unsigned get_page_table_num()	
		{ return cur_page_table_num; }

		PageTable* get_root_directory()
		{   return page_directory;	}
	
		void calculate_stats()
		{
			long unsigned overhead = (long unsigned)cur_page_table_num*1024;
			info("page table number: %d", cur_page_table_num);
			info("overhead of extended storage:%f MB", (double)overhead/(double)(1024*1024));
		}
	protected:
		PageTable* allocate_one_pagetable(unsigned pd_entry_id, int& allocate_time );
		bool allocate_page_table(entry_list pd_entry);
		//allocate page table for [addr , addr+size]
		bool remove_page_table(entry_list pd_entry);
		bool remove_page_table(unsigned pd_entry_id);
	public:
	    PageTable* page_directory;	//page directory table
	private:
		PagingStyle mode;
		unsigned cur_page_table_num;
		unsigned buffer_table_entry_num;
		unsigned buffer_table_shift;
		BaseTlb* itlb;
		BaseTlb* dtlb; 
		static lock_t table_lock;
};

/*#----Legacy-PAE-Paging(supports 4KB&&2MB)-----#*/
class PAEPaging: public BasePaging
{
	public:
		PAEPaging( PagingStyle select);
		~PAEPaging();
		//access
		Address access(MemReq& req );
		int map_page_table( Address addr , void* pg_ptr , bool pbuffer, BasePDTEntry*&  mapped_entry);
		int map_page_table( Address addr , void* pg_ptr , bool pbuffer = false);
		bool unmap_page_table( Address addr );		

		//PAE-Huge mode
		bool allocate_pdt(Address addr , Address size);

		bool allocate_page_table(Address addr , Address size);

		void remove_root_directory();	//remove whole page table structures
		bool remove_pdt( Address addr , Address size);

		bool remove_page_table( Address addr, Address size);
		PagingStyle get_paging_style()
		{
			return mode;
		}

		unsigned get_page_table_num()
		{  return cur_pt_num;	}

		unsigned get_pdt_num()
		{ return cur_pdt_num;	}

		PageTable* get_root_directory()
		{  return page_directory_pointer;	}
		
		void calculate_stats()
		{
			long unsigned overhead = (long unsigned)cur_pt_num*512;

			info("page table number: %d", cur_pt_num);
			info("overhead of extended storage:%f MB", (double)overhead/(double)(1024*1024));
		}

	protected:
	   //allocate page directory table,at most to 4 
		bool allocate_pdt(entry_list pdpt_entry);
		PageTable* allocate_pdt(unsigned pdpt_entry_id, int& alloc_time);


		//allocate page table , at most to 512
		bool allocate_page_table(pair_list pdp_entry);
		PageTable* allocate_page_table(unsigned pdpt_entry_id ,
				unsigned pdp_entry_id, int& alloc_time);

		bool remove_pdt( entry_list pdpt_entry);
		bool remove_pdt( unsigned pdp_entry_id);

		bool remove_page_table(pair_list pdp_entry);
		bool remove_page_table(unsigned pdp_entry_id, unsigned pd_entry_id);
	private:
		PagingStyle mode;
		PageTable* page_directory_pointer;
		unsigned cur_pdt_num;  //current page directory table num
		unsigned cur_pt_num;  //current page table num

		unsigned buffer_table_entry_num;
		unsigned buffer_table_shift;
};


/*#----LongMode-Paging(supports 4KB&&2MB&&1GB)---#*/
class LongModePaging: public BasePaging
{
	public:
		LongModePaging(PagingStyle selection);
		~LongModePaging();
		int map_page_table( Address addr , void* pg_ptr , bool pbuffer, BasePDTEntry*& mapped_entry);
		int map_page_table( Address addr , void* pg_ptr , bool pbuffer=false);
		bool unmap_page_table( Address addr );		
		//access
		Address access(MemReq& req);
		bool allocate_page_table(Address addr , Address size);
		void remove_root_directory();
		bool remove_page_table( Address addr , Address size);

		PagingStyle get_paging_style()
		{ return mode;			}
	
		unsigned get_page_table_num()
		{ return cur_pt_num;	}

		unsigned get_page_directory_num()
		{ return cur_pd_num;	}

		unsigned get_page_directory_pointer_num()
		{ return cur_pdp_num;	}
		
		PageTable* get_root_directory()
		{ return pml4;		}

		void calculate_stats()
		{
			long unsigned overhead = (long unsigned)cur_pt_num*512;
			info("page table number: %d", cur_pt_num);
			info("overhead of extended storage:%f MB", (double)overhead/(double)(1024*1024));
		}
	protected:
		//allocate multiple
		PageTable* allocate_page_directory_pointer( unsigned pml4_entry_id, int& alloc_time);
		bool allocate_page_directory_pointer(entry_list pml4_entry);
		
		PageTable* allocate_page_directory( unsigned pml4_entry_id , unsigned pdpt_entry_id, int& alloc_time);
		bool allocate_page_directory(pair_list high_level_entry);

		PageTable* allocate_page_table( unsigned pml4_entry_id , 
				unsigned pdpt_entry_id , unsigned pdt_entry_id, int& alloc_time);
		bool allocate_page_table(triple_list high_level_entry);

		//remove
		bool remove_page_directory_pointer(unsigned pml4_entry_id );
		bool remove_page_directory_pointer(entry_list pml4_entry);
		bool remove_page_directory(unsigned pml4_entry_id , unsigned pdp_entry_id);
		bool remove_page_directory(pair_list high_level_entry);
		bool remove_page_table(unsigned pml4_entry_id , unsigned pdp_entry_id , unsigned pd_entry_id);
		inline bool remove_page_table(triple_list high_level_entry);
	
		PageTable* get_tables(unsigned level , std::vector<unsigned> entry_id_vec);
	public:
		PageTable* pml4;
	private:
		PagingStyle mode;
		//number of page directory pointer at most 512
		unsigned cur_pdp_num;
		uint32_t cur_pd_num;
		uint32_t cur_pt_num;
		
		unsigned buffer_table_entry_num;
		unsigned buffer_table_shift;
		static lock_t table_lock;
};

class PagingFactory
{
	public:
		PagingFactory()
		{}
		static BasePaging* CreatePaging( PagingStyle mode)
		{
			std::string mode_str = pagingmode_to_string(mode);
			debug_printf("paging mode is "+mode_str);
			if( mode_str == "Legacy" )
				return (new NormalPaging(mode));
			else if( mode_str == "PAE")
				return (new PAEPaging(mode));	
			else if( mode_str == "LongMode")
				return (new LongModePaging(mode));
			else 
				return (new NormalPaging(Legacy_Normal));	//default is legacy paging
		}
};
#endif
