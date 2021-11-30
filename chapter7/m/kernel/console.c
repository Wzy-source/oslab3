
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
PRIVATE void flush(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
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
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
    u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

    switch(ch) {
        case '\n':
            if (p_con->cursor < p_con->original_addr +
                                p_con->v_mem_limit - SCREEN_WIDTH) {
                //TODO 4 入栈方法的调用
                push_pos(p_con,p_con->cursor);
                p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
                                                       ((p_con->cursor - p_con->original_addr) /
                                                        SCREEN_WIDTH + 1);
            }
            break;
        case '\b':
            if (p_con->cursor > p_con->original_addr) {
                //TODO 4 出栈方法的调用
                unsigned int temp = p_con->cursor; // 原先位置
                p_con->cursor = pop_pos(p_con);//出栈
                int i;
                for(i=0;i<temp-p_con->cursor;++i){ // 用空格填充
                    *(p_vmem-2-2*i) = ' ';
                    *(p_vmem-1-2*i) = DEFAULT_CHAR_COLOR;
                }
//                *(p_vmem - 2) = ' ';
//                *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
            }
            break;
        case '\t':
            if(p_con->cursor < p_con->original_addr +
                               p_con->v_mem_limit - TAB_WIDTH){
                int i;
                for(i=0;i<TAB_WIDTH;++i){ // 用空格填充
                    *p_vmem++ = ' ';
                    *p_vmem++ = DEFAULT_CHAR_COLOR;
                }
                push_pos(p_con,p_con->cursor);
                p_con->cursor += TAB_WIDTH; // 调整光标
            }
            break;
        default:
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1) {
                *p_vmem++ = ch;
                //TODO 5 新增对于颜色的显示
                if (mode == 0) {
                    *p_vmem++ = DEFAULT_CHAR_COLOR;
                } else {//mode==1或mode==2
                    *p_vmem++ = RED;
                }
                //TODO 4 入栈方法的调用
                push_pos(p_con,p_con->cursor);
                p_con->cursor++;
            }
            break;
    }

    while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
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
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, position & 0xFF);
    enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
    disable_int();
    out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, addr & 0xFF);
    enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
        return;
    }

    nr_current_console = nr_console;

    set_cursor(console_table[nr_console].cursor);
    set_video_start_addr(console_table[nr_console].current_start_addr);
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

PUBLIC void push_pos(CONSOLE *console, unsigned int pos) {//注意点：pos始终指向下一个未填写内容的pos（填入第一个stack，pos=1，指向的是数组的第二个元素）
    int topStackItemIndex = console->pos_stack.stack_depth;
    console->pos_stack.pos[topStackItemIndex] = pos;
    console->pos_stack.stack_depth++;
}

PUBLIC unsigned int pop_pos(CONSOLE *console) {//注意点：这里并不是真正的出栈，而是将pos-1，在下一次入栈的时候，原来的值会被覆盖掉！！！1
    if (console->pos_stack.stack_depth == 0) {
        return 0;
    } else {
        console->pos_stack.stack_depth--;
        int topStackItemIndex = console->pos_stack.stack_depth;
        return console->pos_stack.pos[topStackItemIndex];
    }

}
//TODO 6 再按ESC时候清屏操作的实现
PUBLIC void exit_search_mode(CONSOLE *console) {
    u8 *p_vmem = (u8 *) (V_MEM_BASE + console->cursor * 2);//显存指针，指向的是当前光标所指向的字符位置
    //找到开始搜索时候的光标位置，这个也是需要复原到的位置
    int i;
    int start_depth = console->pos_stack.search_start_stack_depth;
    int start_pos = console->pos_stack.pos[start_depth-1];
    for(i=0;i<console->cursor-1-start_pos;++i){
        *(p_vmem - 2 - 2 * i) = ' ';
        *(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
    }
    //将栈的高度调整为原来的位置
    console->pos_stack.stack_depth = console->pos_stack.search_start_stack_depth;
    console->cursor = start_pos+1;
    //TODO 9 将所有文字颜色变为白色
    for(int i=0;i<SCREEN_WIDTH*2;i+=2){
        *(u8*)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
    }

    flush(console);

}

//TODO 8 用于字符串匹配操作的具体实现：找到字符串，将字符串颜色标红
PUBLIC void search_target_str(CONSOLE *console){
    /*
     * 使用滑动窗口：开辟一个长度为target_str长度的窗口（内层循环）
     * 然后使用for循环，从0到search_start_cursor_pos开始遍历每一个输入的字符（外层循环）
     * 如果全都match，则found
     * 否则直接break掉
     * 如果match，则将其颜色修改为RED
     * */
    int stack_depth = console->pos_stack.stack_depth;
    int search_start_stack_depth = console->pos_stack.search_start_stack_depth;
    int search_start_pos = (int) console->pos_stack.pos[search_start_stack_depth - 1];
    int end_pos = (int) console->pos_stack.pos[stack_depth - 1];
    int target_str_len = (int) (end_pos - search_start_pos);
    for (int i = 0; i < (search_start_pos-target_str_len+2) * 2; i += 2) {
        int found = 1;
        int in_str_ptr = i;//从i位置开始遍历，每次加2
        for (int j = 0 ; j < target_str_len * 2; j += 2) {
            u8 *p_vmem_current_char = (u8 *) (V_MEM_BASE + in_str_ptr);//当前原字符串的字符指针
            u8 *p_vmem_target_char = (u8 *) (V_MEM_BASE + (search_start_pos+1)*2 +  j);//当前目标字符串的字符指针
            if (*p_vmem_current_char != *p_vmem_target_char) {
                found = 0;
                break;
            } else {
                in_str_ptr += 2;
            }
        }
        if (found == 1) {
            for (int j = i; j < i + target_str_len * 2; j += 2) {
                *(u8 *) (V_MEM_BASE + j + 1) = RED;
            }
        }
    }

}