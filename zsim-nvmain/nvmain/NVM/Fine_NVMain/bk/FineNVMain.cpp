/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
* 
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: 
*   Matt Poremba    ( Email: mrp5060 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/
#include "NVM/Fine_NVMain/FineNVMain.h"
#include "src/EventQueue.h"
#include "MemControl/MemoryControllerFactory.h"
#include "traceWriter/TraceWriterFactory.h"
//#include "Decoders/DecoderFactory.h"
#include "Decoders/BufferDecoder/BufferDecoder.h"
#include "include/NVMainRequest.h"
#include "include/NVMHelpers.h"
#include "Prefetchers/PrefetcherFactory.h"
#include "NVM/NVMainFactory.h"
#include <sstream>
#include <cassert>

using namespace NVM;
FineNVMain::FineNVMain( )
{
	std::cout<<"Fine memory"<<std::endl;
	main_memory_ = NULL;
	buffer_memory_ = NULL;
	main_memory_size_ = 0;
	buffer_memory_size_ = 0;
}

FineNVMain::~FineNVMain( )
{
}

void FineNVMain::SetConfig( Config *conf, std::string memoryName, bool createChildren )
{
	std::string main_mem_type = conf->GetString("MainMemType");
	main_memory_ = NVMainFactory::CreateNewNVMain( main_mem_type );
	main_memory_->SetConfig( conf , memoryName , createChildren);
	AddChild( main_memory_ );
	main_memory_->SetParent(this);
	if( conf->KeyExists("BufferConfigFile") )
	{
		Config* buffer_conf = new Config();
		std::string buffer_config_path;
		buffer_config_path = NVM::GetFilePath( config->GetFileName() );
		buffer_config_path += config->GetString("BufferConfigFile");
		buffer_conf->Read(buffer_config_path);
		buffer_memory_ = NVMainFactory::CreateNewNVMain( conf->GetString("BufferMemType"));
		buffer_memory_->SetConfig( buffer_conf , "bufferMemory" , createChildren);
		buffer_memory_size_ = buffer_memory_->GetMemorySize();
		dynamic_cast<BufferDecoder*>(buffer_memory_->GetDecoder())->SetAddrMask( buffer_memory_size_-1);
		AddChild( buffer_memory_ );
		buffer_memory_->SetParent(this);
		buffer_memory_size_ = buffer_memory_->GetMemorySize();
	}
	main_memory_size_ = main_memory_->GetMemorySize();
}

bool FineNVMain::IsIssuable( NVMainRequest *request, FailReason *reason )
{
  if( request->address.GetPhysicalAddress( ) > main_memory_size_)
  {
	 return	buffer_memory_->IsIssuable(request , reason);
  }
  else
	 return  main_memory_->IsIssuable( request , reason);
}

bool FineNVMain::IssueCommand( NVMainRequest *request )
{
	if( request->address.GetPhysicalAddress()>main_memory_size_)
		return buffer_memory_->IssueCommand(request);
	else
		return main_memory_->IssueCommand(request);
}



bool FineNVMain::IssueAtomic( NVMainRequest *request )
{
	if( request->address.GetPhysicalAddress()>main_memory_size_)
 		return buffer_memory_->IssueCommand(request);
	else
		return main_memory_->IssueCommand(request);
}


void FineNVMain::RegisterStats( )
{
	//if( buffer_memory_)
	//	buffer_memory_->RegisterStats();
	//if( main_memory_)
	//	main_memory_->RegisterStats();
}


void FineNVMain::CalculateStats( )
{
	if(buffer_memory_)
		buffer_memory_->CalculateStats();
	if(main_memory_)
		main_memory_->CalculateStats();
}

void FineNVMain::Cycle( ncycle_t steps )
{
	if( buffer_memory_)
		buffer_memory_->Cycle(steps);
	if(main_memory_)
		main_memory_->Cycle(steps);
}

bool FineNVMain::RequestComplete(NVMainRequest* req)
{
	bool rv = false;
	if(req->owner==this)
	{
		delete req;
		rv = true;
	}
	else
		rv = GetParent()->RequestComplete(req);
	return rv;
}
