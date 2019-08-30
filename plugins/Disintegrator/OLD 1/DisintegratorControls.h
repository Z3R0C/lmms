/*
 * DisintegratorControls.h
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#ifndef DISINTEGRATOR_CONTROLS_H
#define DISINTEGRATOR_CONTROLS_H

#include "EffectControls.h"
#include "DisintegratorControlDialog.h"
#include "Knob.h"


class DisintegratorEffect;


class DisintegratorControls : public EffectControls
{
	Q_OBJECT
public:
	DisintegratorControls( DisintegratorEffect* effect );
	virtual ~DisintegratorControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "DisintegratorControls";
	}

	virtual int controlCount()
	{
		// ==TAG== CONTROLCOUNT ==TAG== //
		return 4;
	}

	virtual EffectControlDialog* createView()
	{
		return new DisintegratorControlDialog( this );
	}

private:
	DisintegratorEffect* m_effect;

	// ==TAG== DEFINEMODELS ==TAG== //
	FloatModel m_volumeModel;
	FloatModel m_panModel;
	FloatModel m_leftModel;
	FloatModel m_rightModel;
	// ==TAG== ENDDEFINEMODELS ==TAG== //

	friend class DisintegratorControlDialog;
	friend class DisintegratorEffect;

} ;

#endif
