R4300-code-analysis
===================

A standalone dynamic recompiler / analysis tool for R4300 (N64 roms). 
Currently it can load N64 roms, analyze the MIPS instructions and 
generate some ARMv6 code.


Tasks to complete:

v Fix Register number translations.

- Build 'Branch to Uncompiled' code/DR code.

- Build cc_interrupt() code/DR code.

- Build MIPS TRAP code/DR code.

- Build Virtual address lookup and TLB code/DR code.

- Finish translations of standard instructions.

- Finish ARM decompiler

- Add MIPS FPU instructions. The Mupen64plus demo file does not use most of 
  the available FPU functions but these heavn't been written yet.

- Refactor badly named variables. 

- Check Comments.

- Investigate setting up DR code space in 'C' code region. 
  Currently Recompiled code is placed at 0x81000000 but if it could 
  be placed in shared object (.so) code region then we could branch to 
  C functions rather than loading a register with address and lr register
  with pc before jumping.

- Utilize r12 register.

- Graft into Mupen64plus.
