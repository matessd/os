#include<my_os.h>
void sem_init(sem_t *sem, const char *name, int value){
  sem->value = value;
  sem->name = name;
  kmt->spin_init(&sem->lk, name);
  //kmt->spin_init(&sem->lk2, name);
  sem->end = sem->start = 0;
  return;
}
void sem_wait(sem_t *sem){
  kmt->spin_lock(&sem->lk);
  //printf("name: %s\n",sem->name);
  //printf("wait: %s\n",current->name);
  //printf("value: %d\n\n",sem->value);
  sem->value--;
  if (sem->value < 0) {
    //assert(task_head!=NULL);
    current->sleep_flg = 1;
    sem->queue[sem->end] = current;
    sem->end = (sem->end + 1) % NPROC;
    //先解锁？
    kmt->spin_unlock(&sem->lk);
    _yield();
    //kmt->spin_lock(&sem->lk);
  }else 
    kmt->spin_unlock(&sem->lk);
  return;
}
void sem_signal(sem_t *sem){
  kmt->spin_lock(&sem->lk);
  //printf("name: %s\n",sem->name);
  //printf("sig: %s\n",current->name);
  //printf("value: %d\n\n",sem->value);
  sem->value++;
  if (sem->value <= 0) {
    sem->queue[sem->start]->sleep_flg = 0;

    //printf("sem: %s\n",sem->queue[sem->start]->name);
    //kmt->spin_lock(task_lk);
    //add_tail(sem->queue[sem->start]);
    //kmt->spin_unlock(task_lk);

    sem->queue[sem->start] = NULL;
    sem->start = (sem->start + 1) % NPROC;
  }
  kmt->spin_unlock(&sem->lk);
  return;
}
