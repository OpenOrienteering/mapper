/*
 *    Copyright 2012, 2013 Jan Dalheimer
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

#ifndef _OPENORIENTEERING_SETTINGS_DIALOG_H_
#define _OPENORIENTEERING_SETTINGS_DIALOG_H_

#include <QDialog>
#include <QVector>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QAbstractButton;
class QDialogButtonBox;
QT_END_NAMESPACE

class SettingsPage;


/** A dialog for editing Mapper's settings. */
class SettingsDialog : public QDialog
{
Q_OBJECT
public:
	/** Constructs a new settings dialog. */
	SettingsDialog(QWidget* parent = 0);
	
	/** Destroys the settings dialog. */
	virtual ~SettingsDialog();
	
private slots:
	/** Reacts to button presses (Ok, Cancel, Apply[?]) */
	void buttonPressed(QAbstractButton* button);
	
private:
	/** Adds a page to the dialog. */
	void addPage(SettingsPage* page);
	
	/** A tab widget which holds all pages. */
	QTabWidget* tab_widget;
	
	/** The box of dialog buttons. */
	QDialogButtonBox* button_box;
};

#endif
