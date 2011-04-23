/**
 * @file    cldthread.h
 * @author  Sebastian Hollington
 * @version 0.0.1
 *
 * @section LICENSE
 *
 * Copyright (c) 2011, Sebastian Hollington
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   * The names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SEBASTIAN HOLLINGTON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *
 *
 * @section DESCRIPTION
 *
 * This header file delares the API used by libCloudThreads.
 *
 */

#pragma once

#include "cielID.h"
#include "cldvalue.h"

/* cldthread struct defn */

typedef cielID cldthread;


/* Initialization */

/*!
 *  Initializes the libCloudThread framework.
 *  @return A non-zero value if successful.
 */
int cldthread_init( void );


/* Managing cloud threads */

/*!
 *  Create a thread for execution.
 *
 *  @param[in] fptr       Function that becomes the entry point of the new thread.
 *                        The return value of this function becomes the value of the
 *                        thread output (unless the output has been opened as a stream).
 *  @param[in] arg        Argument that fptr is executed with.
 *
 *  @returns A value that represents the new cloud thread.
 */
#define cldthread_create( fptr, arg ) cldthread_smart_create( NULL, fptr, arg )


/*!
 *  Create a thread for execution.
 *
 *  @param[in] fptr       Function that becomes the entry point of the new thread.
 *                        The return value of this function becomes the value of the
 *                        thread output (unless the output has been opened as a stream).
 *  @param[in] arg        Argument that fptr is executed with.
 *
 *  @returns A value that represent the new cloud thread.
 */
cldthread *cldthread_posix_create( void *(*fptr)(void *), void *arg );


/*!
 *  Start and schedule a thread for execution under memoisation. The parameters supplied
 *  are combined to create a unqiue identifier for the execution.  If a thread with the
 *  same unique identifier has already been executed and produced an output then we won't
 *  bother to schedule the thread again, and return the previously computed result.
 *
 *  @param[in] group_id   A user-specified unique identifier that is used for memoisation.
 *  @param[in] fptr       Function that becomes the entry point of the new thread.
 *                        The return value of this function becomes the value of the
 *                        thread output (unless the output has been opened as a stream).
 *  @param[in] arg        Argument that fptr is executed with.
 *
 *  @returns A value that represent the new cloud thread.
 */
cldthread *cldthread_smart_create( char *group_id, cldvalue *(*fptr)(void *), void *arg );


/*!
 *  Retrieves a value that is used to represent the current cloud thread.
 *  @return A value that can be used to represent the current cloud thread.
 */
#define cldthread_current_thread() ((cldthread *)_ciel_get_task_id())


/*!
 *  Wait on the result of a thread.
 *
 *  @param[in] thread     A pointer to the thread concerned.
 *
 *  @returns A non-zero value if a result was successfully retrieved.
 */
#define cldthread_join( thread ) cldthread_joins( &(thread), 1 )


/*!
 *  Wait on the result of multiple threads.
 *
 *  @param[in] threads An array of pointers to the threads whose result we want to evaluate.
 *  @param[in] count The number of thread pointers in the array.
 *  @returns The number of thread values that were successfully retrieved.
 */
#define cldthread_joins( threads, count ) cielID_read_streams( threads, count )


/*!
 *  Stream output from the current thread.  The returned file descriptor will close
 *  automatically after returning from the entry point of the thread or can be closed
 *  earlier using cldthread_close_result_stream().  Any thread attempting to join this thread
 *  will resume as soon as possible and can read from the stream using
 *  cldthread_result_as_fd().
 *
 *  Note: As soon as the thread returns, the stream to any reading Cloud Threads will be
 *  instantly terminated and they won't be able to read any further from it.
 *
 *  @returns A file descriptor that we can stream thread output to.
 */
int cldthread_stream_result( void );


/*!
 *  Write data directly to a Cloud Thread's output.  The caller is responsible for
 *  closing the returned fd once they have finished with it.
 *
 *  @returns A file descriptor that thread output can be written to.
 */
int cldthread_write_result( void );


/*!
 *  Finalise a thread's output stream and close the concerned file descriptor.
 *
 *  @returns A non-zero value if the output was published (for the first time).
 */
#define cldthread_close_streaming_result( thread ) cielID_publish_stream( thread )


/*!
 *  Retrieve the result of a thread as a cloud value.
 *
 *  The caller is responsible for freeing this value once they have
 *  finished with it.
 *
 *  @param[in] thread   A pointer to the thread concerned.
 *
 *  @sa cldthread_result_as_ref(), cldthread_result_as_fd()
 *
 *  @returns A pointer to a cloud value.
 */
cldvalue *cldthread_result_as_cldvalue( cldthread *thread );


/*!
 *  Retrieve the result of a thread as a read-only file descriptor.
 *  This should be used to access data that is being streamed out
 *  by a thread that has called cldthread_open_result_as_stream().
 *
 *  The caller is responsible for closing this FD once they have
 *  finished with it.
 *
 *  @param[in] thread   A pointer to the thread concerned.
 *
 *  @sa cldthread_result_as_ref(), cldthread_result_as_cldvalue()
 *
 *  @returns A file descriptor that we can stream thread output to.
 */
#define cldthread_result_as_fd( thread ) cielID_read_stream( thread )


/*!
 *  Retrieve the result of a thread as a cloud reference.
 *
 *  The caller is responsible for freeing this reference once they have
 *  finished with it.
 *
 *  @param[in] thread   A pointer to the thread concerned.
 *
 *  @sa cldthread_result_as_cldvalue(), cldthread_result_as_fd()
 *
 *  @returns A pointer to a cloud reference.
 */
swref *cldthread_result_as_ref( cldthread *thread );


/*!
 *  Publish the global result for an application that is run in a cloud
 *  environment.  The function returns EXIT_SUCCESS for convenience.
 *
 *  Example:
 *
 *  @code
 *  int main(){
 *      ...
 *      return cldapp_exit( cldvalue_integer( 42 ) );
 *  }
 *  @endcode
 *
 *  @param[in] exit_value A pointer to the cloud value that will become the
 *                        application's output.
 *
 *  @sa cldthread_result_as_cldvalue(), cldthread_result_as_fd()
 *
 *  @returns EXIT_SUCCESS
 */
int cldapp_exit( cldvalue *exit_value );


/*!
 *  Free any memory associated with a thread pointer.  Note that this is a
 *  local change i.e. it doesn't perform any operations on the cloud.
 *
 *  @param[in] thread A pointer to the thread whose memory is to be freed.
 *
 *  @sa cldthread_create(), cldthread_smart_create(), cldthread_posix_create()
 */
#define cldthread_free( thread ) cielID_free( thread )

