/*
 * 2015.8.18
 *
 */
#ifndef _Locality_QUEUE_H_
#define _Locality_QUEUE_H_

#include "stdint.h"
#include "include/CommonMath.h"
namespace NVM
{
  //
  class LocalityQueue
  {
	  public:
		/*
		 *@function: general constructor
		 *@queue_size: size Locality queue
		 *@queue_id: id of locality queue
		 */
		LocalityQueue( int queue_size , int queue_id ) 
		{
			queue_size_ = queue_size ;
			queue_id_ = queue_id_;
			member_num_ = 0;
		}
		/*
		 * function: copy constructor
		 */
		LocalityQueue( LocalityQueue &queue)
		{
			queue_size_ = queue.queue_size_;
			queue_id_ = queue.queue_id_;
			member_num_ = queue.member_num_;
		}

		/*@function: push back page description to Locality queue
		 *@p_descr: description of page  
		 *@return: true: push back succeed; false:push back failed
		 */
		bool PushBack( PageDescription* p_descr)
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
		 *@p:
		 *@return value: true:upgrade succeed
	     *				 false: upgrade failed---
		 *				can't upgrade or queue is empty
		 */
		bool UpGrade( PageDescription* &p)
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
		bool DownGrade( PageDescription* &p)
		void AccessPage( uint64_t page_num )
	  private:
		bool can_upgrade( PageDescription* p)
		{
			bool ret = false;
			if( p->reference_time_ == Power(2,queue_id_+1) )
				ret = true;
			return ret;
		}

		bool can_downgrade( PageDescription* p)
		{
			bool ret = false;
			if( !p->life_time_)
				ret = true;
			return ret;
		}
	 private:
		int queue_size_;
		int queue_id_;
		int member_num_;
		std::list<PageDescription *> p_des_queue_; 
  };
 
  //node describe page information
  class PageDescription
  {
	public:
		PageDescritpion( uint64_t page_num , uint64_t life_time)
		{
			page_num_ = page_num;
			reference_time_ = 0;
			rb_miss_num_ = 0;
			life_time_ = life_time;
		}

		//copy constructor
		PageDescription( PageDescription &p)
		{
		    page_num_ = p.page_num_;
			reference_num_ = p.reference_num_;
			rb_miss_num_ = p.rb_miss_num_;
			life_time_ = p.life_time_;
		}

		uint64_t GetPageNum()
		{
			return page_num_;
		}

		uint64_t GetReferenceNum()
		{
			return reference_num_;
		}

		uint64_t GetLifeTime()
		{
			return life_time_;
		}

		uint64_t GetRowBufferMissNum()
		{
			return rb_miss_num_;
		}
		
		void IncreLifeNum( int step = 1)
		{
			life_time_ += step;
		}
		
		void IncreReferenceNum ( int ref = 1)
		{
			reference_num_ += ref;
		}

		void IncreRbMissNum( int rb_miss = 1)
		{	
			reference_time_ += rb_miss;
		}
	private:
			uint64_t page_num_;
			uint64_t reference_time_;
			uint64_t rb_miss_num_;
			uint64_t life_time_; 
  };
};

#endif
