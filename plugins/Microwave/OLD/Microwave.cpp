/*
 * Microwave.cpp - instrument which uses a usereditable wavetable
 *
 * Copyright (c) 2018 Robert Black AKA DouglasDGI <r94231/at/gmail/dot/com>
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



#include <QDomElement>

#include "Microwave.h"
#include "base64.h"
#include "Engine.h"
#include "Graph.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "templates.h"
#include "gui_templates.h"
#include "ToolTip.h"
#include "Song.h"
#include "interpolation.h"
#include "LcdSpinBox.h"
#include "lmms_math.h"
#include "SampleBuffer.h"
#include "FileDialog.h"
#include <QPainter>

#include "embed.h"

#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT microwave_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Microwave",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Unique wavetable synthesizer" ),
	"DouglasDGI",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}
	


bSynth::bSynth( float * _shape, int _length, NotePlayHandle * _nph, bool _interpolation,
				float _factor, const sample_rate_t _sample_rate,
				float phase, float phaseRand) :
	nph( _nph ),
	sample_length( _length ),
	sample_rate( _sample_rate ),
	interpolation( _interpolation)
{
	sample_shape = new float[sample_length];
	for( int i=0; i < _length; ++i )
	{
		sample_shape[i] = _shape[i] * _factor;
	}

	for( int i=0; i < 32; ++i )
	{
		// Randomize the phases of all of the waveforms
		sample_realindex[i] = int( ( ( fastRandf( sample_length ) - ( sample_length / 2.f ) ) * ( phaseRand / 100.f ) ) + ( sample_length / 2.f ) + phase ) % ( sample_length );
	}

	for( int i=0; i < 32; ++i )
	{
		sample_subindex[i] = 0;
	}

	noteDuration = -1;
}


bSynth::~bSynth()
{
	delete[] sample_shape;
}


std::vector<float> bSynth::nextStringSample( int modifyModeVal, float _modifyVal, float _morphVal, float rangeVal, float sprayVal, float unisonVoices, float unisonDetune, float unisonMorph, float unisonModify, float morphMaxVal, float modifyMaxVal, float detuneVal, float * waveforms, float * envs, int * envLens, bool * envEnableds, int * envSignals, float * lfos, int * lfoLens, bool * lfoEnableds, int * lfoSignals, float * seqs, int * seqLens, bool * seqEnableds, int * seqSignals, int * stepNums, float * subs, bool * subEnableds, bool noiseEnabled, float * noises )
{
	//=====================//
	//== MAIN OSCILLATOR ==//
	//=====================//

	noteDuration = noteDuration + 1;
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();
	float sample_morerealindex[32] = {0};
	sample_t sample[32] = {0};
	float sample_step[32] = {0};
	float resultsample[32] = {0};
	float sample_length_modify[32] = {0};
	float morphVal;
	float modifyVal;
	float curEnvVal;
	float curLfoVal;
	float curSeqVal;
	float curModVal;
	int curModSignal;
	for( int l = 0; l < 32; l++ )
	{
		curEnvVal = 0;
		curLfoVal = 0;
		curSeqVal = 0;
		curModVal = 0;
		if( envEnableds[l] && envLens[l] )
		{
			curEnvVal = envs[qBound( 0, (l*1024)+int((noteDuration+0.f)/envLens[l]*1024), (l+1)*1024-1)];// Finds Envelope position
		}
		if( lfoEnableds[l] && lfoLens[l] )
		{
			curLfoVal = lfos[((l*1024)+int((noteDuration+0.f)/lfoLens[l]*1024))%(1024)];// Finds LFO position
		}
		if( seqEnableds[l] && seqLens[l] )
		{
			curSeqVal = seqs[((l*64)+(int((noteDuration+0.f)/seqLens[l]*64))%(stepNums[l]))];// Finds Step Sequencer position
		}
		for( int m = 0; m < 3; m++ )
		{
			switch( m )
			{
				case 0: curModVal = curEnvVal; curModSignal = envSignals[l]; break;
				case 1: curModVal = curLfoVal; curModSignal = lfoSignals[l]; break;
				case 2: curModVal = curSeqVal; curModSignal = seqSignals[l]; break;
			}
			if( curModVal )
			{
				switch( curModSignal )// Changes the values controlled by the Envelopes, LFOs, and Step Sequencers
				{
					case 1:
						_morphVal = qBound( 0.f, _morphVal + round((curModVal+1)*morphMaxVal*0.5f), morphMaxVal );
						break;
					case 2:
						rangeVal = qBound( 1.f, rangeVal + round((curModVal+1)*16.f*0.5f), 16.f );
						break;
					case 3:
						_modifyVal = qBound( 0.f, _modifyVal + round((curModVal+1)*1024.f*0.5f), 1024.f );
						break;
				}
			}
		}
	}
	for( int l = 0; l < unisonVoices; l++ )
	{
		sample_morerealindex[l] = sample_realindex[l];

		float noteFreq = unisonVoices - 1 ? detuneWithCents( nph->frequency(), (fastRandf(unisonDetune*2)-unisonDetune)+detuneVal ) : detuneWithCents( nph->frequency(), detuneVal );// Calculates frequency depending on detune and unison detune

		sample_step[l] = static_cast<float>( sample_length / ( sample_rate / noteFreq ) );

		if( unisonVoices - 1 )// Figures out Morph and Modify values for individual unison voices
		{
			morphVal = (((unisonVoices-1)-l)/(unisonVoices-1))*unisonMorph;
			morphVal = qBound( 0.f, morphVal - ( unisonMorph / 2.f ) + _morphVal, morphMaxVal );

			modifyVal = (((unisonVoices-1)-l)/(unisonVoices-1))*unisonModify;
			modifyVal = qBound( 0.f, modifyVal - ( unisonModify / 2.f ) + _modifyVal, modifyMaxVal );
		}
		else
		{
			morphVal = _morphVal;
			modifyVal = _modifyVal;
		}

		sample_length_modify[l] = 0;
		switch( modifyModeVal )// Gorgeously horrifying formulas for the various Modify Modes
		{
			case 0:
				sample_morerealindex[l] = sample_morerealindex[l] / ( ( -modifyVal + sample_length ) / sample_length );
				break;
			case 1:
				if( sample_morerealindex[l] < ( sample_length + sample_length_modify[l] ) / 2.f )
				{
					sample_morerealindex[l] = sample_morerealindex[l] - (sample_morerealindex[l] * (modifyVal/( sample_length )));
				}
				break;
			case 2:
				if( sample_morerealindex[l] > ( sample_length + sample_length_modify[l] ) / 2.f )
				{
					sample_morerealindex[l] = sample_morerealindex[l] + (((sample_morerealindex[l])+sample_length/2.f) * (modifyVal/( sample_length )));
				}
				break;
			case 3:
				sample_morerealindex[l] = ( ( ( sin( ( ( ( sample_morerealindex[l] ) / ( sample_length ) ) * ( modifyVal / 50.f ) ) / 2 ) ) * ( sample_length ) ) * ( modifyVal / sample_length ) + ( sample_morerealindex[l] + ( ( -modifyVal + sample_length ) / sample_length ) ) ) / 2.f;
				break;
			case 4:
				sample_morerealindex[l] = pow( ( ( ( sample_morerealindex[l] ) / sample_length ) ), (modifyVal*10/sample_length)+1 ) * ( sample_length );
				break;
			case 5:
				sample_morerealindex[l] = -sample_morerealindex[l] + sample_length;
				sample_morerealindex[l] = pow( ( ( ( sample_morerealindex[l] ) / sample_length ) ), (modifyVal*10/sample_length)+1 ) * ( sample_length );
				sample_morerealindex[l] = -sample_morerealindex[l] + sample_length;
				break;
			case 6:
				if( sample_morerealindex[l] > sample_length / 2.f )
				{
					sample_morerealindex[l] = pow( ( ( ( sample_morerealindex[l] ) / sample_length ) ), (modifyVal*10/sample_length)+1 ) * ( sample_length );
				}
				else
				{
					sample_morerealindex[l] = -sample_morerealindex[l] + sample_length;
					sample_morerealindex[l] = pow( ( ( ( sample_morerealindex[l] ) / sample_length ) ), (modifyVal*10/sample_length)+1 ) * ( sample_length );
					sample_morerealindex[l] = sample_morerealindex[l] - sample_length / 2.f;
				}
				break;
			case 7:
				sample_morerealindex[l] = sample_morerealindex[l] - sample_length / 2.f;
				sample_morerealindex[l] = sample_morerealindex[l] / ( sample_length / 2.f );
				sample_morerealindex[l] = ( sample_morerealindex[l] >= 0 ) ? pow( sample_morerealindex[l], 1 / ( ( modifyVal * 4 ) / sample_length + 1 ) ) : -pow( -sample_morerealindex[l], 1 / ( ( modifyVal * 4 ) / sample_length + 1 ) );
				sample_morerealindex[l] = sample_morerealindex[l] * ( sample_length / 2.f );
				sample_morerealindex[l] = sample_morerealindex[l] + sample_length / 2.f;
				break;
			case 8:
				sample_morerealindex[l] = sample_morerealindex[l] - sample_length / 2.f;
				sample_morerealindex[l] = sample_morerealindex[l] / ( sample_length / 2.f );
				sample_morerealindex[l] = ( sample_morerealindex[l] >= 0 ) ? pow( sample_morerealindex[l], 1 / ( -modifyVal / sample_length + 1  ) ) : -pow( -sample_morerealindex[l], 1 / ( -modifyVal / sample_length + 1 ) );
				sample_morerealindex[l] = sample_morerealindex[l] * ( sample_length / 2.f );
				sample_morerealindex[l] = sample_morerealindex[l] + sample_length / 2.f;
				break;
			case 9:
				sample_morerealindex[l] = sample_morerealindex[l] - sample_length / 2.f;
				sample_morerealindex[l] = sample_morerealindex[l] / ( sample_length / 2.f );
				sample_morerealindex[l] = ( sample_morerealindex[l] >= 0 ) ? pow( sample_morerealindex[l], 1 / ( ( modifyVal * 4 ) / sample_length ) ) : -pow( -sample_morerealindex[l], 1 / ( ( modifyVal * 4 ) / sample_length ) );
				sample_morerealindex[l] = sample_morerealindex[l] * ( sample_length / 2.f );
				sample_morerealindex[l] = sample_morerealindex[l] + sample_length / 2.f;
				break;
			case 10:
				sample_length_modify[l] = modifyVal;
				dynamic_cast<Microwave *>(nph->instrumentTrack()->instrument())->setGraphLength( sample_length + sample_length_modify[l] );
				break;
			case 11:
				sample_morerealindex[l] = sample_morerealindex[l] * ( ( -modifyVal + sample_length ) / sample_length );
				break;
			case 12:
				sample_morerealindex[l] = -sample_morerealindex[l] + sample_length;
				sample_morerealindex[l] = sample_morerealindex[l] * ( ( -modifyVal + sample_length ) / sample_length );
				sample_morerealindex[l] = -sample_morerealindex[l] + sample_length;
				break;
			default:
				{}
		}
		sample_morerealindex[l] = sample_morerealindex[l] + indexOffset;// For Spray knob
		sample_morerealindex[l] = qBound( 0.f, sample_morerealindex[l], sample_length + sample_length_modify[l] - 1);// Keeps sample index within bounds
		
		resultsample[l] = 0;
		if( sample_morerealindex[l] <= sample_length + sample_length_modify[l] )
		{
			// Only grab samples from the waveforms when they will be used, depending on the Range knob
			for( int j = qBound(0.f, morphVal - rangeVal, 262144.f / sample_length ); j < qBound(0.f, morphVal + rangeVal, 262144.f / sample_length); j++ )
			{
				// Get waveform samples, set their volumes depending on Range knob
				resultsample[l] = resultsample[l] + ( waveforms[int(sample_morerealindex[l] + ( j * sample_length ) )] ) * qBound(0.0f, 1 - ( abs( morphVal - j ) / rangeVal ), 1.0f);
			}
		}
		resultsample[l] = resultsample[l] / ( ( rangeVal / 2.f ) + 3 );

		switch( modifyModeVal )// More gorgeously horrifying formulas for the various Modify Modes
		{
			case 0:
				if( sample_realindex[l] / ( ( -modifyVal + sample_length ) / sample_length ) > sample_length )
				{
					resultsample[l] = 0;
				}
				break;
			case 2:
				if( sample_realindex[l] + (((sample_realindex[l])+sample_length/2.f) * (modifyVal/( sample_length ))) > sample_length  )
				{
					resultsample[l] = 0;
				}
				break;
			case 10:
				if( sample_morerealindex[l] > sample_length )
				{
					resultsample[l] = 0;
				}
				break;
			case 13:
				if( sample_realindex[l] > modifyVal )
				{
					resultsample[l] = -resultsample[l];
				}
				break;
			default:
				{}
		}

		sample_realindex[l] = sample_realindex[l] + sample_step[l];

		// check overflow
		while ( sample_realindex[l] >= sample_length + sample_length_modify[l] ) {
			sample_realindex[l] = sample_realindex[l] - (sample_length + sample_length_modify[l]);
			indexOffset = fastRandf( int( sprayVal ) ) - int( sprayVal / 2 );// Reset offset caused by Spray knob
		}

		//sample[l] = ( abs( resultsample[l] ) < 256 ) ? resultsample[l] : 0;
		sample[l] = resultsample[l];
	}

	//====================//
	//== SUB OSCILLATOR ==//
	//====================//

	float sample_step_sub = 0;
	sample_t subsample[32] = {0};
	for( int l = 0; l < 32; l++ )
	{
		float noteFreq = nph->frequency();
		sample_step_sub = static_cast<float>( sample_length / ( sample_rate / noteFreq ) );

		subsample[l] = subs[int( sample_subindex[l] + ( 1024 * l ) )];// That's all that is needed for the sub oscillator calculations.  Happy CPU, no processing power to chew.

		sample_subindex[l] = sample_subindex[l] + sample_step_sub;

		// check overflow
		while ( sample_subindex[l] >= 1024 )
		{
			sample_subindex[l] = sample_subindex[l] - 1024;
		}
	}

	//======================//
	//== NOISE OSCILLATOR ==//
	//======================//

	float noiseSample[2] = {0};
	float noiseSampRand = 0;
	if( noiseEnabled )
	{
		noiseSampRand = fastRandf( 1024 ) - 1;
		noiseSample[0] = qBound( -1.f, ( lastNoiseSample[0]+(noises[int(noiseSampRand)] * ( -( fastRandf( 2 ) - 1 ) ) ) / 8.f ) / 1.05f, 1.f );// Division by 1.05f to keep samples near 0
		noiseSample[1] = qBound( -1.f, ( lastNoiseSample[1]+(noises[int(noiseSampRand)] * ( -( fastRandf( 2 ) - 1 ) ) ) / 8.f ) / 1.05f, 1.f );
		lastNoiseSample[0] = noiseSample[0];
		lastNoiseSample[1] = noiseSample[1];
	}

	std::vector<float> sampleAvg(2);
	if( unisonVoices > 1 )
	{
		for( int i = 0; i < unisonVoices; i++ )
		{
			// Pan unison voices
			sampleAvg[0] = sampleAvg[0] + sample[i] * (((unisonVoices-1)-i)/(unisonVoices-1));
			sampleAvg[1] = sampleAvg[1] + sample[i] * (i/(unisonVoices-1));
		}
		// Decrease volume so more unison voices won't increase volume too much
		sampleAvg[0] = sampleAvg[0] / ( unisonVoices / 2 );
		sampleAvg[1] = sampleAvg[1] / ( unisonVoices / 2 );
	}
	else
	{
		sampleAvg[0] = sample[0];
		sampleAvg[1] = sample[0];
	}
	for( int l = 0; l < 32; l++ )
	{
		if( subEnableds[l] )
		{
			sampleAvg[0] += subsample[l];
			sampleAvg[1] += subsample[l];
		}
	}
	if( noiseEnabled )
	{
		sampleAvg[0] += noiseSample[0];
		sampleAvg[1] += noiseSample[1];
	}
	return sampleAvg;
}


float bSynth::getRealIndex()
{
	return sample_realindex[0];
}


float bSynth::detuneWithCents( float pitchValue, float detuneValue )
{
	return pitchValue * pow( 2, detuneValue / 1200.f );
}


Microwave::Microwave( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &microwave_plugin_descriptor ),
	m_morph( 0, 0, 254, 0.0001f, this, tr( "Morph" ) ),
	m_range( 8, 1, 16, 0.0001f, this, tr( "Range" ) ),
	m_sampLen( 1024, 1, 8192, 1.f, this, tr( "Waveform Sample Length" ) ),
	m_visvol( 100, 0, 1000, 0.01f, this, tr( "Visualizer Volume" ) ),
	m_morphMax( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) ),
	m_waveNum(0, 0, 255, this, tr( "Waveform Number" ) ),
	m_modifyMode( this, tr( "Wavetable Modifier Mode" ) ),
	m_modify( 0, 0, 1024, 0.0001f, this, tr( "Wavetable Modifier Value" ) ),
	m_spray( 0, 0, 2048, 0.0001f, this, tr( "Spray" ) ),
	m_unisonVoices( 1, 1, 32, 1, this, tr( "Unison Voices" ) ),
	m_unisonDetune( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) ),
	m_unisonMorph( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) ),
	m_unisonModify( 0, 0, 1024, 0.0001f, this, tr( "Unison Modify" ) ),
	m_detune( 0, -9600, 9600, 1.f, this, tr( "Detune" ) ),
	m_envNum(1, 1, 32, this, tr( "Envelope Number" ) ),
	m_envLen( 44100, 0, 705600, 1, this, tr( "Envelope Length" ) ),
	m_envSignal( this, tr( "Envelope Signal Variable" ) ),
	m_lfoNum(1, 1, 32, this, tr( "LFO Number" ) ),
	m_lfoLen( 44100, 0, 705600, 1, this, tr( "LFO Length" ) ),
	m_lfoSignal( this, tr( "LFO Signal Variable" ) ),
	m_seqNum(1, 1, 32, this, tr( "Step Sequencer Number" ) ),
	m_seqLen( 44100, 0, 705600, 1, this, tr( "Step Sequencer Length" ) ),
	m_seqSignal( this, tr( "Step Sequencer Signal Variable" ) ),
	m_stepNum(8, 1, 64, this, tr( "Number Of Steps" ) ),
	m_loadAlg( 0, 0, 2, 1, this, tr( "Wavetable Loading Algorithm" ) ),
	m_loadChnl( 0, 0, 1, 1, this, tr( "Wavetable Loading Channel" ) ),
	m_phase( 0, 0, 200, 0.0001f, this, tr( "Phase" ) ),
	m_phaseRand( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) ),
	m_scroll( 1, 1, 6, 0.0001f, this, tr( "Scroll" ) ),
	m_subNum(1, 1, 32, this, tr( "Sub Oscillator Number" ) ),
	m_graph( -1.0f, 1.0f, 1024, this ),
	m_interpolation( false, this ),
	m_normalize( false, this ),
	m_visualize( false, this ),
	m_envEnabled( false, this ),
	m_lfoEnabled( false, this ),
	m_seqEnabled( false, this ),
	m_subEnabled( false, this ),
	m_noiseEnabled( false, this )
{

	m_unisonDetune.setScaleLogarithmic( true );
	m_envLen.setScaleLogarithmic( true );
	m_lfoLen.setScaleLogarithmic( true );
	m_seqLen.setScaleLogarithmic( true );
	m_spray.setScaleLogarithmic( true );
	
	setwavemodel( m_modifyMode )
	signalsmodel( m_envSignal )
	signalsmodel( m_lfoSignal )
	signalsmodel( m_seqSignal )

	m_graph.setWaveToSine();

	connect( &m_morphMax, SIGNAL( dataChanged( ) ),
			this, SLOT( morphMaxChanged( ) ) );

	connect( &m_graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );

	connect( &m_modifyMode, SIGNAL( dataChanged( ) ),
			this, SLOT( resetGraphLength( ) ) );

	connect( &m_envNum, SIGNAL( dataChanged( ) ),
			this, SLOT( envNumChanged( ) ) );

	connect( &m_envLen, SIGNAL( dataChanged( ) ),
			this, SLOT( envLenChanged( ) ) );

	connect( &m_envEnabled, SIGNAL( dataChanged( ) ),
			this, SLOT( envEnabledChanged( ) ) );

	connect( &m_envSignal, SIGNAL( dataChanged( ) ),
			this, SLOT( envSignalChanged( ) ) );

	connect( &m_lfoNum, SIGNAL( dataChanged( ) ),
			this, SLOT( lfoNumChanged( ) ) );

	connect( &m_lfoLen, SIGNAL( dataChanged( ) ),
			this, SLOT( lfoLenChanged( ) ) );

	connect( &m_lfoEnabled, SIGNAL( dataChanged( ) ),
			this, SLOT( lfoEnabledChanged( ) ) );

	connect( &m_lfoSignal, SIGNAL( dataChanged( ) ),
			this, SLOT( lfoSignalChanged( ) ) );

	connect( &m_seqNum, SIGNAL( dataChanged( ) ),
			this, SLOT( seqNumChanged( ) ) );

	connect( &m_seqLen, SIGNAL( dataChanged( ) ),
			this, SLOT( seqLenChanged( ) ) );

	connect( &m_seqEnabled, SIGNAL( dataChanged( ) ),
			this, SLOT( seqEnabledChanged( ) ) );

	connect( &m_seqSignal, SIGNAL( dataChanged( ) ),
			this, SLOT( seqSignalChanged( ) ) );

	connect( &m_stepNum, SIGNAL( dataChanged( ) ),
			this, SLOT( stepNumChanged( ) ) );
	
	connect( &m_subNum, SIGNAL( dataChanged( ) ),
			this, SLOT( subNumChanged( ) ) );

	connect( &m_subEnabled, SIGNAL( dataChanged( ) ),
			this, SLOT( subEnabledChanged( ) ) );

	connect( &m_sampLen, SIGNAL( dataChanged( ) ),
			this, SLOT( sampLenChanged( ) ) );

	// VVV Update visualizer VVV //

	connect( &m_range, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_sampLen, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_morph, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_modifyMode, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_modify, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_visvol, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_spray, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_unisonVoices, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_unisonDetune, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_unisonMorph, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_unisonModify, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

	connect( &m_detune, SIGNAL( dataChanged( ) ),
			this, SLOT( updateVisualizer( ) ) );

}




Microwave::~Microwave()
{
}




void Microwave::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.1" );

	m_morph.saveSettings( _doc, _this, "morph" );
	m_range.saveSettings( _doc, _this, "range" );
	m_sampLen.saveSettings( _doc, _this, "waveformsamplelength" );
	m_visvol.saveSettings( _doc, _this, "visualizervolume" );
	m_waveNum.saveSettings( _doc, _this, "waveformnumber" );
	m_spray.saveSettings( _doc, _this, "spray" );
	m_morphMax.saveSettings( _doc, _this, "morphmax" );
	m_modifyMode.saveSettings( _doc, _this, "modifymode" );
	m_modify.saveSettings( _doc, _this, "modify" );
	m_unisonVoices.saveSettings( _doc, _this, "unisonvoices" );
	m_unisonDetune.saveSettings( _doc, _this, "unisondetune" );
	m_unisonMorph.saveSettings( _doc, _this, "unisonmorph" );
	m_unisonModify.saveSettings( _doc, _this, "unisonmodify" );
	m_detune.saveSettings( _doc, _this, "detune" );
	m_loadAlg.saveSettings( _doc, _this, "loadingalgorithm" );
	m_loadChnl.saveSettings( _doc, _this, "loadingalgorithm" );
	m_phase.saveSettings( _doc, _this, "phase" );
	m_phaseRand.saveSettings( _doc, _this, "phaserand" );

	// Save arrays base64-encoded
	QString sampleString;
	base64::encode( (const char *)m_graph.samples(),
		m_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "sampleShape", sampleString );

	base64::encode( (const char *)waveforms,
		262144 * sizeof(float), sampleString );
	_this.setAttribute( "waveform", sampleString );

	base64::encode( (const char *)envs,
		32768 * sizeof(float), sampleString );
	_this.setAttribute( "envelopes", sampleString );

	base64::encode( (const char *)envLens,
		32 * sizeof(int), sampleString );
	_this.setAttribute( "envlens", sampleString );

	base64::encode( (const char *)envEnableds,
		32 * sizeof(bool), sampleString );
	_this.setAttribute( "envenableds", sampleString );

	base64::encode( (const char *)envSignals,
		32 * sizeof(int), sampleString );
	_this.setAttribute( "envsignals", sampleString );

	base64::encode( (const char *)lfos,
		32768 * sizeof(float), sampleString );
	_this.setAttribute( "lfos", sampleString );

	base64::encode( (const char *)lfoLens,
		32 * sizeof(int), sampleString );
	_this.setAttribute( "lfolens", sampleString );

	base64::encode( (const char *)lfoEnableds,
		32 * sizeof(bool), sampleString );
	_this.setAttribute( "lfoenableds", sampleString );

	base64::encode( (const char *)lfoSignals,
		32 * sizeof(int), sampleString );
	_this.setAttribute( "lfosignals", sampleString );

	base64::encode( (const char *)seqs,
		2048 * sizeof(float), sampleString );
	_this.setAttribute( "seqs", sampleString );

	base64::encode( (const char *)seqLens,
		32 * sizeof(int), sampleString );
	_this.setAttribute( "seqlens", sampleString );

	base64::encode( (const char *)seqEnableds,
		32 * sizeof(bool), sampleString );
	_this.setAttribute( "seqenableds", sampleString );

	base64::encode( (const char *)seqSignals,
		32 * sizeof(int), sampleString );
	_this.setAttribute( "seqsignals", sampleString );
	

	m_interpolation.saveSettings( _doc, _this, "interpolation" );
	
	m_normalize.saveSettings( _doc, _this, "normalize" );

	m_visualize.saveSettings( _doc, _this, "visualize" );
}




void Microwave::loadSettings( const QDomElement & _this )
{
	m_morph.loadSettings( _this, "morph" );
	m_range.loadSettings( _this, "range" );
	m_sampLen.loadSettings( _this, "waveformsamplelength" );
	m_visvol.loadSettings( _this, "visualizervolume" );
	m_waveNum.loadSettings( _this, "waveformnumber" );
	m_spray.loadSettings( _this, "spray" );
	m_morphMax.loadSettings( _this, "morphmax" );
	m_modifyMode.loadSettings( _this, "modifymode" );
	m_modify.loadSettings( _this, "modify" );
	m_unisonVoices.loadSettings( _this, "unisonvoices" );
	m_unisonDetune.loadSettings( _this, "unisondetune" );
	m_unisonMorph.loadSettings( _this, "unisonmorph" );
	m_unisonModify.loadSettings( _this, "unisonmodify" );
	m_detune.loadSettings( _this, "detune" );
	m_loadAlg.loadSettings( _this, "loadingAlgorithm" );
	m_loadChnl.loadSettings( _this, "loadingAlgorithm" );
	m_phase.loadSettings( _this, "phase" );
	m_phaseRand.loadSettings( _this, "phaserand" );

	m_graph.setLength( 1024 );

	// Load arrays

	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "waveform" ), &dst, &size );
	for( int i = 0; i < 262144; i++ )
	{
		waveforms[i] = ( (float*) dst )[i];
	}

	base64::decode( _this.attribute( "envelopes" ), &dst, &size );
	for( int i = 0; i < 32768; i++ )
	{
		envs[i] = ( (float*) dst )[i];
	}

	base64::decode( _this.attribute( "envlens" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		envLens[i] = ( (int*) dst )[i];
	}

	base64::decode( _this.attribute( "envenableds" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		envEnableds[i] = ( (bool*) dst )[i];
	}

	base64::decode( _this.attribute( "envsignals" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		envSignals[i] = ( (int*) dst )[i];
	}

	base64::decode( _this.attribute( "lfos" ), &dst, &size );
	for( int i = 0; i < 32768; i++ )
	{
		lfos[i] = ( (float*) dst )[i];
	}

	base64::decode( _this.attribute( "lfolens" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		lfoLens[i] = ( (int*) dst )[i];
	}

	base64::decode( _this.attribute( "lfoenableds" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		lfoEnableds[i] = ( (bool*) dst )[i];
	}

	base64::decode( _this.attribute( "lfosignals" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		lfoSignals[i] = ( (int*) dst )[i];
	}

	base64::decode( _this.attribute( "seqs" ), &dst, &size );
	for( int i = 0; i < 2048; i++ )
	{
		seqs[i] = ( (float*) dst )[i];
	}

	base64::decode( _this.attribute( "seqlens" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		seqLens[i] = ( (int*) dst )[i];
	}

	base64::decode( _this.attribute( "seqenableds" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		seqEnableds[i] = ( (bool*) dst )[i];
	}

	base64::decode( _this.attribute( "seqsignals" ), &dst, &size );
	for( int i = 0; i < 32; i++ )
	{
		seqSignals[i] = ( (int*) dst )[i];
	}
	delete[] dst;

	m_interpolation.loadSettings( _this, "interpolation" );
	
	m_normalize.loadSettings( _this, "normalize" );

	m_visualize.loadSettings( _this, "visualize" );

}


void MicrowaveView::updateScroll()
{
	
	float scrollVal = int( ( castModel<Microwave>()->m_scroll.value() - 1 ) * 250.f );
	
	m_morphKnob->move( 20 - scrollVal, 165 );
	m_rangeKnob->move( 50 - scrollVal, 165 );
	m_sampLenKnob->move( 50 - scrollVal, 185 );
	m_visvolKnob->move( 80 - scrollVal, 165 );
	m_morphMaxKnob->move( 20 - scrollVal, 195 );
	m_modifyKnob->move( 110 - scrollVal, 165 );
	m_sprayKnob->move( 140 - scrollVal, 165 );
	m_unisonVoicesKnob->move( 170 - scrollVal, 165 );
	m_unisonDetuneKnob->move( 200 - scrollVal, 165 );
	m_unisonMorphKnob->move( 170 - scrollVal, 195 );
	m_unisonModifyKnob->move( 200 - scrollVal, 195 );
	m_detuneKnob->move( 100 - scrollVal, 5 );
	m_envLenKnob->move( 50 + 500 - scrollVal, 165 );
	m_lfoLenKnob->move( 50 + 750 - scrollVal, 165 );
	m_seqLenKnob->move( 50 + 250 - scrollVal, 165 );
	m_loadAlgKnob->move( 110 - scrollVal, 195 );
	m_loadChnlKnob->move( 130 - scrollVal, 195 );
	m_phaseKnob->move( 50 - scrollVal, 215 );
	m_phaseRandKnob->move( 80 - scrollVal, 215 );
	m_interpolationToggle->move( 1310 - scrollVal, 207 );
	m_normalizeToggle->move( 1310 - scrollVal, 221 );
	m_visualizeToggle->move( 131 - scrollVal, 235 );
	m_envEnabledToggle->move( 131 + 500 - scrollVal, 235 );
	m_lfoEnabledToggle->move( 131 + 750 - scrollVal, 235 );
	m_seqEnabledToggle->move( 131 + 250 - scrollVal, 235 );
	m_subEnabledToggle->move( 131 + 1000 - scrollVal, 235 );
	m_noiseEnabledToggle->move( 131 + 1250 - scrollVal, 235 );
	waveNumBox->move( 0 + 10 - scrollVal, 220 );
	envNumBox->move( 500 + 10 - scrollVal, 220 );
	lfoNumBox->move( 750 + 10 - scrollVal, 220 );
	seqNumBox->move( 250 + 10 - scrollVal, 220 );
	subNumBox->move( 1000 + 10 - scrollVal, 220 );
	stepNumBox->move( 250 + 40 - scrollVal, 220 );
	m_envSignalBox->move( 0 + 500 - scrollVal, 10 );
	m_lfoSignalBox->move( 0 + 750 - scrollVal, 10 );
	m_seqSignalBox->move( 0 + 250 - scrollVal, 10 );
	tabChanged( castModel<Microwave>()->m_scroll.value() - 1 );
	updateGraphColor( castModel<Microwave>()->m_scroll.value() );
}

void MicrowaveView::scrollReleased()
{
	float scrollVal = castModel<Microwave>()->m_scroll.value();
	/*if( abs( scrollVal - round(scrollVal) ) < 0.2 )
	{
		castModel<Microwave>()->m_scroll.setValue( round(scrollVal) );
	}*/
	castModel<Microwave>()->m_scroll.setValue( round(scrollVal-0.25) );
}

void MicrowaveView::updateGraphColor( float scrollVal )
{
	if( scrollVal >= 1.5 && scrollVal <= 2 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("wavegraph");
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("stepgraph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-1.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else if( scrollVal >= 2.5 && scrollVal <= 3 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("stepgraph");
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("envgraph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-2.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else if( scrollVal >= 3.5 && scrollVal <= 4 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("envgraph");
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("lfograph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-3.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else if( scrollVal >= 4.5 && scrollVal <= 5 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("lfograph");
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("subgraph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-4.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else if( scrollVal >= 5.5 && scrollVal <= 6 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("subgraph");
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("noisegraph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-5.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else
	{
		switch( int( scrollVal ) )
		{
			case 1:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph") );
				break;
			case 2:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("stepgraph") );
				break;
			case 3:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("envgraph") );
				break;
			case 4:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("lfograph") );
				break;
			case 5:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("subgraph") );
				break;
			case 6:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("noisegraph") );
				break;
		}
	}
	m_graph->setPalette( pal );
}


void Microwave::morphMaxChanged()
{
	m_morph.setRange( m_morph.getMin(), m_morphMax.value(), m_morph.getStep() );
}


void Microwave::updateVisualizer()
{
	updateVis = m_sampLen.value();
}

void Microwave::sampLenChanged()
{
	updateVis = m_sampLen.value()*16;
	m_morphMax.setRange( m_morphMax.getMin(), 262144.f/m_sampLen.value() - 2, m_morphMax.getStep() );
	m_waveNum.setRange( m_waveNum.getMin(), int(262144.f/m_sampLen.value())+0.f, m_waveNum.getStep() );
	m_modify.setRange( m_modify.getMin(), m_sampLen.value() - 1, m_modify.getStep() );
}


void Microwave::envNumChanged()
{
	m_envLen.setValue( envLens[m_envNum.value()-1] );
	for( int i = 0; i < 1024; i++ )
	{
		m_graph.setSampleAt( i, envs[(m_envNum.value()-1)*1024+i] );
	}
	m_envEnabled.setValue( envEnableds[m_envNum.value()-1] );
	m_envSignal.setValue( envSignals[m_envNum.value()-1] );
}

void Microwave::envLenChanged()
{
	envLens[m_envNum.value()-1] = m_envLen.value();
}

void Microwave::envEnabledChanged()
{
	envEnableds[m_envNum.value()-1] = m_envEnabled.value();
}

void Microwave::envSignalChanged()
{
	envSignals[m_envNum.value()-1] = m_envSignal.value();
}


void Microwave::lfoNumChanged()
{
	m_lfoLen.setValue( lfoLens[m_lfoNum.value()-1] );
	for( int i = 0; i < 1024; i++ )
	{
		m_graph.setSampleAt( i, lfos[(m_lfoNum.value()-1)*1024+i] );
	}
	m_lfoEnabled.setValue( lfoEnableds[m_lfoNum.value()-1] );
	m_lfoSignal.setValue( lfoSignals[m_lfoNum.value()-1] );
}

void Microwave::lfoLenChanged()
{
	lfoLens[m_lfoNum.value()-1] = m_lfoLen.value();
}

void Microwave::lfoEnabledChanged()
{
	lfoEnableds[m_lfoNum.value()-1] = m_lfoEnabled.value();
}

void Microwave::lfoSignalChanged()
{
	lfoSignals[m_lfoNum.value()-1] = m_lfoSignal.value();
}


void Microwave::seqNumChanged()
{
	m_seqLen.setValue( seqLens[m_seqNum.value()-1] );
	for( int i = 0; i < 64; i++ )
	{
		m_graph.setSampleAt( i, seqs[(m_seqNum.value()-1)*64+i] );
	}
	m_seqEnabled.setValue( seqEnableds[m_seqNum.value()-1] );
	m_seqSignal.setValue( seqSignals[m_seqNum.value()-1] );
	m_graph.setLength( stepNums[m_seqNum.value()-1] );
	m_stepNum.setValue( stepNums[m_seqNum.value()-1] );
}

void Microwave::seqLenChanged()
{
	seqLens[m_seqNum.value()-1] = m_seqLen.value();
}

void Microwave::seqEnabledChanged()
{
	seqEnableds[m_seqNum.value()-1] = m_seqEnabled.value();
}

void Microwave::seqSignalChanged()
{
	seqSignals[m_seqNum.value()-1] = m_seqSignal.value();
}

void Microwave::stepNumChanged()
{
	m_graph.setLength( m_stepNum.value() );
	for( int i = 0; i < m_stepNum.value(); i++ )
	{
		m_graph.setSampleAt( i, seqs[(m_seqNum.value()-1)*64+i] );
	}
	stepNums[m_seqNum.value()-1] = m_stepNum.value();
}

void Microwave::subNumChanged()
{
	for( int i = 0; i < 1024; i++ )
	{
		m_graph.setSampleAt( i, subs[(m_subNum.value()-1)*1024+i] );
	}
	m_subEnabled.setValue( subEnableds[m_subNum.value()-1] );
}

void Microwave::subEnabledChanged()
{
	subEnableds[m_subNum.value()-1] = m_subEnabled.value();
}


void Microwave::samplesChanged( int _begin, int _end )
{
	normalize();
	//engine::getSongEditor()->setModified();
	switch( currentTab )
	{
		case 1:
			for( int i = _begin; i < _end; i++ )
			{
				seqs[i + ( (m_seqNum.value()-1) * 64 )] = m_graph.samples()[i];
			}
			break;
		case 2:
			for( int i = _begin; i < _end; i++ )
			{
				envs[i + ( (m_envNum.value()-1) * 1024 )] = m_graph.samples()[i];
			}
			break;
		case 3:
			for( int i = _begin; i < _end; i++ )
			{
				lfos[i + ( (m_lfoNum.value()-1) * 1024 )] = m_graph.samples()[i];
			}
			break;
		case 4:
			for( int i = _begin; i < _end; i++ )
			{
				subs[i + ( (m_subNum.value()-1) * 1024 )] = m_graph.samples()[i];
			}
			break;
		case 5:
			for( int i = _begin; i < _end; i++ )
			{
				noises[i] = m_graph.samples()[i];
			}
			break;
	}
}


void Microwave::resetGraphLength()
{
	m_graph.setLength( m_sampLen.value() );
}



void Microwave::normalize()
{
	// analyze
	float max = 0;
	const float* samples = m_graph.samples();
	for(int i=0; i < m_graph.length(); i++)
	{
		const float f = fabsf( samples[i] );
		if (f > max) { max = f; }
	}
	m_normalizeFactor = 1.0 / max;
}




QString Microwave::nodeName() const
{
	return( microwave_plugin_descriptor.name );
}




void Microwave::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	updateVisualizer();

	if ( _n->m_pluginData == NULL || _n->totalFramesPlayed() == 0 )
	{
	
		float factor;
		if( !m_normalize.value() )
		{
			factor = 1.0f;
		}
		else
		{
			factor = m_normalizeFactor;
		}
		_n->m_pluginData = new bSynth(
					const_cast<float*>( m_graph.samples() ),
					m_sampLen.value(),
					_n,
					m_interpolation.value(), factor,
				Engine::mixer()->processingSampleRate(),
					m_phase.value(), m_phaseRand.value() );

	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	bSynth * ps = static_cast<bSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		std::vector<float> sample = ps->nextStringSample( m_modifyMode.value(), m_modify.value(), m_morph.value(), m_range.value(), m_spray.value(), m_unisonVoices.value(), m_unisonDetune.value(), m_unisonMorph.value(), m_unisonModify.value(), m_morphMax.value(), m_modify.getMax(), m_detune.value(), waveforms, envs, envLens, envEnableds, envSignals, lfos, lfoLens, lfoEnableds, lfoSignals, seqs, seqLens, seqEnableds, seqSignals, stepNums, subs, subEnableds, m_noiseEnabled.value(), noises );
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = sample[chnl];
		}

		if( m_visualize.value() && updateVis ) //update visualizer
		{
			if( abs( const_cast<float*>( m_graph.samples() )[int(ps->getRealIndex())] - ( ((sample[0]+sample[1])/2.f) * m_visvol.value() / 100 ) ) >= 0.01f )
			{
				m_graph.setSampleAt( ps->getRealIndex(), ((sample[0]+sample[1])/2.f) * m_visvol.value() / 100 );
			}		
			updateVis--;
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void Microwave::deleteNotePluginData( NotePlayHandle * _n )
{
}




PluginView * Microwave::instantiateView( QWidget * _parent )
{
	return( new MicrowaveView( this, _parent ) );
}







MicrowaveView::MicrowaveView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );

	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	QWidget * view = new QWidget( _parent );
	view->setFixedSize( 250, 250 );

	m_morphKnob = new Knob( knobDark_28, this );
	m_morphKnob->move( 20, 165 );
	m_morphKnob->setHintText( tr( "Morph" ), "" );

	m_rangeKnob = new Knob( knobDark_28, this );
	m_rangeKnob->move( 50, 165 );
	m_rangeKnob->setHintText( tr( "Range" ), "" );

	m_sampLenKnob = new Knob( knobDark_28, this );
	m_sampLenKnob->move( 50, 185 );
	m_sampLenKnob->setHintText( tr( "Waveform Sample Length" ), "" );

	m_visvolKnob = new Knob( knobDark_28, this );
	m_visvolKnob->move( 80, 165 );
	m_visvolKnob->setHintText( tr( "Visualizer Volume" ), "" );

	m_morphMaxKnob = new Knob( knobDark_28, this );
	m_morphMaxKnob->move( 20, 195 );
	m_morphMaxKnob->setHintText( tr( "Morph Max" ), "" );

	m_modifyKnob = new Knob( knobDark_28, this );
	m_modifyKnob->move( 110, 165 );
	m_modifyKnob->setHintText( tr( "Modify" ), "" );

	m_sprayKnob = new Knob( knobDark_28, this );
	m_sprayKnob->move( 140, 165 );
	m_sprayKnob->setHintText( tr( "Spray" ), "" );

	m_unisonVoicesKnob = new Knob( knobDark_28, this );
	m_unisonVoicesKnob->move( 170, 165 );
	m_unisonVoicesKnob->setHintText( tr( "Unison Voices" ), "" );

	m_unisonDetuneKnob = new Knob( knobDark_28, this );
	m_unisonDetuneKnob->move( 200, 165 );
	m_unisonDetuneKnob->setHintText( tr( "Unison Detune" ), "" );

	m_unisonMorphKnob = new Knob( knobDark_28, this );
	m_unisonMorphKnob->move( 170, 195 );
	m_unisonMorphKnob->setHintText( tr( "Unison Morph" ), "" );

	m_unisonModifyKnob = new Knob( knobDark_28, this );
	m_unisonModifyKnob->move( 200, 195 );
	m_unisonModifyKnob->setHintText( tr( "Unison Modify" ), "" );

	m_detuneKnob = new Knob( knobSmall_17, this );
	m_detuneKnob->move( 100, 5 );
	m_detuneKnob->setHintText( tr( "Detune" ), "" );

	m_envLenKnob = new Knob( knobDark_28, this );
	m_envLenKnob->move( 50, 165 );
	m_envLenKnob->setHintText( tr( "Envelope Length" ), "" );

	m_lfoLenKnob = new Knob( knobDark_28, this );
	m_lfoLenKnob->move( 50, 165 );
	m_lfoLenKnob->setHintText( tr( "LFO Length" ), "" );

	m_seqLenKnob = new Knob( knobDark_28, this );
	m_seqLenKnob->move( 50, 165 );
	m_seqLenKnob->setHintText( tr( "Step Sequencer Length" ), "" );

	m_loadAlgKnob = new Knob( knobDark_28, this );
	m_loadAlgKnob->move( 110, 195 );
	m_loadAlgKnob->setHintText( tr( "Wavetable Loading Algorithm" ), "" );

	m_loadChnlKnob = new Knob( knobDark_28, this );
	m_loadChnlKnob->move( 130, 195 );
	m_loadChnlKnob->setHintText( tr( "Wavetable Loading Channel" ), "" );

	m_phaseKnob = new Knob( knobSmall_17, this );
	m_phaseKnob->move( 50, 215 );
	m_phaseKnob->setHintText( tr( "Phase" ), "" );

	m_phaseRandKnob = new Knob( knobSmall_17, this );
	m_phaseRandKnob->move( 80, 215 );
	m_phaseRandKnob->setHintText( tr( "Phase Randomness" ), "" );

	m_scrollKnob = new Knob( knobSmall_17, this );
	m_scrollKnob->move( 5, 135 );
	//m_scrollKnob->setDescription( "" );


	m_graph = new Graph( this, Graph::NearestStyle, 204, 134 );
	m_graph->move(23,23);	// 55,120 - 2px border
	m_graph->setAutoFillBackground( true );
	m_graph->setGraphColor( QColor( 255, 255, 255 ) );

	ToolTip::add( m_graph, tr ( "Draw your own waveform here "
				"by dragging your mouse on this graph."
	));


	QPixmap graphImg = PLUGIN_NAME::getIconPixmap("wavegraph");
	QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("envgraph");
	QPainter painter;
	painter.begin(&graphImg);
	painter.setOpacity(0.5);
	painter.drawPixmap(0, 0, graphImg2);
	painter.end();
	pal.setBrush( backgroundRole(), 
			graphImg );
	m_graph->setPalette( pal );

	const int WAVE_BUTTON_Y = 191;

	m_sinWaveBtn = new PixmapButton( this, tr( "Sine" ) );
	m_sinWaveBtn->move( 131, WAVE_BUTTON_Y );
	m_sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	m_sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	ToolTip::add( m_sinWaveBtn,
			tr( "Sine wave" ) );

	m_triangleWaveBtn = new PixmapButton( this, tr( "Nachos" ) );
	m_triangleWaveBtn->move( 131 + 14, WAVE_BUTTON_Y );
	m_triangleWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	m_triangleWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( m_triangleWaveBtn,
			tr( "Nacho wave" ) );

	m_sawWaveBtn = new PixmapButton( this, tr( "Sawsa" ) );
	m_sawWaveBtn->move( 131 + 14*2, WAVE_BUTTON_Y  );
	m_sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	m_sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	ToolTip::add( m_sawWaveBtn,
			tr( "Sawsa wave" ) );

	m_sqrWaveBtn = new PixmapButton( this, tr( "Sosig" ) );
	m_sqrWaveBtn->move( 131 + 14*3, WAVE_BUTTON_Y );
	m_sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	m_sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	ToolTip::add( m_sqrWaveBtn,
			tr( "Sosig wave" ) );

	m_whiteNoiseWaveBtn = new PixmapButton( this,
					tr( "Metal Fork" ) );
	m_whiteNoiseWaveBtn->move( 131 + 14*4, WAVE_BUTTON_Y );
	m_whiteNoiseWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	ToolTip::add( m_whiteNoiseWaveBtn,
			tr( "Metal Fork" ) );

	m_usrWaveBtn = new PixmapButton( this, tr( "Takeout" ) );
	m_usrWaveBtn->move( 131 + 14*5, WAVE_BUTTON_Y );
	m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	ToolTip::add( m_usrWaveBtn,
			tr( "Takeout Menu" ) );

	m_smoothBtn = new PixmapButton( this, tr( "Microwave Cover" ) );
	m_smoothBtn->move( 131 + 14*6, WAVE_BUTTON_Y );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smooth_active" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smooth_inactive" ) );
	ToolTip::add( m_smoothBtn,
			tr( "Microwave Cover" ) );

	m_applyBtn = new PixmapButton( this, tr( "Apply" ) );
	m_applyBtn->move( 131 + 14*-4, 191 );
	m_applyBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_applyBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	ToolTip::add( m_applyBtn,
			tr( "Apply Waveform" ) );

	m_tab0Btn = new PixmapButton( this, tr( "Main Tab" ) );
	m_tab0Btn->move( 131, 5 );
	m_tab0Btn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	m_tab0Btn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( m_tab0Btn,
			tr( "Main Tab" ) );


	m_interpolationToggle = new LedCheckBox( "Interpolation", this, tr( "Interpolation" ), LedCheckBox::Yellow );
	m_interpolationToggle->move( 131, 207 );


	m_normalizeToggle = new LedCheckBox( "Normalize", this, tr( "Normalize" ), LedCheckBox::Green );
	m_normalizeToggle->move( 131, 221 );

	m_visualizeToggle = new LedCheckBox( "Visualize", this, tr( "Visualize" ), LedCheckBox::Green );
	m_visualizeToggle->move( 131, 235 );

	m_envEnabledToggle = new LedCheckBox( "Envelope Enabled", this, tr( "Envelope Enabled" ), LedCheckBox::Green );
	m_envEnabledToggle->move( 131, 235 );

	m_lfoEnabledToggle = new LedCheckBox( "LFO Enabled", this, tr( "LFO Enabled" ), LedCheckBox::Green );
	m_lfoEnabledToggle->move( 131, 235 );

	m_seqEnabledToggle = new LedCheckBox( "Step Sequencer Enabled", this, tr( "Step Sequencer Enabled" ), LedCheckBox::Green );
	m_seqEnabledToggle->move( 131, 235 );

	m_subEnabledToggle = new LedCheckBox( "Sub Oscillator Enabled", this, tr( "Sub OScillator Enabled" ), LedCheckBox::Green );
	m_subEnabledToggle->move( 131, 235 );

	m_noiseEnabledToggle = new LedCheckBox( "Noise Oscillator Enabled", this, tr( "Noise Oscillator Enabled" ), LedCheckBox::Green );
	m_noiseEnabledToggle->move( 131, 235 );

	m_openWavetableButton = new PixmapButton( this );
	m_openWavetableButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openWavetableButton->move( 115, 176 );
	m_openWavetableButton->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
	m_openWavetableButton->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
	connect( m_openWavetableButton, SIGNAL( clicked() ),
					this, SLOT( openWavetableFileBtnClicked() ) );
	ToolTip::add( m_openWavetableButton, tr( "Open wavetable" ) );

	waveNumBox = new LcdSpinBox( 3, this, "Waveform Number" );
	waveNumBox->move( 10, 220 );

	envNumBox = new LcdSpinBox( 2, this, "Envelope Number" );
	envNumBox->move( 10, 220 );

	lfoNumBox = new LcdSpinBox( 2, this, "LFO Number" );
	lfoNumBox->move( 10, 220 );

	seqNumBox = new LcdSpinBox( 2, this, "Step Sequencer Number" );
	seqNumBox->move( 10, 220 );

	stepNumBox = new LcdSpinBox( 2, this, "Number Of Steps" );
	stepNumBox->move( 40, 220 );

	subNumBox = new LcdSpinBox( 2, this, "Envelope Number" );
	subNumBox->move( 10, 220 );

	m_modifyModeBox = new ComboBox( this );
	m_modifyModeBox->setGeometry( 0, 10, 42, 22 );
	m_modifyModeBox->setFont( pointSize<8>( m_modifyModeBox->font() ) );

	m_envSignalBox = new ComboBox( this );
	m_envSignalBox->setGeometry( 0, 10, 42, 22 );
	m_envSignalBox->setFont( pointSize<8>( m_envSignalBox->font() ) );

	m_lfoSignalBox = new ComboBox( this );
	m_lfoSignalBox->setGeometry( 0, 10, 42, 22 );
	m_lfoSignalBox->setFont( pointSize<8>( m_lfoSignalBox->font() ) );

	m_seqSignalBox = new ComboBox( this );
	m_seqSignalBox->setGeometry( 0, 10, 42, 22 );
	m_seqSignalBox->setFont( pointSize<8>( m_seqSignalBox->font() ) );
	
	
	
	connect( m_sinWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );
	connect( m_triangleWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked() ) );
	connect( m_sawWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );
	connect( m_sqrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );
	connect( m_whiteNoiseWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );
	connect( m_usrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );
	
	connect( m_smoothBtn, SIGNAL ( clicked () ),
			this, SLOT ( smoothClicked() ) );

	connect( m_applyBtn, SIGNAL ( clicked () ),
			this, SLOT ( applyClicked() ) );



	connect( m_interpolationToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( interpolationToggled( bool ) ) );

	connect( m_normalizeToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( normalizeToggled( bool ) ) );

	connect( m_visualizeToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( visualizeToggled( bool ) ) );



	connect( &castModel<Microwave>()->m_scroll, SIGNAL( dataChanged( ) ),
			this, SLOT( updateScroll( ) ) );

	connect( m_scrollKnob, SIGNAL( sliderReleased( ) ),
			this, SLOT( scrollReleased( ) ) );

}




void MicrowaveView::modelChanged()
{
	Microwave * b = castModel<Microwave>();

	m_graph->setModel( &b->m_graph );
	m_morphKnob->setModel( &b->m_morph );
	m_rangeKnob->setModel( &b->m_range );
	m_sampLenKnob->setModel( &b->m_sampLen );
	m_visvolKnob->setModel( &b->m_visvol );
	m_modifyKnob->setModel( &b->m_modify );
	m_morphMaxKnob->setModel( &b->m_morphMax );
	m_sprayKnob->setModel( &b->m_spray );
	m_unisonVoicesKnob->setModel( &b->m_unisonVoices );
	m_unisonDetuneKnob->setModel( &b->m_unisonDetune );
	m_unisonMorphKnob->setModel( &b->m_unisonMorph );
	m_unisonModifyKnob->setModel( &b->m_unisonModify );
	m_detuneKnob->setModel( &b->m_detune );
	m_interpolationToggle->setModel( &b->m_interpolation );
	m_normalizeToggle->setModel( &b->m_normalize );
	m_visualizeToggle->setModel( &b->m_visualize );
	m_envEnabledToggle->setModel( &b->m_envEnabled );
	waveNumBox->setModel( &b->m_waveNum );
	m_modifyModeBox-> setModel( &b-> m_modifyMode );
	envNumBox->setModel( &b->m_envNum );
	m_envLenKnob->setModel( &b->m_envLen );
	m_envSignalBox-> setModel( &b-> m_envSignal );
	m_lfoEnabledToggle->setModel( &b->m_lfoEnabled );
	lfoNumBox->setModel( &b->m_lfoNum );
	m_lfoLenKnob->setModel( &b->m_lfoLen );
	m_lfoSignalBox-> setModel( &b-> m_lfoSignal );
	m_seqEnabledToggle->setModel( &b->m_seqEnabled );
	seqNumBox->setModel( &b->m_seqNum );
	stepNumBox->setModel( &b->m_stepNum );
	m_seqLenKnob->setModel( &b->m_seqLen );
	m_seqSignalBox-> setModel( &b-> m_seqSignal );
	subNumBox->setModel( &b->m_subNum );
	m_subEnabledToggle->setModel( &b->m_subEnabled );
	m_noiseEnabledToggle->setModel( &b->m_noiseEnabled );
	m_loadAlgKnob->setModel( &b->m_loadAlg );
	m_loadChnlKnob->setModel( &b->m_loadChnl );
	m_phaseKnob->setModel( &b->m_phase );
	m_phaseRandKnob->setModel( &b->m_phaseRand );
	m_scrollKnob->setModel( &b->m_scroll );

}




void MicrowaveView::sinWaveClicked()
{
	m_graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}




void MicrowaveView::triangleWaveClicked()
{
	m_graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}




void MicrowaveView::sawWaveClicked()
{
	m_graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}




void MicrowaveView::sqrWaveClicked()
{
	m_graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}




void MicrowaveView::noiseWaveClicked()
{
	m_graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}




void MicrowaveView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	ToolTip::add( m_usrWaveBtn, fileName );
	Engine::getSong()->setModified();
}




void MicrowaveView::smoothClicked()
{
	m_graph->model()->smooth();
	Engine::getSong()->setModified();
}

void MicrowaveView::applyClicked( )
{
	switch( castModel<Microwave>()->currentTab )
	{
		case 0:
			castModel<Microwave>()->waveClicked( castModel<Microwave>()->m_waveNum.value() );
	}
}


void Microwave::waveClicked( int wavenum )
{
	for( int j = 0; j < m_sampLen.value(); j++ )
	{
		waveforms[int( ( 262144 / m_sampLen.value() ) * wavenum )] = m_graph.samples()[int( ( 262144 / m_sampLen.value() ) * wavenum )];
	}
}

void Microwave::setGraphLength( float graphLen )
{
	m_graph.setLength( graphLen );
}

float Microwave::getWaveformSample( int waveformIndex )
{
	return waveforms[waveformIndex];
}


void MicrowaveView::tabChanged( int tabnum )
{
	QPalette pal = QPalette();
	if( castModel<Microwave>()->currentTab != tabnum )
	{
		castModel<Microwave>()->currentTab = tabnum;			
		switch( tabnum )
		{
			case 0:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph") );
				m_graph->setPalette( pal );
				castModel<Microwave>()->m_graph.setLength( 1024 );
				break;
			case 1:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("stepgraph") );
				m_graph->setPalette( pal );
				castModel<Microwave>()->seqNumChanged();
				break;
			case 2:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("envgraph") );
				m_graph->setPalette( pal );
				castModel<Microwave>()->envNumChanged();
				castModel<Microwave>()->m_graph.setLength( 1024 );
				break;
			case 3:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("lfograph") );
				m_graph->setPalette( pal );
				castModel<Microwave>()->lfoNumChanged();
				castModel<Microwave>()->m_graph.setLength( 1024 );
				break;
			case 4:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("subgraph") );
				m_graph->setPalette( pal );
				castModel<Microwave>()->subNumChanged();
				castModel<Microwave>()->m_graph.setLength( 1024 );
				break;
			case 5:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("noisegraph") );
				m_graph->setPalette( pal );
				castModel<Microwave>()->m_graph.setLength( 1024 );
				castModel<Microwave>()->m_graph.setSamples( castModel<Microwave>()->noises );
				break;
		}
	}
}

void MicrowaveView::interpolationToggled( bool value )
{
	m_graph->setGraphStyle( value ? Graph::LinearStyle : Graph::NearestStyle);
	Engine::getSong()->setModified();
}




void MicrowaveView::normalizeToggled( bool value )
{
	Engine::getSong()->setModified();
}


void MicrowaveView::visualizeToggled( bool value )
{
}

void MicrowaveView::openWavetableFileBtnClicked( )
{
	openWavetableFile( castModel<Microwave>()->m_loadAlg.value(), castModel<Microwave>()->m_loadChnl.value() );
}

void MicrowaveView::openWavetableFile( int algorithm, int channel )
{
	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	int filelength = sampleBuffer->sampleLength();
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*44100.f);//in samples
		switch( algorithm )
		{
			case 0:
			{
				castModel<Microwave>()->m_morphMax.setValue( 262144.f/castModel<Microwave>()->m_sampLen.value() );
				castModel<Microwave>()->morphMaxChanged();
				for( int i = 0; i < 262144; i++ )
				{
					castModel<Microwave>()->waveforms[i] = sampleBuffer->userWaveSample( qBound( 0.f, i/262144.f, 1.f ), channel );
				}
				break;
			}
			case 1:
			{
				for( int i = 0; i < 262144; i++ )
				{
					if ( i <= lengthOfSample )
					{
						castModel<Microwave>()->waveforms[i] = sampleBuffer->userWaveSample( i/lengthOfSample, channel );
					}
					else
					{
						castModel<Microwave>()->m_morphMax.setValue( i/castModel<Microwave>()->m_sampLen.value() );
						castModel<Microwave>()->morphMaxChanged();
						for( int j = i; j < 262144; j++ ) { castModel<Microwave>()->waveforms[j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 2:
			{
				//float avgSampleVol = 0;
				float avgPosSampleVol = 0;
				float avgNegSampleVol = 0;
				float posSampleVols = 1;
				float negSampleVols = 1;
				sample_t sampleHere = 0;
				int cycleLengths[256] = {0};
				int cycleLength = 0;
				int cyclePart = 0;
				int cycleNum = 0;
				for( int i = 0; i < lengthOfSample; i++ )
				{
					//castModel<Microwave>()->waveforms[qBound(0,i,262144)] = sampleBuffer->userWaveSample(i/lengthOfSample, channel);
					sampleHere = sampleBuffer->userWaveSample(i/lengthOfSample, channel);
					//avgSampleVol = (avgSampleVol*((lengthOfSample-i)/lengthOfSample)) + (sampleHere/lengthOfSample);
					if( sampleHere < 0 )
					{
						avgNegSampleVol = (sampleHere/negSampleVols) + (avgNegSampleVol*((negSampleVols-1)/negSampleVols));
						negSampleVols++;
					}
					else if( sampleHere > 0 )
					{
						avgPosSampleVol = (sampleHere/posSampleVols) + (avgPosSampleVol*((posSampleVols-1)/posSampleVols));
						posSampleVols++;
					}
					
				}
				for( int i = 0; i < lengthOfSample; i++ )
				{
					sampleHere = sampleBuffer->userWaveSample(i/lengthOfSample, channel);
					cycleLength++;
					switch( cyclePart )
					{
						case 0:
							if( sampleHere >= avgPosSampleVol )
							{
								cyclePart = 1;
							}
							break;
						case 1:
							if( sampleHere <= avgNegSampleVol && cycleLength > 20 )
							{
								cyclePart = 0;
								cycleLengths[cycleNum] = cycleLength;
								cycleLength = 0;
								cycleNum++;
							}
							break;
					}
					if( cycleNum > 255 )
					{
						break;
					}
				}
				std::sort(cycleLengths,cycleLengths+256);
				int realCycleLen = cycleLengths[int(cycleNum/2.f)];
				for( int i = 0; i < 262144; i++ )
				{
					castModel<Microwave>()->waveforms[i] = sampleBuffer->userWaveSample( ( ( realCycleLen / 1024.f ) * i ) / ( ( cycleNum * realCycleLen ) ), channel );
				}
				break;
			}
		}
		
		
		sampleBuffer->dataUnlock();
	}

	sharedObject::unref( sampleBuffer );
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *, void * _data )
{
	return( new Microwave( static_cast<InstrumentTrack *>( _data ) ) );
}


}




