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

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>

class MainWindow;
class QDialogButtonBox;

class SettingsPage : public QWidget
{
	Q_OBJECT
public:
	SettingsPage(MainWindow* main_window, QWidget* parent = 0) : QWidget(parent), main_window(main_window){}
	virtual void cancel() = 0;
	virtual void apply();
	virtual void ok(){ this->apply(); }
	virtual QString title() = 0;

protected:
	MainWindow* main_window;

	//The changes to be done when accepted
	QHash<QString, QVariant> changes;
};

class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget *parent = 0);

private:
	QTabWidget *tabWidget;
	QDialogButtonBox *dbb;
	QList<SettingsPage*> pages;

private slots:
	void buttonPressed(QAbstractButton*);
};

class RenderPage : public SettingsPage
{
	Q_OBJECT
public:
	RenderPage(MainWindow* main_window, QWidget* parent = 0);

	virtual void cancel(){ changes.clear(); }
	virtual QString title(){ return QString("Map Display"); }
	virtual void apply();
private slots:
	void antialiasingClicked();
	void noantialiasingClicked();
};

#endif
