/*
 * ClapFxControlDialog.cpp - ClapFxControlDialog implementation
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapFxControlDialog.h"

#include <QPushButton>

#include "ClapFxControls.h"
#include "PixmapButton.h"

namespace lmms::gui
{

ClapFxControlDialog::ClapFxControlDialog(ClapFxControls* controls)
	: EffectControlDialog{controls}
	, ClapViewBase{this, controls}
{
	if (m_reloadPluginButton)
	{
		connect(m_reloadPluginButton, &QPushButton::clicked, this, [this] { clapControls()->reload(); });
	}

	if (m_toggleUIButton)
	{
		connect(m_toggleUIButton, &QPushButton::toggled, this, [this] { toggleUI(); });
	}

	if (m_helpButton)
	{
		connect(m_helpButton, &QPushButton::toggled, this, [this](bool visible) { toggleHelp(visible); });
	}

	if (m_prevPresetButton)
	{
		connect(m_prevPresetButton, &PixmapButton::clicked, this, [this] { clapControls()->prevPreset(); });
		//connect(m_prevPresetButton, &PixmapButton::clicked, this, &ClapViewBase::update);
	}

	if (m_nextPresetButton)
	{
		connect(m_nextPresetButton, &PixmapButton::clicked, this, [this] { clapControls()->nextPreset(); });
		//connect(m_nextPresetButton, &PixmapButton::clicked, this, &ClapViewBase::update);
	}

	// For Effects, modelChanged only goes to the top EffectView
	// We need to call it manually
	modelChanged();
}

auto ClapFxControlDialog::clapControls() -> ClapFxControls*
{
	return static_cast<ClapFxControls*>(m_effectControls);
}

void ClapFxControlDialog::modelChanged()
{
	ClapViewBase::modelChanged(clapControls());
	connect(clapControls(), &ClapFxControls::modelChanged, this, [this](){ this->modelChanged();} );
}

} // namespace lmms::gui
