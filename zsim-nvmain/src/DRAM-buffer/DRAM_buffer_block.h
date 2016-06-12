#ifndef DRAM_BUFFER_BLOCK
#define DRAM_BUFFER_BLOCK
#include "galloc.h"
struct DRAMBufferBlock: public GlobAlloc
{
	//uint32_t block_size;
	uint16_t flag;
	unsigned hotness;
	unsigned max_queue;
	Address block_id;
	Address original_addr;
	Address vaddr;
	DRAMBufferBlock* next;
	DRAMBufferBlock( Address id):block_id(id)
	{}

	DRAMBufferBlock(Address id , unsigned max_count):flag(0),hotness(max_count),max_queue(max_count),block_id(id),next(NULL)
	{}
	bool is_occupied()
	{	return (flag & USED);	}
	bool is_dirty()
	{	return (flag &DIRTY);	}

	void set_dirty( )
	{	flag |=DIRTY;			}
	
	void validate(Address addr)
	{
		assert( is_occupied()==false );
		set_used();
		original_addr = addr;
	}

	void invalidate()
	{
		flag = 0;
		hotness = max_queue;
	}
	
	Address get_src_addr()
	{
		return original_addr;
	}
	void set_src_addr( Address addr)
	{
		original_addr = addr;
	}

	void set_vaddr( Address addr)
	{
		vaddr = addr;
	}
	Address get_vaddr()
	{
		return vaddr;
	}

protected:
	void set_used()
	{	flag |= USED;			}
};
#endif
