/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAIN_WINDOW_HOME_SCREEN_H_
#define _OPENORIENTEERING_MAIN_WINDOW_HOME_SCREEN_H_

#include "main_window.h"

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QListWidget;
class QLabel;
QT_END_NAMESPACE

class HomeScreenController : public MainWindowController
{
Q_OBJECT
public:
	HomeScreenController();
	~HomeScreenController();
	
    virtual void attach(MainWindow* window);
    virtual void detach();
};

class HomeScreenWidget : public QWidget
{
Q_OBJECT
public:
	HomeScreenWidget(HomeScreenController* controller, QWidget* parent = NULL);
	virtual ~HomeScreenWidget();
	
protected:
    virtual void paintEvent(QPaintEvent* event);
	
private:
	HomeScreenController* controller;
};

class DocumentSelectionWidget : public QWidget
{
Q_OBJECT
public:
	DocumentSelectionWidget(const QString& title, const QString& start_new_text, const QString& open_text,
							const QString& recent_text, const QString& open_title_text, const QString& open_filter_text, QWidget* parent = NULL);
	virtual ~DocumentSelectionWidget();
	
	void refreshRecentDocs();
	
signals:
	void pathOpened(QString path);
	void newClicked();
	
protected slots:
	void openDoc();
	void newDoc();
	
protected:
    virtual void paintEvent(QPaintEvent* event);
	
private:
	QListWidget* recent_docs_list;
	
	QString open_title_text;
	QString open_filter_text;
};

class HomeScreenTipOfTheDayWidget : public QWidget
{
Q_OBJECT
public:
	HomeScreenTipOfTheDayWidget(HomeScreenController* controller, QWidget* parent = NULL);
	
public slots:
	void previousClicked();
	void nextClicked();
	void linkClicked(QString link);
	
private:
	QString getNextTip(int direction = 1);
	
	QLabel* tip_label;
	HomeScreenController* controller;
};

class HomeScreenOtherWidget : public QWidget
{
Q_OBJECT
public:
	HomeScreenOtherWidget(QWidget* parent = NULL);
	
signals:
	void settingsClicked();
	void aboutClicked();
	
private slots:
	void settingsSlot();
	void aboutSlot();
};

#endif
