#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include <lpel.h>

#include "arch/mctx.h"
#include "task.h"
#include "taskqueue.h"
#include "mailbox.h"

typedef struct workerctx_t {
  int wid;
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  lpel_task_t  *current_task;
  mon_worker_t *mon;
  mailbox_t    *mailbox;
  lpel_task_t  *wraptask;
  char          padding[64];
} workerctx_t;


typedef struct masterctx_t {
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  //mon_worker_t *mon; // FIXME
  mailbox_t    *mailbox;
  taskqueue_t  *ready_tasks;
  char          padding[64];
} masterctx_t;



#ifdef LPEL_DEBUG_WORKER
/* use the debug callback if available to print a debug message */
#define WORKER_DBGMSG(wc,...) do {\
  if ((wc)->mon && MON_CB(worker_debug)) { \
    MON_CB(worker_debug)( (wc)->mon, ##__VA_ARGS__ ); \
  }} while(0)

#else /* LPEL_DEBUG_WORKER */
#define WORKER_DBGMSG(wc,...) /*NOP*/
#endif /* LPEL_DEBUG_WORKER */

workerctx_t *LpelCreateWrapperContext();
workerctx_t *LpelWorkerSelf(void);
lpel_task_t *LpelWorkerCurrentTask(void);

void LpelWorkerTaskWakeup( lpel_task_t *whom);
void LpelWorkerTaskExit(lpel_task_t *t);
void LpelWorkerTaskYield(lpel_task_t *t);
void LpelWorkerTaskBlock(lpel_task_t *t);

void LpelWorkerBroadcast(workermsg_t *msg);

void LpelMasterInit( int size);
void LpelMasterCleanup( void);
void LpelMasterSpawn( void);
void LpelMasterTerminate(void);
void LpelStartTask( lpel_task_t *t);

#endif /* _WORKER_H_ */
