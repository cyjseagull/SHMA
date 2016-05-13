/*********************************
 *
 *added on 2015/3/24
 *
 * ******************************/
#ifndef __CACHE_BLOCK_H__
#define __CACHE_BLOCK_H__

#include "include/NVMDataBlock.h"
#include "include/CommonEnums.h"

namespace NVM
{
	class CacheBlock
	{
	public:				
		uint64_t tag;	//tag
		NVMDataBlock *data;	
		unsigned status;	//current status of cache block @CacheBlkStatus
		int set;		//set the cache block belonged to
		unsigned int refCount;	//how many times did the the cache block has been referenced
			
	public:
		CacheBlock():tag(0),status(0),set(-1),refCount(0)
		{}

		/**
		 * copy the cache block of the given one
		 * @param copy_blk:the cache block to copy
		 * @return : the pointer of this cache block
		 **/
		CacheBlock* operator=( const CacheBlock& copy_blk)
		{
			this->tag = copy_blk.tag;
			this->data = copy_blk.data;
			this->status = copy_blk.status;
			this->set = copy_blk.set;
			this->refCount = copy_blk.refCount;
			return this;
		}

		void SetSize( uint64_t n )
		{
			this->data->SetSize(n);
		}

		uint64_t GetTag()
		{
			return tag;
		}

		NVMDataBlock* GetData()
		{
			return this->data;
		}

		uint64_t GetStatus()
		{
			return status;
		}

		int GetRefCount()
		{
			return this->refCount;
		}

		void IncreRefCount()
		{
			refCount++;
		}
		/*
		bool IsWritable()
		{
			return (status & ( BlkWritable | BlkValid ));
		}

		bool IsReadable()
		{
			return ( status & ( BlkReadable | BlkValid ) );
		}
			*/
		bool IsDirty()
		{
			return ( status & ( BlkValid | BlkDirty ));
		}

		bool IsValid()
		{
			return ( status & BlkValid );
		}

		/**
		 * invalidate the cache block
		 **/
		void Invalidate()
		{
			status = 0;
		}
	};
};
#endif
