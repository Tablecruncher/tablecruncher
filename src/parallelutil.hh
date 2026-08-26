/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2025 Stefan Fischerländer
 *
 * This file is part of Tablecruncher.
 *
 * Tablecruncher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * Tablecruncher is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tablecruncher. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _PARALLELUTIL_HH
#define _PARALLELUTIL_HH

//
//	The worker-thread policy, in one place.
//
//	Both the loader and the UTF-8 validator fan out; having them each carry their own copy of
//	"how many threads" and "what quality of service" is how the two drift — the validator's
//	threads originally missed the QoS bump the loader documents as essential.
//
//	No FLTK, no iostream.
//

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

#ifdef __APPLE__
	#include <pthread.h>
#endif


namespace Parallel {

// Beyond this, throughput stops improving and allocator contention starts growing.
const unsigned MAX_THREADS = 16;


/**
 *	How many workers to use. `hint` of 0 means "ask the machine".
 */
inline unsigned threadCount(unsigned hint = 0) {
	if( hint == 0 ) {
		hint = std::thread::hardware_concurrency();
		if( hint == 0 ) hint = 4;
	}
	return std::min(hint, MAX_THREADS);
}


/**
 *	Raises the calling thread's quality of service on Apple platforms.
 *
 *	Without this a worker can inherit UTILITY or BACKGROUND from whoever spawned it, which
 *	parks it on the efficiency cores — losing to the serial path outright. There is
 *	deliberately no attempt to detect P vs E cores: no portable API exists, and the scheduler
 *	already does the right thing for a foreground app once the QoS is honest.
 */
inline void raiseWorkerQos() {
#ifdef __APPLE__
	pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
}


/**
 *	Runs `body(i)` for i in [0, count) across at most `threads` workers, handing out indices
 *	with an atomic counter so a slow item — or a slow core — does not stall the rest.
 *
 *	Returns false if `cancel` was raised before every index had been processed, so that
 *	"did we actually finish?" is part of the mechanism rather than a check each caller has to
 *	remember to repeat.
 */
template <class Body>
inline bool parallelFor(size_t count, unsigned threads, std::atomic<bool>* cancel, Body body) {
	if( count == 0 )
		return true;

	// never spawn more workers than there is work for them
	threads = (unsigned) std::min<size_t>(threadCount(threads), count);

	if( threads <= 1 ) {
		for( size_t i = 0; i < count; ++i ) {
			if( cancel && cancel->load(std::memory_order_relaxed) ) return false;
			body(i);
		}
		return !( cancel && cancel->load(std::memory_order_relaxed) );
	}

	std::atomic<size_t> next{0};
	std::atomic<bool>   completed{true};
	// `spawned` gates the QoS bump: the caller also takes a share of the work, and the caller
	// may be the main UI thread (guessEncoding validates UTF-8 from there). Re-qualifying it
	// as USER_INITIATED would permanently LOWER the interactive thread's priority.
	auto run = [&](bool spawned) {
		if( spawned )
			raiseWorkerQos();
		for(;;) {
			size_t i = next.fetch_add(1, std::memory_order_relaxed);
			if( i >= count ) break;
			if( cancel && cancel->load(std::memory_order_relaxed) ) {
				completed.store(false, std::memory_order_relaxed);
				break;
			}
			body(i);
		}
	};

	std::vector<std::thread> pool;
	pool.reserve(threads - 1);
	for( unsigned t = 1; t < threads; ++t )
		pool.emplace_back(run, true);
	run(false);
	for( auto& t : pool )
		t.join();

	// Also check the flag itself, not just whether a worker happened to observe it at a
	// dispatch point: a cancel raised while every worker is inside its LAST body() call would
	// otherwise see them all exit through `i >= count` and report a clean finish — handing
	// back partly-parsed output as a success.
	if( cancel && cancel->load(std::memory_order_relaxed) )
		return false;
	return completed.load(std::memory_order_relaxed);
}

} // namespace Parallel

#endif
