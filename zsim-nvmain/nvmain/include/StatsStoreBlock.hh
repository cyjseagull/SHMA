/********************************
 *
 * created on 2015/4/2
 *
 * ******************************/
#ifndef __STATS_STORE_BLOCK_HH__
#define __STATS_STORE_BLOCK_HH__
namespace NVM{

class StatsStoreBlock
{
	public:
			uint64_t row_id;	//row id as tag
			uint64_t miss_time;	//row buffer specified by row_id miss time 
			uint64_t status;	//block status(0/1,separately represent valid and invalid)
	
			StatsStoreBlock():row_id(0),miss_time(0),status(0)
			{
			}

			StatsStoreBlock* operator = ( StatsStoreBlock& copy_block)
			{
				row_id = copy_block.row_id;
				miss_time = copy_block.miss_time;
				status = copy_block.status;
				return this;
			}

			void SetValid()
			{
				status = 1;
			}

			void Invalidate()
			{
				status = 0;
			}

			bool IsValid()
			{
				bool valid = false;
				if( status&1 )
						valid = true;
				return valid;
			}
};

};


#endif
