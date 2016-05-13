/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef TLB_FACTORY_H
#define TLB_FACTORY_H

#include "tlb/common_tlb.h"
#include "tlb/hot_monitor_tlb.h"
#include "tlb/tlb_entry.h"
#include "memory_hierarchy.h"
#include "common/global_const.h"

class TlbFactory
{
	public:
	static BaseTlb* CreateTlb( std::string tlb_type , const char *name , unsigned tlb_entry_num , unsigned hit_lat , unsigned response_lat , EVICTSTYLE evict_policy)
	{
		if( tlb_type =="CommonTlb")
		{	
			return (new CommonTlb<TlbEntry>( name , tlb_entry_num , hit_lat , response_lat, evict_policy));
		}
		else if( tlb_type == "HotMonitorTlb")
		{
			return (new HotMonitorTlb<ExtendTlbEntry>(name,tlb_entry_num,hit_lat,response_lat,evict_policy));
		}
		return (new CommonTlb<TlbEntry>( name , tlb_entry_num , hit_lat , response_lat, evict_policy));
	}
};
#endif
