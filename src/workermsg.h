#ifndef _WORKERMSG_H_
#define _WORKERMSG_H_

#include "task.h"

/*
 * worker msg body
 */
typedef int workermsg_type_t;

#define MSG_TERMINATE  			1
#define MSG_TASKASSIGN			2
#define MSG_TASKRET					3		// task return by worker
#define MSG_TASKREQ					4
#define MSG_TASKWAKEUP			5	//sent to both master and wrapper
#define MSG_UPDATEPRIOR			6	//sent to master only

typedef struct {
  workermsg_type_t  type;
  union {
    lpel_task_t    *task;
    int            from_worker;
  } body;
} workermsg_t;

#endif /* _WORKERMSG_H_ */
