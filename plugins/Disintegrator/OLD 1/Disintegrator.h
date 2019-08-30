/*
 * Disintegrator.h
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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


#ifndef DISINTEGRATOR_H
#define DISINTEGRATOR_H

#include "Effect.h"
#include "DisintegratorControls.h"
#include "ValueBuffer.h"

class DisintegratorEffect : public Effect
{
public:
	DisintegratorEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	virtual ~DisintegratorEffect();
	virtual bool processAudioBuffer( sampleFrame* buf, const fpp_t frames );

	virtual EffectControls* controls()
	{
		return &m_disintegratorControls;
	}

	inline float realfmod(float k, float n);
	inline float detuneWithOctaves(float pitchValue, float detuneValue);

	inline void calcLowpassFilter( sample_t &outSamp, sample_t inSamp, int which, float lpCutoff, float resonance, sample_rate_t Fs );
	inline void calcHighpassFilter( sample_t &outSamp, sample_t inSamp, int which, float lpCutoff, float resonance, sample_rate_t Fs );

	float filtLPX[2][3] = {0};// [filter number][samples back in time]
	float filtLPY[2][3] = {0};// [filter number][samples back in time]

	float filtHPX[2][3] = {0};// [filter number][samples back in time]
	float filtHPY[2][3] = {0};// [filter number][samples back in time]

private:
	DisintegratorControls m_disintegratorControls;

	std::vector<float> inBuf[2];
	int inBufLoc = 0;

	friend class DisintegratorControls;

} ;

#endif
