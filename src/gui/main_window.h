/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef OPENORIENTEERING_MAIN_WINDOW_H
#define OPENORIENTEERING_MAIN_WINDOW_H

#include <Qt>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QStringList>

#include "core/autosave.h"
#include "fileformats/file_format.h"

class QAction;
class QCloseEvent;
class QEvent;
class QKeyEvent;
class QLabel;
class QMenu;
class QStackedWidget;
class QToolBar;
class QWidget;

namespace OpenOrienteering {

class MainWindowController;


/**
 * The MainWindow class provides the generic application window.
 * 
 * It always has an active controller (class MainWindowController)
 * which provides the specific window content and behaviours.
 * The controller can be exchanged while the window is visible.
 */
class MainWindow : public QMainWindow, private Autosave
{
Q_OBJECT
public:
	struct FileInfo
	{
		// To be used with aggregate initialization
		QString file_path;
		const FileFormat* file_format;
		
		// cf. QFileInfo::filePath
		QString filePath() const { return file_path; }
		
		const FileFormat* fileFormat() const noexcept { return file_format; }
		
		operator bool() const noexcept { return !file_path.isEmpty(); }
		
	};
	
		
	/**
	 * Creates a new main window.
	 */
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	
private:
	/**
	 * Creates a new main window.
	 * 
	 * The flag as_main_window is a contradiction to the general intent of this
	 * class. The value fals is used only once, in SymbolSettingDialog. For this
	 * case, it disables some features such as the main menu.
	 * 
	 * \todo Refactor to remove the flag as_main_window.
	 */
	explicit MainWindow(bool as_main_window, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	
	friend class SymbolSettingDialog;
	
public:
	/** Destroys a main window. */
	~MainWindow() override;
	
	/** Returns the application's localized name. */
	QString appName() const;
	
	
	/**
	 * Returns whether the window is operating in mobile mode.
	 * 
	 * On the desktop, the default (desktop) mode may be overwritten by
	 * setting the environment variable MAPPER_MOBILE_GUI to 0 or 1.
	 * 
	 * For Android, this evaluates to constexpr true so that the compiler
	 * may optimize away desktop code in conditional blocks.
	 */
#ifndef Q_OS_ANDROID
	static bool mobileMode();
#else
	static constexpr bool mobileMode() { return true; }
#endif
	
	
	/**
	 * Changes the controller.
	 *
	 * The new controller does not edit a file.
	 */
	void setController(MainWindowController* new_controller);
	
	/**
	 * Changes the controller.
	 *
	 * The new controller edits the file with the given path.
	 * The path may be empty for a new (unnamed) file.
	 */
	void setController(MainWindowController* new_controller, const QString& path, const FileFormat* format);
	
private:
	void setController(MainWindowController* new_controller, bool has_file);

public:	
	/** Returns the current controller. */
	MainWindowController* getController() const;
	
	
	/**
	 * Returns the canonical path of the currently open file.
	 * 
	 * It no file is open, returns an empty string.
	 */
	const QString& currentPath() const { return current_path; }
	
	/**
	 * Returns the file format of the currently open file.
	 * 
	 * It no file is open or the format is unknown, returns nullptr.
	 */
	const FileFormat* currentFormat() const { return current_format; }
	
	
	
	/** Registers the given path as most recently used file.
	 * 
	 *  The path is added at (or moved to) the top of the list of most recently
	 *  used files, and the directory is saved as most recently used directory.
	 */
	static void setMostRecentlyUsedFile(const QString& path);
	
	/** Returns true if a file is opened in this main window. */
	bool hasOpenedFile() const;
	
	
	/** Returns true if the opened file is marked as having unsaved changes. */
	bool hasUnsavedChanges() const;
	
	
	/** Sets the text in the status bar. */ 
	void setStatusBarText(const QString& text);
	
	/** Shows a temporary message in the status bar. */
	void showStatusBarMessage(const QString& text, int timeout = 0);
	
	/** Clears temporary messages set in the status bar with showStatusBarMessage(). */
	void clearStatusBarMessage();
	
	
	/**
	 * Blocks shortcuts.
	 * 
	 * During text input, it may be neccessary to disable shortcuts.
	 * 
	 * @param blocked true for blocking shortcuts, false for normal behaviour.
	 */
	void setShortcutsBlocked(bool blocked);
	
	/** Returns true if shortcuts are currently disabled. */
	bool shortcutsBlocked() const;
	
	
	/** Returns the main window's file menu so that it can be extended. */
	QMenu* getFileMenu() const;
	
	/** Returns an QAction which serves as extension point in the file menu. */
	QAction* getFileMenuExtensionAct() const;
	
	/** Returns the save action. */
	QAction* getSaveAct() const;
	
	/** Returns the close action. */
	QAction* getCloseAct() const;
	
	
	/**
	 * Returns a general toolbar with standard file actions (new, open, save).
	 * 
	 * The MainWindowController is responsible to add it to the main window.
	 * It will be destroyed (and recreated) when the controller changes.
	 */
	QToolBar* getGeneralToolBar() const;
	
	
	/** Open the file with the given path after all events have been processed.
	 *  May open a new main window.
	 *  If loading is successful, the selected path will become
	 *  the [new] window's current path.
	 */
	void openPathLater(const QString &path);
	
	/**
	 * Save the content of the main window.
	 */ 
	bool saveTo(const QString &path, const FileFormat* format = nullptr);
	
	/** Shows the open file dialog for the given file type(s) and returns the chosen file
	 *  or an empty string if the dialog is aborted.
	 */
	static MainWindow::FileInfo getOpenFileName(QWidget* parent, const QString& title, FileFormat::FileTypes types);
	
	
	/**
	 * Shows a message box for a list of unformatted messages.
	 */
	static void showMessageBox(QWidget* parent, const QString& title, const QString& headline, const std::vector<QString>& messages);
	
	
	/**
	 * Sets the MainWindow's effective central widget.
	 * 
	 * Any previously set widget will be hidden and scheduled for deletion.
	 * 
	 * Hides an implementation in QMainWindow which causes problems with
	 * dock widgets when switching from home screen widget to map widget.
	 * NEVER call QMainWindow::setCentralWidget(...) on a MainWindow.
	 */
	void setCentralWidget(QWidget* widget);
	
	
	/**
	 * Indicates whether the home screen is disabled.
	 * 
	 * Normally the last main window will return to the home screen when a file
	 * is closed. When the home screen is disabled, the last window will be
	 * closed instead.
	 */
	bool homeScreenDisabled() const;
	
	/**
	 * Sets whether to show the home screen after closing the last file.
	 * 
	 * @see homeScreenDisabled()
	 */
	void setHomeScreenDisabled(bool disabled);
	
public slots:
	/**
	 * Reacts to application state changes.
	 * 
	 * On Android, when the application state becomes Qt::ApplicationActive,
	 * this method looks for the Android activity's current intent and triggers
	 * the loading of a given file (if there is not already another file loaded).
	 * 
	 * In general, when called for the first time after application start, it
	 * opens the most recently used file, unless this feature is disabled in the
	 * settings, and unless other files are registered for opening (i.e. files
	 * given as command line parameters.)
	 */
	void applicationStateChanged();
	
	/**
	 * Show a wizard for creating new maps.
	 * 
	 * May open a new main window.
	 */
	void showNewMapWizard();
	
	/**
	 * Show a file-open dialog and load the select file.
	 * 
	 * May open a new main window.
	 * If loading is successful, the selected path will become
	 * the [new] window's current path.
	 */
	void showOpenDialog();
	
	/**
	 * Show a file-save dialog.
	 * 
	 * If saving is successful, the selected path will become
	 * this window's current path.
	 * 
	 * @return true if saving was succesful, false otherwise
	 */
	bool showSaveAsDialog();
	
	/**
	 * Open the file with the given path.
	 * 
	 * May open a new main window.
	 * If loading is successful, the selected path will become
	 * the [new] window's current path.
	 * 
	 * @return true if loading was succesful, false otherwise
	 */
	bool openPath(const QString &path);
	
	bool openPath(const QString &path, const FileFormat* format);
	
	/**
	 * Open the file specified in the sending action's data.
	 * 
	 * This is intended for opening recent files.
	 */
	void openRecentFile();
	
	/**
	 * Notify the main window of a change to the list of recent files.
	 */
	void updateRecentFileActions();
	
	/**
	 * Save the current content to the current path.
	 * 
	 * This will trigger a file-save dialog if the current path is not set (i.e. empty).
	 */
	bool save();
	
	/** Save the current content to the current path.
	 */
	Autosave::AutosaveResult autosave() override;
	
	/**
	 * Close the file currently opened.
	 * 
	 * If there are changes to the current file, the user will be asked if he
	 * wants to save it - the user may even cancel the closing of the file.
	 * 
	 * This will close the window unless this is the last window.
	 * 
	 * @return True if the file was actually closed, false otherwise.
	 */
	bool closeFile();
	
	/** Toggle between normal window and fullscreen mode.
	 */
	void toggleFullscreenMode();
	
	/** Show the settings dialog.
	 */
	void showSettings();
	
	/** Show the about dialog.
	 */
	void showAbout();
	
	/** Show the index page of the manual in the help browser.
	 */
	void showHelp();
	
	/** Open a link.
	 *  This is called when the user clicks on a link in the UI,
	 *  e.g. in the tip of the day.
	 * 
	 * 	@param link the target URI
	 */
	void linkClicked(const QString &link);
	
	/**
	 * Notifies this window of unsaved changes.
	 * 
	 * If the controller was set as having an opened file, setting this value to
	 * true will start the autosave countdown if the previous value was false.
	 * 
	 * This will update the window title via QWidget::setWindowModified().
	 */
	void setHasUnsavedChanges(bool value);
	
signals:
	/**
	 * This signal is emitted when the actual path changes.
	 * 
	 * @see switchActualPath()
	 */
	void actualPathChanged(const QString &path);
	
	/**
	 * This signal is emitted when an autosave conflict gets resolved.
	 * 
	 * @see setHasAutosaveConflict()
	 */
	void autosaveConflictResolved();
	
protected slots:
	/**
	 * Switches to a different controller and loads the given path.
	 * 
	 * This method is meant for switching between an original file and
	 * autosaved versions. It does not touch current_path. The class of the new
	 * controller is determined from the current_path (i.e. original file).
	 * 
	 * If the given path is the current actual_path, no change is made.
	 * 
	 * If the currently loaded file was modified, the user is asked whether he
	 * really wants to switch to another file which means loosing the changes
	 * he had made.
	 */
	void switchActualPath(const QString &path);
	
	/**
	 * Open the files which have been registered by openPathLater().
	 */
	void openPathBacklog();
	
	/**
	 * Listens to configuration changes.
	 */
	void settingsChanged();
	
protected:
	/** 
	 * Sets the path of the file edited by this windows' controller.
	 * 
	 * This will update the window title via QWidget::setWindowFilePath().
	 * 
	 * If the controller was not set as having an opened file,
	 * the path must be empty.
	 */
	void setCurrentFile(const QString& path, const FileFormat* format);
	
	/**
	 * Notifies the windows of autosave conflicts.
	 * 
	 * An autosave conflict is the situation where a autosaved file exists
	 * when the original file is opened. This autosaved file indicates that
	 * the original file was not properly closed, i.e. the software crashed
	 * before closing.
	 */
	void setHasAutosaveConflict(bool value);
	
	/**
	 * Removes the autosave file if it exists.
	 * 
	 * Returns true if the file was removed or didn't exist, false otherwise.
	 */
	bool removeAutosaveFile() const;
	
	bool event(QEvent* event) override;
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	
	bool eventFilter(QObject* object, QEvent* event) override;
	
private:
	static constexpr int max_recent_files = 10;
	
	/**
	 * Conditionally shows a dialog for saving pending changes.
	 * 
	 * If this main window has an opened file with unsaved changes, shows
	 * a dialog which lets the user save the file, discard the changes or
	 * cancel. 
	 * 
	 * Returns true if the window can be closed, false otherwise.
	 */
	bool showSaveOnCloseDialog();
	
	
	/** Saves the window position and state. */
	void saveWindowSettings();
	
	/** Loads the window position and state. */
	void loadWindowSettings();
	
	
	void createFileMenu();
	void createHelpMenu();

	static MainWindow* findMainWindow(const QString& file_name);
	
	
	/// The active controller
	MainWindowController* controller;
	const bool create_menu;
	bool show_menu;
	bool shortcuts_blocked;
	
	QToolBar* general_toolbar;
	QMenu* file_menu;
	QAction* save_act;
	QMenu* open_recent_menu;
	bool open_recent_menu_inserted;
	QAction* recent_file_act[max_recent_files];
	QAction* settings_act;
	QAction* close_act;
	QLabel* status_label;
	
	/// Canonical path to the currently open file or an empty string if the file was not saved yet ("untitled")
	QString current_path;
	/// The current file's format, as determined during opening the file.
	const FileFormat* current_format;
	/// The actual path loaded by the editor. @see switchActualPath()
	QString actual_path;
	/// Does the main window display a file? If yes, new controllers will be opened in new main windows instead of replacing the active controller of this one
	bool has_opened_file;
	/// If this window has an opened file: does this file have unsaved changes?
	bool has_unsaved_changes;
	/// Indicates the presence of an autosave conflict. @see setHasAutosaveConflict()
	bool has_autosave_conflict;
	
	/// Was the window maximized before going into fullscreen mode? In this case, we have to show it maximized again when leaving fullscreen mode.
	bool maximized_before_fullscreen;
	
	bool homescreen_disabled;

	/// Number of active main windows. The last window shall not close on File > Close.
	static int num_open_files;
	
	/// The central widget which never changes during a MainWindow's lifecycle
	QStackedWidget* central_widget;
	
	/// A list of paths to be opened later
	QStringList path_backlog;
};


// ### MainWindow inline code ###

inline
MainWindowController* MainWindow::getController() const
{
	return controller;
}



inline
bool MainWindow::hasOpenedFile() const
{
	return has_opened_file;
}

inline
bool MainWindow::hasUnsavedChanges() const
{
	return has_unsaved_changes;
}

inline
bool MainWindow::shortcutsBlocked() const
{
	return shortcuts_blocked;
}

inline
QMenu* MainWindow::getFileMenu() const
{
	return file_menu;
}

inline
QAction* MainWindow::getFileMenuExtensionAct() const
{
	return settings_act;
}

inline
QAction* MainWindow::getSaveAct() const
{
	return save_act;
}

inline
QAction* MainWindow::getCloseAct() const
{
	return close_act;
}

inline
QToolBar* MainWindow::getGeneralToolBar() const
{
	return general_toolbar;
}

inline
bool MainWindow::homeScreenDisabled() const
{
	return homescreen_disabled;
}


}  // namespace OpenOrienteering

#endif
