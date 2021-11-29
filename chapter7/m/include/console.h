
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80
//TODO 4 用于记录上一次的输入的字符光标结束的位置
//TODO 6 新增用于记录输入开始时候的栈的深度，要获取到输入开始时候的光标位置，
typedef struct cursor_pos_stack
{
    int stack_depth;//栈的深度
    int search_start_stack_depth;//用于记录查找模式开始时的栈的深度，从而获取到光标位置
    unsigned int pos[SCREEN_SIZE];//输入的字符串的光标起始位置,每新增一个字符，会在栈中新增一个用来记录新增的位置
}CON_POS_TACK;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;			/* 当前光标位置 */
    unsigned int 	search_start_pos;
    CON_POS_TACK pos_stack;	/*新增*/
}CONSOLE;

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

//TODO 2 新增Tab宽度
#define TAB_WIDTH  4




#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */


#endif /* _ORANGES_CONSOLE_H_ */

//TODO 3 在头文件中导入
PUBLIC void init_screen(TTY *p_tty);
PUBLIC void select_console(int nr_console);


//TODO 4 用于栈的入栈和出栈
PUBLIC void push_pos(CONSOLE* console,unsigned int pos);
PUBLIC unsigned int pop_pos(CONSOLE* console);



//TODO 6 用于再按ESC时候清屏操作
PUBLIC void exit_search_mode(CONSOLE* console);


//TODO 8 用于字符串匹配操作
PUBLIC void search_target_str(CONSOLE *console);