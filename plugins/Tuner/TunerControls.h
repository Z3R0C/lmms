/*
 * TunerControls.h
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#ifndef LMMS_TUNER_CONTROLS_H
#define LMMS_TUNER_CONTROLS_H

#include "EffectControls.h"
#include "LcdSpinBox.h"
#include "TunerControlDialog.h"

namespace lmms {

class Tuner;
class TunerControls : public EffectControls
{
public:
	TunerControls(Tuner* tuner);

	void saveSettings(QDomDocument&, QDomElement&) override;
	void loadSettings(const QDomElement&) override;

	QString nodeName() const override;
	int controlCount() override;

	gui::EffectControlDialog* createView() override;

private:
	Tuner* m_tuner = nullptr;
	gui::TunerControlDialog* m_tunerDialog = nullptr;
	gui::LcdSpinBoxModel m_referenceFreqModel;

	friend class gui::TunerControlDialog;
	friend class Tuner;
};
} // namespace lmms

#endif // LMMS_TUNER_CONTROLS_H
