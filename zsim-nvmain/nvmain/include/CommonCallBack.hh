/********************
 *
 * created on 2015/4/9
 *
 * *****************/

#ifndef __COMMON_CALL_BACK__
#define __COMMON_CALL_BACK__

#include <map>

namespace NVM
{
	//dram miss time(open bank): 1.tRP:precharge to activate a new row
	//				  2.tRCD:ras->cas
	//				  3.tCAS:cas latency
	template <class T>
	inline T GetDRAMMissTime( T tRP , T tRCD , T tCAS )
	{
		return (tRP+tRCD+tCAS);
	}
	
	
	//pcm clean miss(write back mode):last request is read(data is no-change)
	//			     1.tRCD:ras->cas
	//			     2.tCAS:cas latency to read/write data
	template <class T>
	inline T GetPCMCleanMissTime( T tRCD,T tCAS )
	{
		return tRCD+tCAS;
	}


	//pcm dirty miss(write back mode): last request is write(data has been changed)
	//				  1.tRCD:ras->cas
	//				  2.tWP: write pluse time
	//				  3.tRP: precharge for tWP
	//				  4.tCAS: cas latency
	template<class T>
	inline T GetPCMDirtyMissTime( T tRCD , T tWP ,T tRP , T tCAS )
	{
		return tRCD+tWP+tRP+tCAS;
	}

	template< typename T , typename S>
	inline S UpdateMapValue( std::map<T,S> &targetMap ,T key , S increStep )
	{
		if( targetMap.count( key ) )
		{
			targetMap[key] += increStep;
		}
		else
		{
			//insert new entry
			targetMap[key] = increStep;
		}
		return targetMap[key];
	}

	template<typename T , typename S>
	inline S DelMapValue( std::map<T,S> &targetMap , T key)
	{
		S value = static_cast<S>(0);

		if( targetMap.count(key))
		{
			value = targetMap[key];
			targetMap.erase( key );
		}
		return value;
	}
	
};

#endif
