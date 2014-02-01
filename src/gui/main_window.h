/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <QMainWindow>

#include "../file_format.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

class MainWindowController;

/** The MainWindow class provides the generic application window. 
 *  It always has an active controller (class MainWindowController) 
 *  which provides the specific window content and behaviours.
 *  The controller can be exchanged while the window is visible.
 */
class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	/** Creates a new main window.
	 *  FIXME: as_main_window seems to be contradictory and is used only
	 *  with value false (in SymbolSettingDialog).
	 *  @param as_main_window if false, disables the loading of save windows geometry and hides the bottom-right handle for resizing.
	 */
	MainWindow(bool as_main_window = true);
	
	/** Destroys a main window. */
	virtual ~MainWindow();
	
	/** Returns the application's name. */
	const QString& appName() const;
	
	
	/** Change the controller to new_controller. */
	void setController(MainWindowController* new_controller);
	
	/** Returns the current controller. */
	inline MainWindowController* getController() const {return controller;}
	
	
	/** Returns the canonical path of the currently open file or 
	 *  an empty string if no file is open.
	 */
	inline const QString& getCurrentFilePath() const {return current_path;}
	
	/** @brief Registers the given path as most recently used file.
	 *  The path is added at (or moved to) the top of the list of most recently
	 *  used files, and the directory is saved as most recently used directory.
	 */
	static void setMostRecentlyUsedFile(const QString& path);
	
	/** Sets the opened-file state to value. */
	void setHasOpenedFile(bool value);
	
	/** Returns true if a file is opened in this main window. */
	inline bool hasOpenedFile() const {return has_opened_file;}
	
	
	/** Sets the unsaved-changes state to value. */
	void setHasUnsavedChanges(bool value);
	
	/** Returns true if the opened file is marked as having unsaved changes. */
	inline bool hasUnsavedChanges() const {return has_unsaved_changes;}
	
	
	/** Sets the text in the status bar. */ 
	void setStatusBarText(const QString& text);
	/** Shows a temporary message in the status bar. */
	void showStatusBarMessage(const QString& text, int timeout = 0);
	/** Clears temporary messages set in the status bar with showStatusBarMessage(). */
	void clearStatusBarMessage();
	
	
	/** Enable or disable shortcuts.
	 *  During text input, it may be neccessary to disable shortcuts.
	 *  @param enable true for enabling shortcuts, false for disabling.
	 */
	inline void setShortcutsEnabled(bool enable) {disable_shortcuts = !enable;}
	
	/** Returns true if shortcuts are currently disabled. */
	inline bool areShortcutsDisabled() const {return disable_shortcuts;}
	
	
	/** Returns the main window's file menu so that it can be extended. */
	inline QMenu* getFileMenu() const {return file_menu;}
	
	/** Returns an QAction which serves as extension point in the file menu. */
	inline QAction* getFileMenuExtensionAct() const {return settings_act;}
	
	/** Returns the save action. */
	inline QAction* getSaveAct() const {return save_act;}
	
	/** Returns the close action. */
	inline QAction* getCloseAct() const {return close_act;}
	
	
	/**
	 * Returns a general toolbar with standard file actions (new, open, save).
	 * 
	 * The MainWindowController is responsible to add it to the main window.
	 * It will be destroyed (and recreated) when the controller changes.
	 */
	inline QToolBar* getGeneralToolBar() const { return general_toolbar; }
	
	
	/** Open the file with the given path after all events have been processed.
	 *  May open a new main window.
	 *  If loading is successful, the selected path will become
	 *  the [new] window's current path.
	 */
	void openPathLater(const QString &path);
	
	/** Save the content of the main window.
	 *  @param path the path where to save.
	 */ 
	bool savePath(const QString &path);
	
	/** Shows the open file dialog for the given file type(s) and returns the chosen file
	 *  or an empty string if the dialog is aborted.
	 */
	static QString getOpenFileName(QWidget* parent, const QString& title, FileFormat::FileTypes types);
	
	/**
	 * Sets the MainWindow's effective central widget.
	 * Any previously set widget will be hidden and scheduled for deletion.
	 * 
	 * Hides an implementation in QMainWindow which causes problems with
	 * dock widgets when switching from home screen widget to map widget.
	 * NEVER call QMainWindow::setCentralWidget(...) on a MainWindow.
	 */
	void setCentralWidget(QWidget* widget);
	
public slots:
	/** Show a wizard for creating new maps.
	 *  May open a new main window.
	 */
	void showNewMapWizard();
	
	/** Show a file-open dialog and load the select file.
	 *  May open a new main window.
	 *  If loading is successful, the selected path will become
	 *  the [new] window's current path.
	 */
	void showOpenDialog();
	
	/** Show a file-save dialog.
	 *  If saving is successful, the selected path will become
	 *  this window's current path.
	 *  @return true if saving was succesful, false otherwise
	 */
	bool showSaveAsDialog();
	
	/** Open the file with the given path.
	 *  May open a new main window.
	 *  If loading is successful, the selected path will become
	 *  the [new] window's current path.
	 *  @return true if loading was succesful, false otherwise
	 */
	bool openPath(const QString &path);
	
	/** Open the file specified in the sending action's data.
	 *  This is intended for opening recent files.
	 */
	void openRecentFile();
	
	/** Notify the main window of a change to the list of recent files.
	 */
	void updateRecentFileActions();
	
	/** Save the current content to the current path.
	 *  This will trigger a file-save dialog if the current path is not set (i.e. empty).
	 */
	bool save();
	
	/** Save the current content to the current path.
	 */
	void autoSave();
	
	/** Close the file currently opened.
	 *  This will close the window unless this is the last window.
	 */
	void closeFile();
	
	/** Toggle between normal window and fullscreen mode.
	 */
	void toggleFullscreenMode();
	
	/** Show the settings dialog.
	 */
	void showSettings();
	
	/** Show the about dialog.
	 */
	void showAbout();
	
	/** Show the index page of the manual in Qt assistant.
	 */
	void showHelp();
	
	/** Open a link.
	 *  This is called when the user clicks on a link in the UI,
	 *  e.g. in the tip of the day.
	 * 
	 * 	@param link the target URI
	 */
	void linkClicked(const QString &link);
	
	/** Notify the main window of unsaved changes.
	 */
	void gotUnsavedChanges();
	
signals:
	void keyPressed(QKeyEvent* event);
	void keyReleased(QKeyEvent* event);
	
protected slots:
	/**
	 * Open the files which have been registered by openPathLater().
	 */
	void openPathBacklog();
	
	/**
	 * Listens to configuration changes.
	 */
	void settingsChanged();
	
protected:
	/** Notify main window of the current path where the content is saved.
	 *  This will trigger updates to the window title, 
	 *  to the list of recently used files, and 
	 *  to the least recently used directory.
	 */
	void setCurrentFile(const QString& path);
	
	virtual bool event(QEvent* event);
	virtual void closeEvent(QCloseEvent *event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	
	/**
	 * Configures the auto-save feature from the current settings.
	 * 
	 * When auto-save gets activated, and the times is not yet active,
	 * it starts the timer.
	 */
	void updateAutoSave();
	
private:
	enum {
		max_recent_files = 10
	};
	
	/** If this main window has an opened file with unsaved changes, shows
	 *  a dialog which lets the user save the file, discard the changes or
	 *  cancel. 
	 *  Returns true if the window can be closed, false otherwise.
	 */
	bool showSaveOnCloseDialog();
	
	
	/** Saves the window position and state. */
	void saveWindowSettings();
	
	/** Loads the window position and state. */
	void loadWindowSettings();
	
	
	void updateWindowTitle();
	
	void createFileMenu();
	void createHelpMenu();

	bool eventFilter(QObject* object, QEvent* event);
	
	static MainWindow* findMainWindow(const QString& file_name);
	
	
	/// The active controller
	MainWindowController* controller;
	bool create_menu;
	bool show_menu;
	bool disable_shortcuts;
	
	QToolBar* general_toolbar;
	QMenu* file_menu;
	QAction* save_act;
	QAction* save_as_act;
	QMenu* open_recent_menu;
	bool open_recent_menu_inserted;
	QAction* recent_file_act[max_recent_files];
	QAction* settings_act;
	QAction* close_act;
	QLabel* status_label;
	
	/// Canonical path to the currently open file or an empty string if the file was not saved yet ("untitled")
	QString current_path;
	/// Does the main window display a file? If yes, new controllers will be opened in new main windows instead of replacing the active controller of this one
	bool has_opened_file;
	/// If this window has an opened file: does this file have unsaved changes?
	bool has_unsaved_changes;
	
	/// Was the window maximized before going into fullscreen mode? In this case, we have to show it maximized again when leaving fullscreen mode.
	bool maximized_before_fullscreen;

	/// Number of active main windows. The last window shall not close on File > Close.
	static int num_open_files;
	
	/// The central widget which never changes during a MainWindow's lifecycle
	QStackedWidget* central_widget;
	
	/// A list of paths to be opened later
	QStringList path_backlog;
	
	QTimer* auto_save_timer;
};

#endif
