
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
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
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST    (tty_table)
#define TTY_END        (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY *p_tty);

PRIVATE void tty_do_read(TTY *p_tty);

PRIVATE void tty_do_write(TTY *p_tty);

PRIVATE void put_key(TTY *p_tty, u32 key);

/*======================================================================*
                           task_tty
 *======================================================================*/

/*
 * 核心！！！在TTY任务中执行一个循环，这个循环将轮询每一个TTY
 * 处理它的事件，包括从键盘缓冲区读取数据、显示字符等内容。
 * */
PUBLIC _Noreturn void task_tty() {
    TTY *p_tty;

    init_keyboard();

    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
        init_tty(p_tty);
    }
    select_console(0);
    while (1) {
        for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
            tty_do_read(p_tty);//联系起来了！这是从keyboard中读取一个自己实际期望的字符
            tty_do_write(p_tty);//这里是将字符进行判断！如果是特殊字符：\t \n \b，进行操作，如果是普通字符，就进行输出！
        }
    }
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY *p_tty) {
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

    init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY *p_tty, u32 key)//key的输出程序
{
    //TODO 7 如果是展示模式，则不进行in_process
    if (show_search_res==1&&key!=ESC) return;

    if (!(key & FLAG_EXT)) {//判断当前的key是不是0~9，A~Z，a~z的范围之内
        put_key(p_tty, key);
    } else {
        int raw_code = key & MASK_RAW;
        switch (raw_code) {
             //TODO 6 在enter中新增对mode的判断
            case ENTER:
                if(mode==0) {
                    put_key(p_tty, '\n');
                } else if (mode == 1) {//如果是1，则不用put-key了，转入查找模式，此时会1.查询匹配的所有字符2.屏蔽所有的输入
                    //TODO 7 将show_search_res状态改变
                    //说明进入了搜索展示模式，1.将匹配的字符串标红2.屏蔽所有输入
                    //TODO 8 调用search_target_str方法
                    search_target_str(p_tty->p_console);
                    show_search_res = 1;
                }
                break;
            case BACKSPACE:
                put_key(p_tty, '\b');
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
                //TODO 2 新增Tab判断
            case TAB:
                put_key(p_tty, '\t');
                break;
                //TODO 5 新增按下ESC的判断，ESC键对于输出没有任何意义，不用放入buffer中，改变模式就好
            case ESC:
                if (mode == 0) {
                    mode = 1;
                    p_tty->p_console->pos_stack.search_start_stack_depth = p_tty->p_console->pos_stack.stack_depth;
                    p_tty->p_console->search_start_pos = p_tty->p_console->cursor;
                } else if (mode == 1) {
                    mode = 0;
                    // TODO 6 实现再次按ESC时候的光标复位
                    // TODO 7 将 show_search_res 复原
                    show_search_res = 0;
                    exit_search_mode(p_tty->p_console);
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
PRIVATE void put_key(TTY *p_tty, u32 key) {//将key的内容放在buffer中
    if (p_tty->inbuf_count < TTY_IN_BYTES) {
        *(p_tty->p_inbuf_head) = key;
        p_tty->p_inbuf_head++;
        if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_head = p_tty->in_buf;
        }
        p_tty->inbuf_count++;
    }
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY *p_tty) {
    if (is_current_console(p_tty->p_console)) {
        keyboard_read(p_tty);
    }
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY *p_tty) {
    if (p_tty->inbuf_count) {
        char ch = *(p_tty->p_inbuf_tail);
        p_tty->p_inbuf_tail++;
        if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;

        out_char(p_tty->p_console, ch);
    }
}


