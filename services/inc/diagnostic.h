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

#ifndef SERVICES_DIAGNOSTIC_H
#define SERVICES_DIAGNOSTIC_H

#include <stddef.h>
#include <stdint.h>
#include "spark_macros.h"

#ifndef DIAGNOSTIC_SKIP_PLATFORM
#include "platform_diagnostic.h"
#endif

typedef enum {
  CHECKPOINT_TYPE_INVALID = 0,
  CHECKPOINT_TYPE_INSTRUCTION_ADDRESS,
  CHECKPOINT_TYPE_TEXTUAL
} diagnostic_checkpoint_type_t;

typedef struct {
  uint8_t version;
  diagnostic_checkpoint_type_t type;
  void* instruction;
  const char* text;
} diagnostic_checkpoint_t;

typedef enum {
  DIAGNOSTIC_FLAG_NONE = 0x00,
  DIAGNOSTIC_FLAG_DUMP_STACKTRACES = 0x01,
  DIAGNOSTIC_FLAG_FREEZE = 0x02
} diagnostic_flag_t;


typedef int (*diagnostic_save_checkpoint_callback_t)(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved);

typedef struct {
  uint8_t version;
  void* obj;
  diagnostic_save_checkpoint_callback_t diagnostic_save_checkpoint;
} diagnostic_callbacks_t;

typedef enum {
  DIAGNOSTIC_ERROR_NONE = 0,
  DIAGNOSTIC_ERROR,
  DIAGNOSTIC_ERROR_NO_THREAD_ENTRY,
  DIAGNOSTIC_ERROR_FROZEN,
  DIAGNOSTIC_ERROR_NO_SPACE
} diagnostic_error_t;

#ifdef __cplusplus
extern "C" {
#endif

int diagnostic_set_callbacks(diagnostic_callbacks_t* cb, void* reserved);
int diagnostic_set_callbacks_(diagnostic_callbacks_t* cb, void* reserved);
int diagnostic_save_checkpoint(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved);
int diagnostic_save_checkpoint_(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved);
int diagnostic_save_checkpoint__(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved);
size_t diagnostic_dump_saved(char* buf, size_t bufSize);
size_t diagnostic_dump_current(char* buf, size_t bufSize);

#ifdef __cplusplus
}
#endif

#define DIAGNOSTIC_CHECKPOINT_FILE_NAME(X) (__builtin_strrchr(X, '/') ? __builtin_strrchr(X, '/') + 1 : X)

#if defined(DIAGNOSTIC_USE_CALLBACKS)
#define DIAGNOSTIC_SAVE_CHECKPOINT_FUNC diagnostic_save_checkpoint_
#elif defined(DIAGNOSTIC_USE_CALLBACKS2)
#define DIAGNOSTIC_SAVE_CHECKPOINT_FUNC diagnostic_save_checkpoint__
#else
#define DIAGNOSTIC_SAVE_CHECKPOINT_FUNC diagnostic_save_checkpoint
#endif

#if PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10

#define DIAGNOSTIC_CHECKPOINT_F(type, pc, text, flags)            \
  {                                                             \
    diagnostic_checkpoint_t chkpt = {0, CHECKPOINT_TYPE_##type, \
                                     (void*)pc, text};          \
    DIAGNOSTIC_SAVE_CHECKPOINT_FUNC(&chkpt, flags, NULL);       \
  }

#define DIAGNOSTIC_TEXT_CHECKPOINT_C(txt) DIAGNOSTIC_CHECKPOINT_F(TEXTUAL, platform_get_current_pc(), txt, DIAGNOSTIC_FLAG_NONE)
#define DIAGNOSTIC_TEXT_CHECKPOINT() DIAGNOSTIC_TEXT_CHECKPOINT_C(DIAGNOSTIC_CHECKPOINT_FILE_NAME(__FILE__ ":" stringify(__LINE__)))
#define DIAGNOSTIC_INSTRUCTION_CHECKPOINT(pc) DIAGNOSTIC_CHECKPOINT_F(INSTRUCTION_ADDRESS, pc, NULL, DIAGNOSTIC_FLAG_NONE)
#define DIAGNOSTIC_CHECKPOINT() DIAGNOSTIC_CHECKPOINT_F(INSTRUCTION_ADDRESS, platform_get_current_pc(), NULL, DIAGNOSTIC_FLAG_NONE)
#define DIAGNOSTIC_PANIC_CHECKPOINT() DIAGNOSTIC_CHECKPOINT_F(INSTRUCTION_ADDRESS, platform_get_current_pc(), NULL, DIAGNOSTIC_FLAG_DUMP_STACKTRACES | DIAGNOSTIC_FLAG_FREEZE)
#define DIAGNOSTIC_CRASH_CHECKPOINT(pc) DIAGNOSTIC_CHECKPOINT_F(INSTRUCTION_ADDRESS, pc, NULL, DIAGNOSTIC_FLAG_DUMP_STACKTRACES | DIAGNOSTIC_FLAG_FREEZE)
#define DIAGNOSTIC_UPDATE() DIAGNOSTIC_SAVE_CHECKPOINT_FUNC(NULL, DIAGNOSTIC_FLAG_DUMP_STACKTRACES, NULL)

#else

#define DIAGNOSTIC_TEXT_CHECKPOINT_C(txt)
#define DIAGNOSTIC_TEXT_CHECKPOINT()
#define DIAGNOSTIC_INSTRUCTION_CHECKPOINT(pc)
#define DIAGNOSTIC_CHECKPOINT()
#define DIAGNOSTIC_PANIC_CHECKPOINT()
#define DIAGNOSTIC_CRASH_CHECKPOINT(pc)
#define DIAGNOSTIC_UPDATE()

#endif /* PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10 */

#endif // SERVICES_DIAGNOSTIC_H
