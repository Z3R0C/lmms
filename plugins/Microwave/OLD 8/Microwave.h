/*
 * Microwave.h - declaration of class Microwave (a wavetable synthesizer)
 *
 * Copyright (c) 2019 Robert Black AKA DouglasDGI AKA Lost Robot <r94231/at/gmail/dot/com>
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
		name ->addItem( tr( "Squarify" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Pulsify" ), make_unique<PluginPixmapLoader>( "sqr" ) );

#define signalsmodel( name )\
		name .addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name .addItem( tr( "Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name .addItem( tr( "Range" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name .addItem( tr( "Modify" ), make_unique<PluginPixmapLoader>( "sqr" ) );

#define modsectionsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Main OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Sub OSC" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Filter Input" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "Filter Parameters" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Matrix" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Vocoder Input" ), make_unique<PluginPixmapLoader>( "exp" ) );\
		name ->addItem( tr( "Sample OSC" ), make_unique<PluginPixmapLoader>( "noise" ) );

#define mainoscsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name ->addItem( tr( "Range" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Modify" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "ramp" ) );

#define subsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "ramp" ) );

#define samplesignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name ->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "ramp" ) );

#define matrixsignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Amount" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Curve" ), make_unique<PluginPixmapLoader>( "moog" ) );

#define filtersignalsmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Input Volume" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Filter Type" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Slope" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Cutoff Frequency" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "Resonance" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "db Gain" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Saturation" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Wet/Dry" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Balance/Panning" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name ->addItem( tr( "Output Volume" ), make_unique<PluginPixmapLoader>( "ramp" ) );

#define modinmodel( name )\
		name ->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name ->addItem( tr( "Main OSC" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name ->addItem( tr( "Sub OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name ->addItem( tr( "Filter" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name ->addItem( tr( "Sample OSC" ), make_unique<PluginPixmapLoader>( "noise" ) );

#define mod8model( name )\
		name ->addItem( tr( "1" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name ->addItem( tr( "2" ), make_unique<PluginPixmapLoader>( "2" ) );\
		name ->addItem( tr( "3" ), make_unique<PluginPixmapLoader>( "3" ) );\
		name ->addItem( tr( "4" ), make_unique<PluginPixmapLoader>( "4" ) );\
		name ->addItem( tr( "5" ), make_unique<PluginPixmapLoader>( "5" ) );\
		name ->addItem( tr( "6" ), make_unique<PluginPixmapLoader>( "6" ) );\
		name ->addItem( tr( "7" ), make_unique<PluginPixmapLoader>( "7" ) );\
		name ->addItem( tr( "8" ), make_unique<PluginPixmapLoader>( "8" ) );

#define filtertypesmodel( name )\
		name ->addItem( tr( "Lowpass" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name ->addItem( tr( "Highpass" ), make_unique<PluginPixmapLoader>( "2" ) );\
		name ->addItem( tr( "Bandpass" ), make_unique<PluginPixmapLoader>( "3" ) );\
		name ->addItem( tr( "Peak" ), make_unique<PluginPixmapLoader>( "4" ) );\
		name ->addItem( tr( "Notch" ), make_unique<PluginPixmapLoader>( "5" ) );\
		name ->addItem( tr( "Low Shelf" ), make_unique<PluginPixmapLoader>( "6" ) );\
		name ->addItem( tr( "High Shelf" ), make_unique<PluginPixmapLoader>( "7" ) );\
		name ->addItem( tr( "Allpass" ), make_unique<PluginPixmapLoader>( "8" ) );\
		name ->addItem( tr( "Moog 24db Lowpass" ), make_unique<PluginPixmapLoader>( "9" ) );

#define filterslopesmodel( name )\
		name ->addItem( tr( "12 db" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name ->addItem( tr( "24 db" ), make_unique<PluginPixmapLoader>( "2" ) );\
		name ->addItem( tr( "36 db" ), make_unique<PluginPixmapLoader>( "3" ) );\
		name ->addItem( tr( "48 db" ), make_unique<PluginPixmapLoader>( "4" ) );\
		name ->addItem( tr( "60 db" ), make_unique<PluginPixmapLoader>( "5" ) );\
		name ->addItem( tr( "72 db" ), make_unique<PluginPixmapLoader>( "6" ) );\
		name ->addItem( tr( "84 db" ), make_unique<PluginPixmapLoader>( "7" ) );\
		name ->addItem( tr( "96 db" ), make_unique<PluginPixmapLoader>( "8" ) );

#define modcombinetypemodel( name )\
		name ->addItem( tr( "Add" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name ->addItem( tr( "Multiply" ), make_unique<PluginPixmapLoader>( "1" ) );

#define oversamplemodel( name )\
		name.addItem( tr( "1x" ), make_unique<PluginPixmapLoader>( "1" ) );\
		name.addItem( tr( "2x" ), make_unique<PluginPixmapLoader>( "2" ) );\
		name.addItem( tr( "3x" ), make_unique<PluginPixmapLoader>( "3" ) );\
		name.addItem( tr( "4x" ), make_unique<PluginPixmapLoader>( "4" ) );\
		name.addItem( tr( "5x" ), make_unique<PluginPixmapLoader>( "5" ) );\
		name.addItem( tr( "6x" ), make_unique<PluginPixmapLoader>( "6" ) );\
		name.addItem( tr( "7x" ), make_unique<PluginPixmapLoader>( "7" ) );\
		name.addItem( tr( "8x" ), make_unique<PluginPixmapLoader>( "8" ) );

public:
	Microwave(InstrumentTrack * _instrument_track );
	virtual ~Microwave();
	virtual PluginView * instantiateView( QWidget * _parent );
	virtual QString nodeName() const;
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	void setGraphLength( float graphLen );
	virtual void playNote( NotePlayHandle * _n, sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 64 );
	}

protected slots:
	void valueChanged( int, int );
	void morphMaxChanged();
	void sampLenChanged();
	void subSampLenChanged( int );
	void subEnabledChanged( int );
	void modEnabledChanged( int );
	void sampleEnabledChanged( int );
	void samplesChanged( int, int );
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
	IntModel * m_modOutSecNum[256];
	ComboBoxModel * m_modIn[256];
	IntModel * m_modInNum[256];
	IntModel * m_modInOtherNum[256];
	FloatModel * m_modInAmnt[256];
	FloatModel * m_modInCurve[256];
	ComboBoxModel * m_modIn2[256];
	IntModel * m_modInNum2[256];
	IntModel * m_modInOtherNum2[256];
	FloatModel * m_modInAmnt2[256];
	FloatModel * m_modInCurve2[256];
	BoolModel * m_modEnabled[256];
	ComboBoxModel * m_modCombineType[256];
	IntModel  m_modNum;
	FloatModel  * m_spray[8];
	FloatModel  * m_unisonVoices[8];
	FloatModel  * m_unisonDetune[8];
	FloatModel  * m_unisonMorph[8];
	FloatModel  * m_unisonModify[8];
	FloatModel  * m_detune[8];
	FloatModel  m_loadAlg;
	FloatModel  m_loadChnl;
	FloatModel  * m_phase[8];
	FloatModel  * m_phaseRand[8];
	FloatModel  * m_vol[8];
	BoolModel * m_enabled[8];
	BoolModel * m_muted[8];
	FloatModel  m_scroll;
	FloatModel  m_modScroll;
	FloatModel  m_effectScroll;
	IntModel  m_subNum;
	IntModel  m_sampNum;
	IntModel  m_mainNum;
	ComboBoxModel  m_oversample;

	FloatModel  * m_filtInVol[8];
	ComboBoxModel  * m_filtType[8];
	ComboBoxModel  * m_filtSlope[8];
	FloatModel  * m_filtCutoff[8];
	FloatModel  * m_filtReso[8];
	FloatModel  * m_filtGain[8];
	FloatModel  * m_filtSatu[8];
	FloatModel  * m_filtWetDry[8];
	FloatModel  * m_filtBal[8];
	FloatModel  * m_filtOutVol[8];
	BoolModel  * m_filtEnabled[8];

	FloatModel  * vocoBandNum[8];
	FloatModel  * vocoFreqBias[8];
	BoolModel  * vocoSwapInputs[8];
	FloatModel  * vocoLowBound[8];
	FloatModel  * vocoHighBound[8];
	BoolModel  * vocoEnabled[8];
	FloatModel  * vocoReactTime[8];
	
	graphModel  m_graph;
	
	
	BoolModel m_interpolation;
	BoolModel m_normalize;
	BoolModel m_visualize;
	BoolModel * m_subEnabled[64];
	FloatModel * m_subVol[64];
	FloatModel * m_subPhase[64];
	FloatModel * m_subPhaseRand[64];
	BoolModel * m_subMuted[64];
	BoolModel * m_subKeytrack[64];
	FloatModel * m_subDetune[64];
	FloatModel * m_subSampLen[64];
	BoolModel * m_subNoise[64];

	
	float m_normalizeFactor;
	float sample_realindex[8][32] = {{0}};
	float sample_subindex[64] = {0};
	float waveforms[8][262144] = {{0}};
	int currentTab = 0;
	int indexOffset = 0;
	float subs[65536] = {0};
	float sampGraphs[1024] = {0};
	std::vector<float> samples[8][2];

	BoolModel * m_sampleEnabled[8];
	BoolModel * m_sampleGraphEnabled[8];
	BoolModel * m_sampleMuted[8];
	BoolModel * m_sampleKeytracking[8];
	BoolModel * m_sampleLoop[8];

	FloatModel * m_sampleVolume[8];
	FloatModel * m_samplePanning[8];
	FloatModel * m_sampleDetune[8];
	FloatModel * m_samplePhase[8];
	FloatModel * m_samplePhaseRand[8];

	SampleBuffer m_sampleBuffer;

	//Below is for passing to mSynth initialization
	int m_modifyModeArr[8] = {0};
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
	int m_modInOtherNumArr[256] = {0};
	float m_modInAmntArr[256] = {0};
	float m_modInCurveArr[256] = {0};
	int m_modIn2Arr[256] = {0};
	int m_modInNum2Arr[256] = {0};
	int m_modInOtherNum2Arr[256] = {0};
	float m_modInAmnt2Arr[256] = {0};
	float m_modInCurve2Arr[256] = {0};
	int m_modOutSecArr[256] = {0};
	int m_modOutSigArr[256] = {0};
	int m_modOutSecNumArr[256] = {0};
	bool m_modEnabledArr[256] = {false};
	int m_modCombineTypeArr[256] = {0};
	float m_phaseArr[8] = {0};
	float m_volArr[8] = {0};
	bool m_enabledArr[8] = {false};
	bool m_mutedArr[8] = {false};
	
	bool m_subEnabledArr[64] = {false};
	float m_subVolArr[64] = {0};
	float m_subPhaseArr[64] = {0};
	float m_subPhaseRandArr[64] = {0};
	float m_subDetuneArr[64] = {0};
	bool m_subMutedArr[64] = {false};
	bool m_subKeytrackArr[64] = {false};
	float m_subSampLenArr[64] = {0};
	bool m_subNoiseArr[64] = {false};
	
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

	float vocoBandNumArr[8] = {0};
	float vocoFreqBiasArr[8] = {0};
	bool vocoSwapInputsArr[8] = {false};
	float vocoLowBoundArr[8] = {0};
	float vocoHighBoundArr[8] = {0};
	bool vocoEnabledArr[8] = {false};
	float vocoReactTimeArr[8] = {0};

	bool m_sampleEnabledArr[8] = {false};
	bool m_sampleGraphEnabledArr[8] = {false};
	bool m_sampleMutedArr[8] = {false};
	bool m_sampleKeytrackingArr[8] = {false};
	bool m_sampleLoopArr[8] = {false};

	float m_sampleVolumeArr[8] = {0};
	float m_samplePanningArr[8] = {0};
	float m_sampleDetuneArr[8] = {0};
	float m_samplePhaseArr[8] = {0};
	float m_samplePhaseRandArr[8] = {0};
	//Above is for passing to mSynth initialization

	int maxModEnabled = 0;//256;// The highest number of matrix sections that must be looped through
	int maxSubEnabled = 0;//64;// The highest number of sub oscillator sections that must be looped through
	int maxSampleEnabled = 0;//8;// The highest number of sample sections that must be looped through

	InstrumentTrack * microwaveTrack = this->instrumentTrack();
	
	friend class MicrowaveView;
	friend class mSynth;
};



class MicrowaveView : public InstrumentView
{
	Q_OBJECT
public:
	MicrowaveView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~MicrowaveView() {};

protected slots:
	void updateScroll();
	void scrollReleased();
	void mainNumChanged();
	void subNumChanged();
	void sampNumChanged();
	void modOutSecChanged( int i );
	void modInChanged( int i );
	void tabChanged( int tabnum );
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
	void updateGraphColor( float scrollVal );
	void openWavetableFile( int algorithm, int channel );
	void openWavetableFileBtnClicked();
	void openSampleFile();
	void openSampleFileBtnClicked();

private:
	virtual void modelChanged();

	void mouseMoveEvent( QMouseEvent * _me );

	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_triangleWaveBtn;
	PixmapButton * m_sqrWaveBtn;
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_whiteNoiseWaveBtn;
	PixmapButton * m_smoothBtn;
	PixmapButton * m_usrWaveBtn;

	PixmapButton * m_sinWave2Btn;
	PixmapButton * m_triangleWave2Btn;
	PixmapButton * m_sqrWave2Btn;
	PixmapButton * m_sawWave2Btn;
	PixmapButton * m_whiteNoiseWave2Btn;
	PixmapButton * m_smooth2Btn;
	PixmapButton * m_usrWave2Btn;

	PixmapButton * m_openWavetableButton;
	PixmapButton * m_openSampleButton;

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
	Knob * m_loadAlgKnob;
	Knob * m_loadChnlKnob;
	Knob * m_phaseKnob[8];
	Knob * m_phaseRandKnob[8];
	Knob * m_volKnob[8];
	LedCheckBox * m_enabledToggle[8];
	LedCheckBox * m_mutedToggle[8];
	Knob * m_scrollKnob;
	Knob * m_modScrollKnob;
	Knob * m_effectScrollKnob;

	Knob * m_filtInVolKnob[8];
	ComboBox * m_filtTypeBox[8];
	ComboBox * m_filtSlopeBox[8];
	Knob * m_filtCutoffKnob[8];
	Knob * m_filtResoKnob[8];
	Knob * m_filtGainKnob[8];
	Knob * m_filtSatuKnob[8];
	Knob * m_filtWetDryKnob[8];
	Knob * m_filtBalKnob[8];
	Knob * m_filtOutVolKnob[8];
	LedCheckBox * m_filtEnabledToggle[8];

	Knob * vocoBandNumKnob[8];
	Knob * vocoFreqBiasKnob[8];
	LedCheckBox * vocoSwapInputsToggle[8];
	Knob * vocoLowBoundKnob[8];
	Knob * vocoHighBoundKnob[8];
	LedCheckBox * vocoEnabledToggle[8];
	Knob * vocoReactTimeKnob[8];

	LcdSpinBox * subNumBox;
	LcdSpinBox * sampNumBox;
	LcdSpinBox * mainNumBox;

	ComboBox * m_oversampleBox;

	ComboBox * m_modifyModeBox[8];
	Knob * m_modifyKnob[8];
	ComboBox * m_modOutSecBox[256];
	ComboBox * m_modOutSigBox[256];
	LcdSpinBox * m_modOutSecNumBox[256];
	ComboBox * m_modInBox[256];
	LcdSpinBox * m_modInNumBox[256];
	LcdSpinBox * m_modInOtherNumBox[256];
	Knob * m_modInAmntKnob[256];
	Knob * m_modInCurveKnob[256];
	ComboBox * m_modInBox2[256];
	LcdSpinBox * m_modInNumBox2[256];
	LcdSpinBox * m_modInOtherNumBox2[256];
	Knob * m_modInAmntKnob2[256];
	Knob * m_modInCurveKnob2[256];
	LedCheckBox * m_modEnabledToggle[256];
	ComboBox * m_modCombineTypeBox[256];

	LcdSpinBox * modNumBox;
	
	static QPixmap * s_artwork;

	Graph * m_graph;
	LedCheckBox * m_interpolationToggle;
	LedCheckBox * m_normalizeToggle;
	LedCheckBox * m_visualizeToggle;
	LedCheckBox * m_subEnabledToggle[64];
	Knob * m_subVolKnob[64];
	Knob * m_subPhaseKnob[64];
	Knob * m_subPhaseRandKnob[64];
	Knob * m_subDetuneKnob[64];
	LedCheckBox * m_subMutedToggle[64];
	LedCheckBox * m_subKeytrackToggle[64];
	Knob * m_subSampLenKnob[64];
	LedCheckBox * m_subNoiseToggle[64];

	LedCheckBox * m_sampleEnabledToggle[8];
	LedCheckBox * m_sampleGraphEnabledToggle[8];
	LedCheckBox * m_sampleMutedToggle[8];
	LedCheckBox * m_sampleKeytrackingToggle[8];
	LedCheckBox * m_sampleLoopToggle[8];

	Knob * m_sampleVolumeKnob[8];
	Knob * m_samplePanningKnob[8];
	Knob * m_sampleDetuneKnob[8];
	Knob * m_samplePhaseKnob[8];
	Knob * m_samplePhaseRandKnob[8];

	QPalette pal = QPalette();
	QPixmap fullArtworkImg = PLUGIN_NAME::getIconPixmap("artwork");

	Microwave * m_microwave;
};


class mSynth
{
	MM_OPERATORS
public:
	mSynth( NotePlayHandle * _nph,
			float factor, 
			const sample_rate_t _sample_rate,
			float * phaseRand,
			int * modifyModeVal, float * _modifyVal, float * _morphVal, float * rangeVal, float * sprayVal, float * unisonVoices, float * unisonDetune, float * unisonMorph, float * unisonModify, float * morphMaxVal, float * detuneVal, float waveforms[8][262144], float * subs, bool * subEnabled, float * subVol, float * subPhase, float * subPhaseRand, float * subDetune, bool * subMuted, bool * subKeytrack, float * subSampLen, bool * subNoise, int * sampLen, int * m_modIn, int * m_modInNum, int * m_modInOtherNum, float * m_modInAmnt, float * m_modInCurve, int * m_modIn2, int * m_modInNum2, int * m_modInOtherNum2, float * m_modInAmnt2, float * m_modInCurve2, int * m_modOutSec, int * m_modOutSig, int * m_modOutSecNum, int * m_modCombineType, float * phase, float * vol, float * filtInVol, int * filtType, int * filtSlope, float * filtCutoff, float * filtReso, float * filtGain, float * filtSatu, float * filtWetDry, float * filtBal, float * filtOutVol, bool * filtEnabled, float * vocoBandNum, float * vocoFreqBias, bool * vocoSwapInputs, float * vocoLowBound, float * vocoHighBound, bool * vocoEnabled, float * vocoReactTime, bool * enabled, bool * modEnabled, float * sampGraphs, bool * muted, bool * sampleEnabled, bool * sampleGraphEnabled, bool * sampleMuted, bool * sampleKeytracking, bool * sampleLoop, float * sampleVolume, float * samplePanning, float * sampleDetune, float * samplePhase, float * samplePhaseRand, std::vector<float> (&samples)[8][2] );
	virtual ~mSynth();
	
	std::vector<float> nextStringSample( float waveforms[8][262144], float * subs, float * sampGraphs, std::vector<float> (&samples)[8][2], int maxModEnabled, int maxSubEnabled, int maxSampleEnabled, int sample_rate );

	inline float detuneWithCents( float pitchValue, float detuneValue );

	void refreshValue( int which, int num );

private:
	float sample_realindex[8][32] = {(0)};
	float sample_subindex[64] = {0};
	float sample_sampleindex[8] = {0};
	NotePlayHandle* nph;
	const sample_rate_t sample_rate;

	int indexOffset[8] = {0};

	int noteDuration;
	float noteFreq = 0;

	float lastMainOscVal[8] = {0};
	float lastSubVal[64] = {0};
	float sample_step_sub = 0;
	float noiseSampRand = 0;
	float lastSampleVal[8][2] = {{0}};
	float sample_step_sample = 0;

	int loopStart = 0;
	int loopEnd = 0;
	float currentMorphVal = 0;
	float currentRangeValInvert = 0;
	int currentSampLen = 0;
	int currentIndex = 0;

	float filtOutputs[8][8][2] = {{{0}}};// [filter number][port][channel]
	float filtPrevSampIn[8][8][8][5][2] = {{{{0}}}};// [filter number][port][slope][samples back in time][channel]
	float filtPrevSampOut[8][8][8][5][2] = {{{{0}}}};// [filter number][port][slope][samples back in time][channel]

	float filty1[2] = {0};
	float filty2[2] = {0};
	float filty3[2] = {0};
	float filty4[2] = {0};
	float filtoldx[2] = {0};
	float filtoldy1[2] = {0};
	float filtoldy2[2] = {0};
	float filtoldy3[2] = {0};
	float filtx[2] = {0};

	float vocoOutputs[8][8][2] = {{{0}}};// [vocoder number][port][channel]
	float vocoPrevSampIn[8][8][64][3][2] = {{{{{0}}}}};// [vocoder number][port][which][samples back in time][channel]
	float vocoPrevSampOut[8][8][64][3][2] = {{{{{0}}}}};// [vocoder number][port][which][samples back in time][channel]
	float vocoPrevSampIn2[8][8][64][3][2] = {{{{{0}}}}};// [vocoder number][port][which][samples back in time][channel]
	float vocoPrevSampOut2[8][8][64][3][2] = {{{{{0}}}}};// [vocoder number][port][which][samples back in time][channel]
	float vocoEnvFollow[8][8][64][2] = {0};// [vocoder number][port][which][channel]

	std::vector<int> modValType;
	std::vector<int> modValNum;



	int modifyModeVal[8];
	float _modifyVal[8];
	float _morphVal[8];
	float rangeVal[8];
	float sprayVal[8];
	float unisonVoices[8];
	float unisonDetune[8];
	float unisonMorph[8];
	float unisonModify[8];
	float morphMaxVal[8];
	float detuneVal[8];
	int sampLen[8];
	int m_modIn[256];
	int m_modInNum[256];
	int m_modInOtherNum[256];
	float m_modInAmnt[256];
	float m_modInCurve[256];
	int m_modIn2[256];
	int m_modInNum2[256];
	int m_modInOtherNum2[256];
	float m_modInAmnt2[256];
	float m_modInCurve2[256];
	int m_modOutSec[256];
	int m_modOutSig[256];
	int m_modOutSecNum[256];
	bool modEnabled[256];
	int m_modCombineType[256];
	float phase[8];
	float vol[8];
	bool enabled[8];
	bool muted[8];
	
	bool subEnabled[64];
	float subVol[64];
	float subPhase[64];
	float subPhaseRand[64];
	float subDetune[64];
	bool subMuted[64];
	bool subKeytrack[64];
	float subSampLen[64];
	bool subNoise[64];
	
	float filtInVol[8];
	int filtType[8];
	int filtSlope[8];
	float filtCutoff[8];
	float filtReso[8];
	float filtGain[8];
	float filtSatu[8];
	float filtWetDry[8];
	float filtBal[8];
	float filtOutVol[8];
	bool filtEnabled[8];

	float vocoBandNum[8];
	float vocoFreqBias[8];
	bool vocoSwapInputs[8];
	float vocoLowBound[8];
	float vocoHighBound[8];
	bool vocoEnabled[8];
	float vocoReactTime[8];

	bool sampleEnabled[8];
	bool sampleGraphEnabled[8];
	bool sampleMuted[8];
	bool sampleKeytracking[8];
	bool sampleLoop[8];

	float sampleVolume[8];
	float samplePanning[8];
	float sampleDetune[8];
	float samplePhase[8];
	float samplePhaseRand[8];

	float cutoff;
	float reso;
	float Fs;
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	float alpha;
	float w0;

	friend class Microwave;
	
};




#endif
