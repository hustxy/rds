/*
*����:�����
*ʱ��:20110412
*����:�û��ͻ��ˣ�ʹ����curses��
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

/*�������ļ������ݵ��ڴ���*/
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
	file_length = ftell(fp);/*��ȡ�ļ��ĳ���*/
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


/*���������ṩ�˵��������û���ѡ��ִ����ص�����*/
int main(void)
{
    int choice;

    initscr();

	if((clifd = cli_conn(WDB_SERVER)) < 0){/*���������������*/
		   mvprintw(MESSAGE_LINE, 0, "Connect error");
			refresh();
		   sleep(1);
			endwin();
		   return -1;
	}

    do {

		choice = getchoice("Options:", menu);/*��ȡ�û���ѡ��*/

		switch (choice) {
			case '1':
				full_backup();/*ȫ������*/
	    		break;

			case '2':
	    		inc_backup();/*��������*/
	    		break;

			case '3':
	    		restore(); /*�ָ�*/
	    		break;

			case '4':
	    		choice = 'q';
	    		break;
		}
    } while (choice != 'q');/*���ѡ��quit���˳�*/

	 close(clifd);
    endwin();
    exit(EXIT_SUCCESS);

}				/* main */

/*
 *����û���ѡ��
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
	/*ͨ�����¼����ƹ��������ѡ����selected_rowΪѡ����к�*/
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
		draw_menu(choices, selected_row, start_screenrow, start_screencol);/*�����˵�ѡ��*/
		key = getch();
    	}

    keypad(stdscr, FALSE);
    nocbreak();
    echo();

    if (key == 'q')
		selected = 'q';

    return (selected);
}

/*�����˵�ѡ�������selected_row�Ӻڵ�*/
void draw_menu(char *options[], int current_highlight, int start_row, int start_col)
{
    int current_row = 0;
    char **option_ptr;
    char *txt_ptr;

    option_ptr = options;
    while (*option_ptr) {
		if (current_row == current_highlight) attron(A_STANDOUT);/*selected_row�Ӻڵ�*/
		txt_ptr = options[current_row];
		mvprintw(start_row + current_row, start_col, "%s", txt_ptr);/*���˵�д����������*/
		if (current_row == current_highlight) attroff(A_STANDOUT);
		current_row++;
		option_ptr++;
    	}

    mvprintw(start_row + current_row + 3, start_col, "Move highlight then press Return ");

    refresh();
}

/*
 *����
 */
void clear_all_screen()
{
    clear();
    mvprintw(2, Q_LINE, "%s", "De-duplication System With Data Reliability");
    refresh();
}


/*
 *���һ���س���
 */
void get_return()
{
    int ch;
    mvprintw(23, 0, "%s", " Press return ");
    refresh();
    while ((ch = getchar()) != '\n' && ch != EOF);
}

/*
 *���ȷ��
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
    if (first_char == 'Y' || first_char == 'y') {/*�������y,��Y���ʾ�϶�*/
		confirmed = 1;
   	 }
    nocbreak();

    if (!confirmed) {    /*������Ƿ񶨣�ȡ������*/
		mvprintw(Q_LINE, 1, "    Cancelled");
		clrtoeol();
		refresh();
		sleep(1);
   	 }
    return confirmed;
}

/*
 *����û�������ַ���
 */
void get_string(char *string)
{
    int len;

    wgetnstr(stdscr, string, MAX_STRING);
    len = strlen(string);
    if (len > 0 && string[len - 1] == '\n')
		 string[len - 1] = '\0';
}

/*ȫ������*/
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

	get_string(backup_path);/*��ȡ����·��*/
    screenrow++;

	move(PROMPT_LINE, 0);
    if (get_confirm()) {
		timer = time(NULL);
		now = localtime(&timer);
		strcpy(cmd,"F ");
		/*���ɱ��ݶ�����*/
		sprintf(cmd+2,OBJECT,now->tm_year+1900,now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec);
		strcat(cmd," ");
		strcat(cmd,backup_path);
		send_len = strlen(cmd)+1;
		write(clifd,cmd,send_len);/*����ȫ����������*/
    }
}

#define BOXED_LINES    11
#define BOXED_COLS     60
#define BOX_LINE_POS   8
#define BOX_COL_POS    2


/*��������*/
void inc_backup(void){
	char *all_obj,*ptr,*start;
	char **obj_menu,**option_ptr;
	char cmd[MAX_NAME_LEN * 2];
	int i,obj_num,key,p = 0;
	int lines_op = 0,send_len,selected_row,first_line;
	int cur_pos,max_lines;
    WINDOW *obj_pad_ptr;

	
	obj_num = read_tmpfile(objtmp,&all_obj);/*��ȡ�����ļ�*/
	obj_menu = (char **)malloc((obj_num + 1) * sizeof(char *));

	/*��all_obj���ַ�����\nΪ�ָ�����ת�����ַ�������obj_menu����ʽ*/
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
   obj_pad_ptr = newpad(obj_num + 1 + BOXED_LINES, BOXED_COLS + 1);/*����һ�����±����ü��±����������Դ�����ʾ���ڵ�����*/
	if(!obj_pad_ptr)
		return;
	mvprintw(4, 0, "Object Listing\n");

	mvprintw(MESSAGE_LINE, 0, "Please select a object, RETURN or q to exit");

 	for(option_ptr = obj_menu;*option_ptr;option_ptr++)	/*��obj_menu������д�����±���*/
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
	max_lines = obj_num-1 > BOXED_LINES?BOXED_LINES:obj_num-1;/*��ǰ������ʾ����������*/

	/*ͨ���ƶ����¼��������ƴ����е����ݣ����ƶ��������ʾ�û���ѡ��
	*���±��е�����������ڴ��ڵ���ʾ�����ݣ���ɽ����������¹�������ʾ��ǰδ�ڴ�����ʾ�ļ��±��е�����
	*ͨ���ƶ����¼�Ҳ���ƶ���꣬��ǰ������Ϊ�û���ѡ��
	*selected_rowΪ�û�ѡ��������ڼ��±��е��к�
	*cur_posΪ����ڵ�ǰ��ʾ�����е�λ��
	*first_lineΪ��ǰ��ʾ�����е�һ�е������ڼ��±��е��к�
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

		/*���ֻ���ڵ�ǰ��ʾ���ݵ���С�к������֮���ƶ�*/
		if(cur_pos < BOX_LINE_POS)
			cur_pos = BOX_LINE_POS;
		if(cur_pos > BOX_LINE_POS + max_lines)
			cur_pos = BOX_LINE_POS + max_lines;
		/*ѡ����кŲ��ܳ������±������ݵ�����кţ����Ҳ���С��0*/
		if(selected_row < 0)
			selected_row = 0;
		if(selected_row > obj_num - 1)
			selected_row = obj_num -1;		
		/* now draw the appropriate part of the pad on the screen */
		/*�����±���ָ�������������ʾ��������*/
		prefresh(obj_pad_ptr, first_line, 0,
			 BOX_LINE_POS, BOX_COL_POS,
			 BOX_LINE_POS + BOXED_LINES, BOX_COL_POS + BOXED_COLS);
			/*wrefresh(stdscr);*/
		move(cur_pos,BOX_COL_POS);/*�ƶ����*/
		key = getch();
		
	}
	/*�����û���ѡ��*/
	if(key == '\n' || key == KEY_ENTER){
		for(ptr = obj_menu[selected_row];*ptr;ptr++){
			if(*ptr == ' '){
				*ptr = '\0';
				break;
			}
		}
		mvprintw(PROMPT_LINE, 4, "You select:%s\n",obj_menu[selected_row]);
		move(Q_LINE,0);
		if(get_confirm()){/*ȷ��*/
			strcpy(cmd,"I ");
			strcat(cmd,obj_menu[selected_row]);
			send_len = strlen(cmd) + 1;
			write(clifd,cmd,send_len);/*����������������*/
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

/*�ָ�*/
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

	/*���������Ϣ*/
	obj_num = read_tmpfile(objtmp,&all_obj);
	if(obj_num == -1)
	           return;
	obj_menu = (char **)malloc((obj_num + 1) * sizeof(char *));

	/*�����ַ�����\nΪ�ָ���ת�����ַ�������*/
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
	/*��������Ϣд�����±���*/
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

	/*��ʾ������Ϣ���û��ƶ����ѡ�����inc_backup*/
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

	/*�����û���ѡ��*/
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
			write(clifd,cmd,send_len);/*���ͻָ�����*/
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

	/*����汾��Ϣ*/
	flag = 1;
	all_ver = (char *)malloc(BUFLEN * sizeof(char));
	/*�ӷ��������ذ汾�б������浽�ַ���������*/
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
	 /*���汾��Ϣд�����±���*/
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

	/*��ʾ�汾��Ϣ���ƶ�����Ա�ʶ�û���ѡ�����inc_backup*/
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

	/*�����û���ѡ��*/
	if(key == '\n' || key == KEY_ENTER){
		mvprintw(PROMPT_LINE, 4, "You select:%s\n",ver_menu[selected_row]);
		move(Q_LINE,0);
		if(get_confirm()){
			strcpy(cmd,ver_menu[selected_row]);
		}else{
			write(clifd,"EOD",4);/*ȡ���ָ�*/
			for(i = 0;i < ver_num;i++)
				free(ver_menu[i]);
			free(ver_menu);
			return;
		}
	}else{
			write(clifd,"EOD",4);/*ȡ���ָ�*/
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
		/*����·��*/
		clear_all_screen();
		mvprintw(MESSAGE_LINE, 10, "Input a subpath of %s or return:",path);
		get_string(buf);/*��ȡ�û������·����Ĭ��Ϊ����·��*/
		if(strcmp(buf,"") == 0)
			strcpy(buf,path);
		mvprintw(PROMPT_LINE, 4, "Your path:%s",buf);
		if(get_confirm()){
			strcpy(cmd + cmdlen," ");
			strcat(cmd,buf);
			send_len = strlen(cmd) + 1;
			write(clifd,cmd,send_len);/*���Ͱ汾��Ϣ��·����Ϣ*/
			rec_len = read(clifd,buf,BUFLEN);
			if(strcmp(buf,"cmd error") == 0){
				mvprintw(PROMPT_LINE, 4, "path error:\n");
				refresh();
				sleep(1);
				flag = 1;
			}
		}else{
			write(clifd,"EOD",4);/*ȡ���ָ�����*/
		}
	}while(flag);
}



