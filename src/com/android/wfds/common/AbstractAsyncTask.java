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
package com.android.wfds.common;

import android.content.Context;
import android.os.AsyncTask;

import java.util.Collections;
import java.util.LinkedList;
import java.util.concurrent.ExecutionException;

public abstract class AbstractAsyncTask<Params, Progress, Result> extends AsyncTask<Params, Progress, Result> {

    /**
     * Task Completion interface
     * @param <Result> Expected result type
     */
    public interface AsyncTaskCompleteCallback<Result> {
    	/**
    	 * Informs client of task completion
    	 * <p>
    	 * @param task			pointer to whichever task is running (this).  Can be used to determine which task is actually done
    	 * @param result		resulting object.
    	 * @param wasCancelled	indicates if task was cancelled.
    	 */
        void onReceiveTaskResult(AbstractAsyncTask<?, ?, ?> task, Result result, boolean wasCancelled);
    }

    /**
     * Task progress interface
     * @param <Progress> Expected progress type
     */
    public interface AsyncTaskProgressCallback<Progress> {
        /**
         * Informs client of task progress
         * <p>
         * @param task			pointer to whichever task is running (this).  Can be used to determine which task is actually done
         * @param progress		progress object.
         * @param wasCancelled	indicates if task was cancelled.
         */
        void onReceiveTaskProgress(AbstractAsyncTask<?, ?, ?> task, LinkedList<Progress> progress, boolean wasCancelled);
    }

    private AsyncTaskCompleteCallback<Result> mTaskCompleteCallback = null;
    private AsyncTaskProgressCallback<Progress> mTaskProgressCallback = null;
    private LinkedList<Progress> mQueuedProgress= new LinkedList<Progress>();

    protected final Context mContext;
    /**
     * Constructor 
     * @param context
     */
    protected AbstractAsyncTask(Context context) {
        super();
        mContext = ((context != null) ? context.getApplicationContext() : null);
    }

    // make private to force derived classed to implement their own constructors
    @SuppressWarnings("unused")
	private AbstractAsyncTask() {
        super();
        mContext = null;
    }

    /**
     * Assign a task complete callback to a task
     * <p>
     * @param callback		Client provided callback
     * @return				Returns itself (the same object) for making multiple calls in a single statement
     */
    public final synchronized AbstractAsyncTask<Params, Progress, Result> attach(AsyncTaskCompleteCallback<Result> callback) {
        mTaskCompleteCallback = callback;
        AsyncTask.Status status = super.getStatus();
        if (status == Status.FINISHED) {
            try {
                publishResultToCallback(status, super.get());
            } catch (InterruptedException ignored) {
            } catch (ExecutionException ignored) {
            }
        }
        return this;
    }

    /**
     * Assign a progress callback to a task
     * @param callback      Client provided callback
     * @return              Returns itself (the same object) for making multiple calls in a single statement
     */
    public final synchronized AbstractAsyncTask<Params, Progress, Result> attach(AsyncTaskProgressCallback<Progress> callback) {
        mTaskProgressCallback = callback;
        publishProgressToCallback();
        return this;
    }

    /**
     * Assign task progress and completetion callbacks to a task
     * @param taskProgressCallback      Client provided task progress callback
     * @param taskCompleteCallback      Client provided task complete callback
     * @return                          Returns itself (the same object) for making multiple calls in a single statement
     */
    public final synchronized AbstractAsyncTask<Params, Progress, Result> attach(AsyncTaskProgressCallback<Progress> taskProgressCallback,
                                                                                 AsyncTaskCompleteCallback<Result> taskCompleteCallback) {
        attach(taskProgressCallback);
        attach(taskCompleteCallback);
        return this;
    }

    /**
     * Removes client callbacks.  (Client does this in onPause or onDestroy (eg during a rotation or when client is no longer interested in result)
     * <p>
     * @return 		Returns itself (the same object) for making multiple calls in a single statement
     */
    public final synchronized AbstractAsyncTask<Params, Progress, Result> detach() {
        mTaskProgressCallback = null;
        mTaskCompleteCallback = null;
        return this;
    }

    @Override
    protected void onProgressUpdate(Progress... values) {
        queueProgress(values);
    }

    /**
     * Runs on UI thread after doInBackground. Specified value is returned by doInBackground
     * 
     * @see {android.os.AsyncTask{@link #onPostExecute(Object)}
     */
    @Override
    protected void onPostExecute(Result result) {
        publishResultToCallback(Status.FINISHED, result);
    }

    /**
     * Runs on the UI thread after cancel(boolean) is invoked and doInBackground(Object[]) has finished.
     * <p>
     * * @see {android.os.AsyncTask{@link #onCancelled(Object)}
     */
    @Override
    protected void onCancelled(Result result) {
        publishResultToCallback(Status.FINISHED, result);
    }

    // pushes result to client when task is finished.
    private synchronized void publishResultToCallback(AsyncTask.Status status, Result result) {
        if ((mTaskCompleteCallback != null) && (status == Status.FINISHED)) {
            mTaskCompleteCallback.onReceiveTaskResult(this, result, isCancelled());
            detach(); // avoid sending the result multiple times
        }
    }

    // queue up all provided progress
    private synchronized void queueProgress(Progress... values) {
        if (values != null) {
            Collections.addAll(mQueuedProgress, values);
        }
        publishProgressToCallback();
    }

    // pushes progress to client
    private synchronized void publishProgressToCallback() {
        if ((mTaskProgressCallback != null) && !mQueuedProgress.isEmpty()) {
            mTaskProgressCallback.onReceiveTaskProgress(this, mQueuedProgress, isCancelled());
            mQueuedProgress.clear();
        }
    }
}
