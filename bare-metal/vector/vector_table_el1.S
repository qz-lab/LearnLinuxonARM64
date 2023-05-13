// FILE: vector_table_el1.s
.text
.align 2
// Typical exception vector table code.
//-----------------------------------------------
.balign 0x800 // align to a 2 kilobyte boundary
.global vector_table_el1
vector_table_el1:
curr_el_sp0_sync:
// The exception handler for the synchronous
// exception from the current EL using SP0.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_sp0_irq:
// The exception handler for the IRQ exception
// from the current EL using SP0.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_sp0_fiq:
// The exception handler for the FIQ exception
// from the current EL using SP0.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_sp0_serror:
// The exception handler for the system error
// exception from the current EL using SP0.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_spx_sync:
// The exception handler for the synchronous
// exception from the current EL using the
// current SP.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_spx_irq:
// The exception handler for IRQ exception
// from the current EL using the current SP.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_spx_fiq:
// The exception handler for the FIQ exception
// from the current EL using the current SP.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
curr_el_spx_serror:
// The exception handler for the system error
// exception from the current EL using the
// current SP.
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch64_sync:
// The exception handler for the synchronous
// exception from a lower EL (AArch64).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch64_irq:
// The exception handler for the IRQ exception
// from a lower EL (AArch64).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch64_fiq:
// The exception handler for the FIQ exception
// from a lower EL (AArch64).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch64_serror:
// The exception handler for the system error
// exception from a lower EL(AArch64).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch32_sync:
// The exception handler for the synchronous
// exception from a lower EL(AArch32).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch32_irq:
// The exception handler for the IRQ exception
// from a lower EL (AArch32).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch32_fiq:
// The exception handler for the FIQ exception
// from a lower EL (AArch32).
eret // return from exception

.balign 0x80 // align to next 128 byte boundary
lower_el_aarch32_serror:
// The exception handler for the system error
// exception from a lower EL(AArch32).
eret // return from exception
