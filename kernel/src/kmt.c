#include<common.h>
#include<klib.h>
#include<my_os.h>

void kmt_init();
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);

void teardown(task_t *task);
void spin_init(spinlock_t *lk, const char *name);
void spin_lock(spinlock_t *lk);
void spin_unlock(spinlock_t *lk);
void sem_init(sem_t *sem, const char *name, int value);
void sem_wait(sem_t *sem);
void sem_signal(sem_t *sem);
  
MODULE_DEF(kmt) {
  .init = kmt_init,
  .create = kmt_create,
  .teardown = teardown,
  .spin_init = spin_init,
  .spin_lock = spin_lock,
  .spin_unlock = spin_unlock,
  .sem_init = sem_init,
  .sem_wait = sem_wait,
  .sem_signal = sem_signal,
};
