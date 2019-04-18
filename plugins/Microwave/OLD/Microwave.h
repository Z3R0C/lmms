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
		name .addItem( tr( "Pulse Width" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name .addItem( tr( "Stretch Left Half" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name .addItem( tr( "Shrink Right Half To Left Edge" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name .addItem( tr( "Weird 1" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name .addItem( tr( "Asym To Right" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name .addItem( tr( "Asym To Left" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name .addItem( tr( "Weird 2" ), make_unique<PluginPixmapLoader>( "sqrsoft" ) );\
		name .addItem( tr( "Stretch From Center" ), make_unique<PluginPixmapLoader>( "sinabs" ) );\
		name .addItem( tr( "Squish To Center" ), make_unique<PluginPixmapLoader>( "exp" ) );\
		name .addItem( tr( "Stretch And Squish" ), make_unique<PluginPixmapLoader>( "noise" ) );\
		name .addItem( tr( "Add Tail" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name .addItem( tr( "Cut Off Right" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name .addItem( tr( "Cut Off Left" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name .addItem( tr( "Flip" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name .addItem( tr( "" ), make_unique<PluginPixmapLoader>( "moog" ) );

#define signalsmodel( name )\
		name .addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name .addItem( tr( "Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name .addItem( tr( "Range" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name .addItem( tr( "Modify" ), make_unique<PluginPixmapLoader>( "sqr" ) );

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

	float getWaveformSample( int waveformIndex );

protected slots:
	void morphMaxChanged();
	void samplesChanged( int, int );
	void updateVisualizer();
	void sampLenChanged();
	void resetGraphLength();
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
	void subNumChanged();
	void subEnabledChanged();

	void normalize();


private:
	FloatModel  m_morph;
	FloatModel  m_range;
	FloatModel  m_sampLen;
	FloatModel  m_visvol;
	FloatModel  m_morphMax;
	IntModel  m_waveNum;
	ComboBoxModel m_modifyMode;
	FloatModel  m_modify;
	FloatModel  m_spray;
	FloatModel  m_unisonVoices;
	FloatModel  m_unisonDetune;
	FloatModel  m_unisonMorph;
	FloatModel  m_unisonModify;
	FloatModel  m_detune;
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
	FloatModel  m_phase;
	FloatModel  m_phaseRand;
	FloatModel  m_scroll;
	IntModel  m_subNum;
	
	graphModel  m_graph;
	
	
	BoolModel m_interpolation;
	BoolModel m_normalize;
	BoolModel m_visualize;
	BoolModel m_envEnabled;
	BoolModel m_lfoEnabled;
	BoolModel m_seqEnabled;
	BoolModel m_subEnabled;
	BoolModel m_noiseEnabled;

	
	float m_normalizeFactor;
	float sample_realindex[32] = {0};
	float sample_subindex[32] = {0};
	float waveforms[262144] = {0};
	float updateVis = 1024;
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
	bool subEnableds[32] = {false};
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

	void applyClicked();
	
	void smoothClicked( void  );

	void tabChanged( int tabnum );

	void openWavetableFile( int algorithm, int channel );
	void openWavetableFileBtnClicked();

	void updateScroll();
	void scrollReleased();
	void updateGraphColor( float scrollVal );

private:
	virtual void modelChanged();

	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_triangleWaveBtn;
	PixmapButton * m_sqrWaveBtn;
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_whiteNoiseWaveBtn;
	PixmapButton * m_smoothBtn;
	PixmapButton * m_usrWaveBtn;

	PixmapButton * m_applyBtn;

	PixmapButton * m_openWavetableButton;

	PixmapButton * m_tab0Btn;
	PixmapButton * m_tab1Btn;
	PixmapButton * m_tab2Btn;

	Knob * m_morphKnob;
	Knob * m_rangeKnob;
	Knob * m_visvolKnob;
	Knob * m_sampLenKnob;
	Knob * m_sprayKnob;
	Knob * m_morphMaxKnob;
	Knob * m_unisonVoicesKnob;
	Knob * m_unisonDetuneKnob;
	Knob * m_unisonMorphKnob;
	Knob * m_unisonModifyKnob;
	Knob * m_detuneKnob;
	Knob * m_envLenKnob;
	Knob * m_lfoLenKnob;
	Knob * m_seqLenKnob;
	Knob * m_loadAlgKnob;
	Knob * m_loadChnlKnob;
	Knob * m_phaseKnob;
	Knob * m_phaseRandKnob;
	Knob * m_scrollKnob;

	LcdSpinBox * waveNumBox;
	LcdSpinBox * envNumBox;
	LcdSpinBox * lfoNumBox;
	LcdSpinBox * seqNumBox;
	LcdSpinBox * stepNumBox;
	LcdSpinBox * subNumBox;

	ComboBox * m_modifyModeBox;
	Knob * m_modifyKnob;

	ComboBox * m_envSignalBox;
	ComboBox * m_lfoSignalBox;
	ComboBox * m_seqSignalBox;
	
	static QPixmap * s_artwork;

	Graph * m_graph;
	LedCheckBox * m_interpolationToggle;
	LedCheckBox * m_normalizeToggle;
	LedCheckBox * m_visualizeToggle;
	LedCheckBox * m_envEnabledToggle;
	LedCheckBox * m_lfoEnabledToggle;
	LedCheckBox * m_seqEnabledToggle;
	LedCheckBox * m_subEnabledToggle;
	LedCheckBox * m_noiseEnabledToggle;

	QPalette pal = QPalette();
	QPixmap graphImg = PLUGIN_NAME::getIconPixmap("wavegraph");

	Microwave * m_microwave;

} ;


class bSynth
{
	MM_OPERATORS
public:
	bSynth( float * sample, int length, NotePlayHandle * _nph,
			bool _interpolation, float factor, 
			const sample_rate_t _sample_rate,
			float phase, float phaseRand );
	virtual ~bSynth();
	
	std::vector<float> nextStringSample( int modifyModeVal, float _modifyVal, float _morphVal, float rangeVal, float sprayVal, float unisonVoices, float unisonDetune, float unisonMorph, float unisonModify, float morphMaxVal, float modifyMaxVal, float detuneVal, float * waveforms, float * envs, int * envLens, bool * envEnableds, int * envSignals, float * lfos, int * lfoLens, bool * lfoEnableds, int * lfoSignals, float * seqs, int * seqLens, bool * seqEnableds, int * seqSignals, int * stepNums, float * subs, bool * subEnableds, bool noiseEnabled, float * noises );

	float getRealIndex();

	float detuneWithCents( float pitchValue, float detuneValue );


private:
	float sample_realindex[32] = {0};
	float sample_subindex[32] = {0};
	float* sample_shape;
	NotePlayHandle* nph;
	const int sample_length;
	const sample_rate_t sample_rate;
	float lastNoiseSample[2] = {0};

	int indexOffset = 0;

	bool interpolation;
	int noteDuration;
	
} ;


#endif
