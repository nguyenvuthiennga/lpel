#include <stdlib.h>
#include "../taskqueue.h"
#include <lpel.h>

#include <assert.h>
#include "../task.h"

#ifdef _USE_FIFO_QUEUE_
/**
 * A simple doubly linked list for task queues
 * - enqueue at tail, dequeue from head (FIFO)
 *
 * Invariants:
 *   (head != NULL)   =>  (head->prev == NULL) AND
 *   (tail != NULL)   =>  (tail->next == NULL) AND
 *   (head == NULL)  <=>  (tail == NULL)
 */



/**
 * Initialise a taskqueue
 */

taskqueue_t* LpelTaskqueueInit()
{
	taskqueue_t *tq = (taskqueue_t *) malloc(sizeof(taskqueue_t));
  tq->head = NULL;
  tq->tail = NULL;
  tq->count = 0;
}


/**
 * Enqueue a task at the tail
 *
 */
void LpelTaskqueuePush(taskqueue_t *tq, lpel_task_t *t)
{
  assert( t->prev==NULL && t->next==NULL );

  if ( tq->head == NULL ) {
    tq->head = t;
    /* t->prev = NULL; is precond */
  } else {
    tq->tail->next = t;
    t->prev = tq->tail;
  }
  tq->tail = t;
  /* t->next = NULL; is precond */

  /* increment task count */
  tq->count++;
}




/**
 * Dequeue a task from the head
 *
 * @return NULL if taskqueue is empty
 */
lpel_task_t *LpelTaskqueuePop(taskqueue_t *tq)
{
  lpel_task_t *t;

  if ( tq->head == NULL ) return NULL;

  t = tq->head;
  /* t->prev == NULL by invariant */
  if ( t->next == NULL ) {
    /* t is the single element in the list */
    /* t->next == t->prev == NULL by invariant */
    tq->head = NULL;
    tq->tail = NULL;
  } else {
    tq->head = t->next;
    tq->head->prev = NULL;
    t->next = NULL;
  }
  /* decrement task count */
  tq->count--;
  assert( t!=NULL || tq->count==0);
  assert(t->next==NULL && t->prev==NULL);
  return t;
}


int LpelTaskqueueSize(taskqueue_t *tq){ return tq->count; }


void LpelTaskqueueDestroy(taskqueue_t *tq) {
	assert(tq->count == 0);
	free (tq);
}


lpel_task_t *LpelTaskqueuePeek( taskqueue_t *tq) {
	return tq->head;
}

void LpelTaskqueueUpdatePrior(taskqueue_t *tq, lpel_task_t *t, double np) {}


#endif
