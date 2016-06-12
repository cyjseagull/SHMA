/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */

#ifndef COMMON_FUNCTION_H
#define COMMON_FUNCTION_H

#include <iostream>
//#include <assert.h>
#include "log.h"
#include <stdarg.h>
#include "string.h"
#include "stdlib.h"
#include "common/global_const.h"

//#define DEBUG_PRINT

inline void debug_printf( std::string format_str , ...)
{
	#ifdef DEBUG_PRINT
		format_str ="ZSIM-NVMAIN Debug:" + format_str+"\n";
		va_list parg;
		va_start(parg , format_str);
		vfprintf(stdout , format_str.c_str() , parg);
		va_end(parg);
	#endif
}

inline void warning( std::string format_str , ...)
{
	format_str ="ZSIM-NVMAIN Warning:" + format_str;
	va_list parg;
	va_start(parg , format_str);
	vfprintf(stdout , format_str.c_str() , parg);
	va_end(parg);
}

inline void fatal( std::string format_str , ...)
{
	format_str ="ZSIM-NVMAIN Fatal:" + format_str;
	va_list parg;
	va_start(parg , format_str);
	vfprintf(stdout , format_str.c_str() , parg);
	va_end(parg);
}

inline uint64_t mask( int bit_len)
{
	return (bit_len==64)?(uint64_t)-1LL:( (1<<bit_len)-1);
}

template <class S , class T>
inline S get_bit_value(T value, unsigned start_bit , unsigned end_bit)
{
	int bit_len = end_bit - start_bit+1;
	return (S)((value>>start_bit)&mask(bit_len));
}

inline PagingStyle string_to_pagingmode( const char* mode_str)
{
	if( !strcmp(mode_str , "Legacy_Normal") )
		return Legacy_Normal;
	if( !strcmp(mode_str , "Legacy_Huge") )
		return Legacy_Huge;
	if( !strcmp(mode_str , "PAE_Normal") )
		return PAE_Normal;
	if( !strcmp(mode_str , "PAE_Huge") )
		return PAE_Huge;
	if( !strcmp(mode_str , "LongMode_Normal") )
		return LongMode_Normal;
	if( !strcmp(mode_str , "LongMode_Middle") )
		return LongMode_Middle;
	if( !strcmp(mode_str , "LongMode_Huge") )
		return LongMode_Huge;
	return Legacy_Normal;	//default return Legacy_Normal
}

inline std::string pagingmode_to_string( PagingStyle mode)
{
	if( mode==Legacy_Normal || mode==Legacy_Huge)
		return "Legacy";
	if( mode==PAE_Normal || mode==PAE_Huge)
		return "PAE";
	else if( mode==LongMode_Normal || mode==LongMode_Middle || mode==LongMode_Huge)
		return "LongMode";
	else
		return "";

}

/*
 *@function: translate string to ZoneType
 *			 "zone.zone_dma"->Zone_DMA
 *			 "zone.zone_dma32"->Zone_DMA32
 *			 "zone.zone_normal"->Zone_Normal
 *			 "zone.zone_highmem"->Zone_HighMem
 *@param type_dir: str corresponding to a ZoneType
 *@return: return ZoneType; 
 *		   if type_dir has no corresponding ZoneType,
 *		   return MAX_NR_ZONES
 */
inline ZoneType string_to_zonetype( std::string type_str)
{
	if( type_str == c_zone_dma )
		return Zone_DMA;
	if( type_str == c_zone_dma32 )
		return Zone_DMA32;
	if( type_str == c_zone_normal)
		return Zone_Normal;
	if( type_str == c_zone_highmem)
		return Zone_HighMem;
	return MAX_NR_ZONES;
}

//calculate x^y
template<class S , class T>
uint64_t power(S base , T exp )
{
	if( exp == 1)
		return (uint64_t)base;
	else if( exp % 2 )
		return base*power(base,exp/2)*power(base,exp/2);
	else
		return (uint64_t)(power(base,exp/2)*power(base , exp/2));
}

/*
 *@function: get page size in bytes
 *@param mode: paging style , Legacy_Normal , Legacy_Huge
 *			   PAE_Normal ,PAE_Huge
 *			   LongMode_Normal,LongMode_Middle,LongMode_Huge
 */
inline uint64_t get_page_size_by_mode( PagingStyle mode)
{
	uint64_t kb = power(2,10);
	uint64_t mb = power(2,20);
	uint64_t gb = power(2,30);
	if( mode == Legacy_Normal || mode==PAE_Normal
			|| mode==LongMode_Normal)
		return (4*kb);	//4KB
	if (mode==Legacy_Huge)
		return (4*mb);	//4MB
	if ( mode==PAE_Huge || mode==LongMode_Middle)	//2MB
		return (2*mb);
	if(mode==LongMode_Huge)
		return gb; 
}

template <class T>
inline T Max( T first , T second)
{
	return (first>=second) ? first:second;
}


template <class T>
inline T Min(T first , T second)
{
	return (first<=second)?first:second;
}

bool inline is_highmem_zone( ZoneType type)
{
	return (type==Zone_HighMem);
}

//log function
inline unsigned log2( uint8_t x )
{
	unsigned result = 0;
	if( x & 0xf0 ){ result += 4; x >>= 4;}
	if( x & 0x0c ){ result += 2; x >>= 2;}
	if( x & 0x02 ){ result += 1; x >>= 1;}
	return result;
}

inline unsigned log2( uint16_t x)
{
	unsigned result = 0; 
	if( x & 0xff00){ result += 8;  x >>=8; }
	if( x & 0x00f0){ result += 4;  x >>=4; }
	if( x & 0x000c){ result += 2;  x >>=2; }
	if( x & 0x0002){ result += 1;  x >>=1; }
	return result;
}

inline unsigned log2(uint32_t x)
{
	unsigned result=0;
	if( x & 0xffff0000){ result += 16; x >>= 16; }
	if( x & 0x0000ff00){ result += 8 ; x >>= 8;  }
	if( x & 0x000000f0){ result += 4 ; x >>= 4;  }
	if( x & 0x0000000c){ result += 2 ; x >>= 2;  }
	if( x & 0x00000002){ result += 1 ; x >>= 1;  }
	return result;
}

inline unsigned log2(uint64_t x)
{
	unsigned result = 0;
	if( x & 0xffffffff00000000){ result += 32; x >>= 32; }
	if( x & 0x00000000ffff0000){ result += 16; x >>= 16; }
	if( x & 0x000000000000ff00){ result +=  8; x >>=  8; }
	if( x & 0x00000000000000f0){ result +=  4; x >>=  4; }
	if( x & 0x000000000000000c){ result +=  2; x >>=  2; }
	if( x & 0x0000000000000002){ result +=  1; x >>=  1; }
	return result;
}

inline EVICTSTYLE stringToPolicy( std::string policy_str )
{
	if( policy_str == "LRU")
		return LRU;
	if( policy_str == "HOTNESSAware" )
		return HOTNESSAware;
	if(policy_str == "HotMonitorTLBLRU")
		return HotMonitorTLBLRU;
	//default return LRU
	return LRU;
}
#endif

