/****************************************
 *
 * created on 2015/3/24 
 *
 ****************************************/


 #ifndef __CACHE_SET_H__
 #define __CACHE_SET_H__
 #include "include/CacheBlock.hh"
 #include "include/CommonHeaders.h"
#include "include/Exception.h"
		  
namespace NVM
{
  template <class block>
  class CacheSet
 {
	public:
		uint64_t assoc; //associate of cache set
		block **cacheSet;
    public:
												    
		 //block* FindBlk( uint64_t tag , uint64_t& way_id);
		 //block* FindBlk( uint64_t tag );
										   
		 void MoveToHead( block *blk);
		void MoveToTail( block *blk);
		 uint64_t GetAssoc()
		  {
		            return this->assoc;
		   }
 };
/*
 *find cache block according to tag
 *@param tag: used to find the block
 *@param way_id: if find the target cache block , return its way id to way_id
 *@param return: the pointer of found cache block
 */
 /*
template <class block>
block* CacheSet<block>::FindBlk( uint64_t tag , uint64_t& way_id)
{
	
	//std::cout<<"cache set 0 size:"<<cacheSet[0]->data->GetSize()<<std::endl;
	for( int i = 0 ; i < assoc ; i++ )
	{
		
	//	std::cout<<"cache set i size:"<<cacheSet[i]->data->GetSize()<<std::endl;
		//hit
		if( (tag == cacheSet[i]->GetTag())&&(cacheSet[i]->IsValid() ))
		{	
				//std::cout<<"finded cache set size:"<<cacheSet[i]->data->GetSize()<<"  way is:"<<way_id<<std::endl;
				return cacheSet[i];
		}
	}

	return NULL;
}
*/
/*template <class block>
block* CacheSet<block>::FindBlk( uint64_t tag )
{
	uint64_t tmp_way_id;
	return FindBlk( tag , tmp_way_id);
}*/

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
	uint64_t i=0;
	//std::cout<<"assoc:"<<assoc<<std::endl;
	do
	{
	if( i<assoc ){
		//swap tmp_blk with cacheSet[i]
		tmp = tmp_blk;
		tmp_blk = cacheSet[i];
		cacheSet[i] = tmp;
		i++;
		}
	else
			break;
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
	//std::cout<<"assoc:"<<assoc<<std::endl;
	do
	{
		if( i >= 0)
		{
			//swap cacheSet[i] with tmp_blk
			swap_tmp = tmp_blk;
			tmp_blk = cacheSet[i];
			cacheSet[i] = tmp_blk;
			i--;
		}
		else
			break;	
	}while( tmp_blk!=blk);	
}
};
#endif
