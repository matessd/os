#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<assert.h>
#include<sys/mman.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>

#define MB (1024*1024)
//扇区sector大小为512字节
#define SECSZ 512
#define FATNUM 2
unsigned char *start=NULL, *tmp_start;
int32_t FD, FILE_SZ/*文件大小*/, FAT_SEC/*FAT扇区数*/, RES_SEC/*保留扇区数*/, SEC_PER_CLU/*每簇扇区数，簇：cluster*/, ST_CLU/*起始簇号*/, data_off/*数据区偏移*/, RES/*数据区大小*/;
typedef struct{
  unsigned char name[15];
  int8_t vis;
  uint32_t stclu, fsz;
}SDE;
typedef struct{
  unsigned char name[15];
  unsigned char checksum;
  unsigned char idx; 
  int8_t vis;
}LDE;
#define MAX_NAME 60
typedef struct{
  unsigned char name[MAX_NAME];
  unsigned char checksum;
  int8_t ok;
  uint32_t stclu, fsz;
}myDIR;
SDE sde[30000];
LDE lde[30000];
myDIR dir[30000];
int scnt, lcnt, dircnt;

int file_size2(char* filename){  
    struct stat statbuf;  
    stat(filename,&statbuf);  
    int size=statbuf.st_size;    
    return size;  
}

void init(char *filename){
  FILE_SZ = file_size2(filename);
  FD = open(filename, O_RDWR);
  assert(FD!=-1);
  //64*MB = 0x4,000,000
  start = mmap(NULL, FILE_SZ, PROT_READ, MAP_PRIVATE, FD, 0);
  assert((intptr_t)start!=-1);
  tmp_start = start;

  FAT_SEC = *(int16_t*)(start+0x16);
  if(FAT_SEC==0) FAT_SEC = *(int32_t*)(start+0x24);//0x1f8
  RES_SEC = *(int16_t*)(start+0xe);//0x20
  SEC_PER_CLU = *(int8_t*)(start+0xd);//0x1
  ST_CLU = *(int32_t*)(start+0x2c);//0x2
  data_off = (RES_SEC + FAT_SEC*FATNUM + (ST_CLU-2)*SEC_PER_CLU)*SECSZ;//0x82000
  //printf("%x\n",64*MB);
  start = start+data_off;
  RES = FILE_SZ - data_off;
}

void find_sde(){
  unsigned char *cur = NULL;  scnt = 0;
  for(int i=0; i<RES/32; i++){
    cur = start+i*32;
    if(cur[0xc]==0 &&cur[0xb]==0x20) {
      if(*cur==0xe5) {
        sde[scnt].name[0] = '-';
        sprintf((char*)&sde[scnt].name[1],"%s",cur+1);
      }else{
        sprintf((char*)sde[scnt].name,"%s",cur);
      }
      sde[scnt].vis = 0;
      uint32_t l = *(uint16_t*)(cur+0x1a);
      uint32_t h = *(uint16_t*)(cur+0x14);
      h <<= 16;
      sde[scnt].stclu = h|l;
      sde[scnt].fsz = *(uint32_t*)(cur+0x1c);
      sde[scnt++].name[11]='\0';
    }
  }
  //printf("scnt:%d\n",scnt);
}

int uniread(unsigned char *dst, unsigned char *src, int cnt){
  int ret = 0;
  for(int i=0; i<cnt; i++){
    dst[i] = src[i*2];
    ret++;
    if((src[i*2]==0x0&&src[i*2+1]==0x0) || (src[i*2]==0xff&&src[i*2+1]==0xff)){
      ret--;
      break;
    }
  }
  dst[ret] = '\0';
  return ret;
}  

void find_lde(){
  unsigned char *cur = NULL, tmp[MAX_NAME];  
  lcnt = 0;  
  for(int i=0; i<RES/32; i++){
    cur = start+i*32;
    if(cur[0xc]==0 &&cur[0xb]==0xf&&cur[0x1a]==0) {
      //printf("0x%x &&",*cur);
      int cnt = 0;
      cnt += uniread(&tmp[cnt], &cur[1], 5);
      cnt += uniread(&tmp[cnt], &cur[0xe], 6);
      cnt += uniread(&tmp[cnt], &cur[0x1c], 2); 
      strcpy((char*)lde[lcnt].name, (char*)tmp);
      //if(*cur==0xe5) lde[lcnt].ife5 = 1;
      //else lde[lcnt].ife5 = 0;
      lde[lcnt].checksum = cur[0xd];
      lde[lcnt].idx = *cur;
      lde[lcnt].vis = 0;
      //printf("%s*\n",tmp);
      lcnt++;
    }
  }
  //printf("lcnt:%d*\n",lcnt);  
}

void merge_lde(){
  unsigned char tmp[MAX_NAME]; dircnt=0;
  for(int i=lcnt-1; i>=0; i--){
    if(lde[i].vis==1) continue;
    lde[i].vis = 1;
    strcpy((char*)tmp, (char*)lde[i].name);
    /*if(tmp[0]=='p'&&tmp[1]=='\0'){
      lde[i].vis = 0;
      continue;
    }*/
    unsigned char checksum = lde[i].checksum;
    unsigned char idx = lde[i].idx;
    if(idx==0xe5){
      for(int j=lcnt-1; j>=0; j--){
        if(lde[j].vis) continue;
        if((lde[j].idx!=0xe5)
            ||(lde[j].checksum!=checksum))
          continue;
        lde[j].vis=1;
        strcat((char*)tmp, (char*)lde[j].name);
      }
    }
    else if((idx&0x40)==0){
      for(int j=i-1; j>=0; j--){
        if(lde[j].vis) continue;
        if(((lde[j].idx&0x1f)!=(idx&0x1f)+1) 
           || lde[j].checksum!=checksum)
           continue;
        lde[j].vis=1;
        strcat((char*)tmp, (char*)lde[j].name);
        if(lde[j].idx&0x40) break;
      }
    }
    strcpy((char*)dir[dircnt].name, (char*)tmp);
    dir[dircnt].checksum = checksum;
    dir[dircnt].ok = 0;
    //dir[dircnt].ife5 = lde[i].ife5;
    dircnt++;
    //printf("%s*\n",tmp);
    int len = strlen((char*)tmp);
    if(tmp[0]=='p'&&tmp[1]=='\0'){
      dircnt--;
      continue;
    }
    if(tmp[len-1]!='p'){
      dircnt--;
      continue;
    }
  }
  //printf("dircnt:%d\n",dircnt);
}

unsigned char compute_checksum(unsigned char* shortname){
  int i, j = 0; 
  unsigned char chksum=0;
  for (i = 11; i > 0; i--)
    chksum = ((chksum & 1) ? 0x80 : 0) + (chksum >> 1) + shortname[j++];
  return chksum;
}

void traverse(){
  int cnt=0;
  unsigned char tmp[15];
  for(int i=dircnt-1; i>=0; i--){
    unsigned char checksum = dir[i].checksum;
    for(int j=0; j<scnt; j++){
      if(sde[j].vis==1) continue;
      strcpy((char*)tmp, (char*)sde[j].name);
      if(tmp[0]=='-') tmp[0] = dir[i].name[0];
      if((compute_checksum(tmp)==checksum)){
        sde[j].vis = 1;
        if(sde[j].fsz==0) break;
        dir[i].ok = 1;
        dir[i].fsz = sde[j].fsz;
        dir[i].stclu = sde[j].stclu;
        cnt++;
        //printf("%s  %s\n",tmp,dir[i].name);
        break;
      }
    }
  }
  //printf("okcnt:%d\n",cnt);
}

void recover(int flg){
  //printf("%d\n",dircnt);
  int cnt = 0, bmpfd;
  unsigned char *bmpst, env[100];
  strcpy((char*)env, "sha1sum ");
  for(int i=dircnt-1; i>=0; i--){
    if(dir[i].ok==1){
      bmpst = start+(dir[i].stclu-2)*SECSZ;
      //printf("%x\n",bmpst[dir[i].fsz-1]);
      if(flg&&(bmpst[dir[i].fsz]!='\0')&&(bmpst[dir[i].fsz+1]!='\0')){
        continue;
      }
      //assert(bmpst[0]==0x42&&bmpst[1]==0x4d);
      if(*(uint32_t*)(bmpst+0xe)!=0x28){
        dir[i].ok = 0; 
        continue;
        //printf("%s\n",dir[i].name);
      }
      dir[i].ok = 0;
      cnt++;
      bmpfd = open((char*)dir[i].name,O_RDWR|O_CREAT|O_TRUNC, 0777);
      //assert(bmpfd!=-1);
      if(bmpfd==-1) continue;
      int ret = write(bmpfd, (char*)bmpst, dir[i].fsz);
      //assert(ret!=-1&&ret!=0);
      if(ret==-1||ret==0) continue;

      strcpy((char*)&env[8], (char*)dir[i].name);
      system((char*)env);
      unlink((char*)dir[i].name);
      close(bmpfd);
    }
  }
  //printf("%d*\n",cnt);
}

int main(int argc, char *argv[]) { 
  init(argv[1]);
  find_sde();
  find_lde();
  merge_lde();
  traverse();
  recover(1);
  recover(0);
  munmap(tmp_start, FILE_SZ);
  close(FD);
  return 0;
}
