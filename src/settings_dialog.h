/*
 *    Copyright 2012 Jan Dalheimer
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

#include <QHash>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QAbstractButton;
QT_END_NAMESPACE

class MainWindow;
class QDialogButtonBox;

class SettingsPage : public QWidget
{
Q_OBJECT
public:
	SettingsPage(QWidget* parent = 0) : QWidget(parent) {}
	virtual void cancel() { changes.clear(); }
	virtual void apply();
	virtual void ok() { this->apply(); }
	virtual QString title() = 0;

protected:
	// The changes to be done when accepted
	QHash<QString, QVariant> changes;
};

class SettingsDialog : public QDialog
{
Q_OBJECT
public:
	SettingsDialog(QWidget* parent = 0);

private:
	void addPage(SettingsPage* page);
	
	QTabWidget* tabWidget;
	QDialogButtonBox* button_box;
	QList<SettingsPage*> pages;

private slots:
	void buttonPressed(QAbstractButton* button);
};

class EditorPage : public SettingsPage
{
Q_OBJECT
public:
	EditorPage(QWidget* parent = 0);

	virtual QString title() { return tr("Editor"); }
	
private slots:
	void antialiasingClicked(bool checked);
	void toleranceChanged(int value);
};

#endif
