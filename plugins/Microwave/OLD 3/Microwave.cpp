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
	


bSynth::bSynth( NotePlayHandle * _nph, float _factor, const sample_rate_t _sample_rate, float * phase, float * phaseRand, int * sampLen) :
	nph( _nph ),
	sample_rate( _sample_rate )
{
	for( int i=0; i < 8; ++i )
	{
		for( int j=0; j < 32; ++j )
		{
			// Randomize the phases of all of the waveforms
			sample_realindex[i][j] = int( ( ( fastRandf( sampLen[i] ) - ( sampLen[i] / 2.f ) ) * ( phaseRand[i] / 100.f ) ) + ( sampLen[i] / 2.f ) ) % ( sampLen[i] );
		}
	}

	for( int i=0; i < 128; ++i )
	{
		sample_subindex[i] = 0;
	}

	noteDuration = -1;
}


bSynth::~bSynth()
{
}


std::vector<float> bSynth::nextStringSample( int * modifyModeVal, float * _modifyVal, float * _morphVal, float * rangeVal, float * sprayVal, float * unisonVoices, float * unisonDetune, float * unisonMorph, float * unisonModify, float * morphMaxVal, float * detuneVal, float waveforms[8][262144], float * subs, bool * subEnabled, float * subVol, float * subPhase, float * subPhaseRand, float * subDetune, bool * subMuted, bool * subKeytrack, float * subSampLen, bool * subNoise, int * sampLen, int * m_modIn, int * m_modInNum, float * m_modInAmnt, float * m_modInCurve, int * m_modIn2, int * m_modInNum2, float * m_modInAmnt2, float * m_modInCurve2, int * m_modOutSec, int * m_modOutSig, int * m_modOutSecNum, float * phase, float * vol, float * filtInVol, int * filtType, int * filtSlope, float * filtCutoff, float * filtReso, float * filtGain, float * filtSatu, float * filtWetDry, float * filtBal, float * filtOutVol, bool * filtEnabled )// waveforms here may cause CPU problem, check back later
{
	//=====================//
	//== MAIN OSCILLATOR ==//
	//=====================//

	noteDuration = noteDuration + 1;
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();
	float morphVal[8] = {0};
	float modifyVal[8] = {0};
	float curModVal;
	int curModSignal;

	float sample_morerealindex[8][32] = {{0}};
	sample_t sample[8][32] = {{0}};
	float sample_step[8][32] = {{0}};
	float resultsample[8][32] = {{0}};
	float sample_length_modify[8][32] = {{0}};
	int * sample_length = sampLen;
	
	for( int l = 0; l < 256; l++ )
	{
		curModVal = 0;
		switch( m_modIn[l] )
		{
			case 0:
				break;
			case 1:
				curModVal = lastMainOscVal[m_modInNum[l]-1];
				break;
			case 2:
				curModVal = lastSubVal[m_modInNum[l]-1];
				break;
			case 3:
				break;
			default:
			{
			}
		}
		switch( m_modOutSec[l] )// Do effects processing (and modulation of effect parameters) before modulation
		{
			case 0:
				break;
			case 1:
				break;
			case 2:
				break;
			case 3:// Filter Input
			{
				curModVal *= filtInVol[l] / 100.f;
				float cutoff = filtCutoff[m_modOutSig[l]];
				int mode = filtType[m_modOutSig[l]];
				float reso = filtReso[m_modOutSig[l]];
				float dbgain = filtGain[m_modOutSig[l]];
				float Fs = 44100.f;
				float a0 = 0;
				float a1 = 0;
				float a2 = 0;
				float b0 = 0;
				float b1 = 0;
				float b2 = 0;
				float alpha;
				float w0 = F_2PI * cutoff / Fs;
				float f = 2 * cutoff / Fs;
				float k = 3.6*f - 1.6*f*f - 1;
				float p = (k+1)*0.5f;
				float scale = pow( 2.718281828f, (1-p)*1.386249f );
				float r = reso*scale;
				float A;
				if( mode <= 6 )
				{
					switch( mode )
					{
						case 0:// LP
							alpha = sin(w0) / (2*reso);
							b0 = ( 1 - cos(w0) ) / 2;
							b1 = 1 - cos(w0);
							b2 = ( 1 - cos(w0) ) / 2;
							a0 = 1 + alpha;
							a1 = -2 * cos(w0);
							a2 = 1 - alpha;
							break;
						case 1:// HP
							alpha = sin(w0) / (2*reso);
							b0 =  (1 + cos(w0))/2;
			       	     			b1 = -(1 + cos(w0));
			       	     			b2 =  (1 + cos(w0))/2;
		            				a0 =   1 + alpha;
		            				a1 =  -2*cos(w0);
		            				a2 =   1 - alpha;
							break;
						case 2:// BP
							alpha = sin(w0)*sinh( (log(2)/log(2.7182818284590452353602874713527))/2 * reso * w0/sin(w0) );
							b0 =   alpha;
		            				b1 =   0;
		            				b2 =  -alpha;
		            				a0 =   1 + alpha;
		            				a1 =  -2*cos(w0);
		            				a2 =   1 - alpha;
							break;
						case 3:// Peak
							alpha = sin(w0)*sinh( (log(2)/log(2.7182818284590452353602874713527))/2 * reso * w0/sin(w0) );
							A = pow( 10, dbgain / 40 );
							b0 =   1 + alpha*A;
							b1 =  -2*cos(w0);
							b2 =   1 - alpha*A;
							a0 =   1 + alpha/A;
							a1 =  -2*cos(w0);
						        a2 =   1 - alpha/A;
						case 4:// Notch
							alpha = sin(w0)*sinh( (log(2)/log(2.7182818284590452353602874713527))/2 * reso * w0/sin(w0) );
							b0 =   1;
            						b1 =  -2*cos(w0);
            						b2 =   1;
            						a0 =   1 + alpha;
            						a1 =  -2*cos(w0);
            						a2 =   1 - alpha;
						case 5:// Low Shelf
							alpha = sin(w0) / (2*reso);
							A = pow( 10, dbgain / 40 );
							b0 =    A*( (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha );
            						b1 =  2*A*( (A-1) - (A+1)*cos(w0)                   );
            						b2 =    A*( (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha );
            						a0 =        (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha;
            						a1 =   -2*( (A-1) + (A+1)*cos(w0)                   );
            						a2 =        (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha;
						case 6:// High Shelf
							alpha = sin(w0) / (2*reso);
							A = pow( 10, dbgain / 40 );
							b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha );
            						b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   );
            						b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha );
            						a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha;
            						a1 =    2*( (A-1) - (A+1)*cos(w0)                   );
            						a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha;
					}
					// Translation of this monstrosity: y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
					// See www.musicdsp.org/files/Audio-EQ-Cookbook.txt
					filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][0][0] = (b0/a0)*curModVal + (b1/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][1][0] + (b2/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][2][0] - (a1/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][1][0] - (a2/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][2][0];// Left ear
					filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][0][1] = (b0/a0)*curModVal + (b1/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][1][1] + (b2/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][2][1] - (a1/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][1][1] - (a2/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][2][1];// Right ear

					// I'm sorry you had to read this
					filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][2][0] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][1][0];
					filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][2][1] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][1][1];
			
					filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][1][0] = curModVal;
					filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][1][1] = curModVal;
			
					filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][2][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][1][0];
					filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][2][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][1][1];
			
					filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][1][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][0][0];
					filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][1][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][0][1];
			
					filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][0][0] * ( filtOutVol[l] / 100.f );
					filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][0][1] * ( filtOutVol[l] / 100.f );
					}
				break;
			}
			case 4:// Filter Parameters
				switch( m_modOutSig[l] )
				{
					case 0:
						break;
					case 1:// Input Volume
						filtInVol[m_modOutSecNum[l]-1] = qMax( 0.f, filtInVol[m_modOutSecNum[l]-1] + ( round((curModVal+1)*100.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
					case 2:// Filter Type
						filtType[m_modOutSecNum[l]-1] = qMax( 0, int(filtType[m_modOutSecNum[l]-1] + ( round((curModVal+1)*7.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) ) );
						break;
					case 3:
						break;
					case 4:// Cutoff Frequency
						filtCutoff[m_modOutSecNum[l]-1] = qBound( 20.f, filtCutoff[m_modOutSecNum[l]-1] + ( round((curModVal+1)*19980.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ), 20000.f );
						break;
					case 5:// Resonance
						filtReso[m_modOutSecNum[l]-1] = qMax( 0.0001f, filtReso[m_modOutSecNum[l]-1] + ( round((curModVal+1)*16.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
					case 6:// dbGain
						filtGain[m_modOutSecNum[l]-1] = filtGain[m_modOutSecNum[l]-1] + ( round((curModVal+1)*64.f*0.5f) ) * ( m_modInAmnt[l] / 100.f );
						break;
					case 7:
						break;
					case 8:
						break;
					case 9:
						break;
					case 10:// Output Volume
						filtOutVol[m_modOutSecNum[l]-1] = qMax( 0.f, filtOutVol[m_modOutSecNum[l]-1] + ( round((curModVal+1)*100.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
				}
			default:
			{
			}
		}
		switch( m_modOutSec[l] )// Do modulations after effects processing
		{
			case 0:
				break;
			case 1:// Main Oscillator
				switch( m_modOutSig[l] )
				{
					case 0:
						break;
					case 1:// Send input to Morph
						_morphVal[m_modOutSecNum[l]-1] = qBound( 0.f, _morphVal[m_modOutSecNum[l]-1] + ( round((curModVal+1)*morphMaxVal[m_modOutSecNum[l]-1]*0.5f) ) * ( m_modInAmnt[l] / 100.f ), morphMaxVal[m_modOutSecNum[l]-1] );
						break;
					case 2:// Send input to Range
						rangeVal[m_modOutSecNum[l]-1] = qMax( 0.f, rangeVal[m_modOutSecNum[l]-1] + ( round((curModVal+1)*16.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
					case 3:// Send input to Modify
						_modifyVal[m_modOutSecNum[l]-1] = qMax( 0.f, _modifyVal[m_modOutSecNum[l]-1] + ( round((curModVal+1)*1024.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
					case 4:// Send input to Pitch/Detune
						detuneVal[m_modOutSecNum[l]-1] = detuneVal[m_modOutSecNum[l]-1] + ( round((curModVal)*9600.f) ) * ( m_modInAmnt[l] / 100.f );
						break;
					case 5:// Send input to Phase
						phase[m_modOutSecNum[l]-1] = fmod( phase[m_modOutSecNum[l]-1] + ( round((curModVal)*100.f) ) * ( m_modInAmnt[l] / 100.f ), 100 );
						break;
					case 6:// Send input to Volume
						vol[m_modOutSecNum[l]-1] = qMax( 0.f, vol[m_modOutSecNum[l]-1] + ( round((curModVal+1)*100.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
					default:
					{
					}
				}
				break;
			case 2:// Sub Oscillator
				switch( m_modOutSig[l] )
				{
					case 0:
						break;
					case 1:// Send input to Pitch/Detune
						subDetune[m_modOutSecNum[l]-1] = subDetune[m_modOutSecNum[l]-1] + ( round((curModVal)*9600.f) ) * ( m_modInAmnt[l] / 100.f );
						break;
					case 2:// Send input to Phase
						subPhase[m_modOutSecNum[l]-1] = fmod( subPhase[m_modOutSecNum[l]-1] + ( round((curModVal)*100.f) ) * ( m_modInAmnt[l] / 100.f ), 100 );
						break;
					case 3:// Send input to Volume
						subVol[m_modOutSecNum[l]-1] = qMax( 0.f, subVol[m_modOutSecNum[l]-1] + ( round((curModVal+1)*100.f*0.5f) ) * ( m_modInAmnt[l] / 100.f ) );
						break;
					default:
					{
					}
				}
				break;
			case 3:
				break;
			case 4:
				break;
			case 5:
				break;
			case 6:
				break;
			default:
			{
			}
		}
	}

	/*for( int l = 0; l < 32; l++ )
	{
		if( seqEnableds[l] && seqLens[l] )
		{
			lastSeqVal[l] = seqs[((l*64)+(int((noteDuration+0.f)/seqLens[l]*64))%(stepNums[l]))];// Finds Step Sequencer position
		}
		if( envEnableds[l] && envLens[l] )
		{
			lastEnvVal[l] = envs[qBound( 0, (l*1024)+int((noteDuration+0.f)/envLens[l]*1024), (l+1)*1024-1)];// Finds Envelope position
		}
		if( lfoEnableds[l] && lfoLens[l] )
		{
			lastLfoVal[l] = lfos[((l*1024)+int((noteDuration+0.f)/lfoLens[l]*1024))%(1024)];// Finds LFO position
		}
	}*/

	for( int i=0; i < 8; ++i )
	{
		sample_length[i] = sampLen[i];
	}

	for( int i = 0; i < 8; i++ )
	{
		for( int l = 0; l < unisonVoices[i]; l++ )
		{
			sample_morerealindex[i][l] = fmod( ( sample_realindex[i][l] + ( fmod( phase[i], 100.f ) * sample_length[i] / 100.f ) ), sample_length[i] );// Calculates phase
	
			float noteFreq = unisonVoices[i] - 1 ? detuneWithCents( nph->frequency(), (fastRandf(unisonDetune[i]*2)-unisonDetune[i])+detuneVal[i] ) : detuneWithCents( nph->frequency(), detuneVal[i] );// Calculates frequency depending on detune and unison detune
	
			sample_step[i][l] = static_cast<float>( sample_length[i] / ( sample_rate / noteFreq ) );
	
			if( unisonVoices[i] - 1 )// Figures out Morph and Modify values for individual unison voices
			{
				morphVal[i] = (((unisonVoices[i]-1)-l)/(unisonVoices[i]-1))*unisonMorph[i];
				morphVal[i] = qBound( 0.f, morphVal[i] - ( unisonMorph[i] / 2.f ) + _morphVal[i], morphMaxVal[i] );
	
				modifyVal[i] = (((unisonVoices[i]-1)-l)/(unisonVoices[i]-1))*unisonModify[i];
				modifyVal[i] = qBound( 0.f, modifyVal[i] - ( unisonModify[i] / 2.f ) + _modifyVal[i], sample_length[i]-1.f);// SampleLength - 1 = ModifyMax
			}
			else
			{
				morphVal[i] = _morphVal[i];
				modifyVal[i] = _modifyVal[i];
			}
	
			sample_length_modify[i][l] = 0;
			switch( modifyModeVal[i] )// Horrifying formulas for the various Modify Modes
			{
				case 0:// Pulse Width
					sample_morerealindex[i][l] = sample_morerealindex[i][l] / ( ( -modifyVal[i] + sample_length[i] ) / sample_length[i] );
					break;
				case 1:// Stretch Left Half
					if( sample_morerealindex[i][l] < ( sample_length[i] + sample_length_modify[i][l] ) / 2.f )
					{
						sample_morerealindex[i][l] = sample_morerealindex[i][l] - (sample_morerealindex[i][l] * (modifyVal[i]/( sample_length[i] )));
					}
					break;
				case 2:// Shrink Right Half To Left Edge
					if( sample_morerealindex[i][l] > ( sample_length[i] + sample_length_modify[i][l] ) / 2.f )
					{
						sample_morerealindex[i][l] = sample_morerealindex[i][l] + (((sample_morerealindex[i][l])+sample_length[i]/2.f) * (modifyVal[i]/( sample_length[i] )));
					}
					break;
				case 3:// Weird 1
					sample_morerealindex[i][l] = ( ( ( sin( ( ( ( sample_morerealindex[i][l] ) / ( sample_length[i] ) ) * ( modifyVal[i] / 50.f ) ) / 2 ) ) * ( sample_length[i] ) ) * ( modifyVal[i] / sample_length[i] ) + ( sample_morerealindex[i][l] + ( ( -modifyVal[i] + sample_length[i] ) / sample_length[i] ) ) ) / 2.f;
					break;
				case 4:// Asym To Right
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sample_length[i] ) ), (modifyVal[i]*10/sample_length[i])+1 ) * ( sample_length[i] );
					break;
				case 5:// Asym To Left
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sample_length[i];
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sample_length[i] ) ), (modifyVal[i]*10/sample_length[i])+1 ) * ( sample_length[i] );
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sample_length[i];
					break;
				case 6:// Weird 2
					if( sample_morerealindex[i][l] > sample_length[i] / 2.f )
					{
						sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sample_length[i] ) ), (modifyVal[i]*10/sample_length[i])+1 ) * ( sample_length[i] );
					}
					else
					{
						sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sample_length[i];
						sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sample_length[i] ) ), (modifyVal[i]*10/sample_length[i])+1 ) * ( sample_length[i] );
						sample_morerealindex[i][l] = sample_morerealindex[i][l] - sample_length[i] / 2.f;
					}
					break;
				case 7:// Stretch From Center
					sample_morerealindex[i][l] = sample_morerealindex[i][l] - sample_length[i] / 2.f;
					sample_morerealindex[i][l] = sample_morerealindex[i][l] / ( sample_length[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sample_length[i] + 1 ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sample_length[i] + 1 ) );
					sample_morerealindex[i][l] = sample_morerealindex[i][l] * ( sample_length[i] / 2.f );
					sample_morerealindex[i][l] = sample_morerealindex[i][l] + sample_length[i] / 2.f;
					break;
				case 8:// Squish To Center
					sample_morerealindex[i][l] = sample_morerealindex[i][l] - sample_length[i] / 2.f;
					sample_morerealindex[i][l] = sample_morerealindex[i][l] / ( sample_length[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sample_length[i] + 1  ) ) : -pow( -sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sample_length[i] + 1 ) );
					sample_morerealindex[i][l] = sample_morerealindex[i][l] * ( sample_length[i] / 2.f );
					sample_morerealindex[i][l] = sample_morerealindex[i][l] + sample_length[i] / 2.f;
					break;
				case 9:// Stretch And Squish
					sample_morerealindex[i][l] = sample_morerealindex[i][l] - sample_length[i] / 2.f;
					sample_morerealindex[i][l] = sample_morerealindex[i][l] / ( sample_length[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sample_length[i] ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sample_length[i] ) );
					sample_morerealindex[i][l] = sample_morerealindex[i][l] * ( sample_length[i] / 2.f );
					sample_morerealindex[i][l] = sample_morerealindex[i][l] + sample_length[i] / 2.f;
					break;
				case 10:// Add Tail
					sample_length_modify[i][l] = modifyVal[i];
					dynamic_cast<Microwave *>(nph->instrumentTrack()->instrument())->setGraphLength( sample_length[i] + sample_length_modify[i][l] );
					break;
				case 11:// Cut Off Right
					sample_morerealindex[i][l] = sample_morerealindex[i][l] * ( ( -modifyVal[i] + sample_length[i] ) / sample_length[i] );
					break;
				case 12:// Cut Off Left
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sample_length[i];
					sample_morerealindex[i][l] = sample_morerealindex[i][l] * ( ( -modifyVal[i] + sample_length[i] ) / sample_length[i] );
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sample_length[i];
					break;
				default:
					{}
			}
			sample_morerealindex[i][l] = sample_morerealindex[i][l] + indexOffset[i];// Moves index for Spray knob
			sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sample_length[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds
			
			resultsample[i][l] = 0;
			if( sample_morerealindex[i][l] <= sample_length[i] + sample_length_modify[i][l] )
			{
				int loopStart = qBound(0.f, morphVal[i] - rangeVal[i], 262144.f / sample_length[i] );
				int loopEnd = qBound(0.f, morphVal[i] + rangeVal[i], 262144.f / sample_length[i]) + 1;
				float currentMorphVal = morphVal[i];
				float currentRangeValInvert = 1.f / rangeVal[i];// Inverted to prevent excessive division in the loop below, for a minor CPU improvement.  Don't ask.
				int currentSampLen = sample_length[i];
				int currentIndex = sample_morerealindex[i][l];
				// Only grab samples from the waveforms when they will be used, depending on the Range knob
				for( int j = loopStart; j < loopEnd; j++ )
				{
					// Get waveform samples, set their volumes depending on Range knob
					resultsample[i][l] += waveforms[i][currentIndex + j * currentSampLen] * qMax(0.0f, 1 - ( abs( currentMorphVal - j ) * currentRangeValInvert ));
				}
			}
			resultsample[i][l] = resultsample[i][l] / ( ( rangeVal[i] / 2.f ) + 3 );// Decreases volume so Range value doesn't make things too loud
	
			switch( modifyModeVal[i] )// More horrifying formulas for the various Modify Modes
			{
				case 0:// Pulse Width
					if( sample_realindex[i][l] / ( ( -modifyVal[i] + sample_length[i] ) / sample_length[i] ) > sample_length[i] )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 2:// Shrink Right Half To Left Edge
					if( sample_realindex[i][l] + (((sample_realindex[i][l])+sample_length[i]/2.f) * (modifyVal[i]/( sample_length[i] ))) > sample_length[i]  )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 10:// Add Tail
					if( sample_morerealindex[i][l] > sample_length[i] )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 13:// Flip
					if( sample_realindex[i][l] > modifyVal[i] )
					{
						resultsample[i][l] = -resultsample[i][l];
					}
					break;
				default:
					{}
			}
	
			sample_realindex[i][l] = sample_realindex[i][l] + sample_step[i][l];
	
			// check overflow
			while ( sample_realindex[i][l] >= sample_length[i] + sample_length_modify[i][l] ) {
				sample_realindex[i][l] = sample_realindex[i][l] - (sample_length[i] + sample_length_modify[i][l]);
				indexOffset[i] = fastRandf( int( sprayVal[i] ) ) - int( sprayVal[i] / 2 );// Reset offset caused by Spray knob
			}
	
			sample[i][l] = resultsample[i][l] * ( vol[i] / 100.f );
		}
	}

	//====================//
	//== SUB OSCILLATOR ==//
	//====================//

	float sample_step_sub = 0;
	sample_t subsample[128] = {0};
	for( int l = 0; l < 128; l++ )
	{
		if( subEnabled[l] )
		{
			if( !subNoise[l] )
			{
				float noteFreq = detuneWithCents( nph->frequency(), subDetune[l] );
				sample_step_sub = static_cast<float>( subSampLen[l] / ( sample_rate / noteFreq ) );
	
				subsample[l] = ( subVol[l] / 100.f ) * subs[int( ( int( sample_subindex[l] + ( subPhase[l] * subSampLen[l] ) ) % int(subSampLen[l]) ) + ( 1024 * l ) )];
				/* That is all that is needed for the sub oscillator calculations.
		
				(To the tune of Hallelujah)
		
					There was a Happy CPU
				No processing power to chew through
				In this wonderful synthesis brew
					Hallelujah
		
					But with some wavetable synthesis
				Your CPU just blows a kiss
				And leaves you there despite your miss
					Hallelujah
		
				Hallelujah, Hallelujah, Hallelujah, Halleluuu-uuuuuuuu-uuuujah
		
					Your music ambition lays there, dead
				Can't get your ideas out of your head
				Because your computer's slower than lead
					Hallelujah
		
					Sometimes you may try and try
				To keep your ping from saying goodbye
				Leaving you to die and cry
					Hallelujah
		
				Hallelujah, Hallelujah, Hallelujah, Halleluuu-uuuuuuuu-uuuujah
		
					But what is this, an alternative?
				Sub oscillators supported native
				To prevent CPU obliteratives
					Hallelujah
		
					Your music has come back to life
				CPU problems cut off like a knife
				Sub oscillators removing your strife
					Hallelujah
		
				Hallelujah, Hallelujah, Hallelujah, Halleluuu-uuuuuuuu-uuuujah
		
				*cool outro*
					
		
				*/
		
				lastSubVal[l] = subsample[l];// Store value for modulation
	
				subsample[l] = subMuted[l] ? 0 : subsample[l];// Mutes sub after saving value for modulation if the muted option is on
		
				sample_subindex[l] = sample_subindex[l] + sample_step_sub;
		
				// move waveform position back if it passed the end of the waveform
				while ( sample_subindex[l] >= subSampLen[l] )
				{
					sample_subindex[l] = sample_subindex[l] - subSampLen[l];
				}
			}
			else// sub oscillator is noise
			{
				noiseSampRand = fastRandf( subSampLen[l] ) - 1;
				subsample[l] = qBound( -1.f, ( lastSubVal[0]+(subs[int(noiseSampRand)] / 8.f) ) / 1.2f, 1.f );// Division by 1.05f to tame DC offset
				lastSubVal[l] = subsample[l];
			}
		}
		else
		{
			subsample[l] = 0;
			lastSubVal[l] = 0;
		}
	}

	std::vector<float> sampleAvg(2);
	std::vector<float> sampleMainOsc(2);
	for( int i = 0; i < 8; i++ )
	{
		lastMainOscVal[i] = ( sample[i][0] + sample[i][1] ) / 2.f;// Store results for modulations
		if( unisonVoices[i] > 1 )
		{
			sampleMainOsc[0] = 0;
			sampleMainOsc[1] = 0;
			for( int j = 0; j < unisonVoices[i]; j++ )
			{
				// Pan unison voices
				sampleMainOsc[0] = sampleMainOsc[0] + sample[i][j] * (((unisonVoices[i]-1)-j)/(unisonVoices[i]-1));
				sampleMainOsc[1] = sampleMainOsc[1] + sample[i][j] * (j/(unisonVoices[i]-1));
			}
			// Decrease volume so more unison voices won't increase volume too much
			sampleMainOsc[0] = sampleMainOsc[0] / ( unisonVoices[i] / 2 );
			sampleMainOsc[1] = sampleMainOsc[1] / ( unisonVoices[i] / 2 );
			
			sampleAvg[0] = sampleAvg[0] + sampleMainOsc[0];
			sampleAvg[1] = sampleAvg[1] + sampleMainOsc[1];
		}
		else
		{
			sampleAvg[0] = sampleAvg[0] + sample[i][0];
			sampleAvg[1] = sampleAvg[1] + sample[i][0];
		}
	}
	for( int l = 0; l < 128; l++ )
	{
		if( subEnabled[l] )
		{
			sampleAvg[0] += subsample[l];
			sampleAvg[1] += subsample[l];
		}
	}

	// Filter outputs
	for( int l = 0; l < 8; l++ )
	{
		if( filtEnabled[l] )
		{
			for( int m = 0; m < 8; m++ )
			{
				sampleAvg[0] += filtOutputs[l][m][0];
				sampleAvg[1] += filtOutputs[l][m][1];
			}
		}
	}

	return sampleAvg;
}


float bSynth::getRealIndex()
{
	return sample_realindex[0][0];
}


float bSynth::detuneWithCents( float pitchValue, float detuneValue )
{
	return pitchValue * pow( 2, detuneValue / 1200.f );
}


Microwave::Microwave( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &microwave_plugin_descriptor ),
//	m_morph( 0, 0, 254, 0.0001f, this, tr( "Morph" ) ),
//	m_range( 8, 1, 16, 0.0001f, this, tr( "Range" ) ),
//	m_sampLen( 1024, 1, 8192, 1.f, this, tr( "Waveform Sample Length" ) ),
	m_visvol( 100, 0, 1000, 0.01f, this, tr( "Visualizer Volume" ) ),
//	m_morphMax( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) ),
//	m_modifyMode( this, tr( "Wavetable Modifier Mode" ) ),
//	m_modify( 0, 0, 1024, 0.0001f, this, tr( "Wavetable Modifier Value" ) ),
	m_modNum(1, 1, 32, this, tr( "Modulation Page Number" ) ),
//	m_spray( 0, 0, 2048, 0.0001f, this, tr( "Spray" ) ),
//	m_unisonVoices( 1, 1, 32, 1, this, tr( "Unison Voices" ) ),
//	m_unisonDetune( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) ),
//	m_unisonMorph( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) ),
//	m_unisonModify( 0, 0, 1024, 0.0001f, this, tr( "Unison Modify" ) ),
//	m_detune( 0, -9600, 9600, 1.f, this, tr( "Detune" ) ),
	m_loadAlg( 0, 0, 3, 1, this, tr( "Wavetable Loading Algorithm" ) ),
	m_loadChnl( 0, 0, 1, 1, this, tr( "Wavetable Loading Channel" ) ),
//	m_phase( 0, 0, 200, 0.0001f, this, tr( "Phase" ) ),
//	m_phaseRand( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) ),
	m_scroll( 1, 1, 4, 0.0001f, this, tr( "Scroll" ) ),
	m_subNum(1, 1, 128, this, tr( "Sub Oscillator Number" ) ),
	m_mainNum(1, 1, 8, this, tr( "Main Oscillator Number" ) ),
	m_graph( -1.0f, 1.0f, 1024, this ),
	m_interpolation( false, this ),
	m_normalize( false, this ),
	m_visualize( false, this )
{

	for( int i = 0; i < 8; i++ )
	{
		m_morph[i] = new FloatModel( 0, 0, 254, 0.0001f, this, tr( "Morph" ) );
		m_range[i] = new FloatModel( 8, 1, 16, 0.0001f, this, tr( "Range" ) );
		m_sampLen[i] = new FloatModel( 1024, 1, 8192, 1.f, this, tr( "Waveform Sample Length" ) );
		m_morphMax[i] = new FloatModel( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) );
		m_modifyMode[i] = new ComboBoxModel( this, tr( "Wavetable Modifier Mode" ) );
		m_modify[i] = new FloatModel( 0, 0, 1024, 0.0001f, this, tr( "Wavetable Modifier Value" ) );
		m_spray[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Spray" ) );
		m_spray[i]->setScaleLogarithmic( true );
		m_unisonVoices[i] = new FloatModel( 1, 1, 32, 1, this, tr( "Unison Voices" ) );
		m_unisonDetune[i] = new FloatModel( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) );
		m_unisonDetune[i]->setScaleLogarithmic( true );
		m_unisonMorph[i] = new FloatModel( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) );
		m_unisonModify[i] = new FloatModel( 0, 0, 1024, 0.0001f, this, tr( "Unison Modify" ) );
		m_detune[i] = new FloatModel( 0, -9600, 9600, 1.f, this, tr( "Detune" ) );
		m_phase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		m_phaseRand[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		m_vol[i] = new FloatModel( 100.f, 0, 200.f, 0.0001f, this, tr( "Volume" ) );
		setwavemodel( m_modifyMode[i] )

		//float * filtInVol, int * filtType, int * filtSlope, float * filtCutoff, float * filtReso, float * filtGain, float * filtSatu, float * filtWetDry, float * filtBal, float * filtOutVol
		m_filtInVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Input Volume" ) );
		m_filtType[i] = new ComboBoxModel( this, tr( "Filter Type" ) );
		m_filtSlope[i] = new ComboBoxModel( this, tr( "Filter Slope" ) );
		m_filtCutoff[i] = new FloatModel( 2000, 20, 20000, 0.0001f, this, tr( "Cutoff Frequency" ) );
		m_filtCutoff[i]->setScaleLogarithmic( true );
		m_filtReso[i] = new FloatModel( 1, 0, 16, 0.0001f, this, tr( "Resonance" ) );
		m_filtReso[i]->setScaleLogarithmic( true );
		m_filtGain[i] = new FloatModel( 0, -64, 64, 0.0001f, this, tr( "dbGain" ) );
		m_filtGain[i]->setScaleLogarithmic( true );
		m_filtSatu[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Saturation" ) );
		m_filtWetDry[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Wet/Dry" ) );
		m_filtBal[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Balance/Panning" ) );
		m_filtOutVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Output Volume" ) );
		m_filtEnabled[i] = new BoolModel( false, this );
		filtertypesmodel( m_filtType[i] )
		filterslopesmodel( m_filtSlope[i] )
	}

	for( int i = 0; i < 128; i++ )
 	{
		m_subEnabled[i] = new BoolModel( false, this );
		m_subVol[i] = new FloatModel( 100.f, 0.f, 200.f, 0.0001f, this, tr( "Sub Oscillator Volume" ) );
		m_subPhase[i] = new FloatModel( 0.f, 0.f, 200.f, 0.0001f, this, tr( "Sub Oscillator Phase" ) );
		m_subPhaseRand[i] = new FloatModel( 0.f, 0.f, 100.f, 0.0001f, this, tr( "Sub Oscillator Phase Randomness" ) );
		m_subDetune[i] = new FloatModel( 0.f, -9600.f, 9600.f, 1.f, this, tr( "Sub Oscillator Detune" ) );
		m_subMuted[i] = new BoolModel( true, this );
		m_subKeytrack[i] = new BoolModel( true, this );
		m_subSampLen[i] = new FloatModel( 1024.f, 1.f, 1024.f, 1.f, this, tr( "Sub Oscillator Sample Length" ) );
		m_subNoise[i] = new BoolModel( false, this );
	}

	for( int i = 0; i < 256; i++ )
	{
		m_modOutSec[i] = new ComboBoxModel( this, tr( "Modulation Section" ) );
		m_modOutSig[i] = new ComboBoxModel( this, tr( "Modulation Signal" ) );
		m_modOutSecNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulation Section Number" ) );
		modsectionsmodel( m_modOutSec[i] )
		mainoscsignalsmodel( m_modOutSig[i] )
		
		m_modIn[i] = new ComboBoxModel( this, tr( "Modulator" ) );
		m_modInNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		modinmodel( m_modIn[i] )

		m_modInAmnt[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Modulator Amount" ) );
		m_modInCurve[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Modulator Curve" ) );

		m_modIn2[i] = new ComboBoxModel( this, tr( "Secondary Modulator" ) );
		m_modInNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		modinmodel( m_modIn2[i] )

		m_modInAmnt2[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Secondary Modulator Amount" ) );
		m_modInCurve2[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Secondary Modulator Curve" ) );
	}


	m_graph.setWaveToSine();

	for( int i = 0; i < 8; i++ )
	{
		connect( m_morphMax[i], SIGNAL( dataChanged( ) ),
			this, SLOT( morphMaxChanged( ) ) );

//		connect( m_sampLen[i], SIGNAL( dataChanged( ) ),
//			this, SLOT( sampLenChanged( ) ) );
	}

	connect( &m_graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );

}




Microwave::~Microwave()
{
}




void Microwave::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.4" );

	m_visvol.saveSettings( _doc, _this, "visualizervolume" );
	m_loadAlg.saveSettings( _doc, _this, "loadingalgorithm" );
	m_loadChnl.saveSettings( _doc, _this, "loadingchannel" );

	float m_morphArr[8] = {0};
	float m_rangeArr[8] = {0};
	float m_sampLenArr[8] = {0};
	float m_sprayArr[8] = {0};
	float m_morphMaxArr[8] = {0};
	int m_modifyModeArr[8] = {0};
	float m_modifyArr[8] = {0};
	float m_unisonVoicesArr[8] = {0};
	float m_unisonDetuneArr[8] = {0};
	float m_unisonMorphArr[8] = {0};
	float m_unisonModifyArr[8] = {0};
	float m_detuneArr[8] = {0};
	float m_phaseArr[8] = {0};
	float m_phaseRandArr[8] = {0};
	float m_volArr[8] = {0};
	for( int i = 0; i < 8; i++ )
	{
		m_morphArr[i] = m_morph[i]->value();
		m_rangeArr[i] = m_range[i]->value();
		m_sampLenArr[i] = m_sampLen[i]->value();
		m_sprayArr[i] = m_spray[i]->value();
		m_morphMaxArr[i] = m_morphMax[i]->value();
		m_modifyModeArr[i] = m_modifyMode[i]->value();
		m_modifyArr[i] = m_modify[i]->value();
		m_unisonVoicesArr[i] = m_unisonVoices[i]->value();
		m_unisonDetuneArr[i] = m_unisonDetune[i]->value();
		m_unisonMorphArr[i] = m_unisonMorph[i]->value();
		m_unisonModifyArr[i] = m_unisonModify[i]->value();
		m_detuneArr[i] = m_detune[i]->value();
		m_phaseArr[i] = m_phase[i]->value();
		m_phaseRandArr[i] = m_phaseRand[i]->value();
		m_volArr[i] = m_vol[i]->value();
	}

	// Save arrays base64-encoded
	QString saveString;
	
	base64::encode( (const char *)m_graph.samples(),
		m_graph.length() * sizeof(float), saveString );
	_this.setAttribute( "sampleShape", saveString );

	for( int i = 0; i < 8; i++ )
	{
		base64::encode( (const char *)waveforms[i],
			262144 * sizeof(float), saveString );
		_this.setAttribute( "waveform"+QString::number(i), saveString );
	}

	base64::encode( (const char *)m_morphArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "morph", saveString );

	base64::encode( (const char *)m_rangeArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "range", saveString );

	base64::encode( (const char *)m_sampLenArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "sampLen", saveString );

	base64::encode( (const char *)m_sprayArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "spray", saveString );

	base64::encode( (const char *)m_morphMaxArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "morphMax", saveString );

	base64::encode( (const char *)m_modifyModeArr,
		8 * sizeof(int), saveString );
	_this.setAttribute( "modifyMode", saveString );

	base64::encode( (const char *)m_modifyArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "modify", saveString );

	base64::encode( (const char *)m_unisonVoicesArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "unisonVoices", saveString );

	base64::encode( (const char *)m_unisonDetuneArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "unisonDetune", saveString );

	base64::encode( (const char *)m_unisonMorphArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "unisonMorph", saveString );

	base64::encode( (const char *)m_unisonModifyArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "unisonModify", saveString );

	base64::encode( (const char *)m_detuneArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "detune", saveString );

	base64::encode( (const char *)m_phaseArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "phase", saveString );

	base64::encode( (const char *)m_phaseRandArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "phaseRand", saveString );

	base64::encode( (const char *)m_volArr,
		8 * sizeof(float), saveString );
	_this.setAttribute( "volume", saveString );
	

	m_interpolation.saveSettings( _doc, _this, "interpolation" );
	
	m_normalize.saveSettings( _doc, _this, "normalize" );

	m_visualize.saveSettings( _doc, _this, "visualize" );
}




void Microwave::loadSettings( const QDomElement & _this )
{
	m_visvol.loadSettings( _this, "visualizervolume" );
	m_loadAlg.loadSettings( _this, "loadingalgorithm" );
	m_loadChnl.loadSettings( _this, "loadingchannel" );

	m_graph.setLength( 1024 );

	// Load arrays
	int size = 0;
	char * dst = 0;
	for( int j = 0; j < 8; j++ )
	{
		base64::decode( _this.attribute( "waveform"+QString::number(j) ), &dst, &size );
		for( int i = 0; i < 262144; i++ )
		{
			waveforms[j][i] = ( (float*) dst )[i];
		}
	}

	base64::decode( _this.attribute( "morph" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_morph[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "range" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_range[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "sampLen" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_sampLen[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "spray" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_spray[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "morphMax" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_morphMax[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "modifyMode" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_modifyMode[i]->setValue( ( (int*) dst )[i] );
	}

	base64::decode( _this.attribute( "modify" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_modify[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "unisonVoices" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_unisonVoices[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "unisonDetune" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_unisonDetune[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "unisonMorph" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_unisonMorph[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "unisonModify" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_unisonModify[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "detune" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_detune[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "phase" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_phase[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "phaseRand" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_phaseRand[i]->setValue( ( (float*) dst )[i] );
	}

	base64::decode( _this.attribute( "volume" ), &dst, &size );
	for( int i = 0; i < 8; i++ )
	{
		m_vol[i]->setValue( ( (float*) dst )[i] );
	}

	delete[] dst;

	m_interpolation.loadSettings( _this, "interpolation" );
	
	m_normalize.loadSettings( _this, "normalize" );

	m_visualize.loadSettings( _this, "visualize" );

}


void MicrowaveView::updateScroll()
{
	
	float scrollVal = int( ( castModel<Microwave>()->m_scroll.value() - 1 ) * 250.f );

	for( int i = 0; i < 8; i++ )
	{
		m_morphKnob[i]->move( 20 - scrollVal, 165 );
		m_rangeKnob[i]->move( 50 - scrollVal, 165 );
		m_sampLenKnob[i]->move( 50 - scrollVal, 185 );
		m_morphMaxKnob[i]->move( 20 - scrollVal, 195 );
		m_modifyKnob[i]->move( 110 - scrollVal, 165 );
		m_sprayKnob[i]->move( 140 - scrollVal, 165 );
		m_unisonVoicesKnob[i]->move( 170 - scrollVal, 165 );
		m_unisonDetuneKnob[i]->move( 200 - scrollVal, 165 );
		m_unisonMorphKnob[i]->move( 170 - scrollVal, 195 );
		m_unisonModifyKnob[i]->move( 200 - scrollVal, 195 );
		m_detuneKnob[i]->move( 100 - scrollVal, 5 );
		m_phaseKnob[i]->move( 50 - scrollVal, 215 );
		m_phaseRandKnob[i]->move( 80 - scrollVal, 215 );
		m_volKnob[i]->move( 150 - scrollVal, 195 );

		//float * filtInVol, int * filtType, int * filtSlope, float * filtCutoff, float * filtReso, float * filtGain, float * filtSatu, float * filtWetDry, float * filtBal, float * filtOutVol
		m_filtInVolKnob[i]->move( 20 + 750 - scrollVal, i*60 );
		m_filtTypeBox[i]->move( 50 + 750 - scrollVal, i*60 );
		m_filtSlopeBox[i]->move( 90 + 750 - scrollVal, i*60 );
		m_filtCutoffKnob[i]->move( 130 + 750 - scrollVal, i*60 );
		m_filtResoKnob[i]->move( 160 + 750 - scrollVal, i*60 );
		m_filtGainKnob[i]->move( 190 + 750 - scrollVal, i*60 );
		m_filtSatuKnob[i]->move( 20 + 750 - scrollVal, i*60+30 );
		m_filtWetDryKnob[i]->move( 50 + 750 - scrollVal, i*60+30 );
		m_filtBalKnob[i]->move( 80 + 750 - scrollVal, i*60+30 );
		m_filtOutVolKnob[i]->move( 110 + 750 - scrollVal, i*60+30 );
		m_filtEnabledToggle[i]->move( 140 + 750 - scrollVal, i*60+30 );
	}

	for( int i = 0; i < 128; i++ )
	{
		m_subEnabledToggle[i]->move( 120 + 250 - scrollVal, 220 );
		m_subVolKnob[i]->move( 30 + 250 - scrollVal, 165 );
		m_subPhaseKnob[i]->move( 60 + 250 - scrollVal, 195 );
		m_subPhaseRandKnob[i]->move( 90 + 250 - scrollVal, 195 );
		m_subDetuneKnob[i]->move( 60 + 250 - scrollVal, 165 );
		m_subMutedToggle[i]->move( 120 + 250 - scrollVal, 210 );
		m_subKeytrackToggle[i]->move( 120 + 250 - scrollVal, 200 );
		m_subSampLenKnob[i]->move( 90 + 250 - scrollVal, 165 );
		m_subNoiseToggle[i]->move( 120 + 250 - scrollVal, 230 );
	}

	for( int i = 0; i < 256; i++ )
	{
		m_modInBox[i]->move( 40 + 500 - scrollVal, i*60 );
		m_modInNumBox[i]->move( 80 + 500 - scrollVal, i*60 );
		m_modInAmntKnob[i]->move( 120 + 500 - scrollVal, i*60 );
		m_modInCurveKnob[i]->move( 160 + 500 - scrollVal, i*60 );
		m_modOutSecBox[i]->move( 40 + 500 - scrollVal, i*60+30 );
		m_modOutSigBox[i]->move( 80 + 500 - scrollVal, i*60+30 );
		m_modOutSecNumBox[i]->move( 120 + 500 - scrollVal, i*60+30 );
	}
	
	
	m_visvolKnob->move( 80 - scrollVal, 165 );
	
	m_loadAlgKnob->move( 110 - scrollVal, 195 );
	m_loadChnlKnob->move( 130 - scrollVal, 195 );
	m_interpolationToggle->move( 1310 - scrollVal, 207 );
	m_normalizeToggle->move( 1310 - scrollVal, 221 );
	m_visualizeToggle->move( 131 - scrollVal, 235 );
	subNumBox->move( 250 + 10 - scrollVal, 220 );
	mainNumBox->move( 10 - scrollVal, 220 );
	m_graph->move( scrollVal >= 250 ? 250 + 23 - scrollVal : 23 , 13 );
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
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("subgraph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-1.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else if( scrollVal >= 2.5 && scrollVal <= 3 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("subgraph");
		QPixmap graphImg2 = PLUGIN_NAME::getIconPixmap("noisegraph");
		QPainter painter;
		painter.begin(&graphImg);
		painter.setOpacity((scrollVal-2.5f)/0.5f);
		painter.drawPixmap(0, 0, graphImg2);
		painter.end();
		pal.setBrush( backgroundRole(), graphImg );
	}
	else if( scrollVal >= 3.5 && scrollVal <= 4 )
	{
		QPixmap graphImg = PLUGIN_NAME::getIconPixmap("noisegraph");
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
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("subgraph") );
				break;
			case 3:
				pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("noisegraph") );
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
	for( int i = 0; i < 8; i ++ )
	{
		m_morph[i]->setRange( m_morph[i]->getMin(), m_morphMax[i]->value(), m_morph[i]->getStep() );
	}
}


void Microwave::sampLenChanged()
{
	for( int i = 0; i < 8; i ++ )
	{
		m_morphMax[i]->setRange( m_morphMax[i]->getMin(), 262144.f/m_sampLen[i]->value() - 2, m_morphMax[i]->getStep() );
		m_modify[i]->setRange( m_modify[i]->getMin(), m_sampLen[i]->value() - 1, m_modify[i]->getStep() );
	}
}




void MicrowaveView::mainNumChanged()
{
	for( int i = 0; i < 8; i++ )
	{
		m_morphKnob[i]->hide();
		m_rangeKnob[i]->hide();
		m_sampLenKnob[i]->hide();
		m_morphMaxKnob[i]->hide();
		m_modifyKnob[i]->hide();
		m_sprayKnob[i]->hide();
		m_unisonVoicesKnob[i]->hide();
		m_unisonDetuneKnob[i]->hide();
		m_unisonMorphKnob[i]->hide();
		m_unisonModifyKnob[i]->hide();
		m_detuneKnob[i]->hide();
		m_phaseKnob[i]->hide();
		m_phaseRandKnob[i]->hide();
		m_volKnob[i]->hide();
		if( castModel<Microwave>()->m_mainNum.value() - 1 == i )
		{
			m_morphKnob[i]->show();
			m_rangeKnob[i]->show();
			m_sampLenKnob[i]->show();
			m_morphMaxKnob[i]->show();
			m_modifyKnob[i]->show();
			m_sprayKnob[i]->show();
			m_unisonVoicesKnob[i]->show();
			m_unisonDetuneKnob[i]->show();
			m_unisonMorphKnob[i]->show();
			m_unisonModifyKnob[i]->show();
			m_detuneKnob[i]->show();
			m_phaseKnob[i]->show();
			m_phaseRandKnob[i]->show();
			m_volKnob[i]->show();
		}
	}
}

void MicrowaveView::modOutSecChanged( int i )
{
	switch( castModel<Microwave>()->m_modOutSec[i]->value() )// Changing boxes may cause problems if original value was larger than the current number of boxes
	{
		case 0:// None
			m_modOutSigBox[i]->hide();
			m_modOutSecNumBox[i]->hide();
			break;
		case 1:// Main OSC
			m_modOutSigBox[i]->show();
			m_modOutSecNumBox[i]->show();
			castModel<Microwave>()->m_modOutSig[i]->clear();
			mainoscsignalsmodel( castModel<Microwave>()->m_modOutSig[i] )
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		case 2:// Sub OSC
			m_modOutSigBox[i]->show();
			m_modOutSecNumBox[i]->show();
			castModel<Microwave>()->m_modOutSig[i]->clear();
			subsignalsmodel( castModel<Microwave>()->m_modOutSig[i] )
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 32.f, 1.f );
			break;
		case 3:// Filter Input
			m_modOutSigBox[i]->show();
			m_modOutSecNumBox[i]->show();
			castModel<Microwave>()->m_modOutSig[i]->clear();
			mod8model( castModel<Microwave>()->m_modOutSig[i] )
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		case 4:// Filter Parameters
			m_modOutSigBox[i]->show();
			m_modOutSecNumBox[i]->show();
			castModel<Microwave>()->m_modOutSig[i]->clear();
			filtersignalsmodel( castModel<Microwave>()->m_modOutSig[i] )
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		default:
			break;
	}
}

void MicrowaveView::subNumChanged()
{
	for( int i = 0; i < 1024; i++ )
	{
		castModel<Microwave>()->m_graph.setSampleAt( i, castModel<Microwave>()->subs[(castModel<Microwave>()->m_subNum.value()-1)*1024+i] );
	}
	//m_subEnabled.setValue( subEnableds[m_subNum.value()-1] );
	for( int i = 0; i < 128; i++ )
	{
		if( i != castModel<Microwave>()->m_subNum.value()-1 )
		{
			m_subEnabledToggle[i]->hide();
			m_subVolKnob[i]->hide();
			m_subPhaseKnob[i]->hide();
			m_subPhaseRandKnob[i]->hide();
			m_subDetuneKnob[i]->hide();
			m_subMutedToggle[i]->hide();
			m_subKeytrackToggle[i]->hide();
			m_subSampLenKnob[i]->hide();
			m_subNoiseToggle[i]->hide();
		}
		else
		{
			m_subEnabledToggle[i]->show();
			m_subVolKnob[i]->show();
			m_subPhaseKnob[i]->show();
			m_subPhaseRandKnob[i]->show();
			m_subDetuneKnob[i]->show();
			m_subMutedToggle[i]->show();
			m_subKeytrackToggle[i]->show();
			m_subSampLenKnob[i]->show();
			m_subNoiseToggle[i]->show();
		}
	}
}


void Microwave::samplesChanged( int _begin, int _end )
{
	normalize();
	//engine::getSongEditor()->setModified();
	switch( currentTab )
	{
		case 0:
			break;
		case 1:
			for( int i = _begin; i < _end; i++ )
			{
				subs[i + ( (m_subNum.value()-1) * 1024 )] = m_graph.samples()[i];
			}
			break;
		case 2:
			break;
		case 3:
			break;
	}
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
		float m_phaseArr[8] = {0};// CPU may be a problem here
		float m_phaseRandArr[8] = {0};
		int m_sampLenArr[8] = {0};
		for( int i = 0; i < 8; i++ )
		{
			m_phaseArr[i] = m_phase[i]->value();
			m_phaseRandArr[i] = m_phaseRand[i]->value();
			m_sampLenArr[i] = m_sampLen[i]->value();
		}
		_n->m_pluginData = new bSynth(
					_n,
					factor,
				Engine::mixer()->processingSampleRate(),
					m_phaseArr, m_phaseRandArr, m_sampLenArr );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	bSynth * ps = static_cast<bSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		int m_modifyModeArr[8] = {0};// WARNING: This entire section here is likely causing a huge CPU problem
		float m_modifyArr[8] = {0};
		float m_morphArr[8] = {0};
		float m_rangeArr[8] = {0};
		float m_sprayArr[8] = {0};
		float m_unisonVoicesArr[8] = {0};
		float m_unisonDetuneArr[8] = {0};
		float m_unisonMorphArr[8] = {0};
		float m_unisonModifyArr[8] = {0};
		float m_morphMaxArr[8] = {0};
		float m_detuneArr[8] = {0};
		int m_sampLenArr[8] = {0};
		int m_modInArr[256] = {0};
		int m_modInNumArr[256] = {0};
		float m_modInAmntArr[256] = {0};
		float m_modInCurveArr[256] = {0};
		int m_modIn2Arr[256] = {0};
		int m_modInNum2Arr[256] = {0};
		float m_modInAmnt2Arr[256] = {0};
		float m_modInCurve2Arr[256] = {0};
		int m_modOutSecArr[256] = {0};
		int m_modOutSigArr[256] = {0};
		int m_modOutSecNumArr[256] = {0};
		float m_phaseArr[8] = {0};
		float m_volArr[8] = {0};
		
		bool m_subEnabledArr[128] = {0};
		float m_subVolArr[128] = {0};
		float m_subPhaseArr[128] = {0};
		float m_subPhaseRandArr[128] = {0};
		float m_subDetuneArr[128] = {0};
		bool m_subMutedArr[128] = {0};
		bool m_subKeytrackArr[128] = {0};
		float m_subSampLenArr[128] = {0};
		bool m_subNoiseArr[128] = {0};
		
		float m_filtInVolArr[8] = {0};
		int m_filtTypeArr[8] = {0};
		int m_filtSlopeArr[8] = {0};
		float m_filtCutoffArr[8] = {0};
		float m_filtResoArr[8] = {0};
		float m_filtGainArr[8] = {0};
		float m_filtSatuArr[8] = {0};
		float m_filtWetDryArr[8] = {0};
		float m_filtBalArr[8] = {0};
		float m_filtOutVolArr[8] = {0};
		bool m_filtEnabledArr[8] = {false};
		for( int i = 0; i < 8; i++ )
		{
			m_modifyModeArr[i] = m_modifyMode[i]->value();
			m_modifyArr[i] = m_modify[i]->value();
			m_morphArr[i] = m_morph[i]->value();
			m_rangeArr[i] = m_range[i]->value();
			m_sprayArr[i] = m_spray[i]->value();
			m_unisonVoicesArr[i] = m_unisonVoices[i]->value();
			m_unisonDetuneArr[i] = m_unisonDetune[i]->value();
			m_unisonMorphArr[i] = m_unisonMorph[i]->value();
			m_unisonModifyArr[i] = m_unisonModify[i]->value();
			m_morphMaxArr[i] = m_morphMax[i]->value();
			m_detuneArr[i] = m_detune[i]->value();
			m_sampLenArr[i] = m_sampLen[i]->value();
			m_modInArr[i] = m_modIn[i]->value();
			m_modInNumArr[i] = m_modInNum[i]->value();
			m_modInAmntArr[i] = m_modInAmnt[i]->value();
			m_modInCurveArr[i] = m_modInCurve[i]->value();
			m_modIn2Arr[i] = m_modIn2[i]->value();
			m_modInNum2Arr[i] = m_modInNum2[i]->value();
			m_modInAmnt2Arr[i] = m_modInAmnt2[i]->value();
			m_modInCurve2Arr[i] = m_modInCurve2[i]->value();
			m_modOutSecArr[i] = m_modOutSec[i]->value();
			m_modOutSigArr[i] = m_modOutSig[i]->value();
			m_modOutSecNumArr[i] = m_modOutSecNum[i]->value();
			m_phaseArr[i] = m_phase[i]->value();
			m_volArr[i] = m_vol[i]->value();

			m_filtInVolArr[i] = m_filtInVol[i]->value();
			m_filtTypeArr[i] = m_filtType[i]->value();
			m_filtSlopeArr[i] = m_filtSlope[i]->value();
			m_filtCutoffArr[i] = m_filtCutoff[i]->value();
			m_filtResoArr[i] = m_filtReso[i]->value();
			m_filtGainArr[i] = m_filtGain[i]->value();
			m_filtSatuArr[i] = m_filtSatu[i]->value();
			m_filtWetDryArr[i] = m_filtWetDry[i]->value();
			m_filtBalArr[i] = m_filtBal[i]->value();
			m_filtOutVolArr[i] = m_filtOutVol[i]->value();
			m_filtEnabledArr[i] = m_filtEnabled[i]->value();
		}

		for( int i = 0; i < 128; i++ )
		{
			m_subEnabledArr[i] = m_subEnabled[i]->value();
			m_subVolArr[i] = m_subVol[i]->value();
			m_subPhaseArr[i] = m_subPhase[i]->value();
			m_subPhaseRandArr[i] = m_subPhaseRand[i]->value();
			m_subDetuneArr[i] = m_subDetune[i]->value();
			m_subMutedArr[i] = m_subMuted[i]->value();
			m_subKeytrackArr[i] = m_subKeytrack[i]->value();
			m_subSampLenArr[i] = m_subSampLen[i]->value();
			m_subNoiseArr[i] = m_subNoise[i]->value();
		}

		std::vector<float> sample = ps->nextStringSample( m_modifyModeArr, m_modifyArr, m_morphArr, m_rangeArr, m_sprayArr, m_unisonVoicesArr, m_unisonDetuneArr, m_unisonMorphArr, m_unisonModifyArr, m_morphMaxArr, m_detuneArr, waveforms, subs, m_subEnabledArr, m_subVolArr, m_subPhaseArr, m_subPhaseRandArr, m_subDetuneArr, m_subMutedArr, m_subKeytrackArr, m_subSampLenArr, m_subNoiseArr, m_sampLenArr, m_modInArr, m_modInNumArr, m_modInAmntArr, m_modInCurveArr, m_modIn2Arr, m_modInNum2Arr, m_modInAmnt2Arr, m_modInCurve2Arr, m_modOutSecArr, m_modOutSigArr, m_modOutSecNumArr, m_phaseArr, m_volArr, m_filtInVolArr, m_filtTypeArr, m_filtSlopeArr, m_filtCutoffArr, m_filtResoArr, m_filtGainArr, m_filtSatuArr, m_filtWetDryArr, m_filtBalArr, m_filtOutVolArr, m_filtEnabledArr );

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = sample[chnl];
		}

		if( m_visualize.value() ) //update visualizer
		{
			if( abs( const_cast<float*>( m_graph.samples() )[int(ps->getRealIndex())] - ( ((sample[0]+sample[1])/2.f) * m_visvol.value() / 100 ) ) >= 0.01f )
			{
				m_graph.setSampleAt( ps->getRealIndex(), ((sample[0]+sample[1])/2.f) * m_visvol.value() / 100 );
			}		
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

	for( int i = 0; i < 8; i++ )
	{
		m_morphKnob[i] = new Knob( knobDark_28, this );
		m_morphKnob[i]->move( 20, 165 );
		m_morphKnob[i]->setHintText( tr( "Morph" ), "" );
	
		m_rangeKnob[i] = new Knob( knobDark_28, this );
		m_rangeKnob[i]->move( 50, 165 );
		m_rangeKnob[i]->setHintText( tr( "Range" ), "" );

		m_sampLenKnob[i] = new Knob( knobDark_28, this );
		m_sampLenKnob[i]->move( 50, 185 );
		m_sampLenKnob[i]->setHintText( tr( "Waveform Sample Length" ), "" );

		m_morphMaxKnob[i] = new Knob( knobDark_28, this );
		m_morphMaxKnob[i]->move( 20, 195 );
		m_morphMaxKnob[i]->setHintText( tr( "Morph Max" ), "" );

		m_modifyKnob[i] = new Knob( knobDark_28, this );
		m_modifyKnob[i]->move( 110, 165 );
		m_modifyKnob[i]->setHintText( tr( "Modify" ), "" );

		m_sprayKnob[i] = new Knob( knobDark_28, this );
		m_sprayKnob[i]->move( 140, 165 );
		m_sprayKnob[i]->setHintText( tr( "Spray" ), "" );

		m_unisonVoicesKnob[i] = new Knob( knobDark_28, this );
		m_unisonVoicesKnob[i]->move( 170, 165 );
		m_unisonVoicesKnob[i]->setHintText( tr( "Unison Voices" ), "" );

		m_unisonDetuneKnob[i] = new Knob( knobDark_28, this );
		m_unisonDetuneKnob[i]->move( 200, 165 );
		m_unisonDetuneKnob[i]->setHintText( tr( "Unison Detune" ), "" );

		m_unisonMorphKnob[i] = new Knob( knobDark_28, this );
		m_unisonMorphKnob[i]->move( 170, 195 );
		m_unisonMorphKnob[i]->setHintText( tr( "Unison Morph" ), "" );

		m_unisonModifyKnob[i] = new Knob( knobDark_28, this );
		m_unisonModifyKnob[i]->move( 200, 195 );
		m_unisonModifyKnob[i]->setHintText( tr( "Unison Modify" ), "" );

		m_detuneKnob[i] = new Knob( knobSmall_17, this );
		m_detuneKnob[i]->move( 100, 5 );
		m_detuneKnob[i]->setHintText( tr( "Detune" ), "" );

		m_phaseKnob[i] = new Knob( knobSmall_17, this );
		m_phaseKnob[i]->move( 50, 215 );
		m_phaseKnob[i]->setHintText( tr( "Phase" ), "" );

		m_phaseRandKnob[i] = new Knob( knobSmall_17, this );
		m_phaseRandKnob[i]->move( 80, 215 );
		m_phaseRandKnob[i]->setHintText( tr( "Phase Randomness" ), "" );

		m_volKnob[i] = new Knob( knobDark_28, this );
		m_volKnob[i]->move( 170, 195 );
		m_volKnob[i]->setHintText( tr( "Volume" ), "" );

		m_modifyModeBox[i] = new ComboBox( this );
		m_modifyModeBox[i]->setGeometry( 0, 5, 42, 22 );
		m_modifyModeBox[i]->setFont( pointSize<8>( m_modifyModeBox[i]->font() ) );


		//float * filtInVol, int * filtType, int * filtSlope, float * filtCutoff, float * filtReso, float * filtGain, float * filtSatu, float * filtWetDry, float * filtBal, float * filtOutVol
		m_filtInVolKnob[i] = new Knob( knobDark_28, this );
		m_filtInVolKnob[i]->move( 1400, 165 );
		m_filtInVolKnob[i]->setHintText( tr( "Input Volume" ), "" );

		m_filtTypeBox[i] = new ComboBox( this );
		m_filtTypeBox[i]->setGeometry( 1000, 5, 42, 22 );
		m_filtTypeBox[i]->setFont( pointSize<8>( m_filtTypeBox[i]->font() ) );

		m_filtSlopeBox[i] = new ComboBox( this );
		m_filtSlopeBox[i]->setGeometry( 1000, 5, 42, 22 );
		m_filtSlopeBox[i]->setFont( pointSize<8>( m_filtSlopeBox[i]->font() ) );

		m_filtCutoffKnob[i] = new Knob( knobDark_28, this );
		m_filtCutoffKnob[i]->move( 1400, 165 );
		m_filtCutoffKnob[i]->setHintText( tr( "Cutoff Frequency" ), "" );

		m_filtResoKnob[i] = new Knob( knobDark_28, this );
		m_filtResoKnob[i]->move( 1400, 165 );
		m_filtResoKnob[i]->setHintText( tr( "Resonance" ), "" );

		m_filtGainKnob[i] = new Knob( knobDark_28, this );
		m_filtGainKnob[i]->move( 1400, 165 );
		m_filtGainKnob[i]->setHintText( tr( "db Gain" ), "" );

		m_filtSatuKnob[i] = new Knob( knobDark_28, this );
		m_filtSatuKnob[i]->move( 1400, 165 );
		m_filtSatuKnob[i]->setHintText( tr( "Saturation" ), "" );

		m_filtWetDryKnob[i] = new Knob( knobDark_28, this );
		m_filtWetDryKnob[i]->move( 1400, 165 );
		m_filtWetDryKnob[i]->setHintText( tr( "Wet/Dry" ), "" );

		m_filtBalKnob[i] = new Knob( knobDark_28, this );
		m_filtBalKnob[i]->move( 1400, 165 );
		m_filtBalKnob[i]->setHintText( tr( "Balance/Panning" ), "" );

		m_filtOutVolKnob[i] = new Knob( knobDark_28, this );
		m_filtOutVolKnob[i]->move( 1400, 165 );
		m_filtOutVolKnob[i]->setHintText( tr( "Output Volume" ), "" );

		m_filtEnabledToggle[i] = new LedCheckBox( "Filter Enabled", this, tr( "Filter Enabled" ), LedCheckBox::Green );
		m_filtEnabledToggle[i]->move( 1400, 165 );

		if( i != 0 )
		{
			m_morphKnob[i]->hide();
			m_rangeKnob[i]->hide();
			m_sampLenKnob[i]->hide();
			m_morphMaxKnob[i]->hide();
			m_modifyKnob[i]->hide();
			m_sprayKnob[i]->hide();
			m_unisonVoicesKnob[i]->hide();
			m_unisonDetuneKnob[i]->hide();
			m_unisonMorphKnob[i]->hide();
			m_unisonModifyKnob[i]->hide();
			m_detuneKnob[i]->hide();
			m_phaseKnob[i]->hide();
			m_phaseRandKnob[i]->hide();
			m_modifyModeBox[i]->hide();
			m_volKnob[i]->hide();
		}
	}

	for( int i = 0; i < 128; i++ )
	{
		//float * subVol, float * subPhase, float * subPhaseRand, float * subDetune, bool * subMuted, bool * subKeytrack, float * subSampLen

		m_subEnabledToggle[i] = new LedCheckBox( "Sub Oscillator Enabled", this, tr( "Sub Oscillator Enabled" ), LedCheckBox::Green );
		m_subEnabledToggle[i]->move( 131, 235 );

		m_subVolKnob[i] = new Knob( knobDark_28, this );
		m_subVolKnob[i]->move( 100, 5 );
		m_subVolKnob[i]->setHintText( tr( "Sub Oscillator Volume" ), "" );

		m_subPhaseKnob[i] = new Knob( knobDark_28, this );
		m_subPhaseKnob[i]->move( 100, 5 );
		m_subPhaseKnob[i]->setHintText( tr( "Sub Oscillator Phase" ), "" );

		m_subPhaseRandKnob[i] = new Knob( knobDark_28, this );
		m_subPhaseRandKnob[i]->move( 100, 5 );
		m_subPhaseRandKnob[i]->setHintText( tr( "Sub Oscillator Phase Randomness" ), "" );

		m_subDetuneKnob[i] = new Knob( knobDark_28, this );
		m_subDetuneKnob[i]->move( 100, 5 );
		m_subDetuneKnob[i]->setHintText( tr( "Sub Oscillator Pitch" ), "" );

		m_subMutedToggle[i] = new LedCheckBox( "Sub Oscillator Muted", this, tr( "Sub Oscillator Muted" ), LedCheckBox::Green );
		m_subMutedToggle[i]->move( 131, 235 );

		m_subKeytrackToggle[i] = new LedCheckBox( "Sub Oscillator Keytracking Enabled", this, tr( "Sub Oscillator Keytracking Enabled" ), LedCheckBox::Green );
		m_subKeytrackToggle[i]->move( 131, 235 );

		m_subSampLenKnob[i] = new Knob( knobDark_28, this );
		m_subSampLenKnob[i]->move( 100, 5 );
		m_subSampLenKnob[i]->setHintText( tr( "Sub Oscillator Sample Length" ), "" );

		m_subNoiseToggle[i] = new LedCheckBox( "Sub Noise Enabled", this, tr( "Sub Noise Enabled" ), LedCheckBox::Green );
		m_subNoiseToggle[i]->move( 131, 235 );

		if( i != 0 )
		{
			m_subEnabledToggle[i]->hide();
			m_subVolKnob[i]->hide();
			m_subPhaseKnob[i]->hide();
			m_subPhaseRandKnob[i]->hide();
			m_subDetuneKnob[i]->hide();
			m_subMutedToggle[i]->hide();
			m_subKeytrackToggle[i]->hide();
			m_subSampLenKnob[i]->hide();
			m_subNoiseToggle[i]->hide();
		}


	}

	for( int i = 0; i < 256; i++ )
	{
		m_modOutSecBox[i] = new ComboBox( this );
		m_modOutSecBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modOutSecBox[i]->setFont( pointSize<8>( m_modOutSecBox[i]->font() ) );
		//m_modOutSecBox[i]->hide();

		m_modOutSigBox[i] = new ComboBox( this );
		m_modOutSigBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modOutSigBox[i]->setFont( pointSize<8>( m_modOutSigBox[i]->font() ) );
		//m_modOutSigBox[i]->hide();

		m_modOutSecNumBox[i] = new LcdSpinBox( 3, this, "Mod Output Number" );
		m_modOutSecNumBox[i]->move( 2000, 5 );
		//m_modOutSecNumBox[i]->hide();

		m_modInBox[i] = new ComboBox( this );
		m_modInBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modInBox[i]->setFont( pointSize<8>( m_modInBox[i]->font() ) );
		//m_modInBox[i]->hide();

		m_modInNumBox[i] = new LcdSpinBox( 3, this, "Mod Input Number" );
		m_modInNumBox[i]->move( 2000, 5 );
		//m_modInNumBox[i]->hide();

		m_modInAmntKnob[i] = new Knob( knobDark_28, this );
		m_modInAmntKnob[i]->move( 8000, 215 );
		m_modInAmntKnob[i]->setHintText( tr( "Modulator Amount" ), "" );
		//m_modInAmntKnob[i]->hide();

		m_modInCurveKnob[i] = new Knob( knobDark_28, this );
		m_modInCurveKnob[i]->move( 8000, 215 );
		m_modInCurveKnob[i]->setHintText( tr( "Modulator Curve" ), "" );
		//m_modInCurveKnob[i]->hide();

		m_modInBox2[i] = new ComboBox( this );
		m_modInBox2[i]->setGeometry( 2000, 5, 42, 22 );
		m_modInBox2[i]->setFont( pointSize<8>( m_modInBox2[i]->font() ) );
		//m_modInBox2[i]->hide();

		m_modInNumBox2[i] = new LcdSpinBox( 3, this, "Secondary Mod Input Number" );
		m_modInNumBox2[i]->move( 2000, 5 );
		//m_modInNumBox2[i]->hide();

		m_modInAmntKnob2[i] = new Knob( knobDark_28, this );
		m_modInAmntKnob2[i]->move( 8000, 215 );
		m_modInAmntKnob2[i]->setHintText( tr( "Secondary Modulator Amount" ), "" );
		//m_modInAmntKnob2[i]->hide();

		m_modInCurveKnob2[i] = new Knob( knobDark_28, this );
		m_modInCurveKnob2[i]->move( 8000, 215 );
		m_modInCurveKnob2[i]->setHintText( tr( "Secondary Modulator Curve" ), "" );
		//m_modInCurveKnob2[i]->hide();
	}

	m_scrollKnob = new Knob( knobSmall_17, this );
	m_scrollKnob->move( 5, 135 );
	//m_scrollKnob->setDescription( "" );

	m_visvolKnob = new Knob( knobDark_28, this );
	m_visvolKnob->move( 80, 165 );
	m_visvolKnob->setHintText( tr( "Visualizer Volume" ), "" );

	m_loadAlgKnob = new Knob( knobDark_28, this );
	m_loadAlgKnob->move( 110, 195 );
	m_loadAlgKnob->setHintText( tr( "Wavetable Loading Algorithm" ), "" );

	m_loadChnlKnob = new Knob( knobDark_28, this );
	m_loadChnlKnob->move( 130, 195 );
	m_loadChnlKnob->setHintText( tr( "Wavetable Loading Channel" ), "" );


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


	m_interpolationToggle = new LedCheckBox( "Interpolation", this, tr( "Interpolation" ), LedCheckBox::Yellow );
	m_interpolationToggle->move( 131, 207 );

	m_normalizeToggle = new LedCheckBox( "Normalize", this, tr( "Normalize" ), LedCheckBox::Green );
	m_normalizeToggle->move( 131, 221 );

	m_visualizeToggle = new LedCheckBox( "Visualize", this, tr( "Visualize" ), LedCheckBox::Green );
	m_visualizeToggle->move( 131, 235 );

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

	subNumBox = new LcdSpinBox( 3, this, "Sub Oscillator Number" );
	subNumBox->move( 10, 220 );

	mainNumBox = new LcdSpinBox( 2, this, "Oscillator Number" );
	mainNumBox->move( 10, 220 );
	
	
	
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

	connect( &castModel<Microwave>()->m_mainNum, SIGNAL( dataChanged( ) ),
			this, SLOT( mainNumChanged( ) ) );

	connect( &castModel<Microwave>()->m_subNum, SIGNAL( dataChanged( ) ),
			this, SLOT( subNumChanged( ) ) );

	for( int i = 0; i < 256; i++ )
	{
		connect( castModel<Microwave>()->m_modOutSec[i], &ComboBoxModel::dataChanged,
            		this, [this, i]() { modOutSecChanged(i); } );
	}

	updateScroll();

}




void MicrowaveView::modelChanged()
{
	Microwave * b = castModel<Microwave>();

	for( int i = 0; i < 8; i++ )
	{
		m_morphKnob[i]->setModel( b->m_morph[i] );
		m_rangeKnob[i]->setModel( b->m_range[i] );
		m_sampLenKnob[i]->setModel( b->m_sampLen[i] );
		m_modifyKnob[i]->setModel( b->m_modify[i] );
		m_morphMaxKnob[i]->setModel( b->m_morphMax[i] );
		m_sprayKnob[i]->setModel( b->m_spray[i] );
		m_unisonVoicesKnob[i]->setModel( b->m_unisonVoices[i] );
		m_unisonDetuneKnob[i]->setModel( b->m_unisonDetune[i] );
		m_unisonMorphKnob[i]->setModel( b->m_unisonMorph[i] );
		m_unisonModifyKnob[i]->setModel( b->m_unisonModify[i] );
		m_detuneKnob[i]->setModel( b->m_detune[i] );
		m_modifyModeBox[i]-> setModel( b-> m_modifyMode[i] );
		m_phaseKnob[i]->setModel( b->m_phase[i] );
		m_phaseRandKnob[i]->setModel( b->m_phaseRand[i] );
		m_volKnob[i]->setModel( b->m_vol[i] );

		m_filtInVolKnob[i]->setModel( b->m_filtInVol[i] );
		m_filtTypeBox[i]->setModel( b->m_filtType[i] );
		m_filtSlopeBox[i]->setModel( b->m_filtSlope[i] );
		m_filtCutoffKnob[i]->setModel( b->m_filtCutoff[i] );
		m_filtResoKnob[i]->setModel( b->m_filtReso[i] );
		m_filtGainKnob[i]->setModel( b->m_filtGain[i] );
		m_filtSatuKnob[i]->setModel( b->m_filtSatu[i] );
		m_filtWetDryKnob[i]->setModel( b->m_filtWetDry[i] );
		m_filtBalKnob[i]->setModel( b->m_filtBal[i] );
		m_filtOutVolKnob[i]->setModel( b->m_filtOutVol[i] );
		m_filtEnabledToggle[i]->setModel( b->m_filtEnabled[i] );
	}

	for( int i = 0; i < 128; i++ )
	{
		m_subEnabledToggle[i]->setModel( b->m_subEnabled[i] );
		m_subVolKnob[i]->setModel( b->m_subVol[i] );
		m_subPhaseKnob[i]->setModel( b->m_subPhase[i] );
		m_subPhaseRandKnob[i]->setModel( b->m_subPhaseRand[i] );
		m_subDetuneKnob[i]->setModel( b->m_subDetune[i] );
		m_subMutedToggle[i]->setModel( b->m_subMuted[i] );
		m_subKeytrackToggle[i]->setModel( b->m_subKeytrack[i] );
		m_subSampLenKnob[i]->setModel( b->m_subSampLen[i] );
		m_subNoiseToggle[i]->setModel( b->m_subNoise[i] );
	}

	for( int i = 0; i < 256; i++ )
	{
		m_modOutSecBox[i]-> setModel( b-> m_modOutSec[i] );
		m_modOutSigBox[i]-> setModel( b-> m_modOutSig[i] );
		m_modOutSecNumBox[i]-> setModel( b-> m_modOutSecNum[i] );

		m_modInBox[i]-> setModel( b-> m_modIn[i] );
		m_modInNumBox[i]-> setModel( b-> m_modInNum[i] );
		m_modInAmntKnob[i]-> setModel( b-> m_modInAmnt[i] );
		m_modInCurveKnob[i]-> setModel( b-> m_modInCurve[i] );

		m_modInBox2[i]-> setModel( b-> m_modIn2[i] );
		m_modInNumBox2[i]-> setModel( b-> m_modInNum2[i] );
		m_modInAmntKnob2[i]-> setModel( b-> m_modInAmnt2[i] );
		m_modInCurveKnob2[i]-> setModel( b-> m_modInCurve2[i] );
	}

	m_graph->setModel( &b->m_graph );
	m_visvolKnob->setModel( &b->m_visvol );
	m_interpolationToggle->setModel( &b->m_interpolation );
	m_normalizeToggle->setModel( &b->m_normalize );
	m_visualizeToggle->setModel( &b->m_visualize );
	subNumBox->setModel( &b->m_subNum );
	m_loadAlgKnob->setModel( &b->m_loadAlg );
	m_loadChnlKnob->setModel( &b->m_loadChnl );
	m_scrollKnob->setModel( &b->m_scroll );
	mainNumBox->setModel( &b->m_mainNum );
	

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


void Microwave::waveClicked( int wavenum )
{
	/*for( int j = 0; j < m_sampLen.value(); j++ )
	{
		waveforms[int( ( 262144 / m_sampLen.value() ) * wavenum )] = m_graph.samples()[int( ( 262144 / m_sampLen.value() ) * wavenum )];
	}*/
}

void Microwave::setGraphLength( float graphLen )
{
	m_graph.setLength( graphLen );
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
				mainNumChanged();
				castModel<Microwave>()->m_graph.setLength( 1024 );
				break;
			case 1:
				subNumChanged();
				castModel<Microwave>()->m_graph.setLength( 1024 );
				break;
			case 2:
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
	int oscilNum = castModel<Microwave>()->m_mainNum.value() - 1;
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*44100.f);//in samples
		switch( algorithm )
		{
			case 0:// Squeeze entire sample into 256 waveforms
			{
				castModel<Microwave>()->m_morphMax[oscilNum]->setValue( 262144.f/castModel<Microwave>()->m_sampLen[oscilNum]->value() );
				castModel<Microwave>()->morphMaxChanged();
				for( int i = 0; i < 262144; i++ )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( qBound( 0.f, i/262144.f, 1.f ), channel );
				}
				break;
			}
			case 1:// Load sample without changes, cut off end if too long
			{
				for( int i = 0; i < 262144; i++ )
				{
					if ( i <= lengthOfSample )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( i/lengthOfSample, channel );
					}
					else
					{
						castModel<Microwave>()->m_morphMax[oscilNum]->setValue( i/castModel<Microwave>()->m_sampLen[oscilNum]->value() );
						castModel<Microwave>()->morphMaxChanged();
						for( int j = i; j < 262144; j++ ) { castModel<Microwave>()->waveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 2:// Attempt to guess at the frequency of the sound and adjust accordingly
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
					//castModel<Microwave>()->waveforms[oscilNum][qBound(0,i,262144)] = sampleBuffer->userWaveSample(i/lengthOfSample, channel);
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
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( ( ( realCycleLen / 1024.f ) * i ) / ( ( cycleNum * realCycleLen ) ), channel );
				}
				break;
			}
			case 3:// The same as case 1??  I don't even know, check later...
			{
				for( int i = 0; i < 262144; i++ )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( qBound( 0.f, i/(lengthOfSample), 1.f ), channel );
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


