#ifndef _FETCHER_FACTORY_H_
#define _FETCHER_FACTORY_H_
#include "src/NVMObject.h"
#include "Utils/BlockFetcher/BlockFetcher.h"
namespace NVM
{
	class FetcherFactory
	{
		public:
			static NVMObject* CreateFetcher( std::string fetcher_name)
			{
				if( fetcher_name == "BlockFetcher")
				{
					std::cout<<"create block fetcher"<<std::endl;
					return (new BlockFetcher());
				}
				return NULL;
			}
	};
};
#endif
