/****************************************
 *
 * created on 2015/3/24 
 *
 ****************************************/
#ifndef __CACHE_SET_H__
#define __CACHE_SET_H__

#include "CacheBlock.hh"
#include "include/CommonHeaders.h"


namespace NVM
{
template <class block>
class CacheSet
{
	public:
		uint64_t assoc;	//associate of cache set
		block **cacheSet;

	public:

		block* FindBlk( uint64_t tag , uint64_t& way_id);
		block* FindBlk( uint64_t tag );

		void MoveToHead( block *blk);
		void MoveToTail( block *blk);
		uint64_t GetAssoc()
		{
			return this->assoc;
		}
};
};
#endif
