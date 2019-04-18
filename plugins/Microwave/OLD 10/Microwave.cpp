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
#include <QScrollBar>
#include <QLabel>






















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
	visvol( 100, 0, 1000, 0.01f, this, tr( "Visualizer Volume" ) ),
	modNum(1, 1, 32, this, tr( "Modulation Page Number" ) ),
	loadAlg( 0, 0, 7, 1, this, tr( "Wavetable Loading Algorithm" ) ),
	loadChnl( 0, 0, 1, 1, this, tr( "Wavetable Loading Channel" ) ),
	scroll( 1, 1, 5, 0.0001f, this, tr( "Scroll" ) ),
	subNum(1, 1, 64, this, tr( "Sub Oscillator Number" ) ),
	sampNum(1, 1, 8, this, tr( "Sample Number" ) ),
	mainNum(1, 1, 8, this, tr( "Main Oscillator Number" ) ),
	oversample( this, tr("Oversampling") ),
	graph( -1.0f, 1.0f, 2048, this ),
	visualize( false, this )
{

	for( int i = 0; i < 8; ++i )
	{
		morph[i] = new FloatModel( 0, 0, 254, 0.0001f, this, tr( "Morph" ) );
		range[i] = new FloatModel( 8, 1, 16, 0.0001f, this, tr( "Range" ) );
		sampLen[i] = new FloatModel( 2048, 1, 8192, 1.f, this, tr( "Waveform Sample Length" ) );
		morphMax[i] = new FloatModel( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) );
		modifyMode[i] = new ComboBoxModel( this, tr( "Wavetable Modifier Mode" ) );
		modify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Wavetable Modifier Value" ) );
		unisonVoices[i] = new FloatModel( 1, 1, 32, 1, this, tr( "Unison Voices" ) );
		unisonDetune[i] = new FloatModel( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) );
		unisonDetune[i]->setScaleLogarithmic( true );
		unisonMorph[i] = new FloatModel( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) );
		unisonModify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Unison Modify" ) );
		detune[i] = new FloatModel( 0, -9600, 9600, 1.f, this, tr( "Detune" ) );
		phase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		phaseRand[i] = new FloatModel( 100, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		vol[i] = new FloatModel( 100.f, 0, 200.f, 0.0001f, this, tr( "Volume" ) );
		enabled[i] = new BoolModel( false, this );
		muted[i] = new BoolModel( false, this );
		pan[i] = new FloatModel( 0.f, -100.f, 100.f, 0.0001f, this, tr( "Panning" ) );
		setwavemodel( modifyMode[i] )

		filtInVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Input Volume" ) );
		filtType[i] = new ComboBoxModel( this, tr( "Filter Type" ) );
		filtSlope[i] = new ComboBoxModel( this, tr( "Filter Slope" ) );
		filtCutoff[i] = new FloatModel( 2000, 20, 20000, 0.0001f, this, tr( "Cutoff Frequency" ) );
		filtCutoff[i]->setScaleLogarithmic( true );
		filtReso[i] = new FloatModel( 0.707, 0, 16, 0.0001f, this, tr( "Resonance" ) );
		filtReso[i]->setScaleLogarithmic( true );
		filtGain[i] = new FloatModel( 0, -64, 64, 0.0001f, this, tr( "dbGain" ) );
		filtGain[i]->setScaleLogarithmic( true );
		filtSatu[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Saturation" ) );
		filtWetDry[i] = new FloatModel( 100, 0, 100, 0.0001f, this, tr( "Wet/Dry" ) );
		filtBal[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Balance/Panning" ) );
		filtOutVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Output Volume" ) );
		filtEnabled[i] = new BoolModel( false, this );
		filtFeedback[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Feedback" ) );
		filtDetune[i] = new FloatModel( 0, -4800, 4800, 0.0001f, this, tr( "Detune" ) );
		filtKeytracking[i] = new BoolModel( true, this );
		filtertypesmodel( filtType[i] )
		filterslopesmodel( filtSlope[i] )

		sampleEnabled[i] = new BoolModel( false, this );
		sampleGraphEnabled[i] = new BoolModel( false, this );
		sampleMuted[i] = new BoolModel( false, this );
		sampleKeytracking[i] = new BoolModel( true, this );
		sampleLoop[i] = new BoolModel( true, this );

		sampleVolume[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Volume" ) );
		samplePanning[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Panning" ) );
		sampleDetune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
		samplePhase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		samplePhaseRand[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		sampleStart[i] = new FloatModel( 0, 0, 0.9999f, 0.0001f, this, tr( "Start" ) );
		sampleEnd[i] = new FloatModel( 1, 0.0001f, 1, 0.0001f, this, tr( "End" ) );
	}

	for( int i = 0; i < 64; ++i )
 	{
		subEnabled[i] = new BoolModel( false, this );
		subVol[i] = new FloatModel( 100.f, 0.f, 200.f, 0.0001f, this, tr( "Volume" ) );
		subPhase[i] = new FloatModel( 0.f, 0.f, 200.f, 0.0001f, this, tr( "Phase" ) );
		subPhaseRand[i] = new FloatModel( 0.f, 0.f, 100.f, 0.0001f, this, tr( "Phase Randomness" ) );
		subDetune[i] = new FloatModel( 0.f, -9600.f, 9600.f, 1.f, this, tr( "Detune" ) );
		subMuted[i] = new BoolModel( true, this );
		subKeytrack[i] = new BoolModel( true, this );
		subSampLen[i] = new FloatModel( 2048.f, 1.f, 2048.f, 1.f, this, tr( "Sample Length" ) );
		subNoise[i] = new BoolModel( false, this );
		subPanning[i] = new FloatModel( 0.f, -100.f, 100.f, 0.0001f, this, tr( "Panning" ) );
		subTempo[i] = new FloatModel( 0.f, 0.f, 400.f, 1.f, this, tr( "Tempo" ) );

		modEnabled[i] = new BoolModel( false, this );

		modOutSec[i] = new ComboBoxModel( this, tr( "Modulation Section" ) );
		modOutSig[i] = new ComboBoxModel( this, tr( "Modulation Signal" ) );
		modOutSecNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulation Section Number" ) );
		modsectionsmodel( modOutSec[i] )
		mainoscsignalsmodel( modOutSig[i] )
		
		modIn[i] = new ComboBoxModel( this, tr( "Modulator" ) );
		modInNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		modInOtherNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		modinmodel( modIn[i] )

		modInAmnt[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Modulator Amount" ) );
		modInCurve[i] = new FloatModel( 100, 0.0001f, 200, 0.0001f, this, tr( "Modulator Curve" ) );

		modIn2[i] = new ComboBoxModel( this, tr( "Secondary Modulator" ) );
		modInNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		modInOtherNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		modinmodel( modIn2[i] )

		modInAmnt2[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Secondary Modulator Amount" ) );
		modInCurve2[i] = new FloatModel( 100, 0.0001f, 200, 0.0001f, this, tr( "Secondary Modulator Curve" ) );

		modCombineType[i] = new ComboBoxModel( this, tr( "Combination Type" ) );
		modcombinetypemodel( modCombineType[i] )

		modType[i] = new BoolModel( false, this );
	}

	oversamplemodel( oversample )


	graph.setWaveToSine();

	for( int i = 0; i < 8; ++i )
	{
		connect( morphMax[i], SIGNAL( dataChanged( ) ),
			this, SLOT( morphMaxChanged( ) ) );
	}

	connect( &graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );

	for( int i = 0; i < 8; ++i )
	{
		connect( morph[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(1, i); } );
		connect( range[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(2, i); } );
		connect( modify[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(3, i); } );
		connect( modifyMode[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(4, i); } );
		connect( unisonVoices[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(6, i); } );
		connect( unisonDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(7, i); } );
		connect( unisonMorph[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(8, i); } );
		connect( unisonModify[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(9, i); } );
		connect( morphMax[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(10, i); } );
		connect( detune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(11, i); } );
		connect( sampLen[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(12, i); } );
		connect( phase[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(13, i); } );
		connect( vol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(14, i); } );
		connect( muted[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(57, i); } );

		connect( filtInVol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(15, i); } );
		connect( filtType[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(16, i); } );
		connect( filtSlope[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(17, i); } );
		connect( filtCutoff[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(18, i); } );
		connect( filtReso[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(19, i); } );
		connect( filtGain[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(20, i); } );
		connect( filtSatu[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(21, i); } );
		connect( filtWetDry[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(22, i); } );
		connect( filtBal[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(23, i); } );
		connect( filtOutVol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(24, i); } );
		connect( filtEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(25, i); } );
		connect( filtFeedback[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(69, i); } );
		connect( filtDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(70, i); } );
		connect( filtKeytracking[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(71, i); } );

		connect( enabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(48, i); } );

		connect( sampleEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(59, i); } );

		connect( sampleGraphEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(60, i); } );

		connect( sampleMuted[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(61, i); } );

		connect( sampleKeytracking[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(62, i); } );

		connect( sampleLoop[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(63, i); } );

		connect( sampleVolume[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(64, i); } );

		connect( samplePanning[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(65, i); } );

		connect( sampleDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(66, i); } );

		connect( samplePhase[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(67, i); } );

		connect( samplePhaseRand[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(68, i); } );

		connect( sampleStart[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(73, i); } );

		connect( sampleEnd[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(74, i); } );

		connect( pan[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(75, i); } );

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

		connect( sampleEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { sampleEnabledChanged(i); } );

		connect( enabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { mainEnabledChanged(i); } );
	}

	for( int i = 0; i < 64; ++i )
	{
		connect( subEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(26, i); } );
		connect( subVol[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(27, i); } );
		connect( subPhase[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(28, i); } );
		connect( subPhaseRand[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(29, i); } );
		connect( subDetune[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(30, i); } );
		connect( subMuted[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(31, i); } );
		connect( subKeytrack[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(32, i); } );
		connect( subSampLen[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(33, i); } );
		connect( subNoise[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(34, i); } );
		connect( subPanning[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(72, i); } );
		connect( subTempo[i], &FloatModel::dataChanged,
            			this, [this, i]() { valueChanged(76, i); } );
		for( int j = 26; j <= 35; ++j )
		{
			valueChanged(j, i);
		}

		connect( subSampLen[i], &FloatModel::dataChanged,
            			this, [this, i]() { subSampLenChanged(i); } );
		connect( subEnabled[i], &BoolModel::dataChanged,
            			this, [this, i]() { subEnabledChanged(i); } );

		connect( modIn[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(35, i); } );
		connect( modInNum[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(36, i); } );
		connect( modInOtherNum[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(37, i); } );
		connect( modInAmnt[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(38, i); } );
		connect( modInCurve[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(39, i); } );
		connect( modIn2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(40, i); } );
		connect( modInNum2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(41, i); } );
		connect( modInOtherNum2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(42, i); } );
		connect( modInAmnt2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(43, i); } );
		connect( modInCurve2[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(44, i); } );
		connect( modOutSec[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(45, i); } );
		connect( modOutSig[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(46, i); } );
		connect( modOutSecNum[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(47, i); } );
		connect( modEnabled[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(49, i); } );
		connect( modCombineType[i], &ComboBoxModel::dataChanged,
            			this, [this, i]() { valueChanged(50, i); } );
		connect( modType[i], &BoolModel::dataChanged,
            			this, [this, i]() { valueChanged(77, i); } );
		for( int j = 35; j <= 47; ++j )
		{
			valueChanged(j, i);
		}
		valueChanged(49, i);
		valueChanged(50, i);
		valueChanged(72, i);
		valueChanged(76, i);
		valueChanged(77, i);

		connect( modEnabled[i], &BoolModel::dataChanged,
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

	visvol.saveSettings( _doc, _this, "visualizervolume" );
	loadAlg.saveSettings( _doc, _this, "loadingalgorithm" );
	loadChnl.saveSettings( _doc, _this, "loadingchannel" );

	QString saveString;

	for( int i = 0; i < 8; ++i )
	{
		if( enabled[i]->value() )
		{
			base64::encode( (const char *)waveforms[i],
				524288 * sizeof(float), saveString );
			_this.setAttribute( "waveforms"+QString::number(i), saveString );
		}
	}

	base64::encode( (const char *)subs,
		131072 * sizeof(float), saveString );
	_this.setAttribute( "subs", saveString );

	base64::encode( (const char *)sampGraphs,
		1024 * sizeof(float), saveString );
	_this.setAttribute( "sampGraphs", saveString );

	int sampleSizes[8] = {0};
	for( int i = 0; i < 8; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			for( int j = 0; j < 2; ++j )
			{
				base64::encode( (const char *)samples[i][j].data(),
					samples[i][j].size() * sizeof(float), saveString );
				_this.setAttribute( "samples_"+QString::number(i)+"_"+QString::number(j), saveString );
			}

			sampleSizes[i] = samples[i][0].size();
		}
	}

	base64::encode( (const char *)sampleSizes,
		8 * sizeof(int), saveString );
	_this.setAttribute( "sampleSizes", saveString );

	for( int i = 0; i < maxMainEnabled; ++i )
	{
		if( enabled[i]->value() )
		{
			morph[i]->saveSettings( _doc, _this, "morph_"+QString::number(i) );
			range[i]->saveSettings( _doc, _this, "range_"+QString::number(i) );
			modify[i]->saveSettings( _doc, _this, "modify_"+QString::number(i) );
			modifyMode[i]->saveSettings( _doc, _this, "modifyMode_"+QString::number(i) );
			unisonVoices[i]->saveSettings( _doc, _this, "unisonVoices_"+QString::number(i) );
			unisonDetune[i]->saveSettings( _doc, _this, "unisonDetune_"+QString::number(i) );
			unisonMorph[i]->saveSettings( _doc, _this, "unisonMorph_"+QString::number(i) );
			unisonModify[i]->saveSettings( _doc, _this, "unisonModify_"+QString::number(i) );
			morphMax[i]->saveSettings( _doc, _this, "morphMax_"+QString::number(i) );
			detune[i]->saveSettings( _doc, _this, "detune_"+QString::number(i) );
			sampLen[i]->saveSettings( _doc, _this, "sampLen_"+QString::number(i) );
			phase[i]->saveSettings( _doc, _this, "phase_"+QString::number(i) );
			vol[i]->saveSettings( _doc, _this, "vol_"+QString::number(i) );
			enabled[i]->saveSettings( _doc, _this, "enabled_"+QString::number(i) );
			muted[i]->saveSettings( _doc, _this, "muted_"+QString::number(i) );
			pan[i]->saveSettings( _doc, _this, "pan_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxSampleEnabled; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			sampleEnabled[i]->saveSettings( _doc, _this, "sampleEnabled_"+QString::number(i) );
			sampleGraphEnabled[i]->saveSettings( _doc, _this, "sampleGraphEnabled_"+QString::number(i) );
			sampleMuted[i]->saveSettings( _doc, _this, "sampleMuted_"+QString::number(i) );
			sampleKeytracking[i]->saveSettings( _doc, _this, "sampleKeytracking_"+QString::number(i) );
			sampleLoop[i]->saveSettings( _doc, _this, "sampleLoop_"+QString::number(i) );
			sampleVolume[i]->saveSettings( _doc, _this, "sampleVolume_"+QString::number(i) );
			samplePanning[i]->saveSettings( _doc, _this, "samplePanning_"+QString::number(i) );
			sampleDetune[i]->saveSettings( _doc, _this, "sampleDetune_"+QString::number(i) );
			samplePhase[i]->saveSettings( _doc, _this, "samplePhase_"+QString::number(i) );
			samplePhaseRand[i]->saveSettings( _doc, _this, "samplePhaseRand_"+QString::number(i) );
			sampleStart[i]->saveSettings( _doc, _this, "sampleStart_"+QString::number(i) );
			sampleEnd[i]->saveSettings( _doc, _this, "sampleEnd_"+QString::number(i) );
		}
	}

	for( int i = 0; i < 8; ++i )
	{
		if( filtEnabled[i]->value() )
		{
			filtInVol[i]->saveSettings( _doc, _this, "filtInVol_"+QString::number(i) );
			filtType[i]->saveSettings( _doc, _this, "filtType_"+QString::number(i) );
			filtSlope[i]->saveSettings( _doc, _this, "filtSlope_"+QString::number(i) );
			filtCutoff[i]->saveSettings( _doc, _this, "filtCutoff_"+QString::number(i) );
			filtReso[i]->saveSettings( _doc, _this, "filtReso_"+QString::number(i) );
			filtGain[i]->saveSettings( _doc, _this, "filtGain_"+QString::number(i) );
			filtSatu[i]->saveSettings( _doc, _this, "filtSatu_"+QString::number(i) );
			filtWetDry[i]->saveSettings( _doc, _this, "filtWetDry_"+QString::number(i) );
			filtBal[i]->saveSettings( _doc, _this, "filtBal_"+QString::number(i) );
			filtOutVol[i]->saveSettings( _doc, _this, "filtOutVol_"+QString::number(i) );
			filtEnabled[i]->saveSettings( _doc, _this, "filtEnabled_"+QString::number(i) );
			filtFeedback[i]->saveSettings( _doc, _this, "filtFeedback_"+QString::number(i) );
			filtDetune[i]->saveSettings( _doc, _this, "filtDetune_"+QString::number(i) );
			filtKeytracking[i]->saveSettings( _doc, _this, "filtKeytracking_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxSubEnabled; ++i )
	{
		if( subEnabled[i]->value() )
		{
			subEnabled[i]->saveSettings( _doc, _this, "subEnabled_"+QString::number(i) );
			subVol[i]->saveSettings( _doc, _this, "subVol_"+QString::number(i) );
			subPhase[i]->saveSettings( _doc, _this, "subPhase_"+QString::number(i) );
			subPhaseRand[i]->saveSettings( _doc, _this, "subPhaseRand_"+QString::number(i) );
			subDetune[i]->saveSettings( _doc, _this, "subDetune_"+QString::number(i) );
			subMuted[i]->saveSettings( _doc, _this, "subMuted_"+QString::number(i) );
			subKeytrack[i]->saveSettings( _doc, _this, "subKeytrack_"+QString::number(i) );
			subSampLen[i]->saveSettings( _doc, _this, "subSampLen_"+QString::number(i) );
			subNoise[i]->saveSettings( _doc, _this, "subNoise_"+QString::number(i) );
			subPanning[i]->saveSettings( _doc, _this, "subPanning_"+QString::number(i) );
			subTempo[i]->saveSettings( _doc, _this, "subTempo_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxModEnabled; ++i )
	{
		if( modEnabled[i]->value() )
		{
			modIn[i]->saveSettings( _doc, _this, "modIn_"+QString::number(i) );
			modInNum[i]->saveSettings( _doc, _this, "modInNu"+QString::number(i) );
			modInOtherNum[i]->saveSettings( _doc, _this, "modInOtherNu"+QString::number(i) );
			modInAmnt[i]->saveSettings( _doc, _this, "modInAmnt_"+QString::number(i) );
			modInCurve[i]->saveSettings( _doc, _this, "modInCurve_"+QString::number(i) );
			modIn2[i]->saveSettings( _doc, _this, "modIn2_"+QString::number(i) );
			modInNum2[i]->saveSettings( _doc, _this, "modInNum2_"+QString::number(i) );
			modInOtherNum2[i]->saveSettings( _doc, _this, "modInOtherNum2_"+QString::number(i) );
			modInAmnt2[i]->saveSettings( _doc, _this, "modAmnt2_"+QString::number(i) );
			modInCurve2[i]->saveSettings( _doc, _this, "modCurve2_"+QString::number(i) );
			modOutSec[i]->saveSettings( _doc, _this, "modOutSec_"+QString::number(i) );
			modOutSig[i]->saveSettings( _doc, _this, "modOutSig_"+QString::number(i) );
			modOutSecNum[i]->saveSettings( _doc, _this, "modOutSecNu"+QString::number(i) );
			modEnabled[i]->saveSettings( _doc, _this, "modEnabled_"+QString::number(i) );
			modCombineType[i]->saveSettings( _doc, _this, "modCombineType_"+QString::number(i) );
			modType[i]->saveSettings( _doc, _this, "modType_"+QString::number(i) );
		}
	}
}


void Microwave::loadSettings( const QDomElement & _this )
{
	visvol.loadSettings( _this, "visualizervolume" );
	loadAlg.loadSettings( _this, "loadingalgorithm" );
	loadChnl.loadSettings( _this, "loadingchannel" );

	graph.setLength( 2048 );

	for( int i = 0; i < 8; ++i )
	{
		enabled[i]->loadSettings( _this, "enabled_"+QString::number(i) );
		if( enabled[i]->value() )
		{
			morph[i]->loadSettings( _this, "morph_"+QString::number(i) );
			range[i]->loadSettings( _this, "range_"+QString::number(i) );
			modify[i]->loadSettings( _this, "modify_"+QString::number(i) );
			modifyMode[i]->loadSettings( _this, "modifyMode_"+QString::number(i) );
			unisonVoices[i]->loadSettings( _this, "unisonVoices_"+QString::number(i) );
			unisonDetune[i]->loadSettings( _this, "unisonDetune_"+QString::number(i) );
			unisonMorph[i]->loadSettings( _this, "unisonMorph_"+QString::number(i) );
			unisonModify[i]->loadSettings( _this, "unisonModify_"+QString::number(i) );
			morphMax[i]->loadSettings( _this, "morphMax_"+QString::number(i) );
			detune[i]->loadSettings( _this, "detune_"+QString::number(i) );
			sampLen[i]->loadSettings( _this, "sampLen_"+QString::number(i) );
			phase[i]->loadSettings( _this, "phase_"+QString::number(i) );
			vol[i]->loadSettings( _this, "vol_"+QString::number(i) );
			muted[i]->loadSettings( _this, "muted_"+QString::number(i) );
			pan[i]->loadSettings( _this, "pan_"+QString::number(i) );
		}

		filtEnabled[i]->loadSettings( _this, "filtEnabled_"+QString::number(i) );
		if( filtEnabled[i]->value() )
		{
			filtInVol[i]->loadSettings( _this, "filtInVol_"+QString::number(i) );
			filtType[i]->loadSettings( _this, "filtType_"+QString::number(i) );
			filtSlope[i]->loadSettings( _this, "filtSlope_"+QString::number(i) );
			filtCutoff[i]->loadSettings( _this, "filtCutoff_"+QString::number(i) );
			filtReso[i]->loadSettings( _this, "filtReso_"+QString::number(i) );
			filtGain[i]->loadSettings( _this, "filtGain_"+QString::number(i) );
			filtSatu[i]->loadSettings( _this, "filtSatu_"+QString::number(i) );
			filtWetDry[i]->loadSettings( _this, "filtWetDry_"+QString::number(i) );
			filtBal[i]->loadSettings( _this, "filtBal_"+QString::number(i) );
			filtOutVol[i]->loadSettings( _this, "filtOutVol_"+QString::number(i) );
			filtFeedback[i]->loadSettings( _this, "filtFeedback_"+QString::number(i) );
			filtDetune[i]->loadSettings( _this, "filtDetune_"+QString::number(i) );
			filtKeytracking[i]->loadSettings( _this, "filtKeytracking_"+QString::number(i) );
		}

		sampleEnabled[i]->loadSettings( _this, "sampleEnabled_"+QString::number(i) );
		if( sampleEnabled[i]->value() )
		{
			sampleGraphEnabled[i]->loadSettings( _this, "sampleGraphEnabled_"+QString::number(i) );
			sampleMuted[i]->loadSettings( _this, "sampleMuted_"+QString::number(i) );
			sampleKeytracking[i]->loadSettings( _this, "sampleKeytracking_"+QString::number(i) );
			sampleLoop[i]->loadSettings( _this, "sampleLoop_"+QString::number(i) );
			sampleVolume[i]->loadSettings( _this, "sampleVolume_"+QString::number(i) );
			samplePanning[i]->loadSettings( _this, "samplePanning_"+QString::number(i) );
			sampleDetune[i]->loadSettings( _this, "sampleDetune_"+QString::number(i) );
			samplePhase[i]->loadSettings( _this, "samplePhase_"+QString::number(i) );
			samplePhaseRand[i]->loadSettings( _this, "samplePhaseRand_"+QString::number(i) );
			sampleStart[i]->loadSettings( _this, "sampleStart_"+QString::number(i) );
			sampleEnd[i]->loadSettings( _this, "sampleEnd_"+QString::number(i) );
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		subEnabled[i]->loadSettings( _this, "subEnabled_"+QString::number(i) );
		if( subEnabled[i]->value() )
		{
			subVol[i]->loadSettings( _this, "subVol_"+QString::number(i) );
			subPhase[i]->loadSettings( _this, "subPhase_"+QString::number(i) );
			subPhaseRand[i]->loadSettings( _this, "subPhaseRand_"+QString::number(i) );
			subDetune[i]->loadSettings( _this, "subDetune_"+QString::number(i) );
			subMuted[i]->loadSettings( _this, "subMuted_"+QString::number(i) );
			subKeytrack[i]->loadSettings( _this, "subKeytrack_"+QString::number(i) );
			subSampLen[i]->loadSettings( _this, "subSampLen_"+QString::number(i) );
			subNoise[i]->loadSettings( _this, "subNoise_"+QString::number(i) );
			subPanning[i]->loadSettings( _this, "subPanning_"+QString::number(i) );
			subTempo[i]->loadSettings( _this, "subTempo_"+QString::number(i) );
		}

		modEnabled[i]->loadSettings( _this, "modEnabled_"+QString::number(i) );
		if( modEnabled[i]->value() )
		{
			modIn[i]->loadSettings( _this, "modIn_"+QString::number(i) );
			modInNum[i]->loadSettings( _this, "modInNu"+QString::number(i) );
			modInOtherNum[i]->loadSettings( _this, "modInOtherNu"+QString::number(i) );
			modInAmnt[i]->loadSettings( _this, "modInAmnt_"+QString::number(i) );
			modInCurve[i]->loadSettings( _this, "modInCurve_"+QString::number(i) );
			modIn2[i]->loadSettings( _this, "modIn2_"+QString::number(i) );
			modInNum2[i]->loadSettings( _this, "modInNum2_"+QString::number(i) );
			modInOtherNum2[i]->loadSettings( _this, "modInOtherNum2_"+QString::number(i) );
			modInAmnt2[i]->loadSettings( _this, "modAmnt2_"+QString::number(i) );
			modInCurve2[i]->loadSettings( _this, "modCurve2_"+QString::number(i) );
			modOutSec[i]->loadSettings( _this, "modOutSec_"+QString::number(i) );
			modOutSig[i]->loadSettings( _this, "modOutSig_"+QString::number(i) );
			modOutSecNum[i]->loadSettings( _this, "modOutSecNu"+QString::number(i) );
			modCombineType[i]->loadSettings( _this, "modCombineType_"+QString::number(i) );
			modType[i]->loadSettings( _this, "modType_"+QString::number(i) );
		}
	}

	// Load arrays
	int size = 0;
	char * dst = 0;
	for( int j = 0; j < 8; ++j )
	{
		if( enabled[j]->value() )
		{
			base64::decode( _this.attribute( "waveforms"+QString::number(j) ), &dst, &size );
			for( int i = 0; i < 524288; ++i )
			{
				waveforms[j][i] = ( (float*) dst )[i];
			}
		}
	}

	base64::decode( _this.attribute( "subs" ), &dst, &size );
	for( int i = 0; i < 131072; ++i )
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
		if( sampleEnabled[i]->value() )
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
	}

	delete[] dst;

}


// When a knob is changed, send the new value to the array holding the knob values, as well as the note values within mSynths already initialized (notes already playing)
void Microwave::valueChanged( int which, int num )
{
	//Send new values to array
	switch( which )
	{
		case 1: morphArr[num] = morph[num]->value(); break;
		case 2: rangeArr[num] = range[num]->value(); break;
		case 3: modifyArr[num] = modify[num]->value(); break;
		case 4: modifyModeArr[num] = modifyMode[num]->value(); break;
		case 6: unisonVoicesArr[num] = unisonVoices[num]->value(); break;
		case 7: unisonDetuneArr[num] = unisonDetune[num]->value(); break;
		case 8: unisonMorphArr[num] = unisonMorph[num]->value(); break;
		case 9: unisonModifyArr[num] = unisonModify[num]->value(); break;
		case 10: morphMaxArr[num] = morphMax[num]->value(); break;
		case 11: detuneArr[num] = detune[num]->value(); break;
		case 12: sampLenArr[num] = sampLen[num]->value(); break;
		case 13: phaseArr[num] = phase[num]->value(); break;
		case 14: volArr[num] = vol[num]->value(); break;
		case 15: filtInVolArr[num] = filtInVol[num]->value(); break;
		case 16: filtTypeArr[num] = filtType[num]->value(); break;
		case 17: filtSlopeArr[num] = filtSlope[num]->value(); break;
		case 18: filtCutoffArr[num] = filtCutoff[num]->value(); break;
		case 19: filtResoArr[num] = filtReso[num]->value(); break;
		case 20: filtGainArr[num] = filtGain[num]->value(); break;
		case 21: filtSatuArr[num] = filtSatu[num]->value(); break;
		case 22: filtWetDryArr[num] = filtWetDry[num]->value(); break;
		case 23: filtBalArr[num] = filtBal[num]->value(); break;
		case 24: filtOutVolArr[num] = filtOutVol[num]->value(); break;
		case 25: filtEnabledArr[num] = filtEnabled[num]->value(); break;
		case 26: subEnabledArr[num] = subEnabled[num]->value(); break;
		case 27: subVolArr[num] = subVol[num]->value(); break;
		case 28: subPhaseArr[num] = subPhase[num]->value(); break;
		case 29: subPhaseRandArr[num] = subPhaseRand[num]->value(); break;
		case 30: subDetuneArr[num] = subDetune[num]->value(); break;
		case 31: subMutedArr[num] = subMuted[num]->value(); break;
		case 32: subKeytrackArr[num] = subKeytrack[num]->value(); break;
		case 33: subSampLenArr[num] = subSampLen[num]->value(); break;
		case 34: subNoiseArr[num] = subNoise[num]->value(); break;
		case 35: modInArr[num] = modIn[num]->value(); break;
		case 36: modInNumArr[num] = modInNum[num]->value(); break;
		case 37: modInOtherNumArr[num] = modInOtherNum[num]->value(); break;
		case 38: modInAmntArr[num] = modInAmnt[num]->value(); break;
		case 39: modInCurveArr[num] = modInCurve[num]->value(); break;
		case 40: modIn2Arr[num] = modIn2[num]->value(); break;
		case 41: modInNum2Arr[num] = modInNum2[num]->value(); break;
		case 42: modInOtherNum2Arr[num] = modInOtherNum2[num]->value(); break;
		case 43: modInAmnt2Arr[num] = modInAmnt2[num]->value(); break;
		case 44: modInCurve2Arr[num] = modInCurve2[num]->value(); break;
		case 45: modOutSecArr[num] = modOutSec[num]->value(); break;
		case 46: modOutSigArr[num] = modOutSig[num]->value(); break;
		case 47: modOutSecNumArr[num] = modOutSecNum[num]->value(); break;
		case 48: enabledArr[num] = enabled[num]->value(); break;
		case 49: modEnabledArr[num] = modEnabled[num]->value(); break;
		case 50: modCombineTypeArr[num] = modCombineType[num]->value(); break;
		case 57: mutedArr[num] = muted[num]->value(); break;
		case 59: sampleEnabledArr[num] = sampleEnabled[num]->value(); break;
		case 60: sampleGraphEnabledArr[num] = sampleGraphEnabled[num]->value(); break;
		case 61: sampleMutedArr[num] = sampleMuted[num]->value(); break;
		case 62: sampleKeytrackingArr[num] = sampleKeytracking[num]->value(); break;
		case 63: sampleLoopArr[num] = sampleLoop[num]->value(); break;
		case 64: sampleVolumeArr[num] = sampleVolume[num]->value(); break;
		case 65: samplePanningArr[num] = samplePanning[num]->value(); break;
		case 66: sampleDetuneArr[num] = sampleDetune[num]->value(); break;
		case 67: samplePhaseArr[num] = samplePhase[num]->value(); break;
		case 68: samplePhaseRandArr[num] = samplePhaseRand[num]->value(); break;
		case 69: filtFeedbackArr[num] = filtFeedback[num]->value(); break;
		case 70: filtDetuneArr[num] = filtDetune[num]->value(); break;
		case 71: filtKeytrackingArr[num] = filtKeytracking[num]->value(); break;
		case 72: subPanningArr[num] = subPanning[num]->value(); break;
		case 73: sampleStartArr[num] = sampleStart[num]->value(); break;
		case 74: sampleEndArr[num] = sampleEnd[num]->value(); break;
		case 75: panArr[num] = pan[num]->value(); break;
		case 76: subTempoArr[num] = subTempo[num]->value(); break;
		case 77: modTypeArr[num] = modType[num]->value(); break;
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
			case 1: ps->morphVal[num] = morph[num]->value(); break;
			case 2: ps->rangeVal[num] = range[num]->value(); break;
			case 3: ps->modifyVal[num] = modify[num]->value(); break;
			case 4: ps->modifyModeVal[num] = modifyMode[num]->value(); break;
			case 6: ps->unisonVoices[num] = unisonVoices[num]->value(); break;
			case 7: ps->unisonDetune[num] = unisonDetune[num]->value(); break;
			case 8: ps->unisonMorph[num] = unisonMorph[num]->value(); break;
			case 9: ps->unisonModify[num] = unisonModify[num]->value(); break;
			case 10: ps->morphMaxVal[num] = morphMax[num]->value(); break;
			case 11: ps->detuneVal[num] = detune[num]->value(); break;
			case 12: ps->sampLen[num] = sampLen[num]->value(); break;
			case 13: ps->phase[num] = phase[num]->value(); break;
			case 14: ps->vol[num] = vol[num]->value(); break;
			case 15: ps->filtInVol[num] = filtInVol[num]->value(); break;
			case 16: ps->filtType[num] = filtType[num]->value(); break;
			case 17: ps->filtSlope[num] = filtSlope[num]->value(); break;
			case 18: ps->filtCutoff[num] = filtCutoff[num]->value(); break;
			case 19: ps->filtReso[num] = filtReso[num]->value(); break;
			case 20: ps->filtGain[num] = filtGain[num]->value(); break;
			case 21: ps->filtSatu[num] = filtSatu[num]->value(); break;
			case 22: ps->filtWetDry[num] = filtWetDry[num]->value(); break;
			case 23: ps->filtBal[num] = filtBal[num]->value(); break;
			case 24: ps->filtOutVol[num] = filtOutVol[num]->value(); break;
			case 25: ps->filtEnabled[num] = filtEnabled[num]->value(); break;
			case 26: ps->subEnabled[num] = subEnabled[num]->value(); break;
			case 27: ps->subVol[num] = subVol[num]->value(); break;
			case 28: ps->subPhase[num] = subPhase[num]->value(); break;
			case 29: ps->subPhaseRand[num] = subPhaseRand[num]->value(); break;
			case 30: ps->subDetune[num] = subDetune[num]->value(); break;
			case 31: ps->subMuted[num] = subMuted[num]->value(); break;
			case 32: ps->subKeytrack[num] = subKeytrack[num]->value(); break;
			case 33: ps->subSampLen[num] = subSampLen[num]->value(); break;
			case 34: ps->subNoise[num] = subNoise[num]->value(); break;
			case 35: ps->modIn[num] = modIn[num]->value(); break;
			case 36: ps->modInNum[num] = modInNum[num]->value(); break;
			case 37: ps->modInOtherNum[num] = modInOtherNum[num]->value(); break;
			case 38: ps->modInAmnt[num] = modInAmnt[num]->value(); break;
			case 39: ps->modInCurve[num] = modInCurve[num]->value(); break;
			case 40: ps->modIn2[num] = modIn2[num]->value(); break;
			case 41: ps->modInNum2[num] = modInNum2[num]->value(); break;
			case 42: ps->modInOtherNum2[num] = modInOtherNum2[num]->value(); break;
			case 43: ps->modInAmnt2[num] = modInAmnt2[num]->value(); break;
			case 44: ps->modInCurve2[num] = modInCurve2[num]->value(); break;
			case 45: ps->modOutSec[num] = modOutSec[num]->value(); break;
			case 46: ps->modOutSig[num] = modOutSig[num]->value(); break;
			case 47: ps->modOutSecNum[num] = modOutSecNum[num]->value(); break;
			case 48: ps->enabled[num] = enabled[num]->value(); break;
			case 49: ps->modEnabled[num] = modEnabled[num]->value(); break;
			case 50: ps->modCombineType[num] = modCombineType[num]->value(); break;
			case 57: ps->muted[num] = muted[num]->value(); break;
			case 59: ps->sampleEnabled[num] = sampleEnabled[num]->value(); break;
			case 60: ps->sampleGraphEnabled[num] = sampleGraphEnabled[num]->value(); break;
			case 61: ps->sampleMuted[num] = sampleMuted[num]->value(); break;
			case 62: ps->sampleKeytracking[num] = sampleKeytracking[num]->value(); break;
			case 63: ps->sampleLoop[num] = sampleLoop[num]->value(); break;
			case 64: ps->sampleVolume[num] = sampleVolume[num]->value(); break;
			case 65: ps->samplePanning[num] = samplePanning[num]->value(); break;
			case 66: ps->sampleDetune[num] = sampleDetune[num]->value(); break;
			case 67: ps->samplePhase[num] = samplePhase[num]->value(); break;
			case 68: ps->samplePhaseRand[num] = samplePhaseRand[num]->value(); break;
			case 69: ps->filtFeedback[num] = filtFeedback[num]->value(); break;
			case 70: ps->filtDetune[num] = filtDetune[num]->value(); break;
			case 71: ps->filtKeytracking[num] = filtKeytracking[num]->value(); break;
			case 72: ps->subPanning[num] = subPanning[num]->value(); break;
			case 73: ps->sampleStart[num] = sampleStart[num]->value(); break;
			case 74: ps->sampleEnd[num] = sampleEnd[num]->value(); break;
			case 75: ps->pan[num] = pan[num]->value(); break;
			case 76: ps->subTempo[num] = subTempo[num]->value(); break;
			case 77: ps->modType[num] = modType[num]->value(); break;
		}
	}
}


// Set the range of Morph based on Morph Max
void Microwave::morphMaxChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		morph[i]->setRange( morph[i]->getMin(), morphMax[i]->value(), morph[i]->getStep() );
	}
}


// Set the range of morphMax and Modify based on new sample length
void Microwave::sampLenChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		morphMax[i]->setRange( morphMax[i]->getMin(), 524288.f/sampLen[i]->value() - 2, morphMax[i]->getStep() );
		modify[i]->setRange( modify[i]->getMin(), sampLen[i]->value() - 1, modify[i]->getStep() );
	}
}


//Change graph length to sample length
void Microwave::subSampLenChanged( int num )
{
	graph.setLength( subSampLen[num]->value() );
}


//Stores the highest enabled main oscillator.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::mainEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( enabled[i]->value() )
		{
			maxMainEnabled = i+1;
		}
	}
}


//Stores the highest enabled sub oscillator.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::subEnabledChanged( int num )
{
	for( int i = 0; i < 64; ++i )
	{
		if( subEnabled[i]->value() )
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
		if( modEnabled[i]->value() )
		{
			maxModEnabled = i+1;
		}
	}

	//matrixScrollBar.setRange( 0, maxModEnabled * 100.f );
}


//Stores the highest enabled sample.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::sampleEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			maxSampleEnabled = i+1;
		}
	}
}


//When user drawn on graph, send new values to the correct arrays
void Microwave::samplesChanged( int _begin, int _end )
{
	switch( currentTab )
	{
		case 0:
			break;
		case 1:
			for( int i = _begin; i <= _end; ++i )
			{
				subs[i + ( (subNum.value()-1) * 2048 )] = graph.samples()[i];
			}
			break;
		case 2:
			for( int i = _begin; i <= _end; ++i )
			{
				sampGraphs[i + ( (sampNum.value()-1) * 128 )] = graph.samples()[i];
			}
			break;
		case 3:
			break;
	}
}


void Microwave::switchMatrixSections( int source, int destination )
{
	int modInTemp = modInArr[destination];
	int modInNumTemp = modInNumArr[destination];
	float modInOtherNumTemp = modInOtherNumArr[destination];
	float modInAmntTemp = modInAmntArr[destination];
	float modInCurveTemp = modInCurveArr[destination];
	int modIn2Temp = modIn2Arr[destination];
	int modInNum2Temp = modInNum2Arr[destination];
	int modInOtherNum2Temp = modInOtherNum2Arr[destination];
	float modInAmnt2Temp = modInAmnt2Arr[destination];
	float modInCurve2Temp = modInCurve2Arr[destination];
	int modOutSecTemp = modOutSecArr[destination];
	int modOutSigTemp = modOutSigArr[destination];
	int modOutSecNumTemp = modOutSecNumArr[destination];
	bool modEnabledTemp = modEnabledArr[destination];
	int modCombineTypeTemp = modCombineTypeArr[destination];
	bool modTypeTemp = modTypeArr[destination];

	modIn[destination]->setValue( modInArr[source] );
	modInNum[destination]->setValue( modInNumArr[source] );
	modInOtherNum[destination]->setValue( modInOtherNumArr[source] );
	modInAmnt[destination]->setValue( modInAmntArr[source] );
	modInCurve[destination]->setValue( modInCurveArr[source] );
	modIn2[destination]->setValue( modIn2Arr[source] );
	modInNum2[destination]->setValue( modInNum2Arr[source] );
	modInOtherNum2[destination]->setValue( modInOtherNum2Arr[source] );
	modInAmnt2[destination]->setValue( modInAmnt2Arr[source] );
	modInCurve2[destination]->setValue( modInCurve2Arr[source] );
	modOutSec[destination]->setValue( modOutSecArr[source] );
	modOutSig[destination]->setValue( modOutSigArr[source] );
	modOutSecNum[destination]->setValue( modOutSecNumArr[source] );
	modEnabled[destination]->setValue( modEnabledArr[source] );
	modCombineType[destination]->setValue( modCombineTypeArr[source] );
	modType[destination]->setValue( modTypeArr[source] );

	modIn[source]->setValue( modInTemp );
	modInNum[source]->setValue( modInNumTemp );
	modInOtherNum[source]->setValue( modInOtherNumTemp );
	modInAmnt[source]->setValue( modInAmntTemp );
	modInCurve[source]->setValue( modInCurveTemp );
	modIn2[source]->setValue( modIn2Temp );
	modInNum2[source]->setValue( modInNum2Temp );
	modInOtherNum2[source]->setValue( modInOtherNum2Temp );
	modInAmnt2[source]->setValue( modInAmnt2Temp );
	modInCurve2[source]->setValue( modInCurve2Temp );
	modOutSec[source]->setValue( modOutSecTemp );
	modOutSig[source]->setValue( modOutSigTemp );
	modOutSecNum[source]->setValue( modOutSecNumTemp );
	modEnabled[source]->setValue( modEnabledTemp );
	modCombineType[source]->setValue( modCombineTypeTemp );
	modType[source]->setValue( modTypeTemp );
}


// For when notes are playing.  This initializes a new mSynth if the note is new.  It also uses mSynth::nextStringSample to get the synthesizer output.  This is where oversampling and the visualizer are handled.
void Microwave::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{

	if ( _n->m_pluginData == NULL || _n->totalFramesPlayed() == 0 )
	{
	
		float phaseRandArr[8] = {0};// Minor CPU problem here
		for( int i = 0; i < 8; ++i )
		{
			phaseRandArr[i] = phaseRand[i]->value();
		}
		_n->m_pluginData = new mSynth(
					_n,
				Engine::mixer()->processingSampleRate(),
					phaseRandArr,
					modifyModeArr, modifyArr, morphArr, rangeArr, unisonVoicesArr, unisonDetuneArr, unisonMorphArr, unisonModifyArr, morphMaxArr, detuneArr, waveforms, subs, subEnabledArr, subVolArr, subPhaseArr, subPhaseRandArr, subDetuneArr, subMutedArr, subKeytrackArr, subSampLenArr, subNoiseArr, sampLenArr, modInArr, modInNumArr, modInOtherNumArr, modInAmntArr, modInCurveArr, modIn2Arr, modInNum2Arr, modInOtherNum2Arr, modInAmnt2Arr, modInCurve2Arr, modOutSecArr, modOutSigArr, modOutSecNumArr, modCombineTypeArr, modTypeArr, phaseArr, volArr, filtInVolArr, filtTypeArr, filtSlopeArr, filtCutoffArr, filtResoArr, filtGainArr, filtSatuArr, filtWetDryArr, filtBalArr, filtOutVolArr, filtEnabledArr, enabledArr, modEnabledArr, sampGraphs, mutedArr, sampleEnabledArr, sampleGraphEnabledArr, sampleMutedArr, sampleKeytrackingArr, sampleLoopArr, sampleVolumeArr, samplePanningArr, sampleDetuneArr, samplePhaseArr, samplePhaseRandArr, samples, filtFeedbackArr, filtDetuneArr, filtKeytrackingArr, subPanningArr, sampleStartArr, sampleEndArr, panArr, subTempoArr );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	mSynth * ps = static_cast<mSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		// Process some samples and ignore the output, depending on the oversampling value.  For example, if the oversampling is set to 4x, it will process 4 samples and output 1 of those.
		for( int i = 0; i < oversample.value(); ++i )
		{
			ps->nextStringSample( waveforms, subs, sampGraphs, samples, maxModEnabled, maxSubEnabled, maxSampleEnabled, maxMainEnabled, Engine::mixer()->processingSampleRate() * (oversample.value()+1) );
		}
		//Get the actual synthesizer output
		std::vector<float> sample = ps->nextStringSample( waveforms, subs, sampGraphs, samples, maxModEnabled, maxSubEnabled, maxSampleEnabled, maxMainEnabled, Engine::mixer()->processingSampleRate() * (oversample.value()+1) );

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			//Send to output
			_working_buffer[frame][chnl] = sample[chnl];
		}

		//update visualizer
		if( visualize.value() )
		{
			if( abs( const_cast<float*>( graph.samples() )[int(ps->sample_realindex[0][0])] - ( ((sample[0]+sample[1])/2.f) * visvol.value() / 100 ) ) >= 0.01f )
			{
				graph.setSampleAt( ps->sample_realindex[0][0], ((sample[0]+sample[1])/2.f) * visvol.value() / 100 );
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

	QPixmap filtBoxesImg = PLUGIN_NAME::getIconPixmap("filterBoxes");
	filtBoxesLabel = new QLabel(this);
	filtBoxesLabel->setPixmap(filtBoxesImg);
	filtBoxesLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	QPixmap matrixBoxesImg = PLUGIN_NAME::getIconPixmap("matrixBoxes");
	matrixBoxesLabel = new QLabel(this);
	matrixBoxesLabel->setPixmap(matrixBoxesImg);
	matrixBoxesLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	for( int i = 0; i < 8; ++i )
	{
		morphKnob[i] = new Knob( knobMicrowave, this );
		morphKnob[i]->setHintText( tr( "Morph" ), "" );
	
		rangeKnob[i] = new Knob( knobMicrowave, this );
		rangeKnob[i]->setHintText( tr( "Range" ), "" );

		sampLenKnob[i] = new Knob( knobMicrowave, this );
		sampLenKnob[i]->setHintText( tr( "Waveform Sample Length" ), "" );

		morphMaxKnob[i] = new Knob( knobMicrowave, this );
		morphMaxKnob[i]->setHintText( tr( "Morph Max" ), "" );

		modifyKnob[i] = new Knob( knobMicrowave, this );
		modifyKnob[i]->setHintText( tr( "Modify" ), "" );

		unisonVoicesKnob[i] = new Knob( knobSmallMicrowave, this );
		unisonVoicesKnob[i]->setHintText( tr( "Unison Voices" ), "" );

		unisonDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		unisonDetuneKnob[i]->setHintText( tr( "Unison Detune" ), "" );

		unisonMorphKnob[i] = new Knob( knobSmallMicrowave, this );
		unisonMorphKnob[i]->setHintText( tr( "Unison Morph" ), "" );

		unisonModifyKnob[i] = new Knob( knobSmallMicrowave, this );
		unisonModifyKnob[i]->setHintText( tr( "Unison Modify" ), "" );

		detuneKnob[i] = new Knob( knobSmallMicrowave, this );
		detuneKnob[i]->setHintText( tr( "Detune" ), "" );

		phaseKnob[i] = new Knob( knobSmallMicrowave, this );
		phaseKnob[i]->setHintText( tr( "Phase" ), "" );

		phaseRandKnob[i] = new Knob( knobSmallMicrowave, this );
		phaseRandKnob[i]->setHintText( tr( "Phase Randomness" ), "" );

		volKnob[i] = new Knob( knobMicrowave, this );
		volKnob[i]->setHintText( tr( "Volume" ), "" );

		modifyModeBox[i] = new ComboBox( this );
		modifyModeBox[i]->setGeometry( 0, 5, 42, 22 );
		modifyModeBox[i]->setFont( pointSize<8>( modifyModeBox[i]->font() ) );

		enabledToggle[i] = new LedCheckBox( "Oscillator Enabled", this, tr( "Oscillator Enabled" ), LedCheckBox::Green );

		mutedToggle[i] = new LedCheckBox( "Oscillator Muted", this, tr( "Oscillator Muted" ), LedCheckBox::Green );


		filtInVolKnob[i] = new Knob( knobSmallMicrowave, this );
		filtInVolKnob[i]->setHintText( tr( "Input Volume" ), "" );

		filtTypeBox[i] = new ComboBox( this );
		filtTypeBox[i]->setGeometry( 1000, 5, 42, 22 );
		filtTypeBox[i]->setFont( pointSize<8>( filtTypeBox[i]->font() ) );

		filtSlopeBox[i] = new ComboBox( this );
		filtSlopeBox[i]->setGeometry( 1000, 5, 42, 22 );
		filtSlopeBox[i]->setFont( pointSize<8>( filtSlopeBox[i]->font() ) );

		filtCutoffKnob[i] = new Knob( knobSmallMicrowave, this );
		filtCutoffKnob[i]->setHintText( tr( "Cutoff Frequency" ), "" );

		filtResoKnob[i] = new Knob( knobSmallMicrowave, this );
		filtResoKnob[i]->setHintText( tr( "Resonance" ), "" );

		filtGainKnob[i] = new Knob( knobSmallMicrowave, this );
		filtGainKnob[i]->setHintText( tr( "db Gain" ), "" );

		filtSatuKnob[i] = new Knob( knobSmallMicrowave, this );
		filtSatuKnob[i]->setHintText( tr( "Saturation" ), "" );

		filtWetDryKnob[i] = new Knob( knobSmallMicrowave, this );
		filtWetDryKnob[i]->setHintText( tr( "Wet/Dry" ), "" );

		filtBalKnob[i] = new Knob( knobSmallMicrowave, this );
		filtBalKnob[i]->setHintText( tr( "Balance/Panning" ), "" );

		filtOutVolKnob[i] = new Knob( knobSmallMicrowave, this );
		filtOutVolKnob[i]->setHintText( tr( "Output Volume" ), "" );

		filtEnabledToggle[i] = new LedCheckBox( "", this, tr( "Filter Enabled" ), LedCheckBox::Green );

		filtFeedbackKnob[i] = new Knob( knobSmallMicrowave, this );
		filtFeedbackKnob[i]->setHintText( tr( "Feedback" ), "" );

		filtDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		filtDetuneKnob[i]->setHintText( tr( "Detune" ), "" );

		filtKeytrackingToggle[i] = new LedCheckBox( "", this, tr( "Keytracking" ), LedCheckBox::Green );

		sampleEnabledToggle[i] = new LedCheckBox( "", this, tr( "Sample Enabled" ), LedCheckBox::Green );
		sampleGraphEnabledToggle[i] = new LedCheckBox( "", this, tr( "Sample Graph Enabled" ), LedCheckBox::Green );
		sampleMutedToggle[i] = new LedCheckBox( "", this, tr( "Sample Muted" ), LedCheckBox::Green );
		sampleKeytrackingToggle[i] = new LedCheckBox( "", this, tr( "Sample Keytracking" ), LedCheckBox::Green );
		sampleLoopToggle[i] = new LedCheckBox( "", this, tr( "Loop Sample" ), LedCheckBox::Green );

		sampleVolumeKnob[i] = new Knob( knobMicrowave, this );
		sampleVolumeKnob[i]->setHintText( tr( "Volume" ), "" );
		samplePanningKnob[i] = new Knob( knobMicrowave, this );
		samplePanningKnob[i]->setHintText( tr( "Panning" ), "" );
		sampleDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		sampleDetuneKnob[i]->setHintText( tr( "Detune" ), "" );
		samplePhaseKnob[i] = new Knob( knobSmallMicrowave, this );
		samplePhaseKnob[i]->setHintText( tr( "Phase" ), "" );
		samplePhaseRandKnob[i] = new Knob( knobSmallMicrowave, this );
		samplePhaseRandKnob[i]->setHintText( tr( "Phase Randomness" ), "" );
		sampleStartKnob[i] = new Knob( knobSmallMicrowave, this );
		sampleStartKnob[i]->setHintText( tr( "Start" ), "" );
		sampleEndKnob[i] = new Knob( knobSmallMicrowave, this );
		sampleEndKnob[i]->setHintText( tr( "End" ), "" );

		panKnob[i] = new Knob( knobMicrowave, this );
		panKnob[i]->setHintText( tr( "Panning" ), "" );

		if( i != 0 )
		{
			morphKnob[i]->hide();
			rangeKnob[i]->hide();
			sampLenKnob[i]->hide();
			morphMaxKnob[i]->hide();
			modifyKnob[i]->hide();
			unisonVoicesKnob[i]->hide();
			unisonDetuneKnob[i]->hide();
			unisonMorphKnob[i]->hide();
			unisonModifyKnob[i]->hide();
			detuneKnob[i]->hide();
			phaseKnob[i]->hide();
			phaseRandKnob[i]->hide();
			modifyModeBox[i]->hide();
			volKnob[i]->hide();
			enabledToggle[i]->hide();
			mutedToggle[i]->hide();
			sampleEnabledToggle[i]->hide();
			sampleGraphEnabledToggle[i]->hide();
			sampleMutedToggle[i]->hide();
			sampleKeytrackingToggle[i]->hide();
			sampleLoopToggle[i]->hide();
			sampleVolumeKnob[i]->hide();
			samplePanningKnob[i]->hide();
			sampleDetuneKnob[i]->hide();
			samplePhaseKnob[i]->hide();
			samplePhaseRandKnob[i]->hide();
			sampleStartKnob[i]->hide();
			sampleEndKnob[i]->hide();
			panKnob[i]->hide();
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		subEnabledToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Enabled" ), LedCheckBox::Green );

		subVolKnob[i] = new Knob( knobMicrowave, this );
		subVolKnob[i]->setHintText( tr( "Sub Oscillator Volume" ), "" );

		subPhaseKnob[i] = new Knob( knobMicrowave, this );
		subPhaseKnob[i]->setHintText( tr( "Sub Oscillator Phase" ), "" );

		subPhaseRandKnob[i] = new Knob( knobMicrowave, this );
		subPhaseRandKnob[i]->setHintText( tr( "Sub Oscillator Phase Randomness" ), "" );

		subDetuneKnob[i] = new Knob( knobMicrowave, this );
		subDetuneKnob[i]->setHintText( tr( "Sub Oscillator Pitch" ), "" );

		subMutedToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Muted" ), LedCheckBox::Green );

		subKeytrackToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Keytracking Enabled" ), LedCheckBox::Green );

		subSampLenKnob[i] = new Knob( knobMicrowave, this );
		subSampLenKnob[i]->setHintText( tr( "Sub Oscillator Sample Length" ), "" );

		subNoiseToggle[i] = new LedCheckBox( "", this, tr( "Sub Oscillator Noise Enabled" ), LedCheckBox::Green );

		subPanningKnob[i] = new Knob( knobMicrowave, this );
		subPanningKnob[i]->setHintText( tr( "Sub Oscillator Panning" ), "" );

		subTempoKnob[i] = new Knob( knobMicrowave, this );
		subTempoKnob[i]->setHintText( tr( "Sub Oscillator Tempo" ), "" );

		if( i != 0 )
		{
			subEnabledToggle[i]->hide();
			subVolKnob[i]->hide();
			subPhaseKnob[i]->hide();
			subPhaseRandKnob[i]->hide();
			subDetuneKnob[i]->hide();
			subMutedToggle[i]->hide();
			subKeytrackToggle[i]->hide();
			subSampLenKnob[i]->hide();
			subNoiseToggle[i]->hide();
			subPanningKnob[i]->hide();
			subTempoKnob[i]->hide();
		}

		modOutSecBox[i] = new ComboBox( this );
		modOutSecBox[i]->setGeometry( 2000, 5, 42, 22 );
		modOutSecBox[i]->setFont( pointSize<8>( modOutSecBox[i]->font() ) );

		modOutSigBox[i] = new ComboBox( this );
		modOutSigBox[i]->setGeometry( 2000, 5, 42, 22 );
		modOutSigBox[i]->setFont( pointSize<8>( modOutSigBox[i]->font() ) );

		modOutSecNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Output Number" );

		modInBox[i] = new ComboBox( this );
		modInBox[i]->setGeometry( 2000, 5, 42, 22 );
		modInBox[i]->setFont( pointSize<8>( modInBox[i]->font() ) );

		modInNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Input Number" );

		modInOtherNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Input Number" );

		modInAmntKnob[i] = new Knob( knobMicrowave, this );
		modInAmntKnob[i]->setHintText( tr( "Modulator Amount" ), "" );

		modInCurveKnob[i] = new Knob( knobMicrowave, this );
		modInCurveKnob[i]->setHintText( tr( "Modulator Curve" ), "" );

		modInBox2[i] = new ComboBox( this );
		modInBox2[i]->setGeometry( 2000, 5, 42, 22 );
		modInBox2[i]->setFont( pointSize<8>( modInBox2[i]->font() ) );

		modInNumBox2[i] = new LcdSpinBox( 2, "microwave", this, "Secondary Mod Input Number" );

		modInOtherNumBox2[i] = new LcdSpinBox( 2, "microwave", this, "Secondary Mod Input Number" );

		modInAmntKnob2[i] = new Knob( knobMicrowave, this );
		modInAmntKnob2[i]->setHintText( tr( "Secondary Modulator Amount" ), "" );

		modInCurveKnob2[i] = new Knob( knobMicrowave, this );
		modInCurveKnob2[i]->setHintText( tr( "Secondary Modulator Curve" ), "" );

		modEnabledToggle[i] = new LedCheckBox( "", this, tr( "Modulation Enabled" ), LedCheckBox::Green );

		modCombineTypeBox[i] = new ComboBox( this );
		modCombineTypeBox[i]->setGeometry( 2000, 5, 42, 22 );
		modCombineTypeBox[i]->setFont( pointSize<8>( modCombineTypeBox[i]->font() ) );

		modTypeToggle[i] = new LedCheckBox( "", this, tr( "Envelope Enabled" ), LedCheckBox::Green );

		modUpArrow[i] = new PixmapButton( this, tr( "Move Matrix Section Up" ) );
		modUpArrow[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"arrowup" ) );
		modUpArrow[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"arrowup" ) );
		ToolTip::add( modUpArrow[i], tr( "Move Matrix Section Up" ) );

		modDownArrow[i] = new PixmapButton( this, tr( "Move Matrix Section Down" ) );
		modDownArrow[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"arrowdown" ) );
		modDownArrow[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"arrowdown" ) );
		ToolTip::add( modDownArrow[i], tr( "Move Matrix Section Down" ) );
	}

	visvolKnob = new Knob( knobMicrowave, this );
	visvolKnob->setHintText( tr( "Visualizer Volume" ), "" );

	loadAlgKnob = new Knob( knobMicrowave, this );
	loadAlgKnob->setHintText( tr( "Wavetable Loading Algorithm" ), "" );

	loadChnlKnob = new Knob( knobMicrowave, this );
	loadChnlKnob->setHintText( tr( "Wavetable Loading Channel" ), "" );


	graph = new Graph( this, Graph::BarCenterGradStyle, 204, 134 );
	graph->setAutoFillBackground( true );
	graph->setGraphColor( QColor( 121, 222, 239 ) );

	ToolTip::add( graph, tr ( "Draw your own waveform here "
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
	graph->setPalette( pal );

	QPixmap filtForegroundImg = PLUGIN_NAME::getIconPixmap("filterForeground");
	filtForegroundLabel = new QLabel(this);
	filtForegroundLabel->setPixmap(filtForegroundImg);
	filtForegroundLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	QPixmap matrixForegroundImg = PLUGIN_NAME::getIconPixmap("matrixForeground");
	matrixForegroundLabel = new QLabel(this);
	matrixForegroundLabel->setPixmap(matrixForegroundImg);
	matrixForegroundLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	sinWaveBtn = new PixmapButton( this, tr( "Sine" ) );
	sinWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	sinWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	ToolTip::add( sinWaveBtn,
			tr( "Sine wave" ) );

	triangleWaveBtn = new PixmapButton( this, tr( "Nachos" ) );
	triangleWaveBtn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	triangleWaveBtn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( triangleWaveBtn,
			tr( "Nacho wave" ) );

	sawWaveBtn = new PixmapButton( this, tr( "Sawsa" ) );
	sawWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	sawWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	ToolTip::add( sawWaveBtn,
			tr( "Sawsa wave" ) );

	sqrWaveBtn = new PixmapButton( this, tr( "Sosig" ) );
	sqrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	sqrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	ToolTip::add( sqrWaveBtn,
			tr( "Sosig wave" ) );

	whiteNoiseWaveBtn = new PixmapButton( this,
					tr( "Metal Fork" ) );
	whiteNoiseWaveBtn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	whiteNoiseWaveBtn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( whiteNoiseWaveBtn,
			tr( "Metal Fork" ) );

	usrWaveBtn = new PixmapButton( this, tr( "Takeout" ) );
	usrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	usrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	ToolTip::add( usrWaveBtn,
			tr( "Takeout Menu" ) );

	smoothBtn = new PixmapButton( this, tr( "Microwave Cover" ) );
	smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	ToolTip::add( smoothBtn,
			tr( "Microwave Cover" ) );


	sinWave2Btn = new PixmapButton( this, tr( "Sine" ) );
	sinWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	sinWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sinwave" ) );
	ToolTip::add( sinWave2Btn,
			tr( "Sine wave" ) );

	triangleWave2Btn = new PixmapButton( this, tr( "Nachos" ) );
	triangleWave2Btn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	triangleWave2Btn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( triangleWave2Btn,
			tr( "Nacho wave" ) );

	sawWave2Btn = new PixmapButton( this, tr( "Sawsa" ) );
	sawWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	sawWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"sawwave" ) );
	ToolTip::add( sawWave2Btn,
			tr( "Sawsa wave" ) );

	sqrWave2Btn = new PixmapButton( this, tr( "Sosig" ) );
	sqrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	sqrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
					"sqrwave" ) );
	ToolTip::add( sqrWave2Btn,
			tr( "Sosig wave" ) );

	whiteNoiseWave2Btn = new PixmapButton( this,
					tr( "Metal Fork" ) );
	whiteNoiseWave2Btn->setActiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	whiteNoiseWave2Btn->setInactiveGraphic(
		PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( whiteNoiseWave2Btn,
			tr( "Metal Fork" ) );

	usrWave2Btn = new PixmapButton( this, tr( "Takeout" ) );
	usrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	usrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"fileload" ) );
	ToolTip::add( usrWave2Btn,
			tr( "Takeout Menu" ) );

	smooth2Btn = new PixmapButton( this, tr( "Microwave Cover" ) );
	smooth2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	smooth2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smoothwave" ) );
	ToolTip::add( smooth2Btn,
			tr( "Microwave Cover" ) );



	visualizeToggle = new LedCheckBox( "", this, tr( "Visualize" ), LedCheckBox::Green );

	openWavetableButton = new PixmapButton( this );
	openWavetableButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	openWavetableButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	openWavetableButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	connect( openWavetableButton, SIGNAL( clicked() ),
					this, SLOT( openWavetableFileBtnClicked() ) );
	ToolTip::add( openWavetableButton, tr( "Open wavetable" ) );

	subNumBox = new LcdSpinBox( 2, "microwave", this, "Sub Oscillator Number" );

	sampNumBox = new LcdSpinBox( 2, "microwave", this, "Sample Number" );

	mainNumBox = new LcdSpinBox( 2, "microwave", this, "Oscillator Number" );

	oversampleBox = new ComboBox( this );
	oversampleBox->setGeometry( 2000, 5, 42, 22 );
	oversampleBox->setFont( pointSize<8>( oversampleBox->font() ) );
	
	openSampleButton = new PixmapButton( this );
	openSampleButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	openSampleButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );
	openSampleButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fileload" ) );

	scrollKnob = new Knob( knobSmallMicrowave, this );

	effectScrollBar = new QScrollBar( Qt::Vertical, this );
	effectScrollBar->setSingleStep( 1 );
	effectScrollBar->setPageStep( 100 );
	effectScrollBar->setFixedHeight( 197 );
	effectScrollBar->setRange( 0, 590 );
	connect( effectScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( updateScroll( ) ) );

	matrixScrollBar = new QScrollBar( Qt::Vertical, this );
	matrixScrollBar->setSingleStep( 1 );
	matrixScrollBar->setPageStep( 100 );
	matrixScrollBar->setFixedHeight( 197 );
	matrixScrollBar->setRange( 0, 6232 );
	connect( matrixScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( updateScroll( ) ) );


	connect( openSampleButton, SIGNAL( clicked() ),
					this, SLOT( openSampleFileBtnClicked() ) );
	ToolTip::add( openSampleButton, tr( "Open sample" ) );


	
	connect( sinWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );
	connect( triangleWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked() ) );
	connect( sawWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );
	connect( sqrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );
	connect( whiteNoiseWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );
	connect( usrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );
	connect( smoothBtn, SIGNAL ( clicked () ),
			this, SLOT ( smoothClicked() ) );

	connect( sinWave2Btn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );
	connect( triangleWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked() ) );
	connect( sawWave2Btn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );
	connect( sqrWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );
	connect( whiteNoiseWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );
	connect( usrWave2Btn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );
	connect( smooth2Btn, SIGNAL ( clicked () ),
			this, SLOT ( smoothClicked() ) );


	connect( visualizeToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( visualizeToggled( bool ) ) );



	connect( &castModel<Microwave>()->scroll, SIGNAL( dataChanged( ) ),
			this, SLOT( updateScroll( ) ) );

	connect( scrollKnob, SIGNAL( sliderReleased( ) ),
			this, SLOT( scrollReleased( ) ) );

	connect( &castModel<Microwave>()->mainNum, SIGNAL( dataChanged( ) ),
			this, SLOT( mainNumChanged( ) ) );

	connect( &castModel<Microwave>()->subNum, SIGNAL( dataChanged( ) ),
			this, SLOT( subNumChanged( ) ) );

	connect( &castModel<Microwave>()->sampNum, SIGNAL( dataChanged( ) ),
			this, SLOT( sampNumChanged( ) ) );

	for( int i = 0; i < 64; ++i )
	{
		connect( castModel<Microwave>()->modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { modOutSecChanged(i); } );
		connect( castModel<Microwave>()->modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { modInChanged(i); } );

		connect( modUpArrow[i], &PixmapButton::clicked, this, [this, i]() { modUpClicked(i); } );
		connect( modDownArrow[i], &PixmapButton::clicked, this, [this, i]() { modDownClicked(i); } );
	}

	updateScroll();
}


// Connects knobs/GUI elements to their models
void MicrowaveView::modelChanged()
{
	Microwave * b = castModel<Microwave>();

	for( int i = 0; i < 8; ++i )
	{
		morphKnob[i]->setModel( b->morph[i] );
		rangeKnob[i]->setModel( b->range[i] );
		sampLenKnob[i]->setModel( b->sampLen[i] );
		modifyKnob[i]->setModel( b->modify[i] );
		morphMaxKnob[i]->setModel( b->morphMax[i] );
		unisonVoicesKnob[i]->setModel( b->unisonVoices[i] );
		unisonDetuneKnob[i]->setModel( b->unisonDetune[i] );
		unisonMorphKnob[i]->setModel( b->unisonMorph[i] );
		unisonModifyKnob[i]->setModel( b->unisonModify[i] );
		detuneKnob[i]->setModel( b->detune[i] );
		modifyModeBox[i]-> setModel( b-> modifyMode[i] );
		phaseKnob[i]->setModel( b->phase[i] );
		phaseRandKnob[i]->setModel( b->phaseRand[i] );
		volKnob[i]->setModel( b->vol[i] );
		enabledToggle[i]->setModel( b->enabled[i] );
		mutedToggle[i]->setModel( b->muted[i] );
		panKnob[i]->setModel( b->pan[i] );

		filtInVolKnob[i]->setModel( b->filtInVol[i] );
		filtTypeBox[i]->setModel( b->filtType[i] );
		filtSlopeBox[i]->setModel( b->filtSlope[i] );
		filtCutoffKnob[i]->setModel( b->filtCutoff[i] );
		filtResoKnob[i]->setModel( b->filtReso[i] );
		filtGainKnob[i]->setModel( b->filtGain[i] );
		filtSatuKnob[i]->setModel( b->filtSatu[i] );
		filtWetDryKnob[i]->setModel( b->filtWetDry[i] );
		filtBalKnob[i]->setModel( b->filtBal[i] );
		filtOutVolKnob[i]->setModel( b->filtOutVol[i] );
		filtEnabledToggle[i]->setModel( b->filtEnabled[i] );
		filtFeedbackKnob[i]->setModel( b->filtFeedback[i] );
		filtDetuneKnob[i]->setModel( b->filtDetune[i] );
		filtKeytrackingToggle[i]->setModel( b->filtKeytracking[i] );

		sampleEnabledToggle[i]->setModel( b->sampleEnabled[i] );
		sampleGraphEnabledToggle[i]->setModel( b->sampleGraphEnabled[i] );
		sampleMutedToggle[i]->setModel( b->sampleMuted[i] );
		sampleKeytrackingToggle[i]->setModel( b->sampleKeytracking[i] );
		sampleLoopToggle[i]->setModel( b->sampleLoop[i] );

		sampleVolumeKnob[i]->setModel( b->sampleVolume[i] );
		samplePanningKnob[i]->setModel( b->samplePanning[i] );
		sampleDetuneKnob[i]->setModel( b->sampleDetune[i] );
		samplePhaseKnob[i]->setModel( b->samplePhase[i] );
		samplePhaseRandKnob[i]->setModel( b->samplePhaseRand[i] );
		sampleStartKnob[i]->setModel( b->sampleStart[i] );
		sampleEndKnob[i]->setModel( b->sampleEnd[i] );

	}

	for( int i = 0; i < 64; ++i )
	{
		subEnabledToggle[i]->setModel( b->subEnabled[i] );
		subVolKnob[i]->setModel( b->subVol[i] );
		subPhaseKnob[i]->setModel( b->subPhase[i] );
		subPhaseRandKnob[i]->setModel( b->subPhaseRand[i] );
		subDetuneKnob[i]->setModel( b->subDetune[i] );
		subMutedToggle[i]->setModel( b->subMuted[i] );
		subKeytrackToggle[i]->setModel( b->subKeytrack[i] );
		subSampLenKnob[i]->setModel( b->subSampLen[i] );
		subNoiseToggle[i]->setModel( b->subNoise[i] );
		subPanningKnob[i]->setModel( b->subPanning[i] );
		subTempoKnob[i]->setModel( b->subTempo[i] );

		modOutSecBox[i]-> setModel( b-> modOutSec[i] );
		modOutSigBox[i]-> setModel( b-> modOutSig[i] );
		modOutSecNumBox[i]-> setModel( b-> modOutSecNum[i] );

		modInBox[i]-> setModel( b-> modIn[i] );
		modInNumBox[i]-> setModel( b-> modInNum[i] );
		modInOtherNumBox[i]-> setModel( b-> modInOtherNum[i] );
		modInAmntKnob[i]-> setModel( b-> modInAmnt[i] );
		modInCurveKnob[i]-> setModel( b-> modInCurve[i] );

		modInBox2[i]-> setModel( b-> modIn2[i] );
		modInNumBox2[i]-> setModel( b-> modInNum2[i] );
		modInOtherNumBox2[i]-> setModel( b-> modInOtherNum2[i] );
		modInAmntKnob2[i]-> setModel( b-> modInAmnt2[i] );
		modInCurveKnob2[i]-> setModel( b-> modInCurve2[i] );

		modEnabledToggle[i]-> setModel( b-> modEnabled[i] );

		modCombineTypeBox[i]-> setModel( b-> modCombineType[i] );
		modTypeToggle[i]-> setModel( b-> modType[i] );
	}

	graph->setModel( &b->graph );
	visvolKnob->setModel( &b->visvol );
	visualizeToggle->setModel( &b->visualize );
	subNumBox->setModel( &b->subNum );
	sampNumBox->setModel( &b->sampNum );
	loadAlgKnob->setModel( &b->loadAlg );
	loadChnlKnob->setModel( &b->loadChnl );
	scrollKnob->setModel( &b->scroll );
	mainNumBox->setModel( &b->mainNum );
	oversampleBox->setModel( &b->oversample );
}


// Puts all of the GUI elements in their correct locations, depending on the scroll knob value.
void MicrowaveView::updateScroll()
{
	
	int scrollVal = ( castModel<Microwave>()->scroll.value() - 1 ) * 250.f;
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int effectScrollVal = ( effectScrollBar->value() ) / 100.f * 92.f;

	for( int i = 0; i < 8; ++i )
	{
		morphKnob[i]->move( 23 - scrollVal, 172 );
		rangeKnob[i]->move( 55 - scrollVal, 172 );
		sampLenKnob[i]->move( 0 - scrollVal, 50 );
		morphMaxKnob[i]->move( 0 - scrollVal, 75 );
		modifyKnob[i]->move( 87 - scrollVal, 172 );
		modifyModeBox[i]->move( 127 - scrollVal, 186 );
		unisonVoicesKnob[i]->move( 184 - scrollVal, 172 );
		unisonDetuneKnob[i]->move( 209 - scrollVal, 172 );
		unisonMorphKnob[i]->move( 184 - scrollVal, 203 );
		unisonModifyKnob[i]->move( 209 - scrollVal, 203 );
		detuneKnob[i]->move( 148 - scrollVal, 216 );
		phaseKnob[i]->move( 86 - scrollVal, 216 );
		phaseRandKnob[i]->move( 113 - scrollVal, 216 );
		volKnob[i]->move( 0 - scrollVal, 100 );
		enabledToggle[i]->move( 50 - scrollVal, 240 );
		mutedToggle[i]->move( 0 - scrollVal, 205 );
		panKnob[i]->move( 55 - scrollVal, 130 );

		filtInVolKnob[i]->move( 30 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtTypeBox[i]->move( 128 + 1000 - scrollVal, i*92+63 - effectScrollVal );
		filtSlopeBox[i]->move( 171 + 1000 - scrollVal, i*92+63 - effectScrollVal );
		filtCutoffKnob[i]->move( 32 + 1000 - scrollVal, i*92+55 - effectScrollVal );
		filtResoKnob[i]->move( 63 + 1000 - scrollVal, i*92+55 - effectScrollVal );
		filtGainKnob[i]->move( 94 + 1000 - scrollVal, i*92+55 - effectScrollVal );
		filtSatuKnob[i]->move( 135 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtWetDryKnob[i]->move( 80 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtBalKnob[i]->move( 105 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtOutVolKnob[i]->move( 55 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtEnabledToggle[i]->move( 27 + 1000 - scrollVal, i*92+35 - effectScrollVal );
		filtFeedbackKnob[i]->move( 167 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtDetuneKnob[i]->move( 192 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		filtKeytrackingToggle[i]->move( 199 + 1000 - scrollVal, i*92+35 - effectScrollVal );

		sampleEnabledToggle[i]->move( 86 + 500 - scrollVal, 229 );
		sampleGraphEnabledToggle[i]->move( 138 + 500 - scrollVal, 229 );
		sampleMutedToggle[i]->move( 103 + 500 - scrollVal, 229 );
		sampleKeytrackingToggle[i]->move( 121 + 500 - scrollVal, 229 );
		sampleLoopToggle[i]->move( 155 + 500 - scrollVal, 229 );

		sampleVolumeKnob[i]->move( 23 + 500 - scrollVal, 172 );
		samplePanningKnob[i]->move( 55 + 500 - scrollVal, 172 );
		sampleDetuneKnob[i]->move( 93 + 500 - scrollVal, 172 );
		samplePhaseKnob[i]->move( 180 + 500 - scrollVal, 172 );
		samplePhaseRandKnob[i]->move( 206 + 500 - scrollVal, 172 );
		sampleStartKnob[i]->move( 121 + 500 - scrollVal, 172 );
		sampleEndKnob[i]->move( 145 + 500 - scrollVal, 172 );
	}

	for( int i = 0; i < 64; ++i )
	{
		subEnabledToggle[i]->move( 108 + 250 - scrollVal, 214 );
		subVolKnob[i]->move( 23 + 250 - scrollVal, 172 );
		subPhaseKnob[i]->move( 97 + 250 - scrollVal, 172 );
		subPhaseRandKnob[i]->move( 129 + 250 - scrollVal, 172 );
		subDetuneKnob[i]->move( 60 + 250 - scrollVal, 0 );
		subMutedToggle[i]->move( 147 + 250 - scrollVal, 214 );
		subKeytrackToggle[i]->move( 108 + 250 - scrollVal, 229 );
		subSampLenKnob[i]->move( 187 + 250 - scrollVal, 172 );
		subNoiseToggle[i]->move( 147 + 250 - scrollVal, 229 );
		subPanningKnob[i]->move( 55 + 250 - scrollVal, 172 );
		subTempoKnob[i]->move( 30 + 250 - scrollVal, 0 );

		modInBox[i]->move( 43 + 750 - scrollVal, i*115+57 - modScrollVal );
		modInNumBox[i]->move( 88 + 750 - scrollVal, i*115+57 - modScrollVal );
		modInOtherNumBox[i]->move( 122 + 750 - scrollVal, i*115+57 - modScrollVal );
		modInAmntKnob[i]->move( 167 + 750 - scrollVal, i*115+53 - modScrollVal );
		modInCurveKnob[i]->move( 192 + 750 - scrollVal, i*115+53 - modScrollVal );
		modOutSecBox[i]->move( 27 + 750 - scrollVal, i*115+88 - modScrollVal );
		modOutSigBox[i]->move( 69 + 750 - scrollVal, i*115+88 - modScrollVal );
		modOutSecNumBox[i]->move( 112 + 750 - scrollVal, i*115+88 - modScrollVal );
		modInBox2[i]->move( 43 + 750 - scrollVal, i*115+118 - modScrollVal );
		modInNumBox2[i]->move( 88 + 750 - scrollVal, i*115+118 - modScrollVal );
		modInOtherNumBox2[i]->move( 122 + 750 - scrollVal, i*115+118 - modScrollVal );
		modInAmntKnob2[i]->move( 167 + 750 - scrollVal, i*115+114 - modScrollVal );
		modInCurveKnob2[i]->move( 192 + 750 - scrollVal, i*115+114 - modScrollVal );
		modEnabledToggle[i]->move( 27 + 750 - scrollVal, i*115+36 - modScrollVal );
		modCombineTypeBox[i]->move( 149 + 750 - scrollVal, i*115+88 - modScrollVal );
		modTypeToggle[i]->move( 195 + 750 - scrollVal, i*115+98 - modScrollVal );

		modUpArrow[i]->move( 181 + 750 - scrollVal, i*115+37 - modScrollVal );
		modDownArrow[i]->move( 199 + 750 - scrollVal, i*115+37 - modScrollVal );
	}
	
	
	visvolKnob->move( 0 - scrollVal, 150 );
	scrollKnob->move( 0, 220 );
	
	loadAlgKnob->move( 0 - scrollVal, 25 );
	loadChnlKnob->move( 0 - scrollVal, 0 );
	visualizeToggle->move( 213 - scrollVal, 26 );
	subNumBox->move( 250 + 18 - scrollVal, 219 );
	sampNumBox->move( 500 + 18 - scrollVal, 219 );
	mainNumBox->move( 18 - scrollVal, 219 );
	graph->move( scrollVal >= 500 ? 500 + 23 - scrollVal : 23 , 30 );
	tabChanged( castModel<Microwave>()->scroll.value() - 1 );
	updateGraphColor( castModel<Microwave>()->scroll.value() );
	openWavetableButton->move( 54 - scrollVal, 220 );
	openSampleButton->move( 54 + 500 - scrollVal, 220 );

	sinWaveBtn->move( 178 + 250 - scrollVal, 215 );
	triangleWaveBtn->move( 196 + 250 - scrollVal, 215 );
	sawWaveBtn->move( 214 + 250 - scrollVal, 215 );
	sqrWaveBtn->move( 178 + 250 - scrollVal, 230 );
	whiteNoiseWaveBtn->move( 196 + 250 - scrollVal, 230 );
	usrWaveBtn->move( 54 + 250 - scrollVal, 220 );
	smoothBtn->move( 214 + 250 - scrollVal, 230 );

	sinWave2Btn->move( 178 + 500 - scrollVal, 215 );
	triangleWave2Btn->move( 196 + 500 - scrollVal, 215 );
	sawWave2Btn->move( 214 + 500 - scrollVal, 215 );
	sqrWave2Btn->move( 178 + 500 - scrollVal, 230 );
	whiteNoiseWave2Btn->move( 196 + 500 - scrollVal, 230 );
	usrWave2Btn->move( 54 + 500 - scrollVal, 220 );
	smooth2Btn->move( 214 + 500 - scrollVal, 230 );

	oversampleBox->move( 0 + 500 - scrollVal, 0 );

	effectScrollBar->move( 221 + 1000 - scrollVal, 32 );

	matrixScrollBar->move( 221 + 750 - scrollVal, 32 );

	filtForegroundLabel->move( 1000 - scrollVal, 0 );
	filtBoxesLabel->move( 1000 + 24 - scrollVal, 35 - ( effectScrollVal % 92 ) );

	matrixForegroundLabel->move( 750 - scrollVal, 0 );
	matrixBoxesLabel->move( 750 + 24 - scrollVal, 35 - ( modScrollVal % 115 ) );

	QRect rect(scrollVal, 0, 250, 250);
	pal.setBrush( backgroundRole(), fullArtworkImg.copy(rect) );
	setPalette( pal );

}


// Snaps scroll to nearest tab when the scroll knob is released.
void MicrowaveView::scrollReleased()
{
	const float scrollVal = castModel<Microwave>()->scroll.value();
	castModel<Microwave>()->scroll.setValue( round(scrollVal-0.25) );
}


void MicrowaveView::mouseMoveEvent( QMouseEvent * _me )
{
	//castModel<Microwave>()->morph[0]->setValue(_me->x());
}


void MicrowaveView::wheelEvent( QWheelEvent * _me )
{
	if( _me->delta() < 0 )
	{
		const float scrollVal = castModel<Microwave>()->scroll.value();
		castModel<Microwave>()->scroll.setValue( scrollVal + 1 );
	}
	else if( _me->delta() > 0 )
	{
		const float scrollVal = castModel<Microwave>()->scroll.value();
		castModel<Microwave>()->scroll.setValue( scrollVal - 1 );
	}
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::mainNumChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		morphKnob[i]->hide();
		rangeKnob[i]->hide();
		sampLenKnob[i]->hide();
		morphMaxKnob[i]->hide();
		modifyKnob[i]->hide();
		unisonVoicesKnob[i]->hide();
		unisonDetuneKnob[i]->hide();
		unisonMorphKnob[i]->hide();
		unisonModifyKnob[i]->hide();
		detuneKnob[i]->hide();
		phaseKnob[i]->hide();
		phaseRandKnob[i]->hide();
		volKnob[i]->hide();
		enabledToggle[i]->hide();
		mutedToggle[i]->hide();
		modifyModeBox[i]->hide();
		panKnob[i]->hide();
		if( castModel<Microwave>()->mainNum.value() - 1 == i )
		{
			morphKnob[i]->show();
			rangeKnob[i]->show();
			sampLenKnob[i]->show();
			morphMaxKnob[i]->show();
			modifyKnob[i]->show();
			unisonVoicesKnob[i]->show();
			unisonDetuneKnob[i]->show();
			unisonMorphKnob[i]->show();
			unisonModifyKnob[i]->show();
			detuneKnob[i]->show();
			phaseKnob[i]->show();
			phaseRandKnob[i]->show();
			volKnob[i]->show();
			enabledToggle[i]->show();
			mutedToggle[i]->show();
			modifyModeBox[i]->show();
			panKnob[i]->show();
		}
	}
}


// Trades out the GUI elements when switching between oscillators, and adjusts graph length when needed
void MicrowaveView::subNumChanged()
{
	castModel<Microwave>()->graph.setLength( castModel<Microwave>()->subSampLen[castModel<Microwave>()->subNum.value()-1]->value() );
	for( int i = 0; i < 2048; ++i )
	{
		castModel<Microwave>()->graph.setSampleAt( i, castModel<Microwave>()->subs[(castModel<Microwave>()->subNum.value()-1)*2048+i] );
	}
	for( int i = 0; i < 64; ++i )
	{
		if( i != castModel<Microwave>()->subNum.value()-1 )
		{
			subEnabledToggle[i]->hide();
			subVolKnob[i]->hide();
			subPhaseKnob[i]->hide();
			subPhaseRandKnob[i]->hide();
			subDetuneKnob[i]->hide();
			subMutedToggle[i]->hide();
			subKeytrackToggle[i]->hide();
			subSampLenKnob[i]->hide();
			subNoiseToggle[i]->hide();
			subPanningKnob[i]->hide();
			subTempoKnob[i]->hide();
		}
		else
		{
			subEnabledToggle[i]->show();
			subVolKnob[i]->show();
			subPhaseKnob[i]->show();
			subPhaseRandKnob[i]->show();
			subDetuneKnob[i]->show();
			subMutedToggle[i]->show();
			subKeytrackToggle[i]->show();
			subSampLenKnob[i]->show();
			subNoiseToggle[i]->show();
			subPanningKnob[i]->show();
			subTempoKnob[i]->show();
		}
	}
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::sampNumChanged()
{
	for( int i = 0; i < 128; ++i )
	{
		castModel<Microwave>()->graph.setSampleAt( i, castModel<Microwave>()->sampGraphs[(castModel<Microwave>()->sampNum.value()-1)*128+i] );
	}
	for( int i = 0; i < 8; ++i )
	{
		if( i != castModel<Microwave>()->sampNum.value()-1 )
		{
			sampleEnabledToggle[i]->hide();
			sampleGraphEnabledToggle[i]->hide();
			sampleMutedToggle[i]->hide();
			sampleKeytrackingToggle[i]->hide();
			sampleLoopToggle[i]->hide();

			sampleVolumeKnob[i]->hide();
			samplePanningKnob[i]->hide();
			sampleDetuneKnob[i]->hide();
			samplePhaseKnob[i]->hide();
			samplePhaseRandKnob[i]->hide();
			sampleStartKnob[i]->hide();
			sampleEndKnob[i]->hide();
		}
		else
		{
			sampleEnabledToggle[i]->show();
			sampleGraphEnabledToggle[i]->show();
			sampleMutedToggle[i]->show();
			sampleKeytrackingToggle[i]->show();
			sampleLoopToggle[i]->show();

			sampleVolumeKnob[i]->show();
			samplePanningKnob[i]->show();
			sampleDetuneKnob[i]->show();
			samplePhaseKnob[i]->show();
			samplePhaseRandKnob[i]->show();
			sampleStartKnob[i]->show();
			sampleEndKnob[i]->show();
		}
	}
}


// Moves/changes the GUI around depending on the mod out section value
void MicrowaveView::modOutSecChanged( int i )
{
	switch( castModel<Microwave>()->modOutSec[i]->value() )
	{
		case 0:// None
			modOutSigBox[i]->hide();
			modOutSecNumBox[i]->hide();
			break;
		case 1:// Main OSC
			modOutSigBox[i]->show();
			modOutSecNumBox[i]->show();
			castModel<Microwave>()->modOutSig[i]->clear();
			mainoscsignalsmodel( castModel<Microwave>()->modOutSig[i] )
			castModel<Microwave>()->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		case 2:// Sub OSC
			modOutSigBox[i]->show();
			modOutSecNumBox[i]->show();
			castModel<Microwave>()->modOutSig[i]->clear();
			subsignalsmodel( castModel<Microwave>()->modOutSig[i] )
			castModel<Microwave>()->modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
			break;
		case 3:// Filter Input
			modOutSigBox[i]->show();
			modOutSecNumBox[i]->show();
			castModel<Microwave>()->modOutSig[i]->clear();
			mod8model( castModel<Microwave>()->modOutSig[i] )
			castModel<Microwave>()->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		case 4:// Filter Parameters
			modOutSigBox[i]->show();
			modOutSecNumBox[i]->show();
			castModel<Microwave>()->modOutSig[i]->clear();
			filtersignalsmodel( castModel<Microwave>()->modOutSig[i] )
			castModel<Microwave>()->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		case 5:// Matrix Parameters
			modOutSigBox[i]->show();
			modOutSecNumBox[i]->show();
			castModel<Microwave>()->modOutSig[i]->clear();
			matrixsignalsmodel( castModel<Microwave>()->modOutSig[i] )
			castModel<Microwave>()->modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
			break;
		case 6:// Sample OSC
			modOutSigBox[i]->show();
			modOutSecNumBox[i]->show();
			castModel<Microwave>()->modOutSig[i]->clear();
			samplesignalsmodel( castModel<Microwave>()->modOutSig[i] )
			castModel<Microwave>()->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
			break;
		default:
			break;
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modInChanged( int i )
{
	switch( castModel<Microwave>()->modIn[i]->value() )
	{
		case 0:
			modInNumBox[i]->hide();
			modInOtherNumBox[i]->hide();
			break;
		case 1:// Main OSC
			modInNumBox[i]->show();
			modInOtherNumBox[i]->hide();
			castModel<Microwave>()->modInNum[i]->setRange( 1, 8, 1 );
			break;
		case 2:// Sub OSC
			modInNumBox[i]->show();
			modInOtherNumBox[i]->hide();
			castModel<Microwave>()->modInNum[i]->setRange( 1, 64, 1 );
			break;
		case 3:// Filter
			modInNumBox[i]->show();
			modInOtherNumBox[i]->show();
			castModel<Microwave>()->modInNum[i]->setRange( 1, 8, 1 );
			castModel<Microwave>()->modInOtherNum[i]->setRange( 1, 8, 1 );
			break;
		case 4:// Sample OSC
			modInNumBox[i]->show();
			modInOtherNumBox[i]->hide();
			castModel<Microwave>()->modInNum[i]->setRange( 1, 8, 1 );
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
				castModel<Microwave>()->graph.setLength( 2048 );
				mainNumChanged();
				break;
			case 1:
				subNumChanged();
				break;
			case 2:
				castModel<Microwave>()->graph.setLength( 128 );
				sampNumChanged();
				break;
		}
	}
}



// This doesn't do anything right now.  I should probably delete it until I need it.
void MicrowaveView::visualizeToggled( bool value )
{
}


// Buttons that change the graph
void MicrowaveView::sinWaveClicked()
{
	graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}


void MicrowaveView::triangleWaveClicked()
{
	graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}


void MicrowaveView::sawWaveClicked()
{
	graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}


void MicrowaveView::sqrWaveClicked()
{
	graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}


void MicrowaveView::noiseWaveClicked()
{
	graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}


void MicrowaveView::usrWaveClicked()
{
	QString fileName = graph->model()->setWaveToUser();
	ToolTip::add( usrWaveBtn, fileName );
	Engine::getSong()->setModified();
}


void MicrowaveView::smoothClicked()
{
	graph->model()->smooth();
	Engine::getSong()->setModified();
}
// Buttons that change the graph



void MicrowaveView::modUpClicked( int i )
{
	if( i > 0 )
	{
		castModel<Microwave>()->switchMatrixSections( i, i - 1 );
	}
}


void MicrowaveView::modDownClicked( int i )
{
	if( i < 63 )
	{
		castModel<Microwave>()->switchMatrixSections( i, i + 1 );
	}
}



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
	graph->setPalette( pal );
}


// Calls MicrowaveView::openWavetableFile when the wavetable opening button is clicked.
void MicrowaveView::openWavetableFileBtnClicked( )
{
	openWavetableFile( castModel<Microwave>()->loadAlg.value(), castModel<Microwave>()->loadChnl.value() );
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
	int oscilNum = castModel<Microwave>()->mainNum.value() - 1;
	if( !fileName.isEmpty() )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		switch( algorithm )
		{
			case 0:// Squeeze entire sample into 256 waveforms
			{
				castModel<Microwave>()->morphMax[oscilNum]->setValue( 524288.f/castModel<Microwave>()->sampLen[oscilNum]->value() );
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
						castModel<Microwave>()->morphMax[oscilNum]->setValue( i/castModel<Microwave>()->sampLen[oscilNum]->value() );
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
						castModel<Microwave>()->morphMax[oscilNum]->setValue( i/castModel<Microwave>()->sampLen[oscilNum]->value() );
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
					if( fmod( i, castModel<Microwave>()->sampLen[oscilNum]->value() ) >= castModel<Microwave>()->sampLen[oscilNum]->value() - 200 )
					{
						float thing = ( -fmod(i, castModel<Microwave>()->sampLen[oscilNum]->value() ) + castModel<Microwave>()->sampLen[oscilNum]->value() ) / 200.f;
						castModel<Microwave>()->waveforms[oscilNum][i] = (castModel<Microwave>()->waveforms[oscilNum][i] * thing) + ((-thing+1) * castModel<Microwave>()->waveforms[oscilNum][int(i-(fmod( i, castModel<Microwave>()->sampLen[oscilNum]->value() )))]);
					}
				}
				break;
			}
			/*case 6:// Delete this.  Interpolates edges of waveforms.
			{
				for( int i = 0; i < 524288; ++i )
				{
					if( fmod( i, castModel<Microwave>()->sampLen[oscilNum]->value() ) <= 50 )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] *= fmod( i, castModel<Microwave>()->sampLen[oscilNum]->value() ) / 50.f;
					}
					if( fmod( i, castModel<Microwave>()->sampLen[oscilNum]->value() ) >= castModel<Microwave>()->sampLen[oscilNum]->value() - 50 )
					{
						castModel<Microwave>()->waveforms[oscilNum][i] *= ( -fmod(i, castModel<Microwave>()->sampLen[oscilNum]->value() ) + castModel<Microwave>()->sampLen[oscilNum]->value() ) / 50.f;
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
	int oscilNum = castModel<Microwave>()->sampNum.value() - 1;

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
	int oscilNum = castModel<Microwave>()->mainNum.value() - 1;
	int tabNum = castModel<Microwave>()->currentTab;
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		if( tabNum != 100000 )
		{
			openWavetableFile( castModel<Microwave>()->loadAlg.value(), castModel<Microwave>()->loadChnl.value(), value );
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
mSynth::mSynth( NotePlayHandle * _nph, const sample_rate_t _sample_rate, float * phaseRand, int * _modifyModeArr, float * _modifyArr, float * _morphArr, float * _rangeArr, float * _unisonVoicesArr, float * _unisonDetuneArr, float * _unisonMorphArr, float * _unisonModifyArr, float * _morphMaxArr, float * _detuneArr, float waveforms[8][524288], float * _subsArr, bool * _subEnabledArr, float * _subVolArr, float * _subPhaseArr, float * _subPhaseRandArr, float * _subDetuneArr, bool * _subMutedArr, bool * _subKeytrackArr, float * _subSampLenArr, bool * _subNoiseArr, int * _sampLenArr, int * _modInArr, int * _modInNumArr, int * _modInOtherNumArr, float * _modInAmntArr, float * _modInCurveArr, int * _modIn2Arr, int * _modInNum2Arr, int * _modInOtherNum2Arr, float * _modInAmnt2Arr, float * _modInCurve2Arr, int * _modOutSecArr, int * _modOutSigArr, int * _modOutSecNumArr, int * modCombineTypeArr, bool * modTypeArr, float * _phaseArr, float * _volArr, float * _filtInVolArr, int * _filtTypeArr, int * _filtSlopeArr, float * _filtCutoffArr, float * _filtResoArr, float * _filtGainArr, float * _filtSatuArr, float * _filtWetDryArr, float * _filtBalArr, float * _filtOutVolArr, bool * _filtEnabledArr, bool * _enabledArr, bool * _modEnabledArr, float * sampGraphs, bool * _mutedArr, bool * _sampleEnabledArr, bool * _sampleGraphEnabledArr, bool * _sampleMutedArr, bool * _sampleKeytrackingArr, bool * _sampleLoopArr, float * _sampleVolumeArr, float * _samplePanningArr, float * _sampleDetuneArr, float * _samplePhaseArr, float * _samplePhaseRandArr, std::vector<float> (&samples)[8][2], float * _filtFeedbackArr, float * _filtDetuneArr, bool * _filtKeytrackingArr, float * _subPanningArr, float * _sampleStartArr, float * _sampleEndArr, float * _panArr, float * _subTempoArr ) :
	nph( _nph ),
	sample_rate( _sample_rate )
{

	memcpy( modifyModeVal, _modifyModeArr, sizeof(int) * 8 );
	memcpy( modifyVal, _modifyArr, sizeof(float) * 8 );
	memcpy( morphVal, _morphArr, sizeof(float) * 8 );
	memcpy( rangeVal, _rangeArr, sizeof(float) * 8 );
	memcpy( unisonVoices, _unisonVoicesArr, sizeof(float) * 8 );
	memcpy( unisonDetune, _unisonDetuneArr, sizeof(float) * 8 );
	memcpy( unisonMorph, _unisonMorphArr, sizeof(float) * 8 );
	memcpy( unisonModify, _unisonModifyArr, sizeof(float) * 8 );
	memcpy( morphMaxVal, _morphMaxArr, sizeof(float) * 8 );
	memcpy( detuneVal, _detuneArr, sizeof(float) * 8 );
	memcpy( sampLen, _sampLenArr, sizeof(int) * 8 );
	memcpy( modIn, _modInArr, sizeof(int) * 64 );
	memcpy( modInNum, _modInNumArr, sizeof(int) * 64 );
	memcpy( modInOtherNum, _modInOtherNumArr, sizeof(int) * 64 );
	memcpy( modInAmnt, _modInAmntArr, sizeof(float) * 64 );
	memcpy( modInCurve, _modInCurveArr, sizeof(float) * 64 );
	memcpy( modIn2, _modIn2Arr, sizeof(int) * 64 );
	memcpy( modInNum2, _modInNum2Arr, sizeof(int) * 64 );
	memcpy( modInOtherNum2, _modInOtherNum2Arr, sizeof(int) * 64 );
	memcpy( modInAmnt2, _modInAmnt2Arr, sizeof(float) * 64 );
	memcpy( modInCurve2, _modInCurve2Arr, sizeof(float) * 64 );
	memcpy( modOutSec, _modOutSecArr, sizeof(int) * 64 );
	memcpy( modOutSig, _modOutSigArr, sizeof(int) * 64 );
	memcpy( modOutSecNum, _modOutSecNumArr, sizeof(int) * 64 );
	memcpy( modEnabled, _modEnabledArr, sizeof(bool) * 64 );
	memcpy( modCombineType, modCombineTypeArr, sizeof(int) * 64 );
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
	memcpy( pan, _panArr, sizeof(float) * 8 );
	memcpy( subTempo, _subTempoArr, sizeof(float) * 64 );
	memcpy( modType, modTypeArr, sizeof(bool) * 64 );

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
std::vector<float> mSynth::nextStringSample( float waveforms[8][524288], float * subs, float * sampGraphs, std::vector<float> (&samples)[8][2], int maxModEnabled, int maxSubEnabled, int maxSampleEnabled, int maxMainEnabled, int sample_rate )// "waveforms" here may cause CPU problem, check back later
{

	noteDuration = noteDuration + 1;

	sample_t sample[8][32] = {{0}};
	float resultsample[8][32] = {{0}};
	
	for( int l = 0; l < maxModEnabled; ++l )// maxModEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( modEnabled[l] )
		{
			switch( modIn[l] )
			{
				case 0:
					curModVal[0] = 0;
					curModVal[1] = 0;
					break;
				case 1:
					curModVal[0] = lastMainOscVal[modInNum[l]-1];
					curModVal[1] = curModVal[0];
					break;
				case 2:
					if( modType[l] )// If envelope
					{
						curModVal[0] = lastSubEnvVal[modInNum[l]-1][0];
						curModVal[1] = lastSubEnvVal[modInNum[l]-1][1];
					}
					else
					{
						curModVal[0] = lastSubVal[modInNum[l]-1][0];
						curModVal[1] = lastSubVal[modInNum[l]-1][1];
					}
					break;
				case 3:
					curModVal[0] = filtOutputs[modInNum[l]-1][0];
					curModVal[1] = filtOutputs[modInNum[l]-1][1];
					break;
				case 4:
					curModVal[0] = lastSampleVal[modInNum[l]-1][0];
					curModVal[1] = lastSampleVal[modInNum[l]-1][1];
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
					curModVal[0] = humanizer[modInNum[l]];
					curModVal[1] = humanizer[modInNum[l]];
					break;
				default: {}
			}
			switch( modIn2[l] )
			{
				case 0:
					curModVal2[0] = 0;
					curModVal2[1] = 0;
					break;
				case 1:
					curModVal2[0] = lastMainOscVal[modInNum2[l]-1];
					curModVal2[1] = curModVal2[0];
					break;
				case 2:
					curModVal2[0] = lastSubVal[modInNum[l]-1][0];
					curModVal2[1] = lastSubVal[modInNum[l]-1][1];
					break;
				case 3:
					curModVal2[0] = filtOutputs[modInNum2[l]-1][0];
					curModVal2[1] = filtOutputs[modInNum2[l]-1][1];
				case 4:
					curModVal2[0] = lastSampleVal[modInNum2[l]-1][0];
					curModVal2[1] = lastSampleVal[modInNum2[l]-1][1];
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
					curModVal2[0] = humanizer[modInNum2[l]];
					curModVal2[1] = humanizer[modInNum2[l]];
					break;
				default: {}
			}

			if( curModVal[0]  ) { curModVal[0]  *= modInAmnt[l]  / 100.f; }// "if" statements are there to prevent unnecessary division if the modulation isn't used.
			if( curModVal[1]  ) { curModVal[1]  *= modInAmnt[l]  / 100.f; }
			if( curModVal2[0] ) { curModVal2[0] *= modInAmnt2[l] / 100.f; }
			if( curModVal2[1] ) { curModVal2[1] *= modInAmnt2[l] / 100.f; }

			// Calculate curve
			if( modInCurve[l] != 100.f )// The "if" statement is there so unnecessary CPU isn't spent (pow is very expensive) if the curve knob isn't being used.
			{
				curModValCurve[0] = (curModVal[0] <= -1) ? curModVal[0] : pow( ( curModVal[0] + 1 ) / 2.f, 1.f / ( modInCurve[l] / 100.f ) );
				curModValCurve[1] = (curModVal[1] <= -1) ? curModVal[1] : pow( ( curModVal[1] + 1 ) / 2.f, 1.f / ( modInCurve[l] / 100.f ) );
			}
			else
			{
				curModValCurve[0] = curModVal[0];
				curModValCurve[1] = curModVal[1];
			}
			if( modInCurve2[l] != 100.f )
			{
				curModVal2Curve[0] = (curModVal2[0] <= -1) ? curModVal[0] : pow( ( curModVal2[0] + 1 ) / 2.f, 1.f / ( modInCurve2[l] / 100.f ) );
				curModVal2Curve[1] = (curModVal2[1] <= -1) ? curModVal[1] : pow( ( curModVal2[1] + 1 ) / 2.f, 1.f / ( modInCurve2[l] / 100.f ) );
			}
			else
			{
				curModVal2Curve[0] = curModVal2[0];
				curModVal2Curve[1] = curModVal2[1];
			}
			
			switch( modCombineType[l] )
			{
				case 0:// Add
					comboModVal[0] = curModValCurve[0] + curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] + curModVal2Curve[1];
					break;
				case 1:// Multiply
					comboModVal[0] = curModValCurve[0] * curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] * curModVal2Curve[1];
					break;
				default:
					comboModVal[0] = 0;
					comboModVal[1] = 0;
			}
			comboModValMono = ( comboModVal[0] + comboModVal[1] ) / 2.f - 1;// "-1" is because Bipolar modulation
			switch( modOutSec[l] )
			{
				case 0:
					break;
				case 1:// Main Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Send input to Morph
							morphVal[modOutSecNum[l]-1] = qBound( 0.f, morphVal[modOutSecNum[l]-1] + ( round((comboModValMono)*morphMaxVal[modOutSecNum[l]-1]) ), morphMaxVal[modOutSecNum[l]-1] );
							modValType.push_back( 1 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 2:// Send input to Range
							rangeVal[modOutSecNum[l]-1] = qMax( 0.f, rangeVal[modOutSecNum[l]-1] + comboModValMono * 16.f );
							modValType.push_back( 2 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 3:// Send input to Modify
							modifyVal[modOutSecNum[l]-1] = qMax( 0.f, modifyVal[modOutSecNum[l]-1] + comboModValMono * 2048.f );
							modValType.push_back( 3 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 4:// Send input to Pitch/Detune
							detuneVal[modOutSecNum[l]-1] = detuneVal[modOutSecNum[l]-1] + comboModValMono * 9600.f;
							modValType.push_back( 11 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 5:// Send input to Phase
							phase[modOutSecNum[l]-1] = fmod( phase[modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 13 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 6:// Send input to Volume
							vol[modOutSecNum[l]-1] = qMax( 0.f, vol[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 14 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						default:
						{
						}
					}
					break;
				}
				case 2:// Sub Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Send input to Pitch/Detune
							subDetune[modOutSecNum[l]-1] = subDetune[modOutSecNum[l]-1] + comboModValMono * 9600.f;
							modValType.push_back( 30 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 2:// Send input to Phase
							subPhase[modOutSecNum[l]-1] = fmod( subPhase[modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 28 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 3:// Send input to Volume
							subVol[modOutSecNum[l]-1] = qMax( 0.f, subVol[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 27 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						default:
						{
						}
					}
					break;
				}
				case 3:// Filter Input
				{
					filtInputs[modOutSig[l]][0] += curModVal[0];
					filtInputs[modOutSig[l]][1] += curModVal[1];
					break;
				}
				case 4:// Filter Parameters
				{
					switch( modOutSig[l] )
					{
						case 0:// None
							break;
						case 1:// Input Volume
							filtInVol[modOutSecNum[l]-1] = qMax( 0.f, filtInVol[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 15 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 2:// Filter Type
							filtType[modOutSecNum[l]-1] = qMax( 0, int(filtType[modOutSecNum[l]-1] + comboModValMono*7.f ) );
							modValType.push_back( 16 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 3:// Filter Slope
							filtSlope[modOutSecNum[l]-1] = qMax( 0, int(filtSlope[modOutSecNum[l]-1] + comboModValMono*8.f ) );
							modValType.push_back( 17 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
							break;
						case 4:// Cutoff Frequency
							filtCutoff[modOutSecNum[l]-1] = qBound( 20.f, filtCutoff[modOutSecNum[l]-1] + comboModValMono*19980.f, 21999.f );
							modValType.push_back( 18 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 5:// Resonance
							filtReso[modOutSecNum[l]-1] = qMax( 0.0001f, filtReso[modOutSecNum[l]-1] + comboModValMono*16.f );
							modValType.push_back( 19 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 6:// dbGain
							filtGain[modOutSecNum[l]-1] = filtGain[modOutSecNum[l]-1] + comboModValMono*64.f;
							modValType.push_back( 20 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 7:// Saturation
							filtSatu[modOutSecNum[l]-1] = qMax( 0.f, filtSatu[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 21 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 8:// Wet/Dry
							filtWetDry[modOutSecNum[l]-1] = qBound( 0.f, filtWetDry[modOutSecNum[l]-1] + comboModValMono*100.f, 100.f );
							modValType.push_back( 22 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 9:// Balance/Panning
							filtWetDry[modOutSecNum[l]-1] = qBound( -100.f, filtWetDry[modOutSecNum[l]-1] + comboModValMono*200.f, 100.f );
							modValType.push_back( 23 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 10:// Output Volume
							filtOutVol[modOutSecNum[l]-1] = qMax( 0.f, filtOutVol[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 24 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 11:// Feedback
							filtFeedback[modOutSecNum[l]-1] = qMax( 0.f, filtFeedback[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 69 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 12:// Detune
							filtDetune[modOutSecNum[l]-1] = qMax( 0.f, filtDetune[modOutSecNum[l]-1] + comboModValMono*9600.f );
							modValType.push_back( 70 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
					}
					break;
				}
				case 5:// Mod Parameters
				{
					switch( modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Mod In Amount
							modInAmnt[modOutSecNum[l]-1] = modInAmnt[modOutSecNum[l]-1] + comboModValMono*100.f;
							modValType.push_back( 38 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 2:// Mod In Curve
							modInCurve[modOutSecNum[l]-1] = qMax( 0.f, modInCurve[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 39 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
					}
					break;
				}
				case 6:// Sample Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
							break;
						case 1:// Send input to Pitch/Detune
							sampleDetune[modOutSecNum[l]-1] = sampleDetune[modOutSecNum[l]-1] + comboModValMono * 9600.f;
							modValType.push_back( 66 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 2:// Send input to Phase
							samplePhase[modOutSecNum[l]-1] = fmod( samplePhase[modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 67 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						case 3:// Send input to Volume
							sampleVolume[modOutSecNum[l]-1] = qMax( 0.f, sampleVolume[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 64 );
							modValNum.push_back( modOutSecNum[l]-1 );
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

	//============//
	//== FILTER ==//
	//============//

	for( int l = 0; l < 8; ++l )
	{
		if( filtEnabled[l] )
		{
			filtInputs[l][0] *= filtInVol[l] / 100.f;
			filtInputs[l][0] *= filtInVol[l] / 100.f;
			
			if( filtKeytracking[l] )
			{
				filtDelayBuf[l][0].resize( round( sample_rate / detuneWithCents( nph->frequency(), filtDetune[l] ) ) );
				filtDelayBuf[l][1].resize( round( sample_rate / detuneWithCents( nph->frequency(), filtDetune[l] ) ) );
			}
			else
			{
				filtDelayBuf[l][0].resize( round( sample_rate / detuneWithCents( 440.f, filtDetune[l] ) ) );
				filtDelayBuf[l][1].resize( round( sample_rate / detuneWithCents( 440.f, filtDetune[l] ) ) );
			}
			filtInputs[l][0] += filtDelayBuf[l][0].at(filtDelayBuf[l][0].size() - 1);
			filtInputs[l][1] += filtDelayBuf[l][1].at(filtDelayBuf[l][1].size() - 1);
			
			for( int m = 0; m < filtSlope[l] + 1; ++m )// m is the slope number.  So if m = 2, then the sound is going from a 24 db to a 36 db slope, for example.
			{
				if( m )
				{
					filtInputs[l][0] = filtOutputs[l][0];
					filtInputs[l][1] = filtOutputs[l][1];
				}
				float cutoff = filtCutoff[l];
				int mode = filtType[l];
				float reso = filtReso[l];
				float dbgain = filtGain[l];
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
					filtPrevSampOut[l][m][0][0] = (b0/a0)*filtInputs[l][0] + (b1/a0)*filtPrevSampIn[l][m][1][0] + (b2/a0)*filtPrevSampIn[l][m][2][0] - (a1/a0)*filtPrevSampOut[l][m][1][0] - (a2/a0)*filtPrevSampOut[l][m][2][0];// Left ear
					filtPrevSampOut[l][m][0][1] = (b0/a0)*filtInputs[l][1] + (b1/a0)*filtPrevSampIn[l][m][1][1] + (b2/a0)*filtPrevSampIn[l][m][2][1] - (a1/a0)*filtPrevSampOut[l][m][1][1] - (a2/a0)*filtPrevSampOut[l][m][2][1];// Right ear

					//Output results
					filtOutputs[l][0] = filtPrevSampOut[l][m][0][0] * ( filtOutVol[l] / 100.f );
					filtOutputs[l][1] = filtPrevSampOut[l][m][0][1] * ( filtOutVol[l] / 100.f );

				}
				else if( mode >= 8 )
				{
					switch( mode )
					{
						case 8:// Moog 24db Lowpass
							filtx[0] = filtInputs[l][0]-r*filty4[0];
							filtx[1] = filtInputs[l][1]-r*filty4[1];
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
							filtOutputs[l][0] = filty4[0] * ( filtOutVol[l] / 100.f );
							filtOutputs[l][1] = filty4[1] * ( filtOutVol[l] / 100.f );
							break;
					}

				}

				// Calculates Saturation.  It looks ugly, but the algorithm is just y = x ^ ( 1 - saturation );
				filtOutputs[l][0] = pow( abs( filtOutputs[l][0] ), 1 - ( filtSatu[l] / 100.f ) ) * ( filtOutputs[l][0] < 0 ? -1 : 1 );
				filtOutputs[l][1] = pow( abs( filtOutputs[l][1] ), 1 - ( filtSatu[l] / 100.f ) ) * ( filtOutputs[l][1] < 0 ? -1 : 1 );

				// Balance knob wet
				filtOutputs[l][0] *= filtBal[l] > 0 ? ( 100.f - filtBal[l] ) / 100.f : 1.f;
				filtOutputs[l][1] *= filtBal[l] < 0 ? ( 100.f + filtBal[l] ) / 100.f : 1.f;

				// Wet
				filtOutputs[l][0] *= filtWetDry[l] / 100.f;
				filtOutputs[l][1] *= filtWetDry[l] / 100.f;

				// Balance knob dry
				filtOutputs[l][0] += ( 1.f - ( filtBal[l] > 0 ? ( 100.f - filtBal[l] ) / 100.f : 1.f ) ) * filtInputs[l][0];
				filtOutputs[l][1] += ( 1.f - ( filtBal[l] < 0 ? ( 100.f + filtBal[l] ) / 100.f : 1.f ) ) * filtInputs[l][1];

				// Dry
				filtOutputs[l][0] += ( 1.f - ( filtWetDry[l] / 100.f ) ) * filtInputs[l][0];
				filtOutputs[l][1] += ( 1.f - ( filtWetDry[l] / 100.f ) ) * filtInputs[l][1];

				// Send back past samples
				// I'm sorry you had to read this
				filtPrevSampIn[l][m][4][0] = filtPrevSampIn[l][m][3][0];
				filtPrevSampIn[l][m][4][1] = filtPrevSampIn[l][m][3][1];

				filtPrevSampIn[l][m][3][0] = filtPrevSampIn[l][m][2][0];
				filtPrevSampIn[l][m][3][1] = filtPrevSampIn[l][m][2][1];

				filtPrevSampIn[l][m][2][0] = filtPrevSampIn[l][m][1][0];
				filtPrevSampIn[l][m][2][1] = filtPrevSampIn[l][m][1][1];
			
				filtPrevSampIn[l][m][1][0] = filtInputs[l][0];
				filtPrevSampIn[l][m][1][1] = filtInputs[l][1];

				filtPrevSampOut[l][m][4][0] = filtPrevSampOut[l][m][3][0];
				filtPrevSampOut[l][m][4][1] = filtPrevSampOut[l][m][3][1];

				filtPrevSampOut[l][m][3][0] = filtPrevSampOut[l][m][2][0];
				filtPrevSampOut[l][m][3][1] = filtPrevSampOut[l][m][2][1];
			
				filtPrevSampOut[l][m][2][0] = filtPrevSampOut[l][m][1][0];
				filtPrevSampOut[l][m][2][1] = filtPrevSampOut[l][m][1][1];
			
				filtPrevSampOut[l][m][1][0] = filtPrevSampOut[l][m][0][0];
				filtPrevSampOut[l][m][1][1] = filtPrevSampOut[l][m][0][1];
			}

			filtDelayBuf[l][0].insert( filtDelayBuf[l][0].begin(), filtOutputs[l][0] * filtFeedback[l] / 100.f );
			filtDelayBuf[l][1].insert( filtDelayBuf[l][1].begin(), filtOutputs[l][1] * filtFeedback[l] / 100.f );

			filtInputs[l][0] = 0;
			filtInputs[l][1] = 0;
		}
	}

	//=====================//
	//== MAIN OSCILLATOR ==//
	//=====================//

	for( int i = 0; i < maxMainEnabled; ++i )// maxMainEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		if( !enabled[i] )
		{
			continue;// Skip to next loop if oscillator is not enabled
		}
		for( int l = 0; l < unisonVoices[i]; ++l )
		{
			sample_morerealindex[i][l] = fmod( ( sample_realindex[i][l] + ( fmod( phase[i], 100.f ) * sampLen[i] * 0.01f ) ), sampLen[i] );// Calculates phase

			unisonVoicesMinusOne = unisonVoices[i] - 1;// unisonVoices[i] - 1 is needed many times, which is why unisonVoicesMinusOne exists
	
			noteFreq = unisonVoicesMinusOne ? detuneWithCents( nph->frequency(), unisonDetuneAmounts[i][l]*unisonDetune[i]+detuneVal[i] ) : detuneWithCents( nph->frequency(), detuneVal[i] );// Calculates frequency depending on detune and unison detune
	
			sample_step[i][l] = sampLen[i] * (  noteFreq / sample_rate );
	
			if( unisonVoicesMinusOne )// Figures out Morph and Modify values for individual unison voices
			{
				temp1 = (((unisonVoicesMinusOne-l)/unisonVoicesMinusOne)*unisonMorph[i]);
				morphVal[i] = qBound( 0.f, temp1 - ( unisonMorph[i] * 0.5f ) + morphVal[i], morphMaxVal[i] );
	
				temp1 = ((unisonVoicesMinusOne-l)/unisonVoicesMinusOne)*unisonModify[i];
				modifyVal[i] = qBound( 0.f, temp1 - ( unisonModify[i] * 0.5f ) + modifyVal[i], sampLen[i]-1.f);// SampleLength - 1 = ModifyMax
			}
	
			sample_length_modify[i][l] = 0;
			switch( modifyModeVal[i] )// Horrifying formulas for the various Modify Modes
			{
				case 0:// None
					break;
				case 1:// Pulse Width
					sample_morerealindex[i][l] /= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];

					sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sampLen[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds
					break;
				case 2:// Stretch Left Half
					if( sample_morerealindex[i][l] < ( sampLen[i] + sample_length_modify[i][l] ) / 2.f )
					{
						sample_morerealindex[i][l] -= (sample_morerealindex[i][l] * (modifyVal[i]/( sampLen[i] )));
					}
					break;
				case 3:// Shrink Right Half To Left Edge
					if( sample_morerealindex[i][l] > ( sampLen[i] + sample_length_modify[i][l] ) / 2.f )
					{
						sample_morerealindex[i][l] += (((sample_morerealindex[i][l])+sampLen[i]/2.f) * (modifyVal[i]/( sampLen[i] )));

						sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sampLen[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds
					}
					break;
				case 4:// Weird 1
					sample_morerealindex[i][l] = ( ( ( sin( ( ( ( sample_morerealindex[i][l] ) / ( sampLen[i] ) ) * ( modifyVal[i] / 50.f ) ) / 2 ) ) * ( sampLen[i] ) ) * ( modifyVal[i] / sampLen[i] ) + ( sample_morerealindex[i][l] + ( ( -modifyVal[i] + sampLen[i] ) / sampLen[i] ) ) ) / 2.f;
					break;
				case 5:// Asym To Right
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					break;
				case 6:// Asym To Left
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					break;
				case 7:// Weird 2
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
				case 8:// Stretch From Center
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= sampLen[i] / 2.f;
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] + 1 ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] + 1 ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				case 9:// Squish To Center
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sampLen[i] + 1  ) ) : -pow( -sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sampLen[i] + 1 ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				case 10:// Stretch And Squish
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				case 11:// Add Tail
					sample_length_modify[i][l] = modifyVal[i];
					dynamic_cast<Microwave *>(nph->instrumentTrack()->instrument())->graph.setLength( sampLen[i] + sample_length_modify[i][l] );
					break;
				case 12:// Cut Off Right
					sample_morerealindex[i][l] *= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					break;
				case 13:// Cut Off Left
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					sample_morerealindex[i][l] *= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					break;
				case 14:// Flip
					break;
				case 15:// Squarify
					break;
				case 16:// Pulsify
					break;
				default:
					{}
			}
			//sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sampLen[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds
			
			resultsample[i][l] = 0;

			temp1 = 524288.f / sampLen[i];
			loopStart = qMax( 0.f, morphVal[i] - rangeVal[i] ) + 1;
			loopEnd = qMin( morphVal[i] + rangeVal[i], temp1 ) + 1;
			currentRangeValInvert = 1.f / rangeVal[i];// Inverted to prevent excessive division in the loop below, for a minor CPU improvement.  Don't ask.
			currentSampLen = sampLen[i];
			currentIndex = sample_morerealindex[i][l];
			if( modifyModeVal[i] != 15 && modifyModeVal[i] != 16 )// If NOT Squarify or Pulsify Modify Mode
			{
				// Only grab samples from the waveforms when they will be used, depending on the Range knob
				for( int j = loopStart; j < loopEnd; ++j )
				{
					// Get waveform samples, set their volumes depending on Range knob
					resultsample[i][l] += waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert ));
				}
			}
			else if( modifyModeVal[i] == 15 )// If Squarify Modify Mode
			{
				// Self-made formula, may be buggy.  Crossfade one half of waveform with the inverse of the other.
				// Some major CPU improvements can be made here.
				for( int j = loopStart; j < loopEnd; ++j )
				{
					resultsample[i][l] +=
					(    waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) + // Normal
					( ( -waveforms[i][( ( currentIndex + ( currentSampLen / 2 ) ) % currentSampLen ) + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) * ( ( modifyVal[i] * 2.f ) / currentSampLen ) ) / // Inverted other half of waveform
					( ( modifyVal[i] / currentSampLen ) + 1 ); // Volume compensation
				}
			}
			else if( modifyModeVal[i] == 16 )// Pulsify Mode
			{
				// Self-made formula, may be buggy.  Crossfade one side of waveform with the inverse of the other.
				// Some major CPU improvements can be made here.
				for( int j = loopStart; j < loopEnd; ++j )
				{
					resultsample[i][l] +=
					(    waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) + // Normal
					( ( -waveforms[i][( ( currentIndex + int( currentSampLen * ( modifyVal[i] / currentSampLen ) ) ) % currentSampLen ) + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) * 2.f ) / // Inverted other side of waveform
					2.f; // Volume compensation
				}
			}

			resultsample[i][l] /= rangeVal[i];// Decreases volume so Range value doesn't make things too loud
	
			switch( modifyModeVal[i] )// More horrifying formulas for the various Modify Modes
			{
				case 1:// Pulse Width
					if( sample_realindex[i][l] / ( ( -modifyVal[i] + sampLen[i] ) / sampLen[i] ) > sampLen[i] )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 3:// Shrink Right Half To Left Edge
					if( sample_realindex[i][l] + (((sample_realindex[i][l])+sampLen[i]/2.f) * (modifyVal[i]/( sampLen[i] ))) > sampLen[i]  )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 11:// Add Tail
					if( sample_morerealindex[i][l] > sampLen[i] )
					{
						resultsample[i][l] = 0;
					}
					break;
				case 14:// Flip
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
			temp1 = sampLen[i] + sample_length_modify[i][l];
			if( unlikely( sample_realindex[i][l] >= temp1 ) )
			{
				sample_realindex[i][l] -= temp1;
			}
	
			sample[i][l] = resultsample[i][l] * vol[i] * 0.01f;
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
				if( subTempo[l] )
				{
					noteFreq = subKeytrack[l] ? detuneWithCents( nph->frequency(), subDetune[l] ) * ( subTempo[l] / 26400.f ) : detuneWithCents( 440.f, subDetune[l] ) * ( subTempo[l] / 26400.f );
				}
				else
				{
					noteFreq = subKeytrack[l] ? detuneWithCents( nph->frequency(), subDetune[l] ) : detuneWithCents( 440.f, subDetune[l] );
				}
				sample_step_sub = subSampLen[l] / ( sample_rate / noteFreq );
	
				subsample[l][0] = ( subVol[l] / 100.f ) * subs[int( ( int( sample_subindex[l] + ( subPhase[l] * subSampLen[l] ) ) % int(subSampLen[l]) ) + ( 2048 * l ) )];
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
		
				lastSubVal[l][0] = subsample[l][0];// Store value for matrix
				lastSubVal[l][1] = subsample[l][1];

				if( !lastSubEnvDone[l] )
				{
					lastSubEnvVal[l][0] = subsample[l][0];// Store envelope value for matrix
					lastSubEnvVal[l][1] = subsample[l][1];
				}
	
				subsample[l][0] = subMuted[l] ? 0 : subsample[l][0];// Mutes sub after saving value for modulation if the muted option is on
				subsample[l][1] = subMuted[l] ? 0 : subsample[l][1];// Mutes sub after saving value for modulation if the muted option is on
		
				sample_subindex[l] += sample_step_sub;
		
				// move waveform position back if it passed the end of the waveform
				while ( sample_subindex[l] >= subSampLen[l] )
				{
					sample_subindex[l] -= subSampLen[l];
					lastSubEnvDone[l] = true;
				}
			}
			else// sub oscillator is noise
			{
				noiseSampRand = fastRandf( subSampLen[l] ) - 1;
				subsample[l][0] = qBound( -1.f, ( lastSubVal[l][0]+(subs[int(noiseSampRand)] / 8.f) ) / 1.2f, 1.f ) * ( subVol[l] / 100.f );// Division by 1.2f to tame DC offset
				subsample[l][1] = subsample[l][0];
				lastSubVal[l][0] = subsample[l][0];
				lastSubVal[l][1] = subsample[l][1];
				lastSubEnvVal[l][0] = subsample[l][0];// Store envelope value for matrix
				lastSubEnvVal[l][1] = subsample[l][1];
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
				sample_step_sample = ( detuneWithCents( nph->frequency(), sampleDetune[l] ) / 440.f ) * ( 44100.f / sample_rate );
			}
			else
			{
				sample_step_sample = 44100.f / sample_rate;
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
	for( int i = 0; i < maxMainEnabled; ++i )
	{
		if( enabled[i] )
		{
			lastMainOscVal[i] = sample[i][0];// Store results for modulations
			if( !muted[i] )
			{
				if( unisonVoices[i] > 1 )
				{
					sampleMainOsc[0] = 0;
					sampleMainOsc[1] = 0;
					for( int j = 0; j < unisonVoices[i]; ++j )
					{
						// Pan unison voices
						sampleMainOsc[0] += sample[i][j] * ((unisonVoicesMinusOne-j)/unisonVoicesMinusOne);
						sampleMainOsc[1] += sample[i][j] * (j/unisonVoicesMinusOne);
					}
					// Decrease volume so more unison voices won't increase volume too much
					temp1 = unisonVoices[i] / 2.f;
					sampleMainOsc[0] /= temp1;
					sampleMainOsc[1] /= temp1;
					
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
			sampleAvg[0] += filtOutputs[l][0];
			sampleAvg[1] += filtOutputs[l][1];
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
		case 1: morphVal[num] = mwc->morph[num]->value(); break;
		case 2: rangeVal[num] = mwc->range[num]->value(); break;
		case 3: modifyVal[num] = mwc->modify[num]->value(); break;
		case 4: modifyModeVal[num] = mwc->modifyMode[num]->value(); break;
		case 6: unisonVoices[num] = mwc->unisonVoices[num]->value(); break;
		case 7: unisonDetune[num] = mwc->unisonDetune[num]->value(); break;
		case 8: unisonMorph[num] = mwc->unisonMorph[num]->value(); break;
		case 9: unisonModify[num] = mwc->unisonModify[num]->value(); break;
		case 10: morphMaxVal[num] = mwc->morphMax[num]->value(); break;
		case 11: detuneVal[num] = mwc->detune[num]->value(); break;
		case 12: sampLen[num] = mwc->sampLen[num]->value(); break;
		case 13: phase[num] = mwc->phase[num]->value(); break;
		case 14: vol[num] = mwc->vol[num]->value(); break;
		case 15: filtInVol[num] = mwc->filtInVol[num]->value(); break;
		case 16: filtType[num] = mwc->filtType[num]->value(); break;
		case 17: filtSlope[num] = mwc->filtSlope[num]->value(); break;
		case 18: filtCutoff[num] = mwc->filtCutoff[num]->value(); break;
		case 19: filtReso[num] = mwc->filtReso[num]->value(); break;
		case 20: filtGain[num] = mwc->filtGain[num]->value(); break;
		case 21: filtSatu[num] = mwc->filtSatu[num]->value(); break;
		case 22: filtWetDry[num] = mwc->filtWetDry[num]->value(); break;
		case 23: filtBal[num] = mwc->filtBal[num]->value(); break;
		case 24: filtOutVol[num] = mwc->filtOutVol[num]->value(); break;
		case 25: filtEnabled[num] = mwc->filtEnabled[num]->value(); break;
		case 26: subEnabled[num] = mwc->subEnabled[num]->value(); break;
		case 27: subVol[num] = mwc->subVol[num]->value(); break;
		case 28: subPhase[num] = mwc->subPhase[num]->value(); break;
		case 29: subPhaseRand[num] = mwc->subPhaseRand[num]->value(); break;
		case 30: subDetune[num] = mwc->subDetune[num]->value(); break;
		case 31: subMuted[num] = mwc->subMuted[num]->value(); break;
		case 32: subKeytrack[num] = mwc->subKeytrack[num]->value(); break;
		case 33: subSampLen[num] = mwc->subSampLen[num]->value(); break;
		case 34: subNoise[num] = mwc->subNoise[num]->value(); break;
		case 35: modIn[num] = mwc->modIn[num]->value(); break;
		case 36: modInNum[num] = mwc->modInNum[num]->value(); break;
		case 37: modInOtherNum[num] = mwc->modInOtherNum[num]->value(); break;
		case 38: modInAmnt[num] = mwc->modInAmnt[num]->value(); break;
		case 39: modInCurve[num] = mwc->modInCurve[num]->value(); break;
		case 40: modIn2[num] = mwc->modIn2[num]->value(); break;
		case 41: modInNum2[num] = mwc->modInNum2[num]->value(); break;
		case 42: modInOtherNum2[num] = mwc->modInOtherNum2[num]->value(); break;
		case 43: modInAmnt2[num] = mwc->modInAmnt2[num]->value(); break;
		case 44: modInCurve2[num] = mwc->modInCurve2[num]->value(); break;
		case 45: modOutSec[num] = mwc->modOutSec[num]->value(); break;
		case 46: modOutSig[num] = mwc->modOutSig[num]->value(); break;
		case 47: modOutSecNum[num] = mwc->modOutSecNum[num]->value(); break;
		case 48: enabled[num] = mwc->enabled[num]->value(); break;
		case 49: modEnabled[num] = mwc->modEnabled[num]->value(); break;
		case 50: modCombineType[num] = mwc->modCombineType[num]->value(); break;
		case 57: muted[num] = mwc->muted[num]->value(); break;
		case 59: sampleEnabled[num] = mwc->sampleEnabled[num]->value(); break;
		case 60: sampleGraphEnabled[num] = mwc->sampleGraphEnabled[num]->value(); break;
		case 61: sampleMuted[num] = mwc->sampleMuted[num]->value(); break;
		case 62: sampleKeytracking[num] = mwc->sampleKeytracking[num]->value(); break;
		case 63: sampleLoop[num] = mwc->sampleLoop[num]->value(); break;
		case 64: sampleVolume[num] = mwc->sampleVolume[num]->value(); break;
		case 65: samplePanning[num] = mwc->samplePanning[num]->value(); break;
		case 66: sampleDetune[num] = mwc->sampleDetune[num]->value(); break;
		case 67: samplePhase[num] = mwc->samplePhase[num]->value(); break;
		case 68: samplePhaseRand[num] = mwc->samplePhaseRand[num]->value(); break;
		case 69: filtFeedback[num] = mwc->filtFeedback[num]->value(); break;
		case 70: filtDetune[num] = mwc->filtDetune[num]->value(); break;
		case 71: filtKeytracking[num] = mwc->filtKeytracking[num]->value(); break;
		case 72: subPanning[num] = mwc->subPanning[num]->value(); break;
		case 73: sampleStart[num] = mwc->sampleStart[num]->value(); break;
		case 74: sampleEnd[num] = mwc->sampleEnd[num]->value(); break;
		case 75: pan[num] = mwc->pan[num]->value(); break;
		case 76: subTempo[num] = mwc->subTempo[num]->value(); break;
		case 77: modType[num] = mwc->modType[num]->value(); break;
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


