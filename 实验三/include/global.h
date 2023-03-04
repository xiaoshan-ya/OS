
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		ticks;

EXTERN	int		disp_pos; // 显存指针
EXTERN	u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		k_reenter;

EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready;

EXTERN	int		nr_current_console;
EXTERN  int mode; //  0: 基本显示 1:查找模式 2:屏蔽输入,遇到ESC退出
extern	PROCESS		proc_table[];
extern	char		task_stack[];
extern  TASK            task_table[];
extern  TASK            user_proc_table[];
extern	irq_handler	irq_table[];
extern	TTY		tty_table[];
extern  CONSOLE         console_table[];


