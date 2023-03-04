
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

//TODO
int strategy = 1; // 设置策略是哪一个
int isReading[5] = {0,0,0,0,0}; // 正在读的状态数组，1表示正在读，0表示没有正在读
int isWriting[5] = {0,0,0,0,0}; // 正在读的状态数组，1表示正在读，0表示没有正在读
int isRest[5] = {1,1,1,1,1}; // 正在休息的状态数组，1表示正在休息，0表示没有正在休息
char state[5] = {'Z','Z','Z','Z','Z'};

PRIVATE void init_tasks()
{
	init_screen(tty_table); // 初始化屏幕
	clean(console_table); // 清屏

	// 表驱动，对应进程0, 1, 2, 3, 4, 5, 6
	int prior[7] = {1, 1, 1, 1, 1, 1, 1};
	for (int i = 0; i < 7; ++i) {
        proc_table[i].ticks    = prior[i];
        proc_table[i].priority = prior[i];
	}

	// initialization
	k_reenter = 0;
	ticks = 0;
	readers = 0;
	writers = 0;
	writing = 0;

	tr = 0;

	strategy = 0; // 切换策略

	p_proc_ready = proc_table;
}
/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
    //TODO
    u8              privilege;
    u8              rpl;
    int             eflags;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
        //TODO
        if (i < NR_TASKS) {     /* 任务 */
            p_task    = task_table + i;
            privilege = PRIVILEGE_TASK;
            rpl       = RPL_TASK;
            eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
        }
        else {                  /* 用户进程 */
            p_task    = user_proc_table + (i - NR_TASKS);
            privilege = PRIVILEGE_USER;
            rpl       = RPL_USER;
            eflags    = 0x202; /* IF=1, bit 2 is always 1 */
        }
                
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid
        //TODO
		p_proc->sleeping = 0; // 初始化结构体新增成员
		p_proc->blocked = 0;
		
		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

        //TODO
		p_proc->nr_tty = 0;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

    //TODO
	init_tasks(); //main.c

	init_clock(); //clock.c，已实现
    init_keyboard(); //keyboard.c

	restart();

	while(1){}
}

// 进入写进程 proc:进程名  slices:时间片 color:颜色
// 在这里判断正在读
PRIVATE read_proc(char proc, int slices, char color){
    switch (proc) { // 正在
        case 'B':
            state[0] = 'O';
            break;
        case 'C':
            state[1] = 'O';
            break;
        case 'D':
            state[2] = 'O';
            break;
    }
	sleep_ms(slices * TIME_SLICE); // 读耗时slices个时间片
    switch (proc) { // 等待
        case 'B':
            state[0] = 'X';
            break;
        case 'C':
            state[1] = 'X';
            break;
        case 'D':
            state[2] = 'X';
            break;
    }

}

// 进入读进程
PRIVATE	write_proc(char proc, int slices, char color){
    switch (proc) { // 正在
        case 'E':
            state[3] = 'O';
            break;
        case 'F':
            state[4] = 'O';
            break;
    }
	sleep_ms(slices * TIME_SLICE); // 写耗时slices个时间片
    switch (proc) { // 等待
        case 'E':
            state[3] = 'X';
            break;
        case 'F':
            state[4] = 'X';
            break;
    }
}

//读写公平方案
void read_gp(char proc, int slices, char color){
    switch (proc) { // 等待
        case 'B':
            state[0] = 'X';
            break;
        case 'C':
            state[1] = 'X';
            break;
        case 'D':
            state[2] = 'X';
            break;
    }

	P(&queue);
        P(&n_r_mutex);
	P(&r_mutex);
	if (readers==0)
		P(&rw_mutex); // 有读者正在使用，写者不可抢占
	readers++;
	V(&r_mutex);
	V(&queue);
	read_proc(proc, slices, color);
	P(&r_mutex);
	readers--;
	if (readers==0)
		V(&rw_mutex); // 没有读者时，可以开始写了
	V(&r_mutex);
    V(&n_r_mutex);
}

void write_gp(char proc, int slices, char color){
    switch (proc) { // 等待
        case 'E':
            state[3] = 'X';
            break;
        case 'F':
            state[4] = 'X';
            break;
    }

	P(&queue);
	P(&rw_mutex);
	writing = 1;
	V(&queue);
	// 写过程
	write_proc(proc, slices, color);
	writing = 0;
	V(&rw_mutex);
}

// 读者优先
void read_rf(char proc, int slices, char color){
    switch (proc) { // 等待
        case 'B':
            state[0] = 'X';
            break;
        case 'C':
            state[1] = 'X';
            break;
        case 'D':
            state[2] = 'X';
            break;
    }
    
    P(&r_mutex);// 请求读进程
    if (readers==0)// 如果没有读进程，请求写进程
        P(&w_mutex);
    readers++;
    V(&r_mutex); // 释放读进程

    P(&n_r_mutex);
    //读文件
    read_proc(proc, slices, color);

    // 结束

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&w_mutex); // 没有读者，可以开始写了，唤醒写者
    V(&r_mutex);
    V(&n_r_mutex);

}

void write_rf(char proc, int slices, char color){
    switch (proc) { // 等待
        case 'E':
            state[3] = 'X';
            break;
        case 'F':
            state[4] = 'X';
            break;
    }
    P(&w_mutex);
    writing = 1;
    // 写过程
    write_proc(proc, slices, color);

    writing = 0;
    V(&w_mutex);
}

// 写者优先
void read_wf(char proc, int slices, char color){
    switch (proc) { // 等待
        case 'B':
            state[0] = 'X';
            break;
        case 'C':
            state[1] = 'X';
            break;
        case 'D':
            state[2] = 'X';
            break;
    }

    P(&n_r_mutex);

    P(&queue);
    P(&r_mutex);
    if (readers==0) // 如果没有读进程请求写进程
        P(&rw_mutex);
    readers++;
    V(&r_mutex);
    V(&queue);

    //读过程开始
    read_proc(proc, slices, color);

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);

    V(&n_r_mutex);
}

void write_wf(char proc, int slices, char color){
    switch (proc) { // 等待
        case 'E':
            state[3] = 'X';
            break;
        case 'F':
            state[4] = 'X';
            break;
    }
    P(&w_mutex);
    // 写过程
    if (writers==0)
        P(&queue);
    writers++;
    V(&w_mutex);

    P(&rw_mutex);
    writing = 1;
    write_proc(proc, slices, color);
    writing = 0;
    V(&rw_mutex);

    P(&w_mutex);
    writers--;
    if (writers==0)
        V(&queue);
    V(&w_mutex);
}

read_f read_funcs[3] = {read_gp, read_rf, read_wf};
write_f write_funcs[3] = {write_gp, write_rf, write_wf};

/*======================================================================*
                               ReporterA
 *======================================================================*/
void ReporterA()
{
    sleep_ms(TIME_SLICE); // 一个时间单位的消耗
    int time = 0; // 时间计数器
    while (time <= 20) {
        //TODO 打印结果
        if (time == 0) {
            sleep_ms(TIME_SLICE);
            time++;
            continue;
        }
        printf("%c%d ", '\06', time++); // 白色字体
        for (int i = 0; i < 5; i++) {
            if (state[i] == 'O') {                                // 正在读或者正在写
                printf("%cO ", '\02'); // 绿色
            }
            else if (state[i] == 'Z') {                          // 正在休息
                printf("%cZ ", '\03'); // 蓝色
            }
            else {                                               // 等待读或等待写
                printf("%cX ", '\01'); // 红色
            }
        }
        printf("\n");
        sleep_ms(TIME_SLICE);
    }
   while (1){}
}

/*======================================================================*
                               ReaderB
 *======================================================================*/
// 读者进程，消耗两个时间片
void ReaderB()
{
	sleep_ms(1*TIME_SLICE);// 进程进入的时间
	while(1){
		read_funcs[strategy]('B', 2, '\02');
        // TODO 在这里判断休息
        state[0] = 'Z';
		sleep_ms(TIME_SLICE); // 休息时间
        state[0] = 'X';
	}
}

/*======================================================================*
                               ReaderC
 *======================================================================*/
// 读者进程，消耗三个时间片
void ReaderC()
{
	sleep_ms(2*TIME_SLICE); // 消耗时间片
	while(1){
		read_funcs[strategy]('C', 3, '\03');
        state[1] = 'Z';
		sleep_ms(TIME_SLICE); // 休息时间
        state[1] = 'X';
	}
}

/*======================================================================*
                               ReaderD
 *======================================================================*/
// 读者进程，消耗三个时间片
void ReaderD()
{
    sleep_ms(3*TIME_SLICE); // 消耗时间片
    while(1){
        read_funcs[strategy]('D', 3, '\03');
        state[2] = 'Z';
        sleep_ms(TIME_SLICE); // 休息时间
        state[2] = 'X';
    }
}

/*======================================================================*
                               WriterE
 *======================================================================*/
void WriterE()
{
	sleep_ms(4*TIME_SLICE);
	while(1){
		write_funcs[strategy]('E', 3, '\05');
        state[3] = 'Z';
		sleep_ms(TIME_SLICE); // 休息时间
        state[3] = 'X';
	}
}

/*======================================================================*
                               WriterF
 *======================================================================*/
void WriterF()
{
    sleep_ms(5*TIME_SLICE);
    while(1){
        write_funcs[strategy]('F', 4, '\05');
        state[4] = 'Z';
        sleep_ms(TIME_SLICE); // 休息时间
        state[4] = 'X';
    }
}
