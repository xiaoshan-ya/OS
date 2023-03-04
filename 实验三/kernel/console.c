
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void clearHighlightWord(CONSOLE* p_con);
PRIVATE void clearColor(CONSOLE* p_con);
PRIVATE void flush(CONSOLE* p_con);
PUBLIC void exit_esc(CONSOLE* p_con); //TODO
PUBLIC void search(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
void push(CONSOLE *pConsole, unsigned int cursor); //TODO

unsigned int pop(CONSOLE *pConsole);

// 在init_tty中被调用，初始化屏幕
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   exit_esc
*======================================================================*/
void clearHighlightWord(CONSOLE* p_con) { //TODO
    u8* p_v_mem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    int len = p_con->cursor - p_con->start_cursor; // 仅得到高亮字符串长度
    for (int i = 0; i < len; i++) {
        *(p_v_mem - 2 - 2 * i) = ' ';
        *(p_v_mem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
    }
}

void clearColor(CONSOLE* p_con) { //TODO
    for (int i = 0; i < p_con->start_cursor * 2; i += 2) {
        *(u8*)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
    }
}

// 退出ESC，删除关键字，文本恢复白色
PUBLIC void exit_esc(CONSOLE* p_con) { //TODO

	//把关键字清除
    clearHighlightWord(p_con);
	//退出时，红色的都变回白色
    clearColor(p_con);
	//光标恢复到进入esc模式之前，ptr也复原
	p_con->cursor = p_con->start_cursor;
	p_con->pos_stk.ptr = p_con->pos_stk.start;

	flush(p_con);  //设置好要刷新才能显示
}


/*======================================================================*
			   search_key_word
*======================================================================*/
// 查找并显示高亮关键字
PUBLIC void search(CONSOLE* p_con) { // TODO
    // 此时p_con->start_cursor为刚进入
	int highlightLength = p_con->cursor * 2 - p_con->start_cursor * 2; // 高亮字符串占显存的长度, 即字符内容+颜色内容
    int srcLength = p_con->start_cursor * 2 - 2; // 普通模式下字符的长度

    // 滑动匹配
	for (int i = 0; i < p_con->start_cursor * 2; i += 2) {
        int end = i;
        int judge = 1; // 1表示匹配成功
        int judgeWS = 0; // 0表示不是空格

        for (int j = 0; j < highlightLength; j += 2) {
            if (end > srcLength) { // 排除越界的情况
                judge = 0;
                break;
            }
            char matchedSrc = *((u8*)(V_MEM_BASE + end)); // 待匹配字符串，开头是V_MEM_BASE + end
            int highLight = j + p_con->start_cursor*2;
            char highLightSrc = *((u8*)(V_MEM_BASE + highLight)); // 高亮字符串

            if (matchedSrc == highLightSrc) { // 只有匹配到所有高亮字符结束才算匹配成功
                if (matchedSrc == ' ') { // 如果是空格，则进一步区分tab和空格
                    if ((*((u8*)(V_MEM_BASE + end + 1)) == 0x06 && *((u8*)(V_MEM_BASE + highLight + 1)) == 0x46)
                        || (*((u8*)(V_MEM_BASE + end + 1)) == 0x07 && *((u8*)(V_MEM_BASE + highLight + 1)) == 0x40)
                    ) { // 同时是空格或者同时是tab则匹配成功
                        end += 2;
                    }
                    else {
                        judge = 0;
                        break;
                    }
                }
                else { // 否则只要相等则匹配成功
                    end += 2;
                }
            }
            else {
                judge = 0;
                break;
            }
        }

        if (judge == 1) {
            for (int k = i; k < end; k += 2) {
                char ch = *(u8*)(V_MEM_BASE + k);
                if (ch == ' ') {
                    *(u8*)(V_MEM_BASE + k + 1) = RED_CHAR_WS; // 如果是空格或者tab，则设置背景色
                }
                else {
                    *(u8*)(V_MEM_BASE + k + 1) = RED_CHAR_COLOR;//只需要设置颜色，所以更改每个字母的第二个字节即可
                }

            }
        }

	}
}

/*======================================================================*
			   out_char
 *======================================================================*/
// 往控制台输出字符
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2); // 当前显示位置的地址

	switch(ch) {
	case '\n':
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
			push(p_con, p_con->cursor); // 将光标压栈
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
				((p_con->cursor - p_con->original_addr) /
				 SCREEN_WIDTH + 1);
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr) {
			int tmp = p_con->cursor; // 当前光标位置
            if (tmp == p_con->start_cursor && mode == 1) { // 在查找模式下防止删除键越界
                break;
            }
			p_con->cursor = pop(p_con); // 将光标出栈,即区分空格和tab
			for(int i=0;i<tmp-p_con->cursor;i++){
				*(p_vmem - 2 - 2 * i) = ' ';
				*(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
			}
		}
		break;
	case '\t':
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - TAB_WIDTH) { // 当有足够显存显示时
			push(p_con, p_con->cursor); // 将光标压栈
            if (mode == 0) { // 普通模式
                p_con->cursor += TAB_WIDTH;// 光标移动
                for (int i = 0; i < TAB_WIDTH; i++) {
                    *(p_vmem + 1 + 2 * i) = BLACK_CHAR_TAB;
                }
            }
            else { // 查找模式下tab显示红色背景色
                p_con->cursor += TAB_WIDTH;// 光标移动
                for (int i = 0; i < TAB_WIDTH; i++) {
                    *(p_vmem + 1 + 2 * i) = RED_CHAR_TAB;
                }
            }

		}
		break;
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			push(p_con, p_con->cursor);
			*p_vmem++ = ch; // 已经运算过字符内容，之后的p_vmem则是颜色内容
			if(mode==0) { // 普通模式下
                *p_vmem++ = DEFAULT_CHAR_COLOR;
            }
			else { // 进入查找模式，关键字显示红色
                if (*(p_vmem - 1) == ' ') {
                    *p_vmem++ = RED_CHAR_WS; // 设置空格红色背景色
                }
                else{
                    *p_vmem++ = RED_CHAR_COLOR; // 设置普通字符颜色黑底红字
                }
            }
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {// 如果光标位置超出了屏幕，则需要滚动屏幕
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}


/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	//disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	//enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	//disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	//enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
// 切换控制台函数
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor); // 设置当前光标位置
	set_video_start_addr(console_table[nr_console].current_start_addr); // 设置当前显示到了什么位置
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
// 屏幕滚动代码
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

/*======================================================================*
		push和pop
 *======================================================================*/
//TODO
// 将光标压栈
PUBLIC void push(CONSOLE* p_con, unsigned int pos) { // pos:当前光标位置
	p_con->pos_stk.pos[p_con->pos_stk.ptr++] = pos;
}

// 将光标出栈
PUBLIC unsigned int pop(CONSOLE* p_con) {
	if (p_con->pos_stk.ptr == 0) return 0;
	else {
		(p_con->pos_stk.ptr)--;
		return p_con->pos_stk.pos[p_con->pos_stk.ptr];
	}
}