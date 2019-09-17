/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2013-2016  Kai Pastor
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

#ifndef OPENORIENTEERING_GENERAL_SETTINGS_PAGE_H
#define OPENORIENTEERING_GENERAL_SETTINGS_PAGE_H

#include <QObject>
#include <QString>
#include <QVariant>

#include "settings_page.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QEvent;
class QSpinBox;
class QWidget;


namespace OpenOrienteering {

class GeneralSettingsPage : public SettingsPage
{
Q_OBJECT
public:
	explicit GeneralSettingsPage(QWidget* parent = nullptr);
	
	~GeneralSettingsPage() override;
	
	QString title() const override;
	
	void apply() override;
	
	void reset() override;
	
protected:
	/**
	 * Adds the available languages to the language combo box,
	 * and sets the current element.
	 */
	void updateLanguageBox(QVariant code);
	
	void updateWidgets();
	
	/**
	 * This event filter stops LanguageChange events.
	 */
	bool eventFilter(QObject* watched, QEvent* event) override;
	
private slots:
	void openTranslationFileDialog();
	
	void openPPICalculationDialog();
	
	void encodingChanged(const QString& input);
	
private:
	QString    translation_file;
	QString    last_encoding_input;
	QString    last_matching_completition;
	
	QComboBox* language_box;
	
	QDoubleSpinBox* ppi_edit;
	QCheckBox* open_mru_check;
	QCheckBox* tips_visible_check;
	
	QCheckBox* compatibility_check;
	QCheckBox* undo_check;
	QCheckBox* autosave_check;
	QSpinBox*  autosave_interval_edit;
	
	QComboBox* encoding_box;
};


}  // namespace OpenOrienteering

#endif
