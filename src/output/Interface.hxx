/*
 * Copyright 2003-2017 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_AUDIO_OUTPUT_INTERFACE_HXX
#define MPD_AUDIO_OUTPUT_INTERFACE_HXX

#include <chrono>

struct AudioFormat;
struct Tag;

class AudioOutput {
	const unsigned flags;

	bool need_fully_defined_audio_format = false;

protected:
	static constexpr unsigned FLAG_ENABLE_DISABLE = 0x1;
	static constexpr unsigned FLAG_PAUSE = 0x2;

public:
	explicit AudioOutput(unsigned _flags):flags(_flags) {}
	virtual ~AudioOutput() = default;

	AudioOutput(const AudioOutput &) = delete;
	AudioOutput &operator=(const AudioOutput &) = delete;

	bool SupportsEnableDisable() const {
		return flags & FLAG_ENABLE_DISABLE;
	}

	bool SupportsPause() const {
		return flags & FLAG_PAUSE;
	}

	bool GetNeedFullyDefinedAudioFormat() const {
		return need_fully_defined_audio_format;
	}

	/**
	 * Plugins shall call this method if they require an
	 * "audio_format" setting which evaluates
	 * AudioFormat::IsFullyDefined().
	 */
	void NeedFullyDefinedAudioFormat() {
		need_fully_defined_audio_format = true;
	}

	/**
	 * Enable the device.  This may allocate resources, preparing
	 * for the device to be opened.
	 *
	 * Throws #std::runtime_error on error.
	 */
	virtual void Enable() {}

	/**
	 * Disables the device.  It is closed before this method is
	 * called.
	 */
	virtual void Disable() noexcept {}

	/**
	 * Really open the device.
	 *
	 * Throws #std::runtime_error on error.
	 *
	 * @param audio_format the audio format in which data is going
	 * to be delivered; may be modified by the plugin
	 */
	virtual void Open(AudioFormat &audio_format) = 0;

	/**
	 * Close the device.
	 */
	virtual void Close() noexcept = 0;

	/**
	 * Returns a positive number if the output thread shall further
	 * delay the next call to Play() or Pause(), which will happen
	 * until this function returns 0.  This should be implemented
	 * instead of doing a sleep inside the plugin, because this
	 * allows MPD to listen to commands meanwhile.
	 *
	 * @return the duration to wait
	 */
	virtual std::chrono::steady_clock::duration Delay() const noexcept {
		return std::chrono::steady_clock::duration::zero();
	}

	/**
	 * Display metadata for the next chunk.  Optional method,
	 * because not all devices can display metadata.
	 */
	virtual void SendTag(const Tag &) {}

	/**
	 * Play a chunk of audio data.
	 *
	 * Throws #std::runtime_error on error.
	 *
	 * @return the number of bytes played
	 */
	virtual size_t Play(const void *chunk, size_t size) = 0;

	/**
	 * Wait until the device has finished playing.
	 */
	virtual void Drain() {}

	/**
	 * Try to cancel data which may still be in the device's
	 * buffers.
	 */
	virtual void Cancel() noexcept {}

	/**
	 * Pause the device.  If supported, it may perform a special
	 * action, which keeps the device open, but does not play
	 * anything.  Output plugins like "shout" might want to play
	 * silence during pause, so their clients won't be
	 * disconnected.  Plugins which do not support pausing will
	 * simply be closed, and have to be reopened when unpaused.
	 *
	 * @return false on error (output will be closed by caller),
	 * true for continue to pause
	 */
	virtual bool Pause() noexcept {
		/* fail because this method is not implemented */
		return false;
	}
};

#endif
