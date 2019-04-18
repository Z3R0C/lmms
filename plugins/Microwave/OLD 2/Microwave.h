/*
 * Microwave.h - declaration of class Microwave which
 *                         is a wavetable synthesizer
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


#ifndef Microwave_H
#define Microwave_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Graph.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "LedCheckbox.h"
#include "MemoryManager.h"
#include "LcdSpinBox.h"
#include "SampleBuffer.h"
#include "ComboBox.h"
#include "stdshims.h"


class oscillator;
class MicrowaveView;

class Microwave : public Instrument
{
	Q_OBJECT

#define setwavemodel( name )\
		name ->addItem( tr( "Pulse Width" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Stretch Left Half" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name ->addItem( tr( "Shrink Right Half To Left Edge" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Weird 1" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Asym To Right" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Asym To Left" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "Weird 2" ), make_unique<PluginPixmapLoader>( "sqrsoft" ) );\
		name ->addItem( tr( "Stretch From Center" ), make_unique<PluginPixmapLoader>( "sinabs" ) );\
		name ->addItem( tr( "Squish To Center" ), make_unique<PluginPixmapLoader>( "exp" ) );\
		name ->addItem( tr( "Stretch And Squish" ), make_unique<PluginPixmapLoader>( "noise" ) );\
		name ->addItem( tr( "Add Tail" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name ->addItem( tr( "Cut Off Right" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Cut Off Left" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Flip" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "" ), make_unique<PluginPixmapLoader>( "moog" ) );

#define signalsmodel( name )\
		name .addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name .addItem( tr( "Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name .addItem( tr( "Range" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name .addItem( tr( "Modify" ), make_unique<PluginPixmapLoader>( "sqr" ) );

#define modsectionsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Main OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Step Sequencer" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Envelope" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "LFO" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name ->addItem( tr( "Sub OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Noise OSC" ), make_unique<PluginPixmapLoader>( "noise" ) );

#define mainoscsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name ->addItem( tr( "Range" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Modify" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "ramp" ) );

#define stepseqsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );

#define envsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );

#define lfosignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );

#define subsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );

#define noisesignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );

#define modinmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Main OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Step Sequencer" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Envelope" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "LFO" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name ->addItem( tr( "Sub OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Noise OSC" ), make_unique<PluginPixmapLoader>( "noise" ) );

#define mod8model( name )\
		name ->addItem( tr( "1" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name ->addItem( tr( "2" ), make_unique<PluginPixmapLoader>( "2" ) );\
		name ->addItem( tr( "3" ), make_unique<PluginPixmapLoader>( "3" ) );\
		name ->addItem( tr( "4" ), make_unique<PluginPixmapLoader>( "4" ) );\
		name ->addItem( tr( "5" ), make_unique<PluginPixmapLoader>( "5" ) );\
		name ->addItem( tr( "6" ), make_unique<PluginPixmapLoader>( "6" ) );\
		name ->addItem( tr( "7" ), make_unique<PluginPixmapLoader>( "7" ) );\
		name ->addItem( tr( "8" ), make_unique<PluginPixmapLoader>( "8" ) );

#define mod32model( name )\
		name ->addItem( tr( "1" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name ->addItem( tr( "2" ), make_unique<PluginPixmapLoader>( "2" ) );\
		name ->addItem( tr( "3" ), make_unique<PluginPixmapLoader>( "3" ) );\
		name ->addItem( tr( "4" ), make_unique<PluginPixmapLoader>( "4" ) );\
		name ->addItem( tr( "5" ), make_unique<PluginPixmapLoader>( "5" ) );\
		name ->addItem( tr( "6" ), make_unique<PluginPixmapLoader>( "6" ) );\
		name ->addItem( tr( "7" ), make_unique<PluginPixmapLoader>( "7" ) );\
		name ->addItem( tr( "8" ), make_unique<PluginPixmapLoader>( "8" ) );\
		name ->addItem( tr( "9" ), make_unique<PluginPixmapLoader>( "9" ) );\
		name ->addItem( tr( "10" ), make_unique<PluginPixmapLoader>( "10" ) );\
		name ->addItem( tr( "11" ), make_unique<PluginPixmapLoader>( "11" ) );\
		name ->addItem( tr( "12" ), make_unique<PluginPixmapLoader>( "12" ) );\
		name ->addItem( tr( "13" ), make_unique<PluginPixmapLoader>( "13" ) );\
		name ->addItem( tr( "14" ), make_unique<PluginPixmapLoader>( "14" ) );\
		name ->addItem( tr( "15" ), make_unique<PluginPixmapLoader>( "15" ) );\
		name ->addItem( tr( "16" ), make_unique<PluginPixmapLoader>( "16" ) );\
		name ->addItem( tr( "17" ), make_unique<PluginPixmapLoader>( "17" ) );\
		name ->addItem( tr( "18" ), make_unique<PluginPixmapLoader>( "18" ) );\
		name ->addItem( tr( "19" ), make_unique<PluginPixmapLoader>( "19" ) );\
		name ->addItem( tr( "20" ), make_unique<PluginPixmapLoader>( "20" ) );\
		name ->addItem( tr( "21" ), make_unique<PluginPixmapLoader>( "21" ) );\
		name ->addItem( tr( "22" ), make_unique<PluginPixmapLoader>( "22" ) );\
		name ->addItem( tr( "23" ), make_unique<PluginPixmapLoader>( "23" ) );\
		name ->addItem( tr( "24" ), make_unique<PluginPixmapLoader>( "24" ) );\
		name ->addItem( tr( "25" ), make_unique<PluginPixmapLoader>( "25" ) );\
		name ->addItem( tr( "26" ), make_unique<PluginPixmapLoader>( "26" ) );\
		name ->addItem( tr( "27" ), make_unique<PluginPixmapLoader>( "27" ) );\
		name ->addItem( tr( "28" ), make_unique<PluginPixmapLoader>( "28" ) );\
		name ->addItem( tr( "29" ), make_unique<PluginPixmapLoader>( "29" ) );\
		name ->addItem( tr( "30" ), make_unique<PluginPixmapLoader>( "30" ) );\
		name ->addItem( tr( "31" ), make_unique<PluginPixmapLoader>( "31" ) );\
		name ->addItem( tr( "32" ), make_unique<PluginPixmapLoader>( "32" ) );

public:
	Microwave(InstrumentTrack * _instrument_track );
	virtual ~Microwave();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 64 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );

	void waveClicked( int wavenum );

	void setGraphLength( float graphLen );

protected slots:
	void morphMaxChanged();
	void samplesChanged( int, int );
	void sampLenChanged();
	void envNumChanged();
	void envLenChanged();
	void envEnabledChanged();
	void envSignalChanged();
	void lfoNumChanged();
	void lfoLenChanged();
	void lfoEnabledChanged();
	void lfoSignalChanged();
	void seqNumChanged();
	void seqLenChanged();
	void seqEnabledChanged();
	void seqSignalChanged();
	void stepNumChanged();
	void subEnabledChanged();

	void normalize();


private:
	FloatModel  * m_morph[8];
	FloatModel  * m_range[8];
	FloatModel  * m_sampLen[8];
	FloatModel  m_visvol;
	FloatModel  * m_morphMax[8];
	ComboBoxModel * m_modifyMode[8];
	FloatModel  * m_modify[8];
	ComboBoxModel * m_modOutSec[256];
	ComboBoxModel * m_modOutSig[256];
	ComboBoxModel * m_modOutSecNum[256];
	ComboBoxModel * m_modIn[256];
	ComboBoxModel * m_modInNum[256];
	FloatModel * m_modInAmnt[256];
	FloatModel * m_modInCurve[256];
	ComboBoxModel * m_modIn2[256];
	ComboBoxModel * m_modInNum2[256];
	FloatModel * m_modInAmnt2[256];
	FloatModel * m_modInCurve2[256];
	IntModel  m_modNum;
	FloatModel  * m_spray[8];
	FloatModel  * m_unisonVoices[8];
	FloatModel  * m_unisonDetune[8];
	FloatModel  * m_unisonMorph[8];
	FloatModel  * m_unisonModify[8];
	FloatModel  * m_detune[8];
	IntModel  m_envNum;
	FloatModel  m_envLen;
	ComboBoxModel m_envSignal;
	IntModel  m_lfoNum;
	FloatModel  m_lfoLen;
	ComboBoxModel m_lfoSignal;
	IntModel  m_seqNum;
	FloatModel  m_seqLen;
	ComboBoxModel m_seqSignal;
	IntModel  m_stepNum;
	FloatModel  m_loadAlg;
	FloatModel  m_loadChnl;
	FloatModel  * m_phase[8];
	FloatModel  * m_phaseRand[8];
	FloatModel  * m_vol[8];
	FloatModel  m_scroll;
	IntModel  m_subNum;
	IntModel  m_mainNum;
	
	graphModel  m_graph;
	
	
	BoolModel m_interpolation;
	BoolModel m_normalize;
	BoolModel m_visualize;
	BoolModel m_envEnabled;
	BoolModel m_lfoEnabled;
	BoolModel m_seqEnabled;
	BoolModel * m_subEnabled[32];
	BoolModel m_noiseEnabled;

	
	float m_normalizeFactor;
	float sample_realindex[8][32] = {{0}};
	float sample_subindex[32] = {0};
	float waveforms[8][262144] = {{0}};
	int currentTab = 0;
	int indexOffset = 0;
	float envs[32768] = {0};
	int envLens[32] = {0};
	bool envEnableds[32] = {false};
	int envSignals[32] = {0};
	float lfos[32768] = {0};
	int lfoLens[32] = {0};
	bool lfoEnableds[32] = {false};
	int lfoSignals[32] = {0};
	float seqs[2048] = {0};
	int seqLens[32] = {0};
	bool seqEnableds[32] = {false};
	int seqSignals[32] = {0};
	int stepNums[32] = {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
	float subs[32768] = {0};
	int subNums[32] = {0};
	//bool subEnableds[32] = {false};
	float noises[1024] = {0};

	SampleBuffer m_sampleBuffer;
	
	friend class MicrowaveView;
} ;



class MicrowaveView : public InstrumentView
{
	Q_OBJECT
public:
	MicrowaveView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~MicrowaveView() {};

protected slots:
	//void sampleSizeChanged( float _new_sample_length );

	void interpolationToggled( bool value );
	void normalizeToggled( bool value );
	void visualizeToggled( bool value );

	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void usrWaveClicked();
	
	void smoothClicked( void  );

	void tabChanged( int tabnum );

	void openWavetableFile( int algorithm, int channel );
	void openWavetableFileBtnClicked();

	void updateScroll();
	void scrollReleased();
	void updateGraphColor( float scrollVal );

	void mainNumChanged();
	void modOutSecChanged( int i );

	void subNumChanged();


private:
	virtual void modelChanged();

	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_triangleWaveBtn;
	PixmapButton * m_sqrWaveBtn;
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_whiteNoiseWaveBtn;
	PixmapButton * m_smoothBtn;
	PixmapButton * m_usrWaveBtn;

	PixmapButton * m_openWavetableButton;

	PixmapButton * m_tab0Btn;
	PixmapButton * m_tab1Btn;
	PixmapButton * m_tab2Btn;

	Knob * m_morphKnob[8];
	Knob * m_rangeKnob[8];
	Knob * m_visvolKnob;
	Knob * m_sampLenKnob[8];
	Knob * m_sprayKnob[8];
	Knob * m_morphMaxKnob[8];
	Knob * m_unisonVoicesKnob[8];
	Knob * m_unisonDetuneKnob[8];
	Knob * m_unisonMorphKnob[8];
	Knob * m_unisonModifyKnob[8];
	Knob * m_detuneKnob[8];
	Knob * m_envLenKnob;
	Knob * m_lfoLenKnob;
	Knob * m_seqLenKnob;
	Knob * m_loadAlgKnob;
	Knob * m_loadChnlKnob;
	Knob * m_phaseKnob[8];
	Knob * m_phaseRandKnob[8];
	Knob * m_volKnob[8];
	Knob * m_scrollKnob;

	LcdSpinBox * envNumBox;
	LcdSpinBox * lfoNumBox;
	LcdSpinBox * seqNumBox;
	LcdSpinBox * stepNumBox;
	LcdSpinBox * subNumBox;
	LcdSpinBox * mainNumBox;

	ComboBox * m_modifyModeBox[8];
	Knob * m_modifyKnob[8];
	ComboBox * m_modOutSecBox[256];
	ComboBox * m_modOutSigBox[256];
	ComboBox * m_modOutSecNumBox[256];
	ComboBox * m_modInBox[256];
	ComboBox * m_modInNumBox[256];
	Knob * m_modInAmntKnob[256];
	Knob * m_modInCurveKnob[256];
	ComboBox * m_modInBox2[256];
	ComboBox * m_modInNumBox2[256];
	Knob * m_modInAmntKnob2[256];
	Knob * m_modInCurveKnob2[256];

	ComboBox * m_envSignalBox;
	ComboBox * m_lfoSignalBox;
	ComboBox * m_seqSignalBox;
	LcdSpinBox * modNumBox;
	
	static QPixmap * s_artwork;

	Graph * m_graph;
	LedCheckBox * m_interpolationToggle;
	LedCheckBox * m_normalizeToggle;
	LedCheckBox * m_visualizeToggle;
	LedCheckBox * m_envEnabledToggle;
	LedCheckBox * m_lfoEnabledToggle;
	LedCheckBox * m_seqEnabledToggle;
	LedCheckBox * m_subEnabledToggle[32];
	LedCheckBox * m_noiseEnabledToggle;

	QPalette pal = QPalette();
	QPixmap graphImg = PLUGIN_NAME::getIconPixmap("wavegraph");

	Microwave * m_microwave;

} ;


class bSynth
{
	MM_OPERATORS
public:
	bSynth( NotePlayHandle * _nph,
			float factor, 
			const sample_rate_t _sample_rate,
			float * phase, float  *phaseRand, int * sampLen );
	virtual ~bSynth();
	
	std::vector<float> nextStringSample( int * modifyModeVal, float * _modifyVal, float * _morphVal, float * rangeVal, float * sprayVal, float * unisonVoices, float * unisonDetune, float * unisonMorph, float * unisonModify, float * morphMaxVal, float * detuneVal, float waveforms[8][262144], float * envs, int * envLens, bool * envEnableds, int * envSignals, float * lfos, int * lfoLens, bool * lfoEnableds, int * lfoSignals, float * seqs, int * seqLens, bool * seqEnableds, int * seqSignals, int * stepNums, float * subs, bool * subEnabled, bool noiseEnabled, float * noises, int * sampLen, int * m_modIn, int * m_modInNum, float * m_modInAmnt, float * m_modInCurve, int * m_modIn2, int * m_modInNum2, float * m_modInAmnt2, float * m_modInCurve2, int * m_modOutSec, int * m_modOutSig, int * m_modOutSecNum, float * phase, float * vol );

	float getRealIndex();

	float detuneWithCents( float pitchValue, float detuneValue );


private:
	float sample_realindex[8][32] = {(0)};
	float sample_subindex[32] = {0};
	NotePlayHandle* nph;
	const sample_rate_t sample_rate;
	float lastNoiseSample[2] = {0};

	int indexOffset[8] = {0};

	int noteDuration;

	float lastMainOscVal[8] = {0};
	float lastEnvVal[32] = {0};
	float lastLfoVal[32] = {0};
	float lastSeqVal[32] = {0};
	float lastSubVal[32] = {0};
	
} ;


#endif
