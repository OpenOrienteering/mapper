/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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


#ifndef _OPENORIENTEERING_HOME_SCREEN_WIDGET_H_
#define _OPENORIENTEERING_HOME_SCREEN_WIDGET_H_

#include <vector>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QCheckBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QMouseEvent;
class QPaintEvent;
class QPushButton;
class QStackedLayout;
QT_END_NAMESPACE

class HomeScreenController;

/**
 * The user interface of the OpenOrienteering Mapper home screen.
 * The OpenOrienteering Mapper home screen is shown when no document is open,
 * for example after the program is started for the first time.
 */
class AbstractHomeScreenWidget : public QWidget
{
Q_OBJECT
public:
	/** Creates a new AbstractHomeScreenWidget for the given controller. */
	AbstractHomeScreenWidget(HomeScreenController* controller, QWidget* parent = NULL);
	
	/** Destroys the AbstractHomeScreenWidget. */
	virtual ~AbstractHomeScreenWidget();
	
public slots:
	/** Sets the list of recent files. */
	virtual void setRecentFiles(const QStringList& files) = 0;
	
	/** Sets the "checked" state of the control for opening
	 *  the most recently used file on startup. */
	virtual void setOpenMRUFileChecked(bool state) = 0;
	
	/** Sets the text of the the tip-of-the-day. */
	virtual void setTipOfTheDay(const QString& text) = 0;
	
	/** Sets the visiblity of the tip-of-the-day, and
	 *  sets the "checked" state of the control for displaying the tip. */
	virtual void setTipsVisible(bool state) = 0;
	
protected:
	/** Returns a QLabel for displaying a headline in the home screen. */
	QLabel* makeHeadline(const QString& text, QWidget* parent = NULL) const;
	
	/** Returns a button with the given text
	 *  for triggering a top level activity in the home screen.
	 *  There will be no icon or a default icon. */
	QAbstractButton* makeButton(const QString& text, QWidget* parent = NULL) const;
	
	/** Returns a button with the given text and icon
	 *  for triggering a top level activity in the home screen. */
	QAbstractButton* makeButton(const QString& text, const QIcon& icon, QWidget* parent = NULL) const;
	
	
	HomeScreenController* controller;
};


/**
 * Implementation of AbstractHomeScreenWidget for desktop PCs.
 */
class HomeScreenWidgetDesktop : public AbstractHomeScreenWidget
{
Q_OBJECT
public:
	/** Creates a new HomeScreenWidgetDesktop for the given controller. */
	HomeScreenWidgetDesktop(HomeScreenController* controller, QWidget* parent = NULL);
	
	/** Destroys the HomeScreenWidgetDesktop. */
	virtual ~HomeScreenWidgetDesktop();
	
public slots:
	/** Sets the list of recent files. */
	virtual void setRecentFiles(const QStringList& files);
	
	/** Sets the "checked" state of the control for opening
	 *  the most recently used file on startup. */
	virtual void setOpenMRUFileChecked(bool state);
	
	/** Sets the text of the the tip-of-the-day. */
	virtual void setTipOfTheDay(const QString& text);
	
	/** Sets the visiblity of the tip-of-the-day, and
	 *  sets the "checked" state of the control for displaying the tip. */
	virtual void setTipsVisible(bool state);
	
protected slots:
	/** Opens a file when its is list item is clicked. */
	void recentFileClicked(QListWidgetItem* item);
	
protected:
	/** Draws the home screen background when a paint event occurs. */
	virtual void paintEvent(QPaintEvent* event);
	
	/** Creates the activities widget. */
	QWidget* makeMenuWidget(HomeScreenController* controller, QWidget* parent = NULL);
	
	/** Creates the recent files widget. */
	QWidget* makeRecentFilesWidget(HomeScreenController* controller, QWidget* parent = NULL);
	
	/** Creates the tip-of-the-day widget. */
	QWidget* makeTipsWidget(HomeScreenController* controller, QWidget* parent = NULL);
	
	
	QListWidget* recent_files_list;
	QCheckBox* open_mru_file_check;
	QLabel* tips_label;
	QCheckBox* tips_check;
	std::vector<QWidget*> tips_children;
};


/**
 * Implementation of AbstractHomeScreenWidget for mobile devices.
 */
class HomeScreenWidgetMobile : public AbstractHomeScreenWidget
{
Q_OBJECT
public:
	/** Creates a new HomeScreenWidgetMobile for the given controller. */
	HomeScreenWidgetMobile(HomeScreenController* controller, QWidget* parent = NULL);
	
	/** Destroys the HomeScreenWidgetMobile. */
	virtual ~HomeScreenWidgetMobile();
	
public slots:
	/** Sets the list of recent files. */
	virtual void setRecentFiles(const QStringList& files);
	
	/** Sets the "checked" state of the control for opening
	 *  the most recently used file on startup. */
	virtual void setOpenMRUFileChecked(bool state);
	
	/** Sets the text of the the tip-of-the-day. */
	virtual void setTipOfTheDay(const QString& text);
	
	/** Sets the visiblity of the tip-of-the-day, and
	 *  sets the "checked" state of the control for displaying the tip. */
	virtual void setTipsVisible(bool state);
	
	/** Adds the examples to the list of files
	 *  if they are not already there. */
	void showExamples();
	
protected slots:
	/** Opens a file when its is list item is clicked. */
	void fileClicked(QListWidgetItem* item);
	
protected:
	/** Draws the home screen background when a paint event occurs. */
	virtual void paintEvent(QPaintEvent* event);
	
	/** Creates the file list widget. */
	QWidget* makeFileListWidget(HomeScreenController* controller, QWidget* parent = NULL);
	
	/** Iterates over all files at the given path and adds all map files to the list */
	void addFilesToFileList(QListWidget* file_list, const QString& path);
	
private:
	QPushButton* examples_button;
	QStackedLayout* file_list_stack;
	QListWidget* file_list;
};

#endif
