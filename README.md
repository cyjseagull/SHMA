# SHMA---Software-managed Caching for Hybrid DRAM/NVM Memory Architectures

&#160; &#160; &#160; &#160;SHMA is implemented with zsim and NVMain. Hybrid simulator that integrates cycle-accurate main memory simulator for emerging non-volatile memories --NVMain with zsim can be forked from "https://github.com/AXLEproject/axle-zsim-nvmain". 
Comparing to zsim-nvmain hybrid simulator, SHMA has achieved following functions:

 * **Implemented memory management simulations(such as MemoryNode, Zone, BuddyAllocator etc.)**:   Considering that pin-based zsim only replays virtual address into simulation architecture, and 
doesn support OS simulation, SHMA has added memory management simulation into zsim, including 
memory node, zone and buddy allocator.


 * **TLB simulation:** Original zsim-nvmain hybrid simulator has no simulation of TLB, since SHMA has added memory management modules into zsim, TLB simulation is implemented in zsim accordingly to accelerate address translation procedure for virtual address to physical address.

 
 * **Implementation of SHMA, a hierarchical hybrid DRAM/NVM memory system that brought DRAM caching issues into software level:** DRAM cache is managed by hardware totally in tranditional    DRAM-NVM hierarchical hybrid systems, SHMA is based on a novel software-managed cache mechanism that organizes NVM and DRAM in a flat physical address space while logically supporting a hierarchical memory architecture, this design has brought DRAM caching issues into software level.Besides, SHMA only caches hot pages into DRAM cache to reduce cache pollution and bandwidth waste between DRAM cache and NVM main memory.
 
 
 * **Multiple DRMA-NVM hybrid architecture supports:** Support both DRAM-NVM flat-addressable hybrid memory architecuture and DRAM-NVM hierarchical hybrid architecture.As shown in following picture,both DRAM and NVM are used as main memory and managed by OS uniformly in DRAM-NVM flat-addressable hybrid architecture. In DRAM-NVM hierarchical hybrid memory architecture, DRAM is exploited as cache of NVM, hardware-assisted hit-judgement used to determine whether data hits in DRAM cache is necessary in this architecutre. Besides, to reduce hardware overhead, DRAM cache is organized set-associative and uses Demand-based caching policy.
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/DRAM-NVM_architectures.png)
 
 
 *  **Multiple DRAM-NVM hybrid system optimization policies:** We have implemented Row Buffer Locality Aware(RBLA) Migrating policy and MultiQueue-based(MultiQueue) Migrating policy in DRAM-NVM flat addressable hybrid memory system. RBLA Migrating policy is a simple implementation of hybrid memory system proposed in thesis "**Row Buffer Locality Aware Caching Policies for Hybrid Memories**", MultiQueue Migrating policy is a simple implementation of thesis "**Page Placement in Hybrid Memory Systems**". RBLA Migrating policy is aimed at migrating NVM pages with bad row buffer locality to DRAM since row buffer miss of NVM pages pay more overhead than row buffer miss of DRAM pages, and row buffer hit of NVM pages gains more performance than row buffer hit of DRAM pages.MultiQueue Migrating policy migrates hot NVM pages into DRAM, hotness of a page is measured by both time locality and access frequency, MQ algorithm is used to update hotness of pages.


Modules and architecture of hybrid simulator are shown as following:
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/simulator_architecture.png)

The research leading to these results has received funding from National high technology research and development program(**863 program**) project corpus, in-memory computing system software research and development project


Origianl License & Copyright of zsim
--------------------
zsim is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.

zsim was originally written by Daniel Sanchez at Stanford University, and per Stanford University policy, the copyright of this original code remains with Stanford (specifically, the Board of Trustees of Leland Stanford Junior University). Since then, zsim has been substantially modified and enhanced at MIT by Daniel Sanchez, Nathan Beckmann, and Harshad Kasture. zsim also incorporates contributions on main memory performance models from Krishna Malladi, Makoto Takami, and Kenta Yasufuku.

zsim was also modified and enhanced while Daniel Sanchez was an intern at Google. Google graciously agreed to share these modifications under a GPLv2 license. This code is (C) 2011 Google Inc. Files containing code developed at Google have a different license header with the correct copyright attribution.

Additionally, if you use this software in your research, we request that you reference the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the source of the simulator in any publications that use this software, and that you send us a citation of your work.


License & Copyright of SHMA (CGCL & SCTS Lab of HUST)
-------------------------
SHMA was extended by Yujie Chen, Dong Liu at Cluster and Grid Computing Lab & Services Computing Technology and System Lab(CGCL & SCTS Lab) of Huazhong University of Science and Technology, the copyright of this SHMA remains with CGCL & SCTS Lab of Huazhong University of Science and Technology.


Setup,Compiling and Configuration
------------
**1.External Dependencies**  
&#160; &#160; &#160; &#160;Before install hybrid simulator zsim-nvmain, it's essential that you have already install dependencies listing below.
* gcc(>=4.6)
* Intel Pin Toolkit(Already integrates in project, or you can download from [pin-2.13-61206-gcc.4.4.7-linux](http://software.intel.com/sites/landingpage/pintool/downloads/pin-2.13-61206-gcc.4.4.7-linux.tar.gz))
* [boost](http://www.boost.org/users/history/version_1_59_0.html) (version 1.59 is ok)
* [libconfig](http://www.hyperrealm.com/libconfig/libconfig-1.5.tar.gz)
* [hdf5](https://www.hdfgroup.org/ftp/HDF5/releases/)


**2.Compiling**
* update environment script env.sh according to your machine configuration
```javascript
#!/bin/sh
PINPATH= path of pin_kit
NVMAINPATH= path of nvmain
ZSIMPATH= path of zsim-nvmain
BOOST= path of boost
LIBCONFIG= path of libconfig
HDF5=path of hdf5
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PINPATH/intel64/lib:$PINPATH/intel64/runtime:$PINPATH/intel64/lib:$PINPATH/intel64/lib-ext:$BOOST/lib:$HDF5/lib:$LIBCONFIG:/lib
INCLUDE=$INCLUDE:$HDF5/include:$LIBCONFIG:/include
LIBRARY_PATH=$LIBRARY_PATH:$HDF5/lib
CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$HDF5/include
export ZSIMPATH PINPATH NVMAINPATH LD_LIBRARY_PATH BOOST CPLUS_INCLUDE_PATH LIBRARY_PATH
```
* compiling and Installation
```javascript
[root @node1 SHMA]# cd zsim-nvmain
[root @node1 zsim-nvmain]# source env.sh  //init environmental values
[root @node1 zsim-nvmain]# scons -j16    //compiling, -j16 represents that compiling with 16 cores
```


**3.zsim Configuration Keys** (example zsim configuration files is in zsim-nvmain/config directory)
* **Enable TLB、Page Table and Memory Management Simulation**  
**(1) sys.tlbs.tlb_type**: type of TLB, default is "CommonTlb","HotMonitorTlb" enables SHMA policy;  
**(2) sys.tlbs.itlb(dtlb): prefix for configuring instruction/data TLB**  
① **entry_num**: Number of TLB entries, default is 128;  
② **hit_lantency**: Latency(cycles) of TLB hit, default is 1cycle;  
③ **response_latency**: TLB response latency(cycles) to CPU, default is 1cycle;  
④ **evict_policy**: evict policy, default is "LRU";  
**(3) sys.pgt_walker( page table walker configuration)**  
① mode: paging mode configuration, SHMA supports seven paging modes, namely, **Legacy_Normal**(4GB address space, page size is 4KB), **Legacy_Huge**(4GB address space, page size is 4MB), **PAE_Normal**(64GB address space, page size is 4KB),**PAE_Huge**(64GB address space, page size is 2MB),**LongMode_Normal**(address length is 48 bits,page size is 4KB), **LongMode_Middle**(address length is 48 bits, page size is 2MB) and **LongMode_Huge**（address length is 48bits, page size is 1GB);  
② itlb: instruction TLB name corresponding to this page table walker;  
③ dtlb: name of data TLB corresponding to this page table walker;  
**(4) sys.mem.zone: memory management configuration** 
**zone_dma/zone_dma32/zone_normal/zone_highmem**: set OS zone size(MB)


* **SHMA(Software-Managed DRAM Cache) Related Configuration**(example in zsim-nvmain/config/shma.cfg)  
**(1) sys.tlbs.tlb_type**: must be set to be "HotMonitorTlb";  
**(2) sys.init_access_threshold**: set initial value of fetching_threshold, default is 0;  
**(3) sys.adjust_interval**: period of adjusting fetching_threshold automatically, defalut is 10000000 cycles (1000cycles is basic units);


**4.nvmain Configuration Keys** (example nvmain configuration files is in zsim-nvmain/config/nvmain-config directory)
* **Enabling DRAM-NVM hierarchical hybrid architecture**( zsim-nvmain/config/nvmain-config/hierarchy)  
**(1) EventDriven**: true;  
**(2) CMemType**: set physical memory type, is **HierDRAMCache** in hardware managed DRAM Cache hybrid memory system;  
**(3) MM_Config**: configuration file of NVM main memory;  
**(4) DRC_CHANNEL**: configuration file of DRAM Cache;  


*  **Enabling SHMA(software-managed DRAM Cache) policy in DRAM-NVM hierarchical hybrid architecture**(zsim-nvmain/config/nvmain-config/shma)  
**(1) EventDriven**:true;  
**(2) ReservedChannels**: number of DRAM cache channels;  
**(3) CONFIG_DRAM_CHANNEL**: configuration file of every DRAM cache channel;  
**(4) CONFIG_CHANNEL**: configuration files of every NVM main memory channel;  
**(5) CMemType**: physical memory type, is **FineNVMain** in SHMA;  
**(6) DRAMBufferDecoder**: DRAM cache decoder type, is **BufferDecoder** in SHMA;


*  **Enabling RBLA policy in DRAM-NVM hybrid architecture**(zsim-nvmain/config/nvmain-config/rbla)  
**(1) EventDriven**:true;  
**(2) Decoder**: physical decoder object, is **Migrator** in RBLA;  
**(3) PromotionChannel**: channel id of fast memory (DRAM);  
**(4) CMemType**: physical memory type, is **RBLANVMain** in RBLA;  
**(5) CONFIG_CHANNEL**: configuration file path of every main memory channel;  


*  **Enabling MultiQueue policy in DRAM-NVM hybrid architecture**(zsim-nvmain/config/nvmain-config/mq)  
**(1) EventDriven**: true;  
**(2) AddHook**: hook type, is "MultiQueueMigrator" in MultiQueue policy based hybrid memory system;  
**(3) Decoder**: decoder type, is "MQMigrator" in MultiQueue policy based hybrid memory system;  
**(4) PromotionChannel**: channel id of fast memory(DRAM);  
**(5) CONFIG_CHANNEL**: configuration file path of every main memory channel;  


TLB, Page Table and Memory Management Simulation Modules
-----------------------
&#160; &#160; &#160; &#160;As described above, original zsim doesn't support OS simulation, and SHMA has added TLB, page table and memory management simulation into zsim, main modification is shown as following picture. The left side marks major code of original zsim corresponding to system simulation, **the right side marks SHMA modifications to zsim for TLB, page table and memory management simulation support.**
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/zsim_modification.png)

Architecture of SHMA(software-managed DRAM Caching)
---------------------------
&#160; &#160; &#160; &#160; SHMA has extended both page table and TLB to maintain both mappings from virtual address to physical address and physical address to DRAM cache address, this has brought DRAM cache management into software level, so that DRAM cache can be exploited fully. Besides, SHMA adopts utility-based DRAM caching policy that only fetching hot pages into DRAM cache when its memory pressure in high state to reduce DRAM cache pollution. SHMA supports DRAM cache directly bypass,too. Following picture is the architecture of SHMA.![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/SHMA_architecture.png)

Implementations of RBLA and MultiQueue Policies
----------------------------
* **Row Buffer Locality Aware Migrator (RBLA)**  
&#160; &#160; &#160; &#160;RBLA migrates NVM pages with bad row buffer locality to DRAM, and reserve pages with good row buffer locality in NVM to gain benefit from row buffer hit in NVM and reduce overhead caused by row buffer miss in NVM. Its implementation is shown as following picture:![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/RBLA.png)


* **hot page migrator based on MultiQueue Alogrithm (MultiQueue)**  
&#160; &#160; &#160; &#160;MultiQueue classify NVM pages into hot pages and cold pages using multiqueue algorithm accroding to both page access frequency and time locality. Its implementation is shown as following picture:![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/MultiQueue.png)


Happy hacking and hope you find SHMA useful for hybrid memory architecture research.


