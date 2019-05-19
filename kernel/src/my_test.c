#include<my_os.h>
//char *ptr[8][100];
void my_pmmtest(){
  /*int i = 0;
  while(i<100){
    //test_lock();
    ptr[i] = pmm->alloc(1000);
    if(ptr[i]==NULL){
      assert(0);
      return;
    }
    sprintf(ptr[i], "hello%d\n",i);
    printf("%d\n",i);
    i++;
  }
  while(i>0){
    printf("%s\n",ptr[--i]);
    pmm->free(ptr[i]);
  }
  assert(a_head->nxt==NULL);*/
  char s[100];
  int cnt = 0;
  while(1){
    char *ptr = pmm->alloc(100);
    sprintf(ptr,"cpu:%d | cnt: %d\n",_cpu(),cnt);
    cnt++;
    printf("%s\n",ptr);
    pmm->free(ptr);
  }
}

void echo_task(void *name) {
  device_t *tty = dev_lookup(name);
  while (1) {
    char line[1014], text[1024];
    sprintf(text, "(%s) $ ", name); 
    
    tty->ops->write(tty, 0, text, strlen(text));
    int nread = tty->ops->read(tty, 0, line, sizeof(line));
    line[nread - 1] = '\0';
    sprintf(text, "Echo: %s.\n", line); 
    tty->ops->write(tty, 0, text, strlen(text));
  }
}
