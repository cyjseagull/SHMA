# SHMA---Software-managed Caching for Hybrid DRAM/NVM Memory Architectures

SHMA is implemented with zsim and NVMain. Hybrid simulator that integrates cycle-accurate main memory simulator for emerging non-volatile memories --NVMain with zsim can be forked from "https://github.com/AXLEproject/axle-zsim-nvmain". 
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
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/Hybrid_Simulator_Architecture.png)

The research leading to these results has received funding from National high technology research and development program(**863 program**) project corpus, in-memory computing system software research and development project


License & Copyright
----------
zsim is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.

zsim was originally written by Daniel Sanchez at Stanford University, and per Stanford University policy, the copyright of this original code remains with Stanford (specifically, the Board of Trustees of Leland Stanford Junior University). Since then, zsim has been substantially modified and enhanced at MIT by Daniel Sanchez, Nathan Beckmann, and Harshad Kasture. zsim also incorporates contributions on main memory performance models from Krishna Malladi, Makoto Takami, and Kenta Yasufuku.

zsim was also modified and enhanced while Daniel Sanchez was an intern at Google. Google graciously agreed to share these modifications under a GPLv2 license. This code is (C) 2011 Google Inc. Files containing code developed at Google have a different license header with the correct copyright attribution.

Additionally, if you use this software in your research, we request that you reference the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the source of the simulator in any publications that use this software, and that you send us a citation of your work.


Setup and Compiling
------------


Memory Management and TLB Simulation Modules
-----------------------
![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/zsim_modification.png)


