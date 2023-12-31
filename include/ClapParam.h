/*
 * ClapParam.h - Declaration of ClapParam class
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_PARAM_H
#define LMMS_CLAP_PARAM_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "AutomatableModel.h"

#include <string>
#include <string_view>
#include <ostream>
#include <unordered_map>
#include <algorithm>
#include <memory>

#include <QObject>

#include <clap/clap.h>

namespace lmms
{

class ClapInstance;
class ClapParam : public QObject
{
	Q_OBJECT;

public:

	enum class ParamType
	{
		Undefined,
		Bool,
		Integer,
		Float
	};

	ClapParam(ClapInstance* pluginHost, const clap_param_info& info, double value);

	auto model() const -> AutomatableModel* { return m_connectedModel.get(); }
	auto valueType() const -> ParamType { return m_valueType; }
	auto getId() const -> std::string_view { return m_id; }
	auto getDisplayName() const -> std::string_view { return m_displayName; }
	auto getValueText(const clap_plugin* plugin, const clap_plugin_params* params) const -> std::string;

	auto value() const -> double { return m_value; }
	void setValue(double v);

	auto modulation() const -> double { return m_modulation; }
	void setModulation(double v);

	auto modulatedValue() const -> double
	{
		return std::clamp(m_value + m_modulation, m_info.min_value, m_info.max_value);
	}

	auto isValueValid(const double v) const -> bool;

	void printShortInfo(std::ostream& os) const;
	void printInfo(std::ostream& os) const;

	void setInfo(const clap_param_info& info) noexcept { m_info = info; }
	auto isInfoEqualTo(const clap_param_info& info) const -> bool;
	auto isInfoCriticallyDifferentTo(const clap_param_info& info) const -> bool;
	auto info() noexcept -> clap_param_info& { return m_info; }
	auto info() const noexcept -> const clap_param_info& { return m_info; }

	auto isBeingAdjusted() const noexcept -> bool { return m_isBeingAdjusted; }
	void setIsAdjusting(bool isAdjusting)
	{
		if (isAdjusting && !m_isBeingAdjusted) { beginAdjust(); }
		else if (!isAdjusting && m_isBeingAdjusted) { endAdjust(); }
	}
	void beginAdjust()
	{
		Q_ASSERT(!m_isBeingAdjusted);
		m_isBeingAdjusted = true;
		emit isBeingAdjustedChanged();
	}
	void endAdjust()
	{
		Q_ASSERT(m_isBeingAdjusted);
		m_isBeingAdjusted = false;
		emit isBeingAdjustedChanged();
	}

signals:
	void isBeingAdjustedChanged();
	void infoChanged();
	void valueChanged();
	void modulatedValueChanged();

private:
	bool m_isBeingAdjusted = false;
	clap_param_info m_info;
	double m_value = 0.0;
	double m_modulation = 0.0;
	std::unordered_map<int64_t, std::string> m_enumEntries;

	//! An AutomatableModel is created if the param is to be shown to the user
	std::unique_ptr<AutomatableModel> m_connectedModel;

	ParamType m_valueType = ParamType::Undefined;

	std::string m_id;
	std::string m_displayName;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PARAM_H
