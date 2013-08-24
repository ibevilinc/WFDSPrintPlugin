/*
(c) Copyright 2013 Hewlett-Packard Development Company, L.P.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include <unistd.h>
#include <errno.h>
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# ifndef __USE_UNIX98
#  define __USE_UNIX98
# endif
#include <pthread.h>

#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ifc_print_job.h"
#include "lib_wprint.h"
#include "wprint_msgq.h"
#include "wprint_debug.h"
#include "plugin_db.h"

/* status interfaces */
#include "ifc_status_monitor.h"

#include "ippstatus_monitor.h"
#include "ippstatus_capabilities.h"
#include "ipp_print.h"

#include "lib_printable_area.h"
#include "wprint_io_plugin.h"

#define USE_PWG_OVER_PCLM 0

#define _DEFAULT_PLUGIN_PATH   "/sdcard"
#if (USE_PWG_OVER_PCLM != 0)
#define _DEFAULT_PRINT_FORMAT  PRINT_FORMAT_PWG
#define _DEFAULT_PCL_TYPE      PCLPWG
#else // (USE_PWG_OVER_PCLM != 0)
#define _DEFAULT_PRINT_FORMAT  PRINT_FORMAT_PCLM
#define _DEFAULT_PCL_TYPE      PCLm
#endif // (USE_PWG_OVER_PCLM != 0)

#define _MAX_SPOOLED_JOBS     100
#define _MAX_MSGS             (_MAX_SPOOLED_JOBS * 5)

#define _MAX_PAGES_PER_JOB   200

#define MAX_IDLE_WAIT        (5 * 60)

#define DEFAULT_RESOLUTION   (300)
#define BEST_MODE_RESOLUTION (600)

#define MAX_DONE_WAIT        (5 * 60)
#define MAX_START_WAIT       (45)

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))

#define IO_PORT_FILE   0

/**  the following macros allow for upto 8 bits (256) for spooled 
 job id#s and 24 bits (16 million) of a running sequence number to
 provide a reasonably unique job handle  **/

/**  _ENCODE_HANDLE() is only called from _get_handle()  **/

#define _ENCODE_HANDLE(X) ( (((++_running_number) & 0xffffff) << 8) |    \
                                                       ((X) & 0xff) )
#define _DECODE_HANDLE(X)   ((X) & 0xff)

typedef enum
{
	JOB_STATE_FREE, // queue element free
	JOB_STATE_QUEUED, // job queued and waiting to be run
	JOB_STATE_RUNNING, // job running (printing)
	JOB_STATE_BLOCKED, // print job blocked due to printer stall/error
	JOB_STATE_CANCEL_REQUEST, // print job cancelled by user,
	JOB_STATE_CANCELLED, // print job cancelled by user, waiting to be freed
	JOB_STATE_COMPLETED, // print job completed successfully, waiting to be freed
	JOB_STATE_ERROR, // job could not be run due to error
	JOB_STATE_CORRUPTED, // job could not be run due to error

	NUM_JOB_STATES

} _job_state_t;

typedef enum
{
	TOP_MARGIN = 0, LEFT_MARGIN, RIGHT_MARGIN, BOTTOM_MARGIN, NUM_PAGE_MARGINS,
} _page_mergins_t;

typedef enum
{
	MSG_RUN_JOB, MSG_QUIT,
} wprint_msg_t;

typedef struct
{
	wprint_msg_t id;
	wJob_t job_id;
} _msg_t;

typedef struct
{
	wJob_t job_handle;
	_job_state_t job_state;
	unsigned int blocked_reasons;
	wprintSyncCb_t cb_fn;
	char *printer_addr;
	port_t port_num;
	wprint_plugin_t *plugin;
	ifc_print_job_t *print_ifc;
	char *mime_type;
	char *pathname;
	bool is_dir;
	bool last_page_seen;
	int num_pages;
	MSG_Q_ID pageQ;
	MSG_Q_ID saveQ;

	wprint_job_params_t job_params;
	bool cancel_ok;

	const ifc_status_monitor_t *status_ifc;
	char debug_path[MAX_PATHNAME_LENGTH + 1];
    int job_debug_fd;
    int page_debug_fd;
} _job_queue_t;

typedef struct
{
	int page_num;
	bool last_page;
	bool corrupted;
	char filename[MAX_PATHNAME_LENGTH + 1];
	unsigned int top_margin;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int bottom_margin;

} _page_t;

typedef struct
{
	port_t port_num;
	const wprint_io_plugin_t *io_plugin;
	void *dl_handle;
} _io_plugin_t;

static _job_queue_t _job_queue[_MAX_SPOOLED_JOBS];
static MSG_Q_ID _msgQ;

static pthread_t _job_status_tid;
static pthread_t _job_tid;

static pthread_t _q_owner;
static pthread_mutex_t _q_lock;
static pthread_mutexattr_t _q_lock_attr;

static sem_t _job_end_wait_sem;
static sem_t _job_start_wait_sem;

static void _debug(int cond, char *fmt, ...);

static _io_plugin_t _io_plugins[2];

static char _libraryID[65] = { 0 };

/*____________________________________________________________________________
 |
 | _get_job_desc() : static function to return job description to the caller
 |___________________________________________________________________________*/

static _job_queue_t *_get_job_desc(wJob_t job_handle)
{
	int index;
	if (job_handle == WPRINT_BAD_JOB_HANDLE)
		return (NULL);
	index = _DECODE_HANDLE(job_handle);
	if ((index < _MAX_SPOOLED_JOBS)
			&& (_job_queue[index].job_handle == job_handle)
			&& (_job_queue[index].job_state != JOB_STATE_FREE))
	{
		return (&_job_queue[index]);
	} else
	{
		return (NULL);
	}

} /*  _get_job_desc  */

static void _stream_dbg_end_job(wJob_t job_handle) {
	_job_queue_t *jq = _get_job_desc(job_handle);
    if (jq && (jq->job_debug_fd >= 0)) {
        close(jq->job_debug_fd);
        jq->job_debug_fd = -1;
    }
}

static void _stream_dbg_start_job(wJob_t job_handle, const char *ext) {
    _stream_dbg_end_job(job_handle);
	_job_queue_t *jq = _get_job_desc(job_handle);
    if (jq && jq->debug_path[0]) {
        char filename[MAX_PATHNAME_LENGTH + 1];
        snprintf(filename, MAX_PATHNAME_LENGTH, "%s/jobstream.%s", jq->debug_path, ext);
        filename[MAX_PATHNAME_LENGTH] = 0;
        jq->job_debug_fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY);
    }
}

static void _stream_dbg_job_data(wJob_t job_handle, const unsigned char *buff, unsigned long nbytes) {
	_job_queue_t *jq = _get_job_desc(job_handle);
//jdelhier
    ssize_t bytes_written;
    if (jq && (jq->job_debug_fd >= 0)) {
        while(nbytes > 0) {
            bytes_written = write(jq->job_debug_fd, buff, nbytes);
            if (bytes_written < 0) {
                return;
            }
            nbytes -= bytes_written;
            buff += bytes_written;
        }
    }
}

static void _stream_dbg_end_page(wJob_t job_handle) {
	_job_queue_t *jq = _get_job_desc(job_handle);
    if (jq && (jq->page_debug_fd >= 0)) {
        close(jq->page_debug_fd);
        jq->page_debug_fd = -1;
    }
}

static void _stream_dbg_page_data(wJob_t job_handle, const unsigned char *buff, unsigned long nbytes) {
	_job_queue_t *jq = _get_job_desc(job_handle);
    ssize_t bytes_written;
    if (jq && (jq->page_debug_fd >= 0)) {
        while(nbytes > 0) {
            bytes_written = write(jq->page_debug_fd, buff, nbytes);
            if (bytes_written < 0) {
                return;
            }
            nbytes -= bytes_written;
            buff += bytes_written;
        }
    }
}

#define PPM_IDENTIFIER "P6\n"
#define PPM_HEADER_LENGTH 128

static void _stream_dbg_start_page(wJob_t job_handle, int page_number, int width, int height) {
    _stream_dbg_end_page(job_handle);
	_job_queue_t *jq = _get_job_desc(job_handle);
    if (jq && jq->debug_path[0]) {
        union {
            char filename[MAX_PATHNAME_LENGTH + 1];
            char ppm_header[PPM_HEADER_LENGTH + 1];
        } buff;
        snprintf(buff.filename, MAX_PATHNAME_LENGTH, "%s/page%4.4d.ppm", jq->debug_path, page_number);
        buff.filename[MAX_PATHNAME_LENGTH] = 0;
        jq->page_debug_fd = open(buff.filename, O_CREAT | O_TRUNC | O_WRONLY);
        int length = snprintf(buff.ppm_header, sizeof(buff.ppm_header), "%s\n#%*c\n%d %d\n%d\n",
                              PPM_IDENTIFIER,
                              0,
                              ' ',
                              width,
                              height,
                              255);
        int padding = sizeof(buff.ppm_header) - length;
        length = snprintf(buff.ppm_header, sizeof(buff.ppm_header), "%s\n#%*c\n%d %d\n%d\n",
                          PPM_IDENTIFIER,
                          padding,
                          ' ',
                          width,
                          height,
                          255);
        _stream_dbg_page_data(job_handle, buff.ppm_header, PPM_HEADER_LENGTH);
    }
}

static const ifc_wprint_debug_stream_t _debug_stream_ifc =
{
    .debug_start_job  = _stream_dbg_start_job,
    .debug_job_data   = _stream_dbg_job_data,
    .debug_end_job    = _stream_dbg_end_job,
    .debug_start_page = _stream_dbg_start_page,
    .debug_page_data  = _stream_dbg_page_data,
    .debug_end_page   = _stream_dbg_end_page,
};

static const ifc_osimg_t* _osimg_ifc = NULL;
const ifc_osimg_t* getOSIFC() {
    return _osimg_ifc;
}

void wprintSetOSImgIFC(const ifc_osimg_t *osimg_ifc)
{
    _osimg_ifc = osimg_ifc;
}

const ifc_wprint_debug_stream_t* getDebugStreamIFC(wJob_t handle) {
	_job_queue_t *jq = _get_job_desc(handle);
    if (jq) {
        return (jq->debug_path[0] == 0) ? NULL : &_debug_stream_ifc;
    }
    return NULL;
}

const ifc_wprint_t _wprint_ifc =
{
    .debug                = _debug,
    .msgQCreate           = msgQCreate,
    .msgQDelete           = msgQDelete,
    .msgQSend             = msgQSend,
    .msgQReceive          = msgQReceive,
    .msgQNumMsgs          = msgQNumMsgs,
    .get_osimg_ifc        = getOSIFC, 
    .get_debug_stream_ifc = getDebugStreamIFC,
};

#define DEBUG_MSG_SIZE (2048)
static void _debug(int cond, char *fmt, ...)
{
	va_list args;
	char *buff = malloc(DEBUG_MSG_SIZE);
	if (buff != NULL)
	{
		va_start(args, fmt);
		vsnprintf(buff, DEBUG_MSG_SIZE, fmt, args);
		va_end(args);
		ifprint((cond, buff));
		free(buff);
	}
}

const ifc_wprint_t* wprint_get_ifc(void)
{
	return (&_wprint_ifc);
}

/**   DEFAULT OVERRIDES  **/

static char *_plugin_path = _DEFAULT_PLUGIN_PATH;

void wprint_override_plugin_path(char *debug_plugin_path)
{
	ifprint(
			(DBG_FORCE, "plugin path switched from %s to %s", _plugin_path, debug_plugin_path));

	_plugin_path = debug_plugin_path;

} /*  wprint_override_plugin_path  */

static char *_default_print_format = _DEFAULT_PRINT_FORMAT;

void wprint_override_print_format(char *print_format)
{
	if (strcmp(_default_print_format, print_format) != 0)
	{
		ifprint(
				(DBG_FORCE, "print format switched from %s to %s", _default_print_format, print_format));

		_default_print_format = print_format;
	}

} /*  wprint_override_print_format  */

static pcl_t _default_pcl_type = _DEFAULT_PCL_TYPE;

void wprint_override_pcl_type(int pcl_type)
{
	if (pcl_type != (int) _default_pcl_type)
	{
		ifprint(
				(DBG_FORCE, "pcl type switched from %d to %d", _default_pcl_type, pcl_type));

		_default_pcl_type = (pcl_t) pcl_type;
	}

} /*  wprint_override_pcl_type  */

static bool _ipp_detour_on = true;

#define  _REMAP_PORT(port_num)  \
      port_num = port_num;

static const ifc_print_job_t* _printer_file_connect(
		const ifc_wprint_t *wprint_ifc)
{
	return printer_connect(IO_PORT_FILE, wprint_ifc);
}


static const ifc_printer_capabilities_t* _get_caps_ifc(port_t port_num)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(_io_plugins); i++)
	{
		if (_io_plugins[i].port_num == port_num)
		{
			if (_io_plugins[i].io_plugin == NULL)
				return (NULL);
			if (_io_plugins[i].io_plugin->getCapsIFC == NULL)
				return (NULL);
			else
				return (_io_plugins[i].io_plugin->getCapsIFC(&_wprint_ifc));
		}
	}
	return (NULL);
}

static const ifc_status_monitor_t* _get_status_ifc(port_t port_num)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(_io_plugins); i++)
	{
		if (_io_plugins[i].port_num == port_num)
		{
			if (_io_plugins[i].io_plugin == NULL)
				return (NULL);
			if (_io_plugins[i].io_plugin->getStatusIFC == NULL)
				return (NULL);
			else
				return (_io_plugins[i].io_plugin->getStatusIFC(&_wprint_ifc));
		}
	}
	return (NULL);
}

static const ifc_print_job_t* _get_print_ifc(port_t port_num)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(_io_plugins); i++)
	{
		if (_io_plugins[i].port_num == port_num)
		{
			if (_io_plugins[i].io_plugin == NULL)
				return (NULL);
			if (_io_plugins[i].io_plugin->getPrintIFC == NULL)
				return (NULL);
			else
				return (_io_plugins[i].io_plugin->getPrintIFC(&_wprint_ifc));
		}
	}
	return (NULL);
}

/******  semaphore lock  ********/

static void _lock(void)
{
	pthread_mutex_lock(&_q_lock);
	_q_owner = pthread_self();

} /* _lock  */

static void _unlock(void)
{
	_q_owner = 0;
	pthread_mutex_unlock(&_q_lock);

} /*  _unlock  */

/*____________________________________________________________________________
 |
 | _get_handle() : static function to get the next available queue handle 
 |                 for a new print job 
 |___________________________________________________________________________*/

static wJob_t _get_handle(void)
{
	static unsigned long _running_number = 0;
	wJob_t job_handle = WPRINT_BAD_JOB_HANDLE;
	int i, index, size;
	char *ptr;

	for (i = 0; i < _MAX_SPOOLED_JOBS; i++)
	{
		index = (i + _running_number) % _MAX_SPOOLED_JOBS;

		if (_job_queue[index].job_state == JOB_STATE_FREE)
		{
			size = MAX_MIME_LENGTH + MAX_PRINTER_ADDR_LENGTH
					+ MAX_PATHNAME_LENGTH + 4;
			ptr = malloc(size);
			if (ptr)
			{
				memset(&_job_queue[index], 0, sizeof(_job_queue_t));
				memset(ptr, 0, size);

				_job_queue[index].job_debug_fd = -1;
				_job_queue[index].page_debug_fd = -1;

				_job_queue[index].printer_addr = ptr;
				ptr += (MAX_PRINTER_ADDR_LENGTH + 1);

				_job_queue[index].mime_type = ptr;
				ptr += (MAX_MIME_LENGTH + 1);

				_job_queue[index].pathname = ptr;

				_job_queue[index].job_state = JOB_STATE_QUEUED;
				_job_queue[index].job_handle = _ENCODE_HANDLE(index);
				job_handle = _job_queue[index].job_handle;
			}
			break;
		}
	}

	return (job_handle);

} /*  _get_handle  */

/*____________________________________________________________________________
 |
 | _recycle_handle() : static function to recycle the specified queue handle 
 |                     of a completed or cancelled print job
 |___________________________________________________________________________*/

static int _recycle_handle(wJob_t job_handle)
{
	_job_queue_t *jq = _get_job_desc(job_handle);

	if (jq == NULL)
		return (ERROR);
	else if ((jq->job_state == JOB_STATE_CANCELLED)
			|| (jq->job_state == JOB_STATE_ERROR)
			|| (jq->job_state == JOB_STATE_CORRUPTED)
			|| (jq->job_state == JOB_STATE_COMPLETED))
	{
		if (jq->print_ifc != NULL)
			jq->print_ifc->destroy(jq->print_ifc);
		jq->print_ifc = NULL;
		if (jq->status_ifc != NULL)
			jq->status_ifc->destroy(jq->status_ifc);
		jq->status_ifc = NULL;
		if (jq->job_params.useragent != NULL)
		    free((void*)jq->job_params.useragent);
		free(jq->printer_addr);
		jq->job_state = JOB_STATE_FREE;

        if (jq->job_debug_fd != -1) {
            close(jq->job_debug_fd);
        }
        jq->job_debug_fd = -1;
        if (jq->page_debug_fd != -1) {
            close(jq->page_debug_fd);
        }
        jq->page_debug_fd = -1;

		jq->debug_path[0] = 0;
		return (OK);
	} else
	{
		return (ERROR);
	}
} /*  _recycle_handle  */

static void _job_status_callback(const printer_state_dyn_t *new_status,
		const printer_state_dyn_t *old_status, void *param)
{
	wprint_job_callback_params_t cb_param;
	_job_queue_t *jq = (_job_queue_t*) param;
	unsigned int i, blocked_reasons;
	print_status_t statusnew, statusold;

	statusnew = new_status->printer_status & ~PRINTER_IDLE_BIT;
	statusold = old_status->printer_status & ~PRINTER_IDLE_BIT;

	ifprint(
			(DBG_VERBOSE, "_job_status_callback(): current printer state: %d", statusnew));
	blocked_reasons = 0;
	for (i = 0; i <= PRINT_STATUS_MAX_STATE; i++)
	{
		if (new_status->printer_reasons[i] == PRINT_STATUS_MAX_STATE)
			break;
		ifprint(
				(DBG_VERBOSE, "_job_status_callback(): blocking reason %d: %d", i, new_status->printer_reasons[i]));
		blocked_reasons |= (1 << new_status->printer_reasons[i]);
	}

	switch (statusnew)
	{
	case PRINT_STATUS_UNKNOWN:
		if ((new_status->printer_reasons[0] == PRINT_STATUS_OFFLINE)
				|| (new_status->printer_reasons[0] == PRINT_STATUS_UNKNOWN))
		{
			sem_post(&_job_start_wait_sem);
			sem_post(&_job_end_wait_sem);
			_lock();
			if ((new_status->printer_reasons[0] == PRINT_STATUS_OFFLINE)
					&& ((jq->print_ifc != NULL)
							&& (jq->print_ifc->enable_timeout != NULL)))
				jq->print_ifc->enable_timeout(jq->print_ifc, 1);
			_unlock();
		}
		break;

		/* device is idle */
	case PRINT_STATUS_IDLE:
		/* clear errors */
		/* check if the job is finished */
		if ((statusold > PRINT_STATUS_IDLE)
				|| (statusold == PRINT_STATUS_CANCELLED))
		{
			/* somehow the print is over the job wasn't ended correctly */
			if (jq->is_dir && !jq->last_page_seen)
				wprintPage(jq->job_handle, jq->num_pages + 1, NULL, true, 0, 0,
						0, 0);
			sem_post(&_job_end_wait_sem);
		}
		break;

		/* all is well */
	case PRINT_STATUS_CANCELLED:
		sem_post(&_job_start_wait_sem);
		if ((jq->print_ifc != NULL) && (jq->print_ifc->enable_timeout != NULL))
			jq->print_ifc->enable_timeout(jq->print_ifc, 1);
		if (statusold != PRINT_STATUS_CANCELLED)
		{
			ifprint((DBG_LOG, "status requested job cancel"));
			if (new_status->printer_reasons[0] == PRINT_STATUS_OFFLINE)
			{
				sem_post(&_job_start_wait_sem);
				sem_post(&_job_end_wait_sem);
				if ((jq->print_ifc != NULL)
						&& (jq->print_ifc->enable_timeout != NULL))
					jq->print_ifc->enable_timeout(jq->print_ifc, 1);
			}
			_lock();
			jq->job_params.cancelled = true;
			_unlock();
		}
		if (new_status->printer_reasons[0] == PRINT_STATUS_OFFLINE)
		{
			sem_post(&_job_start_wait_sem);
			sem_post(&_job_end_wait_sem);
		}
		break;

		/* all is well */
	case PRINT_STATUS_PRINTING:
		sem_post(&_job_start_wait_sem);
		/* clear errors */
		_lock();
		if ((jq->job_state != JOB_STATE_RUNNING)
				|| (jq->blocked_reasons != blocked_reasons))
		{
			jq->job_state = JOB_STATE_RUNNING;
			jq->blocked_reasons = blocked_reasons;
			if (jq->cb_fn)
			{
				cb_param.id = WPRINT_CB_PARAM_JOB_STATE;
				cb_param.param.state = JOB_RUNNING;
				cb_param.blocked_reasons = jq->blocked_reasons;
				cb_param.job_done_result = OK;

				jq->cb_fn(jq->job_handle, (void *) &cb_param);
			}
		}
		_unlock();
		break;

		/* an error has occurred, report it back to the client */
	default:
		sem_post(&_job_start_wait_sem);
		_lock();

		if ((jq->job_state != JOB_STATE_BLOCKED)
				|| (jq->blocked_reasons != blocked_reasons))
		{
			jq->job_state = JOB_STATE_BLOCKED;
			jq->blocked_reasons = blocked_reasons;
			if (jq->cb_fn)
			{
				cb_param.id = WPRINT_CB_PARAM_JOB_STATE;
				cb_param.param.state = JOB_BLOCKED;
				cb_param.blocked_reasons = blocked_reasons;
				cb_param.job_done_result = OK;

				jq->cb_fn(jq->job_handle, (void *) &cb_param);
			}
		}
		_unlock();
		break;
	}
}

static void *_job_status_thread(void *param)
{
	_job_queue_t *jq = (_job_queue_t*) param;
	(jq->status_ifc->start)(jq->status_ifc, _job_status_callback, param);
	return (NULL);
}

static int _start_status_thread(_job_queue_t *jq)
{
	sigset_t allsig, oldsig;
	int result = ERROR;

	if ((jq == NULL) || (jq->status_ifc == NULL))
		return (result);

	result = OK;
	sigfillset(&allsig);
#if CHECK_PTHREAD_SIGMASK_STATUS
	result = pthread_sigmask(SIG_SETMASK, &allsig, &oldsig);
#else /* else CHECK_PTHREAD_SIGMASK_STATUS */
	pthread_sigmask(SIG_SETMASK, &allsig, &oldsig);
#endif /* CHECK_PTHREAD_SIGMASK_STATUS */
	if (result == OK)
	{
		result = pthread_create(&_job_status_tid, 0, _job_status_thread, jq);
		if ((result == ERROR) && (_job_status_tid != pthread_self()))
		{
#if USE_PTHREAD_CANCEL
			pthread_cancel(_job_status_tid);
#else /* else USE_PTHREAD_CANCEL */
			pthread_kill(_job_status_tid, SIGKILL);
#endif /* USE_PTHREAD_CANCEL */
			_job_status_tid = pthread_self();
		}
	}

	if (result == OK)
	{
		sched_yield();
#if CHECK_PTHREAD_SIGMASK_STATUS
		result = pthread_sigmask(SIG_SETMASK, &oldsig, 0);
#else /* else CHECK_PTHREAD_SIGMASK_STATUS */
		pthread_sigmask(SIG_SETMASK, &oldsig, 0);
#endif /* CHECK_PTHREAD_SIGMASK_STATUS */
	}

	return (result);
} /*  _start_status_thread  */

static int _stop_status_thread(_job_queue_t *jq)
{
	if (!pthread_equal(_job_status_tid, pthread_self())
			&& (jq && jq->status_ifc))
	{
		(jq->status_ifc->stop)(jq->status_ifc);
		_unlock();
		pthread_join(_job_status_tid, 0);
		_lock();
		_job_status_tid = pthread_self();
		return OK;
	} else
	{
		return ERROR;
	}
} /*  _stop_status_thread  */

static void *_job_thread(void *param)
{
	wprint_job_callback_params_t cb_param;
	_msg_t msg;
	wJob_t job_handle;
	_job_queue_t *jq;
	_page_t page;
	int i, job_result;
	int psock = ERROR;
	int corrupted = 0;

	while (OK == msgQReceive(_msgQ, (char *) &msg, sizeof(msg), WAIT_FOREVER))
	{
		if (msg.id == MSG_RUN_JOB)
		{
			ifprint( (DBG_LOG, "_job_thread(): Received message: MSG_RUN_JOB"));
		} else
		{
			ifprint( (DBG_LOG, "_job_thread(): Received message: MSG_QUIT"));
		}

		if (msg.id == MSG_QUIT)
			break;

		job_handle = msg.job_id;

		//  check if this is a valid job_handle that is still active

		_lock();

		jq = _get_job_desc(job_handle);

		//  set state to running and invoke the plugin, there is one

		if (jq)
		{
			if (jq->job_state != JOB_STATE_QUEUED)
			{
				_unlock();
				continue;
			}
			corrupted = 0;
			job_result = OK;
			jq->job_params.plugin_data = NULL;

			/* clear out the semaphore just in case */
			while (sem_trywait(&_job_start_wait_sem) == OK)
			{
			}
			while (sem_trywait(&_job_end_wait_sem) == OK)
			{
			}

			/* initialize the status ifc */
			if (jq->status_ifc != NULL)
				jq->status_ifc->init(jq->status_ifc, jq->printer_addr);

			/* wait for the printer to be idle */
			if ((jq->status_ifc != NULL)
					&& (jq->status_ifc->get_status != NULL))
			{
				int retry = 0;
				int loop = 1;
				printer_state_dyn_t printer_state;
				do
				{
					print_status_t status;
					jq->status_ifc->get_status(jq->status_ifc, &printer_state);
					status = printer_state.printer_status & ~PRINTER_IDLE_BIT;
					switch (status)
					{
					case PRINT_STATUS_IDLE:
						printer_state.printer_status = PRINT_STATUS_IDLE;
						jq->blocked_reasons = 0;
						loop = 0;
						break;
					case PRINT_STATUS_UNKNOWN:
						if (printer_state.printer_reasons[0]
								== PRINT_STATUS_UNKNOWN)
						{
							ifprint(
									(DBG_ERROR, "PRINTER STATUS UNKNOWN - Ln 747 libwprint.c"));
							/* no status available, break out and hope for the best */
							printer_state.printer_status = PRINT_STATUS_IDLE;
							loop = 0;
							break;
						}
					case PRINT_STATUS_SVC_REQUEST:
						if ((printer_state.printer_reasons[0]
								== PRINT_STATUS_UNABLE_TO_CONNECT)
								|| (printer_state.printer_reasons[0]
										== PRINT_STATUS_OFFLINE))
						{
							ifprint(
									(DBG_LOG, "_job_thread: Received an Unable to Connect message"));
							jq->blocked_reasons =
									BLOCKED_REASON_UNABLE_TO_CONNECT;
							loop = 0;
							break;
						}
					default:
						if (printer_state.printer_status & PRINTER_IDLE_BIT)
						{
							ifprint(
									(DBG_LOG, "printer blocked but appears to be in an idle state. Allowing job to proceed"));
							printer_state.printer_status = PRINT_STATUS_IDLE;
							loop = 0;
							break;
						} else if (retry >= MAX_IDLE_WAIT)
						{
							jq->blocked_reasons |= BLOCKED_REASONS_PRINTER_BUSY;
							loop = 0;
						} else if (!jq->job_params.cancelled)
						{
							int blocked_reasons = 0;
							for (i = 0; i <= PRINT_STATUS_MAX_STATE; i++)
							{
								if (printer_state.printer_reasons[i]
										== PRINT_STATUS_MAX_STATE)
									break;
								blocked_reasons |= (1
										<< printer_state.printer_reasons[i]);
							}
							if (blocked_reasons == 0)
								blocked_reasons |= BLOCKED_REASONS_PRINTER_BUSY;

							if ((jq->job_state != JOB_STATE_BLOCKED)
									|| (jq->blocked_reasons != blocked_reasons))
							{
								jq->job_state = JOB_STATE_BLOCKED;
								jq->blocked_reasons = blocked_reasons;
								if (jq->cb_fn)
								{
									cb_param.id = WPRINT_CB_PARAM_JOB_STATE;
									cb_param.param.state = JOB_BLOCKED;
									cb_param.blocked_reasons = blocked_reasons;
									cb_param.job_done_result = OK;

									jq->cb_fn(jq->job_handle,
											(void *) &cb_param);
								}
							}
							_unlock();
							sleep(1);
							_lock();
							retry++;
						}
						break;
					}
					if (jq->job_params.cancelled)
						loop = 0;
				} while (loop);

				if (jq->job_params.cancelled)
					job_result = CANCELLED;
				else
					job_result = (
							((printer_state.printer_status & ~PRINTER_IDLE_BIT)
									== PRINT_STATUS_IDLE) ? OK : ERROR);
			}

			_job_status_tid = pthread_self();
			if (job_result == OK)
			{
				if (jq->print_ifc)
				{
					psock = jq->print_ifc->init(jq->print_ifc,
							jq->printer_addr);
					job_result = ((psock == ERROR) ? ERROR : OK);
					if (job_result == ERROR)
						jq->blocked_reasons = BLOCKED_REASON_UNABLE_TO_CONNECT;
				}
			}
			if (job_result == OK)
				_start_status_thread(jq);

			/*  call the plugin's start_job method, if no other job is running
			 use callback to notify the client */

			if ((job_result == OK) && jq->cb_fn)
			{
				cb_param.id = WPRINT_CB_PARAM_JOB_STATE;
				cb_param.param.state = JOB_RUNNING;
				cb_param.blocked_reasons = 0;
				cb_param.job_done_result = OK;

				jq->cb_fn(job_handle, (void *) &cb_param);
			}

			jq->job_params.page_num = -1;
			if (job_result == OK)
			{
				if (jq->print_ifc != NULL)
				{
					ifprint((DBG_VERBOSE, "_job_thread: Calling validate_job"));
					if (jq->print_ifc->validate_job != NULL)
						jq->print_ifc->validate_job(jq->print_ifc,
								&jq->job_params);

					if (jq->print_ifc->start_job != NULL)
						jq->print_ifc->start_job(jq->print_ifc,
								&jq->job_params);
				}

				if (jq->plugin->start_job != NULL)
					job_result = jq->plugin->start_job(job_handle,
							(void *) &_wprint_ifc, (void *) jq->print_ifc,
							&(jq->job_params));

			}

			if (job_result == OK)
				jq->job_params.page_num = 0;

			/*  multi-page print job */
			if (jq->is_dir && (job_result == OK))
			{
				int per_copy_page_num;
				for (i = 0;
						i < jq->job_params.num_copies
								&& ((job_result == OK)
										|| (job_result == CORRUPT))
								&& !jq->job_params.cancelled; i++)
				{
					per_copy_page_num = 0;
					jq->job_state = JOB_STATE_RUNNING;

					/* while there is a page to print */

					_unlock();

					while (OK
							== msgQReceive(jq->pageQ, (char *) &page,
									sizeof(page), WAIT_FOREVER))
					{
						_lock();

						/* check for any printing problems so far */
						if ((jq->print_ifc != NULL)
								&& (jq->print_ifc->check_status))
						{
							if (jq->print_ifc->check_status(
									jq->print_ifc) == ERROR)
							{
								job_result = ERROR;
								break;
							}
						}

						/*  take empty filename as cue to break out of the loop */
						/*  but we have to do last_page processing  */

						/* all copies are clubbed together as a single print job */
						if (page.last_page
								&& i == jq->job_params.num_copies - 1)
							jq->job_params.last_page = page.last_page;
						else
							jq->job_params.last_page = false;

						if (strlen(page.filename) > 0)
						{
							per_copy_page_num++;

							{
								jq->job_params.page_num++;
							}

							/* setup page margin information */
							jq->job_params.print_top_margin    += page.top_margin;
							jq->job_params.print_left_margin   += page.left_margin;
							jq->job_params.print_right_margin  += page.right_margin;
							jq->job_params.print_bottom_margin += page.bottom_margin;

							jq->job_params.copy_num = (i + 1);
							jq->job_params.copy_page_num = page.page_num;
                            jq->job_params.page_backside = (per_copy_page_num & 0x1);
							jq->job_params.page_corrupted = (
									page.corrupted ? 1 : 0);

							if (jq->cb_fn)
							{
								cb_param.id = WPRINT_CB_PARAM_PAGE_START_INFO;
								cb_param.param.page_info.page_num =
										jq->job_params.copy_page_num;
								cb_param.param.page_info.copy_num =
										jq->job_params.copy_num;
								cb_param.param.page_info.corrupted_file =
										jq->job_params.page_corrupted;
								cb_param.param.page_info.current_page =
										jq->job_params.page_num;
								if (jq->last_page_seen)
									cb_param.param.page_info.total_pages =
											(jq->num_pages
													* jq->job_params.num_copies);
								else
									cb_param.param.page_info.total_pages = -1;
								cb_param.param.page_info.page_total_update = 0;
								jq->cb_fn(job_handle, (void *) &cb_param);
							}

							jq->job_params.page_printing = true;

							_unlock();

							if (!page.corrupted)
							{
								ifprint(
										(DBG_LOG, "_job_thread(): page not corrupt, calling plugin's print_page function for page #%d", page.page_num));
								job_result = jq->plugin->print_page(job_handle,
										&(jq->job_params), jq->mime_type,
										page.filename);
							} else
							{
								ifprint(
										(DBG_LOG, "_job_thread(): page IS corrupt, printing blank page for page #%d", page.page_num));
								job_result = CORRUPT;
								if ((jq->job_params.duplex
										!= DF_DUPLEX_MODE_NONE)
										&& (jq->plugin->print_blank_page != NULL))
									jq->plugin->print_blank_page(job_handle,
											&(jq->job_params));
							}

							_lock();

							jq->job_params.print_top_margin    -= page.top_margin;
							jq->job_params.print_left_margin   -= page.left_margin;
							jq->job_params.print_right_margin  -= page.right_margin;
							jq->job_params.print_bottom_margin -= page.bottom_margin;

							jq->job_params.page_printing = false;

							/* make sure we only count corrupted pages once */
							if (page.corrupted == false)
							{
								page.corrupted = (
										(job_result == CORRUPT) ? true : false);
								corrupted += (job_result == CORRUPT);
							}

							/*  generate a page level callback  */
							if (jq->cb_fn)
							{
								cb_param.id = WPRINT_CB_PARAM_PAGE_INFO;
								cb_param.param.page_info.page_num =
										jq->job_params.copy_page_num;
								cb_param.param.page_info.copy_num =
										jq->job_params.copy_num;
								cb_param.param.page_info.corrupted_file = (
										page.corrupted ? 1 : 0);
								cb_param.param.page_info.current_page =
										jq->job_params.page_num;
								if (jq->last_page_seen)
									cb_param.param.page_info.total_pages =
											(jq->num_pages
													* jq->job_params.num_copies);
								else
									cb_param.param.page_info.total_pages = -1;

								jq->cb_fn(job_handle, (void *) &cb_param);
							}
						}

						/* make sure we always print an even number of pages in duplex jobs */
						if (page.last_page
								&& (jq->job_params.duplex != DF_DUPLEX_MODE_NONE)
								&& (jq->job_params.page_backside)
								&& (jq->plugin->print_blank_page != NULL))
						{
							_unlock();
							jq->plugin->print_blank_page(job_handle,
									&(jq->job_params));
							_lock();
						}

						/*  if multiple copies are requested, save the contents of
						 *  the pageQ message
						 */

						if (jq->saveQ && !jq->job_params.cancelled
								&& (job_result != ERROR))
						{

							job_result = msgQSend(jq->saveQ, (char *) &page,
									sizeof(page), NO_WAIT, MSG_Q_FIFO);

							/*  swap pageQ and saveQ  */
							if (page.last_page && !jq->job_params.last_page)
							{
								MSG_Q_ID tmpQ = jq->pageQ;

								jq->pageQ = jq->saveQ;
								jq->saveQ = tmpQ;

								/*  defensive programming  */
								while (msgQNumMsgs(tmpQ) > 0)
								{
									msgQReceive(tmpQ, (char *) &page,
											sizeof(page), NO_WAIT);
									ifprint(
											(DBG_ERROR, "Warning: pageQ inconsistencies, discarding page # %d, file %s", page.page_num, page.filename));
								}

							} /* last_page processing */

						} /*  save page info for next copy  */

						if (page.last_page || jq->job_params.cancelled)
						{

							break; /*  leave the semaphore locked */
						}

						/*  unlock to go back to the top of the while loop */
						_unlock();

					} /*  while there is another page  */

				} /*  for each copy of the job */

			} else if (job_result == OK) /*  single page job  */
			{
				for (i = 0; i < jq->job_params.num_copies && job_result == OK;
						i++)
				{
					if (i > 0
							&& (strcmp(jq->job_params.print_format,
									PRINT_FORMAT_PDF) == 0))
					{
						// if we are printing a single page pdf multiple times, the copies are handled by the device.
						// because the print pages are controlled here, we must short circuit based on the print format
						// so that we do not send multiple copies of the job in addition to the copies setting.
						ifprint(
								(DBG_VERBOSE, "lib_wprint.c: _job_thread single_page: breaking out of single page pdf job"));
						break;
					}
					/* check for any printing problems so far */
					if ((jq->print_ifc != NULL)
							&& (jq->print_ifc->check_status))
					{
						if (jq->print_ifc->check_status(jq->print_ifc) == ERROR)
						{
							job_result = ERROR;
							break;
						}
					}

					jq->job_state = JOB_STATE_RUNNING;
					jq->job_params.page_num++;
					jq->job_params.last_page = (i
							== (jq->job_params.num_copies - 1));

					jq->job_params.copy_num = (i + 1);
					jq->job_params.copy_page_num = 1;
					jq->job_params.page_corrupted = (job_result == CORRUPT);

					if (jq->cb_fn)
					{
						cb_param.id = WPRINT_CB_PARAM_PAGE_START_INFO;
						cb_param.param.page_info.page_num =
								jq->job_params.copy_page_num;
						cb_param.param.page_info.copy_num =
								jq->job_params.copy_num;
						cb_param.param.page_info.corrupted_file =
								jq->job_params.page_corrupted;
						cb_param.param.page_info.current_page =
								jq->job_params.page_num;
						cb_param.param.page_info.total_pages =
								jq->job_params.num_copies;
						cb_param.param.page_info.page_total_update = 0;

						jq->cb_fn(job_handle, (void *) &cb_param);
					}
					jq->job_params.page_printing = true;

					_unlock();
					job_result = jq->plugin->print_page(job_handle,
							&(jq->job_params), jq->mime_type, jq->pathname);

					if ((jq->job_params.duplex != DF_DUPLEX_MODE_NONE)
							&& (jq->plugin->print_blank_page != NULL))
					{
						jq->plugin->print_blank_page(job_handle,
								&(jq->job_params));
					}

					_lock();
					jq->job_params.page_printing = false;

					corrupted += (job_result == CORRUPT);

					/*  generate a page level callback  */
					if (jq->cb_fn)
					{
						cb_param.id = WPRINT_CB_PARAM_PAGE_INFO;
						cb_param.param.page_info.page_num = 1;
						cb_param.param.page_info.copy_num = i + 1;
						cb_param.param.page_info.corrupted_file = (job_result
								== CORRUPT);
						cb_param.param.page_info.current_page =
								jq->job_params.page_num;
						cb_param.param.page_info.total_pages =
								jq->job_params.num_copies;

						jq->cb_fn(job_handle, (void *) &cb_param);
					}

				} /*  for each copy  */

			}

			/* if we started the job end it */
			if (jq->job_params.page_num >= 0)
			{
				/* if the job was cancelled without sending anything through, print a blank sheet */
				if ((jq->job_params.page_num == 0)
						&& (jq->plugin->print_blank_page != NULL))
					jq->plugin->print_blank_page(job_handle, &(jq->job_params));
				if (jq->plugin->end_job != NULL)
					jq->plugin->end_job(job_handle, &(jq->job_params));
				if ((jq->print_ifc != NULL) && (jq->print_ifc->end_job))
				{
					if (jq->print_ifc->end_job(jq->print_ifc) == ERROR)
					{
						if (job_result == OK)
							job_result = ERROR;
					}
				}
			}

			/* if we started to print, wait for idle */
			if ((jq->job_params.page_num > 0) && (jq->status_ifc != NULL))
			{
				int retry, result;
				_unlock();

				for (retry = 0, result = ERROR;
						((result == ERROR) && (retry <= MAX_START_WAIT));
						retry++)
				{
					if (retry != 0)
						sleep(1);
					// coverity[lock] : used to see if we get idle, don't care about unlocking
					result = sem_trywait(&_job_start_wait_sem);
				}

				if (result == OK)
				{
					for (retry = 0, result = ERROR;
							((result == ERROR) && (retry <= MAX_DONE_WAIT));
							retry++)
					{
						if (retry != 0)
						{
							_lock();
							if (jq->job_params.cancelled && !jq->cancel_ok)
							{
								/*
								 * the user tried to cancel and it either didn't go through
								 * or the printer doesn't support cancel through an OID.
								 * Either way it's pointless to sit here waiting for idle when
								 * may never come, so we'll bail out early
								 */
								retry = (MAX_DONE_WAIT + 1);
							}
							_unlock();
							sleep(1);
							if (retry == MAX_DONE_WAIT)
							{
								_lock();
								if (!jq->job_params.cancelled
										&& (jq->blocked_reasons
												& (BLOCKED_REASON_OUT_OF_PAPER
														| BLOCKED_REASON_JAMMED
														| BLOCKED_REASON_DOOR_OPEN)))
								{
									retry = (MAX_DONE_WAIT - 1);
								}
								_unlock();
							}
						}
						result = sem_trywait(&_job_end_wait_sem);
					}
				} else
				{
					ifprint(
							(DBG_LOG, "_job_thread(): the job doesn't appear to have ever started"));
				}
				_lock();
			}

			/* make sure page_num doesn't stay as a negative number */
			jq->job_params.page_num = MAX(0, jq->job_params.page_num);

			_stop_status_thread(jq);

			if (corrupted != 0)
				job_result = CORRUPT;

			switch (job_result)
			{
			case OK:

				if (!jq->job_params.cancelled)
				{
					jq->job_state = JOB_STATE_COMPLETED;
					jq->blocked_reasons = 0;
					break;
				} else
					job_result = CANCELLED;
				// coverity[fallthrough]

			case CANCELLED:

				jq->job_state = JOB_STATE_CANCELLED;
				jq->blocked_reasons = BLOCKED_REASONS_CANCELLED;
				if (!jq->cancel_ok)
					jq->blocked_reasons |=
							BLOCKED_REASON_KINDA_CANCELLED_BUT_NOT_REALLY;
				break;

			case CORRUPT:
				ifprint(
						(DBG_ERROR, "_job_thread(): ERROR: %d file(s) in the job were corrupted", corrupted));
				jq->job_state = JOB_STATE_CORRUPTED;
				jq->blocked_reasons = 0;
				break;

			case ERROR:
			default:
				ifprint(
						(DBG_ERROR, "_job_thread(): ERROR plugin->start_job(0x%x) : %s => %s", job_handle, jq->mime_type, jq->job_params.print_format));

				job_result = ERROR;
				jq->job_state = JOB_STATE_ERROR;
				break;

			} /*  job_result  */

			/*  end of job callback  */
			if (jq->cb_fn)
			{
				cb_param.id = WPRINT_CB_PARAM_JOB_STATE;
				cb_param.param.state = JOB_DONE;
				cb_param.blocked_reasons = jq->blocked_reasons;
				cb_param.job_done_result = job_result;

				jq->cb_fn(job_handle, (void *) &cb_param);
			}

			if (jq->print_ifc != NULL)
			{
				jq->print_ifc->destroy(jq->print_ifc);
				jq->print_ifc = NULL;
			}

			if (jq->status_ifc != NULL)
			{
				jq->status_ifc->destroy(jq->status_ifc);
				jq->status_ifc = NULL;
			}
		} else
		{
			ifprint(
					(DBG_SESSION, "_job_thread(): job 0x%x not in queue .. maybe cancelled", job_handle));
		}

		_unlock();
		ifprint( (DBG_LOG, "_job_thread(): job finished: 0x%x", job_handle));
	} /*  while  */
	sem_post(&_job_end_wait_sem);

	// coverity[missing_unlock] : don't care about not unlocking _job_start_wait_sem
	return (NULL);

} /*  _job_thread  */

static int _start_thread(void)
{
	sigset_t allsig, oldsig;
	int result;

	_job_tid = pthread_self();

	result = OK;
	sigfillset(&allsig);
#if CHECK_PTHREAD_SIGMASK_STATUS
	result = pthread_sigmask(SIG_SETMASK, &allsig, &oldsig);
#else /* else CHECK_PTHREAD_SIGMASK_STATUS */
	pthread_sigmask(SIG_SETMASK, &allsig, &oldsig);
#endif /* CHECK_PTHREAD_SIGMASK_STATUS */
	if (result == OK)
	{
		result = pthread_create(&_job_tid, 0, _job_thread, NULL);
		if ((result == ERROR) && (_job_tid != pthread_self()))
		{
#if USE_PTHREAD_CANCEL
			pthread_cancel(_job_tid);
#else /* else USE_PTHREAD_CANCEL */
			pthread_kill(_job_tid, SIGKILL);
#endif /* USE_PTHREAD_CANCEL */
			_job_tid = pthread_self();
		}
	}

	if (result == OK)
	{
		sched_yield();
#if CHECK_PTHREAD_SIGMASK_STATUS
		result = pthread_sigmask(SIG_SETMASK, &oldsig, 0);
#else /* else CHECK_PTHREAD_SIGMASK_STATUS */
		pthread_sigmask(SIG_SETMASK, &oldsig, 0);
#endif /* CHECK_PTHREAD_SIGMASK_STATUS */
	}

	return (result);
} /*  _start_thread  */

static int _stop_thread(void)
{
	if (!pthread_equal(_job_tid, pthread_self()))
	{
		pthread_join(_job_tid, 0);
		_job_tid = pthread_self();
		return OK;
	} else
	{
		return ERROR;
	}

} /*  _stop_thread  */

static void _setup_io_plugins()
{
	static const wprint_io_plugin_t _file_io_plugin =
	{ .version = WPRINT_PLUGIN_VERSION(_INTERFACE_MINOR_VERSION), .port_num = PORT_FILE, .getCapsIFC =
			NULL, .getStatusIFC = NULL, .getPrintIFC = _printer_file_connect, };

	static const wprint_io_plugin_t _ipp_io_plugin =
	{ .version = WPRINT_PLUGIN_VERSION(_INTERFACE_MINOR_VERSION), .port_num = PORT_IPP, .getCapsIFC =
			ipp_status_get_capabilities_ifc, .getStatusIFC =
			ipp_status_get_monitor_ifc, .getPrintIFC = ipp_get_print_ifc, };

	_io_plugins[0].port_num = PORT_FILE;
	_io_plugins[0].io_plugin = &_file_io_plugin;

	_io_plugins[1].port_num = PORT_IPP;
	_io_plugins[1].io_plugin = &_ipp_io_plugin;
}

extern wprint_plugin_t *libwprintplugin_pcl_reg(void);
extern wprint_plugin_t *libwprintplugin_pdf_reg(void);
static void _setup_print_plugins()
{
    plugin_add(libwprintplugin_pcl_reg());
    plugin_add(libwprintplugin_pdf_reg());
}

/*____________________________________________________________________________
 |
 | wprintInit()    
 |____________________________________________________________________________
 |
 | Description: Initialize the library. Identify and gather the capabilities
 |              of the available plugins.
 | Arguments:   None 
 | Returns  :   the number of plugins found or ERROR            
 |___________________________________________________________________________*/

int wprintInit(void)
{
	wprint_plugin_t *plugin;
	int count = 0;

    memset(_libraryID, 0, sizeof(_libraryID));

    _setup_print_plugins();
	_setup_io_plugins();

	_msgQ = msgQCreate(_MAX_MSGS, sizeof(_msg_t), MSG_Q_FIFO);

	if (!_msgQ)
	{
		ifprint((DBG_ERROR, "ERROR: cannot create msgQ"));
		return (ERROR);
	}

	sem_init(&_job_end_wait_sem, 0, 0);
	sem_init(&_job_start_wait_sem, 0, 0);

	signal(SIGPIPE, SIG_IGN); /* avoid broken pipe process shutdowns */
	pthread_mutexattr_settype(&_q_lock_attr,
#if __APPLE__
			PTHREAD_MUTEX_RECURSIVE
#else
			PTHREAD_MUTEX_RECURSIVE_NP
#endif
			);

	pthread_mutex_init(&_q_lock, &_q_lock_attr);

	if (_start_thread() != OK)
	{
		ifprint((DBG_ERROR, "could not start job thread"));
		return (ERROR);
	}

	return (count);

} /*  wprintInit  */

static const printer_capabilities_t _default_cap =
{ .hasColor = true, .canPrintBorderless = true, .canPrintJPEG = true,
		.canPrintQualityDraft = true,
		.canPrintQualityNormal = true, .canPrintQualityHigh = true,
		.num_supported_media_sizes = 0, .num_supported_media_trays = 0,
		.num_supported_media_types = 0, }; // matches the default print format (most inkjets)

static void _validate_supported_media_sizes(printer_capabilities_t *printer_cap)
{

	int has5x7 = 0;
	if (printer_cap == NULL)
		return;
	if (printer_cap->num_supported_media_sizes == 0)
	{

		int i = 0;
		printer_cap->supported_media_sizes[i++] = DF_MEDIA_SIZE_A;
		printer_cap->supported_media_sizes[i++] = DF_MEDIA_SIZE_A4;
		printer_cap->supported_media_sizes[i++] = DF_MEDIA_SIZE_PHOTO_4X6;
		printer_cap->supported_media_sizes[i++] = DF_MEDIA_SIZE_PHOTO_5X7;

		printer_cap->num_supported_media_sizes = i;
	} else
	{
		int read, write;
		read = write = 0;

		for (read = write = 0; read < printer_cap->num_supported_media_sizes;
				read++)
		{
			switch (printer_cap->supported_media_sizes[read])
			{
			/* add media we want to support to this list */
			case DF_MEDIA_SIZE_A:
			case DF_MEDIA_SIZE_A4:
			case DF_MEDIA_SIZE_LEGAL:
			case DF_MEDIA_SIZE_HAGAKI:
			case DF_MEDIA_SIZE_PHOTO_4X6:
			case DF_MEDIA_SIZE_PHOTO_L:
			case DF_MEDIA_SIZE_A6:
			case DF_MEDIA_SIZE_PHOTO_8X10:
			case DF_MEDIA_SIZE_PHOTO_5X7:
			case DF_MEDIA_SIZE_PHOTO_10X15:
				has5x7 |= (printer_cap->supported_media_sizes[read]
						== DF_MEDIA_SIZE_PHOTO_5X7);
				if (write <= read)
					printer_cap->supported_media_sizes[write++] =
							printer_cap->supported_media_sizes[read];
				break;
			default:
				break;
			}
		}
		if (has5x7 && printer_cap->hasPhotoTray)
			printer_cap->supported_media_sizes[write++] =
					DF_MEDIA_SIZE_PHOTO_5x7_MAIN_TRAY;

		printer_cap->num_supported_media_sizes = write;
	}
}

static void _validate_supported_media_trays(printer_capabilities_t *printer_cap)
{
	if (printer_cap == NULL)
		return;
	if (printer_cap->num_supported_media_trays == 0)
	{
		int i = 0;
		printer_cap->supported_media_trays[i++] = PT_SRC_AUTO_SELECT;
		printer_cap->num_supported_media_trays = i;
	}
}

static void _validate_supported_media_types(printer_capabilities_t *printer_cap)
{
	if (printer_cap == NULL)
		return;
	if (printer_cap->num_supported_media_types == 0)
	{
		int i = 0;
		printer_cap->supported_media_types[i++] = DF_MEDIA_PLAIN;
		printer_cap->supported_media_types[i++] = DF_MEDIA_PHOTO;
		printer_cap->supported_media_types[i++] = DF_MEDIA_PHOTO_GLOSSY;
		printer_cap->supported_media_types[i++] = DF_MEDIA_PREMIUM_PHOTO;
		printer_cap->supported_media_types[i++] = DF_MEDIA_ADVANCED_PHOTO;
		printer_cap->num_supported_media_types = i;
	}
}

static void _validate_print_formats(printer_capabilities_t *printer_cap)
{
	if (printer_cap == NULL)
		return;

	/* check that this can print something */
	if (!printer_cap->canPrintPDF && !printer_cap->canPrintJPEG
			&& !printer_cap->canPrintPCLm && !printer_cap->canPrintPWG && !printer_cap->canPrintOther)
	{
		/* if no format is supported assume pcl3gui for now */
		ifprint(
				(DBG_ERROR, "the printer doesn't appear to support anything we can print to"));
	}
}

static void _collect_supported_input_formats(
		printer_capabilities_t *printer_caps)
{
	unsigned long long input_formats = 0 ;
	plugin_get_passthru_input_formats(&input_formats);

	/* remove things the printer can't support */
	if (!printer_caps->canPrintPDF)
		input_formats &= ~(1 << INPUT_MIME_TYPE_PDF);
	if (!printer_caps->canPrintJPEG)
		input_formats &= ~(1 << INPUT_MIME_TYPE_JPEG);

	printer_caps->supported_input_mime_types = input_formats;

	input_formats = 0;
	plugin_get_convertable_input_formats(&input_formats);
	printer_caps->supported_input_mime_types |= input_formats;
}

/*____________________________________________________________________________
 |
 | wprintGetCapabilities()
 |____________________________________________________________________________
 |
 | Description: Gets the capabilities of the specified printer
 | Arguments:   Pass in the printer address and pointer to retrieve capabilities
 | Returns  :   OK or ERROR
 |___________________________________________________________________________*/
int wprintGetCapabilities(const char *printer_addr, int port_num,
		printer_capabilities_t *printer_cap)
{
	ifprint((DBG_VERBOSE, "wprintGetCapabilities: Enter"));
	int result = ERROR;
	const ifc_printer_capabilities_t *caps_ifc = NULL;

	_REMAP_PORT(port_num);

	memcpy(printer_cap, &_default_cap, sizeof(printer_capabilities_t));

	caps_ifc = _get_caps_ifc(port_num);
	ifprint(
			(DBG_VERBOSE, "wprintGetCapabilities: after getting caps ifc: %d", caps_ifc));
	switch (port_num)
	{
	case PORT_FILE:
		printer_cap->canDuplex = 1;
		printer_cap->canPrintBorderless = 1;
		printer_cap->canPrintPCLm = (_default_pcl_type == PCLm);
		printer_cap->canPrintPWG = (_default_pcl_type == PCLPWG);
		printer_cap->strip_height = STRIPE_HEIGHT;
		result = OK;
		break;
	default:
		break;
	}

	if (caps_ifc != NULL)
	{
		caps_ifc->init(caps_ifc, printer_addr);
		result = caps_ifc->get_capabilities(caps_ifc, printer_cap);
		caps_ifc->destroy(caps_ifc);
	}

	_validate_supported_media_sizes(printer_cap);
	_validate_print_formats(printer_cap);
	_collect_supported_input_formats(printer_cap);
	_validate_supported_media_types(printer_cap);
	_validate_supported_media_trays(printer_cap);

	printer_cap->isSupported = (printer_cap->canPrintPCLm
			|| printer_cap->canPrintPDF || printer_cap->canPrintJPEG || printer_cap->canPrintPWG);

	if (result == OK)
	{
		ifprint((DBG_LOG, "\tname: %s", printer_cap->name));
		ifprint((DBG_LOG, "\tmake: %s", printer_cap->make));
		ifprint((DBG_LOG, "\thas color: %d", printer_cap->hasColor));
		ifprint((DBG_LOG, "\tcan duplex: %d", printer_cap->canDuplex));
		ifprint(
				(DBG_LOG, "\tcan rotate back page: %d", printer_cap->canRotateDuplexBackPage));
		ifprint((DBG_LOG, "\thas photo tray: %d", printer_cap->hasPhotoTray));
		ifprint(
				(DBG_LOG, "\tcan print borderless: %d", printer_cap->canPrintBorderless));
		ifprint((DBG_LOG, "\tcan print pdf: %d", printer_cap->canPrintPDF));
		ifprint((DBG_LOG, "\tcan print jpeg: %d", printer_cap->canPrintJPEG));
		ifprint((DBG_LOG, "\tcan print pclm: %d", printer_cap->canPrintPCLm));
		ifprint((DBG_LOG, "\tcan print pwg: %d", printer_cap->canPrintPWG));
		ifprint(
				(DBG_LOG, "\tsupports quality: draft: %d\tnormal: %d\tbest: %d\n", printer_cap->canPrintQualityDraft, printer_cap->canPrintQualityNormal, printer_cap->canPrintQualityHigh));
		ifprint(
				(DBG_LOG, "\tprinter supported: %d\n", printer_cap->isSupported));
		ifprint((DBG_LOG, "\tstrip height: %d", printer_cap->strip_height));
		ifprint((DBG_LOG, "\tinkjet: %d\n", printer_cap->inkjet));
		ifprint((DBG_LOG, "\t300dpi: %d\n", printer_cap->canPrint300DPI));
		ifprint((DBG_LOG, "\t600dpi: %d\n", printer_cap->canPrint600DPI));
	}

	ifprint((DBG_VERBOSE, "wprintGetCapabilities: Exit"));
	return (result);
} /*  wprintGetCapabilities  */

static char *_get_print_format(const char *printer_addr, int port_num,
		char *mime_type, const wprint_job_params_t *job_params,
		printer_capabilities_t *cap)
{

	char *print_format = NULL;

	errno = OK;

	/*  use JPEG print path of Photosmart printers, if we are printing a single
	 *  JPEG image in a full page borderless mode to a printer that is capable
	 *  of supporting borderless printing.
	 */

	if (job_params && job_params->borderless && cap->canPrintBorderless
			&& cap->canPrintJPEG && (_default_pcl_type == PCLJPEG)
			&& strcmp(mime_type, MIME_TYPE_JPEG) == 0)
	{
		print_format = PRINT_FORMAT_JPEG;
		ifprint(
				(DBG_LOG, "_get_print_format(): print_format switched to %s for single borderless JPEG print", print_format));
	}
	else if ((strcmp(mime_type, MIME_TYPE_PDF) == 0) && cap->canPrintPDF)
	{
		print_format = PRINT_FORMAT_PDF;
	}
	else if(cap->canPrintPCLm || cap->canPrintPDF) /* PCLm is a subset of PDF */
	{
		print_format = PRINT_FORMAT_PCLM;
#if (USE_PWG_OVER_PCLM != 0)
		if (cap->canPrintPWG) {
			print_format = PRINT_FORMAT_PWG;
		}
#endif // (USE_PWG_OVER_PCLM != 0)
	}
	else if(cap->canPrintPWG)
	{
		print_format = PRINT_FORMAT_PWG;
	}
	else
	{
		errno = EBADRQC;
	}

	if (print_format != NULL)
	{
		ifprint((DBG_LOG, "\t_get_print_format(): print_format: %s", print_format));
	}

	return (print_format);
} /*  _get_print_format  */

/*____________________________________________________________________________
 |
 | wprintGetDefaultJobParams()    
 |____________________________________________________________________________
 |
 | Description: Gets the default job parameters
 | Arguments:   Pass in a pointer to default job parameters
 | Returns  :   void                   
 |___________________________________________________________________________*/
int wprintGetDefaultJobParams(const char *printer_addr, int port_num,
		wprint_job_params_t *job_params, printer_capabilities_t *printer_cap)
{
	int result = ERROR;
	static const wprint_job_params_t _default_job_params =
	{ .print_format = _DEFAULT_PRINT_FORMAT, .pcl_type = _DEFAULT_PCL_TYPE,
			.media_size = US_LETTER, .media_type = DF_MEDIA_PLAIN,
			.print_quality = DF_QUALITY_NORMAL, .duplex = DF_DUPLEX_MODE_NONE,
			.dry_time = DUPLEX_DRY_TIME_NORMAL, .color_space =
					DF_COLOR_SPACE_COLOR, .media_tray = PT_SRC_AUTO_SELECT,
			.pixel_units = DEFAULT_RESOLUTION, .render_flags = 0, .num_copies =
					1, .borderless = false, .cancelled = false,
			.renderInReverseOrder = false, .ipp_1_0_supported = false,
			.ipp_2_0_supported = false, .ePCL_ipp_supported = false,
			.strip_height = STRIPE_HEIGHT,
            .docCategory = { 0 },

	};

	if (job_params == NULL)
		return (result);

	memcpy(job_params, &_default_job_params, sizeof(_default_job_params));

	_REMAP_PORT(port_num);

	switch (port_num)
	{
	case PORT_FILE:
		job_params->print_format = (char *) _default_print_format;
		job_params->pcl_type = _default_pcl_type;
		break;
	default:
		break;
	}
	result = OK;
	return (result);
} /*  wprintGetDefaultJobParams  */

int wprintGetFinalJobParams(const char *printer_addr, int port_num,
		wprint_job_params_t *job_params, printer_capabilities_t *printer_cap)
{
	int i;
	int result = ERROR;
	float margins[NUM_PAGE_MARGINS];

	if (job_params == NULL)
		return (result);
	_REMAP_PORT(port_num);
	result = OK;

    job_params->acceptsPCLm = printer_cap->canPrintPCLm;

	if (printer_cap->ePCL_ipp_version == 1)
	{
		job_params->ePCL_ipp_supported = true;
	}

	if (printer_cap->ippVersionMajor == 2)
	{
		job_params->ipp_1_0_supported = true;
		job_params->ipp_2_0_supported = true;
	}
	else if (printer_cap->ippVersionMajor == 1)
	{
		job_params->ipp_1_0_supported = true;
		job_params->ipp_2_0_supported = false;
	}

	if (!printer_cap->hasColor)
	{
		job_params->color_space = DF_COLOR_SPACE_MONO;
	}

	if (printer_cap->canPrintPCLm || printer_cap->canPrintPDF)
	{
		job_params->pcl_type = PCLm;
#if (USE_PWG_OVER_PCLM != 0)
		if ( printer_cap->canPrintPWG) {
			job_params->pcl_type = PCLPWG;
        }
#endif // (USE_PWG_OVER_PCLM != 0)
	} else if (printer_cap->canPrintPWG)
	{
		job_params->pcl_type = PCLPWG;
	}

	// make sure requested PQ is supported by printer; assumes that
	// printer will always support 'normal' PQ mode
	switch(job_params->print_quality)
	{
		case DF_QUALITY_FAST:
		{
			// try to use 'fast' if printer supports it. if not, try to
			// use 'normal'. if printer doesn't support 'normal', use
			// 'best' PQ.
			if(!printer_cap->canPrintQualityDraft)
			{
				if(!printer_cap->canPrintQualityNormal)
				{
					if(!printer_cap->canPrintQualityHigh)
					{
						// this should NOT happen, defaulting to 'normal'
						job_params->print_quality = DF_QUALITY_NORMAL;
					}
					else
					{
						job_params->print_quality = DF_QUALITY_BEST;
					}
				}
				else
				{
					job_params->print_quality = DF_QUALITY_NORMAL;
				}
			}
			break;
		}
		case DF_QUALITY_NORMAL:
		{
			// try to use 'normal' if printer supports it. if not, try to
			// use 'draft'. if printer doesn't support 'draft', use
			// 'best' PQ.
			if(!printer_cap->canPrintQualityNormal)
			{
				if(!printer_cap->canPrintQualityDraft)
				{
					if(!printer_cap->canPrintQualityHigh)
					{
						// this should NOT happen, defaulting to 'normal'
						job_params->print_quality = DF_QUALITY_NORMAL;
					}
					else
					{
						job_params->print_quality = DF_QUALITY_BEST;
					}
				}
				else
				{
					job_params->print_quality = DF_QUALITY_FAST;
				}
			}
			break;
		}
		case DF_QUALITY_BEST:
		{
			// try to use 'best' if printer supports it. if not, try to
			// use 'normal'.  if printer doesn't support 'normal', use
			// 'fast' PQ.
			if(!printer_cap->canPrintQualityHigh)
			{
				if(!printer_cap->canPrintQualityNormal)
				{
					if(!printer_cap->canPrintQualityDraft)
					{
						// this should NOT happen, defaulting to 'normal'
						job_params->print_quality = DF_QUALITY_NORMAL;
					}
					else
					{
						job_params->print_quality = DF_QUALITY_FAST;
					}
				}
				else
				{
					job_params->print_quality = DF_QUALITY_NORMAL;
				}
			}
			break;
		}
		default:
		{
			job_params->print_quality = DF_QUALITY_NORMAL;
		}
	}

	ifprint(
			(DBG_VERBOSE, "wprintGetFinalJobParams: Using PCL Type %s", getPCLTypeString(job_params->pcl_type)));

	/* set strip height */
	job_params->strip_height = printer_cap->strip_height;

	/* make sure the number of copies is valid */
	if (job_params->num_copies <= 0)
		job_params->num_copies = 1;

	/* confirm that the media size is supported */
	for (i = 0; i < printer_cap->num_supported_media_sizes; i++)
	{
		DF_media_size_t size = (
				(job_params->media_size == DF_MEDIA_SIZE_PHOTO_5x7_MAIN_TRAY) ?
						DF_MEDIA_SIZE_PHOTO_5X7 : job_params->media_size);
		if (size == printer_cap->supported_media_sizes[i])
			break;
	}

	if (i >= printer_cap->num_supported_media_sizes)
	{
		job_params->media_size = DF_MEDIA_SIZE_A;
		job_params->media_tray = PT_SRC_AUTO_SELECT;
	}

	/* check that we support the media tray */
	for (i = 0; i < printer_cap->num_supported_media_trays; i++)
	{
		if (job_params->media_tray == printer_cap->supported_media_trays[i])
			break;
	}

	/* media tray not supported, default to automatic */
	if (i >= printer_cap->num_supported_media_trays)
	{
		job_params->media_tray = PT_SRC_AUTO_SELECT;
	}

	/* check that we support the media type */
	for (i = 0; i < printer_cap->num_supported_media_types; i++)
	{
		if (job_params->media_type == printer_cap->supported_media_types[i])
			break;
	}

	/* media type not supported, default to plain */
	if (i >= printer_cap->num_supported_media_types)
	{
		job_params->media_type = DF_MEDIA_PLAIN;
	}

	/* if the paper size is 5x7 main tray, switch it back to 5x7 */
	if (job_params->media_size == DF_MEDIA_SIZE_PHOTO_5x7_MAIN_TRAY)
	{
		job_params->media_size = DF_MEDIA_SIZE_PHOTO_5X7;
	}

	/* verify borderless setting */
	if ((job_params->borderless == true) && !printer_cap->canPrintBorderless)
	{
		job_params->borderless = false;
	}

    // borderless and margins don't get along
    if (job_params->borderless &&
        ((job_params->job_top_margin > 0.0f) ||
         (job_params->job_left_margin > 0.0f) ||
         (job_params->job_right_margin > 0.0f) ||
         (job_params->job_bottom_margin > 0.0f)))
    {
		job_params->borderless = false;
    }

	/* verify duplex setting */
	if ((job_params->duplex != DF_DUPLEX_MODE_NONE) && !printer_cap->canDuplex)
	{
		job_params->duplex = DF_DUPLEX_MODE_NONE;
	}

    // borderless and duplex don't get along either
    if (job_params->borderless && (job_params->duplex != DF_DUPLEX_MODE_NONE)) {
		job_params->duplex = DF_DUPLEX_MODE_NONE;
    }

	if ((job_params->duplex == DF_DUPLEX_MODE_BOOK)
			&& !printer_cap->canRotateDuplexBackPage)
	{
		job_params->render_flags |= RENDER_FLAG_ROTATE_BACK_PAGE;
	}

	if (job_params->render_flags & RENDER_FLAG_ROTATE_BACK_PAGE)
	{
		ifprint(
				(DBG_LOG, "wprintGetFinalJobParams: Duplex is on and device needs back page rotated."));
	}

	if ((job_params->duplex == DF_DUPLEX_MODE_NONE)
			&& !printer_cap->hasFaceDownTray)
	{
		job_params->renderInReverseOrder = true;
	}
	else
	{
		job_params->renderInReverseOrder = false;
	}

	if (!printer_cap->hasColor && !printer_cap->canGrayscale)
	{
		job_params->mono1Bit = true;
	}
	else
	{
		job_params->mono1Bit = false;
	}

	if (job_params->render_flags & RENDER_FLAG_AUTO_SCALE)
	{
		job_params->render_flags |= AUTO_SCALE_RENDER_FLAGS;
	}
	else if (job_params->render_flags & RENDER_FLAG_AUTO_FIT)
	{
		job_params->render_flags |= AUTO_FIT_RENDER_FLAGS;
	}

	/*
	 * PQ is normal by default
	 * PQ is MaxDPI (IPP_DRAFT) for PCLm Jobs which eventually switches to 600DPI
	 * we only support draft, normal, and High
	 * these map to    0		1			2
	 * map to 		fast		normal		best
	 * map to IPP: 	draft(3)	normal(4)	high(5)
	 * everything we use, except PCLm will get dumbed down to DEFAULT_RESOLUTION (300)DPI
	 *
	 */
	switch (job_params->print_quality)
	{
	case DF_QUALITY_MAXDPI:
	case DF_QUALITY_BEST:
		job_params->pixel_units = BEST_MODE_RESOLUTION;
		break;
	case DF_QUALITY_FAST:
	case DF_QUALITY_NORMAL:
	case DF_QUALITY_AUTO:
	default: {
		switch(job_params->pcl_type) {
			case PCLm:
			case PCLPWG:
				job_params->pixel_units = (printer_cap->canPrint300DPI ? DEFAULT_RESOLUTION : BEST_MODE_RESOLUTION);
				break;
			default:
				job_params->pixel_units = DEFAULT_RESOLUTION;
				break;
		}
		break;
	}
	}

    printable_area_get_default_margins(job_params,
                                       printer_cap,
                                       &margins[TOP_MARGIN],
                                       &margins[LEFT_MARGIN],
                                       &margins[RIGHT_MARGIN],
                                       &margins[BOTTOM_MARGIN]);

    printable_area_get(job_params,
                       margins[TOP_MARGIN],
                       margins[LEFT_MARGIN],
                       margins[RIGHT_MARGIN],
                       margins[BOTTOM_MARGIN]);

	return (result);
} /* wprintGetFinalJobParams */

/*____________________________________________________________________________
 |
 | wprintStartJob()    
 |____________________________________________________________________________
 |
 | Description: This interface method must be called once per job at the start 
 |              of the job. 
 | Arguments:   printer addr,   (hostname, IP address or file path)
 |              port number     (PORT_FILE will print to file)
 |              pointer to job_params  (default job params assumed, if NULL)
 |              input document MIME type
 |              input document pathname
 |              callback function
 |
 | Returns  :   a print job handle that is used as a pointer in other methods of
 |              this library  or  WPRINT_BAD_JOB_HANDLE 
 |___________________________________________________________________________*/

wJob_t wprintStartJob(const char *printer_addr, port_t port_num,
		wprint_job_params_t *job_params, printer_capabilities_t *printer_cap,
		char *mime_type, char *pathname, wprintSyncCb_t cb_fn, const char *debugDir)
{
	wJob_t job_handle = WPRINT_BAD_JOB_HANDLE;
	_msg_t msg;
	struct stat stat_buf;
	bool is_dir = false;
	_job_queue_t *jq;
	wprint_plugin_t *plugin = NULL;
	char *print_format;
	ifc_print_job_t *print_ifc;
      	plugin_reg_t reg_fn;
      	char reg_fn_name[64];

	_REMAP_PORT(port_num);

	if (mime_type == NULL)
	{
		errno = EINVAL;
		return (job_handle);
	}

	print_format = _get_print_format(printer_addr, port_num, mime_type,
			job_params, printer_cap);

	if (print_format == NULL)
		return (job_handle);

	/*  check to see if we have an appropriate plugin  */
	if (OK == stat(pathname, &stat_buf))
	{
		if (S_ISDIR(stat_buf.st_mode))
			is_dir = true;
		else if (stat_buf.st_size == 0)
		{
			errno = EBADF;
			return (job_handle);
		}
	} else
	{
		errno = ENOENT;
		return (job_handle);
	}

	//TODO: we don't handle if job_params is not setup ahead of time. test and remove default_params logic
	if ((job_params == NULL)) //&& (wprintGetDefaultJobParams(printer_addr, port_num, &default_params, printer_cap) == ERROR))
	{
		errno = ECOMM;
		return (job_handle);
	}

	plugin = plugin_search(mime_type, print_format);

	_lock();

	if (plugin)
	{
		job_handle = _get_handle();
		if (job_handle == WPRINT_BAD_JOB_HANDLE)
			errno = EAGAIN;
	}
	else
	{
		errno = ENOSYS;
		ifprint(
				(DBG_ERROR, "wprintStartJob(): ERROR: no plugin found for %s => %s", mime_type, print_format));
	}

	if (job_handle != WPRINT_BAD_JOB_HANDLE)
	{
		print_ifc = (ifc_print_job_t*) _get_print_ifc(port_num);
		/*  fill out the job queue record  */

		jq = _get_job_desc(job_handle);
		if (jq == NULL)
		{
			_recycle_handle(job_handle);
			job_handle = WPRINT_BAD_JOB_HANDLE;
			_unlock();
			return (job_handle);
		}

        if (debugDir != NULL) {
            strncpy(jq->debug_path, debugDir, MAX_PATHNAME_LENGTH);
            jq->debug_path[MAX_PATHNAME_LENGTH] = 0;
        }

		// coverity[uninspected]
		strncpy(jq->printer_addr, printer_addr, MAX_PRINTER_ADDR_LENGTH);
		strncpy(jq->mime_type, mime_type, MAX_MIME_LENGTH);
		strncpy(jq->pathname, pathname, MAX_PATHNAME_LENGTH);

		jq->port_num = port_num;
		jq->cb_fn = cb_fn;
		jq->print_ifc = print_ifc;
		jq->cancel_ok = true; // assume cancel is ok
        jq->plugin = plugin;

		jq->status_ifc = _get_status_ifc(port_num);

		memcpy((char *) &(jq->job_params), job_params,
				sizeof(wprint_job_params_t));

        {
            int useragent_len = strlen(_libraryID) + strlen(jq->job_params.docCategory) + 1;
            char *useragent = (char*)malloc(useragent_len + 1);
            if (useragent != NULL) {
                memset(useragent, 0, useragent_len + 1);
                snprintf(useragent, useragent_len + 1, "%s%s", _libraryID, jq->job_params.docCategory);
                jq->job_params.useragent = useragent;
            }
        }
// TODO: we are not handling a case where we need to get default params. remove with code above
//if (job_params)
//		else
//           memcpy( (char *) &(jq->job_params),
//                   &default_params,
//                   sizeof(wprint_job_params_t) );

		jq->job_params.page_num = 0;
		//TODO: fix Print_format Logic does not match above
		jq->job_params.print_format = print_format;

		if (strcmp(print_format, PRINT_FORMAT_PCLM) == 0)
		{
			if (printer_cap->canPrintPCLm || printer_cap->canPrintPDF)
				jq->job_params.pcl_type = PCLm;
			else
				jq->job_params.pcl_type = PCLNONE;
		}

		if (strcmp(print_format, PRINT_FORMAT_PWG) == 0)
		{
			if (printer_cap->canPrintPWG)
				jq->job_params.pcl_type = PCLPWG;
			else
				jq->job_params.pcl_type = PCLNONE;
		}

		/* if the pathname is a directory, then it is a multi-page
		 * job, where the client provides individual pages in the
		 * specified MIME type using wprintPage()
		 */

		if (is_dir)
		{
			jq->is_dir = true;
			jq->num_pages = 0;

			/* create a pageQ for queuing page information */
			jq->pageQ = msgQCreate(_MAX_PAGES_PER_JOB, sizeof(_page_t),
					MSG_Q_FIFO);

			/* create a secondary page Q for subsequently saving page
			 * data for copies #2 to n */
			if (jq->job_params.num_copies > 1)
			{
				jq->saveQ = msgQCreate(_MAX_PAGES_PER_JOB, sizeof(_page_t),
						MSG_Q_FIFO);
			}
		} else
		{
			jq->num_pages = 1;
		}

		// post a message with job_handle to the msgQ that is serviced by a thread
		msg.id = MSG_RUN_JOB;
		msg.job_id = job_handle;

		if (print_ifc && plugin && plugin->print_page
				&& (msgQSend(_msgQ, (char *) &msg, sizeof(msg), NO_WAIT,
						MSG_Q_FIFO) == OK))
		{
			errno = OK;
			ifprint(
					(DBG_LOG, "wprintStartJob(): print job 0x%x queued\n\t\t(%s => %s)", job_handle, mime_type, print_format));
		}
		else
		{
			if (print_ifc == NULL)
				errno = EAFNOSUPPORT;
			else if ((plugin == NULL) || (plugin->print_page == NULL))
				errno = ELIBACC;
			else
				errno = EBADMSG;

			ifprint(
					(DBG_ERROR, "wprintStartJob(): ERROR plugin->start_job(0x%x) : %s => %s", job_handle, mime_type, print_format));
			jq->job_state = JOB_STATE_ERROR;
			_recycle_handle(job_handle);
			job_handle = WPRINT_BAD_JOB_HANDLE;
		}
	}

	_unlock();

	return (job_handle);

} /** wprintStartJob **/

/*____________________________________________________________________________
 |
 | wprintEndJob()    
 |____________________________________________________________________________
 |
 | Description: This interface method must be called once per job at the end 
 |              of the job. 
 | Arguments:   job_handle returned by wprintStartJob()
 | Returns  :   OK  or  ERROR
 |___________________________________________________________________________*/

/**
 * wprintEndJob(): Sent once per job at the end of the job. A current print job
 * must end for the next one to start. Returns OK or ERROR as the case maybe. 
 **/

int wprintEndJob(wJob_t job_handle)
{
	_page_t page;
	_job_queue_t *jq;
	int result = ERROR;

	_lock();
	jq = _get_job_desc(job_handle);

	if (jq)
	{
		/*  if the job is done and is to be freed, do it  */

		if ((jq->job_state == JOB_STATE_CANCELLED)
				|| (jq->job_state == JOB_STATE_ERROR)
				|| (jq->job_state == JOB_STATE_CORRUPTED)
				|| (jq->job_state == JOB_STATE_COMPLETED))
		{
			result = OK;

			if (jq->pageQ)
			{
				while ((msgQNumMsgs(jq->pageQ) > 0)
						&& (msgQReceive(jq->pageQ, (char *) &page, sizeof(page),
								WAIT_FOREVER) == OK))
				{
				}
				result |= msgQDelete(jq->pageQ);
				jq->pageQ = NULL;
			}

			if (jq->saveQ)
			{
				while ((msgQNumMsgs(jq->saveQ) > 0)
						&& (msgQReceive(jq->saveQ, (char *) &page, sizeof(page),
								WAIT_FOREVER) == OK))
				{
				}
				result |= msgQDelete(jq->saveQ);
				jq->saveQ = NULL;
			}
			_recycle_handle(job_handle);
		} else
		{
			ifprint(
					(DBG_ERROR, "job 0x%x cannot be ended from state %d", job_handle, jq->job_state));
		}
	} else
	{
		ifprint(
				(DBG_ERROR, "ERROR: wprintEndJob(0x%x), job not found", job_handle));
	}

	_unlock();

	return (result);

} /** wprintEndJob  **/

/*____________________________________________________________________________
 |
 | wprintPage()    
 |____________________________________________________________________________
 |
 | Description: This interface method is called once per page of a multi-page
 |              job to deliver a page image in a previously specified MIME type.
 | Arguments:   job_handle returned by wprintStartJob()
 |              page_number   (1..n)
 |              pointer to filename in known directory OR a full pathname
 |              flag to indicate last page of the job
 | Returns  :   OK  or  ERROR
 |___________________________________________________________________________*/

int wprintPage(wJob_t job_handle, int page_num, char *filename, bool last_page,
		unsigned int top_margin, unsigned int left_margin,
		unsigned int right_margin, unsigned int bottom_margin)
{
	_job_queue_t *jq;
	_page_t page;
	int result = ERROR;
	struct stat stat_buf;

	_lock();
	jq = _get_job_desc(job_handle);

	/* use empty string to indicate EOJ for an empty job */
	if (!filename)
	{
		filename = "";
		last_page = true;
	} else if (OK == stat(filename, &stat_buf))
	{
		if (!S_ISREG(stat_buf.st_mode) || (stat_buf.st_size == 0))
		{
			_unlock();
			return (result);
		}
	} else
	{
		_unlock();
		return (result);
	}

	/*  must be setup as a multi-page job and the page_num must be valid */
	/*  also ensure that the filename will fit in the space allocated in
	 *  the pageQ
	 */
	if (jq && jq->is_dir && !(jq->last_page_seen)
			&& (((strlen(filename) < MAX_PATHNAME_LENGTH))
					|| (jq && (strcmp(filename, "") == 0) && last_page)))
	{
		memset(&page, 0, sizeof(page));
		page.page_num = page_num;
		page.corrupted = false;
		page.last_page = last_page;
		page.top_margin = top_margin;
		page.left_margin = left_margin;
		page.right_margin = right_margin;
		page.bottom_margin = bottom_margin;

		if ((strlen(filename) == 0) || strchr(filename, '/'))
		{
			/* assume empty or complete pathname and use it as it is */
			strncpy(page.filename, filename, MAX_PATHNAME_LENGTH);
		} else
		{
			/* generate a complete pathname */
			snprintf(page.filename, MAX_PATHNAME_LENGTH, "%s/%s", jq->pathname,
					filename);
		}

		if (last_page)
		{
			jq->last_page_seen = true;
			if ((jq->cb_fn != NULL) && (jq->job_params.page_num > 0)
					&& jq->job_params.page_printing)
			{
				wprint_job_callback_params_t cb_param;
				cb_param.id = WPRINT_CB_PARAM_PAGE_START_INFO;
				cb_param.param.page_info.page_num =
						jq->job_params.copy_page_num;
				cb_param.param.page_info.copy_num = jq->job_params.copy_num;
				cb_param.param.page_info.corrupted_file =
						jq->job_params.page_corrupted;
				cb_param.param.page_info.current_page = jq->job_params.page_num;
				cb_param.param.page_info.total_pages = (jq->num_pages
						* jq->job_params.num_copies);
				cb_param.param.page_info.page_total_update = 1;
				jq->cb_fn(job_handle, (void *) &cb_param);
			}
		}

		result = msgQSend(jq->pageQ, (char *) &page, sizeof(page), NO_WAIT,
				MSG_Q_FIFO);
	}

	if (result == OK)
	{
		ifprint(
				(DBG_LOG, "wprintPage(0x%x, %d, %s, %d)", job_handle, page_num, filename, last_page));
		if (!(last_page && (strcmp(filename, "") == 0)))
		{
			jq->num_pages++;
		}
	} else
	{
		ifprint(
				(DBG_ERROR, "ERROR : wprintPage(0x%x, %d, %s, %d)", job_handle, page_num, filename, last_page));
	}

	_unlock();
	return (result);

} /*  wprintPage  */

/**
 * wprintCancelJob(): cancel a spooled or running job 
 * Returns OK or ERROR
 **/

int wprintCancelJob(wJob_t job_handle)
{
	_job_queue_t *jq;
	int result = OK;

	_lock();

	jq = _get_job_desc(job_handle);

	if (jq)
	{
		ifprint((DBG_LOG, "received cancel request"));
		/* send a dummy page in case we're waiting on the msgQ page receive */
		if ((jq->job_state == JOB_STATE_RUNNING)
				|| (jq->job_state == JOB_STATE_BLOCKED))
		{
			bool enableTimeout = true;
			jq->cancel_ok = true;
			jq->job_params.cancelled = true;
			wprintPage(job_handle, jq->num_pages + 1, NULL, true, 0, 0, 0, 0);
			if (jq->status_ifc)
			{
				/* are we blocked waiting for the job to start */
				if ((jq->job_state == JOB_STATE_BLOCKED)
						&& (jq->job_params.page_num == 0))
				{
				} else
				{
					errno = OK;
					jq->cancel_ok = ((jq->status_ifc->cancel)(jq->status_ifc)
							== 0);
					if ((jq->cancel_ok == true) && (errno != OK))
						enableTimeout = false;
				}
			}
			if (!jq->cancel_ok)
			{
				ifprint(
						(DBG_ERROR, "CANCEL did not go through or is not supported for this device"));
				enableTimeout = true;
			}
			if (enableTimeout && (jq->print_ifc != NULL)
					&& (jq->print_ifc->enable_timeout != NULL))
				jq->print_ifc->enable_timeout(jq->print_ifc, 1);

			errno = (jq->cancel_ok ? OK : ENOTSUP);
			jq->job_state = JOB_STATE_CANCEL_REQUEST;
			result = OK;
		} else if ((jq->job_state == JOB_STATE_CANCEL_REQUEST)
				|| (jq->job_state == JOB_STATE_CANCELLED))
		{
			result = OK;
			errno = (jq->cancel_ok ? OK : ENOTSUP);
		} else if (jq->job_state == JOB_STATE_QUEUED)
		{
			jq->job_params.cancelled = true;
			jq->job_state = JOB_STATE_CANCELLED;

			if (jq->cb_fn)
			{
				wprint_job_callback_params_t cb_param;
				cb_param.id = WPRINT_CB_PARAM_JOB_STATE;
				cb_param.param.state = JOB_DONE;
				cb_param.blocked_reasons = BLOCKED_REASONS_CANCELLED;
				cb_param.job_done_result = CANCELLED;

				jq->cb_fn(job_handle, (void *) &cb_param);
			}

			errno = OK;
			result = OK;
		} else
		{
			ifprint((DBG_ERROR, "job in other state"));
			result = ERROR;
			errno = EBADRQC;
		}
	} else
	{
		ifprint((DBG_ERROR, "could not find job"));
		result = ERROR;
		errno = EBADR;
	}

	_unlock();

	return (result);

} /*  wprintCancelJob  */

/**
 * wprintResumePrint(): resumes a suspended printer that is awaiting user action 
 * Returns OK or ERROR
 **/

int wprintResumePrint(wJob_t job_handle)
{
	_job_queue_t *jq;
	int result = OK;

	_lock();

	jq = _get_job_desc(job_handle);

	if (jq)
	{
		ifprint((DBG_LOG, "received resume request"));
		if (jq->job_state == JOB_STATE_BLOCKED)
		{
			/* allow resume only when we're blocked */
			if ((jq->status_ifc) && (jq->status_ifc->resume != NULL))
			{
				/* are we blocked waiting for the job to start */
				if ((jq->job_state == JOB_STATE_BLOCKED)
						&& (jq->job_params.page_num == 0))
				{
					/* blocked for other reasons, ignore */
				} else
				{
					(jq->status_ifc->resume)(jq->status_ifc);
				}
			} else if (jq->status_ifc)
			{
				/* resume not supported by IO */
				errno = ENOTSUP;
			}
			result = OK;
		} else
			ifprint((DBG_ERROR, "job in other state"));
		result = ERROR;
		errno = EBADRQC;
	} else
	{
		ifprint((DBG_ERROR, "could not find job"));
		result = ERROR;
		errno = EBADR;
	}

	_unlock();

	return (result);

} /*  wprintResumePrint  */

int wprintJobDesc(wJob_t job_handle, wprint_job_desc_t *job)
{
	_job_queue_t *jq;
	int result = OK;

	_lock();

	jq = _get_job_desc(job_handle);

	if (jq && job)
	{

		strncpy(job->printer_addr, jq->printer_addr,
				MAX_PRINTER_ADDR_LENGTH - 1);
		strncpy(job->pathname, jq->pathname, MAX_PATHNAME_LENGTH - 1);

		job->blocked_reasons = jq->blocked_reasons;
		job->job_done_result = OK;
		job->current_page = 0;
		job->total_pages = -1;

		if (jq->last_page_seen)
			job->total_pages = (jq->num_pages * jq->job_params.num_copies);

		switch (jq->job_state)
		{
		case JOB_STATE_QUEUED:

			job->state = JOB_QUEUED;
			break;

		case JOB_STATE_BLOCKED:

			job->current_page = MAX(1, jq->job_params.page_num);
			job->state = JOB_BLOCKED;
			break;

		case JOB_STATE_CANCEL_REQUEST:
		case JOB_STATE_CANCELLED:

			if (job->total_pages == -1)
				job->total_pages = MAX(1, jq->num_pages);
			job->current_page = job->total_pages;
			job->state = JOB_DONE;
			job->job_done_result = CANCELLED;
			break;

		case JOB_STATE_COMPLETED:

			if (job->total_pages == -1)
				job->total_pages = MAX(1, jq->num_pages);
			job->current_page = job->total_pages;
			job->state = JOB_DONE;
			job->job_done_result = OK;
			break;

		case JOB_STATE_CORRUPTED:

			if (job->total_pages == -1)
				job->total_pages = MAX(1, jq->num_pages);
			job->current_page = job->total_pages;
			job->state = JOB_DONE;
			job->job_done_result = CORRUPT;
			break;

		case JOB_STATE_FREE:
		case JOB_STATE_ERROR:

			if (job->total_pages == -1)
				job->total_pages = MAX(1, jq->num_pages);
			job->current_page = job->total_pages;
			job->state = JOB_DONE;
			job->job_done_result = ERROR;
			break;

		case JOB_STATE_RUNNING:
		default:

			job->current_page = MAX(1, jq->job_params.page_num);
			job->state = JOB_RUNNING;
			break;

		} /*  switch(jq->job_state)  */

	} else
	{
		result = ERROR;
	}

	_unlock();

	return (result);

} /*  wprintJobDesc  */

int wprintExit(void)
{
	int i;
	_msg_t msg;

	if (_msgQ)
	{
		/*  toss the remaining messages in the msgQ  */
		while ((msgQNumMsgs(_msgQ) > 0)
				&& (OK
						== msgQReceive(_msgQ, (char *) &msg, sizeof(msg),
								NO_WAIT)))
			;

		/* send a quit messeage */
		msg.id = MSG_QUIT;
		msgQSend(_msgQ, (char *) &msg, sizeof(msg), NO_WAIT, MSG_Q_FIFO);

		/* stop the job thread */
		_stop_thread();

		/* empty out the semaphore */
		while (sem_trywait(&_job_end_wait_sem) == OK)
		{
		}
		while (sem_trywait(&_job_start_wait_sem) == OK)
		{
		}

		/* receive any messages just in case */
		while ((msgQNumMsgs(_msgQ) > 0)
				&& (OK
						== msgQReceive(_msgQ, (char *) &msg, sizeof(msg),
								NO_WAIT)))
			;

		/*  delete the msgQ  */
		msgQDelete(_msgQ);
		_msgQ = NULL;

		sem_destroy(&_job_end_wait_sem);
		sem_destroy(&_job_start_wait_sem);
		pthread_mutex_destroy(&_q_lock);
	}

	for (i = 0; i < ARRAY_SIZE(_io_plugins); i++) {
		if (_io_plugins[i].dl_handle != NULL) {
			dlerror();
			dlclose(_io_plugins[i].dl_handle);
		}
	}

	return (OK);

} /*  wprintExit  */

void wprintGetTimeouts(long int *tcp_timeout_msec, long int *udp_timeout_msec,
		int *udp_retries)
{
	printer_get_timeout(tcp_timeout_msec);
}

void wprintSetTimeouts(long int tcp_timeout_msec, long int udp_timeout_msec,
		int udp_retries)
{
	ifprint((DBG_LOG, "TCP timeout set to %d milliseconds", tcp_timeout_msec));
	ifprint(
			(DBG_LOG, "UDP timeout set to %d milliseconds with %d retries", udp_timeout_msec, udp_retries));
}

char * getPCLTypeString(pcl_t pclenum)
{
	switch (pclenum)
	{
	case PCLNONE:
		return "PCL_NONE";
	case PCLm:
		return "PCLm";
	case PCLJPEG:
		return "PCL_JPEG";
	case PCLPWG:
		return "PWG-Raster";
	default:
		return "unkonwn PCL Type";
	}
}

int wprintGetPrinterStatus(const char           *printer_addr,
                            int                  port_num,
                            printer_state_dyn_t *printer_status)
{
    int i;
    int result = ERROR;
	_REMAP_PORT(port_num);
	const ifc_status_monitor_t *status_ifc;
    do {
        if (!printer_addr || !printer_status)
            continue;

        /* clear the status */
        printer_status->printer_status = PRINT_STATUS_UNKNOWN;
        for(i = 0; i <= PRINT_STATUS_MAX_STATE; i++)
            printer_status->printer_reasons[i] = PRINT_STATUS_MAX_STATE;

        /* try to get the status */
        status_ifc = _get_status_ifc(port_num);
    	if (status_ifc != NULL)
    	{
    		status_ifc->init(status_ifc, printer_addr);
     	    status_ifc->get_status(status_ifc, printer_status);
    		status_ifc->destroy(status_ifc);
    	} else if (port_num == PORT_FILE) {
            printer_status->printer_status = PRINT_STATUS_IDLE | PRINTER_IDLE_BIT;
        }
        result = OK;

    } while(0);
    return result;
}

typedef struct _status_handle_st {
    const ifc_status_monitor_t *status_ifc;
    pthread_t                   status_tid;
    void (*status_callback)(const printer_state_dyn_t *new_status,
                            const printer_state_dyn_t *old_status,
                            void *param);
    void                       *param;
} _status_handle_t;

wStatus_t wprintStatusMonitorSetup(const char *printer_addr,
                                   int        port_num)
{
	const ifc_status_monitor_t *status_ifc;
    _status_handle_t *handle = NULL;
	_REMAP_PORT(port_num);
    do {
        if (!printer_addr) {
            continue;
        }

        status_ifc = _get_status_ifc(port_num);
        if (status_ifc != NULL) {
            handle = malloc(sizeof(_status_handle_t)); 
            if (handle != NULL) {
                handle->status_ifc = status_ifc;
    		    handle->status_ifc->init(handle->status_ifc, printer_addr);
                handle->status_tid = pthread_self();
                handle->param = NULL;
            } else {
    		    status_ifc->destroy(status_ifc);
            }
        } 

    } while(0);

    return (wStatus_t)handle;;
}

static void *_printer_status_thread(void *param)
{
	_status_handle_t *handle = (_status_handle_t*)param;
    handle->status_ifc->start(handle->status_ifc, handle->status_callback, handle->param);
	return (NULL);
}

static int _start_printer_status_thread(_status_handle_t *handle)
{
	sigset_t allsig, oldsig;
	int result = ERROR;

	if (handle == NULL)
		return (result);

	result = OK;
	sigfillset(&allsig);
#if CHECK_PTHREAD_SIGMASK_STATUS
	result = pthread_sigmask(SIG_SETMASK, &allsig, &oldsig);
#else /* else CHECK_PTHREAD_SIGMASK_STATUS */
	pthread_sigmask(SIG_SETMASK, &allsig, &oldsig);
#endif /* CHECK_PTHREAD_SIGMASK_STATUS */
	if (result == OK)
	{
		result = pthread_create(&handle->status_tid, 0, _printer_status_thread, handle);
		if ((result == ERROR) && (handle->status_tid != pthread_self()))
		{
#if USE_PTHREAD_CANCEL
			pthread_cancel(handle->status_tid);
#else /* else USE_PTHREAD_CANCEL */
			pthread_kill(handle->status_tid, SIGKILL);
#endif /* USE_PTHREAD_CANCEL */
			handle->status_tid = pthread_self();
		}
	}

	if (result == OK)
	{
		sched_yield();
#if CHECK_PTHREAD_SIGMASK_STATUS
		result = pthread_sigmask(SIG_SETMASK, &oldsig, 0);
#else /* else CHECK_PTHREAD_SIGMASK_STATUS */
		pthread_sigmask(SIG_SETMASK, &oldsig, 0);
#endif /* CHECK_PTHREAD_SIGMASK_STATUS */
	}

	return (result);
} /*  _start_status_thread  */


int wprintStatusMonitorStart(wStatus_t status_handle,
                              void (*status_callback)(const printer_state_dyn_t *new_status,
                                                      const printer_state_dyn_t *old_status,
                                                      void *param), 
                              void    *param)
{
    int result = ERROR;
    _status_handle_t *handle = (_status_handle_t*)status_handle;
    if (handle != NULL) {
        handle->status_callback = status_callback;
        handle->param = param;
        result = _start_printer_status_thread(handle);
    }
    return result;
}


void wprintStatusMonitorStop(wStatus_t status_handle)
{
    _status_handle_t *handle = (_status_handle_t*)status_handle;
    if (handle != NULL) {
        handle->status_ifc->stop(handle->status_ifc);
		pthread_join(handle->status_tid, 0);
        handle->status_ifc->destroy(handle->status_ifc);
        free(handle);
    }
}

const char* wprintGetApplicationID() {
    return _libraryID;
}

void wprintSetApplicationID(const char *id) {
    memset(_libraryID, 0, sizeof(_libraryID));
    if (id) { 
        strncpy(_libraryID, id, (sizeof(_libraryID) - 1));
    }
}
