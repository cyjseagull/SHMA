#include <iostream>
#include <vector>
#include <map>
#include "stdint.h"
#include "test.h"
/*
struct Page
{
		unsigned count;	//how many processes this page mapped to
		uint64_t pageNo;	//page no

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

};
*/

//calculate x^y
template<class S , class T>
uint64_t Page::power(S base , T exp )
{
	if( exp == 1)
		return (uint64_t)base;
	else if( exp % 2 )
		return base*power(base,exp/2)*power(base,exp/2);
	else
		return (uint64_t)(power(base,exp/2)*power(base , exp/2));
}

int main()
{
	unsigned a = 2;
	unsigned b = 11;
	
		struct Page page_;
		uint64_t result = page_.power(2,3);
		std::cout<<"result is "<<result<<std::endl;
}
