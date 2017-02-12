**Example Configurations of SHMA**
* **dram.cfg**: 32GB 1333MHZ DDR3 DRAM, 10.7GB/Sec, FR-FCFS request scheduling, (tCAS-tRCD-tRP-tRAS: 7-7-7-18 cycles)
* **pcm.cfg**: 32GB 1333MHZ PCM, 10.7GB/Sec,FR-FCFS request scheduling, (tCAS-tRCD-tRP-tRAS:9-37-100-53)
* **flat.cfg**: 1GB 1333MHZ DDR3 DRAM and 32GB 1333MHZ PCM, both DRAM and PCM are managed by OS as the main memory.
* **hdrc.cfg**: 1GB 1333MHZ DDR3 DRAM and 32GB 1333MHZ PCM, DRAM is the cache of PCM, it's managed by hardware, PCM is the main memory, it's allocated and reclaimed by OS.
* **rbla.cfg**: 1GB 1333MHZ DDR3 DRAM and 32GB 1333MHZ PCM, optimized version of flat, OS migrates PCM pages with low row buffer hit rate to DRAM to get performance benefit.
* **shma-dyn.cfg**: 1GB 1333MHZ DDR3 DRAM and 32GB 1333MHZ PCM, DRAM is the cache of PCM, both DRAM and PCM are managed by OS. SHMA provides DRAM cache bypass, cache filtering mechanisms to improve DRAM cache efficiency.
* **shma-static.cfg**: 1GB 1333MHZ DDR3 DRAM and 32GB 1333MHZ PCM, SHMA with static fitering threshold.
* **nvmain-config**: this directory covers the nvmain configuration files needed by dram.cfg, pcm.cfg,etc.
* **bench_simpoints**: examples of simpoint files, include simpoints files of astar, bfs,  bzip2,KNN, mcf,MSF, NBODY, omnetpp and SetCover (you can create simpoints of other benchmarks according to the main page illustrations).

($SIMPOINT_DIR and $CPUSPECPATH are environment variables, you must export them in your shells if you use them, or you can just specify absolute path of simpoint files and executable files of specified benchmarks.)
* **$SIMPOINT_DIR**: directory of simpoint files;
* **$CPUSPECPATH_DIR**: directory of SPEC CPU2006 (you can replace it with absolute path);

 

Happy hacking and hope you find SHMA useful for hybrid memory architecture research.