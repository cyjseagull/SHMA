#include "NVM/Flat_NVMain/FlatNVMain.h"
#include "NVM/NVMainFactory.h"
#include "include/NVMHelpers.h"
using namespace std;
using namespace NVM;

FlatNVMain::FlatNVMain(): fastMemoryConfig(NULL),slowMemoryConfig(NULL),
	fastMemory(NULL), slowMemory(NULL),mem_size(0),fast_mem_size(0),
	slow_mem_size(0), fast_mem_bits(0)
{
	random = false;
	accessed_fast_pages.clear();
	accessed_slow_pages.clear();
	cout<<"create flat nvmain"<<endl;
}

FlatNVMain::~FlatNVMain()
{
	if(fastMemoryConfig)
		delete fastMemoryConfig;
	if(slowMemoryConfig)
		delete slowMemoryConfig;
	if(fastMemory)
		delete fastMemory;
	if(slowMemory)
		delete slowMemory;
}


void FlatNVMain::SetConfig( Config* conf, std::string memoryName,
		bool createChildren)
{
	if( conf->KeyExists("RANDOM"))
		random = conf->GetValue("RANDOM");
	//init config
	if( conf->KeyExists("FAST_CONFIG"))
	{
		fastMemoryConfig = new Config();
		string fast_mem_config_file = conf->GetString("FAST_CONFIG");
		GetAbsolutePath(conf, fast_mem_config_file);
		cout<<"fast memory config path is:"<<fast_mem_config_file<<endl;
		fastMemoryConfig->Read( fast_mem_config_file );
	}
	if( conf->KeyExists("SLOW_CONFIG"))
	{
		slowMemoryConfig = new Config();
		string slow_mem_config_file = conf->GetString("SLOW_CONFIG");
		GetAbsolutePath(conf, slow_mem_config_file);
		slowMemoryConfig->Read(slow_mem_config_file);
	}
	cout<<"init fast memory"<<endl;
	//init main memory
	if( fastMemoryConfig )
		InitMemory( fastMemory,"FastMemory(DRAM)",fastMemoryConfig);
	cout<<"init slow memory"<<endl;
	if( slowMemoryConfig )
		InitMemory( slowMemory, "SlowMemory(NVM)" ,slowMemoryConfig);
	fast_mem_size = fastMemory->GetMemorySize();
	slow_mem_size = slowMemory->GetMemorySize();
	mem_size = fastMemory->GetMemorySize() + slowMemory->GetMemorySize();
	
	fast_mem_bits = NVM::mlog2( fast_mem_size )+1;
	total_channels = fastMemory->GetChannels() + slowMemory->GetChannels();
	memory_slice = fast_mem_size/total_channels;
	cout<<"fast memory size:"<<(fast_mem_size/(1024*1024*1024))<<" GB"<<endl;
	cout<<"slow memory size:"<<(slow_mem_size/(1024*1024*1024))<<" GB"<<endl;
	cout<<"total memory size:"<<(mem_size/(1024*1024*1024))<<" GB"<<endl;
	cout<<"fast memory bits:"<<fast_mem_bits<<endl;
	cout<<"total channels:"<<total_channels<<endl;
}

inline void FlatNVMain::InitMemory( NVMain* &memory, 
		const char* mem_name, Config* conf )
{
	memory = NVMainFactory::CreateNewNVMain( conf->GetString("CMemType"));
	EventQueue* queue = new EventQueue();
	memory->SetParent(this);
	memory->SetEventQueue(queue);
	GetGlobalEventQueue()->AddSystem( memory, conf);
	memory->SetConfig( conf, mem_name, true);
	memory->RegisterStats();
	RegisterStats();
	AddChild(memory);
}

bool FlatNVMain::IsIssuable( NVMainRequest* req, FailReason* reason)
{
	uint64_t addr = req->address.GetPhysicalAddress();
	uint64_t page_num = addr >> (uint64_t)12;
	uint64_t addr_org = addr;
	uint64_t dram_addr = (addr >> (uint64_t)fast_mem_bits);
	bool is_issuable = false;
	req->media = SLOW_MEM;
	if( !dram_addr )
	{
		int unit_num = addr/memory_slice;
		//access DRAM
		if( unit_num%2 == 1 )
		{
			addr -= (((unit_num-1)/2+1)*memory_slice);
			req->address.SetPhysicalAddress(addr);
			uint64_t org = req->address.GetVirtualAddress();
			req->address.SetVirtualAddress(addr_org);
			req->media = FAST_MEM;
			is_issuable = fastMemory->IsIssuable(req);
			if( is_issuable == false)
				std::cout<<"fast memory can't issue"<<std::endl;
			return is_issuable;
		}
		//access NVM
		else
		{
			addr -= ((unit_num/2)*memory_slice);
			req->address.SetPhysicalAddress(addr);
			uint64_t org = req->address.GetVirtualAddress();
			req->address.SetVirtualAddress(addr_org);
			is_issuable= slowMemory->IsIssuable(req);
			if( is_issuable == false)
				std::cout<<"slow memory can't issue"<<std::endl;
			return is_issuable;
		}
	}
	req->address.SetPhysicalAddress( addr-2*fast_mem_size);
	req->address.SetVirtualAddress(addr_org);
	is_issuable = slowMemory->IsIssuable(req);
	if( is_issuable == false)
		std::cout<<"slow memory can't issue"<<std::endl;
	return is_issuable;
}

bool FlatNVMain::IssueCommand( NVMainRequest* req)
{
	uint64_t addr = req->address.GetPhysicalAddress();
	uint64_t page_num = addr >> (uint64_t)12;
	uint64_t addr_org = addr;
	uint64_t dram_addr = (addr >> (uint64_t)fast_mem_bits);
	req->media = SLOW_MEM;
	if( !dram_addr )
	{
		int unit_num = addr/memory_slice;
		//access DRAM
		if( unit_num%2 == 1 )
		{
			addr -= (((unit_num-1)/2+1)*memory_slice);
			req->address.SetPhysicalAddress(addr);
			uint64_t org = req->address.GetVirtualAddress();
			req->address.SetVirtualAddress(addr_org);
			if( !accessed_fast_pages.count(page_num))
			{
				accessed_fast_pages.insert(page_num);
			}
			if( fastMemory->IsIssuable(req) )
			{
				req->media = FAST_MEM;
				//if( req->is_migrated )
					//std::cout<<"translate DRAM "<<(addr_org>>6)<<"to "<<(addr>>6)<<std::endl;
				return fastMemory->IssueCommand(req);
			}
			else
			{
				std::cout<<"fast memory:can't issue command "<<req->address.GetPhysicalAddress()<<" org:"<<req->address.GetVirtualAddress()<<" oldest:"<<org<<std::endl;
			}
		}
		//access NVM
		else
		{
			if( !accessed_slow_pages.count(page_num))
			{
				accessed_slow_pages.insert(page_num);
			}
			addr -= ((unit_num/2)*memory_slice);
			req->address.SetPhysicalAddress(addr);
			uint64_t org = req->address.GetVirtualAddress();
			req->address.SetVirtualAddress(addr_org);
			//std::cout<<"translate NVM "<<(addr_org>>6)<<"to "<<(addr>>6)<<std::endl;
			if( slowMemory->IsIssuable(req))
				return slowMemory->IssueCommand(req);
			else
				std::cout<<"slow memory: can't issue command "<<req->address.GetPhysicalAddress()<<" org:"<<req->address.GetVirtualAddress()<<"oldest:"<<org<<std::endl;
		}
	}
	req->address.SetPhysicalAddress( addr-2*fast_mem_size);
	req->address.SetVirtualAddress(addr_org);
	if( slowMemory->IsIssuable(req) )
	{
		return slowMemory->IssueCommand(req);
	}
	else
		std::cout<<"slow memory: can't issuable2 "<<req->address.GetPhysicalAddress()<<" org:"<<req->address.GetVirtualAddress()<<std::endl;
	return true;
}

void FlatNVMain::Cycle( ncycle_t steps)
{
	fastMemory->Cycle(steps);
	slowMemory->Cycle(steps);
	GetEventQueue()->Loop(steps);
}

void FlatNVMain::CalculateStats()
{
	fastMemory->CalculateStats();
	slowMemory->CalculateStats();
	totalReadRequests = fastMemory->totalReadRequests + 
		slowMemory->totalReadRequests;
	totalWriteRequests = fastMemory->totalWriteRequests +
		slowMemory->totalWriteRequests;
	accessed_dram_pages = accessed_fast_pages.size();
	accessed_nvm_pages = accessed_slow_pages.size();
	dram_usage = (double)accessed_dram_pages*1024/(double)fast_mem_size;
	/*successfulPrefetches = fastMemory->successfulPrefetches
		+ slowMemory->successfulPrefetches;
	unsuccessfulPrefetches = fastMemory->unsuccessfulPrefetches
		+ slowMemory->unsuccessfulPrefetches;*/
}

inline void FlatNVMain::GetAbsolutePath( Config* conf, string & conf_path)
{
	if( conf_path[0] !='/')
		conf_path = NVM::GetFilePath( conf->GetFileName())+conf_path;
}
