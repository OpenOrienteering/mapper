/*
 *    Copyright 2012 Jan Dalheimer
 *    Copyright 2013-2016 Kai Pastor
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

#ifndef OPENORIENTEERING_SETTINGS_PAGE_H
#define OPENORIENTEERING_SETTINGS_PAGE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QWidget>

#include "settings.h"


/**
 * A widget which serves as a page in the SettingsDialog.
 */
class SettingsPage : public QWidget
{
Q_OBJECT
public:
	/**
	 * Constructs a new settings page.
	 */
	explicit SettingsPage(QWidget* parent = nullptr);
	
	/**
	 * Destructor.
	 */
	~SettingsPage() override;
	
	/**
	 * Returns the title of this page.
	 */
	virtual QString title() const = 0;
	
public slots:	
	/**
	 * This slot is to transfer the input fields to the settings.
	 */
	virtual void apply() = 0;
	
	/**
	 * This slot is to reset all input widgets to the state of the settings.
	 */
	virtual void reset() = 0;

protected:	
	/**
	 * Convenience function for retrieving a setting.
	 */
	static QVariant getSetting(Settings::SettingsEnum setting);
	
	/**
	 * Convenience function for changing a setting.
	 */
	template <class T>
	static void setSetting(Settings::SettingsEnum setting, T value);
	
};



inline
QVariant SettingsPage::getSetting(Settings::SettingsEnum setting)
{
	return Settings::getInstance().getSetting(setting);
}

template <class T>
void SettingsPage::setSetting(Settings::SettingsEnum setting, T value)
{
	Settings::getInstance().setSetting(setting, QVariant{ value });
}


#endif
