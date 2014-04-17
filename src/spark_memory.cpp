/**
 ******************************************************************************
 * @file    spark_memory.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    25-March-2014
 * @brief   Spark custom memory management functions
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "spark_memory.h"

unsigned char *memory_buffer_start;
unsigned char *memory_buffer_end;
size_t memory_buffer_size;
size_t memory_buffer_used;

typedef struct spark_memory_block memory_block;
typedef struct spark_memory_block *memory_block_ptr;
size_t memory_block_count;

void spark_memory_init(unsigned char *buffer, size_t size)
{
	memory_buffer_start = buffer;
	memory_buffer_size = size;
	memory_buffer_end = memory_buffer_start + memory_buffer_size;
	memory_buffer_used = 0;
	memory_block_count = 0;
	memset(memory_buffer_start, 0, memory_buffer_size);
}

void *spark_memory_malloc(size_t size)
{
	memory_block_ptr block_ptr = (memory_block_ptr)memory_buffer_start;
	size_t sizeof_memory_block = sizeof(memory_block);
	int memory_block_flag = -1;

	if ((size + sizeof_memory_block) > (memory_buffer_size - (memory_buffer_used + memory_block_count * sizeof_memory_block)))
	{
		return NULL;
	}

	while (memory_buffer_end > ((unsigned char *)block_ptr + size + sizeof_memory_block))
	{
		if (!block_ptr->handle)
		{
			if (block_ptr->size == 0)
			{
				memory_block_flag = 0;	//new memory chunk
				break;
			}
			else if (block_ptr->size >= (size + sizeof_memory_block))
			{
				memory_block_flag = 1;	//reuse memory chunk
				break;
			}
		}

		block_ptr = (memory_block_ptr)((unsigned char *)block_ptr + block_ptr->size);
	}

	if (memory_block_flag != -1)
	{
		block_ptr->handle = 1;

		if (memory_block_flag)
		{
			size = block_ptr->size - sizeof_memory_block;
		}
		else
		{
			block_ptr->size = size + sizeof_memory_block;
		}

		memory_block_count++;
		memory_buffer_used += size;
		return ((unsigned char *)block_ptr + sizeof_memory_block);
	}

	return NULL;
}

void spark_memory_free(void *ptr)
{
	memory_block_ptr block_ptr = (memory_block_ptr)ptr;
	block_ptr--;

	if (block_ptr->handle)
	{
		memory_block_count--;
		block_ptr->handle = 0;
		memory_buffer_used -= (block_ptr->size - sizeof(memory_block));
	}
}
