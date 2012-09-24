#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "task.h"
#include "configuration.h"

#ifdef _USE_FIFO_QUEUE_
struct taskqueue_t{
  lpel_task_t *head, *tail;
  unsigned int count;
};
#endif

#ifdef _USE_PRIORITY_QUEUE_
struct taskqueue_t{
  lpel_task_t **heap;
  unsigned int count;
  unsigned int alloc;
};
#endif


typedef struct taskqueue_t taskqueue_t;

taskqueue_t *LpelTaskqueueInit();

void LpelTaskqueuePush(  taskqueue_t *tq, lpel_task_t *t);
lpel_task_t *LpelTaskqueuePop( taskqueue_t *tq);

int LpelTaskqueueSize(taskqueue_t *tq) ;
void LpelTaskqueueDestroy(taskqueue_t *tq);
lpel_task_t *LpelTaskqueuePeek( taskqueue_t *tq);
void LpelTaskqueueUpdatePrior(taskqueue_t *tq, lpel_task_t *t, double np);

#endif
