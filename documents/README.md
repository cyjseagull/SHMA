**<font size=5>Section I. Complementary Experiments</font>**

&#160; &#160; &#160; &#160;We conduct some experiments with 2GHz, 32 cores configuration. Experimental results are shown in Figure 1. Figure 1(a) depicts the normalized performance of HDRC, SHMA-HMDyn, SHMA-Static and a system with 32GB DRAM only, all with respect to a system with 32GB PCM. For all these applications, HDRC only obtains 69.1% performance of the baseline configuration, SHMA-HMDyn, SHMA-Static and a system with 32GB DRAM(the performance upper bound) achieve 15.0%, 14.7% and 22.3% performance improvement on average. Compared to HDRC, SHMA-Static and SHM-HMDyn exhibit 45.9% and 45.6% performance improvement respectively. 

![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/Performance_result.png)  
Fig.1 (a) Instruction per cycle normalized to a system with 32GB PCM, without DRAM cache   
&#160; &#160; &#160; &#160;&#160; &#160;(b) DRAM cache utilization and  
&#160; &#160; &#160; &#160;&#160; &#160;(c) average access frequency of every DRAM cache page, normalized to HDRC

![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/Energy.png)

&#160; &#160; &#160; &#160;To figure out how modifying hardware parameters might change the experimental results, we compare the IPC of each selected application under4GHz, 4cores configuration with 2GHz, 32cores configuration in Table I. We can observe that performance improvement is more remarkable in 4GHz configuration than 2GHz configuration to SHMA. Because speed gap between memory and 4GHz CPUs is more huge than 2GHz CPUs. Reduction of average memory access latency in SHMA makes greater influence on reducing CPUs’ stall time and cores contentions of 4GHz CPUs configuration than 2GHz CPUs configuration. On the other hand, our applications hold little shared data, hence the number of cores impact their performance barely.  


&#160; &#160; &#160; &#160;Figure 2 shows normalized power and energy of researched systems configuring with 2GHz CPUs. The system with 32GB DRAM is a baseline. We observe that HDRC, SHMA-HMDyn, SHMA-Static and the system with 32GB PCM achieve 26.4%, 68.0%, 67.9% and 66.8% less energy consumption than the system with 32GB DRAM. For these applications, SHMA-HMDyn and SHMA-Static consume 96.2%, 96.4% energy relative to the system with 32GB PCM, while the energy consumption of HDRC is 221.4%. We can conclude that SHMA and its promoted versions exhibit much more energy efficiency than HDRC when configuring with 2GHz, 32 cores.

&#160; &#160; &#160; &#160;As shown in Figure 1(a), we do our experiment with 2GHz, 32 cores configuration and measure IPC of the system with 32GB DRAM. For all these applications, the system with 32GB DRAM (the performance upper bound) achieves22.3% performance improvement compared to the system with 32GB PCM. Performance gap between SHMA-static (SHMA-HMDyn) and the upper bound is within 5% (6%). We can conclude that SHMA and its promoted versions can achieve good performance in DRAM-NVM hybrid memory architecture.

**<font size=5>Details of Last Level Page Table Entry and TLB Entry</font>**
&#160; &#160; &#160; &#160;Concrete extended last level page table entry in different paging modes and modified TLB entry of SHMA in MIPS R2000/3000 architecture are shown in Figure 3 and Figure 4 respectively.


![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/DRAM-NVM_architectures.png)



