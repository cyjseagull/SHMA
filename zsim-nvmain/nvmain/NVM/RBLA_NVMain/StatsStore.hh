/*************************************
 *
 * created on 2014/4/2
 *
 *************************************/

#ifndef	__STATS_STORE_HH__
#define __STATS_STORE_HH__

#include "include/CommonHeaders.h"
#include "include/NVMAddress.h"
#include "include/CacheSet.hh"
#include "include/StatsStoreBlock.hh"
#include "include/Exception.h"
namespace NVM{
		class StatsStore
		{	
			private:
				uint64_t GetRowAddress( NVMAddress* addr);		
				uint64_t entry_num;
				CacheSet<StatsStoreBlock>* stats_table;	
				StatsStoreBlock* blk;
			public:
				StatsStore( uint64_t entry_num)
				{
					if( entry_num <= 0 )
							NVM::Fatal("stats store table entry num must larger than 0");
					this->entry_num = entry_num;
					//create and init cache set
					stats_table = new CacheSet<StatsStoreBlock>();
					blk = new StatsStoreBlock[entry_num];
					stats_table->cacheSet = new StatsStoreBlock* [entry_num];
					//initialize stats_table
					for( uint64_t i=0; i<entry_num ; i++)
					{
						stats_table->cacheSet[i] = &blk[i];
					}

				}


				~StatsStore()
				{
					if( stats_table)
					{
						delete stats_table;
						stats_table = NULL;
					}

					if( blk)
					{
						delete blk;
						blk = NULL;
					}
				}

				StatsStoreBlock* FindEntry( uint64_t row_id )
				{
					for(uint64_t i=0 ; i<entry_num;i++)
					{
						if( (stats_table->cacheSet[i]->row_id== row_id)&&( stats_table->cacheSet[i]->IsValid()))
						{
							return stats_table->cacheSet[i];
						}
					}
				    return NULL;
				}


				StatsStoreBlock* FindVictim(uint64_t& entry_id)
				{
					for( uint64_t i=0 ; i<entry_num;i++)
					{
						if( !stats_table->cacheSet[i]->IsValid())
						{
								entry_id = i;
								return stats_table->cacheSet[i];
						}
					}
					entry_id = entry_num-1;
					return stats_table->cacheSet[entry_id];
				}

			   /*	
				void Evict(StatsStoreBlock* victim)
				{
					victim->miss_time = 0;
					victim->row_id = 0;
					victim->status = 0;
				}
				*/
				void Clear()
				{
					//std::cout<<"clear stats store"<<std::endl;
					for(uint64_t i=0;i<entry_num;i++)
					{
					  stats_table->cacheSet[i]->miss_time=0; 
					}
				}

				void Install( StatsStoreBlock* install_block , uint64_t row_id )
				{
					install_block->row_id = row_id;
					install_block->SetValid();
					install_block->miss_time = 0;
				}	


				void IncreMissTime(StatsStoreBlock* target , uint64_t steps )
				{
					if(target)
					{
						target->miss_time += steps;
					}
				}

				
				void DecreMissTime(StatsStoreBlock* target , uint64_t steps )
				{
					if(target)
					{
						target->miss_time -= steps;
					}
				}	
		};
};

#endif
