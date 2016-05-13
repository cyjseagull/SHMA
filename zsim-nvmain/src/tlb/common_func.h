#ifndef _COMMON_FUNC_H_
#define _COMMON_FUNC_H_

#include "zsim.h"
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
#endif
