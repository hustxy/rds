/*
*对应于没有可靠性编码的server函数
*
*/

#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "apue.h"
#include "wdb.h"
#include "server.h"
#include "htable.h"
#include "apue_rvol.h"

#define HASH_TBALE_SIZE 16777215/*2^24-1*/
/*******************************************************/
extern char *restore_path;
extern char *rvol_name;
extern char *meta_workingdir;
extern char *rvol_workingdir;
//extern char *data_device[M];
//extern char *chk_device[N];
extern char *objtmp;


extern htable *jcrtbl;
extern void read_config(char *);
extern void write_tmpfile(const char *tmpfile,char *buf);
/******************************************************/

Client *client = NULL;
int		 bg = 0;

int main(int argc, char *argv[])
{
	int		c,i;
	MYJCR *jcr = NULL;
	DIR *dp;
    struct dirent *entry;
	char path[512],curpath[512];
	char expath[10];
	char buf[1024];
	char *p,*q;
	

	/*初始化hash表*/
   jcrtbl= init(jcr, &(jcr->link), HASH_TBALE_SIZE);
	read_config("wdb.conf");
	
	while ((c = getopt(argc, argv, "b")) != EOF) {
		switch (c) {
		case 'd':		/* debug */
			bg = 1;
			break;

		case '?':
			err_quit("unrecognized option: -%c", optopt);
		}   
	}

	unlink(objtmp);
    if((dp = opendir(meta_workingdir)) == NULL) {/*打开文件夹*/
        fprintf(stderr,"cannot open directory: %s\n", meta_workingdir);
        return 1;	
    } 
	//getcwd(curpath,512);
	while((entry = readdir(dp)) != NULL){
		if(strcmp(".",entry->d_name) == 0 || 
                strcmp("..",entry->d_name) == 0)
                continue;
		for(p = entry->d_name,q = path; *p != '.';)
			*q++ = *p++;
		*q = '\0';
		for(p++,q = expath;*p;)
			*q++ = *p++;
		*q = '\0';
		if(strcmp(expath,"idx") == 0){
			int fd;
			char *ptr;
			int32_t len;
			strcpy(buf,path);
			strcat(buf," ");
			ptr = buf + strlen(buf);
			strcat(path,".dat");
			/*此处为了读出备份对象的路径，这里为了方便未做任何异常处理*/
			//chdir(meta_workingdir);
			strcpy(curpath,meta_workingdir);
			strcat(curpath,"/");
			strcat(curpath,path);
			if((fd = open(curpath,O_RDONLY)) < 0){
				printf("%s %d:open error!\n",__FILE__,__LINE__);
				return 1;
			}
			//chdir(curpath);
			if(read(fd,ptr,1024) < 0){/*一次多读一些数据到内存中，然后截取需要的数据*/
				printf("%s %d:read error!\n",__FILE__,__LINE__);
				return 1;
			}
			len = *((int32_t *)ptr);
			memmove(ptr,ptr + sizeof(int32_t),len);
			write_tmpfile(objtmp,buf);
			close(fd);
		}
	}
	closedir(dp);

	loop();		/* never returns */

	for(i = 0 ; i < M ; i++){
		if(data_device[i])
			free(data_device[i]);
	}
//	for(i = 0 ; i < N ; i++){
//		if(chk_device[i])
//			free(chk_device[i]);
//	}
	if(restore_path)
		free(restore_path);
	if(meta_workingdir)
		free(meta_workingdir);
	if(rvol_workingdir)
		free(rvol_workingdir);
	destroy(jcrtbl);/*销毁hash表*/
   free(jcrtbl);
   printf("Freed jcrtbl\n");
	exit(0);
}

