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
#define _GNU_SOURCE

/* cpuset for others-threads [proc_workers + proc_sosi -> proc_workers + proc_sosi + proc_others - 1], if proc_others = 0 --> map to same of workers*/
static int offset_others;
static int rot_others = 0;
static cpu_set_t cpuset_others;


/*
 * cpuset for source sink [proc_workers -> proc_worker + proc_sosi - 1], if proc_sosi = 0 --> map to same with others
 * only used if FLAG_SOSI is set
 * */
static int offset_sosi;
static int rot_sosi = 0;
static cpu_set_t cpuset_sosi;


/*
 * cpuset for workers = [0,proc_workers-1]
 */
static int offset_workers;
static cpu_set_t cpuset_workers;

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

  /* input sanity checks*/
  if ( cfg->num_workers <= 1 ) {		// worker and master
    return LPEL_ERR_INVAL;
  }
  if ( cfg->proc_others < 0 | cfg->proc_workers < 0 | cfg->proc_sosi < 0 ) {
    return LPEL_ERR_INVAL;
  }

  if (LPEL_ICFG( LPEL_FLAG_EXCLUSIVE) && cfg->num_workers > cfg->proc_workers)
  	return LPEL_ERR_INVAL;

  if (0 == LpelGetNumCores( &proc_avail)) {
  	/* additional flags for exclusive flag */
  	if ( LPEL_ICFG( LPEL_FLAG_EXCLUSIVE) ) {
  		if (! LPEL_ICFG( LPEL_FLAG_PINNED))	/* flag pinned must be set */
  			return LPEL_ERR_INVAL;
  		/* check if we have extra cpus for others */
  		if (cfg->proc_others == 0)
  			return LPEL_ERR_INVAL;

  		if (cfg->proc_others + cfg->proc_workers + cfg->proc_sosi > proc_avail) /* check if enough cpus */
  			return LPEL_ERR_INVAL;

  		/* check permissions to set exclusive (if we can check) */
  		int can_rt;
  		if ( 0==LpelCanSetExclusive(&can_rt) && !can_rt ) {
  			return LPEL_ERR_EXCL;
  		}
  	}
  }

  return 0;
}

static void CreateCpuSets( void)
{
  #ifdef HAVE_PTHREAD_SETAFFINITY_NP
  lpel_config_t *cfg = &_lpel_global_config;
  int i;

  /** worker + masters */
  offset_workers = 0;
  CPU_ZERO(&cpuset_workers);
  for (i = 0; i < cfg->proc_workers; i++)
  	CPU_SET(i, &cpuset_workers);

  /** others */
  if (cfg->proc_others == 0) {
  	offset_others = offset_workers;
  	cpuset_others = cpuset_workers;
  	cfg->proc_others = cfg->proc_workers;
  } else {
  	offset_others = cfg->proc_workers + cfg->proc_sosi;
  	CPU_ZERO(&cpuset_others);
  	for (i = 0; i < cfg->proc_others; i++)
  		CPU_SET(i + offset_others, &cpuset_others);
  }

  /** sosi */
  if (cfg->proc_sosi == 0) {
  	offset_sosi = offset_others;
  	cpuset_sosi = cpuset_others;
  	cfg->proc_sosi = cfg->proc_others;
  } else {
  	offset_sosi = cfg->proc_workers;
  	CPU_ZERO(&cpuset_sosi);
  	for (i = 0; i < cfg->proc_sosi; i++)
  		CPU_SET(i + offset_sosi, &cpuset_sosi);
  }
  #endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}



/**
 * Initialise the LPEL
 *
 *  proc_workers > 0
 *  proc_others >= 0
 *  proc_workers + proc_others + proc_sosi >= proc_avail
 *
 * EXCLUSIVE: only valid, if
 *      proc_others != 0
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
  CreateCpuSets();

#ifdef USE_MCTX_PCL
  /* initialize machine context for main thread */
  assert( 0 == co_thread_init());
#endif


  /* initialise workers */
  LpelMasterInit( _lpel_global_config.num_workers);


  return 0;
}


void LpelStart(void)
{
  LpelMasterSpawn();
}


void LpelStop(void)
{
  LpelMasterTerminate();
}



/**
 * Cleans the LPEL up
 * - wait for the workers to finish
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  /* Cleanup workers */
  LpelMasterCleanup();

#ifdef USE_MCTX_PCL
  /* cleanup machine context for main thread */
  co_thread_cleanup();
#endif
}



/**
 * @pre core in [0, num_workers] or cpu_sosi or cpu_others
 */
int LpelThreadAssign( int core)
{
  #ifdef HAVE_PTHREAD_SETAFFINITY_NP
  lpel_config_t *cfg = &_lpel_global_config;
  pthread_t pt = pthread_self();
  int res;
  cpu_set_t cpuset;
  if ( LPEL_ICFG(LPEL_FLAG_PINNED)) {
  	CPU_ZERO(&cpuset);
  	switch(core) {
  	case LPEL_MAP_SOSI: /* round robin pinned to cores in the set */
  		assert( LPEL_ICFG(LPEL_FLAG_SOSI));
  		CPU_SET(rot_sosi + offset_sosi, &cpuset);
  		rot_sosi = (rot_sosi + 1) % cfg->proc_sosi;
  		break;
  	case LPEL_MAP_OTHERS:	/* round robin pinned to cores in the set */
  		CPU_SET(rot_others + offset_others, &cpuset);
  		rot_others = (rot_others + 1) % cfg->proc_others;
  		break;

  	default:	// master + workers
  		/* assign to specified core */
  		CPU_SET( core % cfg->proc_workers + offset_workers, &cpuset);
  	}
  } else {
  	switch (core) {
  	case LPEL_MAP_SOSI:
  		cpuset = cpuset_sosi;
  		break;
  	case LPEL_MAP_OTHERS:
  		cpuset = cpuset_others;
  		break;
  	default:
  		cpuset = cpuset_workers;
  	}
  }

  res = pthread_setaffinity_np(pt, sizeof(cpu_set_t), &cpuset);
  if( res != 0) return LPEL_ERR_ASSIGN;

  /* make non-preemptible for workers only */
  if ( LPEL_ICFG(LPEL_FLAG_EXCLUSIVE) && core != LPEL_MAP_OTHERS && core != LPEL_MAP_SOSI) {
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





