/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps, Kai Pastor
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


#ifndef OPENORIENTEERING_HOME_SCREEN_WIDGET_H
#define OPENORIENTEERING_HOME_SCREEN_WIDGET_H

#include <vector>

#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QWidget>

class QAbstractButton;
class QCheckBox;
class QFileInfo;
class QIcon;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPaintEvent;
class QPushButton;
class QResizeEvent;
class QStackedLayout;

namespace OpenOrienteering {

class HomeScreenController;
class StorageLocation;


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
	AbstractHomeScreenWidget(HomeScreenController* controller, QWidget* parent = nullptr);
	
	/** Destroys the AbstractHomeScreenWidget. */
	~AbstractHomeScreenWidget() override;
	
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
	QLabel* makeHeadline(const QString& text, QWidget* parent = nullptr) const;
	
	/** Returns a button with the given text
	 *  for triggering a top level activity in the home screen.
	 *  There will be no icon or a default icon. */
	QAbstractButton* makeButton(const QString& text, QWidget* parent = nullptr) const;
	
	/** Returns a button with the given text and icon
	 *  for triggering a top level activity in the home screen. */
	QAbstractButton* makeButton(const QString& text, const QIcon& icon, QWidget* parent = nullptr) const;
	
	/** Returns the role to be used for storing paths. */
	constexpr static int pathRole() { return Qt::UserRole; }
	
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
	HomeScreenWidgetDesktop(HomeScreenController* controller, QWidget* parent = nullptr);
	
	/** Destroys the HomeScreenWidgetDesktop. */
	~HomeScreenWidgetDesktop() override;
	
public slots:
	/** Sets the list of recent files. */
	void setRecentFiles(const QStringList& files) override;
	
	/** Sets the "checked" state of the control for opening
	 *  the most recently used file on startup. */
	void setOpenMRUFileChecked(bool state) override;
	
	/** Sets the text of the the tip-of-the-day. */
	void setTipOfTheDay(const QString& text) override;
	
	/** Sets the visiblity of the tip-of-the-day, and
	 *  sets the "checked" state of the control for displaying the tip. */
	void setTipsVisible(bool state) override;
	
protected slots:
	/** Opens a file when its is list item is clicked. */
	void recentFileClicked(QListWidgetItem* item);
	
protected:
	/** Draws the home screen background when a paint event occurs. */
	void paintEvent(QPaintEvent* event) override;
	
	/** Creates the activities widget. */
	QWidget* makeMenuWidget(HomeScreenController* controller, QWidget* parent = nullptr);
	
	/** Creates the recent files widget. */
	QWidget* makeRecentFilesWidget(HomeScreenController* controller, QWidget* parent = nullptr);
	
	/** Creates the tip-of-the-day widget. */
	QWidget* makeTipsWidget(HomeScreenController* controller, QWidget* parent = nullptr);
	
	
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
	HomeScreenWidgetMobile(HomeScreenController* controller, QWidget* parent = nullptr);
	
	/** Destroys the HomeScreenWidgetMobile. */
	~HomeScreenWidgetMobile() override;
	
public slots:
	/** Sets the list of recent files. */
	void setRecentFiles(const QStringList& files) override;
	
	/** Sets the "checked" state of the control for opening
	 *  the most recently used file on startup. */
	void setOpenMRUFileChecked(bool state) override;
	
	/** Sets the text of the the tip-of-the-day. */
	void setTipOfTheDay(const QString& text) override;
	
	/** Sets the visiblity of the tip-of-the-day, and
	 *  sets the "checked" state of the control for displaying the tip. */
	void setTipsVisible(bool state) override;
	
	/** Shows the settings dialog, adjusted for small screens. */
	void showSettings();
	
protected:
	/** Opens a file when its is list item is clicked. */
	void fileClicked(QListWidgetItem* item);
	
	/** Triggers title image adjustment on resize events. */
	void resizeEvent(QResizeEvent* event) override;
	
	/** Resizes the title image to fit both the labels width and height */
	void adjustTitlePixmapSize();
	
	/** Creates the file list widget. */
	QListWidget* makeFileListWidget();
	
	/** Updates the file list widget. */
	void updateFileListWidget();
	
	/** Add a single file or dir item to the file list. */
	void addItemToFileList(const QFileInfo& file_info, int hint = 0, const QIcon& icon = {});
	
	/** Add a single item to the file list, using a custom display name. */
	void addItemToFileList(const QString& label, const QFileInfo& file_info, int hint = 0, const QIcon& icon = {});
	
	/** Returns the role to be used for storing hints (number or text). */
	constexpr static int hintRole() { return Qt::UserRole + 1; }
	
private:
	QPixmap title_pixmap;
	QLabel* title_label;
	QListWidget* file_list_widget;
	std::vector<StorageLocation> history;
};


}  // namespace OpenOrienteering
#endif
