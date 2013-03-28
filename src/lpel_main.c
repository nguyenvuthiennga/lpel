/**
 * Main LPEL module
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <errno.h>
#include <sched.h>
#include <unistd.h>  /* sysconf() */
#include <pthread.h> /* worker threads are OS threads */

#include <lpel.h>

#include "arch/mctx.h"
#include "lpel_main.h"
#include "lpelcfg.h"
#include "worker.h"


#ifdef HAVE_PROC_CAPABILITIES
#  include <sys/capability.h>
#endif



/* test if flags are set in lpel config */
#define LPEL_ICFG(f)   ( (_lpel_global_config.flags & (f)) == (f) )

#ifdef HAVE_PTHREAD_SETAFFINITY_NP
/* cpuset for others-threads = [proc_workers, ...]
 * is used if FLAG_EXCLUSIVE is set & FLAG_PINNED is not set
 * */
static cpu_set_t cpuset_others;
static int offset_others;
static int rot_others;

/*
 * cpuset for workers = [0,proc_workers-1]
 * is only used if not FLAG_PINNED is set
 */
static cpu_set_t cpuset_workers;
static int rot_workers;
//static int offset_workers = 0;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */



/**
 * Get the number of available cores
 */
int LpelGetNumCores( int *result)
{
  int proc_avail = -1;
#ifdef HAVE_SYSCONF
  /* query the number of CPUs */
  proc_avail = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  if (proc_avail == -1) {
      char *p = getenv("LPEL_NUM_WORKERS");
      if (p != 0)
      {
          unsigned long n = strtoul(p, 0, 0);
          if (errno != EINVAL)
              proc_avail = n;
      }
  }
  if (proc_avail == -1) {
    return LPEL_ERR_FAIL;
  }
  *result = proc_avail;
  return 0;
}

int LpelCanSetExclusive( int *result)
{
#ifdef HAVE_PROC_CAPABILITIES
  cap_t caps;
  cap_flag_value_t cap;
  /* obtain caps of process */
  caps = cap_get_proc();
  if (caps == NULL) {
    return LPEL_ERR_FAIL;
  }
  cap_get_flag(caps, CAP_SYS_NICE, CAP_EFFECTIVE, &cap);
  *result = (cap == CAP_SET);
  return 0;
#else
  return LPEL_ERR_FAIL;
#endif
}





static int CheckConfig( void)
{
  lpel_config_t *cfg = &_lpel_global_config;
  int proc_avail;

  /* input sanity checks */
  	if ( cfg->num_workers <= 1 ||  cfg->proc_workers <= 1 )
  	  return LPEL_ERR_INVAL;

  if ( cfg->proc_others < 0 ) {
    return LPEL_ERR_INVAL;
  }

  /* check if there are enough processors (if we can check) */
  if (0 == LpelGetNumCores( &proc_avail)) {
    if (cfg->proc_workers + cfg->proc_others > proc_avail) {
      return LPEL_ERR_INVAL;
    }
    /* check exclusive flag sanity */
    if ( LPEL_ICFG( LPEL_FLAG_EXCLUSIVE) ) {
      /* check if we can do a 1-1 mapping */
      if ( (cfg->proc_others== 0) || (cfg->num_workers != cfg->proc_workers) ) {
        return LPEL_ERR_INVAL;
      }
    }
  }

  /* additional flags for exclusive flag */
  if ( LPEL_ICFG( LPEL_FLAG_EXCLUSIVE) ) {
    int can_rt;
    /* pinned flag must also be set */
    if ( !LPEL_ICFG( LPEL_FLAG_PINNED) ) {
      return LPEL_ERR_INVAL;
    }
    /* check permissions to set exclusive (if we can check) */
    if ( 0==LpelCanSetExclusive(&can_rt) && !can_rt ) {
      return LPEL_ERR_EXCL;
    }
  }

  return 0;
}


static void CreateCpusets( void)
{
  #ifdef HAVE_PTHREAD_SETAFFINITY_NP
  lpel_config_t *cfg = &_lpel_global_config;
  int  i;

  /* create the cpu_set for worker threads */
  CPU_ZERO( &cpuset_workers );
  for (i=0; i<cfg->proc_workers; i++) {
    CPU_SET(i, &cpuset_workers);
  }

  /* create the cpu_set for other threads */
  CPU_ZERO( &cpuset_others );
  if (cfg->proc_others == 0) {
    /* distribute on the workers */
    for (i=0; i<cfg->proc_workers; i++) {
      CPU_SET(i, &cpuset_others);
    }
  } else {
    /* set to proc_others */
    for( i=cfg->proc_workers;
        i<cfg->proc_workers+cfg->proc_others;
        i++ ) {
      CPU_SET(i, &cpuset_others);
    }
  }
  #endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}



/**
 * Initialise the LPEL
 *
 *  num_workers, proc_workers > 0
 *  proc_others >= 0
 *
 *
 * EXCLUSIVE: only valid, if
 *       #proc_avail >= proc_workers + proc_others &&
 *       proc_others != 0 &&
 *       num_workers == proc_workers
 *
 */
int LpelInit(lpel_config_t *cfg)
{
  int res;

  /* store a local copy of cfg */
  _lpel_global_config = *cfg;

  /* check the config */
  res = CheckConfig();
  if (res!=0) return res;

  /* create the cpu affinity set for used threads */
  CreateCpusets();

#ifdef USE_MCTX_PCL
  /* initialize machine context for main thread */
  assert( 0 == co_thread_init());
#endif


  /* initialise workers */
  LpelWorkersInit( _lpel_global_config.num_workers);


  return 0;
}


void LpelStart(void)
{
  LpelWorkersSpawn();
}


void LpelStop(void)
{
  LpelWorkersTerminate();
}



/**
 * Cleans the LPEL up
 * - wait for the workers to finish
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  /* Cleanup workers */
  LpelWorkersCleanup();

#ifdef USE_MCTX_PCL
  /* cleanup machine context for main thread */
  co_thread_cleanup();
#endif
}




/**
 * @pre core in [0, num_workers] or -1
 */
int LpelThreadAssign( int core)
{
  #ifdef HAVE_PTHREAD_SETAFFINITY_NP
  lpel_config_t *cfg = &_lpel_global_config;
  pthread_t pt = pthread_self();
  int res;

  if ( LPEL_ICFG(LPEL_FLAG_PINNED)) {
   	CPU_ZERO(&cpuset);
   	switch(core) {
   	case LPEL_MAP_OTHERS:	/* round robin pinned to cores in the set */
   		CPU_SET(rot_others + offset_others, &cpuset);
   		rot_others = (rot_others + 1) % cfg->proc_others;
   		break;

   	default:	// workers
   		/* assign to specified core */
   		assert( 0<=core && core<cfg->num_workers );
   		CPU_SET( core % cfg->proc_workers + offset_workers, &cpuset);
   	}
  }
  else {
  	switch (core) {
  	case LPEL_MAP_OTHERS:
  		cpuset = cpuset_others;
  		break;
  	default: // workers
  		cpuset = cpuset_workers;
  	}
  }

  res = pthread_setaffinity_np(pt, sizeof(cpu_set_t), &cpuset);
  if( res != 0) return LPEL_ERR_ASSIGN;

  /* make non-preemptible for workers only */
  if ( LPEL_ICFG(LPEL_FLAG_EXCLUSIVE) && core != LPEL_MAP_OTHERS) {
  	struct sched_param param;
  	int sp = SCHED_FIFO;
  	/* highest real-time */
  	param.sched_priority = sched_get_priority_max(sp);
  	res = pthread_setschedparam(pt, sp, &param);
  	if ( res != 0) {
  		/* we do best effort at this point */
  		return LPEL_ERR_EXCL;
  	} else {
  		fprintf(stderr, "set realtime priority %d for worker %d.\n",
  				param.sched_priority, core);
  	}
  }

  #endif /* HAVE_PTHREAD_SETAFFINITY_NP */
  return 0;
}




