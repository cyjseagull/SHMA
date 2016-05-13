/******************************
 * created on 2015/5/6
 ******************************/
#ifndef _NVMAIN_FACTORY_H__
#define _NVMAIN_FACTORY_H__

#include "NVM/nvmain.h"
#include "NVM/Fine_NVMain/FineNVMain.h"
#include "NVM/HierDRAMCache/HierDRAMCache.h"
#include "NVM/RBLA_NVMain/RBLA_NVMain.h"
#include "src/NVMObject.h"
#include "string.h"


namespace NVM
{
	class NVMainFactory
	{
		public:
			static NVMain* CreateNewNVMain( std::string nvmain_type );
		private:
			static NVMain* CreateNVMain( std::string nvmain_type);
	};
};

#endif
