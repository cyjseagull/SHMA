#ifndef __FLAT_NVMAIN_H__
#define __FLAT_NVMAIN_H__
#include <string>
#include "NVM/nvmain.h"
#include "src/NVMObject.h"
#include "include/NVMainRequest.h"

namespace NVM
{
	class FlatNVMain: public NVMain
	{
		public:
			FlatNVMain();
			~FlatNVMain();
			virtual void SetConfig( Config* conf, std::string memoryName="MainMemory",
					bool createChildren = true);
			bool IsIssuable( NVMainRequest* request, FailReason* reason=NULL);
			bool IssueCommand( NVMainRequest* req);
			
			void Cycle( ncycle_t steps = 1);
			void CalculateStats();
			
			uint64_t GetMemorySize()
			{ return mem_size;  }

		private:
		   inline void GetAbsolutePath( Config* conf, std::string & re_path);	
		   inline void InitMemory( NVMain* &memory, const char* mem_name, Config* conf);
		private:
			Config* fastMemoryConfig;  //DRAM
			Config* slowMemoryConfig;  //NVM
			//memory controller
			NVMain* fastMemory;
			NVMain* slowMemory; 
			uint64_t mem_size;
			uint64_t fast_mem_size;
			uint64_t slow_mem_size;
			int fast_mem_bits;
			int total_channels;

			bool random;
	};
};
#endif
