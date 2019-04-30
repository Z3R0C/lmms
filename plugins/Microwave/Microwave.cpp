/*
 * Microwave.cpp - morbidly advanced and versatile modular wavetable synthesizer
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
#include "GuiApplication.h"
#include "MainWindow.h"






















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

                       (Do not scroll down if you value your sanity)

*/




// Create Microwave.  Create value models (and set defaults, maximums, minimums, and step sizes).  Connect events to functions.
Microwave::Microwave( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &microwave_plugin_descriptor ),
	visvol( 100, 0, 1000, 0.01f, this, tr( "Visualizer Volume" ) ),
	loadChnl( 0, 0, 1, 1, this, tr( "Wavetable Loading Channel" ) ),
	scroll( 1, 1, 7, 0.0001f, this, tr( "Scroll" ) ),
	subNum(1, 1, 64, this, tr( "Sub Oscillator Number" ) ),
	sampNum(1, 1, 8, this, tr( "Sample Number" ) ),
	mainNum(1, 1, 8, this, tr( "Main Oscillator Number" ) ),
	oversample( this, tr("Oversampling") ),
	loadMode( this, tr("Wavetable Loading Algorithm") ),
	graph( -1.0f, 1.0f, 2048, this ),
	visualize( false, this ),
	wtLoad1( 0, 0, 3000, 1, this, tr( "Wavetable Loading Knob 1" ) ),
	wtLoad2( 0, 0, 3000, 1, this, tr( "Wavetable Loading Knob 2" ) ),
	wtLoad3( 0, 0, 3000, 1, this, tr( "Wavetable Loading Knob 3" ) ),
	wtLoad4( 0, 0, 3000, 1, this, tr( "Wavetable Loading Knob 4" ) )
{

	for( int i = 0; i < 8; ++i )
	{
		morph[i] = new FloatModel( 0, 0, 254, 0.0001f, this, tr( "Morph" ) );
		range[i] = new FloatModel( 1, 1, 16, 0.0001f, this, tr( "Range" ) );
		sampLen[i] = new FloatModel( 2048, 1, 8192, 1.f, this, tr( "Waveform Sample Length" ) );
		morphMax[i] = new FloatModel( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) );
		modifyMode[i] = new ComboBoxModel( this, tr( "Wavetable Modifier Mode" ) );
		modify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Wavetable Modifier Value" ) );
		unisonVoices[i] = new FloatModel( 1, 1, 32, 1, this, tr( "Unison Voices" ) );
		unisonDetune[i] = new FloatModel( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) );
		unisonDetune[i]->setScaleLogarithmic( true );
		unisonMorph[i] = new FloatModel( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) );
		unisonModify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Unison Modify" ) );
		detune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
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
		filtDetune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
		filtKeytracking[i] = new BoolModel( true, this );
		filtMuted[i] = new BoolModel( false, this );
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

		macro[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Macro" ) );
	}

	for( int i = 0; i < 64; ++i )
 	{
		subEnabled[i] = new BoolModel( false, this );
		subVol[i] = new FloatModel( 100.f, 0.f, 200.f, 0.0001f, this, tr( "Volume" ) );
		subPhase[i] = new FloatModel( 0.f, 0.f, 200.f, 0.0001f, this, tr( "Phase" ) );
		subPhaseRand[i] = new FloatModel( 0.f, 0.f, 100.f, 0.0001f, this, tr( "Phase Randomness" ) );
		subDetune[i] = new FloatModel( 0.f, -9600.f, 9600.f, 0.0001f, this, tr( "Detune" ) );
		subMuted[i] = new BoolModel( true, this );
		subKeytrack[i] = new BoolModel( true, this );
		subSampLen[i] = new FloatModel( 2048.f, 1.f, 2048.f, 1.f, this, tr( "Sample Length" ) );
		subNoise[i] = new BoolModel( false, this );
		subPanning[i] = new FloatModel( 0.f, -100.f, 100.f, 0.0001f, this, tr( "Panning" ) );
		subTempo[i] = new FloatModel( 0.f, 0.f, 999.f, 1.f, this, tr( "Tempo" ) );
		subRateLimit[i] = new FloatModel( 0.f, 0.f, 1.f, 0.000001f, this, tr( "Rate Limit" ) );
		subRateLimit[i]->setScaleLogarithmic( true );

		modEnabled[i] = new BoolModel( false, this );

		modOutSec[i] = new ComboBoxModel( this, tr( "Modulation Section" ) );
		modOutSig[i] = new ComboBoxModel( this, tr( "Modulation Signal" ) );
		modOutSecNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulation Section Number" ) );
		modsectionsmodel( modOutSec[i] )
		mainoscsignalsmodel( modOutSig[i] )

		modIn[i] = new ComboBoxModel( this, tr( "Modulator" ) );
		modInNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		modinmodel( modIn[i] )

		modInAmnt[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Modulator Amount" ) );
		modInCurve[i] = new FloatModel( 100, 10.f, 600, 0.0001f, this, tr( "Modulator Curve" ) );
		modInCurve[i]->setScaleLogarithmic( true );

		modIn2[i] = new ComboBoxModel( this, tr( "Secondary Modulator" ) );
		modInNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		modinmodel( modIn2[i] )

		modInAmnt2[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Secondary Modulator Amount" ) );
		modInCurve2[i] = new FloatModel( 100, 10.f, 600, 0.0001f, this, tr( "Secondary Modulator Curve" ) );
		modInCurve2[i]->setScaleLogarithmic( true );

		modCombineType[i] = new ComboBoxModel( this, tr( "Combination Type" ) );
		modcombinetypemodel( modCombineType[i] )

		modType[i] = new BoolModel( false, this );
		modType2[i] = new BoolModel( false, this );
	}

	oversamplemodel( oversample )
	oversample.setValue( 1 );// 2x oversampling is default

	loadmodemodel( loadMode )


	graph.setWaveToSine();

	for( int i = 0; i < 8; ++i )
	{
		connect( morphMax[i], SIGNAL( dataChanged( ) ), this, SLOT( morphMaxChanged( ) ), Qt::DirectConnection );
	}

	connect( &graph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( samplesChanged( int, int ) ) );

	for( int i = 0; i < 8; ++i )
	{
		connect( morph[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(1, i); }, Qt::DirectConnection );
		connect( range[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(2, i); }, Qt::DirectConnection );
		connect( modify[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(3, i); }, Qt::DirectConnection );
		connect( modifyMode[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(4, i); }, Qt::DirectConnection );
		connect( unisonVoices[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(6, i); }, Qt::DirectConnection );
		connect( unisonDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(7, i); }, Qt::DirectConnection );
		connect( unisonMorph[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(8, i); }, Qt::DirectConnection );
		connect( unisonModify[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(9, i); }, Qt::DirectConnection );
		connect( morphMax[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(10, i); }, Qt::DirectConnection );
		connect( detune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(11, i); }, Qt::DirectConnection );
		connect( sampLen[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(12, i); }, Qt::DirectConnection );
		connect( phase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(13, i); }, Qt::DirectConnection );
		connect( phaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(80, i); }, Qt::DirectConnection );
		connect( vol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(14, i); }, Qt::DirectConnection );
		connect( muted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(57, i); }, Qt::DirectConnection );

		connect( filtInVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(15, i); }, Qt::DirectConnection );
		connect( filtType[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(16, i); }, Qt::DirectConnection );
		connect( filtSlope[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(17, i); }, Qt::DirectConnection );
		connect( filtCutoff[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(18, i); }, Qt::DirectConnection );
		connect( filtReso[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(19, i); }, Qt::DirectConnection );
		connect( filtGain[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(20, i); }, Qt::DirectConnection );
		connect( filtSatu[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(21, i); }, Qt::DirectConnection );
		connect( filtWetDry[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(22, i); }, Qt::DirectConnection );
		connect( filtBal[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(23, i); }, Qt::DirectConnection );
		connect( filtOutVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(24, i); }, Qt::DirectConnection );
		connect( filtEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(25, i); }, Qt::DirectConnection );
		connect( filtFeedback[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(69, i); }, Qt::DirectConnection );
		connect( filtDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(70, i); }, Qt::DirectConnection );
		connect( filtKeytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(71, i); }, Qt::DirectConnection );
		connect( filtMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(79, i); }, Qt::DirectConnection );

		connect( enabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(48, i); }, Qt::DirectConnection );

		connect( sampleEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(59, i); }, Qt::DirectConnection );

		connect( sampleGraphEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(60, i); }, Qt::DirectConnection );

		connect( sampleMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(61, i); }, Qt::DirectConnection );

		connect( sampleKeytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(62, i); }, Qt::DirectConnection );

		connect( sampleLoop[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(63, i); }, Qt::DirectConnection );

		connect( sampleVolume[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(64, i); }, Qt::DirectConnection );

		connect( samplePanning[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(65, i); }, Qt::DirectConnection );

		connect( sampleDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(66, i); }, Qt::DirectConnection );

		connect( samplePhase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(67, i); }, Qt::DirectConnection );

		connect( samplePhaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(68, i); }, Qt::DirectConnection );

		connect( sampleStart[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(73, i); }, Qt::DirectConnection );

		connect( sampleEnd[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(74, i); }, Qt::DirectConnection );

		connect( pan[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(75, i); }, Qt::DirectConnection );

		connect( macro[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(78, i); }, Qt::DirectConnection );

		for( int j = 1; j <= 25; ++j )
		{
			valueChanged(j, i);
		}

		valueChanged(48, i);

		for( int j = 51; j <= 71; ++j )
		{
			valueChanged(j, i);
		}

		valueChanged(78, i);
		valueChanged(79, i);
		valueChanged(80, i);

		connect( sampleEnabled[i], &BoolModel::dataChanged, this, [this, i]() { sampleEnabledChanged(i); }, Qt::DirectConnection );

		connect( enabled[i], &BoolModel::dataChanged, this, [this, i]() { mainEnabledChanged(i); }, Qt::DirectConnection );

		connect( filtEnabled[i], &BoolModel::dataChanged, this, [this, i]() { filtEnabledChanged(i); }, Qt::DirectConnection );
	}

	for( int i = 0; i < 64; ++i )
	{
		connect( subEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(26, i); }, Qt::DirectConnection );
		connect( subVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(27, i); }, Qt::DirectConnection );
		connect( subPhase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(28, i); }, Qt::DirectConnection );
		connect( subPhaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(29, i); }, Qt::DirectConnection );
		connect( subDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(30, i); }, Qt::DirectConnection );
		connect( subMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(31, i); }, Qt::DirectConnection );
		connect( subKeytrack[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(32, i); }, Qt::DirectConnection );
		connect( subSampLen[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(33, i); }, Qt::DirectConnection );
		connect( subNoise[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(34, i); }, Qt::DirectConnection );
		connect( subPanning[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(72, i); }, Qt::DirectConnection );
		connect( subTempo[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(76, i); }, Qt::DirectConnection );
		connect( subRateLimit[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(82, i); }, Qt::DirectConnection );
		for( int j = 26; j <= 35; ++j )
		{
			valueChanged(j, i);
		}

		connect( subSampLen[i], &FloatModel::dataChanged, this, [this, i]() { subSampLenChanged(i); } );
		connect( subEnabled[i], &BoolModel::dataChanged, this, [this, i]() { subEnabledChanged(i); } );

		connect( modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(35, i); }, Qt::DirectConnection );
		connect( modInNum[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(36, i); }, Qt::DirectConnection );
		connect( modInAmnt[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(38, i); }, Qt::DirectConnection );
		connect( modInCurve[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(39, i); }, Qt::DirectConnection );
		connect( modIn2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(40, i); }, Qt::DirectConnection );
		connect( modInNum2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(41, i); }, Qt::DirectConnection );
		connect( modInAmnt2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(43, i); }, Qt::DirectConnection );
		connect( modInCurve2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(44, i); }, Qt::DirectConnection );
		connect( modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(45, i); }, Qt::DirectConnection );
		connect( modOutSig[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(46, i); }, Qt::DirectConnection );
		connect( modOutSecNum[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(47, i); }, Qt::DirectConnection );
		connect( modEnabled[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(49, i); }, Qt::DirectConnection );
		connect( modCombineType[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(50, i); }, Qt::DirectConnection );
		connect( modType[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(77, i); }, Qt::DirectConnection );
		connect( modType2[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(81, i); }, Qt::DirectConnection );
		for( int j = 35; j <= 47; ++j )
		{
			valueChanged(j, i);
		}
		valueChanged(49, i);
		valueChanged(50, i);
		valueChanged(72, i);
		valueChanged(76, i);
		valueChanged(77, i);
		valueChanged(81, i);
		valueChanged(82, i);

		connect( modEnabled[i], &BoolModel::dataChanged, this, [this, i]() { modEnabledChanged(i); }, Qt::DirectConnection );
	}

	for( int i = 0; i < 8; ++i )
	{
		samples[i][0].push_back(0);
		samples[i][1].push_back(0);
	}
}


Microwave::~Microwave()
{
	for( int i = 0; i < 64; ++i )
	{
		/*The following disconnected functions will run if not disconnected upon deletion,
		because deleting a ComboBox includes clearing its contents first,
		which will fire a dataChanged event.  So, we need to disconnect them to
		prevent a crash.*/
		disconnect(modIn[i], &ComboBoxModel::dataChanged, 0, 0);
		disconnect(modOutSec[i], &ComboBoxModel::dataChanged, 0, 0);
	}
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
	_this.setAttribute( "version", "Microwave Testing Release 4" );

	/*

	VERSION LIST:

	- 0.9: Every version before Microwave Testing Release 4 was mistakenly listed as 0.9.
	- Microwave Testing Release 4

	*/

	visvol.saveSettings( _doc, _this, "visualizervolume" );
	loadMode.saveSettings( _doc, _this, "loadingalgorithm" );
	loadChnl.saveSettings( _doc, _this, "loadingchannel" );

	oversample.saveSettings( _doc, _this, "oversample" );

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
			phaseRand[i]->saveSettings( _doc, _this, "phaseRand_"+QString::number(i) );
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

	for( int i = 0; i < maxFiltEnabled; ++i )
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
			filtMuted[i]->saveSettings( _doc, _this, "filtMuted_"+QString::number(i) );
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
			subRateLimit[i]->saveSettings( _doc, _this, "subRateLimit_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxModEnabled; ++i )
	{
		if( modEnabled[i]->value() )
		{
			modIn[i]->saveSettings( _doc, _this, "modIn_"+QString::number(i) );
			modInNum[i]->saveSettings( _doc, _this, "modInNu"+QString::number(i) );
			modInAmnt[i]->saveSettings( _doc, _this, "modInAmnt_"+QString::number(i) );
			modInCurve[i]->saveSettings( _doc, _this, "modInCurve_"+QString::number(i) );
			modIn2[i]->saveSettings( _doc, _this, "modIn2_"+QString::number(i) );
			modInNum2[i]->saveSettings( _doc, _this, "modInNum2_"+QString::number(i) );
			modInAmnt2[i]->saveSettings( _doc, _this, "modAmnt2_"+QString::number(i) );
			modInCurve2[i]->saveSettings( _doc, _this, "modCurve2_"+QString::number(i) );
			modOutSec[i]->saveSettings( _doc, _this, "modOutSec_"+QString::number(i) );
			modOutSig[i]->saveSettings( _doc, _this, "modOutSig_"+QString::number(i) );
			modOutSecNum[i]->saveSettings( _doc, _this, "modOutSecNu"+QString::number(i) );
			modEnabled[i]->saveSettings( _doc, _this, "modEnabled_"+QString::number(i) );
			modCombineType[i]->saveSettings( _doc, _this, "modCombineType_"+QString::number(i) );
			modType[i]->saveSettings( _doc, _this, "modType_"+QString::number(i) );
			modType2[i]->saveSettings( _doc, _this, "modType2_"+QString::number(i) );
		}
	}

	for( int i = 0; i < 8; ++i )
	{
		macro[i]->saveSettings( _doc, _this, "macro_"+QString::number(i) );
	}
}


void Microwave::loadSettings( const QDomElement & _this )
{
	visvol.loadSettings( _this, "visualizervolume" );
	loadMode.loadSettings( _this, "loadingalgorithm" );
	loadChnl.loadSettings( _this, "loadingchannel" );

	oversample.loadSettings( _this, "oversample" );

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
			phaseRand[i]->loadSettings( _this, "phaseRand_"+QString::number(i) );
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
			filtMuted[i]->loadSettings( _this, "filtMuted_"+QString::number(i) );
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

		macro[i]->loadSettings( _this, "macro_"+QString::number(i) );
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
			subRateLimit[i]->loadSettings( _this, "subRateLimit_"+QString::number(i) );
		}

		modEnabled[i]->loadSettings( _this, "modEnabled_"+QString::number(i) );
		if( modEnabled[i]->value() )
		{
			modIn[i]->loadSettings( _this, "modIn_"+QString::number(i) );
			modInNum[i]->loadSettings( _this, "modInNu"+QString::number(i) );
			modInAmnt[i]->loadSettings( _this, "modInAmnt_"+QString::number(i) );
			modInCurve[i]->loadSettings( _this, "modInCurve_"+QString::number(i) );
			modIn2[i]->loadSettings( _this, "modIn2_"+QString::number(i) );
			modInNum2[i]->loadSettings( _this, "modInNum2_"+QString::number(i) );
			modInAmnt2[i]->loadSettings( _this, "modAmnt2_"+QString::number(i) );
			modInCurve2[i]->loadSettings( _this, "modCurve2_"+QString::number(i) );
			modOutSec[i]->loadSettings( _this, "modOutSec_"+QString::number(i) );
			modOutSig[i]->loadSettings( _this, "modOutSig_"+QString::number(i) );
			modOutSecNum[i]->loadSettings( _this, "modOutSecNu"+QString::number(i) );
			modCombineType[i]->loadSettings( _this, "modCombineType_"+QString::number(i) );
			modType[i]->loadSettings( _this, "modType_"+QString::number(i) );
			modType2[i]->loadSettings( _this, "modType2_"+QString::number(i) );
		}
	}

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
		case 38: modInAmntArr[num] = modInAmnt[num]->value(); break;
		case 39: modInCurveArr[num] = modInCurve[num]->value(); break;
		case 40: modIn2Arr[num] = modIn2[num]->value(); break;
		case 41: modInNum2Arr[num] = modInNum2[num]->value(); break;
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
		case 78: macroArr[num] = macro[num]->value(); break;
		case 79: filtMutedArr[num] = filtMuted[num]->value(); break;
		case 80: phaseRandArr[num] = phaseRand[num]->value(); break;
		case 81: modType2Arr[num] = modType2[num]->value(); break;
		case 82: subRateLimitArr[num] = subRateLimit[num]->value(); break;
	}

	ConstNotePlayHandleList nphList = NotePlayHandle::nphsOfInstrumentTrack( microwaveTrack, true );

	for( int i = 0; i < nphList.length(); ++i )
	{
		mSynth * ps = static_cast<mSynth *>( nphList[i]->m_pluginData );

		if( ps )// Makes sure "ps" isn't assigned a null value, if m_pluginData hasn't been created yet.
		{
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
				case 38: ps->modInAmnt[num] = modInAmnt[num]->value(); break;
				case 39: ps->modInCurve[num] = modInCurve[num]->value(); break;
				case 40: ps->modIn2[num] = modIn2[num]->value(); break;
				case 41: ps->modInNum2[num] = modInNum2[num]->value(); break;
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
				case 78: ps->macro[num] = macro[num]->value(); break;
				case 79: ps->filtMuted[num] = filtMuted[num]->value(); break;
				case 80: ps->phaseRand[num] = phaseRand[num]->value(); break;
				case 81: ps->modType2[num] = modType2[num]->value(); break;
				case 82: ps->subRateLimit[num] = subRateLimit[num]->value(); break;
			}
		}
	}
}


// Set the range of Morph based on Morph Max
void Microwave::morphMaxChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		morph[i]->setRange( morph[i]->minValue(), morphMax[i]->value(), morph[i]->step<float>() );
	}
}


// Set the range of morphMax and Modify based on new sample length
void Microwave::sampLenChanged()
{
	for( int i = 0; i < 8; ++i )
	{
		morphMax[i]->setRange( morphMax[i]->minValue(), 524288.f / sampLen[i]->value() - 2, morphMax[i]->step<float>() );
		modify[i]->setRange( modify[i]->minValue(), sampLen[i]->value() - 1, modify[i]->step<float>() );
	}
}


//Change graph length to sample length
void Microwave::subSampLenChanged( int num )
{
	if( scroll.value() == 2 && subNum.value() == num + 1 )
	{
		graph.setLength( subSampLen[num]->value() );
	}
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
}

//NOTE: Different from Microwave::modEnabledChanged.
void MicrowaveView::modEnabledChanged()
{
	Microwave * b = castModel<Microwave>();

	matrixScrollBar->setRange( 0, b->maxModEnabled * 100.f );
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


//Stores the highest enabled filter section.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::filtEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( filtEnabled[i]->value() )
		{
			maxFiltEnabled = i+1;
		}
	}
}


//When user drawn on graph, send new values to the correct arrays
void Microwave::samplesChanged( int _begin, int _end )
{
	switch( currentTab )
	{
		case 0:
		{
			break;
		}
		case 1:
		{
			for( int i = _begin; i <= _end; ++i )
			{
				subs[i + ( (subNum.value()-1) * 2048 )] = graph.samples()[i];
			}
			break;
		}
		case 2:
		{
			for( int i = _begin; i <= _end; ++i )
			{
				sampGraphs[i + ( (sampNum.value()-1) * 128 )] = graph.samples()[i];
			}
			break;
		}
		case 3:
		{
			break;
		}
	}
}


void Microwave::switchMatrixSections( int source, int destination )
{
	int modInTemp = modInArr[destination];
	int modInNumTemp = modInNumArr[destination];
	float modInAmntTemp = modInAmntArr[destination];
	float modInCurveTemp = modInCurveArr[destination];
	int modIn2Temp = modIn2Arr[destination];
	int modInNum2Temp = modInNum2Arr[destination];
	float modInAmnt2Temp = modInAmnt2Arr[destination];
	float modInCurve2Temp = modInCurve2Arr[destination];
	int modOutSecTemp = modOutSecArr[destination];
	int modOutSigTemp = modOutSigArr[destination];
	int modOutSecNumTemp = modOutSecNumArr[destination];
	bool modEnabledTemp = modEnabledArr[destination];
	int modCombineTypeTemp = modCombineTypeArr[destination];
	bool modTypeTemp = modTypeArr[destination];
	bool modType2Temp = modType2Arr[destination];

	modIn[destination]->setValue( modInArr[source] );
	modInNum[destination]->setValue( modInNumArr[source] );
	modInAmnt[destination]->setValue( modInAmntArr[source] );
	modInCurve[destination]->setValue( modInCurveArr[source] );
	modIn2[destination]->setValue( modIn2Arr[source] );
	modInNum2[destination]->setValue( modInNum2Arr[source] );
	modInAmnt2[destination]->setValue( modInAmnt2Arr[source] );
	modInCurve2[destination]->setValue( modInCurve2Arr[source] );
	modOutSec[destination]->setValue( modOutSecArr[source] );
	modOutSig[destination]->setValue( modOutSigArr[source] );
	modOutSecNum[destination]->setValue( modOutSecNumArr[source] );
	modEnabled[destination]->setValue( modEnabledArr[source] );
	modCombineType[destination]->setValue( modCombineTypeArr[source] );
	modType[destination]->setValue( modTypeArr[source] );
	modType2[destination]->setValue( modType2Arr[source] );

	modIn[source]->setValue( modInTemp );
	modInNum[source]->setValue( modInNumTemp );
	modInAmnt[source]->setValue( modInAmntTemp );
	modInCurve[source]->setValue( modInCurveTemp );
	modIn2[source]->setValue( modIn2Temp );
	modInNum2[source]->setValue( modInNum2Temp );
	modInAmnt2[source]->setValue( modInAmnt2Temp );
	modInCurve2[source]->setValue( modInCurve2Temp );
	modOutSec[source]->setValue( modOutSecTemp );
	modOutSig[source]->setValue( modOutSigTemp );
	modOutSecNum[source]->setValue( modOutSecNumTemp );
	modEnabled[source]->setValue( modEnabledTemp );
	modCombineType[source]->setValue( modCombineTypeTemp );
	modType[source]->setValue( modTypeTemp );
	modType2[source]->setValue( modType2Temp );

	// If something is sent to a matrix box and the matrix box is moved, we want to make sure it's still attached to the same box after it is moved.
	for( int i = 0; i < 64; ++i )
	{
		if( modOutSec[i]->value() == 4 )// Output is being sent to Matrix
		{
			if( modOutSecNum[i]->value() - 1 == source )// Output was being sent a matrix box that was moved
			{
				modOutSecNum[i]->setValue( destination + 1 );
			}
			else if( modOutSecNum[i]->value() - 1 == destination )// Output was being sent a matrix box that was moved
			{
				modOutSecNum[i]->setValue( source + 1 );
			}
		}
	}
}


// For when notes are playing.  This initializes a new mSynth if the note is new.  It also uses mSynth::nextStringSample to get the synthesizer output.  This is where oversampling and the visualizer are handled.
void Microwave::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{

	if ( _n->m_pluginData == NULL || _n->totalFramesPlayed() == 0 )
	{
		_n->m_pluginData = new mSynth(
					_n,
				Engine::mixer()->processingSampleRate(),
					phaseRandArr, modifyModeArr, modifyArr, morphArr, rangeArr, unisonVoicesArr, unisonDetuneArr, unisonMorphArr, unisonModifyArr, morphMaxArr, detuneArr, waveforms, subs, subEnabledArr, subVolArr, subRateLimitArr, subPhaseArr, subPhaseRandArr, subDetuneArr, subMutedArr, subKeytrackArr, subSampLenArr, subNoiseArr, sampLenArr, modInArr, modInNumArr, modInAmntArr, modInCurveArr, modIn2Arr, modInNum2Arr, modInAmnt2Arr, modInCurve2Arr, modOutSecArr, modOutSigArr, modOutSecNumArr, modCombineTypeArr, modTypeArr, modType2Arr, phaseArr, volArr, filtInVolArr, filtTypeArr, filtSlopeArr, filtCutoffArr, filtResoArr, filtGainArr, filtSatuArr, filtWetDryArr, filtBalArr, filtOutVolArr, filtEnabledArr, enabledArr, modEnabledArr, sampGraphs, mutedArr, sampleEnabledArr, sampleGraphEnabledArr, sampleMutedArr, sampleKeytrackingArr, sampleLoopArr, sampleVolumeArr, samplePanningArr, sampleDetuneArr, samplePhaseArr, samplePhaseRandArr, samples, filtFeedbackArr, filtDetuneArr, filtKeytrackingArr, subPanningArr, sampleStartArr, sampleEndArr, panArr, subTempoArr, macroArr, filtMutedArr );
		mwc = dynamic_cast<Microwave *>(_n->instrumentTrack()->instrument());
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	mSynth * ps = static_cast<mSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		// Process some samples and ignore the output, depending on the oversampling value.  For example, if the oversampling is set to 4x, it will process 4 samples and output 1 of those.
		for( int i = 0; i < oversample.value(); ++i )
		{
			ps->nextStringSample( waveforms, subs, sampGraphs, samples, maxFiltEnabled, maxModEnabled, maxSubEnabled, maxSampleEnabled, maxMainEnabled, Engine::mixer()->processingSampleRate() * (oversample.value()+1), mwc );
		}
		//Get the actual synthesizer output
		std::vector<float> sample = ps->nextStringSample( waveforms, subs, sampGraphs, samples, maxFiltEnabled, maxModEnabled, maxSubEnabled, maxSampleEnabled, maxMainEnabled, Engine::mixer()->processingSampleRate() * (oversample.value()+1), mwc );

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			//Send to output
			_working_buffer[frame][chnl] = sample[chnl];
		}

		//update visualizer
		if( visualize.value() && scroll.value() == 1 )
		{
			if( abs( const_cast<float*>( graph.samples() )[int(ps->sample_realindex[0][0])] - ( ( ( sample[0] + sample[1] ) * 0.5f ) * visvol.value() * 0.01f ) ) >= 0.01f )
			{
				graph.setSampleAt( ps->sample_realindex[0][0], ( ( sample[0] + sample[1] ) * 0.5f ) * visvol.value() * 0.01f );
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
	Microwave * b = castModel<Microwave>();

	setAutoFillBackground( true );

	setMouseTracking( true );

	setAcceptDrops( true );

	pal.setBrush( backgroundRole(), tab1ArtworkImg );
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


	morphKnob = new Knob( knobMicrowave, this );
	morphKnob->setHintText( tr( "Morph" ), "" );
	ToolTip::add( morphKnob, tr( "The Morph knob chooses which waveform out of the wavetable to play." ) );

	rangeKnob = new Knob( knobMicrowave, this );
	rangeKnob->setHintText( tr( "Range" ), "" );
	ToolTip::add( rangeKnob, tr( "The Range knob interpolates (triangularly) the waveforms near the waveform selected by the Morph knob." ) );

	sampLenKnob = new Knob( knobMicrowave, this );
	sampLenKnob->setHintText( tr( "Waveform Sample Length" ), "" );
	ToolTip::add( sampLenKnob, tr( "This knob changes the number of samples per waveform there are in the wavetable.  This is useful for finetuning the wavetbale if the wavetable loading was slightly off." ) );

	morphMaxKnob = new Knob( knobMicrowave, this );
	morphMaxKnob->setHintText( tr( "Morph Max" ), "" );
	ToolTip::add( morphMaxKnob, tr( "This knob sets the maximum of the Morph knob." ) );

	modifyKnob = new Knob( knobMicrowave, this );
	modifyKnob->setHintText( tr( "Modify" ), "" );
	ToolTip::add( modifyKnob, tr( "The Modify knob warps the wavetable realtime using super math powers.  The formula depends on the Modify Mode dropdown box." ) );

	modifyModeBox = new ComboBox( this );
	modifyModeBox->setGeometry( 0, 5, 42, 22 );
	modifyModeBox->setFont( pointSize<8>( modifyModeBox->font() ) );
	ToolTip::add( modifyModeBox, tr( "The Modify Mode dropdown box chooses the formula for the Modify knob to use to warp the wavetable realtime in cool-sounding ways." ) );

	unisonVoicesKnob = new Knob( knobSmallMicrowave, this );
	unisonVoicesKnob->setHintText( tr( "Unison Voices" ), "" );
	ToolTip::add( unisonVoicesKnob, tr( "This knob clones this oscillator multiple times depending on its value, and makes slight changes to the clones depending on the other unison-related knobs." ) );

	unisonDetuneKnob = new Knob( knobSmallMicrowave, this );
	unisonDetuneKnob->setHintText( tr( "Unison Detune" ), "" );
	ToolTip::add( unisonDetuneKnob, tr( "This knob detunes every unison voice by a random number that is less than the specified amount." ) );

	unisonMorphKnob = new Knob( knobSmallMicrowave, this );
	unisonMorphKnob->setHintText( tr( "Unison Morph" ), "" );
	ToolTip::add( unisonMorphKnob, tr( "This knob changes the wavetable position of each individual unison voice." ) );

	unisonModifyKnob = new Knob( knobSmallMicrowave, this );
	unisonModifyKnob->setHintText( tr( "Unison Modify" ), "" );
	ToolTip::add( unisonModifyKnob, tr( "This knob changes the Modify value of each individual unison voice." ) );

	detuneKnob = new Knob( knobSmallMicrowave, this );
	detuneKnob->setHintText( tr( "Detune" ), "" );
	ToolTip::add( detuneKnob, tr( "This knob changes the pitch of the oscillator." ) );

	phaseKnob = new Knob( knobSmallMicrowave, this );
	phaseKnob->setHintText( tr( "Phase" ), "" );
	ToolTip::add( phaseKnob, tr( "This knob changes the phase (starting position) of the oscillator." ) );

	phaseRandKnob = new Knob( knobSmallMicrowave, this );
	phaseRandKnob->setHintText( tr( "Phase Randomness" ), "" );
	ToolTip::add( phaseRandKnob, tr( "This knob chooses a random phase for every note and unison voice.  The phase change will never be larger than this knob's value." ) );

	volKnob = new Knob( knobMicrowave, this );
	volKnob->setHintText( tr( "Volume" ), "" );
	ToolTip::add( volKnob, tr( "This knob changes the volume.  What a surprise!" ) );

	enabledToggle = new LedCheckBox( "", this, tr( "Oscillator Enabled" ), LedCheckBox::Green );
	ToolTip::add( enabledToggle, tr( "This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!" ) );

	mutedToggle = new LedCheckBox( "", this, tr( "Oscillator Muted" ), LedCheckBox::Green );
	ToolTip::add( mutedToggle, tr( "This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );

	panKnob = new Knob( knobMicrowave, this );
	panKnob->setHintText( tr( "Panning" ), "" );
	ToolTip::add( panKnob, tr( "This knob lowers the volume in one ear by an amount depending on this knob's value." ) );


	sampleEnabledToggle = new LedCheckBox( "", this, tr( "Sample Enabled" ), LedCheckBox::Green );
	ToolTip::add( sampleEnabledToggle, tr( "This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!" ) );
	sampleGraphEnabledToggle = new LedCheckBox( "", this, tr( "Sample Graph Enabled" ), LedCheckBox::Green );
	ToolTip::add( sampleGraphEnabledToggle, tr( "This button enables the graph for this oscillator.  On the graph, left/right is time and up/down is position in the sample.  A saw wave in the graph will play the sample normally." ) );
	sampleMutedToggle = new LedCheckBox( "", this, tr( "Sample Muted" ), LedCheckBox::Green );
	ToolTip::add( sampleMutedToggle, tr( "This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );
	sampleKeytrackingToggle = new LedCheckBox( "", this, tr( "Sample Keytracking" ), LedCheckBox::Green );
	ToolTip::add( sampleKeytrackingToggle, tr( "This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound." ) );
	sampleLoopToggle = new LedCheckBox( "", this, tr( "Loop Sample" ), LedCheckBox::Green );
	ToolTip::add( sampleLoopToggle, tr( "This button turns looping on/off.  When looping is on, the sample will go back to the starting position when it is done playing." ) );

	sampleVolumeKnob = new Knob( knobMicrowave, this );
	sampleVolumeKnob->setHintText( tr( "Volume" ), "" );
	ToolTip::add( sampleVolumeKnob, tr( "This, like most other volume knobs, controls the volume." ) );
	samplePanningKnob = new Knob( knobMicrowave, this );
	samplePanningKnob->setHintText( tr( "Panning" ), "" );
	ToolTip::add( samplePanningKnob, tr( "This knob lowers the volume in one ear by an amount depending on this knob's value." ) );
	sampleDetuneKnob = new Knob( knobSmallMicrowave, this );
	sampleDetuneKnob->setHintText( tr( "Detune" ), "" );
	ToolTip::add( sampleDetuneKnob, tr( "This knob changes the pitch (and speed) of the sample." ) );
	samplePhaseKnob = new Knob( knobSmallMicrowave, this );
	samplePhaseKnob->setHintText( tr( "Phase" ), "" );
	ToolTip::add( samplePhaseKnob, tr( "This knob changes the position of the sample, and is updated realtime when automated." ) );
	samplePhaseRandKnob = new Knob( knobSmallMicrowave, this );
	samplePhaseRandKnob->setHintText( tr( "Phase Randomness" ), "" );
	ToolTip::add( samplePhaseRandKnob, tr( "This knob makes the sample start at a random position with every note." ) );
	sampleStartKnob = new Knob( knobSmallMicrowave, this );
	sampleStartKnob->setHintText( tr( "Start" ), "" );
	ToolTip::add( sampleStartKnob, tr( "This knob changes the starting position of the sample." ) );
	sampleEndKnob = new Knob( knobSmallMicrowave, this );
	sampleEndKnob->setHintText( tr( "End" ), "" );
	ToolTip::add( sampleEndKnob, tr( "This knob changes the ending position of the sample." ) );


	for( int i = 0; i < 8; ++i )
	{
		filtInVolKnob[i] = new Knob( knobSmallMicrowave, this );
		filtInVolKnob[i]->setHintText( tr( "Input Volume" ), "" );
		ToolTip::add( filtInVolKnob[i], tr( "This knob changes the input volume of the filter." ) );

		filtTypeBox[i] = new ComboBox( this );
		filtTypeBox[i]->setGeometry( 1000, 5, 42, 22 );
		filtTypeBox[i]->setFont( pointSize<8>( filtTypeBox[i]->font() ) );
		ToolTip::add( filtTypeBox[i], tr( "This dropdown box changes the filter type." ) );

		filtSlopeBox[i] = new ComboBox( this );
		filtSlopeBox[i]->setGeometry( 1000, 5, 42, 22 );
		filtSlopeBox[i]->setFont( pointSize<8>( filtSlopeBox[i]->font() ) );
		ToolTip::add( filtSlopeBox[i], tr( "This dropdown box changes how many times the audio is run through the filter (which changes the slope).  For example, a sound run through a 12 db filter three times will result in a 36 db slope." ) );

		filtCutoffKnob[i] = new Knob( knobSmallMicrowave, this );
		filtCutoffKnob[i]->setHintText( tr( "Cutoff Frequency" ), "" );
		ToolTip::add( filtCutoffKnob[i], tr( "This knob changes cutoff frequency of the filter." ) );

		filtResoKnob[i] = new Knob( knobSmallMicrowave, this );
		filtResoKnob[i]->setHintText( tr( "Resonance" ), "" );
		ToolTip::add( filtInVolKnob[i], tr( "This knob changes the resonance of the filter." ) );

		filtGainKnob[i] = new Knob( knobSmallMicrowave, this );
		filtGainKnob[i]->setHintText( tr( "db Gain" ), "" );
		ToolTip::add( filtGainKnob[i], tr( "This knob changes the gain of the filter.  This only applies to some filter types, e.g. Peak, High Shelf, Low Shelf, etc." ) );

		filtSatuKnob[i] = new Knob( knobSmallMicrowave, this );
		filtSatuKnob[i]->setHintText( tr( "Saturation" ), "" );
		ToolTip::add( filtSatuKnob[i], tr( "This knob applies some basic distortion after the filter is applied." ) );

		filtWetDryKnob[i] = new Knob( knobSmallMicrowave, this );
		filtWetDryKnob[i]->setHintText( tr( "Wet/Dry" ), "" );
		ToolTip::add( filtWetDryKnob[i], tr( "This knob allows mixing the filtered signal with the unfiltered signal." ) );

		filtBalKnob[i] = new Knob( knobSmallMicrowave, this );
		filtBalKnob[i]->setHintText( tr( "Balance/Panning" ), "" );
		ToolTip::add( filtBalKnob[i], tr( "This knob decreases the Wet and increases the Dry of the filter in one ear, depending on the knob's value." ) );

		filtOutVolKnob[i] = new Knob( knobSmallMicrowave, this );
		filtOutVolKnob[i]->setHintText( tr( "Output Volume" ), "" );
		ToolTip::add( filtOutVolKnob[i], tr( "This knob changes the output volume of the filter." ) );

		filtEnabledToggle[i] = new LedCheckBox( "", this, tr( "Filter Enabled" ), LedCheckBox::Green );
		ToolTip::add( filtEnabledToggle[i], tr( "This button enables the filter.  A disabled filter will never do anything and does not use any CPU.  In many cases, the settings of a disabled filter will not be saved, so be careful!" ) );

		filtFeedbackKnob[i] = new Knob( knobSmallMicrowave, this );
		filtFeedbackKnob[i]->setHintText( tr( "Feedback" ), "" );
		ToolTip::add( filtFeedbackKnob[i], tr( "This knob sends the specified portion of the filter output back into the input after a certain delay.  This delay effects the pitch, and can be changed using the filter's Detune and Keytracking options." ) );

		filtDetuneKnob[i] = new Knob( knobSmallMicrowave, this );
		filtDetuneKnob[i]->setHintText( tr( "Detune" ), "" );
		ToolTip::add( filtDetuneKnob[i], tr( "This knob changes the delay of the filter's feedback to match the specified pitch." ) );

		filtKeytrackingToggle[i] = new LedCheckBox( "", this, tr( "Keytracking" ), LedCheckBox::Green );
		ToolTip::add( filtKeytrackingToggle[i], tr( "When this is enabled, the delay of the filter's feedback changes to match the frequency of the notes you play." ) );

		filtMutedToggle[i] = new LedCheckBox( "", this, tr( "Muted" ), LedCheckBox::Green );
		ToolTip::add( filtMutedToggle[i], tr( "This button mutes the filter.  An enabled but muted filter will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );

		macroKnob[i] = new Knob( knobMicrowave, this );
		macroKnob[i]->setHintText( tr( "Macro" ) + " " + QString::number(i), "" );
		ToolTip::add( macroKnob[i], tr( "This knob's value can be used in the Matrix to control many values at the same time, at different amounts." ) );
	}

	subVolKnob = new Knob( knobMicrowave, this );
	subVolKnob->setHintText( tr( "Sub Oscillator Volume" ), "" );
	ToolTip::add( subVolKnob, tr( "This knob, as you probably/hopefully expected, controls the volume of the oscillator." ) );

	subEnabledToggle = new LedCheckBox( "", this, tr( "Sub Oscillator Enabled" ), LedCheckBox::Green );
	ToolTip::add( subEnabledToggle, tr( "This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!" ) );

	subPhaseKnob = new Knob( knobSmallMicrowave, this );
	subPhaseKnob->setHintText( tr( "Sub Oscillator Phase" ), "" );
	ToolTip::add( subPhaseKnob, tr( "This knob changes the phase (starting position) of the oscillator." ) );

	subPhaseRandKnob = new Knob( knobSmallMicrowave, this );
	subPhaseRandKnob->setHintText( tr( "Sub Oscillator Phase Randomness" ), "" );
	ToolTip::add( subPhaseRandKnob, tr( "This knob chooses a random phase for every note.  The phase change will never be larger than this knob's value." ) );

	subDetuneKnob = new Knob( knobMicrowave, this );
	subDetuneKnob->setHintText( tr( "Sub Oscillator Pitch" ), "" );
	ToolTip::add( subDetuneKnob, tr( "This knob changes the pitch of the oscillator." ) );

	subMutedToggle = new LedCheckBox( "", this, tr( "Sub Oscillator Muted" ), LedCheckBox::Green );
	ToolTip::add( subMutedToggle, tr( "This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );

	subKeytrackToggle = new LedCheckBox( "", this, tr( "Sub Oscillator Keytracking Enabled" ), LedCheckBox::Green );
	ToolTip::add( subKeytrackToggle, tr( "This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound." ) );

	subSampLenKnob = new Knob( knobMicrowave, this );
	subSampLenKnob->setHintText( tr( "Sub Oscillator Sample Length" ), "" );
	ToolTip::add( subSampLenKnob, tr( "This knob changes the waveform length, which you can see in the graph." ) );

	subNoiseToggle = new LedCheckBox( "", this, tr( "Sub Oscillator Noise Enabled" ), LedCheckBox::Green );
	ToolTip::add( subNoiseToggle, tr( "This button converts this oscillator into a noise generator.  A random part of the graph is chosen, that value is added to the previous output in the same direction it was going, and when the waveform crosses the top or bottom, the direction changes." ) );

	subPanningKnob = new Knob( knobMicrowave, this );
	subPanningKnob->setHintText( tr( "Sub Oscillator Panning" ), "" );
	ToolTip::add( subPanningKnob, tr( "This knob lowers the volume in one ear by an amount depending on this knob's value." ) );

	subTempoKnob = new Knob( knobMicrowave, this );
	subTempoKnob->setHintText( tr( "Sub Oscillator Tempo" ), "" );
	ToolTip::add( subTempoKnob, tr( "When this knob is anything other than 0, the oscillator is tempo synced to the specified tempo.  This is meant for the creation of envelopes/LFOs/step sequencers in the Matrix tab, and you'll most likely want to enable the Muted LED when using this." ) );

	subRateLimitKnob = new Knob( knobMicrowave, this );
	subRateLimitKnob->setHintText( tr( "Sub Oscillator Rate Limit" ), "" );
	ToolTip::add( subRateLimitKnob, tr( "This knob limits the speed at which the waveform can change." ) );

	for( int i = 0; i < 8; ++i )
	{
		modOutSecBox[i] = new ComboBox( this );
		modOutSecBox[i]->setGeometry( 2000, 5, 42, 22 );
		modOutSecBox[i]->setFont( pointSize<8>( modOutSecBox[i]->font() ) );
		ToolTip::add( modOutSecBox[i], tr( "This dropdown box chooses an output for the Matrix Box." ) );

		modOutSigBox[i] = new ComboBox( this );
		modOutSigBox[i]->setGeometry( 2000, 5, 42, 22 );
		modOutSigBox[i]->setFont( pointSize<8>( modOutSigBox[i]->font() ) );
		ToolTip::add( modOutSigBox[i], tr( "This dropdown box chooses which part of the input to send to the Matrix Box (e.g. which parameter to control, which filter, etc.)." ) );

		modOutSecNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Output Number" );
		ToolTip::add( modOutSecNumBox[i], tr( "This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator)." ) );

		modInBox[i] = new ComboBox( this );
		modInBox[i]->setGeometry( 2000, 5, 42, 22 );
		modInBox[i]->setFont( pointSize<8>( modInBox[i]->font() ) );
		ToolTip::add( modInBox[i], tr( "This dropdown box chooses an input for the Matrix Box." ) );

		modInNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Input Number" );
		ToolTip::add( modInNumBox[i], tr( "This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator, which filter, etc.)." ) );

		modInAmntKnob[i] = new Knob( knobSmallMicrowave, this );
		modInAmntKnob[i]->setHintText( tr( "Modulator Amount" ), "" );
		ToolTip::add( modInAmntKnob[i], tr( "This knob chooses how much of the input to send to the output." ) );

		modInCurveKnob[i] = new Knob( knobSmallMicrowave, this );
		modInCurveKnob[i]->setHintText( tr( "Modulator Curve" ), "" );
		ToolTip::add( modInCurveKnob[i], tr( "This knob gives the input value a bias toward the top or bottom" ) );

		modInBox2[i] = new ComboBox( this );
		modInBox2[i]->setGeometry( 2000, 5, 42, 22 );
		modInBox2[i]->setFont( pointSize<8>( modInBox2[i]->font() ) );
		ToolTip::add( modInBox2[i], tr( "This dropdown box chooses an input for the Matrix Box." ) );

		modInNumBox2[i] = new LcdSpinBox( 2, "microwave", this, "Secondary Mod Input Number" );
		ToolTip::add( modInNumBox2[i], tr( "This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator, which filter, etc.)." ) );

		modInAmntKnob2[i] = new Knob( knobSmallMicrowave, this );
		modInAmntKnob2[i]->setHintText( tr( "Secondary Modulator Amount" ), "" );
		ToolTip::add( modInAmntKnob2[i], tr( "This knob chooses how much of the input to send to the output." ) );

		modInCurveKnob2[i] = new Knob( knobSmallMicrowave, this );
		modInCurveKnob2[i]->setHintText( tr( "Secondary Modulator Curve" ), "" );
		ToolTip::add( modInCurveKnob2[i], tr( "This knob gives the input value a bias toward the top or bottom" ) );

		modEnabledToggle[i] = new LedCheckBox( "", this, tr( "Modulation Enabled" ), LedCheckBox::Green );
		ToolTip::add( modEnabledToggle[i], tr( "This button enables the Matrix Box.  While disabled, this does not use CPU." ) );

		modCombineTypeBox[i] = new ComboBox( this );
		modCombineTypeBox[i]->setGeometry( 2000, 5, 42, 22 );
		modCombineTypeBox[i]->setFont( pointSize<8>( modCombineTypeBox[i]->font() ) );
		ToolTip::add( modCombineTypeBox[i], tr( "This dropdown box chooses how to combine the two Matrix inputs." ) );

		modTypeToggle[i] = new LedCheckBox( "", this, tr( "Envelope Enabled" ), LedCheckBox::Green );
		ToolTip::add( modTypeToggle[i], tr( "This button, when enabled, treats the input as an envelope rather than an LFO." ) );

		modType2Toggle[i] = new LedCheckBox( "", this, tr( "Envelope Enabled" ), LedCheckBox::Green );
		ToolTip::add( modType2Toggle[i], tr( "This button, when enabled, treats the input as an envelope rather than an LFO." ) );

		modUpArrow[i] = new PixmapButton( this, tr( "Move Matrix Section Up" ) );
		modUpArrow[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
		modUpArrow[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
		ToolTip::add( modUpArrow[i], tr( "Move this Matrix Box up" ) );

		modDownArrow[i] = new PixmapButton( this, tr( "Move Matrix Section Down" ) );
		modDownArrow[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
		modDownArrow[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
		ToolTip::add( modDownArrow[i], tr( "Move this Matrix Box down" ) );

		modNumText[i] = new QLineEdit(this);
		modNumText[i]->resize(23, 19);
	}

	visvolKnob = new Knob( knobSmallMicrowave, this );
	visvolKnob->setHintText( tr( "Visualizer Volume" ), "" );
	ToolTip::add( visvolKnob, tr( "This knob works as a vertical zoom knob for the visualizer." ) );

	loadChnlKnob = new Knob( knobMicrowave, this );
	loadChnlKnob->setHintText( tr( "Wavetable Loading Channel" ), "" );
	ToolTip::add( loadChnlKnob, tr( "This knob chooses whether to load the left or right audio of the selected sample/wavetable." ) );


	wtLoad1Knob = new Knob( knobMicrowave, this );
	wtLoad1Knob->setHintText( tr( "Wavetable Loading Knob 1" ), "" );

	wtLoad2Knob = new Knob( knobMicrowave, this );
	wtLoad2Knob->setHintText( tr( "Wavetable Loading Knob 2" ), "" );

	wtLoad3Knob = new Knob( knobMicrowave, this );
	wtLoad3Knob->setHintText( tr( "Wavetable Loading Knob 3" ), "" );

	wtLoad4Knob = new Knob( knobMicrowave, this );
	wtLoad4Knob->setHintText( tr( "Wavetable Loading Knob 4" ), "" );


	graph = new Graph( this, Graph::BarCenterGradStyle, 204, 134 );
	graph->setAutoFillBackground( true );
	graph->setGraphColor( QColor( 121, 222, 239 ) );

	ToolTip::add( graph, tr ( "Draw here by dragging your mouse on this graph." ) );

	pal = QPalette();
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph") );
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
	sinWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave" ) );
	sinWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave" ) );
	ToolTip::add( sinWaveBtn, tr( "Sine wave" ) );

	triangleWaveBtn = new PixmapButton( this, tr( "Nachos" ) );
	triangleWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave" ) );
	triangleWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( triangleWaveBtn, tr( "Nacho wave" ) );

	sawWaveBtn = new PixmapButton( this, tr( "Sawsa" ) );
	sawWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave" ) );
	sawWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave" ) );
	ToolTip::add( sawWaveBtn, tr( "Sawsa wave" ) );

	sqrWaveBtn = new PixmapButton( this, tr( "Sosig" ) );
	sqrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave" ) );
	sqrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave" ) );
	ToolTip::add( sqrWaveBtn, tr( "Sosig wave" ) );

	whiteNoiseWaveBtn = new PixmapButton( this, tr( "Metal Fork" ) );
	whiteNoiseWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	whiteNoiseWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( whiteNoiseWaveBtn, tr( "Metal Fork" ) );

	usrWaveBtn = new PixmapButton( this, tr( "Takeout" ) );
	usrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	usrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	ToolTip::add( usrWaveBtn, tr( "Takeout Menu" ) );

	smoothBtn = new PixmapButton( this, tr( "Microwave Cover" ) );
	smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave" ) );
	smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave" ) );
	ToolTip::add( smoothBtn, tr( "Microwave Cover" ) );


	sinWave2Btn = new PixmapButton( this, tr( "Sine" ) );
	sinWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave" ) );
	sinWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave" ) );
	ToolTip::add( sinWave2Btn, tr( "Sine wave" ) );

	triangleWave2Btn = new PixmapButton( this, tr( "Nachos" ) );
	triangleWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave" ) );
	triangleWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( triangleWave2Btn,
			tr( "Nacho wave" ) );

	sawWave2Btn = new PixmapButton( this, tr( "Sawsa" ) );
	sawWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave" ) );
	sawWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave" ) );
	ToolTip::add( sawWave2Btn,
			tr( "Sawsa wave" ) );

	sqrWave2Btn = new PixmapButton( this, tr( "Sosig" ) );
	sqrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave" ) );
	sqrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave" ) );
	ToolTip::add( sqrWave2Btn, tr( "Sosig wave" ) );

	whiteNoiseWave2Btn = new PixmapButton( this, tr( "Metal Fork" ) );
	whiteNoiseWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	whiteNoiseWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( whiteNoiseWave2Btn, tr( "Metal Fork" ) );

	usrWave2Btn = new PixmapButton( this, tr( "Takeout Menu" ) );
	usrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	usrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	ToolTip::add( usrWave2Btn, tr( "Takeout Menu" ) );

	smooth2Btn = new PixmapButton( this, tr( "Microwave Cover" ) );
	smooth2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave" ) );
	smooth2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave" ) );
	ToolTip::add( smooth2Btn, tr( "Microwave Cover" ) );


	tab1Btn = new PixmapButton( this, tr( "Wavetable Tab" ) );
	tab1Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
	tab1Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
	ToolTip::add( tab1Btn, tr( "Wavetable Tab" ) );

	tab2Btn = new PixmapButton( this, tr( "Sub Tab" ) );
	tab2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
	tab2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
	ToolTip::add( tab2Btn, tr( "Sub Tab" ) );

	tab3Btn = new PixmapButton( this, tr( "Sample Tab" ) );
	tab3Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
	tab3Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
	ToolTip::add( tab3Btn, tr( "Sample Tab" ) );

	tab4Btn = new PixmapButton( this, tr( "Matrix Tab" ) );
	tab4Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
	tab4Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
	ToolTip::add( tab4Btn, tr( "Matrix Tab" ) );

	tab5Btn = new PixmapButton( this, tr( "Effect Tab" ) );
	tab5Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
	tab5Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
	ToolTip::add( tab5Btn, tr( "Effect Tab" ) );

	tab6Btn = new PixmapButton( this, tr( "Miscellaneous Tab" ) );
	tab6Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
	tab6Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
	ToolTip::add( tab6Btn, tr( "Miscellaneous Tab" ) );


	mainFlipBtn = new PixmapButton( this, tr( "Flip to other knobs" ) );
	mainFlipBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
	mainFlipBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
	ToolTip::add( mainFlipBtn, tr( "Flip to other knobs" ) );
	mainFlipBtn->setCheckable(true);

	subFlipBtn = new PixmapButton( this, tr( "Flip to other knobs" ) );
	subFlipBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
	subFlipBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
	ToolTip::add( subFlipBtn, tr( "Flip to other knobs" ) );
	subFlipBtn->setCheckable(true);


	manualBtn = new PixmapButton( this, tr( "Manual" ) );
	manualBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "manual_active" ) );
	manualBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "manual_inactive" ) );
	ToolTip::add( manualBtn, tr( "Manual" ) );


	normalizeBtn = new PixmapButton( this, tr( "Normalize Wavetable" ) );
	normalizeBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "normalize_button" ) );
	normalizeBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "normalize_button" ) );
	ToolTip::add( normalizeBtn, tr( "Normalize Wavetable" ) );

	desawBtn = new PixmapButton( this, tr( "De-saw Wavetable" ) );
	desawBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "desaw_button" ) );
	desawBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "desaw_button" ) );
	ToolTip::add( desawBtn, tr( "De-saw Wavetable" ) );


	XBtn = new PixmapButton( this, tr( "Leave wavetable loading tab" ) );
	XBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "xbtn" ) );
	XBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "xbtn" ) );
	ToolTip::add( XBtn, tr( "Leave wavetable loading tab" ) );


	visualizeToggle = new LedCheckBox( "", this, tr( "Visualize" ), LedCheckBox::Green );

	openWavetableButton = new PixmapButton( this );
	openWavetableButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	openWavetableButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	openWavetableButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	connect( openWavetableButton, SIGNAL( clicked() ), this, SLOT( openWavetableFileBtnClicked() ) );
	ToolTip::add( openWavetableButton, tr( "Open wavetable" ) );

	confirmLoadButton = new PixmapButton( this );
	confirmLoadButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	confirmLoadButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	confirmLoadButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	connect( confirmLoadButton, SIGNAL( clicked() ), this, SLOT( confirmWavetableLoadClicked() ) );
	ToolTip::add( confirmLoadButton, tr( "Load Wavetable" ) );

	subNumBox = new LcdSpinBox( 2, "microwave", this, "Sub Oscillator Number" );

	sampNumBox = new LcdSpinBox( 2, "microwave", this, "Sample Number" );

	mainNumBox = new LcdSpinBox( 2, "microwave", this, "Oscillator Number" );

	oversampleBox = new ComboBox( this );
	oversampleBox->setGeometry( 0, 0, 42, 22 );
	oversampleBox->setFont( pointSize<8>( oversampleBox->font() ) );

	loadModeBox = new ComboBox( this );
	loadModeBox->setGeometry( 0, 0, 202, 22 );
	loadModeBox->setFont( pointSize<8>( loadModeBox->font() ) );

	openSampleButton = new PixmapButton( this );
	openSampleButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	openSampleButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	openSampleButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );

	scrollKnob = new Knob( knobSmallMicrowave, this );
	scrollKnob->hide();

	effectScrollBar = new QScrollBar( Qt::Vertical, this );
	effectScrollBar->setSingleStep( 1 );
	effectScrollBar->setPageStep( 100 );
	effectScrollBar->setFixedHeight( 197 );
	effectScrollBar->setRange( 0, 590 );
	connect( effectScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( updateScroll( ) ) );

	effectScrollBar->setStyleSheet("QScrollBar::handle:horizontal { background: #3f4750; border: none; border-radius: 1px; min-width: 24px; }\
					QScrollBar::handle:horizontal:hover { background: #a6adb1; }\
					QScrollBar::handle:horizontal:pressed { background: #a6adb1; }\
					QScrollBar::handle:vertical { background: #3f4750; border: none; border-radius: 1px; min-height: 24px; }\
					QScrollBar::handle:vertical:hover { background: #a6adb1; }\
					QScrollBar::handle:vertical:pressed { background: #a6adb1; }\
					QScrollBar::handle:horizontal:disabled, QScrollBar::handle:vertical:disabled  { background: #262b30; border-radius: 1px; border: none; }");

	matrixScrollBar = new QScrollBar( Qt::Vertical, this );
	matrixScrollBar->setSingleStep( 1 );
	matrixScrollBar->setPageStep( 100 );
	matrixScrollBar->setFixedHeight( 197 );
	matrixScrollBar->setRange( 0, 6232 );
	connect( matrixScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( updateScroll( ) ) );

	matrixScrollBar->setStyleSheet("QScrollBar::handle:horizontal { background: #3f4750; border: none; border-radius: 1px; min-width: 24px; }\
					QScrollBar::handle:horizontal:hover { background: #a6adb1; }\
					QScrollBar::handle:horizontal:pressed { background: #a6adb1; }\
					QScrollBar::handle:vertical { background: #3f4750; border: none; border-radius: 1px; min-height: 24px; }\
					QScrollBar::handle:vertical:hover { background: #a6adb1; }\
					QScrollBar::handle:vertical:pressed { background: #a6adb1; }\
					QScrollBar::handle:horizontal:disabled, QScrollBar::handle:vertical:disabled  { background: #262b30; border-radius: 1px; border: none; }");


	connect( openSampleButton, SIGNAL( clicked() ), this, SLOT( openSampleFileBtnClicked() ) );
	ToolTip::add( openSampleButton, tr( "Open sample" ) );



	connect( sinWaveBtn, SIGNAL (clicked () ), this, SLOT ( sinWaveClicked() ) );
	connect( triangleWaveBtn, SIGNAL ( clicked () ), this, SLOT ( triangleWaveClicked() ) );
	connect( sawWaveBtn, SIGNAL (clicked () ), this, SLOT ( sawWaveClicked() ) );
	connect( sqrWaveBtn, SIGNAL ( clicked () ), this, SLOT ( sqrWaveClicked() ) );
	connect( whiteNoiseWaveBtn, SIGNAL ( clicked () ), this, SLOT ( noiseWaveClicked() ) );
	connect( usrWaveBtn, SIGNAL ( clicked () ), this, SLOT ( usrWaveClicked() ) );
	connect( smoothBtn, SIGNAL ( clicked () ), this, SLOT ( smoothClicked() ) );

	connect( sinWave2Btn, SIGNAL (clicked () ), this, SLOT ( sinWaveClicked() ) );
	connect( triangleWave2Btn, SIGNAL ( clicked () ), this, SLOT ( triangleWaveClicked() ) );
	connect( sawWave2Btn, SIGNAL (clicked () ), this, SLOT ( sawWaveClicked() ) );
	connect( sqrWave2Btn, SIGNAL ( clicked () ), this, SLOT ( sqrWaveClicked() ) );
	connect( whiteNoiseWave2Btn, SIGNAL ( clicked () ), this, SLOT ( noiseWaveClicked() ) );
	connect( usrWave2Btn, SIGNAL ( clicked () ), this, SLOT ( usrWaveClicked() ) );
	connect( smooth2Btn, SIGNAL ( clicked () ), this, SLOT ( smoothClicked() ) );


	connect( XBtn, SIGNAL (clicked () ), this, SLOT ( XBtnClicked() ) );

	connect( normalizeBtn, SIGNAL (clicked () ), this, SLOT ( normalizeClicked() ) );
	connect( desawBtn, SIGNAL (clicked () ), this, SLOT ( desawClicked() ) );


	// This is a mess, but for some reason just entering a number without a variable didn't work...
	int ii = 1;
	connect( tab1Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 2;
	connect( tab2Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 3;
	connect( tab3Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 4;
	connect( tab4Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 5;
	connect( tab5Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 6;
	connect( tab6Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );


	connect( mainFlipBtn, SIGNAL (clicked () ), this, SLOT ( flipperClicked() ) );

	connect( subFlipBtn, SIGNAL (clicked () ), this, SLOT ( flipperClicked() ) );


	connect( visualizeToggle, SIGNAL( toggled( bool ) ), this, SLOT ( visualizeToggled( bool ) ) );



	connect( &b->scroll, SIGNAL( dataChanged( ) ), this, SLOT( updateScroll( ) ) );

	connect( scrollKnob, SIGNAL( sliderReleased( ) ), this, SLOT( scrollReleased( ) ) );

	connect( &b->mainNum, SIGNAL( dataChanged( ) ), this, SLOT( mainNumChanged( ) ) );

	connect( &b->subNum, SIGNAL( dataChanged( ) ), this, SLOT( subNumChanged( ) ) );

	connect( &b->sampNum, SIGNAL( dataChanged( ) ), this, SLOT( sampNumChanged( ) ) );

	connect( manualBtn, SIGNAL (clicked ( bool ) ), this, SLOT ( manualBtnClicked() ) );

	for( int i = 0; i < 8; ++i )
	{
		connect( b->modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { modOutSecChanged(i); }, Qt::DirectConnection );
		connect( b->modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { modInChanged(i); }, Qt::DirectConnection );

		connect( modUpArrow[i], &PixmapButton::clicked, this, [this, i]() { modUpClicked(i); } );
		connect( modDownArrow[i], &PixmapButton::clicked, this, [this, i]() { modDownClicked(i); } );

		connect( b->modEnabled[i], &BoolModel::dataChanged, this, [this, i]() { modEnabledChanged(); } );
	}

	updateScroll();
	updateBackground();
}


// Connects knobs/GUI elements to their models
void MicrowaveView::modelChanged()
{
	Microwave * b = castModel<Microwave>();

	for( int i = 0; i < 8; ++i )
	{
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
		filtMutedToggle[i]->setModel( b->filtMuted[i] );

		macroKnob[i]->setModel( b->macro[i] );
	}

	graph->setModel( &b->graph );
	visvolKnob->setModel( &b->visvol );
	visualizeToggle->setModel( &b->visualize );
	subNumBox->setModel( &b->subNum );
	sampNumBox->setModel( &b->sampNum );
	loadChnlKnob->setModel( &b->loadChnl );
	scrollKnob->setModel( &b->scroll );
	mainNumBox->setModel( &b->mainNum );
	oversampleBox->setModel( &b->oversample );
	loadModeBox->setModel( &b->loadMode );
	wtLoad1Knob->setModel( &b->wtLoad1 );
	wtLoad2Knob->setModel( &b->wtLoad2 );
	wtLoad3Knob->setModel( &b->wtLoad3 );
	wtLoad4Knob->setModel( &b->wtLoad4 );
	mainFlipBtn->setModel( &b->mainFlipped );
	subFlipBtn->setModel( &b->subFlipped );
}


// Puts all of the GUI elements in their correct locations, depending on the scroll knob value.
void MicrowaveView::updateScroll()
{
	Microwave * b = castModel<Microwave>();

	int scrollVal = ( b->scroll.value() - 1 ) * 250.f;
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int effectScrollVal = ( effectScrollBar->value() ) / 100.f * 92.f;
	int mainFlipped = b->mainFlipped.value();
	int subFlipped = b->subFlipped.value();

	int mainIsFlipped = mainFlipped * 500.f;
	int mainIsNotFlipped = !mainFlipped * 500.f;
	int subIsFlipped = subFlipped * 500.f;
	int subIsNotFlipped = !subFlipped * 500.f;

	morphKnob->move( 23 - scrollVal, 172 + mainIsFlipped );
	rangeKnob->move( 55 - scrollVal, 172 + mainIsFlipped );
	sampLenKnob->move( 23 - scrollVal, 172 + mainIsNotFlipped );
	morphMaxKnob->move( 55 - scrollVal, 172 + mainIsNotFlipped );
	modifyKnob->move( 87 - scrollVal, 172 + mainIsFlipped );
	modifyModeBox->move( 127 - scrollVal, 186 + mainIsFlipped );
	unisonVoicesKnob->move( 184 - scrollVal, 172 + mainIsFlipped );
	unisonDetuneKnob->move( 209 - scrollVal, 172 + mainIsFlipped );
	unisonMorphKnob->move( 184 - scrollVal, 203 + mainIsFlipped );
	unisonModifyKnob->move( 209 - scrollVal, 203 + mainIsFlipped );
	detuneKnob->move( 152 - scrollVal, 216 + mainIsFlipped );
	phaseKnob->move( 86 - scrollVal, 216 + mainIsNotFlipped );
	phaseRandKnob->move( 113 - scrollVal, 216 + mainIsNotFlipped );
	volKnob->move( 87 - scrollVal, 172 + mainIsNotFlipped );
	enabledToggle->move( 95 - scrollVal, 220 + mainIsFlipped );
	mutedToggle->move( 129 - scrollVal, 220 + mainIsFlipped );
	panKnob->move( 122 - scrollVal, 172 + mainIsNotFlipped );


	sampleEnabledToggle->move( 86 + 500 - scrollVal, 229 );
	sampleGraphEnabledToggle->move( 138 + 500 - scrollVal, 229 );
	sampleMutedToggle->move( 103 + 500 - scrollVal, 229 );
	sampleKeytrackingToggle->move( 121 + 500 - scrollVal, 229 );
	sampleLoopToggle->move( 155 + 500 - scrollVal, 229 );

	sampleVolumeKnob->move( 23 + 500 - scrollVal, 172 );
	samplePanningKnob->move( 55 + 500 - scrollVal, 172 );
	sampleDetuneKnob->move( 93 + 500 - scrollVal, 172 );
	samplePhaseKnob->move( 180 + 500 - scrollVal, 172 );
	samplePhaseRandKnob->move( 206 + 500 - scrollVal, 172 );
	sampleStartKnob->move( 121 + 500 - scrollVal, 172 );
	sampleEndKnob->move( 145 + 500 - scrollVal, 172 );

	for( int i = 0; i < 8; ++i )
	{
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
		filtMutedToggle[i]->move( 169 + 1000 - scrollVal, i*92+35 - effectScrollVal );
	}

	subVolKnob->move( 23 + 250 - scrollVal, 172 + subIsFlipped );
	subEnabledToggle->move( 108 + 250 - scrollVal, 214 + subIsFlipped );
	subPhaseKnob->move( 180 + 250 - scrollVal, 172 + subIsFlipped );
	subPhaseRandKnob->move( 206 + 250 - scrollVal, 172 + subIsFlipped );
	subDetuneKnob->move( 95 + 250 - scrollVal, 172 + subIsFlipped );
	subMutedToggle->move( 147 + 250 - scrollVal, 214 + subIsFlipped );
	subKeytrackToggle->move( 108 + 250 - scrollVal, 229 + subIsFlipped );
	subSampLenKnob->move( 130 + 250 - scrollVal, 172 + subIsFlipped );
	subNoiseToggle->move( 147 + 250 - scrollVal, 229 + subIsFlipped );
	subPanningKnob->move( 55 + 250 - scrollVal, 172 + subIsFlipped );
	subTempoKnob->move( 35 + 250 - scrollVal, 172 + subIsNotFlipped );
	subRateLimitKnob->move( 63 + 250 - scrollVal, 172 + subIsNotFlipped );

	for( int i = 0; i < 8; ++i )
	{
		int matrixRemainder = modScrollVal % 460;
		int matrixDivide = modScrollVal / 460 * 4;

		modInBox[i]->move( 45 + 750 - scrollVal, i*115+57 - matrixRemainder );
		modInNumBox[i]->move( 90 + 750 - scrollVal, i*115+57 - matrixRemainder );
		modInAmntKnob[i]->move( 136 + 750 - scrollVal, i*115+53 - matrixRemainder );
		modInCurveKnob[i]->move( 161 + 750 - scrollVal, i*115+53 - matrixRemainder );
		modOutSecBox[i]->move( 27 + 750 - scrollVal, i*115+88 - matrixRemainder );
		modOutSigBox[i]->move( 69 + 750 - scrollVal, i*115+88 - matrixRemainder );
		modOutSecNumBox[i]->move( 112 + 750 - scrollVal, i*115+88 - matrixRemainder );
		modInBox2[i]->move( 45 + 750 - scrollVal, i*115+118 - matrixRemainder );
		modInNumBox2[i]->move( 90 + 750 - scrollVal, i*115+118 - matrixRemainder );
		modInAmntKnob2[i]->move( 136 + 750 - scrollVal, i*115+114 - matrixRemainder );
		modInCurveKnob2[i]->move( 161 + 750 - scrollVal, i*115+114 - matrixRemainder );
		modEnabledToggle[i]->move( 27 + 750 - scrollVal, i*115+36 - matrixRemainder );
		modCombineTypeBox[i]->move( 149 + 750 - scrollVal, i*115+88 - matrixRemainder );
		modTypeToggle[i]->move( 195 + 750 - scrollVal, i*115+67 - matrixRemainder );
		modType2Toggle[i]->move( 195 + 750 - scrollVal, i*115+128 - matrixRemainder );
		modUpArrow[i]->move( 181 + 750 - scrollVal, i*115+37 - matrixRemainder );
		modDownArrow[i]->move( 199 + 750 - scrollVal, i*115+37 - matrixRemainder );
		modNumText[i]->move( 192 + 750 - scrollVal, i*115+89 - matrixRemainder );

		if( i+matrixDivide < 64 )
		{
			modOutSecBox[i]->setModel( b->modOutSec[i+matrixDivide] );
			modOutSigBox[i]->setModel( b->modOutSig[i+matrixDivide] );
			modOutSecNumBox[i]->setModel( b->modOutSecNum[i+matrixDivide] );

			modInBox[i]->setModel( b->modIn[i+matrixDivide] );
			modInNumBox[i]->setModel( b->modInNum[i+matrixDivide] );
			modInAmntKnob[i]->setModel( b->modInAmnt[i+matrixDivide] );
			modInCurveKnob[i]->setModel( b->modInCurve[i+matrixDivide] );

			modInBox2[i]->setModel( b->modIn2[i+matrixDivide] );
			modInNumBox2[i]->setModel( b->modInNum2[i+matrixDivide] );
			modInAmntKnob2[i]->setModel( b->modInAmnt2[i+matrixDivide] );
			modInCurveKnob2[i]->setModel( b->modInCurve2[i+matrixDivide] );

			modEnabledToggle[i]->setModel( b->modEnabled[i+matrixDivide] );

			modCombineTypeBox[i]->setModel( b->modCombineType[i+matrixDivide] );

			modTypeToggle[i]->setModel( b->modType[i+matrixDivide] );
			modType2Toggle[i]->setModel( b->modType2[i+matrixDivide] );

			modNumText[i]->setText( QString::number(i+matrixDivide+1) );
		}
	}

	visvolKnob->move( 230 - scrollVal, 24 );
	scrollKnob->move( 0, 220 );

	loadChnlKnob->move( 1500 + 50 - scrollVal, 160 );
	visualizeToggle->move( 213 - scrollVal, 26 );
	subNumBox->move( 250 + 18 - scrollVal, 219 );
	sampNumBox->move( 500 + 18 - scrollVal, 219 );
	mainNumBox->move( 18 - scrollVal, 219 );
	graph->move( scrollVal >= 500 ? 500 + 23 - scrollVal : 23 , 30 );
	tabChanged( b->scroll.value() - 1 );
	openWavetableButton->move( 54 - scrollVal, 220 );
	openSampleButton->move( 54 + 500 - scrollVal, 220 );

	sinWaveBtn->move( 179 + 250 - scrollVal, 212 + subIsFlipped );
	triangleWaveBtn->move( 197 + 250 - scrollVal, 212 + subIsFlipped );
	sawWaveBtn->move( 215 + 250 - scrollVal, 212 + subIsFlipped );
	sqrWaveBtn->move( 179 + 250 - scrollVal, 227 + subIsFlipped );
	whiteNoiseWaveBtn->move( 197 + 250 - scrollVal, 227 + subIsFlipped );
	smoothBtn->move( 215 + 250 - scrollVal, 227 + subIsFlipped );
	usrWaveBtn->move( 54 + 250 - scrollVal, 220 );

	sinWave2Btn->move( 179 + 500 - scrollVal, 212 );
	triangleWave2Btn->move( 197 + 500 - scrollVal, 212 );
	sawWave2Btn->move( 215 + 500 - scrollVal, 212 );
	sqrWave2Btn->move( 179 + 500 - scrollVal, 227 );
	whiteNoiseWave2Btn->move( 197 + 500 - scrollVal, 227 );
	smooth2Btn->move( 215 + 500 - scrollVal, 227 );
	usrWave2Btn->move( 54 + 500 - scrollVal, 220 );

	oversampleBox->move( 30 + 1250 - scrollVal, 30 );

	effectScrollBar->move( 221 + 1000 - scrollVal, 32 );
	matrixScrollBar->move( 221 + 750 - scrollVal, 32 );

	filtForegroundLabel->move( 1000 - scrollVal, 0 );
	filtBoxesLabel->move( 1000 + 24 - scrollVal, 35 - ( effectScrollVal % 92 ) );

	matrixForegroundLabel->move( 750 - scrollVal, 0 );
	matrixBoxesLabel->move( 750 + 24 - scrollVal, 35 - ( modScrollVal % 115 ) );

	macroKnob[0]->move( 1250 + 30 - scrollVal, 100 );
	macroKnob[1]->move( 1250 + 60 - scrollVal, 100 );
	macroKnob[2]->move( 1250 + 90 - scrollVal, 100 );
	macroKnob[3]->move( 1250 + 120 - scrollVal, 100 );
	macroKnob[4]->move( 1250 + 30 - scrollVal, 130 );
	macroKnob[5]->move( 1250 + 60 - scrollVal, 130 );
	macroKnob[6]->move( 1250 + 90 - scrollVal, 130 );
	macroKnob[7]->move( 1250 + 120 - scrollVal, 130 );

	tab1Btn->move( 1, 48 );
	tab2Btn->move( 1, 63 );
	tab3Btn->move( 1, 78 );
	tab4Btn->move( 1, 93 );
	tab5Btn->move( 1, 108 );
	tab6Btn->move( 1, 123 );

	mainFlipBtn->move( 3 - scrollVal, 145 );
	subFlipBtn->move( 250 + 3 - scrollVal, 145 );

	loadModeBox->move( 1500 + 0 - scrollVal, 0 );

	manualBtn->move( 1250 + 50 - scrollVal, 200);

	confirmLoadButton->move( 1500 + 200 - scrollVal, 50);
	XBtn->move( 230 + 1500 - scrollVal, 10 + subIsFlipped );

	wtLoad1Knob->move( 1500 + 30 - scrollVal, 48 );
	wtLoad2Knob->move( 1500 + 60 - scrollVal, 48 );
	wtLoad3Knob->move( 1500 + 90 - scrollVal, 48 );
	wtLoad4Knob->move( 1500 + 120 - scrollVal, 48 );

	normalizeBtn->move( 175 + 1500 - scrollVal, 233 );
	desawBtn->move( 115 + 1500 - scrollVal, 233 );
}


// Snaps scroll to nearest tab when the scroll knob is released.
void MicrowaveView::scrollReleased()
{
	Microwave * b = castModel<Microwave>();

	const float scrollVal = b->scroll.value();
	b->scroll.setValue( round(scrollVal-0.25) );
}



void MicrowaveView::wheelEvent( QWheelEvent * _me )
{
	Microwave * b = castModel<Microwave>();

	if( _me->x() <= 18 && _me->y() >= 48 && _me->y() <= 138 )// If scroll over tab buttons
	{
		if( b->scroll.value() != 7 )
		{
			if( _me->delta() < 0 && b->scroll.value() != 6 )
			{
				b->scroll.setValue( b->scroll.value() + 1 );
			}
			else if( _me->delta() > 0 )
			{
				b->scroll.setValue( b->scroll.value() - 1 );
			}
		}
	}
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::mainNumChanged()
{
	Microwave * b = castModel<Microwave>();

	int mainNumValue = b->mainNum.value() - 1;

	morphKnob->setModel( b->morph[mainNumValue] );
	rangeKnob->setModel( b->range[mainNumValue] );
	sampLenKnob->setModel( b->sampLen[mainNumValue] );
	modifyKnob->setModel( b->modify[mainNumValue] );
	morphMaxKnob->setModel( b->morphMax[mainNumValue] );
	unisonVoicesKnob->setModel( b->unisonVoices[mainNumValue] );
	unisonDetuneKnob->setModel( b->unisonDetune[mainNumValue] );
	unisonMorphKnob->setModel( b->unisonMorph[mainNumValue] );
	unisonModifyKnob->setModel( b->unisonModify[mainNumValue] );
	detuneKnob->setModel( b->detune[mainNumValue] );
	modifyModeBox-> setModel( b-> modifyMode[mainNumValue] );
	phaseKnob->setModel( b->phase[mainNumValue] );
	phaseRandKnob->setModel( b->phaseRand[mainNumValue] );
	volKnob->setModel( b->vol[mainNumValue] );
	enabledToggle->setModel( b->enabled[mainNumValue] );
	mutedToggle->setModel( b->muted[mainNumValue] );
	panKnob->setModel( b->pan[mainNumValue] );
}


// Trades out the GUI elements when switching between oscillators, and adjusts graph length when needed
void MicrowaveView::subNumChanged()
{
	Microwave * b = castModel<Microwave>();

	b->graph.setLength( b->subSampLen[b->subNum.value()-1]->value() );
	for( int i = 0; i < 2048; ++i )
	{
		b->graph.setSampleAt( i, b->subs[(b->subNum.value()-1)*2048+i] );
	}

	int subNumValue = b->subNum.value() - 1;

	subVolKnob->setModel( b->subVol[subNumValue] );
	subEnabledToggle->setModel( b->subEnabled[subNumValue] );
	subPhaseKnob->setModel( b->subPhase[subNumValue] );
	subPhaseRandKnob->setModel( b->subPhaseRand[subNumValue] );
	subDetuneKnob->setModel( b->subDetune[subNumValue] );
	subMutedToggle->setModel( b->subMuted[subNumValue] );
	subKeytrackToggle->setModel( b->subKeytrack[subNumValue] );
	subSampLenKnob->setModel( b->subSampLen[subNumValue] );
	subNoiseToggle->setModel( b->subNoise[subNumValue] );
	subPanningKnob->setModel( b->subPanning[subNumValue] );
	subTempoKnob->setModel( b->subTempo[subNumValue] );
	subRateLimitKnob->setModel( b->subRateLimit[subNumValue] );
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::sampNumChanged()
{
	Microwave * b = castModel<Microwave>();

	for( int i = 0; i < 128; ++i )
	{
		b->graph.setSampleAt( i, b->sampGraphs[(b->sampNum.value()-1)*128+i] );
	}

	int sampNumValue = b->sampNum.value() - 1;

	sampleEnabledToggle->setModel( b->sampleEnabled[sampNumValue] );
	sampleGraphEnabledToggle->setModel( b->sampleGraphEnabled[sampNumValue] );
	sampleMutedToggle->setModel( b->sampleMuted[sampNumValue] );
	sampleKeytrackingToggle->setModel( b->sampleKeytracking[sampNumValue] );
	sampleLoopToggle->setModel( b->sampleLoop[sampNumValue] );

	sampleVolumeKnob->setModel( b->sampleVolume[sampNumValue] );
	samplePanningKnob->setModel( b->samplePanning[sampNumValue] );
	sampleDetuneKnob->setModel( b->sampleDetune[sampNumValue] );
	samplePhaseKnob->setModel( b->samplePhase[sampNumValue] );
	samplePhaseRandKnob->setModel( b->samplePhaseRand[sampNumValue] );
	sampleStartKnob->setModel( b->sampleStart[sampNumValue] );
	sampleEndKnob->setModel( b->sampleEnd[sampNumValue] );
}


// Moves/changes the GUI around depending on the mod out section value
void MicrowaveView::modOutSecChanged( int i )
{
	Microwave * b = castModel<Microwave>();

	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if( i-matrixDivide < 8 && i-matrixDivide >= 0 )
	{
		switch( b->modOutSec[i]->value() )
		{
			case 0:// None
			{
				modOutSigBox[i-matrixDivide]->hide();
				modOutSecNumBox[i-matrixDivide]->hide();
				break;
			}
			case 1:// Main OSC
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				b->modOutSig[i]->clear();
				mainoscsignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
				break;
			}
			case 2:// Sub OSC
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				b->modOutSig[i]->clear();
				subsignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
				break;
			}
			case 3:// Sample OSC
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				b->modOutSig[i]->clear();
				samplesignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
				break;
			}
			case 4:// Matrix Parameters
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				b->modOutSig[i]->clear();
				matrixsignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
				break;
			}
			case 5:// Filter Input
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->hide();
				b->modOutSig[i]->clear();
				mod8model( b->modOutSig[i] )
				break;
			}
			case 6:// Filter Parameters
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->hide();
				b->modOutSig[i]->clear();
				filtersignalsmodel( b->modOutSig[i] )
				break;
			}
			case 7:// Macro
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->hide();
				b->modOutSig[i]->clear();
				mod8model( b->modOutSig[i] )
				break;
			}
			default:
			{
				break;
			}
		}
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modInChanged( int i )
{
	Microwave * b = castModel<Microwave>();

	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if( i-matrixDivide < 8 && i-matrixDivide >= 0 )
	{
		switch( b->modIn[i]->value() )
		{
			case 0:
			{
				modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 1:// Main OSC
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 2:// Sub OSC
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 64, 1 );
				break;
			}
			case 3:// Sample OSC
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 4:// Filter
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 5:// Velocity
			{
				modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 6:// Panning
			{
				modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 7:// Humanizer
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 8:// Macro
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
		}
	}
}




// Does what is necessary when the user visits a new tab
void MicrowaveView::tabChanged( int tabnum )
{
	Microwave * b = castModel<Microwave>();

	b->currentTab = tabnum;

	updateBackground();

	if( tabnum != 0 )
	{
		tab1Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1" ) );
		tab1Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1" ) );
	}
	else
	{
		tab1Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
		tab1Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
	}

	if( tabnum != 1 )
	{
		tab2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
		tab2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
	}
	else
	{
		tab2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2_active" ) );
		tab2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2_active" ) );
	}

	if( tabnum != 2 )
	{
		tab3Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
		tab3Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
	}
	else
	{
		tab3Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3_active" ) );
		tab3Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3_active" ) );
	}

	if( tabnum != 3 )
	{
		tab4Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
		tab4Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
	}
	else
	{
		tab4Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4_active" ) );
		tab4Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4_active" ) );
	}

	if( tabnum != 4 )
	{
		tab5Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
		tab5Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
	}
	else
	{
		tab5Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5_active" ) );
		tab5Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5_active" ) );
	}

	if( tabnum != 5 )
	{
		tab6Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
		tab6Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
	}
	else
	{
		tab6Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6_active" ) );
		tab6Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6_active" ) );
	}
}



void MicrowaveView::updateBackground()
{
	Microwave * b = castModel<Microwave>();

	int backgroundnum = b->currentTab;
	bool mainFlipped = b->mainFlipped.value();
	bool subFlipped = b->subFlipped.value();

	switch( backgroundnum )
	{
		case 0:// Wavetable
		{
			b->graph.setLength( 2048 );
			mainNumChanged();

			if( !mainFlipped )
			{
				pal.setBrush( backgroundRole(), tab1ArtworkImg.copy() );
			}
			else
			{
				pal.setBrush( backgroundRole(), tab1FlippedArtworkImg.copy() );
			}

			setPalette( pal );
			break;
		}
		case 1:// Sub
		{
			subNumChanged();// Graph length is set in here

			if( !subFlipped )
			{
				pal.setBrush( backgroundRole(), tab2ArtworkImg.copy() );
			}
			else
			{
				pal.setBrush( backgroundRole(), tab2FlippedArtworkImg.copy() );
			}

			setPalette( pal );
			break;
		}
		case 2:// Sample
		{
			b->graph.setLength( 128 );
			sampNumChanged();

			pal.setBrush( backgroundRole(), tab3ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 3:// Matrix
		{
			pal.setBrush( backgroundRole(), tab4ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 4:// Effect
		{
			pal.setBrush( backgroundRole(), tab5ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 5:// Miscellaneous
		{
			pal.setBrush( backgroundRole(), tab6ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 6:// Wavetable Loading
		{
			pal.setBrush( backgroundRole(), tab7ArtworkImg.copy() );
			setPalette( pal );
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


void MicrowaveView::flipperClicked()
{
	updateBackground();
	updateScroll();
}


void MicrowaveView::XBtnClicked()
{
	castModel<Microwave>()->scroll.setValue( 0 );
}


void MicrowaveView::normalizeClicked()
{
	Microwave * b = castModel<Microwave>();

	int oscilNum = b->mainNum.value() - 1;

	for( int i = 0; i < 256; ++i )
	{
		float highestVolume = 0;
		for( int j = 0; j < 2048; ++j )
		{
			highestVolume = abs(b->waveforms[oscilNum][(i*2048)+j]) > highestVolume ? abs(b->waveforms[oscilNum][(i*2048)+j]) : highestVolume;
		}
		if( highestVolume )
		{
			float multiplierThing = 1.f / highestVolume;
			for( int j = 0; j < 2048; ++j )
			{
				b->waveforms[oscilNum][(i*2048)+j] *= multiplierThing;
			}
		}
	}

	Engine::getSong()->setModified();
}

void MicrowaveView::desawClicked()
{
	Microwave * b = castModel<Microwave>();

	int oscilNum = b->mainNum.value() - 1;

	float start;
	float end;
	for( int j = 0; j < 256; ++j )
	{
		start = -b->waveforms[oscilNum][j*2048];
		end = -b->waveforms[oscilNum][j*2048+2047];
		for( int i = 0; i < 2048; ++i )
		{
			b->waveforms[oscilNum][j*2048+i] += (i/2048.f)*end + ((2048.f-i)/2048.f)*start;
		}
	}

	Engine::getSong()->setModified();
}



void MicrowaveView::modUpClicked( int i )
{
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;
	if( i+matrixDivide > 0 )
	{
		castModel<Microwave>()->switchMatrixSections( i+matrixDivide, i+matrixDivide - 1 );
	}
}


void MicrowaveView::modDownClicked( int i )
{
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;
	if( i+matrixDivide < 63 )
	{
		castModel<Microwave>()->switchMatrixSections( i+matrixDivide, i+matrixDivide + 1 );
	}
}



void MicrowaveView::tabBtnClicked( int i )
{
	castModel<Microwave>()->scroll.setValue(i);
}




// Calls MicrowaveView::openWavetableFile when the wavetable opening button is clicked.
void MicrowaveView::openWavetableFileBtnClicked( )
{
	castModel<Microwave>()->scroll.setValue( 7 );
	chooseWavetableFile();
}


void MicrowaveView::chooseWavetableFile()
{
	SampleBuffer * sampleBuffer = new SampleBuffer;
	wavetableFileName = sampleBuffer->openAndSetWaveformFile();
	sharedObject::unref( sampleBuffer );
}


void MicrowaveView::confirmWavetableLoadClicked()
{
	openWavetableFile();
}


// All of the code and algorithms for loading wavetables from samples.  Please don't expect this code to look neat.
void MicrowaveView::openWavetableFile( QString fileName )
{
	Microwave * b = castModel<Microwave>();

	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	if( fileName.isEmpty() )
	{
		if( wavetableFileName.isEmpty() )
		{
			chooseWavetableFile();
		}
		fileName = wavetableFileName;
	}

	SampleBuffer * sampleBuffer = new SampleBuffer;

	sampleBuffer->setAudioFile( fileName );

	int filelength = sampleBuffer->sampleLength();
	int oscilNum = b->mainNum.value() - 1;
	int algorithm = b->loadMode.value();
	int channel = b->loadChnl.value();
	if( !fileName.isEmpty() )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		switch( algorithm )
		{
			case 0:// Load sample without changes
			{
				for( int i = 0; i < 524288; ++i )
				{
					if ( i <= lengthOfSample * 2.f )
					{
						b->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( (i/lengthOfSample)/2.f, channel );
					}
					else// Replace everything else with silence if sample isn't long enough
					{
						b->morphMax[oscilNum]->setValue( i/b->sampLen[oscilNum]->value() );
						b->morphMaxChanged();
						for( int j = i; j < 524288; ++j ) { b->waveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 1:// For loading wavetable files
			{
				for( int i = 0; i < 524288; ++i )
				{
					if ( i <= lengthOfSample )
					{
						b->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( i/lengthOfSample, channel );
					}
					else
					{
						b->morphMax[oscilNum]->setValue( i/b->sampLen[oscilNum]->value() );
						b->morphMaxChanged();
						for( int j = i; j < 524288; ++j ) { b->waveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 2:// Autocorrelation
			{
				// This uses a method called autocorrelation to detect the pitch.  It can get a few Hz off (especially at higher frequencies), so I also compare it with the zero crossings to see if I can get it even more accurate.

				// Estimate pitch using autocorrelation:

				float checkLength = qMin( 4000.f, lengthOfSample );// 4000 samples should be long enough to be able to accurately detect most frequencies this way

				float threshold = -1;
				float combined = 0;
				float oldcombined;
				int stage = 0;

				float period = 0;
				for( int i = 0; i < checkLength; ++i )
				{
					oldcombined = combined;
					combined = 0;
					for( int k = 0; k < checkLength - i; ++k )
					{
						combined += ( sampleBuffer->userWaveSample( k/lengthOfSample, channel ) * sampleBuffer->userWaveSample( (k+i)/lengthOfSample, channel ) + 1 ) * 0.5f - 0.5f;
					}

					if( stage == 2 && combined - oldcombined <= 0 )
					{
						stage = 3;
						period = i;
					}

					if( stage == 1 && combined > threshold && combined - oldcombined > 0 )
					{
						stage = 2;
					}

					if( !i )
					{
						threshold = combined * 0.5f;
						stage = 1;
					}
				}

				if( !period )
				{
					break;
				}

				cout<<sample_rate/period<<std::flush;

				// Now see if the zero crossings can aid in getting the pitch even more accurate:

				// Note:  If the zero crossings give a result very close to the autocorrelation, then it is likely to be more accurate.
				// Otherwise, the zero crossing result is probably very inaccurate (common with complex sounds) and is ignored.

				std::vector<float> crossings;
				crossings.push_back( 0 );
				std::vector<float> crossingsDif;
				bool above = ( sampleBuffer->userWaveSample( 1/lengthOfSample, channel ) > 0 );

				for( int i = 0; i < checkLength; ++i )
				{
					if( ( sampleBuffer->userWaveSample( i/lengthOfSample, channel ) > 0 ) != above )
					{
						above = !above;
						if( above )
						{
							crossingsDif.push_back( i - crossings[crossings.size() - 1] );
							crossings.push_back( i );
						}
					}
				}

				crossings.erase( crossings.begin() );

				if( crossingsDif.size() >= 3 )
				{
					float crossingsMean = std::accumulate( crossingsDif.begin(), crossingsDif.end(), 0.f ) / crossingsDif.size();
					std::vector<float> crossingsToRemove;
					for( int i = 0; i < crossingsDif.size(); ++i )
					{
						if( crossingsDif[i] < crossingsMean )
						{
							crossingsToRemove.push_back( i );
						}
					}
					for( int i = crossingsToRemove.size() - 1; i >= 0; --i )
					{
						crossingsDif.erase( crossingsDif.begin() + crossingsToRemove[i] );
					}
					if( crossingsDif.size() >= 2 )
					{
						float crossingsMedian = crossingsDif[int( crossingsDif.size() / 2.f )];
						cout<<crossingsMedian<<std::flush;
						if( abs( period - crossingsMedian ) < 5.f + period / 100.f )
						{
							period = crossingsMedian;
						}
					}
				}

				for( int i = 0; i < 524288; ++i )
				{
					b->waveforms[oscilNum][i] = sampleBuffer->userWaveSample( ((i/2048.f)*period)/lengthOfSample, channel );
				}

				b->morphMax[oscilNum]->setValue( 254 );
				b->morphMaxChanged();


				break;
			}
		}
		sampleBuffer->dataUnlock();
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
	Microwave * b = castModel<Microwave>();

	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();
	int oscilNum = b->sampNum.value() - 1;

	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	int filelength = sampleBuffer->sampleLength();
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		b->samples[oscilNum][0].clear();
		b->samples[oscilNum][1].clear();

		for( int i = 0; i < lengthOfSample; ++i )
		{
			b->samples[oscilNum][0].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 0));
			b->samples[oscilNum][1].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 1));
		}
		sampleBuffer->dataUnlock();
	}
	sharedObject::unref( sampleBuffer );
}



void MicrowaveView::dropEvent( QDropEvent * _de )
{
	int tabNum = castModel<Microwave>()->currentTab;
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		if( tabNum != 100000 )
		{
			openWavetableFile( value );
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
mSynth::mSynth( NotePlayHandle * _nph, const sample_rate_t _sample_rate, float * _phaseRandArr, int * _modifyModeArr, float * _modifyArr, float * _morphArr, float * _rangeArr, float * _unisonVoicesArr, float * _unisonDetuneArr, float * _unisonMorphArr, float * _unisonModifyArr, float * _morphMaxArr, float * _detuneArr, float waveforms[8][524288], float * _subsArr, bool * _subEnabledArr, float * _subVolArr, float * _subRateLimitArr, float * _subPhaseArr, float * _subPhaseRandArr, float * _subDetuneArr, bool * _subMutedArr, bool * _subKeytrackArr, float * _subSampLenArr, bool * _subNoiseArr, int * _sampLenArr, int * _modInArr, int * _modInNumArr, float * _modInAmntArr, float * _modInCurveArr, int * _modIn2Arr, int * _modInNum2Arr, float * _modInAmnt2Arr, float * _modInCurve2Arr, int * _modOutSecArr, int * _modOutSigArr, int * _modOutSecNumArr, int * modCombineTypeArr, bool * modTypeArr, bool * modType2Arr, float * _phaseArr, float * _volArr, float * _filtInVolArr, int * _filtTypeArr, int * _filtSlopeArr, float * _filtCutoffArr, float * _filtResoArr, float * _filtGainArr, float * _filtSatuArr, float * _filtWetDryArr, float * _filtBalArr, float * _filtOutVolArr, bool * _filtEnabledArr, bool * _enabledArr, bool * _modEnabledArr, float * sampGraphs, bool * _mutedArr, bool * _sampleEnabledArr, bool * _sampleGraphEnabledArr, bool * _sampleMutedArr, bool * _sampleKeytrackingArr, bool * _sampleLoopArr, float * _sampleVolumeArr, float * _samplePanningArr, float * _sampleDetuneArr, float * _samplePhaseArr, float * _samplePhaseRandArr, std::vector<float> (&samples)[8][2], float * _filtFeedbackArr, float * _filtDetuneArr, bool * _filtKeytrackingArr, float * _subPanningArr, float * _sampleStartArr, float * _sampleEndArr, float * _panArr, float * _subTempoArr, float * _macroArr, bool * _filtMutedArr ) :
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
	memcpy( modInAmnt, _modInAmntArr, sizeof(float) * 64 );
	memcpy( modInCurve, _modInCurveArr, sizeof(float) * 64 );
	memcpy( modIn2, _modIn2Arr, sizeof(int) * 64 );
	memcpy( modInNum2, _modInNum2Arr, sizeof(int) * 64 );
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
	memcpy( modType2, modType2Arr, sizeof(bool) * 64 );
	memcpy( filtMuted, _filtMutedArr, sizeof(bool) * 8 );
	memcpy( phaseRand, _phaseRandArr, sizeof(int) * 8 );
	memcpy( subRateLimit, _subRateLimitArr, sizeof(float) * 64 );

	memcpy( sampLen, _sampLenArr, sizeof(int) * 8 );

	memcpy( macro, _macroArr, sizeof(float) * 8 );

	for( int i=0; i < 8; ++i )
	{
		for( int j=0; j < 32; ++j )
		{
			// Randomize the phases of all of the waveforms
			sample_realindex[i][j] = int( ( ( fastRandf( sampLen[i] ) - ( sampLen[i] * 0.5f ) ) * ( phaseRand[i] * 0.01f ) ) + ( sampLen[i] * 0.5f ) ) % ( sampLen[i] );
		}
	}

	for( int i=0; i < 64; ++i )
	{
		sample_subindex[i] = 0;
		subNoiseDirection[i] = 1;
	}

	for( int i=0; i < 8; ++i )
	{
		sample_sampleindex[i] = fmod( fastRandf( samples[i][0].size() ) * ( samplePhaseRand[i] * 0.01f ), ( samples[i][0].size() *sampleEnd[i] ) - ( samples[i][0].size() * sampleStart[i] ) ) + ( samples[i][0].size() * sampleStart[i] );
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
std::vector<float> mSynth::nextStringSample( float (&waveforms)[8][524288], float * subs, float * sampGraphs, std::vector<float> (&samples)[8][2], int maxFiltEnabled, int maxModEnabled, int maxSubEnabled, int maxSampleEnabled, int maxMainEnabled, int sample_rate, Microwave * mwc )
{

	++noteDuration;

	sample_t sample[8][32] = {{0}};

	for( int l = 0; l < maxModEnabled; ++l )// maxModEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( modEnabled[l] )
		{
			switch( modIn[l] )
			{
				case 0:
				{
					curModVal[0] = 0;
					curModVal[1] = 0;
					break;
				}
				case 1:
				{
					if( modType[l] )// If envelope
					{
						curModVal[0] = lastMainOscEnvVal[modInNum[l]-1][0];
						curModVal[1] = lastMainOscEnvVal[modInNum[l]-1][1];
					}
					else
					{
						curModVal[0] = lastMainOscVal[modInNum[l]-1][0];
						curModVal[1] = lastMainOscVal[modInNum[l]-1][1];
					}
					break;
				}
				case 2:
				{
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
				}
				case 3:
				{
					curModVal[0] = lastSampleVal[modInNum[l]-1][0];
					curModVal[1] = lastSampleVal[modInNum[l]-1][1];
					break;
				}
				case 4:
				{
					curModVal[0] = filtModOutputs[modInNum[l]-1][0];
					curModVal[1] = filtModOutputs[modInNum[l]-1][1];
					break;
				}
				case 5:// Velocity
				{
					curModVal[0] = ( nph->getVolume() * 0.01f )-1;
					curModVal[1] = curModVal[0];
					break;
				}
				case 6:// Panning
				{
					curModVal[0] = ( nph->getPanning() * 0.01f );
					curModVal[1] = curModVal[0];
					break;
				}
				case 7:// Humanizer
				{
					curModVal[0] = humanizer[modInNum[l]-1];
					curModVal[1] = humanizer[modInNum[l]-1];
					break;
				}
				case 8:// Macro
				{
					curModVal[0] = macro[modInNum[l]-1] * 0.01f;
					curModVal[1] = macro[modInNum[l]-1] * 0.01f;
					break;
				}
				default:
				{
					switch( modCombineType[l] )
					{
						case 0:// Add Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 1:// Multiply Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 2:// Add Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
						case 3:// Multiply Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
					}
				}
			}
			switch( modIn2[l] )
			{
				case 0:
				{
					curModVal2[0] = 0;
					curModVal2[1] = 0;
					break;
				}
				case 1:
				{
					if( modType2[l] )// If envelope
					{
						curModVal2[0] = lastMainOscEnvVal[modInNum2[l]-1][0];
						curModVal2[1] = lastMainOscEnvVal[modInNum2[l]-1][1];
					}
					else
					{
						curModVal2[0] = lastMainOscVal[modInNum2[l]-1][0];
						curModVal2[1] = lastMainOscVal[modInNum2[l]-1][1];
					}
					break;
				}
				case 2:
				{
					if( modType2[l] )// If envelope
					{
						curModVal2[0] = lastSubEnvVal[modInNum2[l]-1][0];
						curModVal2[1] = lastSubEnvVal[modInNum2[l]-1][1];
					}
					else
					{
						curModVal2[0] = lastSubVal[modInNum2[l]-1][0];
						curModVal2[1] = lastSubVal[modInNum2[l]-1][1];
					}
					break;
				}
				case 3:
				{
					curModVal2[0] = lastSampleVal[modInNum2[l]-1][0];
					curModVal2[1] = lastSampleVal[modInNum2[l]-1][1];
					break;
				}
				case 4:
				{
					curModVal2[0] = filtModOutputs[modInNum2[l]-1][0];
					curModVal2[1] = filtModOutputs[modInNum2[l]-1][1];
				}
				case 5:// Velocity
				{
					curModVal2[0] = (nph->getVolume() * 0.01f)-1;
					curModVal2[1] = curModVal2[0];
					break;
				}
				case 6:// Panning
				{
					curModVal2[0] = (nph->getPanning() * 0.01f);
					curModVal2[1] = curModVal2[0];
					break;
				}
				case 7:// Humanizer
				{
					curModVal2[0] = humanizer[modInNum2[l]-1];
					curModVal2[1] = humanizer[modInNum2[l]-1];
					break;
				}
				case 8:// Macro
				{
					curModVal2[0] = macro[modInNum2[l]-1] * 0.01f;
					curModVal2[1] = macro[modInNum2[l]-1] * 0.01f;
					break;
				}
				default:
				{
					switch( modCombineType[l] )
					{
						case 0:// Add Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 1:// Multiply Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 2:// Add Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
						case 3:// Multiply Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
					}
				}
			}

			if( curModVal[0]  ) { curModVal[0]  *= modInAmnt[l]  * 0.01f; }// "if" statements are there to prevent unnecessary division if the modulation isn't used.
			if( curModVal[1]  ) { curModVal[1]  *= modInAmnt[l]  * 0.01f; }
			if( curModVal2[0] ) { curModVal2[0] *= modInAmnt2[l] * 0.01f; }
			if( curModVal2[1] ) { curModVal2[1] *= modInAmnt2[l] * 0.01f; }

			// Calculate curve
			if( modCombineType[l] <= 1 )// Bidirectional
			{
				if( modInCurve[l] != 100.f )// The "if" statement is there so unnecessary CPU isn't spent (pow is very expensive) if the curve knob isn't being used.
				{
					// Move to a scale of 0 to 1 (from -1 to 1) and then apply the curve.
					curModValCurve[0] = (curModVal[0] <= -1) ? ( curModVal[0] + 1 ) * 0.5f : pow( ( curModVal[0] + 1 ) * 0.5f, 1.f / ( modInCurve[l] * 0.01f ) );
					curModValCurve[1] = (curModVal[1] <= -1) ? ( curModVal[1] + 1 ) * 0.5f : pow( ( curModVal[1] + 1 ) * 0.5f, 1.f / ( modInCurve[l] * 0.01f ) );
				}
				else
				{
					curModValCurve[0] = ( curModVal[0] + 1 ) * 0.5f;
					curModValCurve[1] = ( curModVal[1] + 1 ) * 0.5f;
				}
				if( modInCurve2[l] != 100.f )
				{
					curModVal2Curve[0] = (curModVal2[0] <= -1) ? ( curModVal2[0] + 1 ) * 0.5f : pow( ( curModVal2[0] + 1 ) * 0.5f, 1.f / ( modInCurve2[l] * 0.01f ) );
					curModVal2Curve[1] = (curModVal2[1] <= -1) ? ( curModVal2[1] + 1 ) * 0.5f : pow( ( curModVal2[1] + 1 ) * 0.5f, 1.f / ( modInCurve2[l] * 0.01f ) );
				}
				else
				{
					curModVal2Curve[0] = ( curModVal2[0] + 1 ) * 0.5f;
					curModVal2Curve[1] = ( curModVal2[1] + 1 ) * 0.5f;
				}
			}
			else// Unidirectional
			{
				if( modInCurve[l] != 100.f )
				{
					curModValCurve[0] = ( (curModVal[0] <= -1) ? curModVal[0] : pow( abs( curModVal[0] ), 1.f / ( modInCurve[l] * 0.01f ) ) * ( curModVal[0] < 0 ? -1 : 1 ) ) + (modInAmnt[l] / 100.f);
					curModValCurve[1] = ( (curModVal[1] <= -1) ? curModVal[1] : pow( abs( curModVal[1] ), 1.f / ( modInCurve[l] * 0.01f ) ) * ( curModVal[1] < 0 ? -1 : 1 ) ) + (modInAmnt[l] / 100.f);
				}
				else
				{
					curModValCurve[0] = curModVal[0] + (modInAmnt[l] / 100.f);
					curModValCurve[1] = curModVal[1] + (modInAmnt[l] / 100.f);
				}
				if( modInCurve2[l] != 100.f )
				{
					curModVal2Curve[0] = ( (curModVal2[0] <= -1) ? curModVal2[0] : pow( abs( curModVal2[0] ), 1.f / ( modInCurve2[l] * 0.01f ) ) * ( curModVal2[0] < 0 ? -1 : 1 ) ) + (modInAmnt2[l] / 100.f);
					curModVal2Curve[1] = ( (curModVal2[1] <= -1) ? curModVal2[1] : pow( abs( curModVal2[0] ), 1.f / ( modInCurve2[l] * 0.01f ) ) * ( curModVal2[0] < 0 ? -1 : 1 ) ) + (modInAmnt2[l] / 100.f);
				}
				else
				{
					curModVal2Curve[0] = curModVal2[0] + (modInAmnt2[l] / 100.f);
					curModVal2Curve[1] = curModVal2[1] + (modInAmnt2[l] / 100.f);
				}
			}

			switch( modCombineType[l] )
			{
				case 0:// Add Bidirectional
				{
					comboModVal[0] = curModValCurve[0] + curModVal2Curve[0] - 1;
					comboModVal[1] = curModValCurve[1] + curModVal2Curve[1] - 1;
					break;
				}
				case 1:// Multiply Bidirectional
				{
					comboModVal[0] = curModValCurve[0] * curModVal2Curve[0] - 1;
					comboModVal[1] = curModValCurve[1] * curModVal2Curve[1] - 1;
					break;
				}
				case 2:// Add Unidirectional
				{
					comboModVal[0] = curModValCurve[0] + curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] + curModVal2Curve[1];
					break;
				}
				case 3:// Multiply Unidirectional
				{
					comboModVal[0] = curModValCurve[0] * curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] * curModVal2Curve[1];
					break;
				}
				default:
				{
					comboModVal[0] = 0;
					comboModVal[1] = 0;
				}
			}
			comboModValMono = ( comboModVal[0] + comboModVal[1] ) * 0.5f;
			switch( modOutSec[l] )
			{
				case 0:
				{
					break;
				}
				case 1:// Main Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Morph
						{
							morphVal[modOutSecNum[l]-1] = qBound( 0.f, morphVal[modOutSecNum[l]-1] + ( round((comboModValMono)*morphMaxVal[modOutSecNum[l]-1]) ), morphMaxVal[modOutSecNum[l]-1] );
							modValType.push_back( 1 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 2:// Send input to Range
						{
							rangeVal[modOutSecNum[l]-1] = qMax( 0.f, rangeVal[modOutSecNum[l]-1] + comboModValMono * 16.f );
							modValType.push_back( 2 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 3:// Send input to Modify
						{
							modifyVal[modOutSecNum[l]-1] = qMax( 0.f, modifyVal[modOutSecNum[l]-1] + comboModValMono * 2048.f );
							modValType.push_back( 3 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 4:// Send input to Pitch/Detune
						{
							detuneVal[modOutSecNum[l]-1] = detuneVal[modOutSecNum[l]-1] + comboModValMono * 4800.f;
							modValType.push_back( 11 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 5:// Send input to Phase
						{
							phase[modOutSecNum[l]-1] = fmod( phase[modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 13 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 6:// Send input to Volume
						{
							vol[modOutSecNum[l]-1] = qMax( 0.f, vol[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 14 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 7:// Send input to Panning
						{
							pan[modOutSecNum[l]-1] = qMax( 0.f, pan[modOutSecNum[l]-1] + comboModValMono * 200.f );
							modValType.push_back( 75 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 8:// Send input to Unison Voice Number
						{
							unisonVoices[modOutSecNum[l]-1] = qMax( 0.f, unisonVoices[modOutSecNum[l]-1] + comboModValMono * 32.f );
							modValType.push_back( 6 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 9:// Send input to Unison Detune
						{
							unisonDetune[modOutSecNum[l]-1] = qMax( 0.f, unisonDetune[modOutSecNum[l]-1] + comboModValMono * 2000.f );
							modValType.push_back( 7 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 10:// Send input to Unison Morph
						{
							unisonDetune[modOutSecNum[l]-1] = qBound( 0.f, unisonDetune[modOutSecNum[l]-1] + ( round((comboModValMono)*morphMaxVal[modOutSecNum[l]-1]) ), morphMaxVal[modOutSecNum[l]-1] );
							modValType.push_back( 8 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 11:// Send input to Unison Modify
						{
							unisonModify[modOutSecNum[l]-1] = qMax( 0.f, unisonModify[modOutSecNum[l]-1] + comboModValMono * 2048.f );
							modValType.push_back( 9 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
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
						{
							break;
						}
						case 1:// Send input to Pitch/Detune
						{
							subDetune[modOutSecNum[l]-1] = subDetune[modOutSecNum[l]-1] + comboModValMono * 4800.f;
							modValType.push_back( 30 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 2:// Send input to Phase
						{
							subPhase[modOutSecNum[l]-1] = fmod( subPhase[modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 28 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 3:// Send input to Volume
						{
							subVol[modOutSecNum[l]-1] = qMax( 0.f, subVol[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 27 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 4:// Send input to Panning
						{
							subPanning[modOutSecNum[l]-1] = qMax( 0.f, subPanning[modOutSecNum[l]-1] + comboModValMono * 200.f );
							modValType.push_back( 72 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 5:// Send input to Sample Length
						{
							subSampLen[modOutSecNum[l]-1] = qMax( 0, int( subSampLen[modOutSecNum[l]-1] + comboModValMono * 2048.f ) );
							modValType.push_back( 33 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 6:// Send input to Rate Limit
						{
							subRateLimit[modOutSecNum[l]-1] = qMax( 0.f, subRateLimit[modOutSecNum[l]-1] + comboModValMono * 2.f );
							modValType.push_back( 82 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						default:
						{
						}
					}
					break;
				}
				case 3:// Sample Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Pitch/Detune
						{
							sampleDetune[modOutSecNum[l]-1] = sampleDetune[modOutSecNum[l]-1] + comboModValMono * 4800.f;
							modValType.push_back( 66 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 2:// Send input to Phase
						{
							samplePhase[modOutSecNum[l]-1] = fmod( samplePhase[modOutSecNum[l]-1] + comboModValMono, 100 );
							modValType.push_back( 67 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 3:// Send input to Volume
						{
							sampleVolume[modOutSecNum[l]-1] = qMax( 0.f, sampleVolume[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType.push_back( 64 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						default:
						{
						}
					}
					break;
				}
				case 4:// Matrix
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Mod In Amount
						{
							modInAmnt[modOutSecNum[l]-1] = modInAmnt[modOutSecNum[l]-1] + comboModValMono*100.f;
							modValType.push_back( 38 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 2:// Mod In Curve
						{
							modInCurve[modOutSecNum[l]-1] = qMax( 0.f, modInCurve[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 39 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 3:// Secondary Mod In Amount
						{
							modInAmnt2[modOutSecNum[l]-1] = modInAmnt2[modOutSecNum[l]-1] + comboModValMono*100.f;
							modValType.push_back( 43 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 4:// Secondary Mod In Curve
						{
							modInCurve2[modOutSecNum[l]-1] = qMax( 0.f, modInCurve2[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 44 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
					}
					break;
				}
				case 5:// Filter Input
				{
					filtInputs[modOutSig[l]][0] += curModVal[0];
					filtInputs[modOutSig[l]][1] += curModVal[1];
					break;
				}
				case 6:// Filter Parameters
				{
					switch( modOutSig[l] )
					{
						case 0:// None
						{
							break;
						}
						case 1:// Cutoff Frequency
						{
							filtCutoff[modOutSecNum[l]-1] = qBound( 20.f, filtCutoff[modOutSecNum[l]-1] + comboModValMono*19980.f, 21999.f );
							modValType.push_back( 18 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 2:// Resonance
						{
							filtReso[modOutSecNum[l]-1] = qMax( 0.0001f, filtReso[modOutSecNum[l]-1] + comboModValMono*16.f );
							modValType.push_back( 19 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 3:// dbGain
						{
							filtGain[modOutSecNum[l]-1] = filtGain[modOutSecNum[l]-1] + comboModValMono*64.f;
							modValType.push_back( 20 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 4:// Filter Type
						{
							filtType[modOutSecNum[l]-1] = qMax( 0, int(filtType[modOutSecNum[l]-1] + comboModValMono*7.f ) );
							modValType.push_back( 16 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 5:// Filter Slope
						{
							filtSlope[modOutSecNum[l]-1] = qMax( 0, int(filtSlope[modOutSecNum[l]-1] + comboModValMono*8.f ) );
							modValType.push_back( 17 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 6:// Input Volume
						{
							filtInVol[modOutSecNum[l]-1] = qMax( 0.f, filtInVol[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 15 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 7:// Output Volume
						{
							filtOutVol[modOutSecNum[l]-1] = qMax( 0.f, filtOutVol[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 24 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 8:// Wet/Dry
						{
							filtWetDry[modOutSecNum[l]-1] = qBound( 0.f, filtWetDry[modOutSecNum[l]-1] + comboModValMono*100.f, 100.f );
							modValType.push_back( 22 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 9:// Balance/Panning
						{
							filtWetDry[modOutSecNum[l]-1] = qBound( -100.f, filtWetDry[modOutSecNum[l]-1] + comboModValMono*200.f, 100.f );
							modValType.push_back( 23 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 10:// Saturation
						{
							filtSatu[modOutSecNum[l]-1] = qMax( 0.f, filtSatu[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 21 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 11:// Feedback
						{
							filtFeedback[modOutSecNum[l]-1] = qMax( 0.f, filtFeedback[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType.push_back( 69 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
						case 12:// Detune
						{
							filtDetune[modOutSecNum[l]-1] = qMax( 0.f, filtDetune[modOutSecNum[l]-1] + comboModValMono*4800.f );
							modValType.push_back( 70 );
							modValNum.push_back( modOutSecNum[l]-1 );
							break;
						}
					}
					break;
				}
				case 7:// Macro
				{
					macro[modOutSig[l]] = macro[modOutSig[l]] + comboModValMono * 200.f;
					modValType.push_back( 78 );
					modValNum.push_back( modOutSig[l] );
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

	// As much as it may seem like it, this section contains no intentional attempts at obfuscation.

	for( int l = 0; l < maxFiltEnabled; ++l )
	{
		if( filtEnabled[l] )
		{
			temp1 = filtInVol[l] * 0.01f;
			filtInputs[l][0] *= temp1;
			filtInputs[l][1] *= temp1;

			if( filtKeytracking[l] )
			{
				temp1 = round( sample_rate / detuneWithCents( nph->frequency(), filtDetune[l] ) );
			}
			else
			{
				temp1 = round( sample_rate / detuneWithCents( 440.f, filtDetune[l] ) );
			}
			if( filtDelayBuf[l][0].size() != temp1 )
			{
				filtDelayBuf[l][0].resize( temp1 );
				filtDelayBuf[l][1].resize( temp1 );
			}

			++filtFeedbackLoc[l];
			if( filtFeedbackLoc[l] > filtDelayBuf[l][0].size() - 1 )
			{
				filtFeedbackLoc[l] = 0;
			}

			filtInputs[l][0] += filtDelayBuf[l][0].at( filtFeedbackLoc[l] );
			filtInputs[l][1] += filtDelayBuf[l][1].at( filtFeedbackLoc[l] );

			cutoff = filtCutoff[l];
			mode = filtType[l];
			reso = filtReso[l];
			dbgain = filtGain[l];
			Fs = sample_rate;
			w0 = F_2PI * cutoff / Fs;

			if( mode == 8 )
			{
				f = 2 * cutoff / Fs;
				k = 3.6*f - 1.6*f*f - 1;
				p = (k+1)*0.5f;
				scale = pow( 2.7182818284590452353602874713527f, (1-p)*1.386249f );
				r = reso*scale;
			}

			for( int m = 0; m < filtSlope[l] + 1; ++m )// m is the slope number.  So if m = 2, then the sound is going from a 24 db to a 36 db slope, for example.
			{
				if( m )
				{
					filtInputs[l][0] = filtOutputs[l][0];
					filtInputs[l][1] = filtOutputs[l][1];
				}

				int formulaType = 1;
				if( mode <= 7 )
				{
					switch( mode )
					{
						case 0:// LP
						{
							temp1 = cos(w0);
							alpha = sin(w0) / (2*reso);
							b0 = ( 1 - temp1 ) * 0.5f;
							b1 = 1 - temp1;
							b2 = b0;
							a0 = 1 + alpha;
							a1 = -2 * temp1;
							a2 = 1 - alpha;
							break;
						}
						case 1:// HP
						{
							temp1 = cos(w0);
							alpha = sin(w0) / (2*reso);
							b0 =  (1 + temp1) * 0.5f;
							b1 = -(1 + temp1);
							b2 =  b0;
							a0 =   1 + alpha;
							a1 =  -2 * temp1;
							a2 =   1 - alpha;
							break;
						}
						case 2:// BP
						{
							temp2 = sin(w0);
							alpha = temp2*sinh( (log(2)/log(2.7182818284590452353602874713527)) * 0.5f * reso * w0/temp2 );
							b0 =   alpha;
							b1 =   0;
							b2 =  -alpha;
							a0 =   1 + alpha;
							a1 =  -2*cos(w0);
							a2 =   1 - alpha;
							formulaType = 2;
							break;
						}
						case 3:// Low Shelf
						{
							temp1 = cos(w0);
							alpha = sin(w0) / (2*reso);
							A = exp10( dbgain / 40 );
							temp2 = 2*sqrt(A)*alpha;
							b0 =    A*( (A+1) - (A-1)*temp1 + temp2 );
							b1 =  2*A*( (A-1) - (A+1)*temp1         );
							b2 =    A*( (A+1) - (A-1)*temp1 - temp2 );
							a0 =        (A+1) + (A-1)*temp1 + temp2;
							a1 =   -2*( (A-1) + (A+1)*temp1         );
							a2 =        (A+1) + (A-1)*temp1 - temp2;
							break;
						}
						case 4:// High Shelf
						{
							temp1 = cos(w0);
							alpha = sin(w0) / (2*reso);
							A = exp10( dbgain / 40 );
							temp2 = 2*sqrt(A)*alpha;
							b0 =    A*( (A+1) + (A-1)*temp1 + temp2 );
							b1 = -2*A*( (A-1) + (A+1)*temp1         );
							b2 =    A*( (A+1) + (A-1)*temp1 - temp2 );
							a0 =        (A+1) - (A-1)*temp1 + temp2;
							a1 =    2*( (A-1) - (A+1)*temp1         );
							a2 =        (A+1) - (A-1)*temp1 - temp2;
							break;
						}
						case 5:// Peak
						{
							temp1 = -2 * cos(w0);
							temp2 = sin(w0);
							alpha = temp2*sinh( (log(2)/log(2.7182818284590452353602874713527))/2 * reso * w0/temp2 );
							A = exp10( dbgain / 40 );
							b0 =   1 + alpha*A;
							b1 =  temp1;
							b2 =   1 - alpha*A;
							a0 =   1 + alpha/A;
							a1 =  temp1;
							a2 =   1 - alpha/A;
							break;
						}
						case 6:// Notch
						{
							temp1 = -2 * cos(w0);
							temp2 = sin(w0);
							alpha = temp2*sinh( (log(2)/log(2.7182818284590452353602874713527))/2 * reso * w0/temp2 );
							b0 =   1;
							b1 =  temp1;
							b2 =   1;
							a0 =   1 + alpha;
							a1 =  temp1;
							a2 =   1 - alpha;
							break;
						}
						case 7:// Allpass
						{
							temp1 = -2 * cos(w0);
							alpha = sin(w0) / (2*reso);
							b0 = 1 - alpha;
							b1 = temp1;
							b2 = 1 + alpha;
							a0 = b2;
							a1 = temp1;
							a2 = b0;
							break;
						}
					}

					// Translation of this monstrosity (case 1): y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
					// See www.musicdsp.org/files/Audio-EQ-Cookbook.txt
					switch( formulaType )
					{
						case 1:// Normal
						{
							filtPrevSampOut[l][m][0][0] = (b0/a0)*filtInputs[l][0] + (b1/a0)*filtPrevSampIn[l][m][1][0] + (b2/a0)*filtPrevSampIn[l][m][2][0] - (a1/a0)*filtPrevSampOut[l][m][1][0] - (a2/a0)*filtPrevSampOut[l][m][2][0];// Left ear
							filtPrevSampOut[l][m][0][1] = (b0/a0)*filtInputs[l][1] + (b1/a0)*filtPrevSampIn[l][m][1][1] + (b2/a0)*filtPrevSampIn[l][m][2][1] - (a1/a0)*filtPrevSampOut[l][m][1][1] - (a2/a0)*filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 2:// Bandpass
						{
							filtPrevSampOut[l][m][0][0] = (b0/a0)*filtInputs[l][0] + (b2/a0)*filtPrevSampIn[l][m][2][0] - (a1/a0)*filtPrevSampOut[l][m][1][0] - (a2/a0)*filtPrevSampOut[l][m][2][0];// Left ear
							filtPrevSampOut[l][m][0][1] = (b0/a0)*filtInputs[l][1] + (b2/a0)*filtPrevSampIn[l][m][2][1] - (a1/a0)*filtPrevSampOut[l][m][1][1] - (a2/a0)*filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
					}

					//Output results
					temp1 = filtOutVol[l] * 0.01f;
					filtOutputs[l][0] = filtPrevSampOut[l][m][0][0] * temp1;
					filtOutputs[l][1] = filtPrevSampOut[l][m][0][1] * temp1;

				}
				else if( mode == 8 )
				{
					// Moog 24db Lowpass
					filtx[0] = filtInputs[l][0]-r*filty4[0];
					filtx[1] = filtInputs[l][1]-r*filty4[1];
					for( int i = 0; i < 2; ++i )
					{
						filty1[i]=filtx[i]*p + filtoldx[i]*p - k*filty1[i];
						filty2[i]=filty1[i]*p+filtoldy1[i]*p - k*filty2[i];
						filty3[i]=filty2[i]*p+filtoldy2[i]*p - k*filty3[i];
						filty4[i]=filty3[i]*p+filtoldy3[i]*p - k*filty4[i];
						filty4[i] = filty4[i] - pow(filty4[i], 3) / 6.f;
						filtoldx[i] = filtx[i];
						filtoldy1[i] = filty1[i];
						filtoldy2[i] = filty2[i];
						filtoldy3[i] = filty3[i];
					}
					temp1 = filtOutVol[l] * 0.01f;
					filtOutputs[l][0] = filty4[0] * temp1;
					filtOutputs[l][1] = filty4[1] * temp1;
				}

				// Calculates Saturation.  It looks ugly, but the algorithm is just y = x ^ ( 1 - saturation );
				temp1 = 1 - ( filtSatu[l] * 0.01f );
				filtOutputs[l][0] = pow( abs( filtOutputs[l][0] ), temp1 ) * ( filtOutputs[l][0] < 0 ? -1 : 1 );
				filtOutputs[l][1] = pow( abs( filtOutputs[l][1] ), temp1 ) * ( filtOutputs[l][1] < 0 ? -1 : 1 );

				// Balance knob wet
				filtOutputs[l][0] *= filtBal[l] > 0 ? ( 100.f - filtBal[l] ) * 0.01f : 1.f;
				filtOutputs[l][1] *= filtBal[l] < 0 ? ( 100.f + filtBal[l] ) * 0.01f : 1.f;

				// Wet
				temp1 = filtWetDry[l] * 0.01f;
				filtOutputs[l][0] *= temp1;
				filtOutputs[l][1] *= temp1;

				// Balance knob dry
				filtOutputs[l][0] += ( 1.f - ( filtBal[l] > 0 ? ( 100.f - filtBal[l] ) * 0.01f : 1.f ) ) * filtInputs[l][0];
				filtOutputs[l][1] += ( 1.f - ( filtBal[l] < 0 ? ( 100.f + filtBal[l] ) * 0.01f : 1.f ) ) * filtInputs[l][1];

				// Dry
				temp1 = 1.f - ( filtWetDry[l] * 0.01f );
				filtOutputs[l][0] += temp1 * filtInputs[l][0];
				filtOutputs[l][1] += temp1 * filtInputs[l][1];

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

			temp1 = filtFeedback[l] * 0.01f;
			filtDelayBuf[l][0][filtFeedbackLoc[l]] = filtOutputs[l][0] * temp1;
			filtDelayBuf[l][1][filtFeedbackLoc[l]] = filtOutputs[l][1] * temp1;

			filtInputs[l][0] = 0;
			filtInputs[l][1] = 0;

			filtModOutputs[l][0] = filtOutputs[l][0];
			filtModOutputs[l][1] = filtOutputs[l][1];

			if( filtMuted[l] )
			{
				filtOutputs[l][0] = 0;
				filtOutputs[l][1] = 0;
			}
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
				if( unisonMorph[i] )
				{
					temp1 = (((unisonVoicesMinusOne-l)/unisonVoicesMinusOne)*unisonMorph[i]);
					morphVal[i] = qBound( 0.f, temp1 - ( unisonMorph[i] * 0.5f ) + morphVal[i], morphMaxVal[i] );
				}

				if( unisonModify[i] )
				{
					temp1 = ((unisonVoicesMinusOne-l)/unisonVoicesMinusOne)*unisonModify[i];
					modifyVal[i] = qBound( 0.f, temp1 - ( unisonModify[i] * 0.5f ) + modifyVal[i], sampLen[i]-1.f);// SampleLength - 1 = ModifyMax
				}
			}

			sample_length_modify[i][l] = 0;
			switch( modifyModeVal[i] )// Horrifying formulas for the various Modify Modes
			{
				case 0:// None
				{
					break;
				}
				case 1:// Pulse Width
				{
					sample_morerealindex[i][l] /= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];

					sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sampLen[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds
					break;
				}
				case 2:// Weird 1
				{
					sample_morerealindex[i][l] = ( ( ( sin( ( ( ( sample_morerealindex[i][l] ) / ( sampLen[i] ) ) * ( modifyVal[i] / 50.f ) ) / 2 ) ) * ( sampLen[i] ) ) * ( modifyVal[i] / sampLen[i] ) + ( sample_morerealindex[i][l] + ( ( -modifyVal[i] + sampLen[i] ) / sampLen[i] ) ) ) / 2.f;
					break;
				}
				case 3:// Weird 2
				{
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
				}
				case 4:// Asym To Right
				{
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					break;
				}
				case 5:// Asym To Left
				{
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					sample_morerealindex[i][l] = pow( ( ( ( sample_morerealindex[i][l] ) / sampLen[i] ) ), (modifyVal[i]*10/sampLen[i])+1 ) * ( sampLen[i] );
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					break;
				}
				case 6:// Stretch From Center
				{
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= sampLen[i] / 2.f;
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] + 1 ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] + 1 ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				}
				case 7:// Squish To Center
				{
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sampLen[i] + 1  ) ) : -pow( -sample_morerealindex[i][l], 1 / ( -modifyVal[i] / sampLen[i] + 1 ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				}
				case 8:// Stretch And Squish
				{
					sample_morerealindex[i][l] -= sampLen[i] / 2.f;
					sample_morerealindex[i][l] /= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( modifyVal[i] * 4 ) / sampLen[i] ) );
					sample_morerealindex[i][l] *= ( sampLen[i] / 2.f );
					sample_morerealindex[i][l] += sampLen[i] / 2.f;
					break;
				}
				case 9:// Cut Off Right
				{
					sample_morerealindex[i][l] *= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					break;
				}
				case 10:// Cut Off Left
				{
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					sample_morerealindex[i][l] *= ( -modifyVal[i] + sampLen[i] ) / sampLen[i];
					sample_morerealindex[i][l] = -sample_morerealindex[i][l] + sampLen[i];
					break;
				}
				default:
					{}
			}
			//sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], sampLen[i] + sample_length_modify[i][l] - 1);// Keeps sample index within bounds

			sample[i][l] = 0;

			loopStart = qMax( 0.f, morphVal[i] - rangeVal[i] ) + 1;
			loopEnd = qMin( morphVal[i] + rangeVal[i], 524288.f / sampLen[i] ) + 1;
			currentRangeValInvert = 1.f / rangeVal[i];// Prevents repetitive division in the loop below.
			currentSampLen = sampLen[i];
			currentIndex = sample_morerealindex[i][l];
			if( modifyModeVal[i] != 11 && modifyModeVal[i] != 12 )// If NOT Squarify or Pulsify Modify Mode
			{
				// Only grab samples from the waveforms when they will be used, depending on the Range knob
				for( int j = loopStart; j < loopEnd; ++j )
				{
					// Get waveform samples, set their volumes depending on Range knob
					sample[i][l] += waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert ));
				}
			}
			else if( modifyModeVal[i] == 11 )// If Squarify Modify Mode
			{
				// Self-made formula, may be buggy.  Crossfade one half of waveform with the inverse of the other.
				// Some major CPU improvements can be made here.
				for( int j = loopStart; j < loopEnd; ++j )
				{
					sample[i][l] +=
					(    waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) + // Normal
					( ( -waveforms[i][( ( currentIndex + ( currentSampLen / 2 ) ) % currentSampLen ) + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) * ( ( modifyVal[i] * 2.f ) / currentSampLen ) ) / // Inverted other half of waveform
					( ( modifyVal[i] / currentSampLen ) + 1 ); // Volume compensation
				}
			}
			else if( modifyModeVal[i] == 12 )// Pulsify Mode
			{
				// Self-made formula, may be buggy.  Crossfade one side of waveform with the inverse of the other.
				// Some major CPU improvements can be made here.
				for( int j = loopStart; j < loopEnd; ++j )
				{
					sample[i][l] +=
					(    waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) + // Normal
					( ( -waveforms[i][( ( currentIndex + int( currentSampLen * ( modifyVal[i] / currentSampLen ) ) ) % currentSampLen ) + j * currentSampLen] * ( 1 - ( abs( morphVal[i] - j ) * currentRangeValInvert )) ) * 2.f ) / // Inverted other side of waveform
					2.f; // Volume compensation
				}
			}

			switch( modifyModeVal[i] )// More horrifying formulas for the various Modify Modes
			{
				case 1:// Pulse Width
				{
					if( sample_realindex[i][l] / ( ( -modifyVal[i] + sampLen[i] ) / sampLen[i] ) > sampLen[i] )
					{
						sample[i][l] = 0;
					}
					break;
				}
				case 13:// Flip
				{
					if( sample_realindex[i][l] > modifyVal[i] )
					{
						sample[i][l] *= -1;
					}
					break;
				}
				case 14:// Clip
				{
					temp1 = (modifyVal[i]/( sampLen[i] - 1 ));
					sample[i][l] = qBound( -temp1, sample[i][l], temp1 );
					break;
				}
				case 15:// Inverse Clip
				{
					temp1 = (modifyVal[i]/( sampLen[i] - 1 ));
					if( sample[i][l] > 0 && sample[i][l] < temp1 )
					{
						sample[i][l] = temp1;
					}
					else if( sample[i][l] < 0 && sample[i][l] > -temp1 )
					{
						sample[i][l] = -temp1;
					}
					break;
				}
				case 16:// Sine
				{
					temp1 = (modifyVal[i]/( sampLen[i] - 1 ));
					sample[i][l] = sin( sample[i][l] * ( F_PI * temp1 * 8 + 1 ) );
					break;
				}
				case 17:// Atan
				{
					temp1 = (modifyVal[i]/( sampLen[i] - 1 ));
					sample[i][l] = atan( sample[i][l] * ( F_PI * temp1 * 8 + 1 ) );
					break;
				}
				case 18:// Sin(x)/x
				{
					temp1 = (modifyVal[i]/( sampLen[i] - 1 ));
					sample[i][l] = sin( F_PI * sample[i][l] * ( F_PI * temp1 * 8 + 1 ) ) / ( F_PI * sample[i][l] * ( F_PI * temp1 * 8 + 1 ) ) - 0.5;
					break;
				}
				default:
					{}
			}

			sample_realindex[i][l] += sample_step[i][l];

			// check overflow
			temp1 = sampLen[i] + sample_length_modify[i][l];
			while( sample_realindex[i][l] >= temp1 )
			{
				sample_realindex[i][l] -= temp1;
				lastMainOscEnvDone[l] = true;
			}

			sample[i][l] = sample[i][l] * vol[i] * 0.01f;
		}
	}

	//====================//
	//== SUB OSCILLATOR ==//
	//====================//

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

				subsample[l][0] = ( subVol[l] * 0.01f ) * subs[int( sample_subindex[l] + ( subPhase[l] * subSampLen[l] ) ) % int(subSampLen[l]) + ( 2048 * l )];
				subsample[l][1] = subsample[l][0];

				if( subPanning[l] < 0 )
				{
					subsample[l][1] *= ( 100.f + subPanning[l] ) * 0.01f;
				}
				else if( subPanning[l] > 0 )
				{
					subsample[l][0] *= ( 100.f - subPanning[l] ) * 0.01f;
				}

				if( subRateLimit[l] )
				{
					subsample[l][0] = lastSubVal[l][0] + qBound( -subRateLimit[l], subsample[l][0] - lastSubVal[l][0], subRateLimit[l] );
					subsample[l][1] = lastSubVal[l][1] + qBound( -subRateLimit[l], subsample[l][1] - lastSubVal[l][1], subRateLimit[l] );
				}

				lastSubVal[l][0] = subsample[l][0];// Store value for matrix
				lastSubVal[l][1] = subsample[l][1];

				if( !lastSubEnvDone[l] )
				{
					lastSubEnvVal[l][0] = subsample[l][0];// Store envelope value for matrix
					lastSubEnvVal[l][1] = subsample[l][1];
				}

				sample_subindex[l] += sample_step_sub;

				// move waveform position back if it passed the end of the waveform
				while ( sample_subindex[l] >= subSampLen[l] )
				{
					sample_subindex[l] -= subSampLen[l];
					lastSubEnvDone[l] = true;
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
			}
			else// sub oscillator is noise
			{
				noiseSampRand = fastRandf( subSampLen[l] ) - 1;

				temp1 = ( subs[int(noiseSampRand)] * subNoiseDirection[l] ) + lastSubVal[l][0];
				if( temp1 > 1 || temp1 < -1 )
				{
					subNoiseDirection[l] *= -1;
					temp1 = ( subs[int(noiseSampRand)] * subNoiseDirection[l] ) + lastSubVal[l][0];
				}

				subsample[l][0] = temp1 * ( subVol[l] * 0.01f );// Division by 1.2f to tame DC offset
				subsample[l][1] = subsample[l][0];

				if( subRateLimit[l] )
				{
					subsample[l][0] = lastSubVal[l][0] + qBound( -subRateLimit[l], subsample[l][0] - lastSubVal[l][0], subRateLimit[l] );
					subsample[l][1] = lastSubVal[l][1] + qBound( -subRateLimit[l], subsample[l][1] - lastSubVal[l][1], subRateLimit[l] );
				}

				lastSubVal[l][0] = subsample[l][0];
				lastSubVal[l][1] = subsample[l][1];

				lastSubEnvVal[l][0] = subsample[l][0];// Store envelope value for matrix
				lastSubEnvVal[l][1] = subsample[l][1];
			}

			// Mutes sub after saving value for modulation if the muted option is on
			if( subMuted[l] )
			{
				subsample[l][0] = 0;
				subsample[l][1] = 0;
			}
		}
		else
		{
			lastSubVal[l][0] = 0;
			lastSubVal[l][1] = 0;
			lastSubEnvVal[l][0] = 0;
			lastSubEnvVal[l][1] = 0;
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
					lastSampleEnvDone[l] = true;
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
				float progress = fmod( ( sample_sampleindex[l] + ( samplePhase[l] * sampleSize * 0.01f ) ), sampleSize ) / sampleSize * 128.f;
				int intprogress = int(progress);

				temp1 = fmod( progress, 1 );
				float progress2 = sampGraphs[intprogress] * ( 1 - temp1 );

				float progress3;
				if( intprogress+1 < 128 )
				{
					progress3 = sampGraphs[intprogress+1] * temp1;
				}
				else
				{
					progress3 = sampGraphs[intprogress] * temp1;
				}

				temp1 = int( ( ( progress2 + progress3 + 1 ) * 0.5f ) * sampleSize );
				samplesample[l][0] = samples[l][0][temp1];
				samplesample[l][1] = samples[l][1][temp1];
			}
			else
			{
				temp1 = fmod(( sample_sampleindex[l] + ( samplePhase[l] * sampleSize * 0.01f ) ), sampleSize);
				samplesample[l][0] = samples[l][0][temp1];
				samplesample[l][1] = samples[l][1][temp1];
			}

			temp1 = sampleVolume[l] * 0.01f;
			samplesample[l][0] *= temp1;
			samplesample[l][1] *= temp1;

			if( samplePanning[l] < 0 )
			{
				samplesample[l][1] *= ( 100.f + samplePanning[l] ) * 0.01f;
			}
			else if( samplePanning[l] > 0 )
			{
				samplesample[l][0] *= ( 100.f - samplePanning[l] ) * 0.01f;
			}

			lastSampleVal[l][0] = samplesample[l][0];// Store value for modulation
			lastSampleVal[l][1] = samplesample[l][1];// Store value for modulation

			if( !lastSampleEnvDone[l] )
			{
				lastSampleEnvVal[l][0] = lastSampleVal[l][0];
				lastSampleEnvVal[l][1] = lastSampleVal[l][1];
			}

			if( sampleMuted[l] )
			{
				samplesample[l][0] = 0;
				samplesample[l][1] = 0;
			}

			sample_sampleindex[l] += sample_step_sample;
		}
		else
		{
			lastSampleVal[l][0] = 0;
			lastSampleVal[l][1] = 0;
		}
	}

	std::vector<float> sampleAvg(2);
	std::vector<float> sampleMainOsc(2);
	// Main Oscillator outputs
	for( int i = 0; i < maxMainEnabled; ++i )
	{
		if( enabled[i] )
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
				temp1 = unisonVoices[i] * 0.5f;
				sampleMainOsc[0] /= temp1;
				sampleMainOsc[1] /= temp1;

				if( pan[i] )
				{
					if( pan[i] < 0 )
					{
						sampleMainOsc[1] *= ( 100.f + pan[i] ) * 0.01f;
					}
					else
					{
						sampleMainOsc[0] *= ( 100.f - pan[i] ) * 0.01f;
					}
				}

				sampleAvg[0] += sampleMainOsc[0];
				sampleAvg[1] += sampleMainOsc[1];
			}
			else
			{
				sampleMainOsc[0] = sample[i][0];
				sampleMainOsc[1] = sample[i][0];

				if( pan[i] )
				{
					if( pan[i] < 0 )
					{
						sampleMainOsc[1] *= ( 100.f + pan[i] ) * 0.01f;
					}
					else
					{
						sampleMainOsc[0] *= ( 100.f - pan[i] ) * 0.01f;
					}
				}

				sampleAvg[0] += sampleMainOsc[0];
				sampleAvg[1] += sampleMainOsc[1];
			}

			lastMainOscVal[i][0] = sampleAvg[0];// Store results for modulations
			lastMainOscVal[i][1] = sampleAvg[1];

			if( !lastMainOscEnvDone[i] )
			{
				lastMainOscEnvVal[i][0] = lastMainOscVal[i][0];
				lastMainOscEnvVal[i][1] = lastMainOscVal[i][1];
			}

			if( muted[i] )
			{
				sampleAvg[0] = 0;
				sampleAvg[1] = 0;
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
	for( int l = 0; l < maxSampleEnabled; ++l )// maxSampleEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		if( sampleEnabled[l] )
		{
			sampleAvg[0] += samplesample[l][0];
			sampleAvg[1] += samplesample[l][1];
		}
	}

	// Filter outputs
	for( int l = 0; l < maxFiltEnabled; ++l )// maxFiltEnabled keeps this from looping 8 times every sample, saving some CPU
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
		refreshValue( modValType[i], modValNum[i], mwc );// Possible CPU problem
	}
	modValType.clear();
	modValNum.clear();

	return sampleAvg;
}


// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float mSynth::detuneWithCents( float pitchValue, float detuneValue )
{
	if( detuneValue )// Avoids expensive exponentiation if no detuning is necessary
	{
		return pitchValue * std::exp2( detuneValue / 1200.f ); 
	}
	else
	{
		return pitchValue;
	}
}


// At the end of mSynth::nextStringSample, this will refresh all modulated values back to the value of the knob.
void mSynth::refreshValue( int which, int num, Microwave * mwc )
{
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
		case 38: modInAmnt[num] = mwc->modInAmnt[num]->value(); break;
		case 39: modInCurve[num] = mwc->modInCurve[num]->value(); break;
		case 40: modIn2[num] = mwc->modIn2[num]->value(); break;
		case 41: modInNum2[num] = mwc->modInNum2[num]->value(); break;
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
		case 78: macro[num] = mwc->macro[num]->value(); break;
		case 79: filtMuted[num] = mwc->filtMuted[num]->value(); break;
		case 80: phaseRand[num] = mwc->phaseRand[num]->value(); break;
		case 81: modType2[num] = mwc->modType2[num]->value(); break;
		case 82: subRateLimit[num] = mwc->subRateLimit[num]->value(); break;
	}
}
















QString MicrowaveManualView::s_manualText=
"H<b>OW TO OPERATE YOUR MICROWAVE<br>"
"<br>"
"Table of Contents:<br>"
"<br>"
"1. Feature Overview<br>"
" a. Wavetable tab<br>"
" b. Sub Oscillator Tab<br>"
" c. Sample Tab<br>"
" d. Matrix Tab<br>"
" e. Filter Tab<br>"
" f. Miscellaneous Tab<br>"
"2. CPU Preservation Guidelines<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"<b>==FEATURE OVERVIEW==<br>"
"<br>"
"<br>"
"-=WAVETABLE TAB=-<br>"
"<br>"
"If you zoom in all the way on a sound or waveform, you'll see these little \"audio pixels\".  These are called \"samples\", not to be confused with the common term \"sound sample\" which refers to any stored piece of audio.  These \"audio pixels\" can easily be seen when using LMMS's BitInvader.<br>"
"<br>"
"A \"wavetable synthesizer\" is a synthesizer that stores its waveforms as a list of samples.  This means synthesizers like BitInvader and WatSyn are technically wavetable synthesizers.  But, the term \"wavetable synthesizer\" more commonly (but not more or less correctly) refers to a synthesizer that stores multiple waveforms, plays one waveform and repeats it, and allows the user to move a knob to change which waveform is being played.  Synthesizers of this nature, even the basic ones, are unimaginably powerful.  Microwave is one of them, but in no way is it simple.<br>"
"<br>"
"Microwave's wavetables have 256 waveforms, at 2048 samples each.  The Morph (MPH) knob chooses which of the 256 waveforms in the wavetable to play.  It is important to note that Microwave does not have any wavetable loaded by default, so no sound will be heard currently.  Load a sound file as a wavetable now (click the folder button at the bottom).  If you play a note while moving the Morph knob, you'll notice the waveform that is playing morphing to create new timbres.<br>"
"<br>"
"Range (RNG) is a feature unique to Microwave.  It takes waveforms in the wavetable near the one chosen by the Morph one, and mixes those in with the main waveform (at a lower volume as you get further away from the main waveform).  For example, a Morph of 5 and a Range of 2 will give you a mix between waveform 5, waveform 4 at half volume, and waveform 6 at half volume.<br>"
"<br>"
"MDFY (Modify) and the dropdown box next to it (Modify Mode) are used to warp your wavetable realtime, using formulas I created myself.  Change the Modify Mode and move the Modify knob while playing a note.  Hear how each Modify Mode causes a drastically different change to the sound.  These are extremely useful for injecting more flavor and awesomeness into your sound.  Use all of them to learn what they can do.<br>"
"<br>"
"DET stands for Detune, which changes the pitch of that oscillator, in cents.  PHS stands for Phase, which simply phase shifts the oscillator, and RAND next to it is Phase Randomness, which sets the oscillator to a random phase with each note.<br>"
"<br>"
"Microwave supports very advanced unison abillities.  Unison is when you clone the oscillator multiple times, and play them all at the same time, usually with slight differences applied to them.  The original sound as well as the clones are called \"voices\".  In the UNISON box, NUM chooses the number of voices.  DET detunes each voice slightly, a common unison feature.  MPH and MOD are special.  They change the Morph and Modify (respectively) values for each individual voice, which can create an amazing 3D sound.  It is important to note that every unison voice is calculated individually, so using large numbers of unison voices can be very detrimental to your CPU.<br>"
"<br>"
"Earlier I mentioned that Microwave's wavetables have 256 waveforms, at 2048 samples each.  This can be changed using the Sample Length knob.  This knob is meant to be used for finetuning your wavetable if the loading was slightly inaccurate.  If you notice your waveform moving left/right too much in the visualizer as you morph through the wavetable, the Sample Length knob may be able to fix that.<br>"
"<br>"
"With most synthesizers, CPU would be a major concern when using wavetables.  Luckily, I have put an obscene amount of work into optimizing Microwave, so this should be much less of a problem.  Feel free to go crazy.<br>"
"<br>"
"<br>"
"-=SUB TAB=-<br>"
"<br>"
"This tab behaves a lot like BitInvader, but is significantly more useful in the context of Microwave.  This tab is meant to be used for many things:<br>"
"1. Single-waveform oscillators/modulators<br>"
"2. LFOs<br>"
"3. Envelopes<br>"
"4. Step Sequencers<br>"
"5. Noise Generators<br>"
"<br>"
"In very early versions of Microwave, the five things listed above were all in their own tabs, and were later combined into one for obvious user-friendliness reasons.  I would like to quickly note here that I implore all of you to use Step Sequencers in Microwave all the time.  I wanted it to be one of the more highlighted features of Microwave, but never really had the chance.  Step Sequencers are an awesome and unique way to add rhythmic modulations to your sound.<br>"
"<br>"
"The LENGTH knob changes the length of the oscillator.  Decreasing this to a small number makes it very easy to use this as a Step Sequencer.  In some special cases you may also want to automate this knob for some interesting effects.<br>"
"<br>"
"There are four LEDs you can see at the bottom.  The top left is whether the oscillator is enabled.  The top right is \"Muted\", which is different.  When an oscillator is enabled but muted, it is still calculated and still used for modulation, but that oscillator's sound is never played.  You'll usually want this on when using this as an envelope/LFO/step sequencer.  The bottom left is keytracking.  When keytracking is disabled, the oscillator always plays at the same frequency regardless of the note you press.  You'll want to turn this off when you need your envelopes/LFOs/step sequencers to always go at the same speed.  The bottom right LED converts the oscillator into a noise generator, which generates a different flavor of noise depending on the graph you draw.<br>"
"<br>"
"When the tempo knob is set to anything other than 0, the pitch is decreased drastically (you'll probably want to mute it) so that it perfectly matches up with the set tempo when detune is set to 0.  If you need it at half speed, double speed, etc., just change its pitch by octaves (because increasing by an octave doubles the frequency).<br>"
"<br>"
"Note that explanations on how to use this for modulation is explained in the Matrix Tab section.<br>"
"<br>"
"<br>"
"-=SAMPLE TAB=-<br>"
"<br>"
"This tab is used to import entire samples to use as oscillators.  This means you can frequency modulate your cowbells with your airhorns, which you can then phase modulate with an ogre yelling about his swamp, which you can then amplitude modulate with a full-length movie about bees.  Alternatively, you could just use it as a simple way to layer in a sound or some noise sample with the sound you have already created.  It is important to note that imported samples are stored within Microwave, which means two things:<br>"
"1. Unlike AudioFileProcessor, where the size of the sample does not impact the project file size, any samples in Microwave will be losslessly stored inside the project file and preset, which can make the project file extremely large.<br>"
"2. When sending your project file or Microwave preset to somebody else, they do not need to have the sample to open it, unlike with AudioFileProcessor.<br>"
"<br>"
"With that being said, Microwave's Sample Tab is not meant as a replacement to AudioFileProcessor.  Microwave will use more CPU, and some audio problems may show up when playing notes other than A4 (e.g. unstable pitch and stuff).  In most cases, if what you want can be done with AudioFileProcessor, you should use AudioFileProcessor.  Otherwise, totally use Microwave.  The Sample Tab is useful for many reasons, especially for its modulation capabilities and the weird way Microwave can morph samples depending on its graph waveform.<br>"
"<br>"
"The Sample Tab has two new knobs.  Those change the start and end position of the sample.<br>"
"<br>"
"There are two new LEDs for this tab, at the right.  The second to last one enables the graph.  The graph determines the order in which the sample is played.  A saw wave will play the sample normally, and a reverse wave wave will play it backward.  Draw a random squiggle on it and... well, you'll hear it.  Pretty much, left/right is time, up/down is position in the sample.  Note that this will almost definitely change the pitch in most circumstances, because changing a sound's speed also changes its pitch.  The last LED enables and disabled sample looping.<br>"
"<br>"
"<br>"
"-=MATRIX TAB=-<br>"
"<br>"
"This tab is used for a lot of things, ranging from modulation, effect routing, humanization, etc.  If you think it looks difficult, it's a lot easier than it looks.  If you think it looks easy, it's much more difficult than it looks.<br>"
"<br>"
"The matrix works by taking one or two inputs (whether it be from an oscillator, effect output, humanizer, or anything else) and outputting it somewhere (e.g. to control/modulate a knob, to an effect input, etc.).  It's fairly simple.<br>"
"<br>"
"Notice how there are three rows.  Only focus on the top two, as the top and bottom ones are functionally identical.  The top left dropdown box chooses the matrix input, and the LCD Spinboxes choose which input (e.g. which oscillator, which filter, etc.) to grab from.  The AMT knob chooses the Amount of the input that is fed into the output (e.g. input volume to effect, how much to move a knob by, etc.).  The CRV (Curve) knob gives that input a bias upward or downward, which can be used as an extremely simple way to shape and sculpt your modulations in absolutely brilliant ways.<br>"
"<br>"
"The middle left dropdown box sends which section to send the output to (e.g. which tab), and the dropdown box to the right of it is more specific (e.g. the Morph knob of the main tab, the third filter, etc.), as well as the LCD Spinbox following (for e.g. which oscillator to send it to).  The dropdown box to the right of that lets you choose between unidirectional and bidirectional modulation, as well as choosing how the two inputs are combined (e.g. add vs multiply).  The LED to the right of that converts the inputs from LFOs to envelopes if applicable.  In other words, it ignores repetitions of the input oscillators.<br>"
"<br>"
"It seems simple, but this section is one of the most important parts of Microwave.  Most synthesizers only have some combination of FM, PM, AM, and PWM modulation types.  Because of how Microwave's matrix tab is set up, allowing any parameter to be controlled by any oscillator at any speed, Microwave has those modulation types as well as over a hundred more.  Welcome to Choice Paralysis.  There will never be any point in time when the amount of choice you have will not overwhelm you.  Have fun with your freedom!<br>"
"<br>"
"<br>"
"-=EFFECT TAB=-<br>"
"(temporarily AKA Filter Tab)<br>"
"<br>"
"The current version of Microwave only has filters for effects, but that will be changed in future versions.<br>"
"<br>"
"FREQ is the filter's cutoff frequency, and RESO is the filter's resonance.  GAIN is the gain for peak filters, shelf filters, etc.  TYPE is the type of filter.  Microwave currently has lowpass, highpass, bandpass, peak, notch, low shelf, high shelf, allpass, and moog lowpass filters.  SLOPE runs the sound through the filter multiple times, changing the filter's slope at (usually) increments of 12 db.<br>"
"<br>"
"IN and OUT are volume knobs for the filter's input and output.  W/D is Wet/Dry.  PAN changes the Wet/Dry amount in individual ears, allowing you to use interesting panning filters.  SAT stands for Saturation, which allows you to add some simple distortion to your sound after it is filtered.<br>"
"<br>"
"FDBK (Feedback) stores the filter's output and sends it back to the filter's input after a certain amount of time.  This is a very odd, unique, interesting, and useful feature.  Without a filter in effect, increasing the feedback turns this into a comb filter.  Having a filter selected and working can create some ridiculous tibres you'd rarely ever hear out of most other synthesizers.  The change the feedback makes to the sound is very tonal, and the pitch it creates depends on its delay.  Because of this, I made it so the delay is keytracking by default, so the change it makes to your sound matches the note you play.  DET detunes that, and the keytracking button in the top right turns keytracking off for that.  Definitely have fun with this feature, you can get weird and amazing sound out of this.  Notice that this feature entirely allows Karplus-Strong synthesis, as well as any other ridiculous adaptations that pop into your head.<br>"
"<br>"
"<br>"
"-=MISCELLANEOUS TAB=-<br>"
"<br>"
"The oversampling dropdown box is ***VERY*** important when it comes to Microwave's audio quality.  The higher it is, the cleaner your sound will be.  But, oversampling is also extremely detrimental to your CPU.  The multiplier of oversampling will be almost exactly the multiplier it applies to the processing power it uses.<br>"
"<br>"
"But how do you know whether you need oversampling?  2x should be appropriate in most (?) cases, but definitely not all of them.  If your sound is very basic, and all matrix inputs that control knobs only move very very slowly, then it's possible that as low as 1x oversampling (which means no oversampling) may be appropriate.  But, if the sound makes use of modulation, then higher values of oversampling may be appropriate, especially if the modulator contains or plays at high frequencies.  When in doubt, use your ears and compare.  Never neglect the oversampling.  If you're making a sound that uses modulation and it sounds a bit too much like a dying engine, bumping up the oversampling a bit may be all that is needed to bring the sound from okay to awesome.  I strongly suggest making oversampling tweaking a part of your habitual workflow when using Microwave.<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"<b>==CPU PRESERVATION GUIDELINES==<br>"
"<br>"
"<br>"
"First and foremost, turn the wavetable tab's visualizer off.  That uses a ton of processing power.<br>"
"<br>"
"Microwave stores the highest oscillator that is enabled, and checks every oscillator from the first one to the highest enabled one to see whether it's enabled.  So, having just the 50th oscillator enabled will take significantly more processing power than having just the 1st one enabled, because it would need to check oscillators 1-50 to see whether they're enabled.<br>"
"<br>"
"Increasing the Range knob will use more CPU (since it needs to calculate nearby waveforms as well).  With very large Range values the CPU hit can get quite noticeable.  But, even though this needs to calculate nearby waveforms, it doesn't need to recalculate the entire oscillator, so increasing the Range won't use nearly as much CPU as, for example, increasing the number of unison voices.  <br>"
"<br>"
"Having a larger number of unison voices increases the CPU usage by around the voice amount.  For example, having 30 voices will use approximately 15x as much processing power as just two voices.  This increase is almost exactly linear (with the exception of using only one voice, which uses less than half the CPU as two voices, since having unison disabled entirely will prevent unison stuff from being calculated entirely).<br>"
"<br>"
"The values of the Morph and Modify knobs in the UNISON box have no impact on processing power needed, except for a small performance gain when they are at exactly 0.<br>"
"<br>"
"Having both of the DET knobs in and out of the UNISON box set to exactly 0 will result in a significant performance gain in the wavetable tab.  Any values other than 0 will have a near identical impact in comparison to each other.<br>"
"<br>"
"Even when the modify is not in use, having the Modify Mode set to None will use (sometimes significantly) less CPU than if it is set to something else.<br>"
"<br>"
"When using modify, expect the largest CPU hit from modes that require accessing other parts of the waveform to work (e.g. Squarify and Pulsify), especially with high Range values.<br>"
"<br>"
"Oversampling results in an almost exact multiplication in processing power needed.  So 8x oversampling will use approximately 4x as much CPU as 2x oversampling.<br>"
"<br>"
;



MicrowaveManualView::MicrowaveManualView():QTextEdit( s_manualText )
{
	setWindowTitle ( "Microwave Manual" );
	setTextInteractionFlags ( Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse );
	gui->mainWindow()->addWindowedWidget( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	parentWidget()->resize( 640, 480 );
}


void MicrowaveView::manualBtnClicked() {
	MicrowaveManualView::getInstance()->hide();
	MicrowaveManualView::getInstance()->show();
}





extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new Microwave( static_cast<InstrumentTrack *>( m ) ) );
}


}

