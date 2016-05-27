**<font size=6>Section I. Complementary Experiments</font>**

&#160; &#160; &#160; &#160;We conduct some experiments with 2GHz, 32 cores configuration. Experimental results are shown in Figure 1. Figure 1(a) depicts the normalized performance of HDRC, SHMA-HMDyn, SHMA-Static and a system with 32GB DRAM only, all with respect to a system with 32GB PCM. For all these applications, HDRC only obtains 69.1% performance of the baseline configuration, SHMA-HMDyn, SHMA-Static and a system with 32GB DRAM(the performance upper bound) achieve 15.0%, 14.7% and 22.3% performance improvement on average. Compared to HDRC, SHMA-Static and SHM-HMDyn exhibit 45.9% and 45.6% performance improvement respectively. 

![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/Performance_result.png)  
&#160; &#160; &#160; &#160;<font size=2>**Fig.1 (a) Instruction per cycle normalized to a system with 32GB PCM, without DRAM cache,(b) DRAM cache  
&#160; &#160; &#160; &#160;utilization and(c) average access frequency of every DRAM cache page,normalized to HDRC**</font>


**<font size=2>Table I:Instruction per cycle of selected applications (normalized to a system with 32GB PCM)  
varies with different CPU parameters</font>**
<table class=MsoTableGrid border=1 cellspacing=0 cellpadding=0>
 <tr style='mso-yfti-irow:0;mso-yfti-firstrow:yes;'>
  <td rowspan=2 valign=top style=';border:solid windowtext 1.0pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>App<o:p></o:p></span></p>
  </td>
  <td colspan=3 valign=top style=';border:solid windowtext 1.0pt;
  border-left:none;mso-border-left-alt:solid windowtext .5pt;mso-border-alt:
  solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>4GHz,
  4 cores configuration<o:p></o:p></span></p>
  <p class=MsoNormal align=center style='text-align:center;layout-grid-mode:
  char;mso-layout-grid-align:none'><span lang=EN-US style='font-size:12.0pt;
  font-family:"Times New Roman","serif"'>(Normalized IPC)<o:p></o:p></span></p>
  </td>
  <td colspan=3 valign=top style=';border:solid windowtext 1.0pt;
  border-left:none;mso-border-left-alt:solid windowtext .5pt;mso-border-alt:
  solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>2GHz,
  32 cores configuration<o:p></o:p></span></p>
  <p class=MsoNormal align=center style='text-align:center;layout-grid-mode:
  char;mso-layout-grid-align:none'><span lang=EN-US style='font-size:12.0pt;
  font-family:"Times New Roman","serif"'>(Normalized IPC)<o:p></o:p></span></p>
  </td>
 </tr>
 <tr style='mso-yfti-irow:1;height:7.85pt'>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='mso-bidi-font-size:10.5pt;font-family:"Times New Roman","serif"'>HDRC<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='mso-bidi-font-size:10.5pt;font-family:"Times New Roman","serif"'>SHMA-HMDyn<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='mso-bidi-font-size:10.5pt;font-family:"Times New Roman","serif"'>SHMA-Static<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='mso-bidi-font-size:10.5pt;font-family:"Times New Roman","serif"'>HDRC<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='mso-bidi-font-size:10.5pt;font-family:"Times New Roman","serif"'>SHMA-HMDyn<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='mso-bidi-font-size:10.5pt;font-family:"Times New Roman","serif"'>SHMA-Static<o:p></o:p></span></p>
  </td>
 </tr>
 <tr style='mso-yfti-irow:2'>
  <td valign=top style=';border:solid windowtext 1.0pt;
  border-top:none;mso-border-top-alt:solid windowtext .5pt;mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>Astar<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.30<o:p></o:p></span></p>
  </td>
  <td valign=top style='width:3.0cm;border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;padding:0cm 5.4pt 0cm 5.4pt'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.30<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.33<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.17<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.18<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.18<o:p></o:p></span></p>
  </td>
 </tr>
 <tr style='mso-yfti-irow:3'>
  <td valign=top style=';border:solid windowtext 1.0pt;
  border-top:none;mso-border-top-alt:solid windowtext .5pt;mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>Canneal<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.96<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.27<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.34<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.05<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.16<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.21<o:p></o:p></span></p>
  </td>
 </tr>
 <tr style='mso-yfti-irow:4'>
  <td valign=top style=';border:solid windowtext 1.0pt;
  border-top:none;mso-border-top-alt:solid windowtext .5pt;mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>DICT<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;padding:0cm 5.4pt 0cm 5.4pt'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.58<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.34<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.34<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.67<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.21<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.21<o:p></o:p></span></p>
  </td>
 </tr>
 <tr style='mso-yfti-irow:5'>
  <td valign=top style=';border:solid windowtext 1.0pt;
  border-top:none;mso-border-top-alt:solid windowtext .5pt;mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>KNN<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.23<o:p></o:p></span></p>
  </td>
  <td valign=top style='width:3.0cm;border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.20<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.08<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.31<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.13<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.05<o:p></o:p></span></p>
  </td>
 </tr>
 <tr style='mso-yfti-irow:6;mso-yfti-lastrow:yes'>
  <td valign=top style=';border:solid windowtext 1.0pt;
  border-top:none;mso-border-top-alt:solid windowtext .5pt;mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>BFS<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.18<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.11<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.13<o:p></o:p></span></p>
  </td>
  <td  valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>0.25<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.07<o:p></o:p></span></p>
  </td>
  <td valign=top style=';border-top:none;border-left:none;
  border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  mso-border-top-alt:solid windowtext .5pt;mso-border-left-alt:solid windowtext .5pt;
  mso-border-alt:solid windowtext .5pt;'>
  <p class=MsoNormal style='layout-grid-mode:char;mso-layout-grid-align:none'><span
  lang=EN-US style='font-size:12.0pt;font-family:"Times New Roman","serif"'>1.08<o:p></o:p></span></p>
  </td>
 </tr>
</table>  
<p>
<p>
<p>
<p>

![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/Energy.png)  
&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;<font size=2>**Fig. 2 (a) Power, (b) Energy of HDRC, SHMA-HMDyn, SHMA-Static and a system</font>**  
&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160; &#160; &#160; &#160; &#160;<font size=2>**with 32GB PCM(both are normalized to a system with 32GB DRAM)</font>**  

&#160; &#160; &#160; &#160;To figure out how modifying hardware parameters might change the experimental results, we compare the IPC of each selected application under4GHz, 4cores configuration with 2GHz, 32cores configuration in Table I. We can observe that performance improvement is more remarkable in 4GHz configuration than 2GHz configuration to SHMA. Because speed gap between memory and 4GHz CPUs is more huge than 2GHz CPUs. Reduction of average memory access latency in SHMA makes greater influence on reducing CPUs’ stall time and cores contentions of 4GHz CPUs configuration than 2GHz CPUs configuration. On the other hand, our applications hold little shared data, hence the number of cores impact their performance barely.


&#160; &#160; &#160; &#160;Figure 2 shows normalized power and energy of researched systems configuring with 2GHz CPUs. The system with 32GB DRAM is a baseline. We observe that HDRC, SHMA-HMDyn, SHMA-Static and the system with 32GB PCM achieve 26.4%, 68.0%, 67.9% and 66.8% less energy consumption than the system with 32GB DRAM. For these applications, SHMA-HMDyn and SHMA-Static consume 96.2%, 96.4% energy relative to the system with 32GB PCM, while the energy consumption of HDRC is 221.4%. We can conclude that SHMA and its promoted versions exhibit much more energy efficiency than HDRC when configuring with 2GHz, 32 cores.

&#160; &#160; &#160; &#160;As shown in Figure 1(a), we do our experiment with 2GHz, 32 cores configuration and measure IPC of the system with 32GB DRAM. For all these applications, the system with 32GB DRAM (the performance upper bound) achieves22.3% performance improvement compared to the system with 32GB PCM. Performance gap between SHMA-static (SHMA-HMDyn) and the upper bound is within 5% (6%). We can conclude that SHMA and its promoted versions can achieve good performance in DRAM-NVM hybrid memory architecture.

**<font size=6>Section II Details of Last Level Page Table Entry and TLB Entry</font>**  
&#160; &#160; &#160; &#160;Concrete extended last level page table entry in different paging modes and modified TLB entry of SHMA in MIPS R2000/3000 architecture are shown in Figure 3 and Figure 4 respectively.  

![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/page_table_entry.png)  
&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;<font size=2>**Fig. 3 Extended last level page table entry of SHMA in different paging mode**</font>



![Image of Yaktocat](https://github.com/cyjseagull/SHMA/blob/master/images/TLB_entry.png)  
&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;&#160; &#160; &#160; &#160;<font size=2>**Fig. 4 Modified TLB entry of SHMA in MIPS R2000/3000 architecture**</font>


