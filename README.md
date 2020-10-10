# HV-Playground
A simple and heavily documented test hypervisor for 64-bit Windows 10 systems running under Intel's VT-x.

This hypervisor was designed to be very simple. To achieve this, it outlines all of it's main operations into the `Driver.c` file (and functions directly above it), and only uses the current processor to conduct it's VMX operation cycle; which consists of running the guest code, catching exits in the VMM, and completing the `DriverEntry` routine. It makes use of definitions from the SDM—all of which are promptly cited to enable the end user to conduct research on their own.

As we move past the initial design of our hypervisor—which is only what's required to run guest code—and begin to add functionality, we enter into a considerably more complicated realm of VMX operation; demonstrating concepts such as MMU virtualization (EPT), root to non-root communication, event injection, and so on. To reduce confusion, and to keep our code base as small and simplistic as possible, each these features are outlined individually into their own implementation (branches) which build upon (fork) the prior designs. In doing this, it is my hope that someone without prior experience developing hypervisors could, with some dedication, understand the concepts demonstrated in our master branch, and then go on to learn about these more complicated concepts in a simple and unrestricted environment.

Below you will find a list of the aforementioned branches.

# Branches
  * [**master**](https://github.com/calware/HV-Playground) - Demonstrates the minimum possible design required to enter into VMX operation and run guest code. The code is designed to run on a single processor (on multiprocessor systems) within a `DriverEntry` function, setup VMX operation, redirect to guest execution, break back to the VMM after executing a halt instruction (as the guest), exit VMX operations, and complete the `DriverEntry` function. 
  * [**GuestState**](https://github.com/calware/HV-Playground/tree/GuestState) *\[Forked from master\]* - Adds code to preserve the guest state across VM exits, code to continue the guest execution, and TraceLogging support to enable debug logging from our VMM.
  * [**EPT**](https://github.com/calware/HV-Playground/tree/EPT) *\[Forked from GuestState\]* - Simplistic EPT configuration supporting (only) 4KB guest pages, designed to only virtualize the required guest memory. Complete with memory management helper routines, this branch also demonstrates modifications to the underlying EPT tables (in addition to splitting attacks) to redirect memory pages exposed to the guest.
  * [**EPTIdentity**](https://github.com/calware/HV-Playground/tree/EPTIdentity) *\[Forked from EPT\]* - EPT configuration designed to support 2MB large pages in an guest-to-host identity map (full system memory virtualization). Also demonstrates *EPT splitting* by selectively splitting target 2MB pages to their 4KB equivalents, and then mapping two separate pages for a taget page (depending upon their accesses).
  * [**EventInjection**](https://github.com/calware/HV-Playground/tree/EventInjection) *\[Forked from GuestState\]* - Demonstrates the simulation of VMX instructions outside VMX operation while within VMX operation. This will be done by modifying the behavior of a `VMLAUNCH` instruciton in our guest (simulating non-root mode), with explanations on how could go about building such a feature in for other instructions which cause non-conditional VM-exits.

# Resources
Below are a list of resources I used when developing the hypervisor seen in this repository (**the master branch**).

  * *Intel® 64 and IA-32 Architectures Software Developer’s Manual (Volume 3C): System Programming Guide - Part 3* — https://software.intel.com/content/dam/develop/public/us/en/documents/326019-sdm-vol-3c.pdf | This includes pretty much all of the information you will need to understand VMX operation.
  * *Intel® 64 and IA-32 architectures software developer's manual combined volumes 2A, 2B, 2C, and 2D: Instruction set reference, A-Z* — https://software.intel.com/content/dam/develop/public/us/en/documents/325383-sdm-vol-2abcd.pdf | This includes most of the general information required to build a hypervisor which isn't contained in Volume 3C, such as information on (and structuring of) the GDT/IDT, segment/system segment descriptors, segment selectors, the CR0/4 and FLAGS layout, and so on.
  * *Intel® 64 and IA-32 architectures software developer's manual (Volume 3D): System Programming Guide - Part 4* — https://software.intel.com/content/dam/develop/public/us/en/documents/332831-sdm-vol-3d.pdf | This includes the appendices referenced by Volume 3C, which include information about VM-exit reasons, exit qualifications, and several other structures which are needed in your code.
  * [Sina Karvandi's](https://twitter.com/Intel80x86) [*Hypervisor From Scratch* series](https://github.com/SinaKarvandi/Hypervisor-From-Scratch)
  * [Daax Rynd's](https://twitter.com/daax_rynd) [*5 Days to Virtualization* series](https://revers.engineering/7-days-to-virtualization-a-series-on-hypervisor-development/)
  * DarthTon's [*HyperBone*](https://github.com/DarthTon/HyperBone)
  * [Alex Ionescu's](https://twitter.com/aionescu) [*SimpleVisor*](https://github.com/ionescu007/SimpleVisor)
  
---

Below are an unordered list of public code repositories that demonstrate implementations of hypervisors running on Intel's VT-x (which is inclusive of several of the above). I would like to point out the first entry in the list ([Petr Beneš'](https://twitter.com/PetrBenes) [*hvpp*](https://github.com/wbenny/hvpp)), as he is very experienced when it comes to the development of hypervisors, and you will find many concepts not accurately expressed in the above resources within Petr's code. [Satoshi Tanda's](https://twitter.com/standa_t) [*HyperPlatform*](https://github.com/tandasat/HyperPlatform) is also a very valuable resource, but is very bloated with features outside the scope of simple hypervisor development.

| Project Name | Description | Author | Project Link | 
|--------------|-------------|--------|--------------|
| hvpp | a lightweight Intel x64/VT-x hypervisor written in C++ focused primarily on virtualization of already running operating system | Petr Beneš |https://github.com/wbenny/hvpp |
| MiniVTx64 | Intel Virtualization Technology demo | zhuhuibeishadiao | https://github.com/zhuhuibeishadiao/MiniVTx64 |
| PFHook | Page fault hook use ept (Intel Virtualization Technology) | zhuhuibeishadiao | https://github.com/zhuhuibeishadiao/PFHook |
| Kernel-Bridge | Windows kernel hacking framework, driver template, hypervisor and API written on C++ | HoShiMin | https://github.com/HoShiMin/Kernel-Bridge |
| MiniVT | 该代码改（chao）编（xi）自看雪_小宝的MiniVT（他写的有点乱，我实在看不懂，自己抄着整理了一遍）。| Sqdwr | https://github.com/Sqdwr/MiniVT |
| HyperPlatform | Intel VT-x based hypervisor aiming to provide a thin VM-exit filtering platform on Windows. | Satoshi Tanda | https://github.com/tandasat/HyperPlatform |
| FU_Hypervisor | A hypervisor hiding user-mode memory using EPT | Satoshi Tanda | https://github.com/tandasat/FU_Hypervisor |
| MiniVisorPkg | The research UEFI hypervisor that supports booting an operating system. | Satoshi Tanda | https://github.com/tandasat/MiniVisorPkg |
| SimpleVisor | SimpleVisor is a simple, portable, Intel VT-x hypervisor with two specific goals: using the least amount of assembly code (10 lines), and having the smallest amount of VMX-related code to support dynamic hyperjacking and unhyperjacking (that is, virtualizing the host state from within the host). It works on Windows and UEFI. | Alex Ionescu | https://github.com/ionescu007/SimpleVisor |
| kHypervisor | kHypervisor is a lightweight bluepill-like nested VMM for Windows, it provides and emulating a basic function of Intel VT-x | Kelvinhack | https://github.com/Kelvinhack/kHypervisor |
| gbhv | Simple x86-64 VT-x Hypervisor with EPT Hooking | Gbps | https://github.com/Gbps/gbhv |
| HyperBone | Minimalistic VT-x hypervisor with hooks | DarthTon | https://github.com/DarthTon/HyperBone |
| VT_64_EPT | createfile | in12hacker | https://github.com/in12hacker/VT_64_EPT |
| hypervisor-for-beginners | Intel Vt-x/EPT based thin-hypervisor for windows with minimum possible code. | rohan popat kumbhar | https://github.com/rohaaan/hypervisor-for-beginners |
| ProtoVirt | An ongoing attempt to create own hypervisior from scratch in linux | Shubham Dubey | https://github.com/shubham0d/ProtoVirt |
| Hypervisor From Scratch | Source code of a multiple series of tutorial about hypervisor | Sina Karvandi | https://github.com/SinaKarvandi/Hypervisor-From-Scratch |
