#ifndef COMMON_PAGE_TABLE_OP_H
#define COMMON_PAGE_TABLE_OP_H
#include "page-table/page_table_entry.h"
#include "common/global_const.h"
#include "memory_hierarchy.h"
#include "MMU/page.h"
#include "tlb/common_func.h"
#include "DRAM-buffer/DRAM_buffer_block.h"
inline bool is_present( PageTable* table , unsigned entry_id)
{
	return table->is_present(entry_id);
}

inline bool point_to_buffer_table( PageTable* table , unsigned entry_id)
{
	return ((*table)[entry_id])->point_buffer_page();
}


template<class T>
T* get_next_level_address(PageTable* table , unsigned entry_id)
{
	return (T*)((*table)[entry_id])->get_next_level_address();
}

template<class T>
inline void set_next_level_address(PageTable* &table , unsigned entry_id , T* next_level_ptr)
{
	( (*table)[entry_id])->set_next_level_address((void*)next_level_ptr);
}

template<class T>
inline void validate_entry(PageTable* table , unsigned entry_id , T* next_level_addr , bool map_to_buffer=false )
{
	//std::cout<<std::hex<<(*table)[entry_id]<<std::endl;
	(*table)[entry_id]->validate((void*)next_level_addr , map_to_buffer);
	if( map_to_buffer==false )
		(*table)[entry_id]->set_buffer(false);
}

inline bool is_valid(PageTable* table , unsigned entry_id )
{
	return (*table)[entry_id]->is_present();
}

inline void invalidate_page( PageTable* table , unsigned entry_id)
{
	if( is_present(table,entry_id) )
		(*table)[entry_id]->invalidate_page();
}

template<class T>
inline void invalidate_entry(PageTable* table , unsigned entry_id)
{
	((*table)[entry_id])->invalidate_page_table<T>();	
}

inline unsigned get_pagetable_off( Address addr , PagingStyle mode )
{
	switch( mode )
	{
		case Legacy_Normal:
			return get_bit_value<unsigned>(addr,12,21);
		case PAE_Normal:
			return get_bit_value<unsigned>(addr , 21,20);
		case LongMode_Normal:
			return get_bit_value<unsigned>(addr,12,20);
		default:
			return (unsigned)(-1);
	}
}

inline unsigned get_buffer_table_off( Address addr , unsigned buffer_shift , PagingStyle mode)
{
	return (unsigned)(0);
}
inline unsigned get_page_directory_off( Address addr , PagingStyle mode)
{
	unsigned index;
	if( mode==Legacy_Normal || mode==Legacy_Huge)
		index = get_bit_value<unsigned>(addr , 22, 31);
	else if( mode==PAE_Normal || mode== PAE_Huge ||
			mode == LongMode_Normal || mode==LongMode_Middle)
		index = get_bit_value<unsigned>(addr,21,29);
	else
		index = (unsigned)(-1);
	return index;
}

inline unsigned get_page_directory_pointer_off( Address addr , PagingStyle mode)
{
	if( mode==PAE_Normal || mode == PAE_Huge)
		return get_bit_value<unsigned>(addr , 30,31);
	else if( mode==LongMode_Normal || mode==LongMode_Middle || mode==LongMode_Huge)
		return get_bit_value<unsigned>( addr , 30,38);
	else
		return (unsigned)(-1);
}

inline unsigned get_pml4_off( Address addr , PagingStyle mode)
{
	if( mode== LongMode_Normal || mode==LongMode_Middle || mode==LongMode_Huge)
		return get_bit_value<unsigned>(addr , 39,47);
	else
		return (unsigned)(-1);
}

inline void get_domains( Address addr , unsigned &pml4 , unsigned &pdp , unsigned &pd , unsigned &pt, PagingStyle mode)
{
	pml4=(unsigned)(-1);
	pdp =(unsigned)(-1);
	pd = (unsigned)(-1);
	pt = (unsigned)(-1);
	switch(mode)
	{
		case Legacy_Normal:
			pd  = get_bit_value<unsigned>(addr , 22, 31);
			pt = get_bit_value<unsigned>(addr,12,21);
			break;
		case Legacy_Huge:
			pd = get_bit_value<unsigned>(addr,22,31);
			break;
		case PAE_Normal:
			pt = get_bit_value<unsigned>(addr , 12, 20);
			pd = get_bit_value<unsigned>(addr,21,29);
			pdp = get_bit_value<unsigned>(addr,30,31);
			break;
		case PAE_Huge:
			pd = get_bit_value<unsigned>(addr , 21, 29);
			pdp = get_bit_value<unsigned>(addr , 30,31);
			break;
		case LongMode_Normal:
			pt = get_bit_value<unsigned>(addr,12,20);
			pd = get_bit_value<unsigned>(addr , 21,29);
			pdp = get_bit_value<unsigned>(addr , 30,38);
			pml4 = get_bit_value<unsigned>(addr,39,47);
			break;
		case LongMode_Middle:
			pd = get_bit_value<unsigned>(addr , 21,29);
			pdp = get_bit_value<unsigned>(addr , 30,38);
			pml4 = get_bit_value<unsigned>(addr,39,47);
			break;
		case LongMode_Huge:
			pdp = get_bit_value<unsigned>(addr , 30,38);
			pml4 = get_bit_value<unsigned>(addr,39,47);
			break;
	}
}


inline void extend_one_buffer_map(Address addr , PageTable* table ,void* block_ptr, unsigned pt_entry_id , unsigned buffer_entry_id, unsigned buffer_table_entry_num , BasePDTEntry*& mapped_entry)
{
	void* next_level_addr = get_next_level_address<void>(table ,pt_entry_id);
	PageTable* buffer_table = NULL;
	mapped_entry = NULL;
	if( !point_to_buffer_table( table , pt_entry_id ) )
	{
		buffer_table = new PageTable( buffer_table_entry_num);
		validate_entry(table , pt_entry_id , buffer_table, true);
		if( !is_valid(buffer_table, buffer_entry_id))		
			mapped_entry = (*buffer_table)[buffer_entry_id];

		validate_entry( buffer_table,buffer_entry_id,block_ptr,true);
	}
	else
	{
		if( !is_valid((PageTable*)next_level_addr, buffer_entry_id) )
			mapped_entry = (*(PageTable*)next_level_addr)[buffer_entry_id];
		validate_entry((PageTable*)next_level_addr ,buffer_entry_id,block_ptr,true);
	}
}


inline Address get_block_id(MemReq& req ,PageTable* pgt, void* pblock , unsigned pg_id,
							PagingStyle mode ,bool pbuffer ,  bool set_dirty, 
							bool write_back, uint32_t  access_counter)
{
	if(!pblock)
		return INVALID_PAGE_ADDR;
	else
	{
		if( pbuffer)
		{
			//unsigned buffer_off = get_buffer_table_off(addr, buffer_table_shift,mode);
			//point to DRAM buffer block
			//DRAMBufferBlock* buffer_block = get_next_level_address<DRAMBufferBlock>
			//								( (PageTable*)pblock , buffer_off);
			DRAMBufferBlock* buffer_block = (DRAMBufferBlock*)pblock;
			if( set_dirty )
			{
				//((PageTable*)pblock)->entry_array[buffer_off]->set_dirty();
				//set buffer block to dirty
				//std::cout<<"set dirty"<<std::endl;
				assert(pgt);
				pgt->entry_array[pg_id]->set_dirty();
				buffer_block->set_dirty();
			}
			//req.lineAddr = block_id_to_addr(buffer_block->block_id)>>(zinfo->page_shift);
			//return buffer_block->get_src_addr();
			//std::cout<<"ppn:"<<std::hex<<(block_id_to_addr(buffer_block->block_id)>>(zinfo->page_shift))<<std::endl;
			req.childId = buffer_block->is_dirty();
			return block_id_to_addr(buffer_block->block_id)>>(zinfo->page_shift);
		}
		else
		{
			if( write_back)
			{
				//overlap field
				((Page*)pblock)->set_overlap(access_counter );
			}
			req.childId = ((Page*)pblock)->get_overlap();
			//std::cout<<"ppn2:"<<(((Page*)pblock)->pageNo)<<std::endl;
			return ((Page*)pblock)->pageNo;
		}
	}
}

/*
inline Address get_block_id(MemReq& req , Address addr , void* pblock ,
							unsigned buffer_table_shift, PagingStyle mode ,
							bool pbuffer ,  bool set_dirty , 
							bool write_back, uint32_t  access_counter)
{
	if(!pblock)
		return INVALID_PAGE_ADDR;
	else
	{
		if( pbuffer)
		{
			//unsigned buffer_off = get_buffer_table_off(addr, buffer_table_shift,mode);
			//point to DRAM buffer block
			//DRAMBufferBlock* buffer_block = get_next_level_address<DRAMBufferBlock>
			//								( (PageTable*)pblock , buffer_off);
			DRAMBufferBlock* buffer_block = (DRAMBufferBlock*)pblock;
			if( set_dirty )
			{
				//((PageTable*)pblock)->entry_array[buffer_off]->set_dirty();
				//set buffer block to dirty
				buffer_block->set_dirty();
			}
			//req.lineAddr = block_id_to_addr(buffer_block->block_id)>>(zinfo->page_shift);
			//return buffer_block->get_src_addr();
			std::cout<<"ppn:"<<std::hex<<(block_id_to_addr(buffer_block->block_id)>>(zinfo->page_shift))<<std::endl;
			return block_id_to_addr(buffer_block->block_id)>>(zinfo->page_shift);
		}
		else
		{
			if( write_back)
			{
				//overlap field
				((Page*)pblock)->set_overlap(access_counter );
			}
			req.childId = ((Page*)pblock)->get_overlap();
			std::cout<<"ppn2:"<<(((Page*)pblock)->pageNo)<<std::endl;
			return ((Page*)pblock)->pageNo;
		}
	}
}
*/
#endif
