#ifndef JIVE_I386_MACHINE_H
#define JIVE_I386_MACHINE_H

#include <jive/machine.h>

extern const jive_machine jive_i386_machine;
extern const jive_cpureg_class jive_i386_regcls[];
extern const jive_instruction_class jive_i386_instructions[];

typedef enum {
	jive_i386_flags = 0,
	jive_i386_gpr = 1,
	jive_i386_gpr_byte = 2,	/* registers that are byte-addressible */
	jive_i386_gpr_eax = 3,	
	jive_i386_gpr_ebx = 4,
	jive_i386_gpr_ecx = 5,	
	jive_i386_gpr_edx = 6,
	jive_i386_gpr_esi = 7,
	jive_i386_gpr_edi = 8,
	jive_i386_gpr_esp = 9,
	jive_i386_gpr_ebp = 10,
} jive_i386_regcls_index;

/* integer register index */
typedef enum {
	jive_i386_cc = 0,
	jive_i386_eax = 1,
	jive_i386_ecx = 2,
	jive_i386_ebx = 3,
	jive_i386_edx = 4,
	jive_i386_esi = 5,
	jive_i386_edi = 6,
	jive_i386_ebp = 7,
	jive_i386_esp = 8,
} jive_i386_reg_index;

typedef enum {
	jive_i386_ret = 0,
	
	jive_i386_int_load_imm = 1,
	jive_i386_int_load32_disp = 2,
	jive_i386_int_store32_disp = 3,
	
	jive_i386_int_add = 13,
	jive_i386_int_sub = 14,
	jive_i386_int_and = 15,
	jive_i386_int_or = 16,
	jive_i386_int_xor = 17,
	jive_i386_int_mul = 18,
	
	jive_i386_int_transfer = 19
} jive_i386_instruction_index;


#endif
