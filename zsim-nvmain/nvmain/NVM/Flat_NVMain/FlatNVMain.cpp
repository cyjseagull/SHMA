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
	
	fast_mem_bits = NVM::mlog2( fast_mem_size );
	
	total_channels = fastMemory->GetChannels() + slowMemory->GetChannels();
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
	AddChild(memory);
}

bool FlatNVMain::IsIssuable( NVMainRequest* req, FailReason* reason)
{
	uint64_t addr = req->address.GetPhysicalAddress();
	uint64_t dram_addr = (addr >>(uint64_t)fast_mem_bits);
	if( !dram_addr)
	{	
		if( addr % total_channels == 0)
			return fastMemory->IsIssuable(req);
		else
			return slowMemory->IsIssuable(req);
	}
	return slowMemory->IsIssuable(req);
}

bool FlatNVMain::IssueCommand( NVMainRequest* req)
{
	uint64_t addr = req->address.GetPhysicalAddress();
	uint64_t dram_addr = (addr >> (uint64_t)fast_mem_bits);
	//access fast memory
	if( !dram_addr )
	{
		if( addr % total_channels == 0 )
		{
			if( fastMemory->IsIssuable(req) )
				return fastMemory->IssueCommand(req);
		}
		else
		{
			if( slowMemory->IsIssuable(req))
				return slowMemory->IssueCommand(req);
		}
	}
	if( slowMemory->IsIssuable(req) )
	{
		return slowMemory->IssueCommand(req);
	}
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
}

inline void FlatNVMain::GetAbsolutePath( Config* conf, string & conf_path)
{
	if( conf_path[0] !='/')
		conf_path = NVM::GetFilePath( conf->GetFileName())+conf_path;
}
