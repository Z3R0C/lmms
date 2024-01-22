/*
 * ClapViewBase.cpp - Base class for CLAP plugin views
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

#include "ClapViewBase.h"

#ifdef LMMS_HAVE_CLAP

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "AudioEngine.h"
#include "ClapControlBase.h"
#include "ClapInstance.h"
#include "ClapManager.h"
#include "Controls.h"
#include "CustomTextKnob.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "SubWindow.h"

namespace lmms::gui
{

ClapViewInstance::ClapViewInstance(QWidget* parent, ClapInstance* instance, int colNum)
	: LinkedModelGroupView{parent, &instance->params(), static_cast<std::size_t>(colNum)}
	, m_instance{instance}
{
	for (auto param : m_instance->params().parameters())
	{
		if (!param || !param->model()) { continue; }

		Control* control = nullptr;

		switch (param->valueType())
		{
		case ClapParameter::ValueType::Bool:
			control = new CheckControl{this};
			break;
		case ClapParameter::ValueType::Integer:
			// TODO: What if more digits are needed? Lv2 uses KnobControl in this case.
			control = new LcdControl{(param->info().max_value <= 9.0) ? 1 : 2, this};
			break;
		// TODO: Add support for enum controls
		case ClapParameter::ValueType::Float:
		{
			control = new CustomTextKnobControl{this};

			// CustomTextKnob calls this lambda to update value text
			auto customTextKnob = dynamic_cast<CustomTextKnob*>(control->modelView());
			customTextKnob->setValueText([=]() {
				return QString::fromUtf8(m_instance->params().getValueText(*param).c_str());
			});

			break;
		}
		default:
			throw std::runtime_error{"Invalid CLAP param value type"};
		}

		if (!control) { continue; }

		// This is the param name seen in the GUI
		control->setText(QString::fromUtf8(param->displayName().data()));

		// TODO: Group parameters according to module path?
		if (param->info().module[0] != '\0')
		{
			control->topWidget()->setToolTip(QString::fromUtf8(param->info().module));
		}

		addControl(control, param->id().data(), param->displayName().data(), false);
	}

}

ClapViewBase::ClapViewBase(QWidget* pluginWidget, ClapControlBase* ctrlBase)
{
	auto grid = new QGridLayout{pluginWidget};

	auto btnBox = std::make_unique<QHBoxLayout>();
	if (ClapManager::debugging())
	{
		m_reloadPluginButton = new QPushButton{QObject::tr("Reload Plugin"), pluginWidget};
		btnBox->addWidget(m_reloadPluginButton, 0);
	}

	if (ctrlBase->hasGui())
	{
		m_toggleUIButton = new QPushButton{QObject::tr("Show GUI"), pluginWidget};
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(pointSize<8>(m_toggleUIButton->font()));
		btnBox->addWidget(m_toggleUIButton, 0);
	}
	btnBox->addStretch(1);

	pluginWidget->setAcceptDrops(true);

	if (m_reloadPluginButton || m_toggleUIButton || m_helpButton)
	{
		grid->addLayout(btnBox.release(), static_cast<int>(Rows::ButtonRow), 0, 1, s_colNum);
	}

	m_procView = new ClapViewInstance{pluginWidget, ctrlBase->control(0), s_colNum};
	grid->addWidget(m_procView, static_cast<int>(Rows::ProcRow), 0);
}


ClapViewBase::~ClapViewBase()
{
	// TODO: hide UI if required
}

void ClapViewBase::toggleUI()
{
}

void ClapViewBase::toggleHelp(bool visible)
{
	if (!m_helpWindow) { return; }

	if (visible)
	{
		m_helpWindow->show();
		m_helpWindow->raise();
	}
	else
	{
		m_helpWindow->hide();
	}
}

void ClapViewBase::modelChanged(ClapControlBase* ctrlBase)
{
	// reconnect models
	if (m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(ctrlBase->hasGui());
	}

	LinkedModelGroupsView::modelChanged(ctrlBase);
}

} // namespace lmms::gui

#endif // LMMS_HAVE_CLAP
