/*
 * Microwave.cpp - morbidly advanced and versatile wavetable synthesizer
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
#include <QMouseEvent>
#include "embed.h"
#include "plugin_export.h"
#include "CaptionMenu.h"
#include "StringPairDrag.h"
#include <QDropEvent>






















#include <iostream>
using namespace std;






















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


/*                                                                                                   
          ____                                                                                     
        ,'  , `.                                                                                   
     ,-+-,.' _ |  ,--,                                                                             
  ,-+-. ;   , ||,--.'|              __  ,-.   ,---.           .---.                                
 ,--.'|'   |  ;||  |,             ,' ,'/ /|  '   ,'\         /. ./|                .---.           
|   |  ,', |  ':`--'_       ,---. '  | |' | /   /   |     .-'-. ' |  ,--.--.     /.  ./|   ,---.   
|   | /  | |  ||,' ,'|     /     \|  |   ,'.   ; ,. :    /___/ \: | /       \  .-' . ' |  /     \  
'   | :  | :  |,'  | |    /    / ''  :  /  '   | |: : .-'.. '   ' ..--.  .-. |/___/ \: | /    /  | 
;   . |  ; |--' |  | :   .    ' / |  | '   '   | .; :/___/ \:     ' \__\/: . ..   \  ' ..    ' / | 
|   : |  | ,    '  : |__ '   ; :__;  : |   |   :    |.   \  ' .\    ," .--.; | \   \   ''   ;   /| 
|   : '  |/     |  | '.'|'   | '.'|  , ;    \   \  /  \   \   ' \ |/  /  ,.  |  \   \   '   |  / | 
;   | |`-'      ;  :    ;|   :    :---'      `----'    \   \  |--";  :   .'   \  \   \ ||   :    | 
|   ;/          |  ,   /  \   \  /                      \   \ |   |  ,     .-./   '---"  \   \  /  
'---'            ---`-'    `----'                        '---"     `--`---'               `----'   
                                                                                                   
                                    .----------------.
                                   /_____________ ____\
                                   ||\ _________ /|+++|
                                   || |:       :| |+++|
                                   || |; (◕‿◕) ;| | + |
                                   || |_________| | _ |
                                   ||/___________\|[_]|
                                   "------------------"

*/




// Create Microwave.  Create value models (and set defaults, maximums, minimums, and step sizes).  Connect events to functions.
Microwave::Microwave( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &microwave_plugin_descriptor ),
	m_visvol( 100, 0, 1000, 0.01f, this, tr( "Visualizer Volume" ) ),
	m_modNum(1, 1, 32, this, tr( "Modulation Page Number" ) ),
	m_loadAlg( 0, 0, 7, 1, this, tr( "Wavetable Loading Algorithm" ) ),
	m_loadChnl( 0, 0, 1, 1, this, tr( "Wavetable Loading Channel" ) ),
	m_scroll( 1, 1, 5, 0.0001f, this, tr( "Scroll" ) ),
	m_modScroll( 1, 1, 64, 0.0001f, this, tr( "Mod Scroll" ) ),
	m_effectScroll( 1, 1, 32, 0.0001f, this, tr( "Effect Scroll" ) ),
	m_subNum(1, 1, 64, this, tr( "Sub Oscillator Number" ) ),
	m_sampNum(1, 1, 8, this, tr( "Sample Number" ) ),
	m_mainNum(1, 1, 8, this, tr( "Main Oscillator Number" ) ),
	m_oversample( this, tr("Oversampling") ),
	m_graph( -1.0f, 1.0f, 2048, this ),
	m_interpolation( false, this ),
	m_normalize( false, this ),
	m_visualize( false, this )
{

	for( int i = 0; i < 8; ++i )
	{
		m_morph[i] = new FloatModel( 0, 0, 254, 0.0001f, this, tr( "Morph" ) );
		m_range[i] = new FloatModel( 8, 1, 16, 0.0001f, this, tr( "Range" ) );
		m_sampLen[i] = new FloatModel( 2048, 1, 8192, 1.f, this, tr( "Waveform Sample Length" ) );
		m_morphMax[i] = new FloatModel( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) );
		m_modifyMode[i] = new ComboBoxModel( this, tr( "Wavetable Modifier Mode" ) );
		m_modify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Wavetable Modifier Value" ) );
		m_spray[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Spray" ) );
		m_spray[i]->setScaleLogarithmic( true );
		m_unisonVoices[i] = new FloatModel( 1, 1, 32, 1, this, tr( "Unison Voices" ) );
		m_unisonDetune[i] = new FloatModel( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) );
		m_unisonDetune[i]->setScaleLogarithmic( true );
		m_unisonMorph[i] = new FloatModel( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) );
		m_unisonModify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Unison Modify" ) );
		m_detune[i] = new FloatModel( 0, -9600, 9600, 1.f, this, tr( "Detune" ) );
		m_phase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		m_phaseRand[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		m_vol[i] = new FloatModel( 100.f, 0, 200.f, 0.0001f, this, tr( "Volume" ) );
		m_enabled[i] = new BoolModel( false, this );
		m_muted[i] = new BoolModel( false, this );
		setwavemodel( m_modifyMode[i] )

		m_filtInVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Input Volume" ) );
		m_filtType[i] = new ComboBoxModel( this, tr( "Filter Type" ) );
		m_filtSlope[i] = new ComboBoxModel( this, tr( "Filter Slope" ) );
		m_filtCutoff[i] = new FloatModel( 2000, 20, 20000, 0.0001f, this, tr( "Cutoff Frequency" ) );
		m_filtCutoff[i]->setScaleLogarithmic( true );
		m_filtReso[i] = new FloatModel( 0.707, 0, 16, 0.0001f, this, tr( "Resonance" ) );
		m_filtReso[i]->setScaleLogarithmic( true );
		m_filtGain[i] = new FloatModel( 0, -64, 64, 0.0001f, this, tr( "dbGain" ) );
		m_filtGain[i]->setScaleLogarithmic( true );
		m_filtSatu[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Saturation" ) );
		m_filtWetDry[i] = new FloatModel( 100, 0, 100, 0.0001f, this, tr( "Wet/Dry" ) );
		m_filtBal[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Balance/Panning" ) );
		m_filtOutVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Output Volume" ) );
		m_filtEnabled[i] = new BoolModel( false, this );
		m_filtFeedback[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Feedback" ) );
		m_filtDetune[i] = new FloatModel( 0, -4800, 4800, 0.0001f, this, tr( "Detune" ) );
		m_filtKeytracking[i] = new BoolModel( true, this );
		filtertypesmodel( m_filtType[i] )
		filterslopesmodel( m_filtSlope[i] )

		m_sampleEnabled[i] = new BoolModel( false, this );
		m_sampleGraphEnabled[i] = new BoolModel( false, this );
		m_sampleMuted[i] = new BoolModel( false, this );
		m_sampleKeytracking[i] = new BoolModel( true, this );
		m_sampleLoop[i] = new BoolModel( true, this );

		m_sampleVolume[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Volume" ) );
		m_samplePanning[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Panning" ) );
		m_sampleDetune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
		m_samplePhase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		m_samplePhaseRand[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		m_sampleStart[i] = new FloatModel( 0, 0, 0.9999f, 0.0001f, this, tr( "Start" ) );
		m_sampleEnd[i] = new FloatModel( 1, 0.0001f, 1, 0.0001f, this, tr( "End" ) );
	}

	for( int i = 0; i < 64; ++i )
 	{
		m_subEnabled[i] = new BoolModel( false, this );
		m_subVol[i] = new FloatModel( 100.f, 0.f, 200.f, 0.0001f, this, tr( "Volume" ) );
		m_subPhase[i] = new FloatModel( 0.f, 0.f, 200.f, 0.0001f, this, tr( "Phase" ) );
		m_subPhaseRand[i] = new FloatModel( 0.f, 0.f, 100.f, 0.0001f, this, tr( "Phase Randomness" ) );
		m_subDetune[i] = new FloatModel( 0.f, -9600.f, 9600.f, 1.f, this, tr( "Detune" ) );
		m_subMuted[i] = new BoolModel( true, this );
		m_subKeytrack[i] = new BoolModel( true, this );
		m_subSampLen[i] = new FloatModel( 1024.f, 1.f, 1024.f, 1.f, this, tr( "Sample Length" ) );
		m_subNoise[i] = new BoolModel( false, this );
		m_subPanning[i] = new FloatModel( 0.f, -100.f, 100.f, 0.0001f, this, tr( "Panning" ) );

		m_modEnabled[i] = new BoolModel( false, this );

		m_modOutSec[i] = new ComboBoxModel( this, tr( "Modulation Section" ) );
		m_modOutSig[i] = new ComboBoxModel( this, tr( "Modulation Signal" ) );
		m_modOutSecNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulation Section Number" ) );
		modsectionsmodel( m_modOutSec[i] )
		mainoscsignalsmodel( m_modOutSig[i] )
		
		m_modIn[i] = new ComboBoxModel( this, tr( "Modulator" ) );
		m_modInNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		m_modInOtherNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		modinmodel( m_modIn[i] )

		m_modInAmnt[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Modulator Amount" ) );
		m_modInCurve[i] = new FloatModel( 100, 0.0001f, 200, 0.0001f, this, tr( "Modulator Curve" ) );

		m_modIn2[i] = new ComboBoxModel( this, tr( "Secondary Modulator" ) );
		m_modInNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		m_modInOtherNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		modinmodel( m_modIn2[i] )

		m_modInAmnt2[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Secondary Modulator Amount" ) );
		m_modInCurve2[i] = new FloatModel( 100, 0.0001f, 200, 0.0001f, this, tr( "Secondary Modulator Curve" ) );

		m_modCombineType[i] = new ComboBoxModel( this, tr( "Combination Type" ) );
		modcombinetypemodel( m_modCombineType[i] )
	}

	oversamplemodel( m_oversample )


	m_graph.setWaveToSine();

	for( int i = 0; i < 8; ++i )
	{
		connect( m_morphMax[i], SIGNAL( dataChanged( ) ),
			this, SLOT( morphMaxChanged( ) ) );
	}

	connect( &m_graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );

	for( int i = 0; i < 8; ++i )
	{
		connect( m_morph[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(1, i); } );
		connect( m_range[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(2, i); } );
		connect( m_modify[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(3, i); } );
		connect( m_modifyMode[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(4, i); } );
		connect( m_spray[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(5, i); } );
		connect( m_unisonVoices[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(6, i); } );
		connect( m_unisonDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(7, i); } );
		connect( m_unisonMorph[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(8, i); } );
		connect( m_unisonModify[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(9, i); } );
		connect( m_morphMax[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(10, i); } );
		connect( m_detune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(11, i); } );
		connect( m_sampLen[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(12, i); } );
		connect( m_phase[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(13, i); } );
		connect( m_vol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(14, i); } );
		connect( m_muted[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(57, i); } );

		connect( m_filtInVol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(15, i); } );
		connect( m_filtType[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(16, i); } );
		connect( m_filtSlope[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(17, i); } );
		connect( m_filtCutoff[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(18, i); } );
		connect( m_filtReso[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(19, i); } );
		connect( m_filtGain[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(20, i); } );
		connect( m_filtSatu[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(21, i); } );
		connect( m_filtWetDry[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(22, i); } );
		connect( m_filtBal[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(23, i); } );
		connect( m_filtOutVol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(24, i); } );
		connect( m_filtEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(25, i); } );
		connect( m_filtFeedback[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(69, i); } );
		connect( m_filtDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(70, i); } );
		connect( m_filtKeytracking[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(71, i); } );

		connect( m_enabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(48, i); } );

		connect( m_sampleEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(59, i); } );

		connect( m_sampleGraphEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(60, i); } );

		connect( m_sampleMuted[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(61, i); } );

		connect( m_sampleKeytracking[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(62, i); } );

		connect( m_sampleLoop[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(63, i); } );

		connect( m_sampleVolume[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(64, i); } );

		connect( m_samplePanning[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(65, i); } );

		connect( m_sampleDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(66, i); } );

		connect( m_samplePhase[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(67, i); } );

		connect( m_samplePhaseRand[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(68, i); } );

		connect( m_sampleStart[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(73, i); } );

		connect( m_sampleEnd[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(74, i); } );

		for( int j = 1; j <= 25; ++j )
		{
			valueChanged(j, i);
		}
		valueChanged(48, i);

		valueChanged(51, i);
		valueChanged(52, i);
		valueChanged(53, i);
		valueChanged(54, i);
		valueChanged(55, i);
		valueChanged(56, i);
		valueChanged(57, i);
		valueChanged(58, i);
		valueChanged(59, i);
		valueChanged(60, i);
		valueChanged(61, i);
		valueChanged(62, i);
		valueChanged(63, i);
		valueChanged(64, i);
		valueChanged(65, i);
		valueChanged(66, i);
		valueChanged(67, i);
		valueChanged(68, i);
		valueChanged(69, i);
		valueChanged(70, i);
		valueChanged(71, i);

		connect( m_sampleEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { sampleEnabledChanged(i); } );
	}

	for( int i = 0; i < 64; ++i )
	{
		connect( m_subEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(26, i); } );
		connect( m_subVol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(27, i); } );
		connect( m_subPhase[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(28, i); } );
		connect( m_subPhaseRand[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(29, i); } );
		connect( m_subDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(30, i); } );
		connect( m_subMuted[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(31, i); } );
		connect( m_subKeytrack[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(32, i); } );
		connect( m_subSampLen[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(33, i); } );
		connect( m_subNoise[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(34, i); } );
		connect( m_subPanning[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(72, i); } );
		for( int j = 26; j <= 35; ++j )
		{
			valueChanged(j, i);
		}

		connect( m_subSampLen[i], &FloatModel::dataChanged,
            			this, [this, i]() { subSampLenChanged(i); } );
		connect( m_subEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { subEnabledChanged(i); } );

		connect( m_modIn[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(35, i); } );
		connect( m_modInNum[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(36, i); } );
		connect( m_modInOtherNum[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(37, i); } );
		connect( m_modInAmnt[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(38, i); } );
		connect( m_modInCurve[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(39, i); } );
		connect( m_modIn2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(40, i); } );
		connect( m_modInNum2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(41, i); } );
		connect( m_modInOtherNum2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(42, i); } );
		connect( m_modInAmnt2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(43, i); } );
		connect( m_modInCurve2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(44, i); } );
		connect( m_modOutSec[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(45, i); } );
		connect( m_modOutSig[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(46, i); } );
		connect( m_modOutSecNum[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(47, i); } );
		connect( m_modEnabled[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(49, i); } );
		connect( m_modCombineType[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(50, i); } );
		for( int j = 35; j <= 47; ++j )
		{
			valueChanged(j, i);
		}
		valueChanged(49, i);
		valueChanged(50, i);

		connect( m_modEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { modEnabledChanged(i); } );
	}
}


Microwave::~Microwave()
{
}


PluginView * Microwave::instantiateView( QWidget * _parent )
{
	return( new MicrowaveView( this, _parent ) );
}


QString Microwave::nodeName() const
{
	return( microwave_plugin_descriptor.name );
}


void Microwave::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.9" );

	m_visvol.saveSettings( _doc, _this, "visualizervolume" );
	m_loadAlg.saveSettings( _doc, _this, "loadingalgorithm" );
	m_loadChnl.saveSettings( _doc, _this, "loadingchannel" );

	QString saveString;

	for( int i = 0; i < 8; ++i )
	{
		base64::encode( (const char *)waveforms[i],
			524288 * sizeof(float), saveString );
		_this.setAttribute( "waveforms"+QString::number(i), saveString );
	}

	base64::encode( (const char *)subs,
		65536 * sizeof(float), saveString );
	_this.setAttribute( "subs", saveString );

	base64::encode( (const char *)sampGraphs,
		1024 * sizeof(float), saveString );
	_this.setAttribute( "sampGraphs", saveString );

	int sampleSizes[8] = {0};
	for( int i = 0; i < 8; ++i )
	{
		for( int j = 0; j < 2; ++j )
		{
			base64::encode( (const char *)samples[i][j].data(),
				samples[i][j].size() * sizeof(float), saveString );
			_this.setAttribute( "samples_"+QString::number(i)+"_"+QString::number(j), saveString );
		}

		sampleSizes[i] = samples[i][0].size();
	}

	base64::encode( (const char *)sampleSizes,
		8 * sizeof(int), saveString );
	_this.setAttribute( "sampleSizes", saveString );

	for( int i = 0; i < 8; ++i )
	{
		m_morph[i]->saveSettings( _doc, _this, "morph_"+QString::number(i) );
		m_range[i]->saveSettings( _doc, _this, "range_"+QString::number(i) );
		m_modify[i]->saveSettings( _doc, _this, "modify_"+QString::number(i) );
		m_modifyMode[i]->saveSettings( _doc, _this, "modifyMode_"+QString::number(i) );
		m_spray[i]->saveSettings( _doc, _this, "spray_"+QString::number(i) );
		m_unisonVoices[i]->saveSettings( _doc, _this, "unisonVoices_"+QString::number(i) );
		m_unisonDetune[i]->saveSettings( _doc, _this, "unisonDetune_"+QString::number(i) );
		m_unisonMorph[i]->saveSettings( _doc, _this, "unisonMorph_"+QString::number(i) );
		m_unisonModify[i]->saveSettings( _doc, _this, "unisonModify_"+QString::number(i) );
		m_morphMax[i]->saveSettings( _doc, _this, "morphMax_"+QString::number(i) );
		m_detune[i]->saveSettings( _doc, _this, "detune_"+QString::number(i) );
		m_sampLen[i]->saveSettings( _doc, _this, "sampLen_"+QString::number(i) );
		m_phase[i]->saveSettings( _doc, _this, "phase_"+QString::number(i) );
		m_vol[i]->saveSettings( _doc, _this, "vol_"+QString::number(i) );
		m_filtInVol[i]->saveSettings( _doc, _this, "filtInVol_"+QString::number(i) );
		m_filtType[i]->saveSettings( _doc, _this, "filtType_"+QString::number(i) );
		m_filtSlope[i]->saveSettings( _doc, _this, "filtSlope_"+QString::number(i) );
		m_filtCutoff[i]->saveSettings( _doc, _this, "filtCutoff_"+QString::number(i) );
		m_filtReso[i]->saveSettings( _doc, _this, "filtReso_"+QString::number(i) );
		m_filtGain[i]->saveSettings( _doc, _this, "filtGain_"+QString::number(i) );
		m_filtSatu[i]->saveSettings( _doc, _this, "filtSatu_"+QString::number(i) );
		m_filtWetDry[i]->saveSettings( _doc, _this, "filtWetDry_"+QString::number(i) );
		m_filtBal[i]->saveSettings( _doc, _this, "filtBal_"+QString::number(i) );
		m_filtOutVol[i]->saveSettings( _doc, _this, "filtOutVol_"+QString::number(i) );
		m_filtEnabled[i]->saveSettings( _doc, _this, "filtEnabled_"+QString::number(i) );
		m_filtFeedback[i]->saveSettings( _doc, _this, "filtFeedback_"+QString::number(i) );
		m_filtDetune[i]->saveSettings( _doc, _this, "filtDetune_"+QString::number(i) );
		m_filtKeytracking[i]->saveSettings( _doc, _this, "filtKeytracking_"+QString::number(i) );
		m_enabled[i]->saveSettings( _doc, _this, "enabled_"+QString::number(i) );
		m_muted[i]->saveSettings( _doc, _this, "muted_"+QString::number(i) );
		m_sampleEnabled[i]->saveSettings( _doc, _this, "sampleEnabled_"+QString::number(i) );
		m_sampleGraphEnabled[i]->saveSettings( _doc, _this, "sampleGraphEnabled_"+QString::number(i) );
		m_sampleMuted[i]->saveSettings( _doc, _this, "sampleMuted_"+QString::number(i) );
		m_sampleKeytracking[i]->saveSettings( _doc, _this, "sampleKeytracking_"+QString::number(i) );
		m_sampleLoop[i]->saveSettings( _doc, _this, "sampleLoop_"+QString::number(i) );
		m_sampleVolume[i]->saveSettings( _doc, _this, "sampleVolume_"+QString::number(i) );
		m_samplePanning[i]->saveSettings( _doc, _this, "samplePanning_"+QString::number(i) );
		m_sampleDetune[i]->saveSettings( _doc, _this, "sampleDetune_"+QString::number(i) );
		m_samplePhase[i]->saveSettings( _doc, _this, "samplePhase_"+QString::number(i) );
		m_samplePhaseRand[i]->saveSettings( _doc, _this, "samplePhaseRand_"+QString::number(i) );
		m_sampleStart[i]->saveSettings( _doc, _this, "sampleStart_"+QString::number(i) );
		m_sampleEnd[i]->saveSettings( _doc, _this, "sampleEnd_"+QString::number(i) );
	}

	for( int i = 0; i < 64; ++i )
	{
		m_subEnabled[i]->saveSettings( _doc, _this, "subEnabled_"+QString::number(i) );
		m_subVol[i]->saveSettings( _doc, _this, "subVol_"+QString::number(i) );
		m_subPhase[i]->saveSettings( _doc, _this, "subPhase_"+QString::number(i) );
		m_subPhaseRand[i]->saveSettings( _doc, _this, "subPhaseRand_"+QString::number(i) );
		m_subDetune[i]->saveSettings( _doc, _this, "subDetune_"+QString::number(i) );
		m_subMuted[i]->saveSettings( _doc, _this, "subMuted_"+QString::number(i) );
		m_subKeytrack[i]->saveSettings( _doc, _this, "subKeytrack_"+QString::number(i) );
		m_subSampLen[i]->saveSettings( _doc, _this, "subSampLen_"+QString::number(i) );
		m_subNoise[i]->saveSettings( _doc, _this, "subNoise_"+QString::number(i) );
		m_subPanning[i]->saveSettings( _doc, _this, "subPanning_"+QString::number(i) );

		m_modIn[i]->saveSettings( _doc, _this, "modIn_"+QString::number(i) );
		m_modInNum[i]->saveSettings( _doc, _this, "modInNum_"+QString::number(i) );
		m_modInOtherNum[i]->saveSettings( _doc, _this, "modInOtherNum_"+QString::number(i) );
		m_modInAmnt[i]->saveSettings( _doc, _this, "modInAmnt_"+QString::number(i) );
		m_modInCurve[i]->saveSettings( _doc, _this, "modInCurve_"+QString::number(i) );
		m_modIn2[i]->saveSettings( _doc, _this, "modIn2_"+QString::number(i) );
		m_modInNum2[i]->saveSettings( _doc, _this, "modInNum2_"+QString::number(i) );
		m_modInOtherNum2[i]->saveSettings( _doc, _this, "modInOtherNum2_"+QString::number(i) );
		m_modInAmnt2[i]->saveSettings( _doc, _this, "modAmnt2_"+QString::number(i) );
		m_modInCurve2[i]->saveSettings( _doc, _this, "modCurve2_"+QString::number(i) );
		m_modOutSec[i]->saveSettings( _doc, _this, "modOutSec_"+QString::number(i) );
		m_modOutSig[i]->saveSettings( _doc, _this, "modOutSig_"+QString::number(i) );
		m_modOutSecNum[i]->saveSettings( _doc, _this, "modOutSecNum_"+QString::number(i) );
		m_modEnabled[i]->saveSettings( _doc, _this, "modEnabled_"+QString::number(i) );
		m_modCombineType[i]->saveSettings( _doc, _this, "modCombineType_"+QString::number(i) );
	}
}


void Microwave::loadSettings( const QDomElement & _this )
{
	m_visvol.loadSettings( _this, "visualizervolume" );
	m_loadAlg.loadSettings( _this, "loadingalgorithm" );
	m_loadChnl.loadSettings( _this, "loadingchannel" );

	m_graph.setLength( 2048 );

	// Load arrays
	int size = 0;
	char * dst = 0;
	for( int j = 0; j < 8; ++j )
	{
		base64::decode( _this.attribute( "waveforms"+QString::number(j) ), &dst, &size );
		for( int i = 0; i < 524288; ++i )
		{
			waveforms[j][i] = ( (float*) dst )[i];
		}
	}

	base64::decode( _this.attribute( "subs" ), &dst, &size );
	for( int i = 0; i < 65536; ++i )
	{
		subs[i] = ( (float*) dst )[i];
	}

	base64::decode( _this.attribute( "sampGraphs" ), &dst, &size );
	for( int i = 0; i < 1024; ++i )
	{
		sampGraphs[i] = ( (float*) dst )[i];
	}

	int sampleSizes[8] = {0};
	base64::decode( _this.attribute( "sampleSizes" ), &dst, &size );
	for( int i = 0; i < 8; ++i )
	{
		sampleSizes[i] = ( (int*) dst )[i];
	}

	for( int i = 0; i < 8; ++i )
	{
		for( int j = 0; j < 2; ++j )
		{
			base64::decode( _this.attribute( "samples_"+QString::number(i)+"_"+QString::number(j) ), &dst, &size );
			for( int k = 0; k < sampleSizes[i]; ++k )
			{
				samples[i][j].push_back( ( (float*) dst )[k] );
			}
		}
	}

	delete[] dst;

	for( int i = 0; i < 8; ++i )
	{
		m_morph[i]->loadSettings( _this, "morph_"+QString::number(i) );
		m_range[i]->loadSettings( _this, "range_"+QString::number(i) );
		m_modify[i]->loadSettings( _this, "modify_"+QString::number(i) );
		m_modifyMode[i]->loadSettings( _this, "modifyMode_"+QString::number(i) );
		m_spray[i]->loadSettings( _this, "spray_"+QString::number(i) );
		m_unisonVoices[i]->loadSettings( _this, "unisonVoices_"+QString::number(i) );
		m_unisonDetune[i]->loadSettings( _this, "unisonDetune_"+QString::number(i) );
		m_unisonMorph[i]->loadSettings( _this, "unisonMorph_"+QString::number(i) );
		m_unisonModify[i]->loadSettings( _this, "unisonModify_"+QString::number(i) );
		m_morphMax[i]->loadSettings( _this, "morphMax_"+QString::number(i) );
		m_detune[i]->loadSettings( _this, "detune_"+QString::number(i) );
		m_sampLen[i]->loadSettings( _this, "sampLen_"+QString::number(i) );
		m_phase[i]->loadSettings( _this, "phase_"+QString::number(i) );
		m_vol[i]->loadSettings( _this, "vol_"+QString::number(i) );
		m_filtInVol[i]->loadSettings( _this, "filtInVol_"+QString::number(i) );
		m_filtType[i]->loadSettings( _this, "filtType_"+QString::number(i) );
		m_filtSlope[i]->loadSettings( _this, "filtSlope_"+QString::number(i) );
		m_filtCutoff[i]->loadSettings( _this, "filtCutoff_"+QString::number(i) );
		m_filtReso[i]->loadSettings( _this, "filtReso_"+QString::number(i) );
		m_filtGain[i]->loadSettings( _this, "filtGain_"+QString::number(i) );
		m_filtSatu[i]->loadSettings( _this, "filtSatu_"+QString::number(i) );
		m_filtWetDry[i]->loadSettings( _this, "filtWetDry_"+QString::number(i) );
		m_filtBal[i]->loadSettings( _this, "filtBal_"+QString::number(i) );
		m_filtOutVol[i]->loadSettings( _this, "filtOutVol_"+QString::number(i) );
		m_filtEnabled[i]->loadSettings( _this, "filtEnabled_"+QString::number(i) );
		m_filtFeedback[i]->loadSettings( _this, "filtFeedback_"+QString::number(i) );
		m_filtDetune[i]->loadSettings( _this, "filtDetune_"+QString::number(i) );
		m_filtKeytracking[i]->loadSettings( _this, "filtKeytracking_"+QString::number(i) );
		m_enabled[i]->loadSettings( _this, "enabled_"+QString::number(i) );
		m_muted[i]->loadSettings( _this, "muted_"+QString::number(i) );
		m_sampleEnabled[i]->loadSettings( _this, "sampleEnabled_"+QString::number(i) );
		m_sampleGraphEnabled[i]->loadSettings( _this, "sampleGraphEnabled_"+QString::number(i) );
		m_sampleMuted[i]->loadSettings( _this, "sampleMuted_"+QString::number(i) );
		m_sampleKeytracking[i]->loadSettings( _this, "sampleKeytracking_"+QString::number(i) );
		m_sampleLoop[i]->loadSettings( _this, "sampleLoop_"+QString::number(i) );
		m_sampleVolume[i]->loadSettings( _this, "sampleVolume_"+QString::number(i) );
		m_samplePanning[i]->loadSettings( _this, "samplePanning_"+QString::number(i) );
		m_sampleDetune[i]->loadSettings( _this, "sampleDetune_"+QString::number(i) );
		m_samplePhase[i]->loadSettings( _this, "samplePhase_"+QString::number(i) );
		m_samplePhaseRand[i]->loadSettings( _this, "samplePhaseRand_"+QString::number(i) );
		m_sampleStart[i]->loadSettings( _this, "sampleStart_"+QString::number(i) );
		m_sampleEnd[i]->loadSettings( _this, "sampleEnd_"+QString::number(i) );
	}

	for( int i = 0; i < 64; ++i )
	{
		m_subEnabled[i]->loadSettings( _this, "subEnabled_"+QString::number(i) );
		m_subVol[i]->loadSettings( _this, "subVol_"+QString::number(i) );
		m_subPhase[i]->loadSettings( _this, "subPhase_"+QString::number(i) );
		m_subPhaseRand[i]->loadSettings( _this, "subPhaseRand_"+QString::number(i) );
		m_subDetune[i]->loadSettings( _this, "subDetune_"+QString::number(i) );
		m_subMuted[i]->loadSettings( _this, "subMuted_"+QString::number(i) );
		m_subKeytrack[i]->loadSettings( _this, "subKeytrack_"+QString::number(i) );
		m_subSampLen[i]->loadSettings( _this, "subSampLen_"+QString::number(i) );
		m_subNoise[i]->loadSettings( _this, "subNoise_"+QString::number(i) );
		m_subPanning[i]->loadSettings( _this, "subPanning_"+QString::number(i) );

		m_modIn[i]->loadSettings( _this, "modIn_"+QString::number(i) );
		m_modInNum[i]->loadSettings( _this, "modInNum_"+QString::number(i) );
		m_modInOtherNum[i]->loadSettings( _this, "modInOtherNum_"+QString::number(i) );
		m_modInAmnt[i]->loadSettings( _this, "modInAmnt_"+QString::number(i) );
		m_modInCurve[i]->loadSettings( _this, "modInCurve_"+QString::number(i) );
		m_modIn2[i]->loadSettings( _this, "modIn2_"+QString::number(i) );
		m_modInNum2[i]->loadSettings( _this, "modInNum2_"+QString::number(i) );
		m_modInOtherNum2[i]->loadSettings( _this, "modInOtherNum2_"+QString::number(i) );
		m_modInAmnt2[i]->loadSettings( _this, "modAmnt2_"+QString::number(i) );
		m_modInCurve2[i]->loadSettings( _this, "modCurve2_"+QString::number(i) );
		m_modOutSec[i]->loadSettings( _this, "modOutSec_"+QString::number(i) );
		m_modOutSig[i]->loadSettings( _this, "modOutSig_"+QString::number(i) );
		m_modOutSecNum[i]->loadSettings( _this, "modOutSecNum_"+QString::number(i) );
		m_modEnabled[i]->loadSettings( _this, "modEnabled_"+QString::number(i) );
		m_modCombineType[i]->loadSettings( _this, "modCombineType_"+QString::number(i) );
	}

	m_interpolation.loadSettings( _this, "interpolation" );
	
	m_normalize.loadSettings( _this, "normalize" );

}


// When a knob is changed, send the new value to the array holding the knob values, as well as the note values within mSynths already initialized (notes already playing)
void Microwave::valueChanged( int which, int num )
{
	//Send new values to array
	switch( which )
	{
		case 1: m_morphArr[num] = m_morph[num]->value(); break;
		case 2: m_rangeArr[num] = m_range[num]->value(); break;
		case 3: m_modifyArr[num] = m_modify[num]->value(); break;
		case 4: m_modifyModeArr[num] = m_modifyMode[num]->value(); break;
		case 5: m_sprayArr[num] = m_spray[num]->value(); break;
		case 6: m_unisonVoicesArr[num] = m_unisonVoices[num]->value(); break;
		case 7: m_unisonDetuneArr[num] = m_unisonDetune[num]->value(); break;
		case 8: m_unisonMorphArr[num] = m_unisonMorph[num]->value(); break;
		case 9: m_unisonModifyArr[num] = m_unisonModify[num]->value(); break;
		case 10: m_morphMaxArr[num] = m_morphMax[num]->value(); break;
		case 11: m_detuneArr[num] = m_detune[num]->value(); break;
		case 12: m_sampLenArr[num] = m_sampLen[num]->value(); break;
		case 13: m_phaseArr[num] = m_phase[num]->value(); break;
		case 14: m_volArr[num] = m_vol[num]->value(); break;
		case 15: m_filtInVolArr[num] = m_filtInVol[num]->value(); break;
		case 16: m_filtTypeArr[num] = m_filtType[num]->value(); break;
		case 17: m_filtSlopeArr[num] = m_filtSlope[num]->value(); break;
		case 18: m_filtCutoffArr[num] = m_filtCutoff[num]->value(); break;
		case 19: m_filtResoArr[num] = m_filtReso[num]->value(); break;
		case 20: m_filtGainArr[num] = m_filtGain[num]->value(); break;
		case 21: m_filtSatuArr[num] = m_filtSatu[num]->value(); break;
		case 22: m_filtWetDryArr[num] = m_filtWetDry[num]->value(); break;
		case 23: m_filtBalArr[num] = m_filtBal[num]->value(); break;
		case 24: m_filtOutVolArr[num] = m_filtOutVol[num]->value(); break;
		case 25: m_filtEnabledArr[num] = m_filtEnabled[num]->value(); break;
		case 26: m_subEnabledArr[num] = m_subEnabled[num]->value(); break;
		case 27: m_subVolArr[num] = m_subVol[num]->value(); break;
		case 28: m_subPhaseArr[num] = m_subPhase[num]->value(); break;
		case 29: m_subPhaseRandArr[num] = m_subPhaseRand[num]->value(); break;
		case 30: m_subDetuneArr[num] = m_subDetune[num]->value(); break;
		case 31: m_subMutedArr[num] = m_subMuted[num]->value(); break;
		case 32: m_subKeytrackArr[num] = m_subKeytrack[num]->value(); break;
		case 33: m_subSampLenArr[num] = m_subSampLen[num]->value(); break;
		case 34: m_subNoiseArr[num] = m_subNoise[num]->value(); break;
		case 35: m_modInArr[num] = m_modIn[num]->value(); break;
		case 36: m_modInNumArr[num] = m_modInNum[num]->value(); break;
		case 37: m_modInOtherNumArr[num] = m_modInOtherNum[num]->value(); break;
		case 38: m_modInAmntArr[num] = m_modInAmnt[num]->value(); break;
		case 39: m_modInCurveArr[num] = m_modInCurve[num]->value(); break;
		case 40: m_modIn2Arr[num] = m_modIn2[num]->value(); break;
		case 41: m_modInNum2Arr[num] = m_modInNum2[num]->value(); break;
		case 42: m_modInOtherNum2Arr[num] = m_modInOtherNum2[num]->value(); break;
		case 43: m_modInAmnt2Arr[num] = m_modInAmnt2[num]->value(); break;
		case 44: m_modInCurve2Arr[num] = m_modInCurve2[num]->value(); break;
		case 45: m_modOutSecArr[num] = m_modOutSec[num]->value(); break;
		case 46: m_modOutSigArr[num] = m_modOutSig[num]->value(); break;
		case 47: m_modOutSecNumArr[num] = m_modOutSecNum[num]->value(); break;
		case 48: m_enabledArr[num] = m_enabled[num]->value(); break;
		case 49: m_modEnabledArr[num] = m_modEnabled[num]->value(); break;
		case 50: m_modCombineTypeArr[num] = m_modCombineType[num]->value(); break;
		case 57: m_mutedArr[num] = m_muted[num]->value(); break;
		case 59: m_sampleEnabledArr[num] = m_sampleEnabled[num]->value(); break;
		case 60: m_sampleGraphEnabledArr[num] = m_sampleGraphEnabled[num]->value(); break;
		case 61: m_sampleMutedArr[num] = m_sampleMuted[num]->value(); break;
		case 62: m_sampleKeytrackingArr[num] = m_sampleKeytracking[num]->value(); break;
		case 63: m_sampleLoopArr[num] = m_sampleLoop[num]->value(); break;
		case 64: m_sampleVolumeArr[num] = m_sampleVolume[num]->value(); break;
		case 65: m_samplePanningArr[num] = m_samplePanning[num]->value(); break;
		case 66: m_sampleDetuneArr[num] = m_sampleDetune[num]->value(); break;
		case 67: m_samplePhaseArr[num] = m_samplePhase[num]->value(); break;
		case 68: m_samplePhaseRandArr[num] = m_samplePhaseRand[num]->value(); break;
		case 69: m_filtFeedbackArr[num] = m_filtFeedback[num]->value(); break;
		case 70: m_filtDetuneArr[num] = m_filtDetune[num]->value(); break;
		case 71: m_filtKeytrackingArr[num] = m_filtKeytracking[num]->value(); break;
		case 72: m_subPanningArr[num] = m_subPanning[num]->value(); break;
		case 73: m_sampleStartArr[num] = m_sampleStart[num]->value(); break;
		case 74: m_sampleEndArr[num] = m_sampleEnd[num]->value(); break;
	}

	ConstNotePlayHandleList nphList = NotePlayHandle::nphsOfInstrumentTrack( microwaveTrack );

	for( int i = 0; i < nphList.length(); ++i )
	{
		mSynth * ps;
		do
		{
			ps = static_cast<mSynth *>( nphList[i]->m_pluginData );
		}
		while ( !ps );// Makes sure "ps" isn't assigned a null value, if m_pluginData hasn't been created yet.
		// Above is possible CPU problem, definitely will not work with single-threaded systems.  Fix this.

		//Send new knob values to notes already playing
		switch( which )
		{
			case 1: ps->_morphVal[num] = m_morph[num]->value(); break;
			case 2: ps->rangeVal[num] = m_range[num]->value(); break;
			case 3: ps->_modifyVal[num] = m_modify[num]->value(); break;
			case 4: ps->modifyModeVal[num] = m_modifyMode[num]->value(); break;
			case 5: ps->sprayVal[num] = m_spray[num]->value(); break;
			case 6: ps->unisonVoices[num] = m_unisonVoices[num]->value(); break;
			case 7: ps->unisonDetune[num] = m_unisonDetune[num]->value(); break;
			case 8: ps->unisonMorph[num] = m_unisonMorph[num]->value(); break;
			case 9: ps->unisonModify[num] = m_unisonModify[num]->value(); break;
			case 10: ps->morphMaxVal[num] = m_morphMax[num]->value(); break;
			case 11: ps->detuneVal[num] = m_detune[num]->value(); break;
			case 12: ps->sampLen[num] = m_sampLen[num]->value(); break;
			case 13: ps->phase[num] = m_phase[num]->value(); break;
			case 14: ps->vol[num] = m_vol[num]->value(); break;
			case 15: ps->filtInVol[num] = m_filtInVol[num]->value(); break;
			case 16: ps->filtType[num] = m_filtType[num]->value(); break;
			case 17: ps->filtSlope[num] = m_filtSlope[num]->value(); break;
			case 18: ps->filtCutoff[num] = m_filtCutoff[num]->value(); break;
			case 19: ps->filtReso[num] = m_filtReso[num]->value(); break;
			case 20: ps->filtGain[num] = m_filtGain[num]->value(); break;
			case 21: ps->filtSatu[num] = m_filtSatu[num]->value(); break;
			case 22: ps->filtWetDry[num] = m_filtWetDry[num]->value(); break;
			case 23: ps->filtBal[num] = m_filtBal[num]->value(); break;
			case 24: ps->filtOutVol[num] = m_filtOutVol[num]->value(); break;
			case 25: ps->filtEnabled[num] = m_filtEnabled[num]->value(); break;
			case 26: ps->subEnabled[num] = m_subEnabled[num]->value(); break;
			case 27: ps->subVol[num] = m_subVol[num]->value(); break;
			case 28: ps->subPhase[num] = m_subPhase[num]->value(); break;
			case 29: ps->subPhaseRand[num] = m_subPhaseRand[num]->value(); break;
			case 30: ps->subDetune[num] = m_subDetune[num]->value(); break;
			case 31: ps->subMuted[num] = m_subMuted[num]->value(); break;
			case 32: ps->subKeytrack[num] = m_subKeytrack[num]->value(); break;
			case 33: ps->subSampLen[num] = m_subSampLen[num]->value(); break;
			case 34: ps->subNoise[num] = m_subNoise[num]->value(); break;
			case 35: ps->m_modIn[num] = m_modIn[num]->value(); break;
			case 36: ps->m_modInNum[num] = m_modInNum[num]->value(); break;
			case 37: ps->m_modInOtherNum[num] = m_modInOtherNum[num]->value(); break;
			case 38: ps->m_modInAmnt[num] = m_modInAmnt[num]->value(); break;
			case 39: ps->m_modInCurve[num] = m_modInCurve[num]->value(); break;
			case 40: ps->m_modIn2[num] = m_modIn2[num]->value(); break;
			case 41: ps->m_modInNum2[num] = m_modInNum2[num]->value(); break;
			case 42: ps->m_modInOtherNum2[num] = m_modInOtherNum2[num]->value(); break;
			case 43: ps->m_modInAmnt2[num] = m_modInAmnt2[num]->value(); break;
			case 44: ps->m_modInCurve2[num] = m_modInCurve2[num]->value(); break;
			case 45: ps->m_modOutSec[num] = m_modOutSec[num]->value(); break;
			case 46: ps->m_modOutSig[num] = m_modOutSig[num]->value(); break;
			case 47: ps->m_modOutSecNum[num] = m_modOutSecNum[num]->value(); break;
			case 48: ps->enabled[num] = m_enabled[num]->value(); break;
			case 49: ps->modEnabled[num] = m_modEnabled[num]->value(); break;
			case 50: ps->m_modCombineType[num] = m_modCombineType[num]->value(); break;
			case 57: ps->muted[num] = m_muted[num]->value(); break;
			case 59: ps->sampleEnabled[num] = m_sampleEnabled[num]->value(); break;
			case 60: ps->sampleGraphEnabled[num] = m_sampleGraphEnabled[num]->value(); break;
			case 61: ps->sampleMuted[num] = m_sampleMuted[num]->value(); break;
			case 62: ps->sampleKeytracking[num] = m_sampleKeytracking[num]->value(); break;
			case 63: ps->sampleLoop[num] = m_sampleLoop[num]->value(); break;
			case 64: ps->sampleVolume[num] = m_sampleVolume[num]->value(); break;
			case 65: ps->samplePanning[num] = m_samplePanning[num]->value(); break;
			case 66: ps->sampleDetune[num] = m_sampleDetune[num]->value(); break;
			case 67: ps->samplePhase[num] = m_samplePhase[num]->value(); break;
			case 68: ps->samplePhaseRand[num] = m_samplePhaseRand[num]->value(); break;
			case 69: ps->filtFeedback[num] = m_filtFeedback[num]->value(); break;
			case 70: ps->filtDetune[num] = m_filtDetune[num]->value(); break;
			case 71: ps->filtKeytracking[num] = m_filtKeytracking[num]->value(); break;
			case 72: ps->subPanning[num] = m_subPanning[num]->value(); break;
			case 73: ps->sampleStart[num] = m_sampleStart[num]->value(); break;
			case 74: ps->sampleEnd[num] = m_sampleEnd[num]->value(); break;
		}
	}
}


// Set the range of Morph based on Morph Max
void Microwave::morphMaxChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		m_morph[i]->setRange( m_morph[i]->getMin(), m_morphMax[i]->value(), m_morph[i]->getStep() );
	}
}


// Set the range of morphMax and Modify based on new sample length
void Microwave::sampLenChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		m_morphMax[i]->setRange( m_morphMax[i]->getMin(), 524288.f/m_sampLen[i]->value() - 2, m_morphMax[i]->getStep() );
		m_modify[i]->setRange( m_modify[i]->getMin(), m_sampLen[i]->value() - 1, m_modify[i]->getStep() );
	}
}


//Change graph length to sample length
void Microwave::subSampLenChanged( int num )
{
	m_graph.setLength( m_subSampLen[num]->value() );
}


//Stores the highest enabled sub oscillator.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::subEnabledChanged( int num )
{
	for( int i = 0; i < 64; ++i )
	{
		if( m_subEnabled[i]->value() )
		{
			maxSubEnabled = i+1;
		}
	}
}


//Stores the highest enabled mod section.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::modEnabledChanged( int num )
{
	for( int i = 0; i < 64; ++i )
	{
		if( m_modEnabled[i]->value() )
		{
			maxModEnabled = i+1;
		}
	}
}


//Stores the highest enabled sample.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::sampleEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( m_sampleEnabled[i]->value() )
		{
			maxSampleEnabled = i+1;
		}
	}
}


//When user drawn on graph, send new values to the correct arrays
void Microwave::samplesChanged( int _begin, int _end )
{
	normalize();
	//engine::getSongEditor()->setModified();
	switch( currentTab )
	{
		case 0:
			break;
		case 1:
			for( int i = _begin; i <= _end; ++i )
			{
				subs[i + ( (m_subNum.value()-1) * 1024 )] = m_graph.samples()[i];
			}
			break;
		case 2:
			for( int i = _begin; i <= _end; ++i )
			{
				sampGraphs[i + ( (m_sampNum.value()-1) * 128 )] = m_graph.samples()[i];
			}
			break;
		case 3:
			break;
	}
}


// This will probably be deleted before release.  If not, go annoy Douglas.
void Microwave::normalize()
{
	// analyze
	float max = 0;
	const float* samplesN = m_graph.samples();
	for(int i=0; i < m_graph.length(); ++i)
	{
		const float f = fabsf( samplesN[i] );
		if (f > max) { max = f; }
	}
	m_normalizeFactor = 1.0 / max;
}


// For when notes are playing.  This initializes a new mSynth if the note is new.  It also uses mSynth::nextStringSample to get the synthesizer output.  This is where oversampling and the visualizer are handled.
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
		float m_phaseRandArr[8] = {0};// Minor CPU problem here
		for( int i = 0; i < 8; ++i )
		{
			m_phaseRandArr[i] = m_phaseRand[i]->value();
		}
		_n->m_pluginData = new mSynth(
					_n,
					factor,
				Engine::mixer()->processingSampleRate(),
					m_phaseRandArr,
					m_modifyModeArr, m_modifyArr, m_morphArr, m_rangeArr, m_sprayArr, m_unisonVoicesArr, m_unisonDetuneArr, m_unisonMorphArr, m_unisonModifyArr, m_morphMaxArr, m_detuneArr, waveforms, subs, m_subEnabledArr, m_subVolArr, m_subPhaseArr, m_subPhaseRandArr, m_subDetuneArr, m_subMutedArr, m_subKeytrackArr, m_subSampLenArr, m_subNoiseArr, m_sampLenArr, m_modInArr, m_modInNumArr, m_modInOtherNumArr, m_modInAmntArr, m_modInCurveArr, m_modIn2Arr, m_modInNum2Arr, m_modInOtherNum2Arr, m_modInAmnt2Arr, m_modInCurve2Arr, m_modOutSecArr, m_modOutSigArr, m_modOutSecNumArr, m_modCombineTypeArr, m_phaseArr, m_volArr, m_filtInVolArr, m_filtTypeArr, m_filtSlopeArr, m_filtCutoffArr, m_filtResoArr, m_filtGainArr, m_filtSatuArr, m_filtWetDryArr, m_filtBalArr, m_filtOutVolArr, m_filtEnabledArr, m_enabledArr, m_modEnabledArr, sampGraphs, m_mutedArr, m_sampleEnabledArr, m_sampleGraphEnabledArr, m_sampleMutedArr, m_sampleKeytrackingArr, m_sampleLoopArr, m_sampleVolumeArr, m_samplePanningArr, m_sampleDetuneArr, m_samplePhaseArr, m_samplePhaseRandArr, samples, m_filtFeedbackArr, m_filtDetuneArr, m_filtKeytrackingArr, m_subPanningArr, m_sampleStartArr, m_sampleEndArr );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	mSynth * ps = static_cast<mSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		// Process some samples and ignore the output, depending on the oversampling value.  For example, if the oversampling is set to 4x, it will process 4 samples and output 1 of those.
		for( int i = 0; i < m_oversample.value(); ++i )
		{
			ps->nextStringSample( waveforms, subs, sampGraphs, samples, maxModEnabled, maxSubEnabled, maxSampleEnabled, Engine::mixer()->processingSampleRate() * (m_oversample.value()+1) );
		}
		//Get the actual synthesizer output
		std::vector<float> sample = ps->nextStringSample( waveforms, subs, sampGraphs, samples, maxModEnabled, maxSubEnabled, maxSampleEnabled, Engine::mixer()->processingSampleRate() * (m_oversample.value()+1) );

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			//Send to output
			_working_buffer[frame][chnl] = sample[chnl];
		}

		//update visualizer
		if( m_visualize.value() )
		{
			if( abs( const_cast<float*>( m_graph.samples() )[int(ps->sample_realindex[0][0])] - ( ((sample[0]+sample[1])/2.f) * m_visvol.value() / 100 ) ) >= 0.01f )
			{
				m_graph.setSampleAt( ps->sample_realindex[0][0], ((sample[0]+sample[1])/2.f) * m_visvol.value() / 100 );
			}		
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}


void Microwave::deleteNotePluginData( NotePlayHandle * _n )
{
}




/*

          ____                                                                                                                                
        ,'  , `.                                                                                                                              
     ,-+-,.' _ |  ,--,                                                                                 ,---.  ,--,                            
  ,-+-. ;   , ||,--.'|              __  ,-.   ,---.           .---.                                   /__./|,--.'|                      .---. 
 ,--.'|'   |  ;||  |,             ,' ,'/ /|  '   ,'\         /. ./|                .---.         ,---.;  ; ||  |,                      /. ./| 
|   |  ,', |  ':`--'_       ,---. '  | |' | /   /   |     .-'-. ' |  ,--.--.     /.  ./|   ,---./___/ \  | |`--'_       ,---.       .-'-. ' | 
|   | /  | |  ||,' ,'|     /     \|  |   ,'.   ; ,. :    /___/ \: | /       \  .-' . ' |  /     \   ;  \ ' |,' ,'|     /     \     /___/ \: | 
'   | :  | :  |,'  | |    /    / ''  :  /  '   | |: : .-'.. '   ' ..--.  .-. |/___/ \: | /    /  \   \  \: |'  | |    /    /  | .-'.. '   ' . 
;   . |  ; |--' |  | :   .    ' / |  | '   '   | .; :/___/ \:     ' \__\/: . ..   \  ' ..    ' / |;   \  ' .|  | :   .    ' / |/___/ \:     ' 
|   : |  | ,    '  : |__ '   ; :__;  : |   |   :    |.   \  ' .\    ," .--.; | \   \   ''   ;   /| \   \   ''  : |__ '   ;   /|.   \  ' .\    
|   : '  |/     |  | '.'|'   | '.'|  , ;    \   \  /  \   \   ' \ |/  /  ,.  |  \   \   '   |  / |  \   `  ;|  | '.'|'   |  / | \   \   ' \ | 
;   | |`-'      ;  :    ;|   :    :---'      `----'    \   \  |--";  :   .'   \  \   \ ||   :    |   :   \ |;  :    ;|   :    |  \   \  |--"  
|   ;/          |  ,   /  \   \  /                      \   \ |   |  ,     .-./   '---"  \   \  /     '---" |  ,   /  \   \  /    \   \ |     
'---'            ---`-'    `----'                        '---"     `--`---'               `----'             ---`-'    `----'      '---"      
                                                                                                                                              


*/





// Creates the Microwave GUI.  Creates all GUI elements.  Connects some events to some functions.  Calls updateScroll() to put all of the GUi elements in their correct positions.
MicrowaveView::MicrowaveView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );

	setMouseTracking( true );

	setAcceptDrops( true );

	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	QWidget * view = new QWidget( _parent );
	view->setFixedSize( 250, 250 );

	for( int i = 0; i < 8; ++i )
	{
		m_morphKnob[i] = new Knob( knobMicrowave, this );
		m_morphKnob[i]->setHintText( tr( "Morph" ), "" );
	
		m_rangeKnob[i] = new Knob( knobMicrowave, this );
		m_rangeKnob[i]->setHintText( tr( "Range" ), "" );

		m_sampLenKnob[i] = new Knob( knobMicrowave, this );
		m_sampLenKnob[i]->setHintText( tr( "Waveform Sample Length" ), "" );

		m_morphMaxKnob[i] = new Knob( knobMicrowave, this );
		m_morphMaxKnob[i]->setHintText( tr( "Morph Max" ), "" );

		m_modifyKnob[i] = new Knob( knobMicrowave, this );
		m_modifyKnob[i]->setHintText( tr( "Modify" ), "" );

		m_sprayKnob[i] = new Knob( knobMicrowave, this );
		m_sprayKnob[i]->setHintText( tr( "Spray" ), "" );

		m_unisonVoicesKnob[i] = new Knob( knobSmallMicrowave, this );
		m_unisonVoicesKnob[i]->setHintText( tr( "Unison Voices" ), "" );

		m_unisonDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		m_unisonDetuneKnob[i]->setHintText( tr( "Unison Detune" ), "" );

		m_unisonMorphKnob[i] = new Knob( knobSmallMicrowave, this );
		m_unisonMorphKnob[i]->setHintText( tr( "Unison Morph" ), "" );

		m_unisonModifyKnob[i] = new Knob( knobSmallMicrowave, this );
		m_unisonModifyKnob[i]->setHintText( tr( "Unison Modify" ), "" );

		m_detuneKnob[i] = new Knob( knobSmallMicrowave, this );
		m_detuneKnob[i]->setHintText( tr( "Detune" ), "" );

		m_phaseKnob[i] = new Knob( knobSmallMicrowave, this );
		m_phaseKnob[i]->setHintText( tr( "Phase" ), "" );

		m_phaseRandKnob[i] = new Knob( knobSmallMicrowave, this );
		m_phaseRandKnob[i]->setHintText( tr( "Phase Randomness" ), "" );

		m_volKnob[i] = new Knob( knobMicrowave, this );
		m_volKnob[i]->setHintText( tr( "Volume" ), "" );

		m_modifyModeBox[i] = new ComboBox( this );
		m_modifyModeBox[i]->setGeometry( 0, 5, 42, 22 );
		m_modifyModeBox[i]->setFont( pointSize<8>( m_modifyModeBox[i]->font() ) );

		m_enabledToggle[i] = new LedCheckBox( "Oscillator Enabled", this, tr( "Oscillator Enabled" ), LedCheckBox::Green );

		m_mutedToggle[i] = new LedCheckBox( "Oscillator Muted", this, tr( "Oscillator Muted" ), LedCheckBox::Green );


		m_filtInVolKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtInVolKnob[i]->setHintText( tr( "Input Volume" ), "" );

		m_filtTypeBox[i] = new ComboBox( this );
		m_filtTypeBox[i]->setGeometry( 1000, 5, 42, 22 );
		m_filtTypeBox[i]->setFont( pointSize<8>( m_filtTypeBox[i]->font() ) );

		m_filtSlopeBox[i] = new ComboBox( this );
		m_filtSlopeBox[i]->setGeometry( 1000, 5, 42, 22 );
		m_filtSlopeBox[i]->setFont( pointSize<8>( m_filtSlopeBox[i]->font() ) );

		m_filtCutoffKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtCutoffKnob[i]->setHintText( tr( "Cutoff Frequency" ), "" );

		m_filtResoKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtResoKnob[i]->setHintText( tr( "Resonance" ), "" );

		m_filtGainKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtGainKnob[i]->setHintText( tr( "db Gain" ), "" );

		m_filtSatuKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtSatuKnob[i]->setHintText( tr( "Saturation" ), "" );

		m_filtWetDryKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtWetDryKnob[i]->setHintText( tr( "Wet/Dry" ), "" );

		m_filtBalKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtBalKnob[i]->setHintText( tr( "Balance/Panning" ), "" );

		m_filtOutVolKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtOutVolKnob[i]->setHintText( tr( "Output Volume" ), "" );

		m_filtEnabledToggle[i] = new LedCheckBox( "Filter Enabled", this, tr( "Filter Enabled" ), LedCheckBox::Green );

		m_filtFeedbackKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtFeedbackKnob[i]->setHintText( tr( "Feedback" ), "" );

		m_filtDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		m_filtDetuneKnob[i]->setHintText( tr( "Detune" ), "" );

		m_filtKeytrackingToggle[i] = new LedCheckBox( "Keytracking", this, tr( "Keytracking" ), LedCheckBox::Green );

		m_sampleEnabledToggle[i] = new LedCheckBox( "", this, tr( "Sample Enabled" ), LedCheckBox::Green );
		m_sampleGraphEnabledToggle[i] = new LedCheckBox( "", this, tr( "Sample Graph Enabled" ), LedCheckBox::Green );
		m_sampleMutedToggle[i] = new LedCheckBox( "", this, tr( "Sample Muted" ), LedCheckBox::Green );
		m_sampleKeytrackingToggle[i] = new LedCheckBox( "", this, tr( "Sample Keytracking" ), LedCheckBox::Green );
		m_sampleLoopToggle[i] = new LedCheckBox( "", this, tr( "Loop Sample" ), LedCheckBox::Green );

		m_sampleVolumeKnob[i] = new Knob( knobMicrowave, this );
		m_sampleVolumeKnob[i]->setHintText( tr( "Volume" ), "" );
		m_samplePanningKnob[i] = new Knob( knobMicrowave, this );
		m_samplePanningKnob[i]->setHintText( tr( "Panning" ), "" );
		m_sampleDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		m_sampleDetuneKnob[i]->setHintText( tr( "Detune" ), "" );
		m_samplePhaseKnob[i] = new Knob( knobSmallMicrowave, this );
		m_samplePhaseKnob[i]->setHintText( tr( "Phase" ), "" );
		m_samplePhaseRandKnob[i] = new Knob( knobSmallMicrowave, this );
		m_samplePhaseRandKnob[i]->setHintText( tr( "Phase Randomness" ), "" );
		m_sampleStartKnob[i] = new Knob( knobSmallMicrowave, this );
		m_sampleStartKnob[i]->setHintText( tr( "Start" ), "" );
		m_sampleEndKnob[i] = new Knob( knobSmallMicrowave, this );
		m_sampleEndKnob[i]->setHintText( tr( "End" ), "" );

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
			m_enabledToggle[i]->hide();
			m_mutedToggle[i]->hide();
			m_sampleEnabledToggle[i]->hide();
			m_sampleGraphEnabledToggle[i]->hide();
			m_sampleMutedToggle[i]->hide();
			m_sampleKeytrackingToggle[i]->hide();
			m_sampleLoopToggle[i]->hide();
			m_sampleVolumeKnob[i]->hide();
			m_samplePanningKnob[i]->hide();
			m_sampleDetuneKnob[i]->hide();
			m_samplePhaseKnob[i]->hide();
			m_samplePhaseRandKnob[i]->hide();
			m_sampleStartKnob[i]->hide();
			m_sampleEndKnob[i]->hide();
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		m_subEnabledToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Enabled" ), LedCheckBox::Green );

		m_subVolKnob[i] = new Knob( knobMicrowave, this );
		m_subVolKnob[i]->setHintText( tr( "Sub Oscillator Volume" ), "" );

		m_subPhaseKnob[i] = new Knob( knobMicrowave, this );
		m_subPhaseKnob[i]->setHintText( tr( "Sub Oscillator Phase" ), "" );

		m_subPhaseRandKnob[i] = new Knob( knobMicrowave, this );
		m_subPhaseRandKnob[i]->setHintText( tr( "Sub Oscillator Phase Randomness" ), "" );

		m_subDetuneKnob[i] = new Knob( knobMicrowave, this );
		m_subDetuneKnob[i]->setHintText( tr( "Sub Oscillator Pitch" ), "" );

		m_subMutedToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Muted" ), LedCheckBox::Green );

		m_subKeytrackToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Keytracking Enabled" ), LedCheckBox::Green );

		m_subSampLenKnob[i] = new Knob( knobMicrowave, this );
		m_subSampLenKnob[i]->setHintText( tr( "Sub Oscillator Sample Length" ), "" );

		m_subNoiseToggle[i] = new LedCheckBox( "", this, tr( "Sub Noise Enabled" ), LedCheckBox::Green );

		m_subPanningKnob[i] = new Knob( knobMicrowave, this );
		m_subPanningKnob[i]->setHintText( tr( "Panning" ), "" );

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
			m_subPanningKnob[i]->hide();
		}

		m_modOutSecBox[i] = new ComboBox( this );
		m_modOutSecBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modOutSecBox[i]->setFont( pointSize<8>( m_modOutSecBox[i]->font() ) );

		m_modOutSigBox[i] = new ComboBox( this );
		m_modOutSigBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modOutSigBox[i]->setFont( pointSize<8>( m_modOutSigBox[i]->font() ) );

		m_modOutSecNumBox[i] = new LcdSpinBox( 3, "microwave", this, "Mod Output Number" );

		m_modInBox[i] = new ComboBox( this );
		m_modInBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modInBox[i]->setFont( pointSize<8>( m_modInBox[i]->font() ) );

		m_modInNumBox[i] = new LcdSpinBox( 3, "microwave", this, "Mod Input Number" );

		m_modInOtherNumBox[i] = new LcdSpinBox( 3, "microwave", this, "Mod Input Number" );

		m_modInAmntKnob[i] = new Knob( knobMicrowave, this );
		m_modInAmntKnob[i]->setHintText( tr( "Modulator Amount" ), "" );

		m_modInCurveKnob[i] = new Knob( knobMicrowave, this );
		m_modInCurveKnob[i]->setHintText( tr( "Modulator Curve" ), "" );

		m_modInBox2[i] = new ComboBox( this );
		m_modInBox2[i]->setGeometry( 2000, 5, 42, 22 );
		m_modInBox2[i]->setFont( pointSize<8>( m_modInBox2[i]->font() ) );

		m_modInNumBox2[i] = new LcdSpinBox( 3, "microwave", this, "Secondary Mod Input Number" );

		m_modInOtherNumBox2[i] = new LcdSpinBox( 3, "microwave", this, "Secondary Mod Input Number" );

		m_modInAmntKnob2[i] = new Knob( knobMicrowave, this );
		m_modInAmntKnob2[i]->setHintText( tr( "Secondary Modulator Amount" ), "" );

		m_modInCurveKnob2[i] = new Knob( knobMicrowave, this );
		m_modInCurveKnob2[i]->setHintText( tr( "Secondary Modulator Curve" ), "" );

		m_modEnabledToggle[i] = new LedCheckBox( "Enabled", this, tr( "Modulation Enabled" ), LedCheckBox::Green );

		m_modCombineTypeBox[i] = new ComboBox( this );
		m_modCombineTypeBox[i]->setGeometry( 2000, 5, 42, 22 );
		m_modCombineTypeBox[i]->setFont( pointSize<8>( m_modCombineTypeBox[i]->font() ) );
	}

	m_scrollKnob = new Knob( knobSmallMicrowave, this );
	m_modScrollKnob = new Knob( knobSmallMicrowave, this );
	m_effectScrollKnob = new Knob( knobSmallMicrowave, this );

	m_visvolKnob = new Knob( knobMicrowave, this );
	m_visvolKnob->setHintText( tr( "Visualizer Volume" ), "" );

	m_loadAlgKnob = new Knob( knobMicrowave, this );
	m_loadAlgKnob->setHintText( tr( "Wavetable Loading Algorithm" ), "" );

	m_loadChnlKnob = new Knob( knobMicrowave, this );
	m_loadChnlKnob->setHintText( tr( "Wavetable Loading Channel" ), "" );


	m_graph = new Graph( this, Graph::BarCenterGradStyle, 204, 134 );
	m_graph->move(23,30);
	m_graph->setAutoFillBackground( true );
	m_graph->setGraphColor( QColor( 121, 222, 239 ) );

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
	m_sinWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	m_sinWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	ToolTip::add( m_sinWaveBtn,
			tr( "Sine wave" ) );

	m_triangleWaveBtn = new PixmapButton( this, tr( "Nachos" ) );
	m_triangleWaveBtn->move( 131 + 14, WAVE_BUTTON_Y );
	m_triangleWaveBtn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	m_triangleWaveBtn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( m_triangleWaveBtn,
			tr( "Nacho wave" ) );

	m_sawWaveBtn = new PixmapButton( this, tr( "Sawsa" ) );
	m_sawWaveBtn->move( 131 + 14*2, WAVE_BUTTON_Y  );
	m_sawWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	m_sawWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	ToolTip::add( m_sawWaveBtn,
			tr( "Sawsa wave" ) );

	m_sqrWaveBtn = new PixmapButton( this, tr( "Sosig" ) );
	m_sqrWaveBtn->move( 131 + 14*3, WAVE_BUTTON_Y );
	m_sqrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	m_sqrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	ToolTip::add( m_sqrWaveBtn,
			tr( "Sosig wave" ) );

	m_whiteNoiseWaveBtn = new PixmapButton( this,
					tr( "Metal Fork" ) );
	m_whiteNoiseWaveBtn->move( 131 + 14*4, WAVE_BUTTON_Y );
	m_whiteNoiseWaveBtn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( m_whiteNoiseWaveBtn,
			tr( "Metal Fork" ) );

	m_usrWaveBtn = new PixmapButton( this, tr( "Takeout" ) );
	m_usrWaveBtn->move( 131 + 14*5, WAVE_BUTTON_Y );
	m_usrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	m_usrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	ToolTip::add( m_usrWaveBtn,
			tr( "Takeout Menu" ) );

	m_smoothBtn = new PixmapButton( this, tr( "Microwave Cover" ) );
	m_smoothBtn->move( 131 + 14*6, WAVE_BUTTON_Y );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	ToolTip::add( m_smoothBtn,
			tr( "Microwave Cover" ) );


	m_sinWave2Btn = new PixmapButton( this, tr( "Sine" ) );
	m_sinWave2Btn->move( 131, WAVE_BUTTON_Y );
	m_sinWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	m_sinWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	ToolTip::add( m_sinWave2Btn,
			tr( "Sine wave" ) );

	m_triangleWave2Btn = new PixmapButton( this, tr( "Nachos" ) );
	m_triangleWave2Btn->move( 131 + 14, WAVE_BUTTON_Y );
	m_triangleWave2Btn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	m_triangleWave2Btn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( m_triangleWave2Btn,
			tr( "Nacho wave" ) );

	m_sawWave2Btn = new PixmapButton( this, tr( "Sawsa" ) );
	m_sawWave2Btn->move( 131 + 14*2, WAVE_BUTTON_Y  );
	m_sawWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	m_sawWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	ToolTip::add( m_sawWave2Btn,
			tr( "Sawsa wave" ) );

	m_sqrWave2Btn = new PixmapButton( this, tr( "Sosig" ) );
	m_sqrWave2Btn->move( 131 + 14*3, WAVE_BUTTON_Y );
	m_sqrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	m_sqrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	ToolTip::add( m_sqrWave2Btn,
			tr( "Sosig wave" ) );

	m_whiteNoiseWave2Btn = new PixmapButton( this,
					tr( "Metal Fork" ) );
	m_whiteNoiseWave2Btn->move( 131 + 14*4, WAVE_BUTTON_Y );
	m_whiteNoiseWave2Btn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	m_whiteNoiseWave2Btn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( m_whiteNoiseWave2Btn,
			tr( "Metal Fork" ) );

	m_usrWave2Btn = new PixmapButton( this, tr( "Takeout" ) );
	m_usrWave2Btn->move( 131 + 14*5, WAVE_BUTTON_Y );
	m_usrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	m_usrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	ToolTip::add( m_usrWave2Btn,
			tr( "Takeout Menu" ) );

	m_smooth2Btn = new PixmapButton( this, tr( "Microwave Cover" ) );
	m_smooth2Btn->move( 131 + 14*6, WAVE_BUTTON_Y );
	m_smooth2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	m_smooth2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	ToolTip::add( m_smooth2Btn,
			tr( "Microwave Cover" ) );


	m_interpolationToggle = new LedCheckBox( "Interpolation", this, tr( "Interpolation" ), LedCheckBox::Yellow );

	m_normalizeToggle = new LedCheckBox( "Normalize", this, tr( "Normalize" ), LedCheckBox::Green );

	m_visualizeToggle = new LedCheckBox( "", this, tr( "Visualize" ), LedCheckBox::Green );

	m_openWavetableButton = new PixmapButton( this );
	m_openWavetableButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openWavetableButton->move( 54, 220 );
	m_openWavetableButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	m_openWavetableButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	connect( m_openWavetableButton, SIGNAL( clicked() ),
					this, SLOT( openWavetableFileBtnClicked() ) );
	ToolTip::add( m_openWavetableButton, tr( "Open wavetable" ) );

	subNumBox = new LcdSpinBox( 2, "microwave", this, "Sub Oscillator Number" );

	sampNumBox = new LcdSpinBox( 2, "microwave", this, "Sample Number" );

	mainNumBox = new LcdSpinBox( 2, "microwave", this, "Oscillator Number" );

	m_oversampleBox = new ComboBox( this );
	m_oversampleBox->setGeometry( 2000, 5, 42, 22 );
	m_oversampleBox->setFont( pointSize<8>( m_oversampleBox->font() ) );
	
	m_openSampleButton = new PixmapButton( this );
	m_openSampleButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openSampleButton->move( 54, 220 );
	m_openSampleButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	m_openSampleButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	connect( m_openSampleButton, SIGNAL( clicked() ),
					this, SLOT( openSampleFileBtnClicked() ) );
	ToolTip::add( m_openSampleButton, tr( "Open sample" ) );


	
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

	connect( m_sinWave2Btn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );
	connect( m_triangleWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked() ) );
	connect( m_sawWave2Btn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );
	connect( m_sqrWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );
	connect( m_whiteNoiseWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );
	connect( m_usrWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );
	connect( m_smooth2Btn, SIGNAL ( clicked () ),
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

	connect( &castModel<Microwave>()->m_modScroll, SIGNAL( dataChanged( ) ),
			this, SLOT( updateScroll( ) ) );

	connect( &castModel<Microwave>()->m_effectScroll, SIGNAL( dataChanged( ) ),
			this, SLOT( updateScroll( ) ) );

	connect( &castModel<Microwave>()->m_mainNum, SIGNAL( dataChanged( ) ),
			this, SLOT( mainNumChanged( ) ) );

	connect( &castModel<Microwave>()->m_subNum, SIGNAL( dataChanged( ) ),
			this, SLOT( subNumChanged( ) ) );

	connect( &castModel<Microwave>()->m_sampNum, SIGNAL( dataChanged( ) ),
			this, SLOT( sampNumChanged( ) ) );

	for( int i = 0; i < 64; ++i )
	{
		connect( castModel<Microwave>()->m_modOutSec[i], &ComboBoxModel::dataChanged,
            		this, [this, i]() { modOutSecChanged(i); } );
		connect( castModel<Microwave>()->m_modIn[i], &ComboBoxModel::dataChanged,
            		this, [this, i]() { modInChanged(i); } );
	}

	updateScroll();
}


// Connects knobs/GUI elements to their models
void MicrowaveView::modelChanged()
{
	Microwave * b = castModel<Microwave>();

	for( int i = 0; i < 8; ++i )
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
		m_enabledToggle[i]->setModel( b->m_enabled[i] );
		m_mutedToggle[i]->setModel( b->m_muted[i] );

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
		m_filtFeedbackKnob[i]->setModel( b->m_filtFeedback[i] );
		m_filtDetuneKnob[i]->setModel( b->m_filtDetune[i] );
		m_filtKeytrackingToggle[i]->setModel( b->m_filtKeytracking[i] );

		m_sampleEnabledToggle[i]->setModel( b->m_sampleEnabled[i] );
		m_sampleGraphEnabledToggle[i]->setModel( b->m_sampleGraphEnabled[i] );
		m_sampleMutedToggle[i]->setModel( b->m_sampleMuted[i] );
		m_sampleKeytrackingToggle[i]->setModel( b->m_sampleKeytracking[i] );
		m_sampleLoopToggle[i]->setModel( b->m_sampleLoop[i] );

		m_sampleVolumeKnob[i]->setModel( b->m_sampleVolume[i] );
		m_samplePanningKnob[i]->setModel( b->m_samplePanning[i] );
		m_sampleDetuneKnob[i]->setModel( b->m_sampleDetune[i] );
		m_samplePhaseKnob[i]->setModel( b->m_samplePhase[i] );
		m_samplePhaseRandKnob[i]->setModel( b->m_samplePhaseRand[i] );
		m_sampleStartKnob[i]->setModel( b->m_sampleStart[i] );
		m_sampleEndKnob[i]->setModel( b->m_sampleEnd[i] );

	}

	for( int i = 0; i < 64; ++i )
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
		m_subPanningKnob[i]->setModel( b->m_subPanning[i] );

		m_modOutSecBox[i]-> setModel( b-> m_modOutSec[i] );
		m_modOutSigBox[i]-> setModel( b-> m_modOutSig[i] );
		m_modOutSecNumBox[i]-> setModel( b-> m_modOutSecNum[i] );

		m_modInBox[i]-> setModel( b-> m_modIn[i] );
		m_modInNumBox[i]-> setModel( b-> m_modInNum[i] );
		m_modInOtherNumBox[i]-> setModel( b-> m_modInOtherNum[i] );
		m_modInAmntKnob[i]-> setModel( b-> m_modInAmnt[i] );
		m_modInCurveKnob[i]-> setModel( b-> m_modInCurve[i] );

		m_modInBox2[i]-> setModel( b-> m_modIn2[i] );
		m_modInNumBox2[i]-> setModel( b-> m_modInNum2[i] );
		m_modInOtherNumBox2[i]-> setModel( b-> m_modInOtherNum2[i] );
		m_modInAmntKnob2[i]-> setModel( b-> m_modInAmnt2[i] );
		m_modInCurveKnob2[i]-> setModel( b-> m_modInCurve2[i] );

		m_modEnabledToggle[i]-> setModel( b-> m_modEnabled[i] );

		m_modCombineTypeBox[i]-> setModel( b-> m_modCombineType[i] );
	}

	m_graph->setModel( &b->m_graph );
	m_visvolKnob->setModel( &b->m_visvol );
	m_interpolationToggle->setModel( &b->m_interpolation );
	m_normalizeToggle->setModel( &b->m_normalize );
	m_visualizeToggle->setModel( &b->m_visualize );
	subNumBox->setModel( &b->m_subNum );
	sampNumBox->setModel( &b->m_sampNum );
	m_loadAlgKnob->setModel( &b->m_loadAlg );
	m_loadChnlKnob->setModel( &b->m_loadChnl );
	m_scrollKnob->setModel( &b->m_scroll );
	m_modScrollKnob->setModel( &b->m_modScroll );
	m_effectScrollKnob->setModel( &b->m_effectScroll );
	mainNumBox->setModel( &b->m_mainNum );
	m_oversampleBox->setModel( &b->m_oversample );
}


// Puts all of the GUI elements in their correct locations, depending on the scroll knob value.
void MicrowaveView::updateScroll()
{
	
	int scrollVal = ( castModel<Microwave>()->m_scroll.value() - 1 ) * 250.f;
	int modScrollVal = ( castModel<Microwave>()->m_modScroll.value() - 1 ) * 120.f;
	int effectScrollVal = ( castModel<Microwave>()->m_effectScroll.value() - 1 ) * 120.f;

	for( int i = 0; i < 8; ++i )
	{
		m_morphKnob[i]->move( 23 - scrollVal, 172 );
		m_rangeKnob[i]->move( 55 - scrollVal, 172 );
		m_sampLenKnob[i]->move( 0 - scrollVal, 50 );
		m_morphMaxKnob[i]->move( 0 - scrollVal, 75 );
		m_modifyKnob[i]->move( 87 - scrollVal, 172 );
		m_modifyModeBox[i]->move( 127 - scrollVal, 186 );
		m_sprayKnob[i]->move( 0 - scrollVal, 125 );
		m_unisonVoicesKnob[i]->move( 184 - scrollVal, 172 );
		m_unisonDetuneKnob[i]->move( 209 - scrollVal, 172 );
		m_unisonMorphKnob[i]->move( 184 - scrollVal, 203 );
		m_unisonModifyKnob[i]->move( 209 - scrollVal, 203 );
		m_detuneKnob[i]->move( 148 - scrollVal, 216 );
		m_phaseKnob[i]->move( 86 - scrollVal, 216 );
		m_phaseRandKnob[i]->move( 113 - scrollVal, 216 );
		m_volKnob[i]->move( 0 - scrollVal, 100 );
		m_enabledToggle[i]->move( 50 - scrollVal, 240 );
		m_mutedToggle[i]->move( 0 - scrollVal, 205 );

		m_filtInVolKnob[i]->move( 20 + 1000 - scrollVal, i*60 - effectScrollVal );
		m_filtTypeBox[i]->move( 50 + 1000 - scrollVal, i*60 - effectScrollVal );
		m_filtSlopeBox[i]->move( 90 + 1000 - scrollVal, i*60 - effectScrollVal );
		m_filtCutoffKnob[i]->move( 130 + 1000 - scrollVal, i*60 - effectScrollVal );
		m_filtResoKnob[i]->move( 160 + 1000 - scrollVal, i*60 - effectScrollVal );
		m_filtGainKnob[i]->move( 190 + 1000 - scrollVal, i*60 - effectScrollVal );
		m_filtSatuKnob[i]->move( 20 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtWetDryKnob[i]->move( 50 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtBalKnob[i]->move( 80 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtOutVolKnob[i]->move( 110 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtEnabledToggle[i]->move( 140 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtFeedbackKnob[i]->move( 160 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtDetuneKnob[i]->move( 190 + 1000 - scrollVal, i*60+30 - effectScrollVal );
		m_filtKeytrackingToggle[i]->move( 220 + 1000 - scrollVal, i*60+30 - effectScrollVal );

		m_sampleEnabledToggle[i]->move( 86 + 500 - scrollVal, 229 );
		m_sampleGraphEnabledToggle[i]->move( 138 + 500 - scrollVal, 229 );
		m_sampleMutedToggle[i]->move( 103 + 500 - scrollVal, 229 );
		m_sampleKeytrackingToggle[i]->move( 121 + 500 - scrollVal, 229 );
		m_sampleLoopToggle[i]->move( 155 + 500 - scrollVal, 229 );

		m_sampleVolumeKnob[i]->move( 23 + 500 - scrollVal, 172 );
		m_samplePanningKnob[i]->move( 55 + 500 - scrollVal, 172 );
		m_sampleDetuneKnob[i]->move( 93 + 500 - scrollVal, 172 );
		m_samplePhaseKnob[i]->move( 180 + 500 - scrollVal, 172 );
		m_samplePhaseRandKnob[i]->move( 206 + 500 - scrollVal, 172 );
		m_sampleStartKnob[i]->move( 121 + 500 - scrollVal, 172 );
		m_sampleEndKnob[i]->move( 145 + 500 - scrollVal, 172 );
	}

	for( int i = 0; i < 64; ++i )
	{
		m_subEnabledToggle[i]->move( 108 + 250 - scrollVal, 214 );
		m_subVolKnob[i]->move( 23 + 250 - scrollVal, 172 );
		m_subPhaseKnob[i]->move( 97 + 250 - scrollVal, 172 );
		m_subPhaseRandKnob[i]->move( 129 + 250 - scrollVal, 172 );
		m_subDetuneKnob[i]->move( 60 + 250 - scrollVal, 0 );
		m_subMutedToggle[i]->move( 147 + 250 - scrollVal, 214 );
		m_subKeytrackToggle[i]->move( 108 + 250 - scrollVal, 229 );
		m_subSampLenKnob[i]->move( 187 + 250 - scrollVal, 172 );
		m_subNoiseToggle[i]->move( 147 + 250 - scrollVal, 229 );
		m_subPanningKnob[i]->move( 55 + 250 - scrollVal, 172 );

		m_modInBox[i]->move( 40 + 750 - scrollVal, i*120 - modScrollVal );
		m_modInNumBox[i]->move( 80 + 750 - scrollVal, i*120 - modScrollVal );
		m_modInOtherNumBox[i]->move( 120 + 750 - scrollVal, i*120 - modScrollVal );
		m_modInAmntKnob[i]->move( 160 + 750 - scrollVal, i*120 - modScrollVal );
		m_modInCurveKnob[i]->move( 200 + 750 - scrollVal, i*120 - modScrollVal );
		m_modOutSecBox[i]->move( 40 + 750 - scrollVal, i*120+30 - modScrollVal );
		m_modOutSigBox[i]->move( 80 + 750 - scrollVal, i*120+30 - modScrollVal );
		m_modOutSecNumBox[i]->move( 120 + 750 - scrollVal, i*120+30 - modScrollVal );
		m_modInBox2[i]->move( 40 + 750 - scrollVal, i*120+60 - modScrollVal );
		m_modInNumBox2[i]->move( 80 + 750 - scrollVal, i*120+60 - modScrollVal );
		m_modInOtherNumBox2[i]->move( 120 + 750 - scrollVal, i*120+60 - modScrollVal );
		m_modInAmntKnob2[i]->move( 160 + 750 - scrollVal, i*120+60 - modScrollVal );
		m_modInCurveKnob2[i]->move( 200 + 750 - scrollVal, i*120+60 - modScrollVal );
		m_modEnabledToggle[i]->move( 230 + 750 - scrollVal, i*120+60 - modScrollVal );
		m_modCombineTypeBox[i]->move( 160 + 750 - scrollVal, i*120+30 - modScrollVal );
	}
	
	
	m_visvolKnob->move( 0 - scrollVal, 150 );
	m_scrollKnob->move( 0, 220 );
	m_modScrollKnob->move( 750 - scrollVal, 237 );
	m_effectScrollKnob->move( 1000 - scrollVal, 237 );
	
	m_loadAlgKnob->move( 0 - scrollVal, 25 );
	m_loadChnlKnob->move( 0 - scrollVal, 0 );
	m_interpolationToggle->move( 1310 - scrollVal, 207 );
	m_normalizeToggle->move( 1310 - scrollVal, 221 );
	m_visualizeToggle->move( 213 - scrollVal, 26 );
	subNumBox->move( 250 + 18 - scrollVal, 219 );
	sampNumBox->move( 500 + 18 - scrollVal, 219 );
	mainNumBox->move( 18 - scrollVal, 219 );
	m_graph->move( scrollVal >= 500 ? 500 + 23 - scrollVal : 23 , 30 );
	tabChanged( castModel<Microwave>()->m_scroll.value() - 1 );
	updateGraphColor( castModel<Microwave>()->m_scroll.value() );
	m_openWavetableButton->move( 54 - scrollVal, 220 );
	m_openSampleButton->move( 54 + 500 - scrollVal, 220 );

	m_sinWaveBtn->move( 178 + 250 - scrollVal, 215 );
	m_triangleWaveBtn->move( 196 + 250 - scrollVal, 215 );
	m_sawWaveBtn->move( 214 + 250 - scrollVal, 215 );
	m_sqrWaveBtn->move( 178 + 250 - scrollVal, 230 );
	m_whiteNoiseWaveBtn->move( 196 + 250 - scrollVal, 230 );
	m_usrWaveBtn->move( 54 + 250 - scrollVal, 220 );
	m_smoothBtn->move( 214 + 250 - scrollVal, 230 );

	m_sinWave2Btn->move( 178 + 500 - scrollVal, 215 );
	m_triangleWave2Btn->move( 196 + 500 - scrollVal, 215 );
	m_sawWave2Btn->move( 214 + 500 - scrollVal, 215 );
	m_sqrWave2Btn->move( 178 + 500 - scrollVal, 230 );
	m_whiteNoiseWave2Btn->move( 196 + 500 - scrollVal, 230 );
	m_usrWave2Btn->move( 54 + 500 - scrollVal, 220 );
	m_smooth2Btn->move( 214 + 500 - scrollVal, 230 );

	m_oversampleBox->move( 78 + 750 - scrollVal, 219 );

	QRect rect(scrollVal, 0, 250, 250);
	pal.setBrush( backgroundRole(), fullArtworkImg.copy(rect) );
	setPalette( pal );

}


// Snaps scroll to nearest tab when the scroll knob is released.
void MicrowaveView::scrollReleased()
{
	const float scrollVal = castModel<Microwave>()->m_scroll.value();
	castModel<Microwave>()->m_scroll.setValue( round(scrollVal-0.25) );
}


void MicrowaveView::mouseMoveEvent( QMouseEvent * _me )
{
	//castModel<Microwave>()->m_morph[0]->setValue(_me->x());
}


void MicrowaveView::wheelEvent( QWheelEvent * _me )
{
	if( _me->delta() < 0 )
	{
		const float scrollVal = castModel<Microwave>()->m_scroll.value();
		castModel<Microwave>()->m_scroll.setValue( scrollVal + 1 );
	}
	else if( _me->delta() > 0 )
	{
		const float scrollVal = castModel<Microwave>()->m_scroll.value();
		castModel<Microwave>()->m_scroll.setValue( scrollVal - 1 );
	}
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::mainNumChanged()
{
	for( int i = 0; i < 8; ++i )
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
		m_enabledToggle[i]->hide();
		m_mutedToggle[i]->hide();
		m_modifyModeBox[i]->hide();
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
			m_enabledToggle[i]->show();
			m_mutedToggle[i]->show();
			m_modifyModeBox[i]->show();
		}
	}
}


// Trades out the GUI elements when switching between oscillators, and adjusts graph length when needed
void MicrowaveView::subNumChanged()
{
	castModel<Microwave>()->m_graph.setLength( castModel<Microwave>()->m_subSampLen[castModel<Microwave>()->m_subNum.value()-1]->value() );
	for( int i = 0; i < 1024; ++i )
	{
		castModel<Microwave>()->m_graph.setSampleAt( i, castModel<Microwave>()->subs[(castModel<Microwave>()->m_subNum.value()-1)*1024+i] );
	}
	for( int i = 0; i < 64; ++i )
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
			m_subPanningKnob[i]->hide();
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
			m_subPanningKnob[i]->show();
		}
	}
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::sampNumChanged()
{
	for( int i = 0; i < 128; ++i )
	{
		castModel<Microwave>()->m_graph.setSampleAt( i, castModel<Microwave>()->sampGraphs[(castModel<Microwave>()->m_sampNum.value()-1)*128+i] );
	}
	for( int i = 0; i < 8; ++i )
	{
		if( i != castModel<Microwave>()->m_sampNum.value()-1 )
		{
			m_sampleEnabledToggle[i]->hide();
			m_sampleGraphEnabledToggle[i]->hide();
			m_sampleMutedToggle[i]->hide();
			m_sampleKeytrackingToggle[i]->hide();
			m_sampleLoopToggle[i]->hide();

			m_sampleVolumeKnob[i]->hide();
			m_samplePanningKnob[i]->hide();
			m_sampleDetuneKnob[i]->hide();
			m_samplePhaseKnob[i]->hide();
			m_samplePhaseRandKnob[i]->hide();
			m_sampleStartKnob[i]->hide();
			m_sampleEndKnob[i]->hide();
		}
		else
		{
			m_sampleEnabledToggle[i]->show();
			m_sampleGraphEnabledToggle[i]->show();
			m_sampleMutedToggle[i]->show();
			m_sampleKeytrackingToggle[i]->show();
			m_sampleLoopToggle[i]->show();

			m_sampleVolumeKnob[i]->show();
			m_samplePanningKnob[i]->show();
			m_sampleDetuneKnob[i]->show();
			m_samplePhaseKnob[i]->show();
			m_samplePhaseRandKnob[i]->show();
			m_sampleStartKnob[i]->show();
			m_sampleEndKnob[i]->show();
		}
	}
}


// Moves/changes the GUI around depending on the mod out section value
void MicrowaveView::modOutSecChanged( int i )
{
	switch( castModel<Microwave>()->m_modOutSec[i]->value() )
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
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
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
		case 5:// Matrix Parameters
			m_modOutSigBox[i]->show();
			m_modOutSecNumBox[i]->show();
			castModel<Microwave>()->m_modOutSig[i]->clear();
			matrixsignalsmodel( castModel<Microwave>()->m_modOutSig[i] )
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
			break;
		case 6:// Sample OSC
			m_modOutSigBox[i]->show();
			m_modOutSecNumBox[i]->show();
			castModel<Microwave>()->m_modOutSig[i]->clear();
			samplesignalsmodel( castModel<Microwave>()->m_modOutSig[i] )
			castModel<Microwave>()->m_modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		default:
			break;
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modInChanged( int i )
{
	switch( castModel<Microwave>()->m_modIn[i]->value() )
	{
		case 0:
			m_modInNumBox[i]->hide();
			m_modInOtherNumBox[i]->hide();
			break;
		case 1:// Main OSC
			m_modInNumBox[i]->show();
			m_modInOtherNumBox[i]->hide();
			castModel<Microwave>()->m_modInNum[i]->setRange( 1, 8, 1 );
			break;
		case 2:// Sub OSC
			m_modInNumBox[i]->show();
			m_modInOtherNumBox[i]->hide();
			castModel<Microwave>()->m_modInNum[i]->setRange( 1, 64, 1 );
			break;
		case 3:// Filter
			m_modInNumBox[i]->show();
			m_modInOtherNumBox[i]->show();
			castModel<Microwave>()->m_modInNum[i]->setRange( 1, 8, 1 );
			castModel<Microwave>()->m_modInOtherNum[i]->setRange( 1, 8, 1 );
			break;
		case 4:// Sample OSC
			m_modInNumBox[i]->show();
			m_modInOtherNumBox[i]->hide();
			castModel<Microwave>()->m_modInNum[i]->setRange( 1, 8, 1 );
			break;
	}
}


// Does what is necessary when the user scrolls to a new tab
void MicrowaveView::tabChanged( int tabnum )
{
	QPalette pal = QPalette();
	if( castModel<Microwave>()->currentTab != tabnum )
	{
		castModel<Microwave>()->currentTab = tabnum;
		switch( tabnum )
		{
			case 0:
				castModel<Microwave>()->m_graph.setLength( 2048 );
				mainNumChanged();
				break;
			case 1:
				subNumChanged();
				break;
			case 2:
				castModel<Microwave>()->m_graph.setLength( 128 );
				sampNumChanged();
				break;
		}
	}
}


// Changes graph style depending on interpolation value, will probably be removed
void MicrowaveView::interpolationToggled( bool value )
{
	m_graph->setGraphStyle( value ? Graph::LinearStyle : Graph::NearestStyle);
	Engine::getSong()->setModified();
}


// I hope I delete this.  If I don't, annoy Douglas.
void MicrowaveView::normalizeToggled( bool value )
{
	Engine::getSong()->setModified();
}


// This doesn't do anything right now.  I should probably delete it until I need it.
void MicrowaveView::visualizeToggled( bool value )
{
}


// Buttons that change the graph
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
// Buttons that change the graph


// Change graph image depending on scroll value.  I'll probably delete this, now that there's a new updated GUI.
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


// Calls MicrowaveView::openWavetableFile when the wavetable opening button is clicked.
void MicrowaveView::openWavetableFileBtnClicked( )
{
	openWavetableFile( castModel<Microwave>()->m_loadAlg.value(), castModel<Microwave>()->m_loadChnl.value() );
}


// All of the code and algorithms for loading wavetables from samples.  Please don't expect this code to look neat.
void MicrowaveView::openWavetableFile( int algorithm, int channel, QString fileName )
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	SampleBuffer * sampleBuffer = new SampleBuffer;
	if( fileName.isEmpty() )
	{
		fileName = sampleBuffer->openAndSetWaveformFile();
	}
	else
	{
		sampleBuffer->setAudioFile( fileName );
	}
	int filelength = sampleBuffer->sampleLength();
	int oscilNum = castModel<Microwave>()->m_mainNum.value() - 1;
	if( !fileName.isEmpty() )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		switch( algorithm )
		{
			case 0:// Squeeze entire sample into 256 waveforms
			{
				castModel<Microwave>()->m_morphMax[oscilNum]->setValue( 524288.f/castModel<Microwave>()->m_sampLen[oscilNum]->value() );
				castModel<Microwave>()->morphMaxChanged();
				for( int i = 0; i < 524288; ++i )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( qMin( i/524288.f, 1.f ), channel );
				}
				break;
			}
			case 1:// Load sample without changes, cut off end if too long
			{
				for( int i = 0; i < 524288; ++i )
				{
					if ( i <= lengthOfSample * 2.f )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( (i/lengthOfSample)/2.f, channel );
					}
					else
					{
						castModel<Microwave>()->m_morphMax[oscilNum]->setValue( i/castModel<Microwave>()->m_sampLen[oscilNum]->value() );
						castModel<Microwave>()->morphMaxChanged();
						for( int j = i; j < 524288; ++j ) { castModel<Microwave>()->waveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 2:// Attempt to guess at the frequency of the sound and adjust accordingly
			{
				float avgPosSampleVol = 0;
				float avgNegSampleVol = 0;
				float posSampleVols = 1;
				float negSampleVols = 1;
				sample_t sampleHere = 0;
				int cycleLengths[256] = {0};
				int cycleLength = 0;
				int cyclePart = 0;
				int cycleNum = 0;
				for( int i = 0; i < lengthOfSample; ++i )
				{
					sampleHere = sampleBuffer->userWaveSample(i/lengthOfSample, channel);
					if( sampleHere < 0 )
					{
						avgNegSampleVol = (sampleHere/negSampleVols) + (avgNegSampleVol*((negSampleVols-1)/negSampleVols));
						++negSampleVols;
					}
					else if( sampleHere > 0 )
					{
						avgPosSampleVol = (sampleHere/posSampleVols) + (avgPosSampleVol*((posSampleVols-1)/posSampleVols));
						++posSampleVols;
					}
					
				}
				for( int i = 0; i < lengthOfSample; ++i )
				{
					sampleHere = sampleBuffer->userWaveSample(i/lengthOfSample, channel);
					++cycleLength;
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
								++cycleNum;
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
				for( int i = 0; i < 524288; ++i )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( ( ( realCycleLen / 2048.f ) * i ) / ( ( cycleNum * realCycleLen ) ), channel );
				}
				break;
			}
			case 3:// The same as case 1??  I don't even know, check later...
			{
				for( int i = 0; i < 524288; ++i )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( qBound( 0.f, i/(lengthOfSample), 1.f ), channel );
				}
				break;
			}
			case 4:// Case 1 but halved/doubled/whatever
			{
				for( int i = 0; i < 524288; ++i )
				{
					if ( i <= lengthOfSample / 2.f )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( i/lengthOfSample, channel );
					}
					else
					{
						castModel<Microwave>()->m_morphMax[oscilNum]->setValue( i/castModel<Microwave>()->m_sampLen[oscilNum]->value() );
						castModel<Microwave>()->morphMaxChanged();
						for( int j = i; j < 524288; ++j ) { castModel<Microwave>()->waveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 5:// Looks for patterns between waveforms so it can make the wavetable with waveforms that match the most (broken)
			{
				float avgDif[2000] = {0};
				for( int j = 10; j < ( ( lengthOfSample > 4000 ) ? 2000 : ( lengthOfSample / 2 ) ); j+=100 )
				{
					for( int i = j; i < lengthOfSample; i+=j )
					{
						avgDif[j-10] = ( avgDif[j-10] * ( ((i/j)-1)/(i/j) ) ) + abs( ( sampleBuffer->userWaveSample( i / lengthOfSample ) - sampleBuffer->userWaveSample( ( i - j ) / lengthOfSample, channel ) ) * ( 1/(i/j) ) );
					}
				}
				int smolDif = 9999;
				for( int i = 10; i < 2000; ++i )
				{
					if( avgDif[i-10] < smolDif && avgDif[i-10] != 0 )
					{
						smolDif = avgDif[i-10];
					}
				}
				for( int i = 0; i < 524288; ++i )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( ( i * ( smolDif/2048.f ) )/lengthOfSample, channel );
				}
				break;
			}
			case 6:// Tries to match up with zero crossing
			{
				for( int i = 0; i < 256; ++i )
				{
					float highestVolume = 0;
					for( int j = 0; j < 2048; ++j )
					{
						highestVolume = abs(castModel<Microwave>()->waveforms[oscilNum][(i*2048)+j]) > highestVolume ? abs(castModel<Microwave>()->waveforms[oscilNum][(i*2048)+j]) : highestVolume;
					}
					if( highestVolume )
					{
						float multiplierThing = 1.f / highestVolume;
						for( int j = 0; j < 2048; ++j )
						{
							castModel<Microwave>()->waveforms[oscilNum][(i*2048)+j] *= multiplierThing;
						}
					}
				}
			}
			case 7:// Delete this.  Makes end of waveform match with beginning.
			{
				for( int i = 0; i < 524288; ++i )
				{
					if( fmod( i, castModel<Microwave>()->m_sampLen[oscilNum]->value() ) >= castModel<Microwave>()->m_sampLen[oscilNum]->value() - 200 )
					{
						float thing = ( -fmod(i, castModel<Microwave>()->m_sampLen[oscilNum]->value() ) + castModel<Microwave>()->m_sampLen[oscilNum]->value() ) / 200.f;
						castModel<Microwave>()->waveforms[oscilNum][i] = (castModel<Microwave>()->waveforms[oscilNum][i] * thing) + ((-thing+1) * castModel<Microwave>()->waveforms[oscilNum][int(i-(fmod( i, castModel<Microwave>()->m_sampLen[oscilNum]->value() )))]);
					}
				}
				break;
			}
			/*case 6:// Delete this.  Interpolates edges of waveforms.
			{
				for( int i = 0; i < 524288; ++i )
				{
					if( fmod( i, castModel<Microwave>()->m_sampLen[oscilNum]->value() ) <= 50 )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] *= fmod( i, castModel<Microwave>()->m_sampLen[oscilNum]->value() ) / 50.f;
					}
					if( fmod( i, castModel<Microwave>()->m_sampLen[oscilNum]->value() ) >= castModel<Microwave>()->m_sampLen[oscilNum]->value() - 50 )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] *= ( -fmod(i, castModel<Microwave>()->m_sampLen[oscilNum]->value() ) + castModel<Microwave>()->m_sampLen[oscilNum]->value() ) / 50.f;
					}
				}
				break;
			}*/
			/*case 6:// Looks for zero crossings and attempts to match the wavetable up with that
			{
				std::vector<int> zCross;
				float currentSample = 0;
				float prevSample = 0;
				zCross.push_back( 0 );
				for( int i = 1; i < lengthOfSample; ++i )
				{
					currentSample = sampleBuffer->userWaveSample( i / lengthOfSample, channel );
					if( currentSample >= 0 && prevSample < 0 )// If 0 was crossed upward
					{
						zCross.push_back( i );
					}
					else if( currentSample <= 0 && prevSample > 0 )// If 0 was crossed downward
					{
						zCross.push_back( i );
					}
				}
				
				std::vector<int> zCrossFiltered;
				zCrossFiltered.push_back( 0 );
				for( int i = 1; i < zCross.size(); ++i )
				{
					if( zCross[i] - zCross[i-1] >= 22 )// 22 is just an approximation of the shortest sample difference that might matter
					{
						zCrossFiltered.push_back( i );
					}
				}
				
				std::vector<int> zCrossFilteredDeriv;
				for( int i = 1; i < zCrossFiltered.size(); ++i )
				{
					zCrossFilteredDeriv.push_back( zCrossFiltered[i] - zCrossFiltered[i-1] );
				}
				std::sort( zCrossFilteredDeriv, zCrossFilteredDeriv + zCrossFilteredDeriv.size() );
				
				int gap = zCrossFilteredDeriv[int(zCrossFilteredDeriv.size()*0.75f)];
				int lastAccept = 0;
				std::vector<int> zCrossTargets;
				zCrossTargets.push_back( 0 );
				for( int i = 1; i < zCross.size(); ++i )
				{
					if( zCross[i] >= lastAccept + int( gap * 0.75f ) )
					{
						zCrossTargets.push_back( zCross[i] );
					}
				}
				zCrossTargets.push_back( 524288 );
				
				int lastZCross = 0;
				int nextZCross = zCrossTargets[2];
				int nextZCrossIndex = 2;
				for( int i = 0; i < 524288; ++i )
				{
					castModel<Microwave>()->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( i/(nextZCross-lastZCross), channel );
				}
				break;
			}*/
		}
		
		
		sampleBuffer->dataUnlock();
	}
	else //Delete this
	{
		for( int i = 0; i < 256; ++i )
		{
			float highestVolume = 0;
			for( int j = 0; j < 2048; ++j )
			{
				highestVolume = abs(castModel<Microwave>()->waveforms[oscilNum][(i*2048)+j]) > highestVolume ? abs(castModel<Microwave>()->waveforms[oscilNum][(i*2048)+j]) : highestVolume;
			}
			if( highestVolume )
			{
				float multiplierThing = 1.f / highestVolume;
				for( int j = 0; j < 2048; ++j )
				{
					castModel<Microwave>()->waveforms[oscilNum][(i*2048)+j] *= multiplierThing;
				}
			}
		}
	}

	sharedObject::unref( sampleBuffer );
}



// Calls MicrowaveView::openSampleFile when the sample opening button is clicked.
void MicrowaveView::openSampleFileBtnClicked( )
{
	openSampleFile();
}


// Loads sample for sample oscillator
void MicrowaveView::openSampleFile()
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();
	int oscilNum = castModel<Microwave>()->m_sampNum.value() - 1;

	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	int filelength = sampleBuffer->sampleLength();
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		castModel<Microwave>()->samples[oscilNum][0].clear();
		castModel<Microwave>()->samples[oscilNum][1].clear();
		
		for( int i = 0; i < lengthOfSample; ++i )
		{
			castModel<Microwave>()->samples[oscilNum][0].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 0));
			castModel<Microwave>()->samples[oscilNum][1].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 1));
		}
		sampleBuffer->dataUnlock();
	}
	sharedObject::unref( sampleBuffer );
}



void MicrowaveView::dropEvent( QDropEvent * _de )
{
	int oscilNum = castModel<Microwave>()->m_mainNum.value() - 1;
	int tabNum = castModel<Microwave>()->currentTab;
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		if( tabNum != 100000 )
		{
			openWavetableFile( castModel<Microwave>()->m_loadAlg.value(), castModel<Microwave>()->m_loadChnl.value(), value );
			_de->accept();
			return;
		}
	}
	_de->ignore();
}


void MicrowaveView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( StringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						StringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "samplefile" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}





/*                                                                
                                                                         
          ____    .--.--.                             ___      ,---,     
        ,'  , `. /  /    '.                         ,--.'|_  ,--.' |     
     ,-+-,.' _ ||  :  /`. /                 ,---,   |  | :,' |  |  :     
  ,-+-. ;   , ||;  |  |--`              ,-+-. /  |  :  : ' : :  :  :     
 ,--.'|'   |  |||  :  ;_         .--,  ,--.'|'   |.;__,'  /  :  |  |,--. 
|   |  ,', |  |, \  \    `.    /_ ./| |   |  ,"' ||  |   |   |  :  '   | 
|   | /  | |--'   `----.   \, ' , ' : |   | /  | |:__,'| :   |  |   /' : 
|   : |  | ,      __ \  \  /___/ \: | |   | |  | |  '  : |__ '  :  | | | 
|   : |  |/      /  /`--'  /.  \  ' | |   | |  |/   |  | '.'||  |  ' | : 
|   | |`-'      '--'.     /  \  ;   : |   | |--'    ;  :    ;|  :  :_:,' 
|   ;/            `--'---'    \  \  ; |   |/        |  ,   / |  | ,'     
'---'                          :  \  \'---'          ---`-'  `--''       
                                \  ' ;                                   
                                 `--`                                    
*/




// Initializes mSynth (when a new note is played).  Clone all of the arrays storing the knob values so they can be changed by modulation.
mSynth::mSynth( NotePlayHandle * _nph, float _factor, const sample_rate_t _sample_rate, float * phaseRand, int * _modifyModeArr, float * _modifyArr, float * _morphArr, float * _rangeArr, float * _sprayArr, float * _unisonVoicesArr, float * _unisonDetuneArr, float * _unisonMorphArr, float * _unisonModifyArr, float * _morphMaxArr, float * _detuneArr, float waveforms[8][524288], float * _subsArr, bool * _subEnabledArr, float * _subVolArr, float * _subPhaseArr, float * _subPhaseRandArr, float * _subDetuneArr, bool * _subMutedArr, bool * _subKeytrackArr, float * _subSampLenArr, bool * _subNoiseArr, int * _sampLenArr, int * _m_modInArr, int * _m_modInNumArr, int * _m_modInOtherNumArr, float * _m_modInAmntArr, float * _m_modInCurveArr, int * _m_modIn2Arr, int * _m_modInNum2Arr, int * _m_modInOtherNum2Arr, float * _m_modInAmnt2Arr, float * _m_modInCurve2Arr, int * _m_modOutSecArr, int * _m_modOutSigArr, int * _m_modOutSecNumArr, int * m_modCombineTypeArr, float * _phaseArr, float * _volArr, float * _filtInVolArr, int * _filtTypeArr, int * _filtSlopeArr, float * _filtCutoffArr, float * _filtResoArr, float * _filtGainArr, float * _filtSatuArr, float * _filtWetDryArr, float * _filtBalArr, float * _filtOutVolArr, bool * _filtEnabledArr, bool * _enabledArr, bool * _modEnabledArr, float * sampGraphs, bool * _mutedArr, bool * _sampleEnabledArr, bool * _sampleGraphEnabledArr, bool * _sampleMutedArr, bool * _sampleKeytrackingArr, bool * _sampleLoopArr, float * _sampleVolumeArr, float * _samplePanningArr, float * _sampleDetuneArr, float * _samplePhaseArr, float * _samplePhaseRandArr, std::vector<float> (&samples)[8][2], float * _filtFeedbackArr, float * _filtDetuneArr, bool * _filtKeytrackingArr, float * _subPanningArr, float * _sampleStartArr, float * _sampleEndArr ) :
	nph( _nph ),
	sample_rate( _sample_rate )
{

	memcpy( modifyModeVal, _modifyModeArr, sizeof(int) * 8 );
	memcpy( _modifyVal, _modifyArr, sizeof(float) * 8 );
	memcpy( _morphVal, _morphArr, sizeof(float) * 8 );
	memcpy( rangeVal, _rangeArr, sizeof(float) * 8 );
	memcpy( sprayVal, _sprayArr, sizeof(float) * 8 );
	memcpy( unisonVoices, _unisonVoicesArr, sizeof(float) * 8 );
	memcpy( unisonDetune, _unisonDetuneArr, sizeof(float) * 8 );
	memcpy( unisonMorph, _unisonMorphArr, sizeof(float) * 8 );
	memcpy( unisonModify, _unisonModifyArr, sizeof(float) * 8 );
	memcpy( morphMaxVal, _morphMaxArr, sizeof(float) * 8 );
	memcpy( detuneVal, _detuneArr, sizeof(float) * 8 );
	memcpy( sampLen, _sampLenArr, sizeof(int) * 8 );
	memcpy( m_modIn, _m_modInArr, sizeof(int) * 64 );
	memcpy( m_modInNum, _m_modInNumArr, sizeof(int) * 64 );
	memcpy( m_modInOtherNum, _m_modInOtherNumArr, sizeof(int) * 64 );
	memcpy( m_modInAmnt, _m_modInAmntArr, sizeof(float) * 64 );
	memcpy( m_modInCurve, _m_modInCurveArr, sizeof(float) * 64 );
	memcpy( m_modIn2, _m_modIn2Arr, sizeof(int) * 64 );
	memcpy( m_modInNum2, _m_modInNum2Arr, sizeof(int) * 64 );
	memcpy( m_modInOtherNum2, _m_modInOtherNum2Arr, sizeof(int) * 64 );
	memcpy( m_modInAmnt2, _m_modInAmnt2Arr, sizeof(float) * 64 );
	memcpy( m_modInCurve2, _m_modInCurve2Arr, sizeof(float) * 64 );
	memcpy( m_modOutSec, _m_modOutSecArr, sizeof(int) * 64 );
	memcpy( m_modOutSig, _m_modOutSigArr, sizeof(int) * 64 );
	memcpy( m_modOutSecNum, _m_modOutSecNumArr, sizeof(int) * 64 );
	memcpy( modEnabled, _modEnabledArr, sizeof(bool) * 64 );
	memcpy( m_modCombineType, m_modCombineTypeArr, sizeof(int) * 64 );
	memcpy( phase, _phaseArr, sizeof(int) * 8 );
	memcpy( vol, _volArr, sizeof(int) * 8 );
	memcpy( enabled, _enabledArr, sizeof(bool) * 8 );
	memcpy( muted, _mutedArr, sizeof(bool) * 64 );
	memcpy( subEnabled, _subEnabledArr, sizeof(bool) * 64 );
	memcpy( subVol, _subVolArr, sizeof(float) * 64 );
	memcpy( subPhase, _subPhaseArr, sizeof(float) * 64 );
	memcpy( subPhaseRand, _subPhaseRandArr, sizeof(float) * 64 );
	memcpy( subDetune, _subDetuneArr, sizeof(float) * 64 );
	memcpy( subMuted, _subMutedArr, sizeof(bool) * 64 );
	memcpy( subKeytrack, _subKeytrackArr, sizeof(bool) * 64 );
	memcpy( subSampLen, _subSampLenArr, sizeof(float) * 64 );
	memcpy( subNoise, _subNoiseArr, sizeof(bool) * 64 );
	memcpy( filtInVol, _filtInVolArr, sizeof(float) * 8 );
	memcpy( filtType, _filtTypeArr, sizeof(int) * 8 );
	memcpy( filtSlope, _filtSlopeArr, sizeof(int) * 8 );
	memcpy( filtCutoff, _filtCutoffArr, sizeof(float) * 8 );
	memcpy( filtReso, _filtResoArr, sizeof(float) * 8 );
	memcpy( filtGain, _filtGainArr, sizeof(float) * 8 );
	memcpy( filtSatu, _filtSatuArr, sizeof(float) * 8 );
	memcpy( filtWetDry, _filtWetDryArr, sizeof(float) * 8 );
	memcpy( filtBal, _filtBalArr, sizeof(float) * 8 );
	memcpy( filtOutVol, _filtOutVolArr, sizeof(float) * 8 );
	memcpy( filtEnabled, _filtEnabledArr, sizeof(bool) * 8 );
	memcpy( sampleEnabled, _sampleEnabledArr, sizeof(bool) * 8 );
	memcpy( sampleGraphEnabled, _sampleGraphEnabledArr, sizeof(bool) * 8 );
	memcpy( sampleMuted, _sampleMutedArr, sizeof(bool) * 8 );
	memcpy( sampleKeytracking, _sampleKeytrackingArr, sizeof(bool) * 8 );
	memcpy( sampleLoop, _sampleLoopArr, sizeof(bool) * 8 );
	memcpy( sampleVolume, _sampleVolumeArr, sizeof(float) * 8 );
	memcpy( samplePanning, _samplePanningArr, sizeof(float) * 8 );
	memcpy( sampleDetune, _sampleDetuneArr, sizeof(float) * 8 );
	memcpy( samplePhase, _samplePhaseArr, sizeof(float) * 8 );
	memcpy( samplePhaseRand, _samplePhaseRandArr, sizeof(float) * 8 );
	memcpy( filtFeedback, _filtFeedbackArr, sizeof(float) * 8 );
	memcpy( filtDetune, _filtDetuneArr, sizeof(float) * 8 );
	memcpy( filtKeytracking, _filtKeytrackingArr, sizeof(bool) * 8 );
	memcpy( subPanning, _subPanningArr, sizeof(float) * 64 );
	memcpy( sampleStart, _sampleStartArr, sizeof(float) * 8 );
	memcpy( sampleEnd, _sampleEndArr, sizeof(float) * 8 );

	memcpy( sampLen, _sampLenArr, sizeof(int) * 8 );

	for( int i=0; i < 8; ++i )
	{
		for( int j=0; j < 32; ++j )
		{
			// Randomize the phases of all of the waveforms
			sample_realindex[i][j] = int( ( ( fastRandf( sampLen[i] ) - ( sampLen[i] / 2.f ) ) * ( phaseRand[i] / 100.f ) ) + ( sampLen[i] / 2.f ) ) % ( sampLen[i] );
		}
	}

	for( int i=0; i < 64; ++i )
	{
		sample_subindex[i] = 0;
	}

	for( int i=0; i < 8; ++i )
	{
		sample_sampleindex[i] = fmod( fastRandf( samples[i][0].size() ) * ( samplePhaseRand[i] / 100.f ), ( samples[i][0].size() *sampleEnd[i] ) - ( samples[i][0].size() * sampleStart[i] ) ) + ( samples[i][0].size() * sampleStart[i] );
		humanizer[i] = ( rand() / float(RAND_MAX) ) * 2 - 1;// Generate humanizer values at the beginning of every note
	}

	noteDuration = -1;

	for( int i=0; i < 8; ++i )
	{
		for( int j=0; j < unisonVoices[i]; ++j )
		{
			unisonDetuneAmounts[i][j] = ((rand()/float(RAND_MAX))*2.f)-1;
		}
	}
	
}


mSynth::~mSynth()
{
}


// The heart of Microwave.  As you probably learned in anatomy class, hearts actually aren't too pretty.  This is no exception.
// This is the part that puts everything together and calculates an audio output.
std::vector<float> mSynth::nextStringSample( float waveforms[8][524288], float * subs, float * sampGraphs, std::vector<float> (&samples)[8][2], int maxModEnabled, int maxSubEnabled, int maxSampleEnabled, int sample_rate )// "waveforms" here may cause CPU problem, check back later
{
	//=====================//
	//== MAIN OSCILLATOR ==//
	//=====================//

	noteDuration = noteDuration + 1;
	float morphVal[8] = {0};
	float modifyVal[8] = {0};
	float curModVal[2] = {0};
	float curModVal2[2] = {0};
	float curModValCurve[2] = {0};
	float curModVal2Curve[2] = {0};
	float comboModVal[2] = {0};
	float comboModValMono = 0;

	float sample_morerealindex[8][32] = {{0}};
	sample_t sample[8][32] = {{0}};
	float sample_step[8][32] = {{0}};
	float resultsample[8][32] = {{0}};
	float sample_length_modify[8][32] = {{0}};
	
	for( int l = 0; l < maxModEnabled; ++l )// maxModEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( modEnabled[l] )
		{
			curModVal[0] = 0;
			curModVal[1] = 0;
			curModVal2[0] = 0;
			curModVal2[1] = 0;
			curModValCurve[0] = 0;
			curModValCurve[1] = 0;
			curModVal2Curve[0] = 0;
			curModVal2Curve[1] = 0;
			switch( m_modIn[l] )
			{
				case 0:
					break;
				case 1:
					curModVal[0] = lastMainOscVal[m_modInNum[l]-1];
					curModVal[1] = curModVal[0];
					break;
				case 2:
					curModVal[0] = lastSubVal[m_modInNum[l]-1][0];
					curModVal[1] = lastSubVal[m_modInNum[l]-1][1];
					break;
				case 3:
					curModVal[0] = filtOutputs[m_modInNum[l]-1][m_modInOtherNum[l]-1][0];
					curModVal[1] = filtOutputs[m_modInNum[l]-1][m_modInOtherNum[l]-1][1];
					break;
				case 4:
					curModVal[0] = lastSampleVal[m_modInNum[l]-1][0];
					curModVal[1] = lastSampleVal[m_modInNum[l]-1][1];
					break;
				case 5:// Velocity
					curModVal[0] = (nph->getVolume()/100.f)-1;
					curModVal[1] = curModVal[0];
					break;
				case 6:// Panning
					curModVal[0] = (nph->getPanning()/100.f);
					curModVal[1] = curModVal[0];
					break;
				case 7:// Humanizer
					curModVal[0] = humanizer[m_modInNum[l]];
					curModVal[1] = humanizer[m_modInNum[l]];
					break;
				default: {}
			}
			switch( m_modIn2[l] )
			{
				case 0:
					break;
				case 1:
					curModVal2[0] = lastMainOscVal[m_modInNum2[l]-1];
					curModVal2[1] = curModVal2[0];
					break;
				case 2:
					curModVal2[0] = lastSubVal[m_modInNum[l]-1][0];
					curModVal2[1] = lastSubVal[m_modInNum[l]-1][1];
					break;
				case 3:
					curModVal2[0] = filtOutputs[m_modInNum2[l]-1][m_modInOtherNum2[l]-1][0];
					curModVal2[1] = filtOutputs[m_modInNum2[l]-1][m_modInOtherNum2[l]-1][1];
				case 4:
					curModVal2[0] = lastSampleVal[m_modInNum2[l]-1][0];
					curModVal2[1] = lastSampleVal[m_modInNum2[l]-1][1];
					break;
				case 5:// Velocity
					curModVal2[0] = (nph->getVolume()/100.f)-1;
					curModVal2[1] = curModVal2[0];
					break;
				case 6:// Panning
					curModVal2[0] = (nph->getPanning()/100.f);
					curModVal2[1] = curModVal2[0];
					break;
				case 7:// Humanizer
					curModVal2[0] = humanizer[m_modInNum2[l]];
					curModVal2[1] = humanizer[m_modInNum2[l]];
					break;
				default: {}
			}
			curModVal[0] *= m_modInAmnt[l] / 100.f;
			curModVal[1] *= m_modInAmnt[l] / 100.f;
			curModVal2[0] *= m_modInAmnt2[l] / 100.f;
			curModVal2[1] *= m_modInAmnt2[l] / 100.f;

			// Calculate curve
			curModValCurve[0] = (curModVal[0] <= -1) ? curModVal[0] : pow( ( curModVal[0] + 1 ) / 2.f, 1.f / ( m_modInCurve[l] / 100.f ) );
			curModValCurve[1] = (curModVal[1] <= -1) ? curModVal[0] : pow( ( curModVal[1] + 1 ) / 2.f, 1.f / ( m_modInCurve[l] / 100.f ) );
			curModVal2Curve[0] = (curModVal2[0] <= -1) ? curModVal[0] : pow( ( curModVal2[0] + 1 ) / 2.f, 1.f / ( m_modInCurve2[l] / 100.f ) );
			curModVal2Curve[1] = (curModVal2[1] <= -1) ? curModVal[0] : pow( ( curModVal2[1] + 1 ) / 2.f, 1.f / ( m_modInCurve2[l] / 100.f ) );
			
			switch( m_modCombineType[l] )
			{
				case 0:// Add
					comboModVal[0] = curModValCurve[0] + curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] + curModVal2Curve[1];
					break;
				case 1:// Multiply
					comboModVal[0] = curModValCurve[0] * curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] * curModVal2Curve[1];
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					break;
				case 5:
					break;
				case 6:
					break;
			}
			//float curveModVal = pow( ( comboModVal + 1 ) / 2.f, 1.f / ( m_modInCurve[l] / 50.f ) );
			comboModValMono = ( comboModVal[0] + comboModVal[1] ) / 2.f;
			comboModValMono -= 1;// Bipolar modulation
			switch( m_modOutSec[l] )
			{
				case 0:
					break;
				case 1:// Main Oscillator
				{
					switch( m_modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Send input to Morph
							_morphVal[m_modOutSecNum[l]-1] = qBound( 0.f, _morphVal[m_modOutSecNum[l]-1] + ( round((comboModValMono)*morphMaxVal[m_modOutSecNum[l]-1]) ), morphMaxVal[m_modOutSecNum[l]-1] );
							modValType.push_back( 1 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 2:// Send input to Range
							rangeVal[m_modOutSecNum[l]-1] = qMax( 0.f, rangeVal[m_modOutSecNum[l]-1] + comboModValMono * 16.f );
							modValType.push_back( 2 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 3:// Send input to Modify
							_modifyVal[m_modOutSecNum[l]-1] = qMax( 0.f, _modifyVal[m_modOutSecNum[l]-1] + comboModValMono * 2048.f );
							modValType.push_back( 3 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 4:// Send input to Pitch/Detune
							detuneVal[m_modOutSecNum[l]-1] = detuneVal[m_modOutSecNum[l]-1] + comboModValMono * 9600.f;
							modValType.push_back( 11 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 5:// Send input to Phase
							phase[m_modOutSecNum[l]-1] = fmod( phase[m_modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 13 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 6:// Send input to Volume
							vol[m_modOutSecNum[l]-1] = qMax( 0.f, vol[m_modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 14 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						default:
						{
						}
					}
					break;
				}
				case 2:// Sub Oscillator
				{
					switch( m_modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Send input to Pitch/Detune
							subDetune[m_modOutSecNum[l]-1] = subDetune[m_modOutSecNum[l]-1] + comboModValMono * 9600.f;
							modValType.push_back( 30 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 2:// Send input to Phase
							subPhase[m_modOutSecNum[l]-1] = fmod( subPhase[m_modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 28 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 3:// Send input to Volume
							subVol[m_modOutSecNum[l]-1] = qMax( 0.f, subVol[m_modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 27 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						default:
						{
						}
					}
					break;
				}
				case 3:// Filter Input
				{
					curModVal[0] *= filtInVol[l] / 100.f;
					curModVal[1] *= filtInVol[l] / 100.f;
					
					filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][0].resize( round( sample_rate / detuneWithCents( nph->frequency(), filtDetune[m_modOutSig[l]] ) ) );
					filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][1].resize( round( sample_rate / detuneWithCents( nph->frequency(), filtDetune[m_modOutSig[l]] ) ) );
					curModVal[0] += filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][0].at(filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][0].size() - 1);
					curModVal[1] += filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][1].at(filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][1].size() - 1);
					
					for( int m = 0; m < filtSlope[l] + 1; ++m )// m is the slope number.  So if m = 2, then the sound is going from a 24 db to a 36 db slope, for example.
					{
						if( m )
						{
							curModVal[0] = filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0];
							curModVal[1] = filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1];
						}
						float cutoff = filtCutoff[m_modOutSig[l]];
						int mode = filtType[m_modOutSig[l]];
						float reso = filtReso[m_modOutSig[l]];
						float dbgain = filtGain[m_modOutSig[l]];
						float Fs = sample_rate;
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
						if( mode <= 7 )
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
									break;
								case 4:// Notch
									alpha = sin(w0)*sinh( (log(2)/log(2.7182818284590452353602874713527))/2 * reso * w0/sin(w0) );
									b0 =   1;
			    						b1 =  -2*cos(w0);
			    						b2 =   1;
			    						a0 =   1 + alpha;
			    						a1 =  -2*cos(w0);
			    						a2 =   1 - alpha;
									break;
								case 5:// Low Shelf
									alpha = sin(w0) / (2*reso);
									A = pow( 10, dbgain / 40 );
									b0 =    A*( (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha );
			    						b1 =  2*A*( (A-1) - (A+1)*cos(w0)                   );
			    						b2 =    A*( (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha );
			    						a0 =        (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha;
			    						a1 =   -2*( (A-1) + (A+1)*cos(w0)                   );
			    						a2 =        (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha;
									break;
								case 6:// High Shelf
									alpha = sin(w0) / (2*reso);
									A = pow( 10, dbgain / 40 );
									b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha );
			    						b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   );
			    						b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha );
			    						a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha;
			    						a1 =    2*( (A-1) - (A+1)*cos(w0)                   );
			    						a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha;
									break;
								case 7:// Allpass
									alpha = sin(w0) / (2*reso);
									b0 = 1 - alpha;
			    						b1 =-2*cos(w0);
			    						b2 = 1 + alpha;
			    						a0 = 1 + alpha;
			    						a1 =-2*cos(w0);
			    						a2 = 1 - alpha;
									break;
							}
							// Translation of this monstrosity: y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
							// See www.musicdsp.org/files/Audio-EQ-Cookbook.txt
							filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][0][0] = (b0/a0)*curModVal[0] + (b1/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][0] + (b2/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][0] - (a1/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][0] - (a2/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][0];// Left ear
							filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][0][1] = (b0/a0)*curModVal[1] + (b1/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][1] + (b2/a0)*filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][1] - (a1/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][1] - (a2/a0)*filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][1];// Right ear

							//Output results
							filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][0][0] * ( filtOutVol[l] / 100.f );
							filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][0][1] * ( filtOutVol[l] / 100.f );

						}
						else if( mode >= 8 )
						{
							switch( mode )
							{
								case 8:// Moog 24db Lowpass
									filtx[0] = curModVal[0]-r*filty4[0];
									filtx[1] = curModVal[1]-r*filty4[1];
									for( int i = 0; i < 2; ++i )
									{
										filty1[i]=filtx[i]*p + filtoldx[i]*p - k*filty1[i];
										filty2[i]=filty1[i]*p+filtoldy1[i]*p - k*filty2[i];
										filty3[i]=filty2[i]*p+filtoldy2[i]*p - k*filty3[i];
										filty4[i]=filty3[i]*p+filtoldy3[i]*p - k*filty4[i];
										filty4[i] = filty4[i] - pow(filty4[i], 3)/6;
										filtoldx[i] = filtx[i];
										filtoldy1[i] = filty1[i];
										filtoldy2[i] = filty2[i];
										filtoldy3[i] = filty3[i];
									}
									filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] = filty4[0] * ( filtOutVol[l] / 100.f );
									filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] = filty4[1] * ( filtOutVol[l] / 100.f );
									break;
							}

						}

						// Calculates Saturation.  It looks ugly, but the algorithm is pretty much y = x ^ ( 1 - saturation );
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] = pow( abs( filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] ), 1 - ( filtSatu[l] / 100.f ) ) * ( filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] < 0 ? -1 : 1 );
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] = pow( abs( filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] ), 1 - ( filtSatu[l] / 100.f ) ) * ( filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] < 0 ? -1 : 1 );

						// Balance knob wet
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] *= filtBal[m_modOutSig[l]] > 0 ? ( 100.f - filtBal[m_modOutSig[l]] ) / 100.f : 1.f;
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] *= filtBal[m_modOutSig[l]] < 0 ? ( 100.f + filtBal[m_modOutSig[l]] ) / 100.f : 1.f;

						// Wet
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] *= filtWetDry[m_modOutSig[l]] / 100.f;
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] *= filtWetDry[m_modOutSig[l]] / 100.f;

						// Balance knob dry
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] += ( 1.f - ( filtBal[m_modOutSig[l]] > 0 ? ( 100.f - filtBal[m_modOutSig[l]] ) / 100.f : 1.f ) ) * curModVal[0];
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] += ( 1.f - ( filtBal[m_modOutSig[l]] < 0 ? ( 100.f + filtBal[m_modOutSig[l]] ) / 100.f : 1.f ) ) * curModVal[1];

						// Dry
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] += ( 1.f - ( filtWetDry[m_modOutSig[l]] / 100.f ) ) * curModVal[0];
						filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] += ( 1.f - ( filtWetDry[m_modOutSig[l]] / 100.f ) ) * curModVal[1];

						// Send back past samples
						// I'm sorry you had to read this
						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][4][0] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][0];
						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][4][1] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][1];

						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][0] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][0];
						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][1] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][1];

						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][0] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][0];
						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][1] = filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][1];
					
						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][0] = curModVal[0];
						filtPrevSampIn[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][1] = curModVal[1];

						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][4][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][0];
						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][4][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][1];

						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][0];
						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][3][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][1];
					
						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][0];
						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][2][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][1];
					
						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][0] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][0][0];
						filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][1][1] = filtPrevSampOut[m_modOutSig[l]][m_modOutSecNum[l]-1][m][0][1];
					}

					filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][0].insert( filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][0].begin(), filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][0] * filtFeedback[m_modOutSig[l]] / 100.f );
					filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][1].insert( filtDelayBuf[m_modOutSig[l]][m_modOutSecNum[l]-1][1].begin(), filtOutputs[m_modOutSig[l]][m_modOutSecNum[l]-1][1] * filtFeedback[m_modOutSig[l]] / 100.f );

					break;
				}
				case 4:// Filter Parameters
				{
					switch( m_modOutSig[l] )
					{
						case 0:// None
							break;
						case 1:// Input Volume
							filtInVol[m_modOutSecNum[l]-1] = qMax( 0.f, filtInVol[m_modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 15 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 2:// Filter Type
							filtType[m_modOutSecNum[l]-1] = qMax( 0, int(filtType[m_modOutSecNum[l]-1] + comboModValMono*7.f ) );
							modValType.push_back( 16 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 3:// Filter Slope
							filtSlope[m_modOutSecNum[l]-1] = qMax( 0, int(filtSlope[m_modOutSecNum[l]-1] + comboModValMono*8.f ) );
							modValType.push_back( 17 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
							break;
						case 4:// Cutoff Frequency
							filtCutoff[m_modOutSecNum[l]-1] = qBound( 20.f, filtCutoff[m_modOutSecNum[l]-1] + comboModValMono*19980.f, 21999.f );
							modValType.push_back( 18 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 5:// Resonance
							filtReso[m_modOutSecNum[l]-1] = qMax( 0.0001f, filtReso[m_modOutSecNum[l]-1] + comboModValMono*16.f );
							modValType.push_back( 19 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 6:// dbGain
							filtGain[m_modOutSecNum[l]-1] = filtGain[m_modOutSecNum[l]-1] + comboModValMono*64.f;
							modValType.push_back( 20 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 7:// Saturation
							filtSatu[m_modOutSecNum[l]-1] = qMax( 0.f, filtSatu[m_modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 21 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 8:// Wet/Dry
							filtWetDry[m_modOutSecNum[l]-1] = qBound( 0.f, filtWetDry[m_modOutSecNum[l]-1] + comboModValMono*100.f, 100.f );
							modValType.push_back( 22 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 9:// Balance/Panning
							filtWetDry[m_modOutSecNum[l]-1] = qBound( -100.f, filtWetDry[m_modOutSecNum[l]-1] + comboModValMono*200.f, 100.f );
							modValType.push_back( 23 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 10:// Output Volume
							filtOutVol[m_modOutSecNum[l]-1] = qMax( 0.f, filtOutVol[m_modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 24 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
					}
					break;
				}
				case 5:// Mod Parameters
				{
					switch( m_modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Mod In Amount
							m_modInAmnt[m_modOutSecNum[l]-1] = m_modInAmnt[m_modOutSecNum[l]-1] + comboModValMono*100.f;
							modValType.push_back( 38 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 2:// Mod In Curve
							m_modInCurve[m_modOutSecNum[l]-1] = qMax( 0.f, m_modInCurve[m_modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 39 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
					}
					break;
				}
				case 6:// Sample Oscillator
				{
					switch( m_modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Send input to Pitch/Detune
							sampleDetune[m_modOutSecNum[l]-1] = sampleDetune[m_modOutSecNum[l]-1] + comboModValMono * 9600.f;
							modValType.push_back( 66 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 2:// Send input to Phase
							samplePhase[m_modOutSecNum[l]-1] = fmod( samplePhase[m_modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 67 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						case 3:// Send input to Volume
							sampleVolume[m_modOutSecNum[l]-1] = qMax( 0.f, sampleVolume[m_modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 64 );
							modValNum.push_back( m_modOutSecNum[l]-1 );
							break;
						default:
						{
						}
					}
					break;
				}
				default:
				{
				}
			}
		}
	}


	for( int i = 0; i < 8; ++i )
	{
		if( !enabled[i] )
		{
			continue;// Skip to next loop if oscillator is not enabled
		}
		for( int l = 0; l < unisonVoices[i]; ++l )
		{
			sample_morerealindex[i][l] = fmod( ( sample_realindex[i][l] + ( fmod( phase[i], 100.f ) * sampLen[i] / 100.f ) ), sampLen[i] );// Calculates phase
	
			noteFreq = unisonVoices[i] - 1 ? detuneWithCents( nph->frequency(), unisonDetuneAmounts[i][l]*unisonDetune[i]+detuneVal[i] ) : detuneWithCents( nph->frequency(), detuneVal[i] );// Calculates frequency depending on detune and unison detune
	
			sample_step[i][l] = static_cast<float>( sampLen[i] / ( sample_rate / noteFreq ) );
	
			if( unisonVoices[i] - 1 )// Figures out Morph and Modify values for individual unison voices
			{
				morphVal[i] = (((unisonVoices[i]-1)-l)/(unisonVoices[i]-1))*unisonMorph[i];
				morphVal[i] = qBound( 0.f, morphVal[i] - ( unisonMorph[i] / 2.f ) + _morphVal[i], morphMaxVal[i] );
	
				modifyVal[i] = (((unisonVoices[i]-1)-l)/(unisonVoices[i]-1))*unisonModify[i];
				modifyVal[i] = qBound( 0.f, modifyVal[i] - ( unisonModify[i] / 2.f ) + _modifyVal[i], sampLen[i]-1.f);// SampleLength - 1 = ModifyMax
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
					sample_morerealindex[i][l] /= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					break;
				case 1:// Stretch Left Half
					if( sample_morerealindex[i][l] < ( sampLen[i] + sample_length_modify[i][l] ) / 2.f )
					{
						sample_morerealindex[i][l] -= (sample_morerealindex[i][l] * (modifyVal[i]/( sampLen[i] )));
					}
					break;
				case 2:// Shrink Right Half To Left Edge
					if( sample_morerealindex[i][l] > ( sampLen[i] + sample_length_modify[i][l] ) / 2.f )
					{
						sample_morerealindex[i][l] += (((sample_morerealindex[i][l])+sampLen[i]/2.f) * (modifyVal[i]/( sampLen[i] )));
					}
					break;
				case 3:// Weird 1
					sample_morerealindex[i][l] = ( ( ( sin( ( ( ( sample_morerealindex[i][l] ) / ( sampLen[i] ) ) * ( modifyVal[i] / 50.f ) ) / 2 ) ) * ( sampLen[i] ) ) * ( modifyVal[i] / sampLen[i] ) + ( sample_morerealindex[i][l] + ( ( -modifyVal[i] + sampLen[i] ) / sampLen[i] ) ) ) / 2.f;
					break;
				case 4:// Asym To Right
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					break;
				case 5:// Asym To Left
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					break;
				case 6:// Weird 2
					if( sample_morerealindex[i][l] > sampLen[i] / 2.f )
					{
						sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					}
					else
					{
						sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
						sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
						sample_morerealindex[i][l] = sample_morerealindex[i][l] - sampLen[i] / 2.f;
					}
					break;
				case 7:// Stretch From Center
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= sampLen[i] / 2.f;
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] + 1 ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] + 1 ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				case 8:// Squish To Center
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sampLen[i] + 1  ) ) : -pow( -sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sampLen[i] + 1 ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				case 9:// Stretch And Squish
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				case 10:// Add Tail
					sample_length_modify[i][l] = modifyVal[i];
					dynamic_cast<Microwave *>(nph->instrumentTrack()->instrument())->m_graph.setLength( sampLen[i] + sample_length_modify[i][l] );
					break;
				case 11:// Cut Off Right
					sample_morerealindex[i][l] *= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					break;
				case 12:// Cut Off Left
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					sample_morerealindex[i][l] *= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					break;
				case 13:// Flip
					break;
				case 14:// Squarify
					break;
				case 15:// Pulsify
					break;
				default:
					{}
			}
			sample_morerealindex[i][l] += indexOffset[i];// Moves index for Spray knob
			sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sampLen[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds
			
			resultsample[i][l] = 0;
			if( sample_morerealindex[i][l] <= sampLen[i] + sample_length_modify[i][l] )
			{
				loopStart = qBound(0.f, morphVal[i] - rangeVal[i], 524288.f / sampLen[i] );
				loopEnd = qBound(0.f, morphVal[i] + rangeVal[i], 524288.f / sampLen[i]) + 1;
				currentMorphVal = morphVal[i];
				currentRangeValInvert = 1.f / rangeVal[i];// Inverted to prevent excessive division in the loop below, for a minor CPU improvement.  Don't ask.
				currentSampLen = sampLen[i];
				currentIndex = sample_morerealindex[i][l];
				if( modifyModeVal[i] < 14 )// If NOT Squarify Modify Mode
				{
					// Only grab samples from the waveforms when they will be used, depending on the Range knob
					for( int j = loopStart + 1; j < loopEnd; ++j )
					{
						// Get waveform samples, set their volumes depending on Range knob
						resultsample[i][l] += waveforms[i][currentIndex + j * currentSampLen] * qMax(0.0f, 1 - ( abs( currentMorphVal - j ) * currentRangeValInvert ));
					}
				}
				else if( modifyModeVal[i] == 14 )// If Squarify Modify Mode
				{
					// Self-made formula, may be buggy.  Crossfade one half of waveform with the inverse of the other.
					// Some CPU improvements can be made here.
					for( int j = loopStart + 1; j < loopEnd; ++j )
					{
						resultsample[i][l] +=
						(    waveforms[i][currentIndex + j * currentSampLen] * qMax(0.0f, 1 - ( abs( currentMorphVal - j ) * currentRangeValInvert )) ) + // Normal
						( ( -waveforms[i][( ( currentIndex + ( currentSampLen / 2 ) ) % currentSampLen ) + j * currentSampLen] * qMax(0.0f, 1 - ( abs( currentMorphVal - j ) * currentRangeValInvert )) ) * ( ( modifyVal[i] * 2.f ) / currentSampLen ) ) / // Inverted other half of waveform
						( ( modifyVal[i] / currentSampLen ) + 1 ); // Volume compensation
					}
				}
				else if( modifyModeVal[i] == 15 )// Pulsify Mode
				{
					// Self-made formula, may be buggy.  Crossfade one side of waveform with the inverse of the other.
					// Some CPU improvements can be made here.
					for( int j = loopStart + 1; j < loopEnd; ++j )
					{
						resultsample[i][l] +=
						(    waveforms[i][currentIndex + j * currentSampLen] * qMax(0.0f, 1 - ( abs( currentMorphVal - j ) * currentRangeValInvert )) ) + // Normal
						( ( -waveforms[i][( ( currentIndex + int( currentSampLen * ( modifyVal[i] / currentSampLen ) ) ) % currentSampLen ) + j * currentSampLen] * qMax(0.0f, 1 - ( abs( currentMorphVal - j ) * currentRangeValInvert )) ) * 2.f ) / // Inverted other side of waveform
						2.f; // Volume compensation
					}
				}
			}
			resultsample[i][l] /= ( rangeVal[i] / 2.f ) + 3;// Decreases volume so Range value doesn't make things too loud
	
			switch( modifyModeVal[i] )// More horrifying formulas for the various Modify Modes
			{
				case 0:// Pulse Width
					if( sample_realindex[i][l] / ( ( -modifyVal[i] + sampLen[i] ) / sampLen[i] ) > sampLen[i] )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 2:// Shrink Right Half To Left Edge
					if( sample_realindex[i][l] + (((sample_realindex[i][l])+sampLen[i]/2.f) * (modifyVal[i]/( sampLen[i] ))) > sampLen[i]  )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 10:// Add Tail
					if( sample_morerealindex[i][l] > sampLen[i] )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 13:// Flip
					if( sample_realindex[i][l] > modifyVal[i] )
					{
						resultsample[i][l] *= -1;
					}
					break;
				default:
					{}
			}
	
			sample_realindex[i][l] += sample_step[i][l];
	
			// check overflow
			while ( sample_realindex[i][l] >= sampLen[i] + sample_length_modify[i][l] ) {
				sample_realindex[i][l] -= sampLen[i] + sample_length_modify[i][l];
				indexOffset[i] = fastRandf( int( sprayVal[i] ) ) - int( sprayVal[i] / 2 );// Reset offset caused by Spray knob
			}
	
			sample[i][l] = resultsample[i][l] * ( vol[i] / 100.f );
		}
	}

	//====================//
	//== SUB OSCILLATOR ==//
	//====================//

	sample_step_sub = 0;
	sample_t subsample[64][2] = {{0}};
	for( int l = 0; l < maxSubEnabled; ++l )// maxSubEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( subEnabled[l] )
		{
			if( !subNoise[l] )
			{
				noteFreq = subKeytrack[l] ? detuneWithCents( nph->frequency(), subDetune[l] ) : detuneWithCents( 440.f, subDetune[l] ) ;
				sample_step_sub = static_cast<float>( subSampLen[l] / ( sample_rate / noteFreq ) );
	
				subsample[l][0] = ( subVol[l] / 100.f ) * subs[int( ( int( sample_subindex[l] + ( subPhase[l] * subSampLen[l] ) ) % int(subSampLen[l]) ) + ( 1024 * l ) )];
				subsample[l][1] = subsample[l][0];

				if( subPanning[l] < 0 )
				{
					subsample[l][1] *= ( 100.f + subPanning[l] ) / 100.f;
				}
				else if( subPanning[l] > 0 )
				{
					subsample[l][0] *= ( 100.f - subPanning[l] ) / 100.f;
				}

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
		
				lastSubVal[l][0] = subsample[l][0];// Store value for modulation
				lastSubVal[l][1] = subsample[l][1];// Store value for modulation
	
				subsample[l][0] = subMuted[l] ? 0 : subsample[l][0];// Mutes sub after saving value for modulation if the muted option is on
				subsample[l][1] = subMuted[l] ? 0 : subsample[l][1];// Mutes sub after saving value for modulation if the muted option is on
		
				sample_subindex[l] += sample_step_sub;
		
				// move waveform position back if it passed the end of the waveform
				while ( sample_subindex[l] >= subSampLen[l] )
				{
					sample_subindex[l] -= subSampLen[l];
				}
			}
			else// sub oscillator is noise
			{
				noiseSampRand = fastRandf( subSampLen[l] ) - 1;
				subsample[l][0] = qBound( -1.f, ( lastSubVal[l][0]+(subs[int(noiseSampRand)] / 8.f) ) / 1.2f, 1.f ) * ( subVol[l] / 100.f );// Division by 1.2f to tame DC offset
				subsample[l][1] = subsample[l][0];
				lastSubVal[l][0] = subsample[l][0];
				lastSubVal[l][1] = subsample[l][1];
				subsample[l][0] = subMuted[l] ? 0 : subsample[l][0];
				subsample[l][1] = subMuted[l] ? 0 : subsample[l][1];
			}
		}
		else
		{
			subsample[l][0] = 0;
			subsample[l][1] = 0;
			lastSubVal[l][0] = 0;
			lastSubVal[l][1] = 0;
		}
	}

	//=======================//
	//== SAMPLE OSCILLATOR ==//
	//=======================//

	sample_step_sample = 0;
	sample_t samplesample[8][2] = {{0}};
	for( int l = 0; l < maxSampleEnabled; ++l )// maxSampleEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		int sampleSize = samples[l][0].size() * sampleEnd[l];
		if( sampleEnabled[l] && ( sample_sampleindex[l] < sampleSize || sampleLoop[l] ) )
		{
			if( sampleLoop[l] )
			{
				if( sample_sampleindex[l] > sampleSize )
				{
					sample_sampleindex[l] = sample_sampleindex[l] - sampleSize + ( samples[l][0].size() * sampleStart[l] );
				}
			}

			if( sampleKeytracking[l] )
			{
				sample_step_sample = static_cast<float>( ( detuneWithCents( nph->frequency(), sampleDetune[l] ) / 440.f ) * ( 44100.f / sample_rate ) );
			}
			else
			{
				sample_step_sample = static_cast<float>( 44100.f / sample_rate );
			}

			if( sampleGraphEnabled[l] )
			{
				float progress = fmod( ( sample_sampleindex[l] + ( samplePhase[l] * sampleSize / 100.f ) ), sampleSize ) / sampleSize;

				float progress2 = sampGraphs[int(128*progress)] * ( 1 - fmod( 128*progress, 1 ) );

				float progress3;
				if( int(128*progress)+1 < 128 )
				{
					progress3 = sampGraphs[int(128*progress)+1] * fmod( 128*progress, 1 );
				}
				else
				{
					progress3 = sampGraphs[int(128*progress)] * fmod( 128*progress, 1 );
				}

				samplesample[l][0] = samples[l][0][int( ( ( progress2 + progress3 + 1 ) / 2.f ) * sampleSize )];
				samplesample[l][1] = samples[l][1][int( ( ( progress2 + progress3 + 1 ) / 2.f ) * sampleSize )];
			}
			else
			{
				samplesample[l][0] = samples[l][0][fmod(( sample_sampleindex[l] + ( samplePhase[l] * sampleSize / 100.f ) ), sampleSize)];
				samplesample[l][1] = samples[l][1][fmod(( sample_sampleindex[l] + ( samplePhase[l] * sampleSize / 100.f ) ), sampleSize)];
			}

			samplesample[l][0] *= sampleVolume[l] / 100.f;
			samplesample[l][1] *= sampleVolume[l] / 100.f;

			if( samplePanning[l] < 0 )
			{
				samplesample[l][1] *= ( 100.f + samplePanning[l] ) / 100.f;
			}
			else if( samplePanning[l] > 0 )
			{
				samplesample[l][0] *= ( 100.f - samplePanning[l] ) / 100.f;
			}
	
			lastSampleVal[l][0] = samplesample[l][0];// Store value for modulation
			lastSampleVal[l][1] = samplesample[l][1];// Store value for modulation

			if( sampleMuted[l] )
			{
				samplesample[l][0] = 0;
				samplesample[l][1] = 0;
			}
	
			sample_sampleindex[l] += sample_step_sample;
		}
		else
		{
			samplesample[l][0] = 0;
			samplesample[l][1] = 0;
			lastSampleVal[l][0] = 0;
			lastSampleVal[l][1] = 0;
		}
	}

	std::vector<float> sampleAvg(2);
	std::vector<float> sampleMainOsc(2);
	for( int i = 0; i < 8; ++i )
	{
		if( enabled[i] )
		{
			lastMainOscVal[i] = ( sample[i][0] + sample[i][1] ) / 2.f;// Store results for modulations
			if( !muted[i] )
			{
				if( unisonVoices[i] > 1 )
				{
					sampleMainOsc[0] = 0;
					sampleMainOsc[1] = 0;
					for( int j = 0; j < unisonVoices[i]; ++j )
					{
						// Pan unison voices
						sampleMainOsc[0] += sample[i][j] * (((unisonVoices[i]-1)-j)/(unisonVoices[i]-1));
						sampleMainOsc[1] += sample[i][j] * (j/(unisonVoices[i]-1));
					}
					// Decrease volume so more unison voices won't increase volume too much
					sampleMainOsc[0] /= unisonVoices[i] / 2.f;
					sampleMainOsc[1] /= unisonVoices[i] / 2.f;
					
					sampleAvg[0] += sampleMainOsc[0];
					sampleAvg[1] += sampleMainOsc[1];
				}
				else
				{
					sampleAvg[0] += sample[i][0];
					sampleAvg[1] += sample[i][0];
				}
			}
		}
	}

	// Sub Oscillator outputs
	for( int l = 0; l < maxSubEnabled; ++l )// maxSubEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( subEnabled[l] )
		{
			sampleAvg[0] += subsample[l][0];
			sampleAvg[1] += subsample[l][1];
		}
	}

	// Sample Oscillator outputs
	for( int l = 0; l < maxSampleEnabled; ++l )// maxSubEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		if( sampleEnabled[l] )
		{
			sampleAvg[0] += samplesample[l][0];
			sampleAvg[1] += samplesample[l][1];
		}
	}

	// Filter outputs
	for( int l = 0; l < 8; ++l )
	{
		if( filtEnabled[l] )
		{
			for( int m = 0; m < 8; ++m )
			{
				sampleAvg[0] += filtOutputs[l][m][0];
				sampleAvg[1] += filtOutputs[l][m][1];
			}
		}
	}

	// Refresh all modulated values back to the value of the knob.
	for( int i = 0; i < modValType.size(); ++i )
	{
		refreshValue( modValType[0], modValNum[0] );// Possible CPU problem
		modValType.erase(modValType.begin());
		modValNum.erase(modValNum.begin());
	}

	return sampleAvg;
}


// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float mSynth::detuneWithCents( float pitchValue, float detuneValue )
{
	return pitchValue * pow( 2, detuneValue / 1200.f );
}


// At the end of mSynth::nextStringSample, this will refresh all modulated values back to the value of the knob.
void mSynth::refreshValue( int which, int num )
{
	Microwave * mwc = dynamic_cast<Microwave *>(nph->instrumentTrack()->instrument());
	switch( which )
	{
		case 1: _morphVal[num] = mwc->m_morph[num]->value(); break;
		case 2: rangeVal[num] = mwc->m_range[num]->value(); break;
		case 3: _modifyVal[num] = mwc->m_modify[num]->value(); break;
		case 4: modifyModeVal[num] = mwc->m_modifyMode[num]->value(); break;
		case 5: sprayVal[num] = mwc->m_spray[num]->value(); break;
		case 6: unisonVoices[num] = mwc->m_unisonVoices[num]->value(); break;
		case 7: unisonDetune[num] = mwc->m_unisonDetune[num]->value(); break;
		case 8: unisonMorph[num] = mwc->m_unisonMorph[num]->value(); break;
		case 9: unisonModify[num] = mwc->m_unisonModify[num]->value(); break;
		case 10: morphMaxVal[num] = mwc->m_morphMax[num]->value(); break;
		case 11: detuneVal[num] = mwc->m_detune[num]->value(); break;
		case 12: sampLen[num] = mwc->m_sampLen[num]->value(); break;
		case 13: phase[num] = mwc->m_phase[num]->value(); break;
		case 14: vol[num] = mwc->m_vol[num]->value(); break;
		case 15: filtInVol[num] = mwc->m_filtInVol[num]->value(); break;
		case 16: filtType[num] = mwc->m_filtType[num]->value(); break;
		case 17: filtSlope[num] = mwc->m_filtSlope[num]->value(); break;
		case 18: filtCutoff[num] = mwc->m_filtCutoff[num]->value(); break;
		case 19: filtReso[num] = mwc->m_filtReso[num]->value(); break;
		case 20: filtGain[num] = mwc->m_filtGain[num]->value(); break;
		case 21: filtSatu[num] = mwc->m_filtSatu[num]->value(); break;
		case 22: filtWetDry[num] = mwc->m_filtWetDry[num]->value(); break;
		case 23: filtBal[num] = mwc->m_filtBal[num]->value(); break;
		case 24: filtOutVol[num] = mwc->m_filtOutVol[num]->value(); break;
		case 25: filtEnabled[num] = mwc->m_filtEnabled[num]->value(); break;
		case 26: subEnabled[num] = mwc->m_subEnabled[num]->value(); break;
		case 27: subVol[num] = mwc->m_subVol[num]->value(); break;
		case 28: subPhase[num] = mwc->m_subPhase[num]->value(); break;
		case 29: subPhaseRand[num] = mwc->m_subPhaseRand[num]->value(); break;
		case 30: subDetune[num] = mwc->m_subDetune[num]->value(); break;
		case 31: subMuted[num] = mwc->m_subMuted[num]->value(); break;
		case 32: subKeytrack[num] = mwc->m_subKeytrack[num]->value(); break;
		case 33: subSampLen[num] = mwc->m_subSampLen[num]->value(); break;
		case 34: subNoise[num] = mwc->m_subNoise[num]->value(); break;
		case 35: m_modIn[num] = mwc->m_modIn[num]->value(); break;
		case 36: m_modInNum[num] = mwc->m_modInNum[num]->value(); break;
		case 37: m_modInOtherNum[num] = mwc->m_modInOtherNum[num]->value(); break;
		case 38: m_modInAmnt[num] = mwc->m_modInAmnt[num]->value(); break;
		case 39: m_modInCurve[num] = mwc->m_modInCurve[num]->value(); break;
		case 40: m_modIn2[num] = mwc->m_modIn2[num]->value(); break;
		case 41: m_modInNum2[num] = mwc->m_modInNum2[num]->value(); break;
		case 42: m_modInOtherNum2[num] = mwc->m_modInOtherNum2[num]->value(); break;
		case 43: m_modInAmnt2[num] = mwc->m_modInAmnt2[num]->value(); break;
		case 44: m_modInCurve2[num] = mwc->m_modInCurve2[num]->value(); break;
		case 45: m_modOutSec[num] = mwc->m_modOutSec[num]->value(); break;
		case 46: m_modOutSig[num] = mwc->m_modOutSig[num]->value(); break;
		case 47: m_modOutSecNum[num] = mwc->m_modOutSecNum[num]->value(); break;
		case 48: enabled[num] = mwc->m_enabled[num]->value(); break;
		case 49: modEnabled[num] = mwc->m_modEnabled[num]->value(); break;
		case 50: m_modCombineType[num] = mwc->m_modCombineType[num]->value(); break;
		case 57: muted[num] = mwc->m_muted[num]->value(); break;
		case 59: sampleEnabled[num] = mwc->m_sampleEnabled[num]->value(); break;
		case 60: sampleGraphEnabled[num] = mwc->m_sampleGraphEnabled[num]->value(); break;
		case 61: sampleMuted[num] = mwc->m_sampleMuted[num]->value(); break;
		case 62: sampleKeytracking[num] = mwc->m_sampleKeytracking[num]->value(); break;
		case 63: sampleLoop[num] = mwc->m_sampleLoop[num]->value(); break;
		case 64: sampleVolume[num] = mwc->m_sampleVolume[num]->value(); break;
		case 65: samplePanning[num] = mwc->m_samplePanning[num]->value(); break;
		case 66: sampleDetune[num] = mwc->m_sampleDetune[num]->value(); break;
		case 67: samplePhase[num] = mwc->m_samplePhase[num]->value(); break;
		case 68: samplePhaseRand[num] = mwc->m_samplePhaseRand[num]->value(); break;
		case 69: filtFeedback[num] = mwc->m_filtFeedback[num]->value(); break;
		case 70: filtDetune[num] = mwc->m_filtDetune[num]->value(); break;
		case 71: filtKeytracking[num] = mwc->m_filtKeytracking[num]->value(); break;
		case 72: subPanning[num] = mwc->m_subPanning[num]->value(); break;
		case 73: sampleStart[num] = mwc->m_sampleStart[num]->value(); break;
		case 74: sampleEnd[num] = mwc->m_sampleEnd[num]->value(); break;
	}
}





extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *, void * _data )
{
	return( new Microwave( static_cast<InstrumentTrack *>( _data ) ) );
}


}


