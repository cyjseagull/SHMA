
#ifndef __COMMON_MATH_HH__
#define __COMMON_MATH_HH__
#include "include/CommonHeaders.h"
#include "include/Exception.h"
#include <cctype>
#include <algorithm>


namespace NVM{

template <class T>
inline T
LeastSigBit(T n)
{
    return n & ~(n - 1);
}

template <class T>
inline bool
IsPowerOf2(T n)
{
    return n != 0 && LeastSigBit(n) == n;
}

inline uint64_t
Power(uint32_t n, uint32_t e)
{
    if (n > 20)
        NVM::Warning("Warning, power() function is quite slow for large exponents\n");

    if (e == 0)
        return 1;

    uint64_t result = n;
    uint64_t old_result = 0;
    for (uint64_t x = 1; x < e; x++) {
        old_result = result;
        result *= n;
        if (old_result > result)
            NVM::Warning("power() overflowed!\n");
    }
    return result;
}


inline int
Log2(unsigned x)
{
    assert(x > 0);

    int y = 0;

    if (x & 0xffff0000) { y += 16; x >>= 16; }
    if (x & 0x0000ff00) { y +=  8; x >>=  8; }
    if (x & 0x000000f0) { y +=  4; x >>=  4; }
    if (x & 0x0000000c) { y +=  2; x >>=  2; }
    if (x & 0x00000002) { y +=  1; }

    return y;
}

inline int
Log2(unsigned long x)
{
    assert(x > 0);

    int y = 0;

#if defined(__LP64__)
    if (x & (uint64_t)(0xffffffff00000000)) { y += 32; x >>= 32; }
#endif
    if (x & 0xffff0000) { y += 16; x >>= 16; }
    if (x & 0x0000ff00) { y +=  8; x >>=  8; }
    if (x & 0x000000f0) { y +=  4; x >>=  4; }
    if (x & 0x0000000c) { y +=  2; x >>=  2; }
    if (x & 0x00000002) { y +=  1; }

    return y;
}

inline int
Log2(unsigned long long x)
{
    assert(x > 0);

    int y = 0;

    if (x & (uint64_t)(0xffffffff00000000)) { y += 32; x >>= 32; }
    if (x & (uint64_t)0x00000000ffff0000) { y += 16; x >>= 16; }
    if (x & (uint64_t)(0x000000000000ff00)) { y +=  8; x >>=  8; }
    if (x & (uint64_t)(0x00000000000000f0)) { y +=  4; x >>=  4; }
    if (x & (uint64_t)(0x000000000000000c)) { y +=  2; x >>=  2; }
    if (x & (uint64_t)(0x0000000000000002)) { y +=  1; }

    return y;
}

inline int
Log2(int x)
{
    assert(x > 0);
    return Log2((unsigned)x);
}

inline int
Log2(long x)
{
    assert(x > 0);
    return Log2((unsigned long)x);
}

inline int
Log2(long long x)
{
    assert(x > 0);
    return Log2((unsigned long long)x);
}


/*
 * parser input string into two string(remove all trim)
 * 				---num_str and unit, respectively represent number and unit
 * @param num_str:store one part of parsered string
 * @param unit_str: store unit part of parsered string
 * @param source_str:string to be parsered
 * @param trim: separator,default is blank
 * @param base: string->int's base,maybe 2,10,8,16,etc
 * @param return: return the number num_str represented for
 */
inline uint64_t StringParser( std::string& num_str , std::string& unit_str , std::string source_str , int base=10,char trim=' ')
{
	int pos = 0;
	int len = source_str.find_first_not_of(trim);

	//std::cout<<"len:"<<len<<std::endl;
	//trim head and tail's blank
	source_str.erase( pos , len );
	source_str.erase( source_str.find_last_not_of(trim)+1);
	//std::cout<<"source str:"<<source_str<<std::endl;
	//trim middle blank
	pos = source_str.find_first_of(trim);
	if(pos>=0)
	{	
		len = source_str.find_last_of(trim)-pos+1;
		source_str.erase(pos , len);
	}
	int unit_len = 0;
	pos = source_str.size()-1;
	const char* tmp = source_str.c_str();
	char *end;
	//non-number
	while( ( tmp[pos]<'0')||(tmp[pos]>'9') )
	{
		unit_len++;
		pos--;
	}
	unit_str = source_str.substr( pos+1, unit_len);
	num_str = source_str.substr( 0 , source_str.size()-unit_len );	
	uint64_t result = static_cast<uint64_t>(strtol( num_str.c_str() , &end , base));
	return result;
}

/*uniform all characters of input string to capital 
 */
inline void ToUpperString(std::string &str)
{
	transform( str.begin() , str.end() , str.begin(), (int(*)(int))toupper);
}

/*uniform all characters of input string to lowercase
 */
inline void ToLowerString(std::string &str)
{
	transform( str.begin() , str.end() , str.begin(), (int(*)(int))tolower);
}


/*
 * @return: translate result to Bytes according to unit_str(B/KB/MB...)
 */
inline uint64_t ToBytes( uint64_t result , std::string unit_str)
{
			 ToUpperString( unit_str);
		     if( (unit_str =="B")|| (unit_str==""))
				             return result;
		     if(unit_str == "KB")
				             return result*(NVM::Power(2,10));
		     if( unit_str == "MB")
				            return result*( NVM::Power(2,20));
		     if( unit_str =="GB")
				             return result*( NVM::Power(2,30));
		     if( unit_str =="TB")
				             return result*( NVM::Power(2,40));
			 return -1;
}

/*
 *translate xxxB/XXXMB/XXXgb,etc described by input string to bytes
 *@param capacity_str: string describe memory/disk size in form of XXXb/xxxGB/xxxmb,etc
 *@return: transformed capacity
 */
inline uint64_t TransToBytes( std::string capacity_str , int base=10 )
{

	std::string num_str , unit_str;
	uint64_t result = StringParser( num_str , unit_str , capacity_str,base );
	std::cout<<"num_str:"<<num_str<<" unit_str:"<<unit_str<<std::endl;
	return ToBytes( result , unit_str);
}

/*
 *function is the same of TransToBytes , but firstly it must check if the capacity is the power of two
 *@return:if the capacity is the power of two,translate it to Bytes; else return -1
 */
inline int TransCheck( std::string capacity_str, int base=10)
{
		std::string num_str , unit_str;
		uint64_t result = StringParser( num_str , unit_str , capacity_str,base );
		if(IsPowerOf2(result))
		{
			return ToBytes( result , unit_str );
		}
		else
			 return -1;
}

/*
inline bool CopyChars(char* dest_chars , char* source_chars)
{
	int len = strlen(source_chars);
	if(!dest_chars)
	{
		dest_chars = new char[len+1];
		//allocate space failed
		if( !dest_chars)
			return false;
	}
	memcpy( dest_chars , source_chars , len);
	dest_chars[len] = '\0';
	return true;
}
*/

inline bool CopyChars(char* dest_chars , const char* source_chars)
{
	int len = strlen(source_chars);
	if(!dest_chars)
	{
		dest_chars = new char[len+1];
		//allocate space failed
		if( !dest_chars )
			return false;
	}
	memcpy( dest_chars , source_chars , len);
	dest_chars[len] = '\0';
	return true;
}

};
#endif 
