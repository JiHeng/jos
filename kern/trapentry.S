/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(th0, 0)
TRAPHANDLER_NOEC(th3, 3);  // break point 
TRAPHANDLER(th13, 13);  // general protection 
TRAPHANDLER(th14, 14); // page fault
TRAPHANDLER_NOEC(th_syscall, 0x30); // system call

/*IRQ*/
TRAPHANDLER_NOEC(irq_th0, IRQ_OFFSET + 0);
TRAPHANDLER_NOEC(irq_th1, IRQ_OFFSET + 1);
TRAPHANDLER_NOEC(irq_th2, IRQ_OFFSET + 2);
TRAPHANDLER_NOEC(irq_th3, IRQ_OFFSET + 3);
TRAPHANDLER_NOEC(irq_th4, IRQ_OFFSET + 3);
TRAPHANDLER_NOEC(irq_th5, IRQ_OFFSET + 5);
TRAPHANDLER_NOEC(irq_th6, IRQ_OFFSET + 6);
TRAPHANDLER_NOEC(irq_th7, IRQ_OFFSET + 7);
TRAPHANDLER_NOEC(irq_th8, IRQ_OFFSET + 8);
TRAPHANDLER_NOEC(irq_th9, IRQ_OFFSET + 9);
TRAPHANDLER_NOEC(irq_th10, IRQ_OFFSET + 10);
TRAPHANDLER_NOEC(irq_th11, IRQ_OFFSET + 11);
TRAPHANDLER_NOEC(irq_th12, IRQ_OFFSET + 12);
TRAPHANDLER_NOEC(irq_th13, IRQ_OFFSET + 13);
TRAPHANDLER_NOEC(irq_th14, IRQ_OFFSET + 14);
TRAPHANDLER_NOEC(irq_th15, IRQ_OFFSET + 15);


/*
 * Lab 3: Your code here for _alltraps
 */

 _alltraps:
/*
* here we need to push a trapframe object on the stack and call trap(TrapFrame).
* trapno is pushed by TRAPHANDLER, the rest of the trapframe fields are
* tf_ds, tf_es, tf_regs
*/

    pushl %ds
	pushl %es
	pushal

	/*
	* we need push arg on to the stack. which is the address of the trap frame. 
	* current esp is the address of trap frame 
	*/
	pushl %esp

	/* call trap */
	call trap
