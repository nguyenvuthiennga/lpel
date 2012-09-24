#ifndef _WORKERMSG_H_
#define _WORKERMSG_H_

#include "task.h"

/*
 * worker msg body
 */
typedef enum {
  MSG_TERMINATE = 1,
  MSG_TASKASSIGN,
  MSG_TASKRET,		// task return by worker
  MSG_TASKREQ,
  MSG_TASKWAKEUP,		//sent to both master and wrapper
  MSG_UPDATEPRIOR,	//sent to master only
} workermsg_type_t;

typedef struct {
  workermsg_type_t  type;
  union {
    lpel_task_t    *task;
    int            from_worker;
  } body;
} workermsg_t;

#endif /* _WORKERMSG_H_ */
