/******************************
 * created on 2015/4/13
 *
 * ***************************/
#ifndef __SELF_DEFINED_MIGRATOR_H__
#define __SELF_DEFINED_MIGRATOR_H__

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "fcntl.h"
#include "unistd.h"

#include "sys/ipc.h"
#include "sys/shm.h"

#include "include/CommonEnums.h"
#include "include/Exception.h"
#include <queue>

#define CMDBufferSize 512	//512Bytes
namespace NVM{

	struct command
	{
		Addr start_addr;
		uint64_t migrate_len;
		uint64_t optype;
		uint64_t status;	//valid/invalid
	};

	class SelfDefinedMigrator
	{
	 public:
		SelfDefinedMigrator();
		~SelfDefinedMigrator();
		void SetConfig(Config* conf , bool createChildren=true);

		//void ParseCommand(Addr &start_addr,uint64_t &op_len,uint64_t &op_type);
	public:
		//bool CopyChars(char* dest_chars , char* source_chars);
		//void Migrate( Addr start_addr , uint64_t op_len , uint64_t op);
		//void Migrate( NVMAddress start_addr , uint64_t op_len , uint64_t op);

		//void WriteCommand(NVMAddress start_addr , uint64_t op_len , uint64_t op);
		command* ReadCommand();
		bool CopyChars(char* &dest_chars , const char* source_chars);
		void Invalidate();
	public:
		Addr command_addr; 	
	private:
	    
		command* cmd;
		bool is_allocted;	//wheather command buffer is allocated
		bool is_migratable;
		char *fname;
	};
};


#endif
