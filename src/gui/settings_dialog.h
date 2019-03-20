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

#ifndef OPENORIENTEERING_SETTINGS_DIALOG_H
#define OPENORIENTEERING_SETTINGS_DIALOG_H

#include <QDialog>
#include <QObject>
#include <QString>

class QAbstractButton;
class QCloseEvent;
class QDialogButtonBox;
class QKeyEvent;
class QStackedWidget;
class QTabWidget;
class QWidget;

namespace OpenOrienteering {

class SettingsPage;


/** 
 * A dialog for editing Mapper's settings.
 * 
 * This class is marked as final because its constructor calls virtual functions.
 */
class SettingsDialog final : public QDialog
{
Q_OBJECT
public:
	/** 
	 * Constructs a new settings dialog.
	 */
	explicit SettingsDialog(QWidget* parent = nullptr);
	
	/** 
	 * Destroys the settings dialog.
	 */
	~SettingsDialog() override;
	
protected:
	/**
	 * Adds all known pages to the dialog.
	 * 
	 * This function is called from the constructor. It may be overridden to
	 * provide dialogs with different pages.
	 */
	virtual void addPages();
	
	/**
	 * Adds a single page to the dialog.
	 */
	void addPage(SettingsPage* page);
	
	/**
	 * Calls a SettingsPage member function on all pages.
	 */
	void callOnAllPages(void (SettingsPage::*member)());
	
	
	void closeEvent(QCloseEvent* event) override;
	
	void keyPressEvent(QKeyEvent* event) override;
	
private slots:
	/**
	 * Reacts to dialog buttons (OK, Cancel, Rest).
	 */
	void buttonPressed(QAbstractButton* button);
	
private:
	/**
	 * A tab widget which holds all pages in desktop mode.
	 */
	QTabWidget* tab_widget;
	
	/**
	 * A stack widget which holds all pages in mobile mode.
	 */
	QStackedWidget* stack_widget;
	
	/** 
	 * The box of standard dialog buttons.
	 */
	QDialogButtonBox* button_box;
};


}  // namespace OpenOrienteering

#endif
