#ifndef _COMMON_FUNC_H_
#define _COMMON_FUNC_H_

#include "zsim.h"
#include "src/memory_hierarchy.h"
inline Address block_id_to_addr(Address block_id)
{
	Address addr = (zinfo->high_addr+(block_id<<(zinfo->block_shift)));
	debug_printf("addr of dram is %llx",addr);
	return addr;
}

inline Address dram_addr_to_block_id(Address dram_addr)
{
	return ((dram_addr-zinfo->high_addr)>>zinfo->block_shift);
}

inline void set_block_dirty(Address dram_addr)
{
	Address block_id = dram_addr_to_block_id(dram_addr);
	zinfo->dram_manager->SetBlockDirty(block_id);
}

inline bool is_dram_address( Address paddr )
{
	return (paddr>=zinfo->high_addr)?true:false;
}

inline uint64_t remap_page_table( Address ppn, Address dst_ppn, bool src_dram, bool dst_dram)
{
	uint64_t overhead = 0;
	uint64_t max_overhead = 0;
	for( uint32_t i=0; i<zinfo->numProcs; i++)
	{
		overhead = zinfo->paging_array[i]->remap_page_table(
							ppn, dst_ppn, src_dram, dst_dram);
		max_overhead = (max_overhead > overhead ? max_overhead:overhead);
	}
	return max_overhead;
}
#endif
