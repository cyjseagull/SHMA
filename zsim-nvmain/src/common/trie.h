/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef TRIE_H_
#define TRIE_H_

#include "stdint.h"
#include "locks.h"
#include "common/common_functions.h"
#include "galloc.h"
#include "pad.h"
/*trie tree for increasing TLB entry query speed*/
template <class Key , class Value>
class Trie
{
	protected:
		//tree node
		struct Node:public GlobAlloc
		{
			Key key;
			Key mask;
			Value* value;
			Node* parent;	//parent node
			Node* child[2]; //child node
			//lock_t tlb_entry_lock;
			Node(Key key_ , Key mask_ , Value* value_ ):key(key_),mask(mask_),value(value_)
			{
				parent = NULL;
				child[0] = child[1] = NULL;
			}
			//mainly clear child node
			void clear( unsigned &node_num )
			{
				if( child[0])
				{
				  node_num--;
				  child[0]->clear( node_num );
				  delete child[0];
				  child[0] = NULL;
				}
				if( child[1] )
				{
				  node_num--;
				  child[1]->clear( node_num );
				  delete child[1];
				  child[1] = NULL;
				}
			}

			bool is_match( Key search_key)
			{
				 bool result = ((search_key&mask)==(key& mask));
				//debug_printf("search key: %llx",search_key);
				return result;
			}
		};

	protected:
		//head of trie,has no mask a
		Node trie_head;
		lock_t trie_lock;
	   bool go_through(Node**parent , Node *kid , Key key , Key new_mask )
	   {
		  //key matches kid Node && origin mask is prefix of kid node mask
		  if( kid && kid->is_match(key) && (kid->mask & new_mask) == kid->mask )
		  {
			  *parent = kid;
			  return true;
		  }
		   return false;
		}

	   Key extend_one_bit( Key mask  )
	   {
		 //in case mask is 0
		 Key msb = (uint64_t)(1)<<(max_bit_-1);
		 Key result = mask | (mask >> 1) | msb;
		 return result;
	   }

	public:
	    typedef Node* TrieHandle;
		unsigned node_num;
		static const unsigned int max_bit_ = sizeof(Key)*8;
		Trie():trie_head(0,0,NULL),node_num(0)
		{
			//futex_init( &trie_lock);
		}

		/*
		 *@function: search node from trie tree
		 *@param: key  
		 * */
		Node* search(Key key)
		{
			debug_printf("search key is:%llx",key);
			//std::cout<<"search key is:"<<std::hex<<key<<std::endl;
			Node* node_tmp = &trie_head;
			while (node_tmp)
			{
				if( node_tmp->value)
				{
					//std::cout<<"found key:("<<std::hex<<node_tmp->key<<" , "<<std::hex<<node_tmp->mask<<")"<<std::endl;
					debug_printf("seach hit");
					return node_tmp;
				}
				/*
				std::cout<<"key:("<<std::hex<<node_tmp->key<<" , "<<std::hex<<node_tmp->mask<<")"<<std::endl;
				if( node_tmp->child[0])
				{
					std::cout<<"child0 key:("<<std::hex<<node_tmp->child[0]->key<<" , "<<std::hex<<node_tmp->child[0]->mask<<")"<<std::endl;
				}*/
				if( node_tmp->child[1])
				{
					//std::cout<<"child1 key:("<<std::hex<<node_tmp->child[1]->key<<" , "<<std::hex<<node_tmp->child[1]->mask<<")"<<std::endl;
				}
				if( node_tmp->child[0] && node_tmp->child[0]->is_match(key))
				{
						node_tmp = node_tmp->child[0];
				}
				else if( node_tmp->child[1] && node_tmp->child[1]->is_match(key))
				{
					node_tmp = node_tmp->child[1];
				}
				else	
					node_tmp = NULL;
			}
			return NULL;
		}
		
		/*
		 *@function:insert node(key,value) into trie 
		 *@param key: key of inserted node
		 *@param width: mask width of inserted node
		 *@param value: value of inserted node
		 *@return: pointer of inserted node 
		 */
		Node* insert_node( Key key , unsigned width , Value* value )
		{
			//debug_printf("insert node , key is %llx",key);
			//futex_lock(&trie_lock);
			Key new_mask = ~(Key)0; //init mask
			if( width < max_bit_ )
				new_mask <<= (max_bit_-width);
			key &= new_mask;
			//std::cout<<"key is "<<std::hex<<key<<std::endl;
			Node* tmp_node = &trie_head;
			//go through trie tree to find last node that can match input key
			while( go_through( &tmp_node , tmp_node->child[0],key,new_mask)||
					go_through(&tmp_node , tmp_node->child[1],key,new_mask) ){}
			assert(tmp_node);
			Key cur_mask = tmp_node->mask;
			//is the right node,but has no value
			//only to update value of this node
			if( tmp_node == &trie_head )
			{
				debug_printf("tmp node is root node");
				if( tmp_node->child[0])
					debug_printf("root node has left child");
				if( tmp_node->child[1])
					debug_printf("root node has left child");
			}
			else
				debug_printf("key of tmp node:",tmp_node->key);
			if( cur_mask == new_mask )
			{
				assert( !tmp_node->value);
				tmp_node->value = value;
				//futex_unlock(&trie_lock);
				return tmp_node;
			}
			//else go through child node deeply to find right place to insert
			for( int i=0 ; i<2 ; i++ )
			{
				Node *&kid = tmp_node->child[i];
				if(kid)
					debug_printf("key of child %d is %llx",i,kid->key);
				Node* new_node;
				//has no left child or right child
				if( !kid )
				{
					//assert( !tmp_node->value );
					new_node = new Node(key , new_mask , value);
					node_num++;
					debug_printf("insert %llx into %d child directly , mask:%llx",key , i,new_mask);
					new_node->parent = tmp_node;
					kid = new_node;
					//futex_unlock(&trie_lock);
					return new_node;
				}
				//has childi, extend cur_mask
				Key last_mask;
				bool done;
				do{
					last_mask = cur_mask;
					cur_mask = extend_one_bit(cur_mask);
					//extend over new_mask or find first mask can distinguish key and child's key
					done = (last_mask == new_mask) || ((key&cur_mask)!=(kid->key&cur_mask));
				}while( !done );
				cur_mask = last_mask;
				//no extend , is not the right node
				if( cur_mask == tmp_node->mask)
					continue;
				debug_printf("current mask is %llx",cur_mask);
			    //find first mask that doesn't fit key with child->key
				//create a middle node
				new_node = new Node(key, cur_mask , NULL);
				node_num++;
				debug_printf("create middle node: (%llx,%llx)",key&cur_mask,cur_mask);
				new_node->parent = tmp_node;
				kid->parent = new_node;
				new_node->child[0] = kid;
				kid = new_node;
				if( cur_mask == new_mask )	
				{
					new_node->value = value;
					//futex_unlock(&trie_lock);
					return new_node;
				}
				//new right node for inserted node
				new_node = new Node(key , new_mask , value);
				node_num++;
				new_node->parent = kid;
				kid->child[1] = new_node;
				debug_printf("insert node succeed: (%llx,%llx)",key&new_mask,new_mask);
				//futex_unlock(&trie_lock);
				return new_node;
			}
			//futex_unlock(&trie_lock);
			return NULL;
		}
		/*
		 *@function: remove node pointed trie node from trie tree
		 *@param node: point to node to be removed from trie tree
		 *@return: value of removed node
		 */
		Value* remove_node ( Node* node)
		{
	       //debug_printf("remove node");
			//futex_lock(&trie_lock);
		   Value* val = node->value;
		  //has two children , just remove value of node
		  //and take it as middle node
		  if(node->child[1])
		  {
			assert(val);
			node->value = NULL;
			//futex_unlock(&trie_lock);
			return val;
		  }
		  /*only has a child or has no child,remove node,
		   * and adjust its child node and parent node*/
		  Node* parent = node->parent;
		  //has no parent node , is the root node
		  if( !parent )
			panic("node to be removed is the root node , can't remove root node");
		  if( node->child[0])
			  node->child[0]->parent = parent;
		  //node is the left child
		  if( parent->child[0]==node)
			  parent->child[0] = node->child[0];
		  //node is the right child
		  else if(parent->child[1]==node)
			  parent->child[1] = node->child[0];
		  else
			  fatal("relationship between node and it's parent is inconsistent");
		  //adjusted parent only has right child
		  //adjust it to be parent's left child
		  if( parent->child[1] && !parent->child[0])
		  {
			parent->child[0] = parent->child[1];
			parent->child[1] = NULL;
		  }
		  //futex_unlock(&trie_lock);
		  //if parent is not the root node
		  //and has no more than two child , and has no value , remove it
		  if( !parent->child[1]&& parent->value==NULL && parent->parent )
			  remove_node(parent);
		  delete node;
		  node_num--;
		  return val;
		}

		/*
		 *@function: remove node whose key is key from trie tree
		 *@param key: key of node to be removed
		 *@return: value of removed node
		 */
		Value* remove_node( Key key)
		{
			//find Node according to key first
			Node* node = search(key);
			if( node )
				return remove_node(node);
			return NULL;
		}

		/*
		 *@function: clear all element of trie tree
		 */
		void clear()
		{
			//futex_lock( &trie_lock);
			trie_head.clear( node_num );
			//futex_unlock( &trie_lock);
		}
};
#endif
