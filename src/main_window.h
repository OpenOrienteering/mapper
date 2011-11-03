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


#ifndef _OPENORIENTEERING_MAIN_WINDOW_H_
#define _OPENORIENTEERING_MAIN_WINDOW_H_

#include <QtGui/QMainWindow>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class MainWindow;

/// Provides the window contents for a MainWindow
class MainWindowController : public QObject
{
Q_OBJECT
public:
	
	virtual ~MainWindowController() {}
	
	/// Should save the currently open file to the given path and return if that was sucessful.
	virtual bool save(const QString& path) {return false;}
	/// Should load the given file and return if that was sucessful.
	virtual bool load(const QString& path) {return false;}
	
	/// Attach to a main window. The controller should create its user interface here.
	virtual void attach(MainWindow* window) = 0;
	/// Detach from a main window. The controller should delete its user interface here.
	virtual void detach() {}
	
	inline MainWindow* getWindow() const {return window;}
	
	static MainWindowController* controllerForFile(const QString& filename);
	
protected:
	
	MainWindow* window;
};

/// The main window. Always has an active controller (class MainWindowController) which provides the window contents
class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow();
	virtual ~MainWindow();
	
	/// Change the controller
	void setController(MainWindowController* new_controller);
	
	/// Changes the current file (saves the file in the recently used lists)
	void setCurrentFile(const QString& path);
	/// Returns the canonical path of the currently open file or an empty string if no file is open
	inline const QString& getCurrentFilePath() const {return current_path;}
	
	void setHasOpenedFile(bool value);
	inline bool hasOpenedFile() const {return has_opened_file;}
	
	void setHasUnsavedChanges(bool value);
	inline bool hasUnsavedChanges() const {return has_unsaved_changes;}
	
	/// Sets the text in the status bar, which stays there as long as the current tool is active
	void setStatusBarText(const QString& text);
	
public slots:
	void showNewMapWizard();
	void showOpenDialog();
	bool openPath(QString path);
	void save();
	void saveAs();
	
	void showSettings();
	void showAbout();
	void showHelp();
	void linkClicked(QString link);
	
	void gotUnsavedChanges();
	
protected:
	virtual void closeEvent(QCloseEvent *event);
	
private:
	/// Called when the user closes the window with unsaved changes. Returns true if window should be closed, false otherwise
	bool showSaveOnCloseDialog();
	
	/// Save and load window position, maximized state, etc.
	void saveWindowSettings();
	void loadWindowSettings();
	
	void updateWindowTitle();
	
	void createFileMenu();
	void createHelpMenu();
	
	static MainWindow* findMainWindow(const QString& fileName);
	
	
	/// The active controller
	MainWindowController* controller;
	
	QAction* saveAct;
	QAction* saveAsAct;
	QLabel* status_label;
	
	/// Canonical path to the currently open file or an empty string if the file was not saved yet ("untitled")
	QString current_path;
	/// Does the main window display a file? If yes, new controllers will be opened in new main windows instead of replacing the active controller of this one
	bool has_opened_file;
	/// If this window has an opened file: does this file have unsaved changes?
	bool has_unsaved_changes;
};

#endif
