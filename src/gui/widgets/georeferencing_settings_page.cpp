/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2016, 2020  Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "georeferencing_settings_page.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QWidget>

#include "settings.h"
#include "gui/util_gui.h"
#include "gui/widgets/settings_page.h"


namespace OpenOrienteering {

GeoreferencingSettingsPage::GeoreferencingSettingsPage(QWidget* parent)
 : SettingsPage(parent)
{
	auto layout = new QFormLayout(this);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Map Georeferencing dialog:")));

	control_scale_factor = new QCheckBox(tr("Control scale factor"), this);
	control_scale_factor->setToolTip(tr("Expands the Georeferencing dialog for editing Auxiliary scale factor."));
	layout->addRow(control_scale_factor);

	updateWidgets();
}

GeoreferencingSettingsPage::~GeoreferencingSettingsPage()
{
	// nothing, not inlined
}

QString GeoreferencingSettingsPage::title() const
{
	return tr("Georeferencing");
}

void GeoreferencingSettingsPage::apply()
{
	setSetting(Settings::MapGeoreferencing_ControlScaleFactor, control_scale_factor->isChecked());
}

void GeoreferencingSettingsPage::reset()
{
	updateWidgets();
}

void GeoreferencingSettingsPage::updateWidgets()
{
	
	control_scale_factor->setChecked(getSetting(Settings::MapGeoreferencing_ControlScaleFactor).toBool());
}


}  // namespace OpenOrienteering
