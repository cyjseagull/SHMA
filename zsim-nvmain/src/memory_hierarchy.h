/** $lic$
 * Copyright (C) 2012-2014 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */

#ifndef MEMORY_HIERARCHY_H_
#define MEMORY_HIERARCHY_H_

/* Type and interface definitions of memory hierarchy objects */

#include <stdint.h>
#include "g_std/g_vector.h"
#include "galloc.h"
#include "locks.h"
#include "common/global_const.h"
#include <iostream>
/** TYPES **/

/* Addresses are plain 64-bit uints. This should be kept compatible with PIN addrints */

/* Types of Access. An Access is a request that proceeds from lower to upper
 * levels of the hierarchy (core->l1->l2, etc.)
 */
typedef enum {
    GETS, // get line, exclusive permission not needed (triggered by a processor load)
    GETX, // get line, exclusive permission needed (triggered by a processor store o atomic access)
    PUTS, // clean writeback (lower cache is evicting this line, line was not modified)
    PUTX,  // dirty writeback (lower cache is evicting this line, line was modified)
	EVICTION,
	SETDIRTY,
	WRITEBACK
} AccessType;

/* Types of Invalidation. An Invalidation is a request issued from upper to lower
 * levels of the hierarchy.
 */
typedef enum {
    INV,  // fully invalidate this line
    INVX, // invalidate exclusive access to this line (lower level can still keep a non-exclusive copy)
    FWD,  // don't invalidate, just send up the data (used by directories). Only valid on S lines.
} InvType;

/* Coherence states for the MESI protocol */
typedef enum {
    I, // invalid
    S, // shared (and clean)
    E, // exclusive and clean
    M  // exclusive and dirty
} MESIState;

//Convenience methods for clearer debug traces
const char* AccessTypeName(AccessType t);
const char* InvTypeName(InvType t);
const char* MESIStateName(MESIState s);

/* Memory request */
struct MemReq {
    Address lineAddr;	//line address , virtual address
    AccessType type;	
    uint32_t childId;
    MESIState* state;
    uint64_t cycle; //cycle where request arrives at component

    //Used for race detection/sync
    lock_t* childLock;
    MESIState initialState;

    //Requester id --- used for contention simulation
    uint32_t srcId;

    //Flags propagate across levels, though not to evictions
    //Some other things that can be indicated here: Demand vs prefetch accesses, TLB accesses, etc.
    enum Flag {
        IFETCH        = (1<<1), //For instruction fetches. Purely informative for now, does not imply NOEXCL (but ifetches should be marked NOEXCL)
        NOEXCL        = (1<<2), //Do not give back E on a GETS request (turns MESI protocol into MSI for this line). Used on e.g., ifetches and NUCA.
        NONINCLWB     = (1<<3), //This is a non-inclusive writeback. Do not assume that the line was in the lower level. Used on NUCA (BankDir).
        PUTX_KEEPEXCL = (1<<4), //Non-relinquishing PUTX. On a PUTX, maintain the requestor's E state instead of removing the sharer (i.e., this is a pure writeback)
        PREFETCH      = (1<<5), //Prefetch GETS access. Only set at level where prefetch is issued; handled early in MESICC
    };
    uint32_t flags;
	//Address vpn;
    inline void set(Flag f) {flags |= f;}
    inline bool is (Flag f) const {return flags & f;}
};

/** INTERFACES **/

class AggregateStat;
class Network;

/* Base class for all memory objects (caches and memories) */
class MemObject : public GlobAlloc {
    public:
        //Returns response cycle
        virtual uint64_t access(MemReq& req) = 0;
        virtual void initStats(AggregateStat* parentStat) {}
        virtual const char* getName(){ return NULL; }
};

/*------Base class for all cache objects---------*/
class BaseCache : public MemObject {
    public:
        virtual void setParents(uint32_t _childId, const g_vector<MemObject*>& parents, Network* network) = 0;
        virtual void setChildren(const g_vector<BaseCache*>& children, Network* network) = 0;
        virtual uint64_t invalidate(Address lineAddr, InvType type, bool* reqWriteback, uint64_t reqCycle, uint32_t srcId) = 0;
		virtual uint64_t clflush( Address pAddr, uint64_t curCycle){ return curCycle;}
		virtual uint64_t clflush_all( Address lineAddr, InvType type, bool* reqWriteback, uint64_t reqCycle, uint32_t srcId){ return reqCycle;}
		virtual void calculate_stats(){};
		
};

/*--------Base class for TLB object----------*/
//forward declarition
class BasePageTableWalker;
class BasePageTableWalker;
class BaseTlb: public MemObject{
	public:
		virtual void clear_counter(){ std::cout<<"base clear counter"<<std::endl; }
		//flush all entries of tlb
		virtual bool flush_all() = 0;
		virtual void set_parent(BasePageTableWalker* base_pg_walker) = 0;
		virtual BasePageTableWalker* get_page_table_walker(){ return NULL;};
		virtual uint64_t calculate_stats()=0;
		virtual uint64_t get_access_time(){ return 0; }
			
		virtual ~BaseTlb(){};
};

/*#-----------base class of paging--------------#*/
class PageTable;
class BasePDTEntry;
class BasePaging: public MemObject
{
	public:
		virtual ~BasePaging(){};
		virtual PagingStyle get_paging_style()=0;
		virtual PageTable* get_root_directory()=0;
		virtual Address access(MemReq& req )=0;
		virtual bool unmap_page_table(Address addr)=0;
		virtual int map_page_table(Address addr, void* pg_ptr , bool pbuffer, BasePDTEntry*& mapped_entry){	return 0;	};
		virtual uint64_t remap_page_table( Address ppn,Address dst_ppn,
			bool src_dram, bool dst_dram){return 0; };

		virtual int map_page_table(Address addr, void* pg_ptr , bool pbuffer = false)=0;
		virtual bool allocate_page_table(Address addr , Address size)=0;
		virtual void remove_root_directory()=0;
		virtual bool remove_page_table( Address addr , Address size)
		{ return true; }
		
		virtual void calculate_stats(){}
};

/*--------Base class for PageTableWalker object----------*/
class BasePageTableWalker: public MemObject
{
	public:
		//virtual bool add_child(const char* child_name , BaseTlb* tlb)=0;
		virtual void write_through( MemReq& req){}
		virtual BasePaging* GetPaging(){ return NULL;}
		virtual void SetPaging(uint32_t proc_id , BasePaging* copied_paging){}
		virtual void convert_to_dirty( Address block_id){}
		virtual void calculate_stats(){}
};

/*--------Base class for DRAM buffer Allocator---------*/
class DRAMBufferBlock;
class BaseDRAMBufferManager: public GlobAlloc
{
	public:
		virtual ~BaseDRAMBufferManager(){};
		virtual DRAMBufferBlock* allocate_one_page( uint32_t proc = INVALID_PROC) { return NULL; }
		virtual double get_memory_usage(){ return 0.0;};
		virtual void SetBlockDirty( Address block_id){};
		virtual void convert_to_dirty( unsigned process_id , Address block_id){}
		virtual void evict( DRAMEVICTSTYLE evict_policy){}
		virtual bool should_reclaim(){ return false; }
		virtual bool should_cherish(){ return false;}
		virtual bool should_more_cherish(){ return false;}
		virtual DRAMBufferBlock* get_page_ptr( uint64_t entry_id )=0;
};

class BasePDTEntry;
struct Content: public GlobAlloc
{
  public:
	Content( BasePDTEntry* entry=NULL, Address page_no=0):
		  pgt_entry(entry), vpn(page_no){}
	
	Address get_vpn(){ return vpn; }
	BasePDTEntry* get_pgt_entry(){	return pgt_entry; }

  private:
	Content( Content& forbid_copy){};
	BasePDTEntry* pgt_entry;
	Address vpn;
};
#endif  // MEMORY_HIERARCHY_H_
