/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NB_QUEUE_H_
#define NB_QUEUE_H_

#include <ppapi/c/pp_var.h>

void nb_queue_init(void);
int nb_queue_enqueue(struct PP_Var);
struct PP_Var nb_queue_dequeue(void);

#endif /* NB_QUEUE_H_ */
