/*
*作者:王重韬
*时间:20110412
*功能:用户客户端，使用了curses库
*/
#include "wdb.h"
#include "apue.h"
#include <time.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>


#define MAX_STRING (80)		/* Longest allowed response       */

#define MESSAGE_LINE 6		/* Misc. messages go here         */
#define ERROR_LINE   24		/* The line to use for errors     */
#define Q_LINE       22		/* Line for Questions             */
#define PROMPT_LINE  21		/* Line for prompting on          */

#define BUFLEN 65536

const char *objtmp = "object.tmp";
const char *OBJECT = "meta_object_%.4d%.2d%.2d_%.2d%.2d%.2d";
int clifd;

void clear_all_screen(void);
void get_return(void);
int get_confirm(void);
int getchoice(char *greet, char *choices[]);
void draw_menu(char *options[], int highlight, int start_row, int start_col);
void get_string(char *string);
void full_backup(void);
void inc_backup(void);
void restore(void);

char *menu[] = {
	"1 backup full",
	"2 backup inc",
	"3 restore",
	"4 quit",
	0
};

/*读缓存文件的内容到内存中*/
int read_tmpfile(const char *tmpfile,char **bufp){
	FILE *fp;
	int i = 0;
	long file_length = 0;
	char line[MAXLINE];
	fp = fopen(tmpfile,"r");
	if(fp == NULL){
		mvprintw(ERROR_LINE, 0, "read_tmpfile error");
		refresh();
		return -1;
	}
	fseek(fp,0L,SEEK_END);
	file_length = ftell(fp);/*获取文件的长度*/
	fseek(fp,0L,SEEK_SET);
	(*bufp) = (char *)malloc((file_length+1) * sizeof(char));
	(*bufp)[0] = '\0';
	do{
		if(fgets(line,MAXLINE,fp) == NULL)
			break;
		i++;
		strcat(*bufp,line);				
	}while(1);
	(*bufp)[file_length] = '\0';
	fclose(fp);
	return i;
}


/*主函数，提供菜单，根据用户的选择执行相关的命令*/
int main(void)
{
    int choice;

    initscr();

	if((clifd = cli_conn(WDB_SERVER)) < 0){/*与服务器建立链接*/
		   mvprintw(MESSAGE_LINE, 0, "Connect error");
			refresh();
		   sleep(1);
			endwin();
		   return -1;
	}

    do {

		choice = getchoice("Options:", menu);/*获取用户的选择*/

		switch (choice) {
			case '1':
				full_backup();/*全量备份*/
	    		break;

			case '2':
	    		inc_backup();/*增量备份*/
	    		break;

			case '3':
	    		restore(); /*恢复*/
	    		break;

			case '4':
	    		choice = 'q';
	    		break;
		}
    } while (choice != 'q');/*如果选择quit则退出*/

	 close(clifd);
    endwin();
    exit(EXIT_SUCCESS);

}				/* main */

/*
 *获得用户的选择
 */
int getchoice(char *greet, char *choices[])
{
    static int selected_row = 0;
    int max_row = 0;
    int start_screenrow = MESSAGE_LINE, start_screencol = 10;
    char **option;
    int selected;
    int key = 0;

    option = choices;
    while (*option) {
		max_row++;
		option++;
   	 }

    /* protect against menu getting shorted when CD deleted */
    if (selected_row >= max_row)
		 selected_row = 0;

    clear_all_screen();
    mvprintw(start_screenrow - 2, start_screencol, greet);

    keypad(stdscr, TRUE);
    cbreak();
    noecho();

    key = 0;
	/*通过上下键控制光标来进行选择，邋selected_row为选择的行号*/
    while (key != 'q' && key != KEY_ENTER && key != '\n') {
		if (key == KEY_UP) {
	    	if (selected_row == 0)
				selected_row = max_row - 1;
	    	else
				selected_row--;
		}
		if (key == KEY_DOWN) {
	    	if (selected_row == (max_row - 1))
				selected_row = 0;
	    	else
				selected_row++;
		}
		selected = *choices[selected_row];
		draw_menu(choices, selected_row, start_screenrow, start_screencol);/*画出菜单选项*/
		key = getch();
    	}

    keypad(stdscr, FALSE);
    nocbreak();
    echo();

    if (key == 'q')
		selected = 'q';

    return (selected);
}

/*画出菜单选项，并对邋selected_row加黑底*/
void draw_menu(char *options[], int current_highlight, int start_row, int start_col)
{
    int current_row = 0;
    char **option_ptr;
    char *txt_ptr;

    option_ptr = options;
    while (*option_ptr) {
		if (current_row == current_highlight) attron(A_STANDOUT);/*selected_row加黑底*/
		txt_ptr = options[current_row];
		mvprintw(start_row + current_row, start_col, "%s", txt_ptr);/*将菜单写到窗口区域*/
		if (current_row == current_highlight) attroff(A_STANDOUT);
		current_row++;
		option_ptr++;
    	}

    mvprintw(start_row + current_row + 3, start_col, "Move highlight then press Return ");

    refresh();
}

/*
 *清屏
 */
void clear_all_screen()
{
    clear();
    mvprintw(2, Q_LINE, "%s", "De-duplication System With Data Reliability");
    refresh();
}


/*
 *获得一个回车符
 */
void get_return()
{
    int ch;
    mvprintw(23, 0, "%s", " Press return ");
    refresh();
    while ((ch = getchar()) != '\n' && ch != EOF);
}

/*
 *获得确认
 */
int get_confirm()
{
    int confirmed = 0;
    char first_char = 'N';

    mvprintw(Q_LINE, 5, "Are you sure? ");
    clrtoeol();
    refresh();

    cbreak();
    first_char = getch();
    if (first_char == 'Y' || first_char == 'y') {/*如果输入y,或Y则表示肯定*/
		confirmed = 1;
   	 }
    nocbreak();

    if (!confirmed) {    /*否则就是否定，取消操作*/
		mvprintw(Q_LINE, 1, "    Cancelled");
		clrtoeol();
		refresh();
		sleep(1);
   	 }
    return confirmed;
}

/*
 *获得用户输入的字符串
 */
void get_string(char *string)
{
    int len;

    wgetnstr(stdscr, string, MAX_STRING);
    len = strlen(string);
    if (len > 0 && string[len - 1] == '\n')
		 string[len - 1] = '\0';
}

/*全量备份*/
void full_backup(void){
	char backup_path[MAX_NAME_LEN];
	char cmd[MAX_NAME_LEN * 2];
	time_t timer;
	struct tm *now;
	int send_len;

	int screenrow = MESSAGE_LINE;
    int screencol = 10;

    clear_all_screen();
    mvprintw(screenrow, screencol, "Please input backup path:");
    screenrow += 2;

	get_string(backup_path);/*获取备份路径*/
    screenrow++;

	move(PROMPT_LINE, 0);
    if (get_confirm()) {
		timer = time(NULL);
		now = localtime(&timer);
		strcpy(cmd,"F ");
		/*生成备份对象名*/
		sprintf(cmd+2,OBJECT,now->tm_year+1900,now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec);
		strcat(cmd," ");
		strcat(cmd,backup_path);
		send_len = strlen(cmd)+1;
		write(clifd,cmd,send_len);/*发送全量备份命令*/
    }
}

#define BOXED_LINES    11
#define BOXED_COLS     60
#define BOX_LINE_POS   8
#define BOX_COL_POS    2


/*增量备份*/
void inc_backup(void){
	char *all_obj,*ptr,*start;
	char **obj_menu,**option_ptr;
	char cmd[MAX_NAME_LEN * 2];
	int i,obj_num,key,p = 0;
	int lines_op = 0,send_len,selected_row,first_line;
	int cur_pos,max_lines;
    WINDOW *obj_pad_ptr;

	
	obj_num = read_tmpfile(objtmp,&all_obj);/*读取缓存文件*/
	obj_menu = (char **)malloc((obj_num + 1) * sizeof(char *));

	/*将all_obj长字符串以\n为分隔符，转换成字符串数组obj_menu的形式*/
	for(start = ptr = all_obj;*ptr;ptr++){
		if(*ptr == '\n'){
			obj_menu[p] = (char *)malloc((ptr - start + 1) * sizeof(char));
			*ptr = '\0';
			strcpy(obj_menu[p++],start);
			start = ptr + 1;
		}
	}
	obj_menu[p] = NULL;
	free(all_obj);
	
	clear_all_screen();
   obj_pad_ptr = newpad(obj_num + 1 + BOXED_LINES, BOXED_COLS + 1);/*创建一个记事本，该记事本的容量可以大于显示窗口的容量*/
	if(!obj_pad_ptr)
		return;
	mvprintw(4, 0, "Object Listing\n");

	mvprintw(MESSAGE_LINE, 0, "Please select a object, RETURN or q to exit");

 	for(option_ptr = obj_menu;*option_ptr;option_ptr++)	/*将obj_menu的内容写到记事本中*/
	    mvwprintw(obj_pad_ptr, lines_op++, 0, "%s", *option_ptr);
	wrefresh(stdscr);
	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	
	key = 0;
	selected_row = 0;
	first_line = 0;
	cur_pos = BOX_LINE_POS;
	move(cur_pos,BOX_COL_POS);
	max_lines = obj_num-1 > BOXED_LINES?BOXED_LINES:obj_num-1;/*当前窗口显示的最多的行数*/

	/*通过移动上下键，来控制窗口中的内容，并移动光标以显示用户的选择
	*记事本中的内容如果大于窗口的显示的内容，则可将内容向上下滚动以显示当前未在窗口显示的记事本中的内容
	*通过移动上下键也可移动光标，当前光标出即为用户的选择
	*selected_row为用户选择的内容在记事本中的行号
	*cur_pos为光标在当前显示窗口中的位置
	*first_line为当前显示窗口中第一行的内容在记事本中的行号
	*/
	while (key != 'q' && key != KEY_ENTER && key != '\n') {	
		if (key == KEY_UP) {
			if(selected_row == 0){
				selected_row = 0;
			}
			else
				selected_row--;
			if (first_line > 0 && cur_pos == BOX_LINE_POS){
				first_line--;
			}else{
				cur_pos--;
			}
		}
		if (key == KEY_DOWN) {
			if(selected_row == obj_num - 1)
				selected_row = obj_num - 1;
			else
				selected_row++;
			if (first_line + BOXED_LINES + 1 < obj_num && cur_pos == BOX_LINE_POS + max_lines){
				first_line++;
			}else{
				cur_pos++;
			}
		}

		/*光标只能在当前显示内容的最小行和最大行之间移动*/
		if(cur_pos < BOX_LINE_POS)
			cur_pos = BOX_LINE_POS;
		if(cur_pos > BOX_LINE_POS + max_lines)
			cur_pos = BOX_LINE_POS + max_lines;
		/*选择的行号不能超过记事本中内容的最大行号，而且不得小于0*/
		if(selected_row < 0)
			selected_row = 0;
		if(selected_row > obj_num - 1)
			selected_row = obj_num -1;		
		/* now draw the appropriate part of the pad on the screen */
		/*将记事本中指定区域的内容显示到窗口中*/
		prefresh(obj_pad_ptr, first_line, 0,
			 BOX_LINE_POS, BOX_COL_POS,
			 BOX_LINE_POS + BOXED_LINES, BOX_COL_POS + BOXED_COLS);
			/*wrefresh(stdscr);*/
		move(cur_pos,BOX_COL_POS);/*移动光标*/
		key = getch();
		
	}
	/*处理用户的选择*/
	if(key == '\n' || key == KEY_ENTER){
		for(ptr = obj_menu[selected_row];*ptr;ptr++){
			if(*ptr == ' '){
				*ptr = '\0';
				break;
			}
		}
		mvprintw(PROMPT_LINE, 4, "You select:%s\n",obj_menu[selected_row]);
		move(Q_LINE,0);
		if(get_confirm()){/*确认*/
			strcpy(cmd,"I ");
			strcat(cmd,obj_menu[selected_row]);
			send_len = strlen(cmd) + 1;
			write(clifd,cmd,send_len);/*发送增量备份命令*/
		}
	}
		
	 delwin(obj_pad_ptr);
    keypad(stdscr, FALSE);
    nocbreak();
    echo();
	
	for(i = 0; i < obj_num + 1;i++)
		free(obj_menu[i]);
	free(obj_menu);
}

/*恢复*/
void restore(void){
	char *all_obj,*all_ver,*ptr,*start;
	char **obj_menu,**ver_menu,**option_ptr;
	char cmd[MAX_NAME_LEN * 2];
	char path[MAX_NAME_LEN],buf[BUFLEN];
	int i,obj_num,ver_num,key,p = 0;
	int lines_op = 0,send_len,rec_len,selected_row,first_line;
	int cur_pos,max_lines,start_col = 10;
	int flag,cmdlen;
    WINDOW *obj_pad_ptr,*ver_pad_ptr;

	/*处理对象信息*/
	obj_num = read_tmpfile(objtmp,&all_obj);
	if(obj_num == -1)
	           return;
	obj_menu = (char **)malloc((obj_num + 1) * sizeof(char *));

	/*将长字符串以\n为分隔符转化成字符串数组*/
	for(start = ptr = all_obj;*ptr;ptr++){
		if(*ptr == '\n'){
			obj_menu[p] = (char *)malloc((ptr - start + 1) * sizeof(char));
			*ptr = '\0';
			strcpy(obj_menu[p++],start);
			start = ptr + 1;
		}
	}
	obj_menu[p] = NULL;
	free(all_obj);
	
	clear_all_screen();
	/*将对象信息写到记事本中*/
   obj_pad_ptr = newpad(obj_num + 1 + BOXED_LINES, BOXED_COLS + 1);
	if(!obj_pad_ptr)
		return;
	mvprintw(4, start_col, "Object Listing\n");

	mvprintw(MESSAGE_LINE, start_col, "Please select a object, RETURN or q to exit");

 	for(option_ptr = obj_menu;*option_ptr;option_ptr++){	
	/*	if(option_ptr - obj_menu == selected_row) 	attron(A_STANDOUT);*/
	    mvwprintw(obj_pad_ptr, lines_op++, start_col, "%s", *option_ptr);
	/*	if(option_ptr - obj_menu == selected_row)	attroff(A_STANDOUT);*/
	}

	wrefresh(stdscr);
	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	
	key = 0;
	selected_row = 0;
	first_line = 0;
	cur_pos = BOX_LINE_POS;
	move(cur_pos,start_col+BOX_COL_POS);
	max_lines = obj_num-1 > BOXED_LINES?BOXED_LINES:obj_num-1;

	/*显示对象信息，用户移动光标选择，详见inc_backup*/
	while (key != 'q' && key != KEY_ENTER && key != '\n') {	
		if (key == KEY_UP) {
			if(selected_row == 0){
				selected_row = 0;
			}
			else
				selected_row--;
			if (first_line > 0 && cur_pos == BOX_LINE_POS){
				first_line--;
			}else{
				cur_pos--;
			}
		}
		if (key == KEY_DOWN) {
			if(selected_row == obj_num - 1)
				selected_row = obj_num - 1;
			else
				selected_row++;
			if (first_line + BOXED_LINES + 1 < obj_num && cur_pos == BOX_LINE_POS + max_lines){
				first_line++;
			}else{
				cur_pos++;
			}
		}

		if(cur_pos < BOX_LINE_POS)
			cur_pos = BOX_LINE_POS;
		if(cur_pos > BOX_LINE_POS + max_lines)
			cur_pos = BOX_LINE_POS + max_lines;
		if(selected_row < 0)
			selected_row = 0;
		if(selected_row > obj_num - 1)
			selected_row = obj_num -1;		
		/* now draw the appropriate part of the pad on the screen */
		prefresh(obj_pad_ptr, first_line, 0,
			 BOX_LINE_POS, BOX_COL_POS,
			 BOX_LINE_POS + BOXED_LINES, BOX_COL_POS + BOXED_COLS);
			/*wrefresh(stdscr);*/
		move(cur_pos,start_col+BOX_COL_POS);
		key = getch();
		
	}
		
	delwin(obj_pad_ptr);
    keypad(stdscr, FALSE);
    nocbreak();
    echo();	

	/*处理用户的选择*/
	if(key == '\n' || key == KEY_ENTER){
		for(ptr = obj_menu[selected_row];*ptr;ptr++){
			if(*ptr == ' '){
				*ptr = '\0';
				ptr++;
				strcpy(path,ptr);
				break;
			}
		}
		mvprintw(PROMPT_LINE, 4, "You select:%s\n",obj_menu[selected_row]);
		move(Q_LINE,0);
		if(get_confirm()){
			strcpy(cmd,"R ");
			strcat(cmd,obj_menu[selected_row]);
			send_len = strlen(cmd) + 1;
			write(clifd,cmd,send_len);/*发送恢复命令*/
		}else{
			for(i = 0; i < obj_num + 1;i++)
				free(obj_menu[i]);
			free(obj_menu);	
			return;
		}
	}else{
			for(i = 0; i < obj_num + 1;i++)
				free(obj_menu[i]);
			free(obj_menu);	
			return;
	}
	
	for(i = 0; i < obj_num + 1;i++)
		free(obj_menu[i]);
	free(obj_menu);	

	/*处理版本信息*/
	flag = 1;
	all_ver = (char *)malloc(BUFLEN * sizeof(char));
	/*从服务器返回版本列表，并保存到字符串数组中*/
	while(flag){
		int len;
		rec_len = read(clifd,all_ver,BUFLEN);
		ver_num = 0;
		ptr = all_ver;
		for(;;){
			if(strncmp(ptr,"END",3) == 0){
				flag = 0;
				break;
			}else if(rec_len <= 0){
				break;
			}else{
				ver_num++;
				len = strlen(ptr);
				ptr = ptr + len + 1;
				rec_len = rec_len -len -1;
			}
		}
	}
	ver_menu = (char **)malloc((ver_num + 1) * sizeof(char *));
	for(ptr = all_ver,i = 0; i < ver_num;i++){
		int len;
		len = strlen(ptr);
		ver_menu[i] = (char *)malloc((len+1) * sizeof(char));
		strcpy(ver_menu[i],ptr);
		ptr = ptr + len + 1;
	}
	ver_menu[i] = NULL;
	free(all_ver);

	 lines_op = 0;
	 clear_all_screen();
	 /*将版本信息写到记事本中*/
	 ver_pad_ptr = newpad(ver_num + 1 + BOXED_LINES, BOXED_COLS + 1);
	 if(!ver_pad_ptr)
		 return;
	 mvprintw(4, start_col, "Version Listing\n");
	
	 mvprintw(MESSAGE_LINE, start_col, "Please select a version, RETURN or q to exit");
	
	 for(option_ptr = ver_menu;*option_ptr;option_ptr++)	 
		 mvwprintw(ver_pad_ptr, lines_op++, start_col, "%s", *option_ptr);
	
	 wrefresh(stdscr);
	 keypad(stdscr, TRUE);
	 cbreak();
	 noecho();
	 
	key = 0;
	selected_row = 0;
	first_line = 0;
	cur_pos = BOX_LINE_POS;
	move(cur_pos,start_col+BOX_COL_POS);
	max_lines = ver_num-1 > BOXED_LINES?BOXED_LINES:ver_num-1;

	/*显示版本信息，移动光标以标识用户的选择，详见inc_backup*/
	while (key != 'q' && key != KEY_ENTER && key != '\n') {	
		if (key == KEY_UP) {
			if(selected_row == 0){
				selected_row = 0;
			}
			else
				selected_row--;
			if (first_line > 0 && cur_pos == BOX_LINE_POS){
				first_line--;
			}else{
				cur_pos--;
			}
		}
		if (key == KEY_DOWN) {
			if(selected_row == ver_num - 1)
				selected_row = ver_num - 1;
			else
				selected_row++;
			if (first_line + BOXED_LINES + 1 < ver_num && cur_pos == BOX_LINE_POS + max_lines){
				first_line++;
			}else{
				cur_pos++;
			}
		}

		if(cur_pos < BOX_LINE_POS)
			cur_pos = BOX_LINE_POS;
		if(cur_pos > BOX_LINE_POS + max_lines)
			cur_pos = BOX_LINE_POS + max_lines;
		if(selected_row < 0)
			selected_row = 0;
		if(selected_row > ver_num - 1)
			selected_row = ver_num -1;		
		/* now draw the appropriate part of the pad on the screen */
		prefresh(ver_pad_ptr, first_line, 0,
			 BOX_LINE_POS, BOX_COL_POS,
			 BOX_LINE_POS + BOXED_LINES, BOX_COL_POS + BOXED_COLS);
			/*wrefresh(stdscr);*/
		move(cur_pos,start_col+BOX_COL_POS);
		key = getch();
		
	}	
	delwin(obj_pad_ptr);
    keypad(stdscr, FALSE);
    nocbreak();
    echo();	

	/*处理用户的选择*/
	if(key == '\n' || key == KEY_ENTER){
		mvprintw(PROMPT_LINE, 4, "You select:%s\n",ver_menu[selected_row]);
		move(Q_LINE,0);
		if(get_confirm()){
			strcpy(cmd,ver_menu[selected_row]);
		}else{
			write(clifd,"EOD",4);/*取消恢复*/
			for(i = 0;i < ver_num;i++)
				free(ver_menu[i]);
			free(ver_menu);
			return;
		}
	}else{
			write(clifd,"EOD",4);/*取消恢复*/
			for(i = 0;i < ver_num;i++)
				free(ver_menu[i]);
			free(ver_menu);
			return;
	}
	for(i = 0;i < ver_num;i++)
		free(ver_menu[i]);
	free(ver_menu);

	cmdlen = strlen(cmd);
	do{
		flag = 0;
		/*处理路径*/
		clear_all_screen();
		mvprintw(MESSAGE_LINE, 10, "Input a subpath of %s or return:",path);
		get_string(buf);/*获取用户输入的路径，默认为备份路径*/
		if(strcmp(buf,"") == 0)
			strcpy(buf,path);
		mvprintw(PROMPT_LINE, 4, "Your path:%s",buf);
		if(get_confirm()){
			strcpy(cmd + cmdlen," ");
			strcat(cmd,buf);
			send_len = strlen(cmd) + 1;
			write(clifd,cmd,send_len);/*发送版本信息和路径信息*/
			rec_len = read(clifd,buf,BUFLEN);
			if(strcmp(buf,"cmd error") == 0){
				mvprintw(PROMPT_LINE, 4, "path error:\n");
				refresh();
				sleep(1);
				flag = 1;
			}
		}else{
			write(clifd,"EOD",4);/*取消恢复任务*/
		}
	}while(flag);
}



