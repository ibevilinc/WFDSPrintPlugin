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
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "ippstatus_monitor.h"
#include "ipphelper.h"
#include "wtypes.h"

#include "config.h"
#include "cups.h"
#include "http-private.h"
#include "http.h"
#include "ipp.h"
#include <pthread.h>
#include "wprint_debug.h"

#define HTTP_RESOURCE "/ipp/printer"

static void _init(const ifc_status_monitor_t *this_p,
		const char *printer_address);
static void _get_status(const ifc_status_monitor_t *this_p,
		printer_state_dyn_t *printer_state_dyn);
static void _start(const ifc_status_monitor_t *this_p,
		void (*status_cb)(const printer_state_dyn_t *new_status,
				const printer_state_dyn_t *old_status, void *param),
		void *param);
static void _stop(const ifc_status_monitor_t *this_p);
static int _cancel(const ifc_status_monitor_t *this_p);
static void _resume(const ifc_status_monitor_t *this_p);
static void _destroy(const ifc_status_monitor_t *this_p);

static const ifc_status_monitor_t _status_ifc =
		{ .init = _init, .get_status = _get_status, .cancel = _cancel, .resume =
				_resume, .start = _start, .stop = _stop, .destroy = _destroy, };

typedef struct
{
	unsigned char initialized;
	http_t *http;
	char printer_uri[1024];
	unsigned char stop_monitor;
	unsigned char monitor_running;
	sem_t monitor_sem;
	pthread_mutex_t mutex;
	pthread_mutexattr_t mutexattr;
	ifc_status_monitor_t ifc;
} ipp_monitor_t;

const ifc_status_monitor_t* ipp_status_get_monitor_ifc(
		const ifc_wprint_t *wprint_ifc)
{
	ipp_wprint_ifc = wprint_ifc;
	ipp_monitor_t *monitor = (ipp_monitor_t*) malloc(sizeof(ipp_monitor_t));

	/* setup the ifc */
	monitor->initialized = 0;
	monitor->http = NULL;
	memcpy(&monitor->ifc, &_status_ifc, sizeof(ifc_status_monitor_t));

	return (&monitor->ifc);
}

static void _init(const ifc_status_monitor_t *this_p,
		const char *printer_address)
{
	ipp_monitor_t *monitor;
	ifprint((DBG_VERBOSE, "ippstatus_monitor: _init(): enter"));
	do
	{
		if (this_p == NULL)
			continue;
		monitor = IMPL(ipp_monitor_t, ifc, this_p);

		if (monitor->initialized != 0)
		{
			sem_post(&monitor->monitor_sem);
			sem_destroy(&monitor->monitor_sem);

			pthread_mutex_unlock(&monitor->mutex);
			pthread_mutex_destroy(&monitor->mutex);
		}

		if (monitor->http != NULL)
			httpClose(monitor->http);

		monitor->http = httpConnectEncrypt(printer_address, ippPort(),
				cupsEncryption());
        httpSetTimeout(monitor->http, DEFAULT_IPP_TIMEOUT, NULL, 0);
		httpAssembleURIf(HTTP_URI_CODING_ALL, monitor->printer_uri,
				sizeof(monitor->printer_uri), "ipp", NULL, printer_address,
				ippPort(), "/ipp/printer");

		/* initialize varialbes */
		monitor->monitor_running = 0;
		monitor->stop_monitor = 0;

		pthread_mutexattr_init(&monitor->mutexattr);
		pthread_mutexattr_settype(&(monitor->mutexattr),
#if __APPLE__
				PTHREAD_MUTEX_RECURSIVE
#else /* assume linux for now */
				PTHREAD_MUTEX_RECURSIVE_NP
#endif
				);
		pthread_mutex_init(&monitor->mutex, &monitor->mutexattr);

		sem_init(&monitor->monitor_sem, 0, 0);

		monitor->initialized = 1;

	} while (0);
}

static void _destroy(const ifc_status_monitor_t *this_p)
{
	ipp_monitor_t *monitor;
	ifprint((DBG_VERBOSE, "ippstatus_monitor: _destroy(): enter"));
	do
	{
		if (this_p == NULL)
			continue;

		monitor = IMPL(ipp_monitor_t, ifc, this_p);

		if (monitor->initialized)
		{
			pthread_mutex_lock(&monitor->mutex);

			sem_post(&monitor->monitor_sem);
			sem_destroy(&monitor->monitor_sem);

			pthread_mutex_unlock(&monitor->mutex);
			pthread_mutex_destroy(&monitor->mutex);
		}

		if (monitor->http != NULL)
			httpClose(monitor->http);

		free(monitor);
	} while (0);
}

static void _get_status(const ifc_status_monitor_t *this_p,
		printer_state_dyn_t *printer_state_dyn)
{
	int i;
	ipp_monitor_t *monitor;
	ipp_pstate_t printer_state; /* Printer state */
	ipp_status_t ipp_status; /* Status of IPP request */
	ifprint((DBG_VERBOSE, "ippstatus_monitor: _get_status(): enter"));
	do
	{
		if (printer_state_dyn == NULL)
		{
			ifprint(
					(DBG_VERBOSE, "ippstatus_monitor: _get_status(): printer_state_dyn is null!" ));
			continue;
		}

		printer_state_dyn->printer_status = PRINT_STATUS_UNKNOWN;
		printer_state_dyn->printer_reasons[0] = PRINT_STATUS_UNKNOWN;
		for (i = 0; i <= PRINT_STATUS_MAX_STATE; i++)
			printer_state_dyn->printer_reasons[i] = PRINT_STATUS_MAX_STATE;

		if (this_p == NULL)
		{
			ifprint(
					(DBG_ERROR, "ippstatus_monitor: _get_status(): this_p is null!" ));
			continue;
		}

		monitor = IMPL(ipp_monitor_t, ifc, this_p);
		if (!monitor->initialized)
		{
			ifprint(
					(DBG_ERROR, "ippstatus_monitor: _get_status(): Monitor is uninitialized" ));
			continue;
		}

		if (monitor->http == NULL)
		{
			ifprint(
					(DBG_ERROR, "ippstatus_monitor: _get_status(): monitor->http is NULL, setting Unable to Connect" ));
			printer_state_dyn->printer_reasons[0] =
					PRINT_STATUS_UNABLE_TO_CONNECT;
			continue;
		}

		ifprint(
				(DBG_VERBOSE, "ippstatus_monitor: _get_status(): Setting Printer_Status to IDLE, getting Printer State" ));
		printer_state_dyn->printer_status = PRINT_STATUS_IDLE;
		ipp_status = get_PrinterState(monitor->http, monitor->printer_uri,
				printer_state_dyn, &printer_state);
		ifprint(
				(DBG_VERBOSE, "ippstatus_monitor: _get_status(): ipp_status=%d", ipp_status ));
		debuglist_printerStatus(printer_state_dyn);

	} while (0);
}

static void _start(const ifc_status_monitor_t *this_p,
		void (*status_cb)(const printer_state_dyn_t *new_status,
				const printer_state_dyn_t *old_status, void *param),
		void *param)
{
	int i;
	printer_state_dyn_t last_status, curr_status;
	ipp_monitor_t *monitor = NULL;

	ifprint((DBG_VERBOSE, "ippstatus_monitor: _start(): enter"));

	/* initialize our status structures */
	for (i = 0; i <= PRINT_STATUS_MAX_STATE; i++)
	{
		curr_status.printer_reasons[i] = PRINT_STATUS_MAX_STATE;
		last_status.printer_reasons[i] = PRINT_STATUS_MAX_STATE;
	}

	last_status.printer_status = PRINT_STATUS_UNKNOWN;
	last_status.printer_reasons[0] = PRINT_STATUS_INITIALIZING;

	curr_status.printer_status = PRINT_STATUS_UNKNOWN;
	curr_status.printer_reasons[0] = PRINT_STATUS_INITIALIZING;

	/* send out the first callback */
	if (status_cb != NULL)
		(*status_cb)(&curr_status, &last_status, param);

	do
	{
		curr_status.printer_status = PRINT_STATUS_SVC_REQUEST;
		curr_status.printer_reasons[0] = PRINT_STATUS_UNABLE_TO_CONNECT;

		if (this_p == NULL)
			continue;

		monitor = IMPL(ipp_monitor_t, ifc, this_p);
		if (!monitor->initialized)
			continue;

		if (monitor->monitor_running)
			continue;

		monitor->stop_monitor = 0;
		monitor->monitor_running = 1;

		if (monitor->http == NULL)
		{
			if (status_cb != NULL)
				(*status_cb)(&curr_status, &last_status, param);
			sem_wait(&monitor->monitor_sem);

			last_status.printer_status = PRINT_STATUS_UNKNOWN;
			last_status.printer_reasons[0] = PRINT_STATUS_SHUTTING_DOWN;

			curr_status.printer_status = PRINT_STATUS_UNKNOWN;
			curr_status.printer_reasons[0] = PRINT_STATUS_SHUTTING_DOWN;
		} else
		{
			while (!monitor->stop_monitor)
			{
				pthread_mutex_lock(&monitor->mutex);
				_get_status(this_p, &curr_status);
				pthread_mutex_unlock(&monitor->mutex);
				if ((status_cb != NULL)
						&& (memcmp(&curr_status, &last_status,
								sizeof(printer_state_dyn_t)) != 0))
				{
					(*status_cb)(&curr_status, &last_status, param);
					memcpy(&last_status, &curr_status,
							sizeof(printer_state_dyn_t));
				}
				sleep(1);
			}
		}
		monitor->monitor_running = 0;

	} while (0);

	if (status_cb != NULL)
		(*status_cb)(&curr_status, &last_status, param);
}

static void _stop(const ifc_status_monitor_t *this_p)
{
	/*  request a stop and release the semaphore */
	ipp_monitor_t *monitor;
	ifprint((DBG_VERBOSE, "ippstatus_monitor: _stop(): enter"));
	do
	{
		if (this_p == NULL)
			continue;

		monitor = IMPL(ipp_monitor_t, ifc, this_p);
		if (!monitor->initialized)
			continue;

		sem_post(&monitor->monitor_sem);
		monitor->stop_monitor = 1;

	} while (0);
}

static int _cancel(const ifc_status_monitor_t *this_p)
{
	int return_value = -1;
	int service_unavailable_retry_count = 0;
	int bad_request_retry_count = 0;
	int job_id = -1;
	ipp_monitor_t *monitor = NULL;
	ipp_t *request, *response;
	ipp_attribute_t *attr; /* Attribute pointer */

	ifprint((DBG_VERBOSE, "ippstatus_monitor: _cancel(): enter"));

	monitor = IMPL(ipp_monitor_t, ifc, this_p);

	if (this_p != NULL && monitor != NULL && monitor->initialized)
	{
		pthread_mutex_lock(&monitor->mutex);

		do
		{
			if (monitor->stop_monitor)
				break;

			if (job_id == -1)
			{
				request = ippNewRequest(IPP_GET_JOBS);
				if (request == NULL)
				{
					break;
				}

				if (set_ipp_version(request, monitor->printer_uri,
						monitor->http, ipp_version_has_been_resolved) != 0)
				{
					// 20120302 - LK - added ippDelete(request) to clean up allocated request before
					// returning (resource leak reported by Coverity)
					ippDelete(request);
					continue;
				}
				ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
						"printer-uri", NULL, monitor->printer_uri);
				ippAddBoolean(request, IPP_TAG_OPERATION, "my-jobs", 1);
				ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
						"requesting-user-name", NULL, cupsUser());

				static const char *pattrs[] =
				{ "job-id", "job-state", "job-state-reasons" }; // Requested printer attributes //

				ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
						"requested-attributes",
						sizeof(pattrs) / sizeof(pattrs[1]), NULL, pattrs);
				if ((response = cupsDoRequest(monitor->http, request,
						HTTP_RESOURCE)) == NULL)
				{
					ipp_status_t ipp_status = cupsLastError();
					ifprint(
							(DBG_VERBOSE, "  _cancel get job attributes:  get attributes response is null:  ipp_status %d %s",ipp_status, ippErrorString(ipp_status)));
					if (ipp_status == IPP_SERVICE_UNAVAILABLE)
					{
						ifprint(
								(DBG_ERROR, "1282 received, retrying %d of %d", service_unavailable_retry_count, IPP_SERVICE_UNAVAILBLE_MAX_RETRIES));

						service_unavailable_retry_count++;
						if (service_unavailable_retry_count
								== IPP_SERVICE_UNAVAILBLE_MAX_RETRIES)
							break;
						continue;
					}
					if (ipp_status == IPP_BAD_REQUEST)
					{
						ifprint(
								(DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST received. retry (%d) of (%d)", bad_request_retry_count, IPP_BAD_REQUEST_MAX_RETRIES));
						if (bad_request_retry_count
								> IPP_BAD_REQUEST_MAX_RETRIES)
							break;
						bad_request_retry_count++;
						continue;
					}
					return_value = -1;
				} else
				{
					ipp_status_t ipp_status = cupsLastError();
					ifprint(
							(DBG_VERBOSE, "_cancel get job attributes CUPS last ERROR: %d, %s", ipp_status, ippErrorString(ipp_status)));
					if (ipp_status == IPP_BAD_REQUEST)
					{
						ifprint(
								(DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST received. retry (%d) of (%d)", bad_request_retry_count, IPP_BAD_REQUEST_MAX_RETRIES));
						if (bad_request_retry_count
								> IPP_BAD_REQUEST_MAX_RETRIES)
							break;
						bad_request_retry_count++;
						ippDelete(response);
						continue;
					} else
					{

						attr = ippFindAttribute(response, "job-id",
								IPP_TAG_INTEGER);
						if (attr != NULL)
						{
							job_id = (int) ippGetInteger(attr, 0);
							ifprint(
									(DBG_VERBOSE, "_cancel got job-id: %d", job_id));
						} else
						{ // We need the job id to attempt a cancel
							break;
						}

						attr = ippFindAttribute(response, "job-state",
								IPP_TAG_ENUM);
						if (attr != NULL)
						{
							ipp_jstate_t jobState = ippGetInteger(attr, 0);
							ifprint(
									(DBG_VERBOSE, "_cancel got job-state: %d", jobState));
						}

						attr = ippFindAttribute(response, "job-state-reasons",
								IPP_TAG_KEYWORD);
						if (attr != NULL)
						{
							int idx;
							for (idx = 0; idx < ippGetCount(attr); idx++)
							{
								ifprint(
										(DBG_VERBOSE, "before job-state-reason (%d): %s", idx, ippGetString(attr, idx, NULL)));
							}
						}
						ippDelete(response);
					}
				}
			}
			break;
		} while (1);

		// Reset counts
		service_unavailable_retry_count = 0;
		bad_request_retry_count = 0;

		do
		{
			if (job_id == -1)
				break;

			request = ippNewRequest(IPP_CANCEL_JOB);
			if (request == NULL)
			{
				continue;
			}

			if (set_ipp_version(request, monitor->printer_uri, monitor->http,
					ipp_version_has_been_resolved) != 0)
			{
				// 20120302 - LK - added ippDelete(request) to clean up allocated request before
				// returning (resource leak reported by Coverity)
				ippDelete(request);
				continue;
			}
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri",
					NULL, monitor->printer_uri);
			ippAddInteger(request, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-id",
					job_id);
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
					"requesting-user-name", NULL, cupsUser());
			if ((response = cupsDoRequest(monitor->http, request, HTTP_RESOURCE))
					== NULL)
			{
				ipp_status_t ipp_status = cupsLastError();
				ifprint(
						(DBG_VERBOSE, "cancel:  response is null:  ipp_status %d %s",ipp_status, ippErrorString(ipp_status)));
				if (ipp_status == IPP_SERVICE_UNAVAILABLE)
				{
					ifprint(
							(DBG_ERROR, "1282 received, retrying %d of %d", service_unavailable_retry_count, IPP_SERVICE_UNAVAILBLE_MAX_RETRIES));
					service_unavailable_retry_count++;
					if (service_unavailable_retry_count
							== IPP_SERVICE_UNAVAILBLE_MAX_RETRIES)
						break;
					continue;
				}
				if (ipp_status == IPP_BAD_REQUEST)
				{
					ifprint(
							(DBG_ERROR, "IPP_Status of IPP_BAD_REQUEST received. retry (%d) of (%d)", bad_request_retry_count, IPP_BAD_REQUEST_MAX_RETRIES));
					if (bad_request_retry_count > IPP_BAD_REQUEST_MAX_RETRIES)
						break;
					bad_request_retry_count++;
					continue;
				}
				return_value = -1;
			} else
			{
				ipp_status_t ipp_status = cupsLastError();
				ifprint(
						(DBG_ERROR, "IPP_Status for cancel request was %d %s",ipp_status, ippErrorString(ipp_status)));
				attr = ippFindAttribute(response, "job-state-reasons",
						IPP_TAG_KEYWORD);
				if (attr != NULL)
				{
					int idx;
					for (idx = 0; ippGetCount(attr); idx++)
					{
						ifprint(
								(DBG_VERBOSE, "job-state-reason (%d): %s", idx, ippGetString(attr, idx, NULL)));
					}
				}
				ippDelete(response);
				return_value = 0;
			}
			break;
		} while (1);

		if ((monitor != NULL) && monitor->initialized)
			pthread_mutex_unlock(&monitor->mutex);
	}
	return (return_value);
}

static void _resume(const ifc_status_monitor_t *this_p)
{
	ipp_monitor_t *monitor;
	ifprint((DBG_VERBOSE, "ippstatus_monitor: _resume(): enter"));
	do
	{
		if (this_p == NULL)
			continue;

		monitor = IMPL(ipp_monitor_t, ifc, this_p);
		if (!monitor->initialized)
			continue;

	} while (0);
}
