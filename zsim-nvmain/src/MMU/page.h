/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef _PAGE_H_
#define _PAGE_H_
/*------basic descriptor for page--------*/
//forward declarition
class Zone;
class MemoryNode;
struct Page
{
		Page(uint64_t page_no):count(0),pageNo(page_no),
								map_count(-1),next(NULL),overlap(0)
		{}
		void inc_reference()
		{	count++;	}
		//set zone for page
		void set_page_zone(Zone* zone)
		{
			this->zone = zone;
		}
		//set node for page
		void set_page_node( MemoryNode* node)
		{
			this->node = node;
		}

		void set_overlap (unsigned access_counter)
		{
			overlap = access_counter;
		}

		unsigned get_overlap( )
		{
			return overlap;
		}

		unsigned count;	//how many processes this page mapped to
		Address pageNo;	//page no
		int	map_count;
		Zone* zone;
		MemoryNode* node;
		Page* next;
		unsigned overlap;
		unsigned private_;	//order in buddy system
};

#endif

