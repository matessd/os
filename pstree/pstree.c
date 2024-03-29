#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

int a_op[3];//分别代表-p，-n和-V是否存在
#define MAX_PID 32768
#define MAX_NAME 256
typedef struct{
    int pid;//进程号
    int ppid;//父亲
    char name[MAX_NAME];//进程名
}stProcess;
stProcess a_process[MAX_PID];
int a_grand[MAX_PID];//pid的祖先
int a_child_num[MAX_PID];
int a_pid_num = 0;//不考虑祖先为2的情况下，当前找到几个进程
#define MAX_LINE_LEN 1024
char aa_out[MAX_PID][MAX_LINE_LEN];

void fnRead_proc(FILE* fp);
void fnMake_tree();

#define true 1
#define false 0

void fnRead_data(){
  //below is read dir
  DIR *dir = opendir("/proc");
  assert(dir);
  struct dirent *ptr;
  char file_name[512];
  while((ptr = readdir(dir))){
      if(ptr->d_name[0]>'0' && ptr->d_name[0]<='9'){
          //只读取以数字命名的目录
          //printf("%s\n",ptr->d_name);
          sprintf(file_name,"/proc/%s/status",ptr->d_name);
          FILE* fp = fopen(file_name,"r");
          fnRead_proc(fp);
          
          //read task dir
          char sub_file_name[512];
          char dir_name[255];
          struct dirent *sub_ptr;
          sprintf(dir_name,"/proc/%s/task",ptr->d_name);
          DIR* sub_dir = opendir(dir_name);
          for(int i=0; i<3; i++){
              //跳过task中的前三个文件夹. .. 和自身线程
              sub_ptr = readdir(sub_dir);
          }
          while( (sub_ptr = readdir(sub_dir)) ){
              sprintf(sub_file_name,"%s/%s/status",dir_name,sub_ptr->d_name);
              //printf("%s\n",sub_file_name);
              fp = fopen(sub_file_name,"r");
              fnRead_proc(fp);
          } 
      }
  }
}

int main(int argc, char *argv[]) {
  //printf("Hello, World!\n");
  int i;		 
  for (i = 0; i < argc; i++) { 
    assert(argv[i]); // always true
	if(!strcmp(argv[i],"-p")||!strcmp(argv[i],"--show-pids"))
		a_op[0] = true;
	if(!strcmp(argv[i],"-n")||!strcmp(argv[i],"--numeric-sort"))
		a_op[1] = true;
	if(!strcmp(argv[i],"-V")||!strcmp(argv[i],"--version"))
		a_op[2] = true;
    //printf("argv[%d] = %s\n", i, argv[i]); //teacher's code
  }
  assert(!argv[argc]); // always true
  if(a_op[2]==true){
      printf("This is my version\n");
      return 0;
  } 
  fnRead_data();
  fnMake_tree(); 
  return 0;
}

#define BUFF_LEN 1024//读取一行缓冲区的最大长度

void fnRead_proc(FILE* fp){
    char tmp[32];
    char line_buff[BUFF_LEN];//暂存行
    char name[256];
    int pid, ppid, tgid;
    while(fgets(line_buff, BUFF_LEN, fp)!=NULL){
        sscanf(line_buff,"%s",tmp);
        if(strcmp(tmp,"Name:")==0){
            int len = strlen(tmp);
            //line_buff[len]不是空格，可能是制表符什么的，看着像很多个空格
            for(len=len; line_buff[len]==' '||line_buff[len]=='\t'; len++);
            strcpy(name, &line_buff[len]);
            len = strlen(name);
            name[len-1] = '\0';
        }else if(strcmp(tmp,"Tgid:")==0){
            sscanf(line_buff,"%s %d",tmp,&tgid);
        }else if(strcmp(tmp,"PPid:")==0){
            sscanf(line_buff,"%s %d",tmp,&ppid);
        }else if(strcmp(tmp,"Pid:")==0){
            sscanf(line_buff,"%s %d",tmp,&pid);
        }
    }    
    //线程的父亲认为是tgid
    ppid = (pid==tgid)?ppid:tgid;

    //处理得到的信息
    a_grand[pid] = a_grand[ppid];
    if(pid==1||pid==2){
        a_grand[pid] = pid;
    } 
    if(a_grand[pid]==2){
        return;
    }
    a_child_num[ppid]++;
    a_process[++a_pid_num].pid = pid;
    a_process[a_pid_num].ppid = ppid;
    strcpy(a_process[a_pid_num].name, name);
    //printf("%d %d %d\n",pid,ppid,a_pid_num);
}

int a_vis[MAX_PID];

int cmp(const void* a, const void* b){
    stProcess* aa = (stProcess*)a;
    stProcess* bb = (stProcess*)b;
    if(a_op[1]==true){
        //-n模式，按pid大小排序
        if(aa->pid > bb->pid)
            return 1;
        else return 0;
    }
    //非-n，按字母序排序
    if(strcmp(aa->name,bb->name)>0)
        return 1;
    else return 0;
}

int fnDFS(int pid,  char* name, int x, int y){
    a_vis[pid] = true;
    //printf("%d\n",pid);
    int loop_flag = true;
    char name_num[512];
    if(a_op[0]==true){
        sprintf(name_num,"%s(%d)",name,pid);
    }else{
        sprintf(name_num,"%s",name);
    }
    strcpy(&aa_out[x][y], name_num);
    //先把进程名拷进来
    y += strlen(name_num);
    //printf("%d&\n",y);
    int width = 0;
    int x0 = x;
    while(loop_flag){
        for(int i=1; i<=a_pid_num; i++){
            int child_pid = a_process[i].pid;
            if(a_process[i].ppid==pid && a_vis[child_pid]==false){
                if(width==0){
                    if(a_child_num[pid]==1){
                        aa_out[x][y] = (char)0x5;
                    }else{
                        aa_out[x][y] = (char)0x1;
                    }
                    //这是特殊符号，内存一个占3个char
                    //但是打印出来只占一空格
                    //所以下一行只要空出一格对齐
                }else{
                    for(int j=1; j<width; j++){
                        aa_out[x+j][y] = (char) 0x2;
                    }
                    x+=width;
                    aa_out[x][y] = (char)0x3;
                }
                width = fnDFS(child_pid, a_process[i].name, x, y+2);
                break;
            }
            if(i==a_pid_num)
                loop_flag = false;
        }
    }
    x += width;
    int ret = (width==0)?1:x-x0;
    if(ret>1){
        aa_out[x-width][y] = (char) 0x4;
    }
    return ret;
}

char aa_chSpec[6][10];
void fnMake_tree(){
    //a_pid_num不可能连2个都没有
    qsort(a_process+2,a_pid_num-1,sizeof(a_process[0]),cmp);
    memset(aa_out,' ',sizeof(aa_out));
    int line_cnt = fnDFS(1,a_process[1].name, 0, 0);
    strcpy(&aa_chSpec[1][0],"┬─");
    strcpy(&aa_chSpec[2][0],"│ ");
    strcpy(&aa_chSpec[3][0],"├─");
    strcpy(&aa_chSpec[4][0],"└─");
    strcpy(&aa_chSpec[5][0],"──");
    for(int i=0; i<line_cnt; i++){
        int j = -1;
        while(aa_out[i][++j]!='\0'){
            int ASC = (int)aa_out[i][j];
            if((int)aa_out[i][j]<=0x5){
                printf("%s",&aa_chSpec[ASC][0]);
                j++;
            }else{
                printf("%c",aa_out[i][j]);
            }
        }
        printf("\n");
    }
}

