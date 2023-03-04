
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// ??????

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

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

/*======================================================================*
                           task_tty
 *======================================================================*/
// 通过循环处理TTY中的度和写操作
// TTY调用keyboard_read, keyboard_read调用in_process
PUBLIC void task_tty()
{
	TTY*	p_tty;

	init_keyboard();

    // ???
	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0); // 选择控制台
	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty); // 调用keyboard_read
			tty_do_write(p_tty);
		}
	}
}


/*======================================================================*
				      init_all_screen
 *======================================================================*/
// 将每一个终端屏幕都初始化
PUBLIC void init_all_screen() {
	TTY* p_tty; //指针，指向tty数组中的第一个tty
	for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
	{
		init_screen(p_tty); // console.c
	}
	select_console(0); // 将当前tty设置为第一个，console.c
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key) // key: 输入的按键
{
        char output[2] = {'\0', '\0'};
        // TODO
		if (mode == 2)
		{ // 除了ESC之外，需要屏蔽输入
			if ((key & MASK_RAW) == ESC)
			{
				mode = 0;
				exit_esc(p_tty->p_console);
			}
			return;
		}

        if (!(key & FLAG_EXT)) { // ?????????
		    put_key(p_tty, key);
        }
        else { // ????????????????????
			int raw_code = key & MASK_RAW;
            switch(raw_code) {
				case ESC:
                    if (mode == 0) { // 进入查找模式
                        mode = 1;
                        p_tty->p_console->start_cursor = p_tty->p_console->cursor; // 将原来光标的位置记录为进入时光标的位置
                        p_tty->p_console->pos_stk.start = p_tty->p_console->pos_stk.ptr; // 将ESC开始时的地址设置为栈顶指针
                    }
                    else {
                        mode = 0; // 从查找模式退出
//                        // 退出时要将p_tty->p_console->start_cursor进行初始化，否则影响删除键
//                        p_tty->p_console->start_cursor = 0;
                    }
					break;
                case ENTER: // 在查找模式下按下回车，将匹配的文本以红色显示
					if (mode == 0) { // 在普通模式下是回车
						put_key(p_tty, '\n');
					}
					else{ // 在查找模式下是高亮显示
						search(p_tty->p_console);
						mode = 2;
					}
					break;
                case BACKSPACE:
					put_key(p_tty, '\b');
					break;
				case TAB:
					put_key(p_tty, '\t');
					break;
                case UP:
                    if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                        scroll_screen(p_tty->p_console, SCR_DN);
                    }
					break;
				case DOWN:
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_UP);
					}
					break;
				case F1:
				case F2:
				case F3:
				case F4:
				case F5:
				case F6:
				case F7:
				case F8:
				case F9:
				case F10:
				case F11:
				case F12:
					/* Alt + F1~F12 */
					if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
						select_console(raw_code - F1);
					}
					break;
                default:
                        break;
                }
        }
}

/*======================================================================*
			      put_key
*======================================================================*/
// 将得到的字符放入TTY的缓冲区中
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) { // 如果缓冲区满了，则将指针移动到开头
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) { // 判断TTY的控制台是否是当前控制台
		keyboard_read(p_tty); //TTY调用keyboard_read, keyboard_read调用in_process
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) { // 当TTY的缓冲区里有内容
		char ch = *(p_tty->p_inbuf_tail); // 得到待输出的键
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch); // 往控制台输出字符
	}
}


