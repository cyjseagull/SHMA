/****************************************
 *
 * created on 2015/3/24 
 *
 ****************************************/

#include "CacheSet.h"
using namespace NVM;
/*
 *find cache block according to tag
 *@param tag: used to find the block
 *@param way_id: if find the target cache block , return its way id to way_id
 *@param return: the pointer of found cache block
 */
template <class block>
block* CacheSet<block>::FindBlk( uint64_t tag , uint64_t& way_id)
{
	for( int i = 0 ; i < assoc ; i++ )
	{
		if( (tag == cacheSet[i]->GetTag())&&(cacheSet[i]->IsValid() ))
		{	
				way_id = i;
				return cacheSet[i];
		}
	}

	return NULL;
}

template <class block>
block* CacheSet<block>::FindBlk( uint64_t tag )
{
	uint64_t tmp_way_id;
	return FindBlk( tag , tmp_way_id);
}

/*
 * move a cache block of cache set to the head of the cache set
 * @param blk: specified cache block that demanded to move to the head of cache set
 * @param return : void 
 */
template <class block>
void CacheSet<block>::MoveToHead(block *blk)
{
	block * tmp_blk = blk;
	block * tmp;
	//has already in the head of cache set
	if( cacheSet[0] == blk)
			return;
	int i=0;
	do
	{
		assert( i<assoc );
		//swap tmp_blk with cacheSet[i]
		tmp = tmp_blk;
		tmp_blk = cacheSet[i];
		cacheSet[i] = tmp;
		i++;
	}while( tmp_blk!=blk);
}

/*
 * move specified block to the tail
 * @param blk : specified block need to move to the tail of a cache set 
 */
template <class block>
void CacheSet<block>::MoveToTail(block *blk)
{
	block *tmp_blk = blk;
	block *swap_tmp;
	//has already in the tail of cache set
	if( blk == cacheSet[assoc-1] )
			return;
	int i = assoc-1;
	do
	{
		assert( i >= 0);
		//swap cacheSet[i] with tmp_blk
		swap_tmp = tmp_blk;
		tmp_blk = cacheSet[i];
		cacheSet[i] = tmp_blk;
		i--;	
	}while( tmp_blk!=blk);	
}
