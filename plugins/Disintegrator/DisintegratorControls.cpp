/*
 * DisintegratorControls.cpp
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


#include <QDomElement>

#include "DisintegratorControls.h"
#include "Disintegrator.h"

#include "Engine.h"
#include "Song.h"


DisintegratorControls::DisintegratorControls(DisintegratorEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_lowCutModel(2.0f, 2.0f, 21050.0f, 0.01f, this, tr("Low Cut")),
	m_highCutModel(21050.0f, 2.0f, 21050.0f, 0.01f, this, tr("High Cut")),
	m_amountModel(30.0f, 0.0f, 200.0f, 0.01f, this, tr("Amount")),
	m_typeModel(this, tr("Type")),
	m_freqModel(100.0f, 2.0f, 21050.0f, 0.01f, this, tr("Frequency"))
{
	m_lowCutModel.setScaleLogarithmic(true);
	m_highCutModel.setScaleLogarithmic(true);
	m_amountModel.setScaleLogarithmic(true);
	m_freqModel.setScaleLogarithmic(true);

	m_typeModel.addItem(tr("Mono Noise"));
	m_typeModel.addItem(tr("Stereo Noise"));
	m_typeModel.addItem(tr("Sine Wave"));
}


void DisintegratorControls::saveSettings(QDomDocument& doc, QDomElement& _this)
{
	m_lowCutModel.saveSettings(doc, _this, "lowCut"); 
	m_highCutModel.saveSettings(doc, _this, "highCut");
	m_amountModel.saveSettings(doc, _this, "amount");
	m_typeModel.saveSettings(doc, _this, "type");
	m_freqModel.saveSettings(doc, _this, "freq");
}



void DisintegratorControls::loadSettings(const QDomElement& _this)
{
	m_lowCutModel.loadSettings(_this, "lowCut");
	m_highCutModel.loadSettings(_this, "highCut");
	m_amountModel.loadSettings(_this, "amount");
	m_typeModel.loadSettings(_this, "type");
	m_freqModel.loadSettings(_this, "freq");
}
