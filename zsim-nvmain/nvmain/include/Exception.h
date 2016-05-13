/*********************
 *
 * created on 2015/3/25
 *
 *********************/
#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__
#define __DEBUG__

#include <iostream>
#include <cstdlib>
namespace NVM
{
		/*
		 *  @to do : may be should return error code here
		 */
			
		template < class T>
		static void  Fatal( T description)
		{
			//output fatal reason, and abort program 
			std::cout<<description<<std::endl;
			abort();		
		}
		//output warning message

		template < class T>
		static void Warning( T description)
		{
			std::cout<<description<<std::endl;
		}
		
		/*for debug output*/
		template< class T>
		static void DebugOutput(T description)
		{
			#ifdef __DEBUG__
			std::cout<<"DEBUG:"<<description<<std::endl;
			#endif
		}
}

#endif
