/*
 *created on 2015/3/26
 */
#ifndef __COMMON_ENUMS_H__
#define __COMMON_ENUMS_H__
#include "include/NVMAddress.h"
#include "include/NVMDataBlock.h"
#include "src/NVMObject.h"

namespace NVM
{
	enum CacheState { CACHE_IDLE, CACHE_BUSY };
	enum CacheOperation { CACHE_NONE, CACHE_READ, CACHE_WRITE,CACHE_FILL,CACHE_SCRUB, CACHE_EVICT };

	 enum CacheBlkStatus 
	 {  
	   BlkValid = 0x01,
	   //BlkWritable = 0x02,
	   //BlkReadable = 0x04,
	   BlkDirty = 0x02
	   //BlkReferenced = 0x10
	   };

	 //user self-defined cache request
	 struct CacheRequest
	{   CacheOperation optype;	//CACHE_READ or CACHE_WRITE
		NVMAddress address;		//address of data to be operated 
		NVMAddress endAddr;
	//	uint64_t cache_tag;	
		NVMDataBlock data;	
		bool hit; 				//if the request hitted in cache
		NVMObject *owner;		//who send the request
		NVMainRequest *originalRequest;  //trace the original request in case of it has been hooked
	};

	 typedef uint64_t Addr; 
	 enum 
	 {
	    VALID = 1,
	    INVALID = 0 
     };
};

#endif
