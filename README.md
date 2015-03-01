Dynamic Recompiler
===================

This project is a standalone dynamic recompiler to convert from MIPS R4300 (N64) to ARMv6 running Linux. 

My objective is to produce a dynamic recompiler that will replace the existing ARM implementation in mupen64plus.

To achieve this, I am working to a design that makes it easier to optimize code during 'translation' from MIPS to ARM code. 
I am also trying to make the code easier to follow through modulerization and comments for those that have an interest in learning about recompilers. I have also written a debugger that can manually translate blocks of code and dissasemble ARM and MIPS instructions to assist further development. 

The project can be built on x86 machines and one can use the debugger to test most of the recompilers functionality however the generated ARM code can only be executed on ARM machines.

My long term plan is to add in ARMv7 and NEON optimizations followed by the addion of MIPS R5900 support (for PlayStation2).
