#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "lpel.h"


void taskFunc(void **arg) {
	lpel_task_t *self = (lpel_task_t *) *arg;
	int i;
	int n;
	int tid = LpelTaskGetId(self);
	int count  = (tid + 1) * 100;
	while (1) {
		n = 0;
		printf("task %d run on worker %d\n", tid, LpelTaskGetWorkerId(self));
		for (i = 0; i < count; i++) {
			n++;
		}
		sleep(2);
		printf("task %d count till %d, and now return to master\n", tid, n);
		LpelTaskYield();
	}
}

static void testBasic(void)
{
  lpel_config_t cfg;
  lpel_task_t *t1, *t2, *t3;
  void *arg1, *arg2, *arg3;
  cfg.num_workers = 3;
  cfg.proc_workers = 3;
  cfg.proc_others = 0;
  cfg.proc_sosi = 0;
  cfg.flags = 0;

  LpelInit(&cfg);

  t1 = LpelTaskCreate( 0, taskFunc, &arg1, 0);
  arg1 = t1;

  t2 = LpelTaskCreate( 0, taskFunc, &arg2, 0);
  arg2 = t2;

  t3 = LpelTaskCreate( 0, taskFunc, &arg3, 0);
  arg3 = t3;

  LpelTaskStart(t1);
  LpelTaskStart(t2);
  LpelTaskStart(t3);

  LpelStart();
  LpelCleanup();
  LpelMonCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
