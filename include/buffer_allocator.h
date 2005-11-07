/*
 * buffer_allocator.h - namespace bufferAllocator providing routines for own
 *                      optimized memory-management for audio-buffers
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


#ifndef _BUFFER_ALLOCATOR_H
#define _BUFFER_ALLOCATOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "types.h"




namespace bufferAllocator
{
	void * FASTCALL allocBytes( Uint32 _bytes );

	template<class T>
	inline T * FASTCALL alloc( Uint32 _n )
	{
		return( (T *) allocBytes( sizeof( T ) * _n ) );
	}

	void FASTCALL free( void * _buf );

	void FASTCALL cleanUp( Uint16 _level );


	// simple class for automatically freeing buffer in complex functions
	template<class T = void>
	class autoCleaner
	{
	public:
		autoCleaner( T * _ptr ) :
			m_ptr( _ptr )
		{
		}
		~autoCleaner()
		{
			bufferAllocator::free( m_ptr );
		}
		inline const T * ptr( void ) const
		{
			return( m_ptr );
		}

	private:
		T * m_ptr;
	} ;

} ;


#endif
