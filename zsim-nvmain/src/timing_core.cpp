/** $lic$
 * Copyright (C) 2012-2014 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "timing_core.h"
#include "filter_cache.h"
#include "tlb/common_tlb.h"
#include "zsim.h"

#include "nvmain_mem_ctrl.h"
#include "include/NVMainRequest.h"
#define DEBUG_MSG(args...)
//#define DEBUG_MSG(args...) info(args)

TimingCore::TimingCore(FilterCache* _l1i, FilterCache* _l1d, 
					   BaseTlb* _itlb , BaseTlb* _dtlb,uint32_t _domain,
					   g_string& _name): Core(_name), l1i(_l1i),
					   l1d(_l1d), itlb(_itlb) ,  dtlb(_dtlb) ,
					   instrs(0), curCycle(0), cRec(_domain, _name)
{
	futex_init(&tlb_lock);
}

uint64_t TimingCore::getPhaseCycles() const {
    return curCycle % zinfo->phaseLength;
}

void TimingCore::initStats(AggregateStat* parentStat) {
    AggregateStat* coreStat = new AggregateStat();
    coreStat->init(name.c_str(), "Core stats");

    auto x = [this]() { return cRec.getUnhaltedCycles(curCycle); };
    LambdaStat<decltype(x)>* cyclesStat = new LambdaStat<decltype(x)>(x);
    cyclesStat->init("cycles", "Simulated unhalted cycles");
    coreStat->append(cyclesStat);

    auto y = [this]() { return cRec.getContentionCycles(); };
    LambdaStat<decltype(y)>* cCyclesStat = new LambdaStat<decltype(y)>(y);
    cCyclesStat->init("cCycles", "Cycles due to contention stalls");
    coreStat->append(cCyclesStat);

    ProxyStat* instrsStat = new ProxyStat();
    instrsStat->init("instrs", "Simulated instructions", &instrs);
    coreStat->append(instrsStat);

    parentStat->append(coreStat);
}


void TimingCore::contextSwitch(int32_t gid) {
    if (gid == -1) {
		std::cout<<"context_switch"<<std::endl;
        l1i->contextSwitch();
        l1d->contextSwitch();
		if(itlb)
			itlb->flush_all();
		if(dtlb)
			dtlb->flush_all();
    }
}

void TimingCore::join() {
    DEBUG_MSG("[%s] Joining, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
    curCycle = cRec.notifyJoin(curCycle);
    phaseEndCycle = zinfo->globPhaseCycles + zinfo->phaseLength;
    DEBUG_MSG("[%s] Joined, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
}

void TimingCore::leave() {
    cRec.notifyLeave(curCycle);
}

void TimingCore::loadAndRecord(Address addr) {
    //uint64_t startCycle = curCycle;
	addr = TlbTranslate(addr,false);
    uint64_t startCycle = curCycle;
	//std::cout<<"load:"<<std::hex<<(addr>>12)<<" , srcid:"<<std::hex<<coreId<<std::endl;
    curCycle = l1d->load(addr, curCycle);
    cRec.record(startCycle);
}

void TimingCore::storeAndRecord(Address addr) {
    //uint64_t startCycle = curCycle;
	addr = TlbTranslate(addr,false, true);
    uint64_t startCycle = curCycle;
	//std::cout<<"store:"<<std::hex<<addr<<","<<std::hex<<(addr>>6)<<" , srcid:"<<std::hex<<coreId<<std::endl;
    curCycle = l1d->store(addr, curCycle);
    cRec.record(startCycle);
}

void TimingCore::bblAndRecord(Address bblAddr, BblInfo* bblInfo) {
    instrs += bblInfo->instrs;
    curCycle += bblInfo->instrs;
	Address tmp_addr;
    Address endBblAddr = bblAddr + bblInfo->bytes;
	//bblAddr = TlbTranslate(bblAddr,true);
	//endBblAddr = TlbTranslate(endBblAddr,true);
    for (Address fetchAddr = bblAddr; fetchAddr < endBblAddr; fetchAddr+=(1 << lineBits)) {
        //uint64_t startCycle = curCycle;
		tmp_addr = TlbTranslate( fetchAddr , true );
        uint64_t startCycle = curCycle;
		//std::cout<<"load:"<<std::hex<<addr<<","<<std::hex<<(addr>>6)<<" , srcid:"<<std::hex<<coreId<<std::endl;
        //curCycle = l1i->load(fetchAddr, curCycle);
        curCycle = l1i->load(tmp_addr, curCycle);
        cRec.record(startCycle);
    }
}

uint64_t TimingCore::clflush( Address startAddr, uint64_t startCycle, uint64_t &write_backs )
{
	//std::cout<<"clflush page"<<(startAddr>>6)<<std::endl;
	Address endAddr = startAddr + zinfo->lineNum;
	uint64_t max_cycle = startCycle;
	bool dirty = false;
	write_backs = 0;
	for( Address fetchAddr = startAddr; fetchAddr < endAddr;fetchAddr++)
	{
		
		curCycle = l1i->clflush( fetchAddr, startCycle, true, dirty);
		max_cycle = (max_cycle > curCycle)? max_cycle:curCycle;
		curCycle = l1d->clflush( fetchAddr, startCycle, true , dirty);
		max_cycle = (max_cycle > curCycle)? max_cycle:curCycle;
		if( dirty )
		{
			if( !zinfo->enable_shared_memory)
			{
				NVM::NVMainRequest* req = new NVM::NVMainRequest();
				req->address.SetPhysicalAddress( fetchAddr<<lineBits );
				req->burstCount = 8;
				req->type = NVM::WRITE;
				req->arrivalCycle = curCycle;
				req->isClflush = true;
				NVM::NVMain* parent = dynamic_cast<NVMainMemory*>(zinfo->memoryControllers[0])->GetNVMainPtr();
				req->owner = parent;
				if( parent->IsIssuable(req) )
					parent->IssueCommand(req);
				else
					max_cycle += zinfo->mem_access_time;
			}
			write_backs++;
		}
	}
	if( zinfo->enable_shared_memory && write_backs)
	{
		max_cycle += 60*write_backs;
	}
	curCycle = max_cycle;
	return max_cycle;
}

uint64_t TimingCore::clflush_cacheline( Address startAddr, uint64_t startCycle)
{
	uint64_t max_cycle = startCycle;
	bool is_dirty = false;
	uint64_t cycle = l1i->clflush(startAddr, startCycle, false, is_dirty);
	max_cycle = ( max_cycle > cycle)? max_cycle:cycle;

	cycle = l1d->clflush(startAddr, startCycle, false, is_dirty);
	max_cycle = ( max_cycle > cycle)? max_cycle:cycle;
	return max_cycle;
}

InstrFuncPtrs TimingCore::GetFuncPtrs() {
    return {LoadAndRecordFunc, StoreAndRecordFunc, BblAndRecordFunc, BranchFunc, PredLoadAndRecordFunc, PredStoreAndRecordFunc, FPTR_ANALYSIS, {0}};
}

void TimingCore::LoadAndRecordFunc(THREADID tid, ADDRINT addr) {
    static_cast<TimingCore*>(cores[tid])->loadAndRecord(addr);
}

void TimingCore::StoreAndRecordFunc(THREADID tid, ADDRINT addr) {
    static_cast<TimingCore*>(cores[tid])->storeAndRecord(addr);
}

void TimingCore::BblAndRecordFunc(THREADID tid, ADDRINT bblAddr, BblInfo* bblInfo) {
    TimingCore* core = static_cast<TimingCore*>(cores[tid]);
    core->bblAndRecord(bblAddr, bblInfo);

    while (core->curCycle > core->phaseEndCycle) {
        core->phaseEndCycle += zinfo->phaseLength;
        uint32_t cid = getCid(tid);
		if( cid >= zinfo->numCores)
			std::cout<<"error: cid"<<cid<<std::endl;
        uint32_t newCid = TakeBarrier(tid, cid);
        if (newCid != cid) break; /*context-switch*/
    }
}

//TLB simulation 
ADDRINT TimingCore::TlbTranslate( ADDRINT virtual_addr , bool is_inst,bool is_write)
{
	futex_lock(&tlb_lock);
	//tlb_access_num++;
	BaseTlb* tlb = NULL;
	//instruction tlb
	if( is_inst )
	{
		//std::cout<<name<<".ins vpn:"<<std::hex<<(virtual_addr>>12)<<std::endl;
		tlb = itlb;
	}
	else
	{
		//std::cout<<name<<".data vpn:"<<std::hex<<(virtual_addr>>12)<<std::endl;
		tlb = dtlb;
	}
	if( tlb == NULL)
	{
		//std::cout<<"tlb tranlate: 0x"<<std::hex<<virtual_addr<<std::endl;
		futex_unlock(&tlb_lock);
		return virtual_addr;
	}
	else
	{
	  MemReq req;
	  req.lineAddr = virtual_addr;
	  req.cycle = curCycle;
	  if(is_write)
		req.type = PUTX;
	  else
		req.type = PUTS;
	  debug_printf("access vaddr:%llx",virtual_addr);
	  Address addr = tlb->access(req);
	//Address line_addr= (addr>>6);
	//std::cout<<"process:"<<GetProcIdx()<<" (va:0x"<<std::hex<<virtual_addr<<","<<std::hex<<addr<<")"<<" (vpn: 0x"<<std::hex<<(virtual_addr>>zinfo->page_shift)<<" , lineAddr: 0x"<<std::hex<<line_addr<<" )"<<std::endl;
	  curCycle = req.cycle;	//get updated cycle
	//debug_printf("tlb translate: vaddr:%llx , paddr:%llx",virtual_addr,addr);
	futex_unlock(&tlb_lock);
	return addr;
	}
}

void TimingCore::PredLoadAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred) {
    if (pred) static_cast<TimingCore*>(cores[tid])->loadAndRecord(addr);
}

void TimingCore::PredStoreAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred) {
    if (pred) static_cast<TimingCore*>(cores[tid])->storeAndRecord(addr);
}

