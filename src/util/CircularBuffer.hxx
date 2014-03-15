/*
 * Copyright (C) 2014 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include "WritableBuffer.hxx"

#include <assert.h>
#include <stddef.h>

/**
 * A circular buffer.
 *
 * This class does not manage buffer memory.  It will not allocate or
 * free any memory, it only manages the contents of an existing
 * buffer given to the constructor.
 *
 * Everything between #head and #tail is valid data (may wrap around).
 * If both are equal, then the buffer is empty.  Due to this
 * implementation detail, the buffer is empty when #size-1 items are
 * stored; the last buffer cell cannot be used.
 */
template<typename T>
class CircularBuffer {
public:
	typedef WritableBuffer<T> Range;
	typedef typename Range::pointer_type pointer_type;
	typedef typename Range::size_type size_type;

protected:
	/**
	 * The next index to be read.
	 */
	size_type head;

	/**
	 * The next index to be written to.
	 */
	size_type tail;

	const size_type size;
	const pointer_type data;

public:
	constexpr CircularBuffer(pointer_type _data, size_type _size)
		:head(0), tail(0), size(_size), data(_data) {}

	CircularBuffer(const CircularBuffer &other) = delete;

protected:
	constexpr size_type Next(size_type i) const {
		return i + 1 == size
			? 0
			: i + 1;
	}

public:
	void Clear() {
		head = tail = 0;
	}

	size_type GetSize() const {
		return size;
	}

	constexpr bool IsEmpty() const {
		return head == tail;
	}

	constexpr bool IsFull() const {
		return Next(tail) == head;
	}

	/**
	 * Prepares writing.  Returns a buffer range which may be written.
	 * When you are finished, call Append().
	 */
	Range Write() {
		assert(head < size);
		assert(tail < size);

		size_type end = tail < head
			? head - 1
			/* the "head==0" is there so we don't write
			   the last cell, as this situation cannot be
			   represented by head/tail */
			: size - (head == 0);

		return Range(data + tail, end - tail);
	}

	/**
	 * Expands the tail of the buffer, after data has been written
	 * to the buffer returned by Write().
	 */
	void Append(size_type n) {
		assert(head < size);
		assert(tail < size);
		assert(n < size);
		assert(tail + n <= size);
		assert(head <= tail || tail + n < head);

		tail += n;

		if (tail == size) {
			assert(head > 0);
			tail = 0;
		}
	}

	/**
	 * Return a buffer range which may be read.  The buffer pointer is
	 * writable, to allow modifications while parsing.
	 */
	Range Read() {
		assert(head < size);
		assert(tail < size);

		return Range(data + head, (tail < head ? size : tail) - head);
	}

	/**
	 * Marks a chunk as consumed.
	 */
	void Consume(size_type n) {
		assert(head < size);
		assert(tail < size);
		assert(n < size);
		assert(head + n <= size);
		assert(tail < head || head + n <= tail);

		head += n;
		if (head == size)
			head = 0;
	}
};

#endif
