{\rtf1\ansi\ansicpg936\deff0\nouicompat{\fonttbl{\f0\fnil\fcharset134 \'cb\'ce\'cc\'e5;}{\f1\fnil\fcharset1 Segoe UI Symbol;}}
{\colortbl ;\red0\green0\blue255;}
{\*\generator Riched20 10.0.15063}\viewkind4\uc1 
\pard\sa200\sl276\slmult1\f0\fs22\lang2052 # HSCC---Hardware/Software Cooperative Caching for Hybrid DRAM/NVM Memory Architectures\par
\par
&#160; &#160; &#160; &#160;HSCC is implemented with zsim and NVMain simulators.  Zsim is a fast x86-64 multi-core simulator. It exploits Intel Pin toolkit to collect traces of memory accesses for processes, and replays the traces in the zsim simulator. NVMain is a cycle-accurate memory simulator, it models components of DRAM and NVMs, and memory hierarchy in detail. The integrated "zsim + NVMain" simulators can be forked from "{{\field{\*\fldinst{HYPERLINK https://github.com/AXLEproject/axle-zsim-nvmain }}{\fldrslt{https://github.com/AXLEproject/axle-zsim-nvmain\ul0\cf0}}}}\f0\fs22 ". \par
\par
Based on the "zsim + NVMain" hybrid simulator, HSCC has added the following functions:\par
\par
 * **Memory management simulations (such as MemoryNode, Zone, Buddy allocator etc.)**:   As the pin-based zsim only replays virtual address in the x86 system architecture, and does not support OS simulation, HSCC has added memory management modules into zsim, including memory node, zone and buddy allocator.\par
\par
\par
 * **TLB simulation:** The original "zsim + NVMain" simulator does not simulate the TLB. TLB simulation is implemented in zsim to accelerate address translations from virtual address to physical address.\par
\par
 \par
 * **Implementation of HSCC, a hierarchical hybrid DRAM/NVM memory system that pushes DRAM caching management issues into the software level:** DRAM cache is managed by hardware in tranditional DRAM/NVM hierarchical hybrid systems. HSCC is a novel software-managed cache mechanism that organizes NVM and DRAM in a flat physical address space while logically supporting a hierarchical memory architecture. This design simplifies the hardware design by pushing the burden of DRAM cache management to the software layers. Besides, HSCC only caches hot NVM pages into the DRAM cache to mitigate potential cache thrashing and bandwidth waste between DRAM cache and NVM main memory.\par
 \par
 \par
 * **Multiple DRMA/NVM hybrid architecture supports:** HSCC supports both DRAM/NVM flat-addressable hybrid memory architecuture and DRAM/NVM hierarchical hybrid architecture. As shown in following figure, both DRAM and NVM are used as main memory and managed by OS in a single flat address space. In DRAM/NVM hierarchical hybrid memory architecture, DRAM is exploited as a cache to the NVM, and hardware-assisted hit-judgement mechanism is implemented to determine whether data is hit in DRAM cache. Besides, to reduce hardware overhead, DRAM cache is organized set-associative and usually uses demand-based caching policies.\par
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/DRAM-NVM_architectures.png)\par
 \par
 \par
 * \~**Multiple DRAM/NVM hybrid system optimization policies:** We have implemented Row Buffer Locality Aware (RBLA) page caching policy and MultiQueue-based (MultiQueue) page migration policy in DRAM/NVM flat addressable hybrid memory system. RBLA caching policy is a simple implementation of hybrid memory system proposed in the paper "**Row Buffer Locality Aware Caching Policies for Hybrid Memories**", MultiQueue migration policy is a simple implementation of system proposed in the paper "**Page Placement in Hybrid Memory Systems**". RBLA caching policy is aimed at migrating NVM pages with poor row buffer locality to DRAM since row buffer miss of NVM pages incur higher overhead than that of DRAM pages. The MultiQueue migration policy places hot NVM pages into DRAM, and MQ algorithm is used to update the hotness of pages based on time locality and access frequency.\par
\par
\par
The architecture and modules of HSCC are shown in the following figure:\par
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/simulator_architecture.png)\par
\par
\par
Origianl License & Copyright of zsim\par
--------------------\par
zsim is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.\par
\par
zsim was originally written by Daniel Sanchez at Stanford University, and per Stanford University policy, the copyright of this original code remains with Stanford (specifically, the Board of Trustees of Leland Stanford Junior University). Since then, zsim has been substantially modified and enhanced at MIT by Daniel Sanchez, Nathan Beckmann, and Harshad Kasture. zsim also incorporates contributions on main memory performance models from Krishna Malladi, Makoto Takami, and Kenta Yasufuku.\par
\par
zsim was also modified and enhanced while Daniel Sanchez was an intern at Google. Google graciously agreed to share these modifications under a GPLv2 license. This code is (C) 2011 Google Inc. Files containing code developed at Google have a different license header with the correct copyright attribution.\par
\par
Additionally, if you use this software in your research, we request that you reference the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the source of the simulator in any publications that use this software, and that you send us a citation of your work.\par
\par
\par
License & Copyright of HSCC ([HUST SCTS & CGCL Lab](http://grid.hust.edu.cn/))\par
-------------------------\par
HSCC is implemented by Yujie Chen, Dong Liu and Haikun Liu at Cluster and Grid Computing Lab & Services Computing Technology and System Lab in Huazhong University of Science and Technology([HUST SCTS & CGCL Lab](http://grid.hust.edu.cn/)), the copyright of this HSCC remains with CGCL & SCTS Lab of Huazhong University of Science and Technology.\par
\par
## Citing HSCC\par
\par
If you use HSCC, please cite our reearch paper published at ICS 2017, included as doc/HSCC.pdf.\par
\par
**Haikun Liu, Yujie Chen, Xiaofei Liao, Hai Jin, Bingsheng He, Long Zhen and Rentong Guo, Hardware/Software Cooperative Caching for Hybrid DRAM/NVM Memory Architectures, in: Proceedings of the 31st International Conference on Supercomputing (ICS'17), Chicago, IL, USA, June 14-16, 2017**\par
```javascript\par
@inproceedings\{Liu:2017:HCC:3079079.3079089,\par
 author = \{Liu, Haikun and Chen, Yujie and Liao, Xiaofei and Jin, Hai and He, Bingsheng and Zheng, Long and Guo, Rentong\},\par
 title = \{Hardware/Software Cooperative Caching for Hybrid DRAM/NVM Memory Architectures\},\par
 booktitle = \{Proceedings of the International Conference on Supercomputing\},\par
 series = \{ICS 2017\},\par
 year = \{2017\},\par
 isbn = \{978-1-4503-5020-4\},\par
 location = \{Chicago, Illinois\},\par
 pages = \{26:1--26:10\},\par
 articleno = \{26\},\par
 numpages = \{10\},\par
 url = \{{{\field{\*\fldinst{HYPERLINK http://doi.acm.org/10.1145/3079079.3079089 }}{\fldrslt{http://doi.acm.org/10.1145/3079079.3079089\ul0\cf0}}}}\f0\fs22\},\par
 doi = \{10.1145/3079079.3079089\},\par
 acmid = \{3079089\},\par
 publisher = \{ACM\},\par
 address = \{New York, NY, USA\},\par
 keywords = \{caching, hybird memory, non-volatile memory (NVM)\},\par
\}\par
```\par
\par
Setup,Compiling and Configuration\par
------------\par
**1.External Dependencies**  \par
&#160; &#160; &#160; &#160;Before install hybrid simulator zsim-nvmain, it's essential that you have already install dependencies listing below.\par
* gcc(>=4.6)\par
* Intel Pin Toolkit(Already integrates in project, or you can download from [pin-2.13-61206-gcc.4.4.7-linux](http://software.intel.com/sites/landingpage/pintool/downloads/pin-2.13-61206-gcc.4.4.7-linux.tar.gz))\par
* [boost](http://www.boost.org/users/history/version_1_59_0.html) (version 1.59 is ok)\par
* [libconfig](http://www.hyperrealm.com/libconfig/libconfig-1.5.tar.gz)\par
* [hdf5](https://www.hdfgroup.org/ftp/HDF5/releases/)\par
\par
\par
**2.Compiling**\par
* Update environment script env.sh according to your machine configuration\par
```javascript\par
#!/bin/sh\par
PINPATH= path of pin_kit\par
NVMAINPATH= path of nvmain\par
ZSIMPATH= path of zsim-nvmain\par
BOOST= path of boost\par
LIBCONFIG= path of libconfig\par
HDF5=path of hdf5\par
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PINPATH/intel64/lib:$PINPATH/intel64/runtime:$PINPATH/intel64/lib:$PINPATH/intel64/lib-ext:$BOOST/lib:$HDF5/lib:$LIBCONFIG:/lib\par
INCLUDE=$INCLUDE:$HDF5/include:$LIBCONFIG:/include\par
LIBRARY_PATH=$LIBRARY_PATH:$HDF5/lib\par
CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$HDF5/include\par
export ZSIMPATH PINPATH NVMAINPATH LD_LIBRARY_PATH BOOST CPLUS_INCLUDE_PATH LIBRARY_PATH\par
```\par
* Compiling and Installation\par
```javascript\par
[root @node1 HSCC]# cd zsim-nvmain\par
[root @node1 zsim-nvmain]# source env.sh  //init environmental values\par
[root @node1 zsim-nvmain]# scons -j16    //compiling, -j16 represents that compiling with 16 cores\par
```\par
If error "could not exec $PINPATH/intel64(ia32)/bin/pinbin" happens, it means that you are not authorized to execute pinbin, this can be solved with the following command:\par
```javascript\par
[root @node1 zsim-nvmain]# chmod a+x $PINPATH/intel64(ia32)/bin/pinbin \par
```\par
\par
* **Using a virtual machine**  \par
 If you use another OS, can't make system-wide configuration changes, or just want to test zsim without modifying your system, you can run zsim on a Linux VM. We have included a vagrant configuration file ({{\field{\*\fldinst{HYPERLINK http://vagrantup.com }}{\fldrslt{http://vagrantup.com\ul0\cf0}}}}\f0\fs22 ) that will provision an Ubuntu 12.04 VM to run zsim. You can also follow this Vagrantfile to figure out how to setup zsim on an Ubuntu system. Note that zsim will be much slower on a VM because it relies on fast context-switching, so we don't recommend this for purposes other than testing and development. Assuming you have vagrant installed (sudo apt-get install vagrant on Ubuntu or Debian), follow these steps:\par
Copy the Vagrant file to the zsim root folder, boot up and provision the base VM with all dependencies, then ssh into the VM.\par
```javascript\par
[root @node1 zsim-nvmain]# cp misc/Vagrantfile .\par
[root @node1 zsim-nvmain]# vagrant up\par
[root @node1 zsim-nvmain]# vagrant ssh\par
```\par
Vagrant automatically syncs the zsim root folder of your host machine to /vagrant/ on the guest machine. Now that you're in the VM, navigate to that synced folder, and simply build and use zsim (steps 5 and 6 above)\par
```javascript\par
[root @node1 zsim-nvmain]# cd cd /vagrant/\par
[root @node1 zsim-nvmain]# scons -j4\par
```\par
\par
\par
**3.zsim Configuration Keys** (example zsim configuration files is in zsim-nvmain/config directory)\par
* **Enable TLB\'a1\'a2Page Table and Memory Management Simulation**  \par
**(1) sys.tlbs.tlb_type**: type of TLB, default is "CommonTlb","HotMonitorTlb" enables HSCC policy;  \par
**(2) sys.tlbs.itlb(dtlb): prefix for configuring instruction/data TLB**  \par
\f1\u9312?\f0\lang1033  **entry_num**: Number of TLB entries, default is 128;  \par
\f1\u9313?\f0  **hit_lantency**: Latency(cycles) of TLB hit, default is 1cycle;  \par
\f1\u9314?\f0  **response_latency**: TLB response latency(cycles) to CPU, default is 1cycle;  \par
\f1\u9315?\f0  **evict_policy**: evict policy, default is "LRU";  \par
**(3) sys.pgt_walker( page table walker configuration)**  \par
\f1\u9312?\f0  mode: paging mode configuration, HSCC supports seven paging modes, namely, **Legacy_Normal**(4GB address space, page size is 4KB), **Legacy_Huge**(4GB address space, page size is 4MB), **PAE_Normal**(64GB address space, page size is 4KB),**PAE_Huge**(64GB address space, page size is 2MB),**LongMode_Normal**(address length is 48 bits,page size is 4KB), **LongMode_Middle**(address length is 48 bits, page size is 2MB) and **LongMode_Huge**\'a3\'a8address length is 48bits, page size is 1GB);  \par
\f1\u9313?\f0  itlb: instruction TLB name corresponding to this page table walker;  \par
\f1\u9314?\f0  dtlb: name of data TLB corresponding to this page table walker;   \par
\f1\u9315?\f0  reversed_pgt:  true, enable  reversed  page table; false, disable  reversed page table; when simulating single process, default is false; while simulating multiple processes, default is true;\par
\par
**(4) sys.mem.zone: memory management configuration** \par
**zone_dma/zone_dma32/zone_normal/zone_highmem**: set OS zone size(MB)  \par
**(5) sys.enable_shared_memory**: true, enable shared memory simulation ( default is true )\par
\par
* **Enable Simpoints**    \par
**(1) configuration key**  \par
startFastForwarded=true   \par
simPoints=directory of simpoints  \par
**(2) how to get simpoints**  \par
create .bb files with valgrind: cmd is execution command of the executable programs   \par
```javascript\par
 valgrind --tool=exp-bbv --interval-size=<instructions of a simpoint,example:1000000000> <cmd> \par
```  \par
get simpoints with .bb files with SimPoint(get from {{\field{\*\fldinst{HYPERLINK https://github.com/southerngs/simpoint }}{\fldrslt{https://github.com/southerngs/simpoint\ul0\cf0}}}}\f0\fs22 )  \par
```javascript\par
simpoint -k <simpoints num> -loadFVFile <path of .bb files> -saveSimpoints <file store generated simpoints> -saveSimpointWeights <file store weights of generated simpoints> -sampleSize <instructions of a simpoint, eg:1000000000>\par
```\par
**(3) format of simpoints**  \par
(the first simpoint period) 0  \par
(the second simpoint period) 1  \par
... ...  \par
(the ith simpoint period) i-1  \par
... ...  \par
example( simpoint file of msf with 31 simpoints):\par
```javascript\par
38 0\par
19 1\par
64 2\par
13 3\par
58 4\par
43 5\par
55 6\par
10 7\par
14 8\par
39 9\par
15 10\par
30 11\par
9 12\par
42 13\par
24 14\par
4 15\par
0 16\par
12 17\par
48 18\par
0 21\par
1 22\par
2 23\par
3 24\par
4 25\par
5 26\par
6 27\par
7 28\par
8 29\par
9 30\par
```   \par
\par
* **HSCC Related Configuration**(example in zsim-nvmain/config/shma.cfg)  \par
**(1) sys.tlbs.tlb_type**: must be set to be "HotMonitorTlb";  \par
**(2) sys.init_access_threshold**: set initial value of fetching_threshold, default is 0;  \par
**(3) sys.adjust_interval**: period of adjusting fetching_threshold automatically, defalut is 10000000 cycles (1000cycles is basic units);\par
**(4) sys.mem_access_time**: cycles of per memory access caused by page table walking;\par
**4.nvmain Configuration Keys** (example nvmain configuration files is in zsim-nvmain/config/nvmain-config directory)\par
* **Enabling DRAM-NVM hierarchical hybrid architecture**( zsim-nvmain/config/nvmain-config/hierarchy)  \par
**(1) EventDriven**: true;  \par
**(2) CMemType**: set physical memory type, is **HierDRAMCache** in hardware managed DRAM Cache hybrid memory system;  \par
**(3) MM_Config**: configuration file of NVM main memory;  \par
**(4) DRC_CHANNEL**: configuration file of DRAM Cache;  \par
\par
\par
*  **Enabling HSCC policy in DRAM-NVM hierarchical hybrid architecture**(zsim-nvmain/config/nvmain-config/shma)  \par
**(1) EventDriven**:true;  \par
**(2) ReservedChannels**: number of DRAM cache channels;  \par
**(3) CONFIG_DRAM_CHANNEL**: configuration file of every DRAM cache channel;  \par
**(4) CONFIG_CHANNEL**: configuration files of every NVM main memory channel;  \par
**(5) CMemType**: physical memory type, is **FineNVMain** in HSCC;  \par
**(6) DRAMBufferDecoder**: DRAM cache decoder type, is **BufferDecoder** in HSCC;\par
\par
\par
*  **Enabling RBLA policy in DRAM-NVM hybrid architecture**(zsim-nvmain/config/nvmain-config/rbla)  \par
**(1) EventDriven**:true;  \par
**(2) Decoder**: physical decoder object, is **Migrator** in RBLA;  \par
**(3) PromotionChannel**: channel id of fast memory (DRAM);  \par
**(4) CMemType**: physical memory type, is **RBLANVMain** in RBLA;  \par
**(5) CONFIG_CHANNEL**: configuration file path of every main memory channel;  \par
\par
\par
*  **Enabling MultiQueue policy in DRAM-NVM hybrid architecture**(zsim-nvmain/config/nvmain-config/mq)  \par
**(1) EventDriven**: true;  \par
**(2) AddHook**: hook type, is "MultiQueueMigrator" in MultiQueue policy based hybrid memory system;  \par
**(3) Decoder**: decoder type, is "MQMigrator" in MultiQueue policy based hybrid memory system;  \par
**(4) PromotionChannel**: channel id of fast memory(DRAM);  \par
**(5) CONFIG_CHANNEL**: configuration file path of every main memory channel;  \par
\par
* **Enabling Flat DRAM-NVM hybrid   architectures**(zsim-nvmain/config/nvmain-config/rbla)  \par
**(1) FAST_CONFIG**: configuration file of fast memory (eg DRAM)  \par
**(2) SLOW_CONFIG**: configuration file of slow memory (eg NVM)  \par
**(3) Decoder**: ***FlatDecoder**, decoder of flat DRAM-NVMA hybrid architecture  \par
**(4) CMemType**:**FlatRBLANVMain**, memory type  \par
\par
\par
TLB, Page Table and Memory Management Simulation Modules\par
-----------------------\par
&#160; &#160; &#160; &#160;As described above, original zsim doesn't support OS simulation, and HSCC has added TLB, page table and memory management simulation into zsim. The major modifications are shown in the following figure. The left side presents the major code of original zsim corresponding to system simulation, **the right side describes HSCC modifications to zsim for TLB, page table and memory management simulation support.**\par
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/zsim_modification.png)\par
\par
Architecture of HSCC\par
---------------------------\par
&#160; &#160; &#160; &#160; HSCC has extended both page table and TLB to maintain both virtual-to-NVM and NVM-to-DRAM address mappings. HSCC also develops an utility-based DRAM caching policy that only fetching hot pages into DRAM cache when the DRAM is under high pressure to reduce DRAM cache pollution. HSCC also supports DRAM cache bypassing mechanism. The following figure shows the architecture of HSCC.![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/SHMA_architecture.png)\par
\par
Implementations of RBLA and MultiQueue Policies\par
----------------------------\par
* **Row Buffer Locality Aware Migrator (RBLA)**  \par
&#160; &#160; &#160; &#160;RBLA migrates NVM pages with poor row buffer locality to DRAM, and keeps pages with good row buffer locality in NVM to gain the benefit of row buffer hit. The implementation is shown in the following figure:![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/RBLA.png)\par
\par
\par
* **hot page migrator based on MultiQueue Alogrithm (MultiQueue)**  \par
&#160; &#160; &#160; &#160;MultiQueue classifies NVM pages into hot pages and cold pages using multiqueue algorithm accroding to both page access frequency and time locality. Its implementation is shown in the following figure:![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/MultiQueue.png)\par
\par
* **Architecture of flat memory supporting different channel configurations of DRAM and NVM**  \par
&#160; &#160; &#160; &#160;Considering that DRAM and NVM with different channel configurations have the overlapping address space in the low end, we divide the continuous overlapped address space into \{channel_nums\} and mapping them to different address space interleavingly to make full use of channel parallization.\par
\par
## Support or Contact\par
HSCC is developed at SCTS&CGCL Lab ({{\field{\*\fldinst{HYPERLINK http://grid.hust.edu.cn/ }}{\fldrslt{http://grid.hust.edu.cn/\ul0\cf0}}}}\f0\fs22 ) by Yujie Chen, Haikun Liu and Xiaofei Liao. For any questions, please contact Yujie Chen(yujiechen_hust@163.com), Haikun Liu (hkliu@hust.edu.cn) and Xiaofei Liao (xfliao@hust.edu.cn). \lang2052\par
}
 