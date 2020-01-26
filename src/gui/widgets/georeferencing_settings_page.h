/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2013-2016, 2020  Kai Pastor
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

#ifndef OPENORIENTEERING_GEOREFERENCING_SETTINGS_PAGE_H
#define OPENORIENTEERING_GEOREFERENCING_SETTINGS_PAGE_H

#include <QObject>
#include <QString>

#include "settings_page.h"

class QCheckBox;
class QWidget;


namespace OpenOrienteering {

/**
 * A page in the SettingsDialog, for controlling georeferencing dialogs.
 * The user has the option to enable and show certain advanced features.
 */
class GeoreferencingSettingsPage : public SettingsPage
{
Q_OBJECT
public:
	explicit GeoreferencingSettingsPage(QWidget* parent = nullptr);
	
	~GeoreferencingSettingsPage() override;
	
	QString title() const override;

	void apply() override;
	
	void reset() override;
	
protected:
	void updateWidgets();
	
private:
	QCheckBox *control_scale_factor;
};


}  // namespace OpenOrienteering

#endif
