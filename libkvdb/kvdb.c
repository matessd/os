#include "kvdb.h"
#include <assert.h>
//#include <time.h>
int SEEK1 = 16*1024*1024+512;
int SEEK2 = 4;

/*int flip_a_coin(){
  //srand(time(NULL));
  if(rand()%1000<10)
   return 1;
  else return 0; 
}

void may_crash() {
  if (flip_a_coin()) {
    exit(0); // crash
  }
}*/

int recover(kvdb_t *db){
  if(db->fp==NULL) return -1;
  fseek(db->fp,SEEK2,SEEK_SET);
  int case_num = 0, off1=0, off2=0, len;
  fscanf(db->fp,"%d",&case_num);
  char key[130];
  char *value = malloc(16*1024*1024);
  if(value==NULL) return -1;
  if(case_num==1){
    fscanf(db->fp,"%d %s",&off1,value);
    fseek(db->fp,off1,SEEK_SET);
    fprintf(db->fp,"%s\n",value);
  }else if(case_num==2){
    fscanf(db->fp,"%d %d %d %s %s\n",&off1,&off2,&len,key,value);
    fseek(db->fp,off1,SEEK_SET);
    fprintf(db->fp,"0");
    //may_crash();
    fseek(db->fp,off2,SEEK_SET);
    fprintf(db->fp,"%d %s 1 %s\n",len,key,value);
  }else if(case_num==3){
    fscanf(db->fp,"%d %d %s %s",&off1,&len,key,value);
    fseek(db->fp,off1,SEEK_SET);
    fprintf(db->fp,"%d %s 1 %s\n",len,key,value);
  }else{
    return -1;
  }
  //may_crash();
  free(value);
  fseek(db->fp,0,SEEK_SET);
  fprintf(db->fp,"0 0\n");
  //if(fsync(db->fd)==-1) return -1;
  return 0;
}

int kvdb_open(kvdb_t *db, const char *filename) {
  int a,b;
  int fd = open(filename, O_RDWR|O_CREAT,  0777);
  if(fd==-1) return -1;
  FILE *fp = fdopen(fd,"r+");
  if(fp==NULL) return -1;
  db->fd = fd;
  db->fp = fp;
  //if(pthread_mutex_init(&db->mutex,NULL)) return -1;
  //pthread_mutex_lock(&db->mutex);
  flock(fd, LOCK_EX);
  fseek(fp,0,SEEK_SET);
  if(fscanf(fp, "%d %d",&a,&b)==EOF){
    fseek(fp,0,SEEK_SET);
    fprintf(fp,"0 0\n");
    for(int i=SEEK2; i<SEEK1; i++)
      fputc('*',fp);
    fputc('\n',fp);
  }
  fseek(fp,0,SEEK_SET);
  fscanf(fp,"%d %d",&a,&b);
  db->ifopen = 1;
  if(a==1&&b==1) 
    if(recover(db))
      return -1;
  flock(fd, LOCK_UN);
  //pthread_mutex_unlock(&db->mutex);
  return 0;
}

int kvdb_close(kvdb_t *db){
  db->ifopen = 0;
  int ret = fclose(db->fp);
  //pthread_mutex_destroy(&db->mutex);
  db->fp = NULL;
  db->fd = -1;
  return ret;
}

int kvdb_put(kvdb_t *db, const char *key, const char *value){
  //pthread_mutex_lock(&db->mutex);
  if(db->ifopen==0) return 1;
  char tkey[130], tmpc;  tkey[0] = '\0';
  int used=0, cnt=0, ok=0;
  int len = strlen(value), off1 = 0, off2 = 0;

  flock(db->fd, LOCK_EX);
  fseek(db->fp,SEEK1,SEEK_SET);
  while(1){
    fscanf(db->fp,"%d %s %d%c",&cnt,tkey,&used,&tmpc);
    if(strcmp(tkey,key)==0&&used==1) {
      ok = 1; break;
    }
    if(feof(db->fp)) break;
    fseek(db->fp,cnt+1,SEEK_CUR);
  }

  if(ok==1){
    if(len<=cnt){
      off1 = ftell(db->fp);
      fseek(db->fp, SEEK2, SEEK_SET);
      fprintf(db->fp,"1\n%d %s\n",off1,value);
    }else {
      if(fseek(db->fp,-2,SEEK_CUR)!=0)
        return 2;
      off1 = ftell(db->fp);
      fseek(db->fp,0,SEEK_END);
      off2 = ftell(db->fp);
      fseek(db->fp, SEEK2, SEEK_SET);
      fprintf(db->fp,"2\n%d \n%d %d %s %s\n",off1,off2,len,key,value);
    }
  }else{
    fseek(db->fp,0,SEEK_END);
    off1 = ftell(db->fp);
    fseek(db->fp,SEEK2,SEEK_SET);
    //ret = fprintf(db->fp,"%d %s 1 %s\n",len,key,value);
    fprintf(db->fp,"3\n%d %d %s %s\n",off1,len,key,value);
  }
  //may_crash();
  fseek(db->fp,0,SEEK_SET);
  fprintf(db->fp,"1 1\n");
  //may_crash();
  if(recover(db)) return -1;
  flock(db->fd, LOCK_UN);
  //pthread_mutex_unlock(&db->mutex);
  //if(fsync(db->fd)==-1) return -1;
  return 0;
}

char *kvdb_get(kvdb_t *db, const char *key){
  //pthread_mutex_lock(&db->mutex);
  if(db->ifopen==0) return NULL;
  if(fseek(db->fp,SEEK1,SEEK_SET)!=0) return NULL;
  //assert(db->ifopen!=0);
  //assert(fseek(db->fp,SEEK1,SEEK_SET)==0);
  char tkey[130], tmpc; tkey[0]='\0';
  char *value = malloc(16*1024*1024);//16MB
  if(value==NULL) return NULL;
  //assert(value!=NULL);
  int cnt, used;

  flock(db->fd, LOCK_EX);
  while(1){
    if(fscanf(db->fp,"%d %s %d%c",&cnt,tkey,&used,&tmpc)==EOF)
      return NULL;
    if(strcmp(tkey,key)==0&&used==1){
      fscanf(db->fp,"%s",value);
      break;
    }
    fseek(db->fp,cnt+1,SEEK_CUR);
  }
  flock(db->fd, LOCK_UN);
  //pthread_mutex_unlock(&db->mutex);
  return value;
}

