/*
 * instrument.h - declaration of class instrument, which provides a
 *                standard interface for all instrument plugins
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _INSTRUMENT_H
#define _INSTRUMENT_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QWidget>
#include <QVector>

#else

#include <qwidget.h>
#include <qvaluevector.h>

#endif


#include "plugin.h"
#include "mixer.h"


// forward-declarations
class channelTrack;
class notePlayHandle;


class instrument : public QWidget, public plugin
{
public:
	instrument( channelTrack * _channel_track, const QString & _name );
	virtual ~instrument();

	// if the plugin doesn't play each note, it can create an instrument-
	// play-handle and re-implement this method, so that it mixes it's
	// output buffer only once per mixer-period
	virtual void play( void );

	// must be overloaded by actual plugin
	virtual void FASTCALL playNote( notePlayHandle * note_to_play );

	// needed for deleting plugin-specific-data of a note - plugin has to
	// cast void-ptr so that the plugin-data is deleted properly
	// (call of dtor if it's a class etc.)
	virtual void FASTCALL deleteNotePluginData( notePlayHandle *
							_note_to_play );

	// Get number of sample-frames that should be used when playing beat
	// (note with unspecified length)
	// Per default this function returns 0. In this case, channel is using
	// the length of the longest envelope (if one active).
	virtual Uint32 FASTCALL beatLen( notePlayHandle * _n ) const;


	// instrument-play-handles use this for checking whether they can mark
	// themselves as done, so that mixer trashes them
	inline virtual bool valid( void ) const
	{
		return( m_valid );
	}


	// instantiate instrument-plugin with given name or return NULL
	// on failure
	static instrument * FASTCALL instantiate( const QString & _plugin_name,
						channelTrack * _channel_track );

protected:
	inline channelTrack * getChannelTrack( void ) const
	{
		return( m_channelTrack );
	}

	// instruments can use this for invalidating themselves, which is for
	// example neccessary when being destroyed and having instrument-play-
	// handles running
	inline void invalidate( void )
	{
		m_valid = FALSE;
		mixer::inst()->checkValidityOfPlayHandles();
	}


private:
	channelTrack * m_channelTrack;
	bool m_valid;

} ;


#endif
