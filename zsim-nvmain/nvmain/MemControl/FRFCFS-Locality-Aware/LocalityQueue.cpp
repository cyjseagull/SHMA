/*
 * 2015.8.18
 *
 */
#include "MemControl/FRFCFS-Locality-Aware/LocalityQueue.h"
/*
 *@function: general constructor
 *@queue_size: size Locality queue
 *@queue_id: id of locality queue
 */
LocalityQueue::LocalityQueue( int queue_size , int queue_id )
{
	queue_size_ = queue_size ;
	queue_id_ = queue_id_;
	member_num_ = 0;
		
}

/*
 *function: copy constructor
 */
LocalityQueue::LocalityQueue( LocalityQueue &queue)
{
	queue_size_ = queue.queue_size_;
	queue_id_ = queue.queue_id_;
	member_num_ = queue.member_num_;
}



/*@function: push back page description to Locality queue
 *@p_descr: description of page
 *@return: true: push back succeed; false:push back failed
 */
bool LocalityQueue::PushBack( PageDescription* p_descr)
{
	bool ret = false;
	if( member_num_ < queue_size_)
	{   
		list.push_back( p_descr);   
		member_num_++;
		ret = true;
	}   
	return false;
}

/*@function:
 * @p:
 *@return value: true:upgrade succeed
 * false: upgrade failed---
 * can't upgrade or queue is empty
 */
bool LocalityQueue::UpGrade( PageDescription* &p)
{
	bool ret = false;
	p=NULL;
	if( member_num_ )
	{
		p = *(p_des_queue_.end()-1);
		if( can_upgrade(p) )
		{
			p_des_queue_->pop_back();
			member_num--;
			ret = true;
		}
	}
	return ret;
}

        
bool LocalityQueue::DownGrade( PageDescription* &p)
{
	bool ret = false;
	p = NULL;
	if( member_num_)
	{
		p = *(p_des_queue_.begin())
		p_des_queue.pop_front();
		if( can_downgrade( p ) )
		{
			p_des_queue.pop_front();
			member_num_--;
			ret = true;
		}
	}
	return ret;
}


void LocalityQueue::AccessPage( uint64_t page_num )

{

}


