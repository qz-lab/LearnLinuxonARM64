// FILE: start.S
// This is the startup code for a Raspberry Pi bare-metal
// program. It assumes that only one CPU core is going
// to be used. The startup code for multiple CPU operation
// would require setting up stacks space for each CPU.

// Stack locations
.equ stack_top, 0x20000000 // Raspberry Pi 2
.equ EL3_stack_top, stack_top
.equ EL2_stack_top, stack_top - 0x2000
.equ EL1_stack_top, stack_top - 0x4000
.equ EL0_stack_top, stack_top - 0x6000

// -----------------------------------------------------------
// The startup code should be loaded by the boot loader.
// The entry point is _start which performs initialization of
// the hardware, then calls a C function.
.section .text.boot // a special section name
.global _start
.func _start
_start: // On reset, we should be in EL3

// For our example, we only want to run on a single
// CPU core, so we cause the others to go to sleep
// waiting for an exception to occur. If an exception
// occurs, it just goes back to sleep
mrs x0, mpidr_el1 // get the CPU ID
and x0, x0, #3 // mask off upper bits
cmp x0, #0 // if not CPU 0
beq nosleep
sleep: wfi // wait for interrupt
b sleep // go back to sleep

nosleep:
// Clear the .bss segment to all zeros
// The __bss_start__ and __bss_end__ symbols are
// defined by the linker.
ldr x1,=__bss_start__ // load pointer to bss and
ldr x2,=__bss_end__ // to byte following bss
mov x3,#0 // load fill value (zero)
bssloop:cmp x1,x2 // Start filling
bge bssdone
str x3,[x1],#8
b bssloop // loop until done
bssdone:
// Now we check the current EL, and initilaize from there down
// to EL0
mrs x0, currentel // read the current EL
lsr x0,x0,#2 // shift it right
cmp x0,#3 // are we in EL3?
blt el2_entry // if not, check for EL2
// initialize EL3 stack pointer
ldr x1,=EL3_stack_top
mov sp,x1
// Set up vector table for EL3
ldr x1, =vector_table_el3
msr VBAR_EL3, x1
// Initialize SCTLR_EL2 and HCR_EL2 to safe values before
// entering EL2.
msr SCTLR_EL2, XZR
msr HCR_EL2, XZR
// Set the EL2 execution state.
mrs x0, SCR_EL3
orr x0, x0, #(1<<10) // RW: EL2 Execution state is AArch64
orr x0, x0, #(1<<0) // NS: EL1 is non-secure
msr SCR_EL3, x0
mov x0, #0b01001 // DAIF=0000
msr SPSR_EL3, x0 // M[4:0]=01001 EL2h must match SCR_EL3.RW
// Perform EL2 entry.
adr x0, el2_entry // el2_entry points to the first
msr ELR_EL3, x0 // instruction of EL2 code.
eret

el2_entry:
mrs x0, currentel // read the current EL
lsr x0,x0,#2 // shift it right
cmp x0,#2 // are we in EL2?
blt el1_entry // if not, check for EL1
// initialize EL2 stack pointer
ldr x1,=EL2_stack_top
mov sp,x1
// Set up vector table for EL2
ldr x1, =vector_table_el2
msr VBAR_EL2, x1
// Initialize the SCTLR_EL1 register before entering EL1.
msr SCTLR_EL1, XZR
// Set the EL1 Execution state.
mrs x0, HCR_EL2
orr x0, x0, #(1<<31) // RW: EL1 Execution state is AArch64.
msr HCR_EL2, x0
mov x0, #0b00101 // DAIF=0000
msr SPSR_EL2, x0 // M[4:0]=00101 EL1h must match HCR_EL2.RW
adr x0, el1_entry // el1_entry points to the first
msr ELR_EL2, x0 // instruction of EL1 code.
eret

el1_entry:
mrs x0, currentel // read the current EL
lsr x0,x0,#2 // shift it right
cmp x0,#1 // are we in EL1?
blt el0_entry // if not, we must be in EL0
// initialize EL1 stack pointer
ldr x1,=EL1_stack_top
mov sp,x1
// Set up vector table for EL1
ldr x1, =vector_table_el1
msr VBAR_EL1, x1
// initialize EL0 stack pointer
ldr x1,=EL0_stack_top
msr sp_EL0,x1
// Initialize the SCTLR_EL0 register before entering EL1.
mov x0,#(1<<16) // Allow EL0 to use wfi instruction
orr x0,x0,#(1<<18) // Allow EL0 to use wfe instruction
msr SCTLR_EL1, x0
// Set the EL0 Execution state.
mov x0, #0b00000 // DAIF=0000 M[4:0]=00000 EL0t.
msr SPSR_EL1, x0
adr x0, el0_entry // el0_entry points to the first
msr ELR_EL1, x0 // instruction of EL0 code.
eret

el0_entry:
// Once we are in EL0, the only way to go back up is by causing
// an exception, which will invoke an exception handler.

// Call the Main function
//bl main

// If main ever returns, hang forever
hang: b hang // this should never happen
.size _start, . - _start
.endfunc
