/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef TLB_ENTRY_H_
#define TLB_ENTRY_H_
#include "common/global_const.h"
#include "zsim.h"
#include "galloc.h"
/*---------------TLB entry related-------------*/
class BaseTlbEntry: public GlobAlloc
{
	public: 
		//32bit in none-PAE, 52bit in PAE
		Address v_page_no;
		Address p_page_no;
		uint16_t flag;
		uint64_t lru_seq;	//keep track of LRU seq
		BaseTlbEntry( Address vpn ,Address ppn ):v_page_no(vpn),p_page_no(ppn),flag(0),lru_seq(0)
		{}
		BaseTlbEntry()
		{}
		
		virtual void operator = ( BaseTlbEntry &target_tlb) 
		{
			v_page_no = target_tlb.v_page_no;
			p_page_no = target_tlb.p_page_no;
			flag = target_tlb.flag;
		}

		virtual ~BaseTlbEntry(){}

		bool is_page_global() 
		{ return ( flag & GLOBAL ); }
		bool is_page_cacheable()
		{ return !( flag & UNCACHEABLE );	}

		bool PAT_enabled()
		{ return ( flag & PAT);	}
	
		void set_page_global( bool select)
		{ 
		   if( select )
				flag |= GLOBAL;
		   else
				flag &= (~GLOBAL);}

		void set_page_cacheable(bool select)
		{
			if( select )
				flag &= (~UNCACHEABLE);
			else
				flag |= UNCACHEABLE;
		}

		void enable_PAT(bool select)
		{
			if(select)
				flag |= PAT;
			else
				flag &= (~PAT);
		}

		bool is_dirty( )
		{
			return (flag&DIRTY); 
		}
		
		void set_dirty()
		{
			flag |= DIRTY;
		}
		bool is_valid()
		{
			return (flag&VALID);
		}

		void set_valid()
		{
			flag |= VALID;
		}
		void set_clean()
		{
			flag &=(~DIRTY);
		}
		void set_invalid()
		{
			flag = 0;
		}

		virtual void map( Address vpn , Address ppn )
		{
			v_page_no = vpn;
			p_page_no = ppn;
		}

	   virtual Address get_counter()
	   {	return INVALID_PAGE_ADDR;	}
		
	   virtual bool is_in_dram()
	   {	return false;	}	

	   virtual void clear_counter(){}
};

//forward declaration
class TlbEntry;
class TlbEntry: public BaseTlbEntry
{
	public:
	TlbEntry( Address vpn,Address ppn , bool is_dram=false):BaseTlbEntry(vpn ,ppn)
	 {}
	TlbEntry():BaseTlbEntry()
	{}
	//update virtual address
	void update_vpn( Address new_vpn)
	{	v_page_no = new_vpn;	}

	virtual bool is_in_dram()
	{
		return false;
	}

	void update_ppn( Address new_ppn )
	{	p_page_no = new_ppn;	}

	void remap( Address ppn , bool is_dram)
	{}
};


class ExtendTlbEntry;
class ExtendTlbEntry: public BaseTlbEntry
{
	public:
		Address access_counter;
		unsigned life_time;
		//static unsigned counter_len; 
		ExtendTlbEntry():BaseTlbEntry()
		{}
		ExtendTlbEntry( Address vpn , Address ppn , bool is_dram = false):BaseTlbEntry(vpn ,ppn),access_counter(0)
		{
			set_in_dram( is_dram);
		}

		virtual void operator = ( BaseTlbEntry &target_tlb) 
		{
			ExtendTlbEntry* tlb = dynamic_cast<ExtendTlbEntry*>(&target_tlb);
			access_counter = tlb->access_counter;
			life_time = tlb->life_time;	
			v_page_no = tlb->v_page_no;
			p_page_no = tlb->p_page_no;
			flag = tlb->flag;
		}

		void remap( Address ppn , bool is_dram)
		{
			p_page_no = ppn;
			access_counter = 0;
			set_in_dram(is_dram);	
		}

		bool is_in_dram()
		{  return flag&DRAMVALID; }
		
		/*
		 *@function: increase access counter of when accessing PCM main memory according to access type( read/write )
		 *@param is_write: true: PCM main memory access type is write; false: PCM main memory access type is read
		 *@return: true: access counter over threshold,should cache corresponding PCM page to DRAM page;false:access counter is in the range of threshold,should not cache PCM page
		 */
		TLBSearchResult incre_counter( bool is_write , unsigned write_incre_step , unsigned read_incre_step , unsigned access_threshold)	
		{
			debug_printf("access counter: %d , is_in dram: %d",access_counter,is_in_dram());
			
			if( !is_in_dram())
			{
				if( zinfo->multi_queue)
					life_time = zinfo->life_time;
				if( is_write )
					access_counter += write_incre_step;
				else
				   	access_counter += read_incre_step;
				//std::cout<<"access_counter:"<<access_counter<<std::endl;
				 if( access_counter >= access_threshold)
					return OverThres;
				 else 
					 return unOverThres;
			}
			return InDRAM;
		}

	   void decre_lifetime()
	   {
		   assert( zinfo->multi_queue);
		   life_time--;
		   //demote to lower queue
		   if( life_time == 0)
		   {
				access_counter >>= 2;
				life_time = zinfo->life_time;
		   }
	   }

	   void clear_counter()
	   {
		   access_counter = 0;
	   }

	   void set_counter( Address counter)
	   {
		   //std::cout<<"set counter:"<<counter<<std::endl;
			access_counter = counter;
	   }

	   virtual Address get_counter()
	   {
		   return access_counter;	
	   }
	
	   //update virtual address
	   void update_vpn( Address new_vpn)
	   {	v_page_no = new_vpn;	}

	   void update_ppn( Address new_ppn )
	   {	p_page_no = new_ppn;	}
		
	   void set_in_dram( bool is_dram )
	   {
			if( is_dram )
				flag |= DRAMVALID;
			else
				flag &= (~DRAMVALID);
			set_clean();
		}
};
#endif
