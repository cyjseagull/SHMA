#ifndef _COMMON_STRUCTURES_H_
#define _COMMON_STRUCTURES_H_
template<class T>
class FlexiList
{
	public:
		FlexiList():head(NULL),tail(NULL)
		{
			size = 0;
		}
		bool is_empty()
		{
			if(head)
				return false;
			return true;
		}
		void clear()	
		{
			size = 0;
			head = tail = NULL;
		}
		//add block to the tail of list 
		void push_block_back(T *block)
		{
			if( head == NULL )
			{
				tail = head = block;
				tail->next = NULL;
				size++;
				return;
			}
			else if( head == tail)
			{
				head->next = block;
			}
			tail->next = block;
			tail = block;
			tail->next = NULL;
			size++;
		}

		T* fetch_head()
		{
			assert(head);
			T* tmp_block;
			tmp_block = head;
			if( head->next == NULL)
			{
				//std::cout<<"set head with NULL"<<std::endl;
				head =  NULL;
				tail = NULL;
			}
			else
			{
				head = head->next;
				tmp_block->next = NULL;
			}
			size--;
			return tmp_block;	
		}

		int get_size()
		{
			return size;
		}

		void extend_back( FlexiList<T>* added_list)
		{
			if( added_list->is_empty())
				return;
			if( head == NULL)	
			{
				head = added_list->head;
				tail = added_list->tail;
				return;
			}
			tail->next = added_list->head;
			tail = added_list->tail;
			size += added_list->get_size();
			return;
		}

	T* head;
	T* tail;
	int size;
};
#endif
