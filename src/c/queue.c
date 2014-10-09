/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NB_ONE_FILE
#include "queue.h"
#endif

#include <pthread.h>
#include <stdlib.h>

#define NB_MAX_QUEUE_SIZE 256

/** A mutex that guards |s_nb_queue|. */
static pthread_mutex_t s_nb_queue_mutex;

/** A condition variable that is signalled when |s_nb_queue| is not empty. */
static pthread_cond_t s_nb_queue_not_empty_cond;

/** A circular queue of messages from JavaScript to be handled.
 *
 * If s_nb_queue_start < s_nb_queue_end:
 *   all elements in the range [s_nb_queue_start, s_nb_queue_end) are valid.
 * If s_nb_queue_start > s_nb_queue_end:
 *   all elements in the ranges [0, s_nb_queue_end) and
 *   [s_nb_queue_start, NB_MAX_QUEUE_SIZE) are valid.
 * If s_nb_queue_start == s_nb_queue_end, and s_nb_queue_size > 0:
 *   all elements in the s_nb_queue are valid.
 * If s_nb_queue_start == s_nb_queue_end, and s_nb_queue_size == 0:
 *   No elements are valid. */
static struct PP_Var s_nb_queue[NB_MAX_QUEUE_SIZE];

/** The index of the head of the queue. */
static int s_nb_queue_start = 0;

/** The index of the tail of the queue, non-inclusive. */
static int s_nb_queue_end = 0;

/** The size of the queue. */
static int s_nb_queue_size = 0;

/** Return whether the queue is empty.
 *
 * NOTE: this function assumes s_nb_queue_mutex lock is held.
 * @return non-zero if the queue is empty. */
static int nb_queue_isempty(void) {
  return s_nb_queue_size == 0;
}

/** Return whether the queue is full.
 *
 * NOTE: this function assumes s_nb_queue_mutex lock is held.
 * @return non-zero if the queue is full. */
static int nb_queue_isfull(void) {
  return s_nb_queue_size == NB_MAX_QUEUE_SIZE;
}

/** Initialize the message queue. */
void nb_queue_init(void) {
  pthread_mutex_init(&s_nb_queue_mutex, NULL);
  pthread_cond_init(&s_nb_queue_not_empty_cond, NULL);
}

/** Enqueue a message (i.e. add to the end)
 *
 * If the queue is full, the message will be dropped.
 *
 * NOTE: this function assumes s_nb_queue_mutex is _NOT_ held.
 * @param[in] message The message to enqueue.
 * @return non-zero if the message was added to the queue. */
int nb_queue_enqueue(struct PP_Var message) {
  pthread_mutex_lock(&s_nb_queue_mutex);

  /* We shouldn't block the main thread waiting for the queue to not be full,
   * so just drop the message. */
  if (nb_queue_isfull()) {
    pthread_mutex_unlock(&s_nb_queue_mutex);
    return 0;
  }

  s_nb_queue[s_nb_queue_end] = message;
  s_nb_queue_end = (s_nb_queue_end + 1) % NB_MAX_QUEUE_SIZE;
  s_nb_queue_size++;

  pthread_cond_signal(&s_nb_queue_not_empty_cond);

  pthread_mutex_unlock(&s_nb_queue_mutex);

  return 1;
}

/** Dequeue a message and return it.
 *
 * This function blocks until a message is available. It should not be called
 * on the main thread.
 *
 * NOTE: this function assumes s_nb_queue_mutex is _NOT_ held.
 * @return The message at the head of the queue. */
struct PP_Var nb_queue_dequeue(void) {
  struct PP_Var message;

  pthread_mutex_lock(&s_nb_queue_mutex);

  while (nb_queue_isempty()) {
    pthread_cond_wait(&s_nb_queue_not_empty_cond, &s_nb_queue_mutex);
  }

  message = s_nb_queue[s_nb_queue_start];
  s_nb_queue_start = (s_nb_queue_start + 1) % NB_MAX_QUEUE_SIZE;
  s_nb_queue_size--;

  pthread_mutex_unlock(&s_nb_queue_mutex);

  return message;
}
