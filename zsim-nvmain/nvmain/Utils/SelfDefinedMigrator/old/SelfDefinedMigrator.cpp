/***************************
 * created on 2015/4/13
 * function: only appliable to single process
 * *************************/

#include "SelfDefinedMigrator.h"
using namespace NVM;

/*
 * construct function:initialize some basic vars
 */
SelfDefinedMigrator::SelfDefinedMigrator()
{
	char file_name[255] = "zero";
	command_addr = 0x300000;
	cmd = new command;
	fname = NULL;
	is_migratable = true;
	CopyChars(fname , file_name);
}

SelfDefinedMigrator::~SelfDefinedMigrator()
{
	std::cout<<"cmd->start addr"<<cmd->start_addr<<std::endl;
	if(fname)
	{
		delete fname ;
		fname = NULL;
	}
	if(cmd)
	{
		delete cmd;
		cmd = NULL;
	}
	munmap(cmd,0x1000);
}

/*
 * set basic vars according to conf object ; and allocate space for placing command
 * 1.CommandAddr: address allocated to place command
 * 2.MmapFileName:mmap file name
 */
void SelfDefinedMigrator::SetConfig(Config* conf , bool createChildren)
{
	uint64_t tmp;
	if(conf->KeyExists("MmapFileName"))
		CopyChars(fname , conf->GetString("MmapFileName").c_str());
	if( conf->KeyExists("CommandAddr"))
	{
		tmp = static_cast<uint64_t>(conf->GetValue("CommandAddr"));

		if(tmp<=0)
			NVM::Fatal("address of command must larger than 0 !");
		else
			command_addr = tmp;
	}

	//create file
	//fd = open(fname , O_CREAT|O_RDWR,00777);
	//if( fd == -1 )
	//	NVM::Fatal("create share file failed!");
	//build memory map
	std::cout<<"attach to command buffer"<<std::endl;
	//cmd = (command*)mmap((void* )command_addr ,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS , -1 , 0 );
}


/*
void SelfDefinedMigrator::Migrate( Addr start_addr , uint64_t op_len , uint64_t op )
{
	NVMAddress start;
	start.SetPhysicalAddress(start_addr);
	Migrate(start , op_len ,op);
}

void SelfDefinedMigrator::Migrate( NVMAddress start_addr , uint64_t op_len , uint64_t op )
{
	//mmap a buffer
	if( !cmd )
	{
		NVM::Fatal("has not allocated command buffer!");	//malloc command buffer failed	
	}
	//fill command to command buffer
	WriteCommand(start_addr , op_len , op);
}
*/

/*
 *read command
 */
command* SelfDefinedMigrator::ReadCommand()
{

	int	fd = open(fname , O_CREAT|O_RDWR,00777);
	read(fd , cmd ,sizeof(command));
	if(cmd->status==VALID )
	{
		uint64_t start_addr = cmd->start_addr;
		
	//	uint64_t op_len = cmd->migrate_len;
	//	uint64_t op_type = cmd->optype;
	//	uint64_t status = cmd->status;		
		std::cout<<"  read start address is:"<<std::hex<<start_addr<<std::endl;
	}
	close(fd);
	return cmd;
}

void SelfDefinedMigrator::Invalidate()
{
	cmd->status = INVALID;
	cmd->migrate_len = 0;
	cmd->optype = 0;
	cmd->start_addr = 0;
	//write(fd,cmd,sizeof(command));
}

bool SelfDefinedMigrator::CopyChars(char* &dest_chars , const char* source_chars)
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

/*
void SelfDefinedMigrator::WriteCommand(NVMAddress start_addr , uint64_t op_len ,uint64_t op_type)
{
	//cmd = dynamic_cast<command*>mmap(command_addr , sizeof(command),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	cmd->start_addr = start_addr;
	cmd->migrate_len = op_len;
	cmd->optype = op_type;
	cmd->status = VALID;	
}*/
