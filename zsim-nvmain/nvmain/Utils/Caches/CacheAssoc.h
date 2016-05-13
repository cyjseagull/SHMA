/***********************************
 *
 *add on 2015/3/24
 *
 * *********************************/
#ifndef __CACHE_ASSOC_H__
#define __CACHE_ASSOC_H__
#include <utility>
#include "src/NVMObject.h"
#include "include/NVMAddress.h"
#include "include/NVMDataBlock.h"
#include "include/Exception.h"
#include "include/CommonEnums.h"
#include "include/CommonMath.h"
#include "include/NVMainRequest.h"
#include "src/EventQueue.h"
#include "include/CacheSet.hh"
#include "include/CacheBlock.hh"
namespace NVM{
/*
 * common interface of cache
 */
class BaseCache:public NVMObject
{
	public:
		//invalidate a specified cache block
		//virtual void Invalidate( CacheBlock* blk)=0;
		//find block according to adddress
		virtual CacheBlock* FindBlock( NVMAddress* addr)=0;
		//find victim block
		virtual CacheBlock* FindVictim(NVMAddress* addr)=0;

//		virtual bool Read( NVMAddress* addr , NVMDataBlock* data)=0;
//		virtual bool Write(NVMAddress* addr , NVMDataBlock* data)=0;
		//virtual bool Update( NVMAddress *addr , NVMDataBlock* data)=0;
	
};

class CacheImpl: public NVMObject
{
	public:
			CacheImpl(uint64_t assoc ,uint64_t set , uint64_t cache_line_size , uint64_t read_t = 1 , uint64_t write_t = 1 );
			virtual ~CacheImpl();
			
			//invalidate a specified cache block
			//virtual void Invalidate( CacheBlock* blk);
			//find block according to adddress
			virtual CacheBlock* FindBlock( NVMAddress* addr ,uint64_t &set_id, uint64_t &way_id);
			virtual CacheBlock* FindBlock( NVMAddress* addr );
			//find victim block
			virtual CacheBlock* FindVictim(NVMAddress* addr);
			virtual CacheBlock* FindVictim(NVMAddress* addr , uint64_t &set_id , uint64_t &way_id);

			//evict block
			virtual bool Evict( NVMAddress *addr);
			virtual bool Evict( CacheBlock *blk);

			virtual void Install( NVMAddress* install_addr , CacheBlock* victim_blk  , NVMDataBlock* data);

			virtual bool IssueCommand(NVMainRequest *req);
			virtual bool RequestComplete(NVMainRequest *req);
			virtual void Cycle( ncycle_t = 1 ){}; 

			virtual bool Read(NVMAddress* addr ,  CacheBlock* target, NVMDataBlock* data);
			virtual bool Write(NVMAddress* addr , CacheBlock* target , NVMDataBlock* data);
			virtual bool IsIssuable( NVMainRequest* req , FailReason *reason = NULL );	
			
			void SetReadTime(uint64_t read_t);
			void SetWriteTime( uint64_t write_t );

			uint64_t GetPA( uint64_t tag, uint64_t set_id)
			{
				return tag<<tag_shift|set_id<<set_shift;
			}
	public:
			uint64_t GetTag( uint64_t pa);
			uint64_t GetIndex( uint64_t pa);	
	protected:
			CacheSet<CacheBlock> *sets;
			CacheBlock* blks;
			NVMDataBlock *data_block;
			CacheState state;

	private:
			uint64_t assoc , set_num ,cache_line_size;
			uint64_t write_time , read_time;
			
			//related to cache address translation 
			uint64_t blk_mask ;
			uint64_t set_mask , set_shift;
			uint64_t tag_shift;

			uint64_t set_timer; 	

};

};
#endif
