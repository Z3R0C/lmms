/*
 * Tuner.h - estimate the pitch of an audio signal
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef TUNER_H
#define TUNER_H

#include <array>
#include <aubio/aubio.h>
#include <chrono>

#include "Effect.h"
#include "TunerControls.h"
#include "TunerNote.h"

class Tuner : public Effect
{
public:
	Tuner(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~Tuner() override;

	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;
	EffectControls* controls() override;

	void syncReferenceFrequency();

private:
	TunerControls m_tunerControls;
	TunerNote m_referenceNote;

	int m_aubioFramesCounter = 0;
	int m_aubioWindowSize = 8192;
	int m_aubioHopSize = 4096;

	aubio_pitch_t* m_aubioPitch;
	fvec_t* m_aubioInputBuffer;
	fvec_t* m_aubioOutputBuffer;

	std::chrono::time_point<std::chrono::system_clock> m_intervalStart;
	std::chrono::milliseconds m_interval;

	friend class TunerControls;
};

#endif