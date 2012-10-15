/*
 *    Copyright 2012 Thomas Schöps
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

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

/// File type enumeration. Currently the program is only used for mapping,
/// so "Map" is the only element. "Course" or "Event" are possible additions.
struct FileType
{
	enum Enum
	{
		Map = 1,
		
		All = Map
	};
};

class MainWindow;

/** A MainWindowController provides the specific content and 
 *  behaviours for a main window.
 */
class MainWindowController : public QObject
{
Q_OBJECT
public:
	
	virtual ~MainWindowController() {}
	
	/** Save to a file.
	 *  @param path the path to save to
	 *  @return true if saving was sucessful, false on errors
	 */
	virtual bool save(const QString& path) {return false;}

	/** Load from a file.
	 *  @param path the path to load from
	 *  @return true if loading was sucessful, false on errors
	 */
	virtual bool load(const QString& path) {return false;}
	
	/** Attach the controller to a main window. 
	 *  The controller should create its user interface here.
	 */
	virtual void attach(MainWindow* window) = 0;
	
	/** Detach the controller from a main window. 
	 *  The controller should delete its user interface here.
	 */
	virtual void detach() {}
	
	// Get key press events from the main window
	virtual void keyPressEvent(QKeyEvent* event) {}
	virtual void keyReleaseEvent(QKeyEvent* event) {}
	
	/** Notify the controller of a change to the list of recent files.
	 *  FIXME: This seems to be specific for HomeScreenController.
	 */
	virtual void recentFilesUpdated() {}
	
	/** Get the main window this controller is attached to.
	 */
	inline MainWindow* getWindow() const {return window;}
	
	/** Get a controller suitable for a particular file.
	 *  @param filename the name of the file
	 *  @return a MainWindowController that is able to load the file
	 */
	static MainWindowController* controllerForFile(const QString& filename);
	
protected:
	
	MainWindow* window;
};

/** The MainWindow class provides the generic application window. 
 *  It always has an active controller (class MainWindowController) 
 *  which provides the specific window content and behaviours.
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
	virtual ~MainWindow();
	
	/** Change the controller to new_controller.
	 */
	void setController(MainWindowController* new_controller);
	inline MainWindowController* getController() const {return controller;}
	
	/** Returns the canonical path of the currently open file or 
	 *  an empty string if no file is open.
	 */
	inline const QString& getCurrentFilePath() const {return current_path;}
	
	
	void setHasOpenedFile(bool value);
	inline bool hasOpenedFile() const {return has_opened_file;}
	
	void setHasUnsavedChanges(bool value);
	inline bool hasUnsavedChanges() const {return has_unsaved_changes;}
	
	/** Sets the text in the status bar.
	 */ 
	void setStatusBarText(const QString& text);
	
	/** Enable or disable shortcuts.
	 *  During text input, it may be neccessary to disable shortcuts.
	 *  @param enable true for enabling shortcuts, false for disabling.
	 */
	inline void setShortcutsEnabled(bool enable) {disable_shortcuts = !enable;}
	inline bool areShortcutsDisabled() const {return disable_shortcuts;}
	
	// Getters to make it possible to extend the file menu
	inline QMenu* getFileMenu() const {return file_menu;}
	inline QAction* getFileMenuExtensionAct() const {return settings_act;}
	
	/** Save the content of the main window.
	 *  @param path the path where to save.
	 */ 
	bool savePath(const QString &path);
	
	/** Shows the open file dialog for the given file type(s) and returns the chosen file
	 *  or an empty string if the dialog is aborted.
	 */
	static QString getOpenFileName(QWidget* parent, const QString& title, FileType::Enum types);
	
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
	 *  FIXME: remove obsolete parameter.
	 *  @param show not used
	 */
	void updateRecentFileActions(bool show);
	
	/** Save the current content to the current path.
	 *  This will trigger a file-save dialog if the current path is not set (i.e. empty).
	 */
	bool save();
	
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
	
	/** Show the manual in Qt assistant.
	 *  @param filename the name of the help html file
	 *  @param fragment the fragment in the specified file to jump to
	 */
	void showHelp(QString filename = "index.html", QString fragment = "");
	
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
	
private:
	enum {
		max_recent_files = 10
	};
	
	/// Called when the user closes the window with unsaved changes. Returns true if window should be closed, false otherwise
	bool showSaveOnCloseDialog();
	
	/// Save and load window position, maximized state, etc.
	void saveWindowSettings();
	void loadWindowSettings();
	
	void updateWindowTitle();
	
	void createFileMenu();
	void createHelpMenu();

	bool eventFilter(QObject* object, QEvent* event);
	
	static MainWindow* findMainWindow(const QString& file_name);
	
	static QString makeHelpUrl(QString filename, QString fragment);
	
	
	/// The active controller
	MainWindowController* controller;
	bool show_menu;
	bool disable_shortcuts;
	
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
	static int num_windows;
};

#endif
