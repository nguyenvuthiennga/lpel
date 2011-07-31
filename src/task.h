#ifndef _TASK_H_
#define _TASK_H_



#include "arch/mctx.h"


#include "arch/atomic.h"

#include "lpel_main.h"
#include "scheduler.h"


/**
 * If a task size <= 0 is specified,
 * use the default size
 */
#define LPEL_TASK_SIZE_DEFAULT  8192  /* 8k */

struct workerctx_t;
struct lpel_task_t;
struct mon_task_t;



/* task function signature */
typedef void *(*lpel_taskfunc_t)(void *inarg);



/**
 * TASK CONTROL BLOCK
 */
typedef struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  struct lpel_task_t *prev, *next;
  unsigned int uid;    /** unique identifier */
  enum lpel_taskstate_t state;   /** state */

  struct workerctx_t *worker_context;  /** worker context for this task */

  sched_task_t sched_info;

  /**
   * indicates the SD which points to the stream which has new data
   * and caused this task to be woken up
   */
  struct lpel_stream_desc_t *wakeup_sd;
  atomic_t poll_token;        /** poll token, accessed concurrently */

  /* ACCOUNTING INFORMATION */
  struct mon_task_t *mon;

  /* CODE */
  int size;             /** complete size of the task, incl stack */
  mctx_t mctx;          /** machine context of the task*/
  lpel_taskfunc_t func; /** function of the task */
  void *inarg;          /** input argument  */
  void *outarg;         /** output argument  */
} lpel_task_t;




lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
    void *inarg, int stacksize );

void LpelTaskDestroy( lpel_task_t *t);


void LpelTaskMonitor(lpel_task_t *t, mon_task_t *mt);
void LpelTaskPrio(lpel_task_t *t, int prio);
void LpelTaskRun( lpel_task_t *t);

lpel_task_t *LpelTaskSelf(void);
void LpelTaskExit(void *outarg);
void LpelTaskYield(void);

unsigned int LpelTaskGetID(lpel_task_t *t);
mon_task_t *LpelTaskGetMon( lpel_task_t *t );

void LpelTaskBlock( lpel_task_t *t );
void LpelTaskBlockStream( lpel_task_t *ct);
void LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked);


#endif /* _TASK_H_ */