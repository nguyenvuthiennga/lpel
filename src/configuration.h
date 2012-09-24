#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

/*
 * List all define for different implementations, e.g. buffer, queue...
 *
 */

/** buffer, either bounded or unbounded */
//#define _USE_BOUNDED_BUFFER_
#define _USE_UNBOUNDED_BUFFER_

/** taskqueues: FIFO, PRIORITY_HEAP */
//#define _USE_FIFO_QUEUE_
#define _USE_PRIORITY_QUEUE_

/** dynamic priority */
#ifdef _USE_PRIORITY_QUEUE_
#define _USE_DYNAMIC_PRIORITY_
#endif


//#define _USE_DBG__

#ifdef _USE_DBG__
#define PRT_DBG printf
#else
#define PRT_DBG	//
#endif

#endif
