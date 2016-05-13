# SHMA---Software-managed Caching for Hybrid DRAM/NVM Memory Architectures

SHMA is implemented with zsim and NVMain. Hybrid simulator that integrates cycle-accurate main memory simulator for emerging non-volatile memories --NVMain with zsim can be forked from "https://github.com/AXLEproject/axle-zsim-nvmain". 
Comparing to zsim-nvmain hybrid simulator, SHMA has achieved following functions:

 * **Implemented memory management simulations(such as MemoryNode, Zone, BuddyAllocator etc.)**:   Considering that pin-based zsim only replays virtual address into simulation architecture, and 
doesn support OS simulation, SHMA has added memory management simulation into zsim, including 
memory node, zone and buddy allocator.


 * **TLB simulation:** Original zsim-nvmain hybrid simulator has no simulation of TLB, since SHMA has added memory management modules into zsim, TLB simulation is implemented in zsim accordingly to accelerate address translation procedure for virtual address to physical address.

 
 * **Implementation of SHMA, a hierarchical hybrid DRAM/NVM memory system that brought DRAM caching issues into software level:** DRAM cache is managed by hardware totally in tranditional    DRAM-NVM hierarchical hybrid systems, SHMA is based on a novel software-managed cache mechanism that organizes NVM and DRAM in a flat physical address space while logically supporting a hierarchical memory architecture, this design has brought DRAM caching issues into software level.Besides, SHMA only caches hot pages into DRAM cache to reduce cache pollution and bandwidth waste between DRAM cache and NVM main memory.
 
 
 * **Multiple DRMA-NVM hybrid architecture supports:** Support both DRAM-NVM flat-addressable hybrid 
memory architecuture and DRAM-NVM hierarchical hybrid architecture.
![Image of Yaktocat]()
 
 *  **Multiple DRAM-NVM hybrid system optimization policies:** 


I'm no good at writing sample / filler text, so go write something yourself.

Look, a list!

 * foo
 * bar
 * baz

And here's some code! :+1:

```javascript
$(function(){
  $('div').html('I am a div.');
});
```

This is [on GitHub](https://github.com/jbt/markdown-editor) so let me know if I've b0rked it somewhere.


Props to Mr. Doob and his [code editor](http://mrdoob.com/projects/code-editor/), from which
the inspiration to this, and some handy implementation hints, came.

### Stuff used to make this:

 * [markdown-it](https://github.com/markdown-it/markdown-it) for Markdown parsing
 * [CodeMirror](http://codemirror.net/) for the awesome syntax-highlighted editor
 * [highlight.js](http://softwaremaniacs.org/soft/highlight/en/) for syntax highlighting in output code blocks
 * [js-deflate](https://github.com/dankogai/js-deflate) for gzipping of data to make it fit in URLs
