/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERVICES_TRACER_SERVICE_H
#define SERVICES_TRACER_SERVICE_H

#include <stddef.h>
#include <stdint.h>
#include "spark_macros.h"

#ifndef TRACER_SKIP_PLATFORM
#include "platform_tracer.h"
#endif

typedef enum {
  CHECKPOINT_TYPE_INVALID = 0,
  CHECKPOINT_TYPE_INSTRUCTION_ADDRESS,
  CHECKPOINT_TYPE_TEXTUAL
} tracer_checkpoint_type_t;

typedef struct {
  uint8_t version;
  tracer_checkpoint_type_t type;
  void* instruction;
  const char* text;
} tracer_checkpoint_t;

typedef enum {
  TRACER_FLAG_NONE = 0x00,
  TRACER_FLAG_DUMP_STACKTRACES = 0x01,
  TRACER_FLAG_FREEZE = 0x02
} tracer_flag_t;


typedef int (*tracer_save_checkpoint_callback_t)(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved);

typedef struct {
  uint8_t version;
  void* obj;
  tracer_save_checkpoint_callback_t tracer_save_checkpoint;
} tracer_callbacks_t;

typedef enum {
  TRACER_ERROR_NONE = 0,
  TRACER_ERROR,
  TRACER_ERROR_NO_THREAD_ENTRY,
  TRACER_ERROR_FROZEN,
  TRACER_ERROR_NO_SPACE
} tracer_error_t;

#ifdef __cplusplus
extern "C" {
#endif

int tracer_set_callbacks(tracer_callbacks_t* cb, void* reserved);
int tracer_set_callbacks_(tracer_callbacks_t* cb, void* reserved);
int tracer_save_checkpoint(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved);
int tracer_save_checkpoint_(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved);
int tracer_save_checkpoint__(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved);
size_t tracer_dump_saved(char* buf, size_t bufSize);
size_t tracer_dump_current(char* buf, size_t bufSize);

#ifdef __cplusplus
}
#endif

#define TRACER_CHECKPOINT_FILE_NAME(X) (__builtin_strrchr(X, '/') ? __builtin_strrchr(X, '/') + 1 : X)

#if defined(TRACER_USE_CALLBACKS)
#define TRACER_SAVE_CHECKPOINT_FUNC tracer_save_checkpoint_
#elif defined(TRACER_USE_CALLBACKS2)
#define TRACER_SAVE_CHECKPOINT_FUNC tracer_save_checkpoint__
#else
#define TRACER_SAVE_CHECKPOINT_FUNC tracer_save_checkpoint
#endif

#if PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10

#define TRACER_CHECKPOINT_F(type, pc, text, flags)               \
  {                                                              \
    tracer_checkpoint_t chkpt = {0, CHECKPOINT_TYPE_##type,      \
                                (void*)pc, text};                \
    TRACER_SAVE_CHECKPOINT_FUNC(&chkpt, flags, NULL);            \
  }

#define TRACER_TEXT_CHECKPOINT_C(txt) TRACER_CHECKPOINT_F(TEXTUAL, platform_get_current_pc(), txt, TRACER_FLAG_NONE)
#define TRACER_TEXT_CHECKPOINT() TRACER_TEXT_CHECKPOINT_C(TRACER_CHECKPOINT_FILE_NAME(__FILE__ ":" stringify(__LINE__)))
#define TRACER_INSTRUCTION_CHECKPOINT(pc) TRACER_CHECKPOINT_F(INSTRUCTION_ADDRESS, pc, NULL, TRACER_FLAG_NONE)
#define TRACER_CHECKPOINT() TRACER_CHECKPOINT_F(INSTRUCTION_ADDRESS, platform_get_current_pc(), NULL, TRACER_FLAG_NONE)
#define TRACER_PANIC_CHECKPOINT() TRACER_CHECKPOINT_F(INSTRUCTION_ADDRESS, platform_get_current_pc(), NULL, TRACER_FLAG_DUMP_STACKTRACES | TRACER_FLAG_FREEZE)
#define TRACER_CRASH_CHECKPOINT(pc) TRACER_CHECKPOINT_F(INSTRUCTION_ADDRESS, pc, NULL, TRACER_FLAG_DUMP_STACKTRACES | TRACER_FLAG_FREEZE)
#define TRACER_UPDATE() TRACER_SAVE_CHECKPOINT_FUNC(NULL, TRACER_FLAG_DUMP_STACKTRACES, NULL)

#else

#define TRACER_TEXT_CHECKPOINT_C(txt)
#define TRACER_TEXT_CHECKPOINT()
#define TRACER_INSTRUCTION_CHECKPOINT(pc)
#define TRACER_CHECKPOINT()
#define TRACER_PANIC_CHECKPOINT()
#define TRACER_CRASH_CHECKPOINT(pc)
#define TRACER_UPDATE()

#endif /* PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10 */

#endif // SERVICES_TRACER_SERVICE_H
