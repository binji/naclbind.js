/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NB_ONE_FILE
#include "queue.h"
#endif

#include <pthread.h>
#include <stdlib.h>

struct NB_Queue {
  struct PP_Var* data;
  pthread_mutex_t mutex;
  pthread_cond_t not_empty_cond;
  int capacity;
  int start;
  int end;
  int size;
};

struct NB_Queue* nb_queue_create(int max_size) {
  struct NB_Queue* queue = calloc(1, sizeof(struct NB_Queue));
  queue->data = calloc(max_size, sizeof(struct PP_Var));
  queue->capacity = max_size;

  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->not_empty_cond, NULL);

  return queue;
}

void nb_queue_destroy(struct NB_Queue* queue) {
  free(queue->data);
  free(queue);
}

static int nb_queue_isempty(struct NB_Queue* queue) {
  return queue->size == 0;
}

static int nb_queue_isfull(struct NB_Queue* queue) {
  return queue->size == queue->capacity;
}

int nb_queue_enqueue(struct NB_Queue* queue, struct PP_Var message) {
  pthread_mutex_lock(&queue->mutex);

  /* We shouldn't block the main thread waiting for the queue to not be full,
   * so just drop the message. */
  if (nb_queue_isfull(queue)) {
    pthread_mutex_unlock(&queue->mutex);
    return 0;
  }

  queue->data[queue->end] = message;
  queue->end = (queue->end + 1) % queue->capacity;
  queue->size++;

  pthread_cond_signal(&queue->not_empty_cond);

  pthread_mutex_unlock(&queue->mutex);

  return 1;
}

struct PP_Var nb_queue_dequeue(struct NB_Queue* queue) {
  struct PP_Var message;

  pthread_mutex_lock(&queue->mutex);

  while (nb_queue_isempty(queue)) {
    pthread_cond_wait(&queue->not_empty_cond, &queue->mutex);
  }

  message = queue->data[queue->start];
  queue->start = (queue->start + 1) % queue->capacity;
  queue->size--;

  pthread_mutex_unlock(&queue->mutex);

  return message;
}
