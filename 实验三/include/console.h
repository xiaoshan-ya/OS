
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

// 为了将一个键作为整体，区分tab和空格
typedef struct Sqstack{
	int ptr; // 栈指针
	int pos[10000]; // 记录每一个光标的位置
	int start;   //esc模式开始时，栈指针的位置
}STK;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置，即一个屏幕开始时候的位置，判断是否滚屏*/
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;			    /* 当前光标位置 */
	unsigned int	start_cursor;		/* 原来光标位置，即进入查找模式时光标的位置 */
    STK pos_stk;			    		/* 存放每一个键的栈 */
}CONSOLE;

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

// 颜色内容: 前四位是背景色，后四位是字色
#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */
#define BLACK_CHAR_TAB 0x06 /* tab的黑底 */
#define RED_CHAR_COLOR	0x04	/* 0000 0100 黑底红字 */
#define RED_CHAR_WS 0x40 /* 空格的红底，红色背景色 */
#define RED_CHAR_TAB 0x46 /* tab的红底黑字，红色背景色 */

#define TAB_WIDTH  4        /*TAB的长度是4*/

#endif /* _ORANGES_CONSOLE_H_ */
