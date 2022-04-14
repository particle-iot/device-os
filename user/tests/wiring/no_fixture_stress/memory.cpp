/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "random.h"

// platform supports out of memory notifiation

bool oomEventReceived = false;
size_t oomSizeReceived = 0;
void handle_oom(system_event_t event, int param, void*) {
	// Serial is not thread-safe
	// Serial.printlnf("got event %d %d", event, param);
	if (out_of_memory==event) {
		oomEventReceived = true;
		oomSizeReceived = param;
	}
};

void register_oom() {
	oomEventReceived = false;
	oomSizeReceived = 0;
	System.on(out_of_memory, handle_oom);
}

void unregister_oom() {
	System.off(out_of_memory, handle_oom);
}

test(SYSTEM_06_out_of_memory)
{
	// Disconnect from the cloud and network just in case
	Particle.disconnect();
	Network.disconnect();

	const size_t size = 1024*1024*1024;
	register_oom();
	auto ptr = malloc(size);
	(void)ptr;
	Particle.process();
	unregister_oom();

	assertTrue(oomEventReceived);
	assertEqual(oomSizeReceived, size);
}

test(SYSTEM_07_fragmented_heap) {
	struct Block {
		Block() {
			// Write garbage data to more easily corrupt the RAM
			// in case of issues like static RAM / heap overlap or
			// just simple heap corruption
			Random rng;
			rng.gen(data, sizeof(data));
			next = nullptr;
		}
		char data[508];
		Block* next;
	};
	register_oom();

	Block* next = nullptr;

	// exhaust memory
	for (;;) {
		Block* b = new Block();
		if (!b) {
			break;
		} else {
			b->next = next;
			next = b;
		}
	}

	assertTrue(oomEventReceived);
	assertEqual(oomSizeReceived, sizeof(Block));

	runtime_info_t info;
	info.size = sizeof(info);
	HAL_Core_Runtime_Info(&info, nullptr);

	// we can't really say about the free heap but the block size should be less
	assertLessOrEqual(info.largest_free_block_heap, sizeof(Block));
	size_t low_heap = info.freeheap;

	// free every 2nd block
	Block* head = next;
	int count = 0;
	for (;head;) {
		Block* free = head->next;
		if (free) {
			// skip the next block
			head->next = free->next;
			delete free;
			count++;
			head = head->next;
		} else {
			head = nullptr;
		}
	}

	HAL_Core_Runtime_Info(&info, nullptr);
	const size_t half_fragment_block_size = info.largest_free_block_heap;
	const size_t half_fragment_free = info.freeheap;

	unregister_oom();
	register_oom();
	const size_t BLOCKS_TO_MALLOC = 3;
	Block* b = new Block[BLOCKS_TO_MALLOC];  // no room for 3 blocks, memory is clearly fragmented
	delete[] b;

	// free the remaining blocks
	for (;next;) {
		Block* b = next;
		next = b->next;
		delete b;
	}

	assertMoreOrEqual(half_fragment_block_size, sizeof(Block)); // there should definitely be one block available
	assertLessOrEqual(half_fragment_block_size, BLOCKS_TO_MALLOC*sizeof(Block)-1); // we expect malloc of 3 blocks to fail, so this better allow up to that size less 1
	assertMoreOrEqual(half_fragment_free, low_heap+(sizeof(Block)*count));

	assertTrue(oomEventReceived);
	assertMoreOrEqual(oomSizeReceived, sizeof(Block)*BLOCKS_TO_MALLOC);
}

test(SYSTEM_08_out_of_memory_not_raised_for_0_size_malloc)
{
	const size_t size = 0;
	register_oom();
	auto ptr = malloc(size);
	(void)ptr;
	Particle.process();
	unregister_oom();

	assertFalse(oomEventReceived);
}

test(SYSTEM_09_out_of_memory_restore_state)
{
	// Restore connection to the cloud and network
	Network.connect();
	Particle.connect();
	waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
}
