/**********************************
 *
 *created on 2015/3/25
 *
 ***********************************/
#include "CacheAssoc.h"
using namespace NVM;
CacheImpl::CacheImpl( uint64_t assoc , uint64_t set , uint64_t cache_line_size , uint64_t read_t , uint64_t write_t)
{
	//check input
	if( assoc <= 0 )
		NVM::Fatal("cache assoc must > 0");
	if( cache_line_size <4 || !IsPowerOf2( cache_line_size) )
		NVM::Fatal("cache line size must larger than 4 bytes and the power of 2 ");
	if( set <=0 || !IsPowerOf2( set ) )
		NVM::Fatal("the number of cache sets must more than 0 and the power of 2");
	if( read_t <=0 )
		NVM::Fatal("cache read time must larger than 0");
	if(write_t <=0 )
		NVM::Fatal("cache write time must larger than 0");
		
	this->assoc = assoc;
	this->set_num = set;
	this->cache_line_size = cache_line_size;

	this->read_time = read_t;
	this->write_time = write_t;
	this->state = CACHE_IDLE;
	blk_mask = cache_line_size - 1;
	set_mask = set_num - 1;
	set_shift = Log2( cache_line_size);
	tag_shift = set_shift + Log2(set_num);
	std::cout<<"tag_shift: "<<tag_shift<<std::endl;
	
	uint64_t blk_num = assoc*set;
	//CacheBlock *tmp_blk;

	//create cache sets and cache blocks
	sets = new CacheSet<CacheBlock>[set_num];
	blks = new CacheBlock[set_num*assoc];

	data_block = new NVMDataBlock[blk_num];

	//NVM::DebugOutput("initialize NVM data block");
	//init NVM data block
	for( uint64_t i=0; i< blk_num ; i++)
	{	
		data_block[i].rawData = NULL;
		data_block[i].SetSize(cache_line_size);
		data_block[i].SetValid(false);
		blks[i].data = &data_block[i];
	}
	//initialize sets and blks
	//NVM::DebugOutput("initialize sets and blks");
	for( uint64_t i = 0; i < set_num ; i++)
	{
		sets[i].assoc = assoc;
		sets[i].cacheSet = new CacheBlock* [assoc];
		for(uint64_t j=0 ; j< assoc ; j++)
		{
			CacheBlock *tmp_blk = &blks[i*assoc+j];
			//DebugOutput(tmp_blk->data->GetSize());
			tmp_blk->set = i;
			tmp_blk->tag = j;
			sets[i].cacheSet[j] = tmp_blk;	
		}
	}			
}


CacheImpl::~CacheImpl()
{
	if( sets )
	{
		delete sets;
		sets = NULL;
	}

	if( blks )
	{
		delete blks;
		blks = NULL;
	}

	if( data_block )
	{
		delete data_block;
		data_block = NULL;
	}
}


/*
 * find specified block in cache according to address
 * @param addr : we can get physical address from this object
 * @param way_id : block in which way of specified set
 * @param set_id : block in which set of cache  
 */
CacheBlock* CacheImpl::FindBlock( NVMAddress* addr ,uint64_t &set_id , uint64_t &way_id) 
{	
	//CacheBlock* tmp = NULL;
	uint64_t pa= addr->GetPhysicalAddress();
	uint64_t tag = GetTag( pa );
	set_id = GetIndex( pa );
	for( uint64_t i = 0  ; i < this->assoc ; i++)
	{
		if( (tag == sets[set_id].cacheSet[i]->tag)&&(sets[set_id].cacheSet[i]->IsValid()) )
		{
			way_id = i;
			//std::cout<<"find block's data size:"<<sets[set_id].cacheSet[i]->data->GetSize()<<std::endl;
			return sets[set_id].cacheSet[i];
		}

	}
		
	//tmp = sets[set_id].FindBlk( tag , way_id );
	//std::cout<<"set"<<set_id<<" status: "<<sets[set_id].cacheSet[0]->status<<std::endl;
	return NULL;
}


CacheBlock* CacheImpl::FindBlock(NVMAddress* addr)
{
		uint64_t ignore_set_id;
		uint64_t ignore_way_id;
		return FindBlock( addr ,ignore_set_id, ignore_way_id );
}


uint64_t CacheImpl::GetTag( uint64_t pa )
{
	return pa>>tag_shift;	
}

uint64_t CacheImpl::GetIndex( uint64_t pa)
{
	return (pa & set_mask)>>set_shift;	
}

void CacheImpl::SetReadTime(uint64_t read_t)
{
		this->read_time = read_t;
}

void CacheImpl::SetWriteTime( uint64_t write_t)
{
	this->write_time = write_t;
}

/*
 * find victim for fetching addr pointed memory data
 * @param addr: address of memory data to be fetched to cache
 * @param set_id: set id the address mapped to
 * @param way_id: way id the victim block located 
 * @return: pointer of victim cache block , if not found , return the replace block pointer(the last block of mapped cache set) , way_id is assoc-1
 */
CacheBlock* CacheImpl::FindVictim( NVMAddress *addr , uint64_t& set_id ,uint64_t &way_id )
{
	
	CacheBlock* tmp_blk = NULL;
	uint64_t pa = addr->GetPhysicalAddress();
	set_id = GetIndex( pa );
	bool set_full = true;
	for( uint64_t i = 0 ; i < assoc ;i++)
	{
		if( !(sets[set_id].cacheSet[i]->IsValid()) )
		{
			way_id = i;
			tmp_blk = sets[set_id].cacheSet[i];
			set_full = false;
			break;
		}
	}	
	if(set_full)
	{
		 //DebugOutput("set full");
		 tmp_blk = sets[set_id].cacheSet[assoc-1];
		way_id = assoc-1;
	}
	return tmp_blk;
}



CacheBlock* CacheImpl::FindVictim( NVMAddress *addr )
{
	uint64_t ignore_set_id ;
	uint64_t ignore_way_id;
	return FindVictim( addr , ignore_set_id , ignore_way_id );
}



/*
 * read data block from cache fetched from memory address addr
 * @param addr: read address
 * @param data: pointer point to data of addr
 * @ return : true--read hit; false:read miss
 */
bool CacheImpl::Read( NVMAddress * addr , CacheBlock* target, NVMDataBlock *data)
{
	int set_id = 0;
	bool read_success = true;
	//read hit		
	if(target)
	{
	*data = *(target->data);
	sets[set_id].MoveToHead(target);	//modify target to MRU
	read_success = true;
	}
	return read_success;
	
}


/*
 * if data of addr locating in the cache,write it with new data,modify its status with dirty
 * @param addr:write address
 * @param data:pointer of the modified data
 */
bool CacheImpl::Write( NVMAddress* addr , CacheBlock* target ,  NVMDataBlock *data)
{
	uint64_t way_id = 0;
   	uint64_t set_id = 0;
	bool write_success = false;
	if(target)
	{
		write_success = true;
		*(target->data) = *data;
		sets[set_id].cacheSet[way_id]->status |=BlkDirty;	//set status dirty
		sets[set_id].MoveToHead(target);	//modify target to MRU	
	}
	return write_success;	
}


/*
 * evict data of addr from cache block , getting its data
 * @param addr:address of block needed to be evicted 
 * @param data:evicted block's data 
 * @return: if evict dirty block(need to be written to main memory),return true;
 * @		else return false 
 */
bool CacheImpl::Evict( NVMAddress *addr)
{
	CacheBlock* target = FindBlock( addr );	
	return Evict( target);
}

/*
 * evict cache block from cache
 * @param blk: cache block to be evicted
 * @param data: pointer of evicted cache block's data
 * @return : true--evict dirty cache block that need to be written to memory
 * 			 false--evict undirty cache block that not need to be written to memory
 */
bool CacheImpl::Evict( CacheBlock *blk)
{
	bool evict_dirty = false;

	if(blk)
	{	
		//blk is dirty
		if( blk->IsDirty())
		{
			//NVM::DebugOutput("wb block");
			evict_dirty = true;
		}
	}
	return evict_dirty;
}

/*
 * install data from install_addr to victim_blk of cache
 * @param data: data pointer of new cache block
 * @
 */
void CacheImpl::Install( NVMAddress* install_addr , CacheBlock* victim_blk , NVMDataBlock* data)
{
	uint64_t pa = install_addr->GetPhysicalAddress();
	uint64_t install_tag = GetTag( pa );
 	victim_blk->status = BlkValid;
	//std::cout<<"victim status"<<victim_blk->status<<std::endl;
	//must be copy operation!!!
	*(victim_blk->data) = *data;
	victim_blk->data->SetValid(true);
	victim_blk->tag = install_tag;
	//because new cache block's set id is equal of victim_blk's set id
	//so it's no necessary to assign set again
	//victim_blk->set = set_id;	
}

/*
 * judge whether can issue command
 */
bool CacheImpl::IsIssuable( NVMainRequest* req ,FailReason *reason)
{
	bool can_issue = false;
	if( state == CACHE_IDLE)
			can_issue = true;
	return can_issue;	
}


/*
 * send cache access request: CACHE_READ or CACHE_WRITE
 * CACHE_READ: if read hit, issue command and insert it to event queue;
 * 			   if read miss, issue command CACHE_WRITE , continue processing
 * CACHE_WRITE:if write hit, issue command and insert it to event queue;
 * 			   if write miss , issue CACHE_EVICT command , then CACHE_WRITE command
 *@param req: request to issue
 *@return : true --- issue command successfully
 *			false -- issue command unsuccessfully
 */
bool CacheImpl::IssueCommand( NVMainRequest *req)
{
	uint64_t set_id;
	uint64_t way_id;
	if( IsIssuable( req ))
	{
	//user defined request
	CacheRequest *cache_req = static_cast<CacheRequest *>(req->reqInfo);
	CacheBlock *target = NULL;
	switch( cache_req->optype)
	{
		case CACHE_READ:
				state = CACHE_BUSY;
				set_timer = GetEventQueue()->GetCurrentCycle() + read_time;
				//read hit
				if( (target = FindBlock( &req->address , set_id , way_id )) )	
				{
					cache_req->hit = true;
					Read(&(req->address),target , &(req->data) );	
				}
				GetEventQueue()->InsertEvent(EventResponse , this , req , set_timer);			
				break;
				//write include:1.replacement(step one:evict; step two:install new block)
				//				2.modify data 
		case CACHE_WRITE:
				state = CACHE_BUSY;
				 
				//write hit
				if( (target = FindBlock(&req->address , set_id , way_id)) )
				{
					Write(&(req->address),target,&(req->data));
					cache_req -> hit = true;
					set_timer = GetEventQueue()->GetCurrentCycle() + write_time;
					//insert event
					GetEventQueue()->InsertEvent(EventResponse , this ,req , set_timer);
				}
				//write miss
				else
				{
					CacheBlock* victim_blk = FindVictim(&req->address ,set_id , way_id );
					
					//set full , evict cache block 
					if( way_id == assoc-1 )
					{
						NVMainRequest *nv_evict_req = new NVMainRequest();
						CacheRequest *cache_evict = new CacheRequest;

					    cache_evict->optype = CACHE_EVICT;
						cache_evict->address.SetPhysicalAddress(GetPA(victim_blk->tag , assoc-1));  
		
						Evict( &(req->address) );
						nv_evict_req->reqInfo = static_cast<void*>(cache_evict);
						nv_evict_req->owner = req->owner;
						nv_evict_req->tag = req->tag;	
						GetEventQueue()->InsertEvent( EventResponse , this , nv_evict_req , set_timer);
					}
						//install new cache block
						Install( &(req->address) , victim_blk , &(req->data));
						GetEventQueue()->InsertEvent( EventResponse , this , req , set_timer);
				
				}
				break;


		default:
				NVM::Warning("Unknown cache operation");
				break;
	}
		return true;
	}
	else
		return false;
}

bool CacheImpl::RequestComplete( NVMainRequest* req)
{
	GetParent()->RequestComplete(req);
	state = CACHE_IDLE;
	return true;		
}


