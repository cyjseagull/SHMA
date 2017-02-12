/*
 * cache name: scalable high performance dram cache
 * function: implementation of DRAM cache proposed in ISCA'09 thesis:
 *           "Scalable High Performance Main Memory System Using 
 *            Phase-Change Memory Technology"
 * time && author:    2016.1.4, cyjseagull@163.com
 */
#ifndef _DRAMCACHE_SHP_H__
#define _DRAMCACHE_SHP_H__
#include "MemControl/DRAMCache/DRAMCache.h"
namespace NVM{
class DRAMCacheSHP: public DRAMCache
{
  public:
	  DRAMCacheSHP();
	  ~DRAMCacheSHP();
	  void SetConfig();
};
};
#endif

