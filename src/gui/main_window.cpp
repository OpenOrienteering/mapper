/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#include "main_window.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMenuBar>
#include <QProxyStyle>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringBuilder>
#include <QToolBar>
#include <QWhatsThis>

#include <mapper_config.h>

#include "about_dialog.h"
#include "autosave_dialog.h"
#include "../file_format_registry.h"
#include "../file_import_export.h"
#include "home_screen_controller.h"
#include "../map.h"
#include "../map_dialog_new.h"
#include "../map_editor.h"
#include "../mapper_resource.h"
#include "../file_format.h"
#include "../settings.h"
#include "settings_dialog.h"
#include "text_browser_dialog.h"
#include "../util.h"

#if defined(Q_OS_ANDROID)
#include <QtAndroidExtras/QAndroidJniObject>
#endif



int MainWindow::num_open_files = 0;

MainWindow::MainWindow(bool as_main_window)
: QMainWindow()
, has_autosave_conflict(false)
, homescreen_disabled(false)
{
#if (defined Q_OS_MAC)
	// Cf. qtbase/src/plugins/platforms/cocoa/qcocoamenuloader.mm.
	// These translations should come with Qt, but were missing
	// for some languages (at least for de in Qt 5.0.1).
	static const char *application_menu_strings[] = {
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Services"),
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Hide %1"),
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Hide Others"),
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Show All"),
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Preferences..."),
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Quit %1"),
	  QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "About %1")
	};
	Q_UNUSED(application_menu_strings)
#endif

	controller = NULL;
	has_unsaved_changes = false;
	has_opened_file = false;

	create_menu = as_main_window;
	show_menu = create_menu && !mobileMode();
	
	disable_shortcuts = false;
	setCurrentPath(QString());
	maximized_before_fullscreen = false;
	general_toolbar = NULL;
	file_menu = NULL;
	
	setWindowIcon(QIcon(":/images/mapper.png"));
	setAttribute(Qt::WA_DeleteOnClose);
	
	status_label = new QLabel();
	statusBar()->addWidget(status_label, 1);
	statusBar()->setSizeGripEnabled(as_main_window);
	if (mobileMode())
		statusBar()->hide();
	
	central_widget = new QStackedWidget(this);
	QMainWindow::setCentralWidget(central_widget);
	
	if (as_main_window)
		loadWindowSettings();
	
#if defined(Q_OS_ANDROID)
	// Needed to catch Qt::Key_Back, cf. MainWindow::eventFilter()
	qApp->installEventFilter(this);
#else
	installEventFilter(this);
#endif
	
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
}

MainWindow::~MainWindow()
{
	if (controller)
	{
		controller->detach();
		delete controller;
		delete general_toolbar;
	}
}

void MainWindow::settingsChanged()
{
	updateRecentFileActions();
}

const QString& MainWindow::appName() const
{
	static QString app_name(APP_NAME);
	return app_name;
}

bool MainWindow::mobileMode() const
{
#ifdef Q_OS_ANDROID
	static bool mobile_mode = qEnvironmentVariableIsSet("MAPPER_MOBILE_GUI")
	                          ? (qgetenv("MAPPER_MOBILE_GUI") != "0")
	                          : 1;
#else
	static bool mobile_mode = qEnvironmentVariableIsSet("MAPPER_MOBILE_GUI")
	                          ? (qgetenv("MAPPER_MOBILE_GUI") != "0")
	                          : 0;
#endif
	return mobile_mode;
}

void MainWindow::setCentralWidget(QWidget* widget)
{
	if (widget != NULL)
	{
		// Main window shall not resize to central widget size hint.
		widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
		
		int index = central_widget->addWidget(widget);
		central_widget->setCurrentIndex(index);
	}
	
	if (central_widget->count() > 1)
	{
		QWidget* w = central_widget->widget(0);
		central_widget->removeWidget(w);
		w->deleteLater();
	}
}

void MainWindow::setHomeScreenDisabled(bool disabled)
{
	homescreen_disabled = disabled;
}

void MainWindow::setController(MainWindowController* new_controller, const QString& path)
{
	if (controller)
	{
		controller->detach();
		delete controller;
		controller = NULL;
		
		if (show_menu)
			menuBar()->clear();
		delete general_toolbar;
		general_toolbar = NULL;
	}
	
	has_opened_file = false;
	has_unsaved_changes = false;
	disable_shortcuts = false;
	setCurrentPath(path);
	
	if (create_menu)
		createFileMenu();
	
	controller = new_controller;
	controller->attach(this);
	
	if (create_menu)
		createHelpMenu();
		
#if defined(Q_OS_MAC)
	if (isVisible() && qApp->activeWindow() == this)
	{
		// Force a menu synchronisation,
		// QCocoaMenuBar::updateMenuBarImmediately(),
		// via QCocoaNativeInterface::onAppFocusWindowChanged().
		qApp->focusWindowChanged(qApp->focusWindow());
	}
#endif
}

void MainWindow::createFileMenu()
{
	QAction* new_act = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
	new_act->setShortcuts(QKeySequence::New);
	new_act->setStatusTip(tr("Create a new map"));
	new_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(new_act, SIGNAL(triggered()), this, SLOT(showNewMapWizard()));
	
	QAction* open_act = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
	open_act->setShortcuts(QKeySequence::Open);
	open_act->setStatusTip(tr("Open an existing file"));
	open_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(open_act, SIGNAL(triggered()), this, SLOT(showOpenDialog()));
	
	open_recent_menu = new QMenu(tr("Open &recent"), this);
	open_recent_menu->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	for (int i = 0; i < max_recent_files; ++i)
	{
		recent_file_act[i] = new QAction(this);
		connect(recent_file_act[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
	open_recent_menu_inserted = false;
	
	// NOTE: if you insert something between open_recent_menu and save_act, adjust updateRecentFileActions()!
	
	save_act = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
	save_act->setShortcuts(QKeySequence::Save);
	save_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(save_act, SIGNAL(triggered()), this, SLOT(save()));
	
	save_as_act = new QAction(tr("Save &as..."), this);
	if (QKeySequence::keyBindings(QKeySequence::SaveAs).empty())
		save_as_act->setShortcut(tr("Ctrl+Shift+S"));
	else
		save_as_act->setShortcuts(QKeySequence::SaveAs);
	save_as_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(save_as_act, SIGNAL(triggered()), this, SLOT(showSaveAsDialog()));
	
	settings_act = new QAction(tr("Settings..."), this);
#if defined(Q_OS_MAC)
	settings_act->setShortcut(QKeySequence::Preferences);
	settings_act->setMenuRole(QAction::PreferencesRole);
#endif
	connect(settings_act, SIGNAL(triggered()), this, SLOT(showSettings()));
	
	close_act = new QAction(QIcon(":/images/close.png"), tr("Close"), this);
	close_act->setShortcut(QKeySequence::Close);
	close_act->setStatusTip(tr("Close this file"));
	close_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(close_act, SIGNAL(triggered()), this, SLOT(closeFile()));
	
	QAction* exit_act = new QAction(tr("E&xit"), this);
	exit_act->setShortcuts(QKeySequence::Quit);
	exit_act->setStatusTip(tr("Exit the application"));
#if defined(Q_OS_MAC)
	exit_act->setMenuRole(QAction::QuitRole);
#endif
	exit_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(exit_act, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	
	if (show_menu)
		file_menu = menuBar()->addMenu(tr("&File"));
	else
	{
		delete file_menu;
		file_menu = new QMenu(this);
	}

	file_menu->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	file_menu->addAction(new_act);
	file_menu->addAction(open_act);
	file_menu->addAction(save_act);
	file_menu->addAction(save_as_act);
	file_menu->addSeparator();
	file_menu->addAction(settings_act);
	file_menu->addSeparator();
	file_menu->addAction(close_act);
	file_menu->addAction(exit_act);
	
	general_toolbar = new QToolBar(tr("General"));
	general_toolbar->setObjectName("General toolbar");
	general_toolbar->addAction(new_act);
	general_toolbar->addAction(open_act);
	general_toolbar->addAction(save_act);
	
	save_act->setEnabled(false);
	save_as_act->setEnabled(false);
	close_act->setEnabled(false);
	updateRecentFileActions();
}

void MainWindow::createHelpMenu()
{
	// Help menu
	QAction* manualAct = new QAction(QIcon(":/images/help.png"), tr("Open &Manual"), this);
	manualAct->setStatusTip(tr("Show the help file for this application"));
	manualAct->setShortcut(QKeySequence::HelpContents);
	connect(manualAct, SIGNAL(triggered()), this, SLOT(showHelp()));
	
	QAction* aboutAct = new QAction(tr("&About %1").arg(APP_NAME), this);
	aboutAct->setStatusTip(tr("Show information about this application"));
#if defined(Q_OS_MAC)
	aboutAct->setMenuRole(QAction::AboutRole);
#endif
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(showAbout()));
	
	QAction* aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setStatusTip(tr("Show information about Qt"));
#if defined(Q_OS_MAC)
	aboutQtAct->setMenuRole(QAction::AboutQtRole);
#endif
	connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	
	if (show_menu)
	{
		QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
		helpMenu->addAction(manualAct);
		helpMenu->addAction(QWhatsThis::createAction(this));
		helpMenu->addSeparator();
		helpMenu->addAction(aboutAct);
		helpMenu->addAction(aboutQtAct);
	}
}

void MainWindow::setCurrentPath(const QString& path)
{
	if (current_path != path)
	{
		current_path = QFileInfo(path).canonicalFilePath();
		updateWindowTitle();
	}
}

void MainWindow::setMostRecentlyUsedFile(const QString& path)
{
	if (!path.isEmpty())
	{
		Settings& settings = Settings::getInstance();
		
		// Update least recently used directory
		const QString open_directory = QFileInfo(path).canonicalPath();
		QSettings().setValue("openFileDirectory", open_directory);
		
		// Update recent file lists
		QStringList files = settings.getSettingCached(Settings::General_RecentFilesList).toStringList();
		files.removeAll(path);
		files.prepend(path);
		if (files.size() > max_recent_files)
			files.erase(files.begin() + max_recent_files, files.end());
		settings.setSetting(Settings::General_RecentFilesList, files);
	}
}

void MainWindow::setHasOpenedFile(bool value)
{
	if (create_menu)
	{
		if (value && !has_opened_file)
		{
			save_act->setEnabled(true);
			save_as_act->setEnabled(true);
			close_act->setEnabled(true);
		}
		else if (!value && has_opened_file)
		{
			save_act->setEnabled(false);
			save_as_act->setEnabled(false);
			close_act->setEnabled(false);
		}
	}
	has_opened_file = value;
	updateWindowTitle();
}

void MainWindow::setHasUnsavedChanges(bool value)
{
	has_unsaved_changes = value;
	setAutosaveNeeded(has_unsaved_changes && !has_autosave_conflict);
	updateWindowTitle();
}

void MainWindow::setStatusBarText(const QString& text)
{
	status_label->setText(text);
	status_label->setToolTip(text);
}

void MainWindow::showStatusBarMessage(const QString& text, int timeout)
{
#if defined(Q_OS_ANDROID)
	Q_UNUSED(timeout);
	QAndroidJniObject java_string = QAndroidJniObject::fromString(text);
	QAndroidJniObject::callStaticMethod<void>(
		"org/openorienteering/mapper/MapperActivity",
		"showShortMessage",
		"(Ljava/lang/String;)V",
		java_string.object<jstring>());
#else
	statusBar()->showMessage(text, timeout);
#endif
}

void MainWindow::clearStatusBarMessage()
{
#if !defined(Q_OS_ANDROID)
	statusBar()->clearMessage();
#endif
}

bool MainWindow::closeFile()
{
	bool closed = !has_opened_file || showSaveOnCloseDialog();
	if (closed)
	{
		if (has_opened_file)
		{
			num_open_files--;
			has_opened_file = false;
		}
		if (homescreen_disabled || num_open_files > 0)
			close();
		else
			setController(new HomeScreenController());
	}
	return closed;
}

bool MainWindow::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && disable_shortcuts)
		event->accept();
	
	return QMainWindow::event(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	if (!has_opened_file)
	{
		saveWindowSettings();
		event->accept();
	}
	else if (showSaveOnCloseDialog())
	{
		if (has_opened_file)
		{
			num_open_files--;
			has_opened_file = false;
		}
		saveWindowSettings();
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if (controller && controller->keyPressEventFilter(event))
	{
		// Event filtered, stop handling
		return;
	}
	
	QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
	if (controller && controller->keyReleaseEventFilter(event))
	{
		// Event filtered, stop handling
		return;
	}
	
	QMainWindow::keyReleaseEvent(event);
}

bool MainWindow::showSaveOnCloseDialog()
{
	if (has_opened_file && (has_unsaved_changes || has_autosave_conflict))
	{
		// Show the window in case it is minimized
		setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		raise();
		activateWindow();
		
		QMessageBox::StandardButton ret;
		if (!has_unsaved_changes && actual_path != autosavePath(currentPath()))
		{
			ret = QMessageBox::warning(this, APP_NAME,
			                           tr("Do you want to remove the autosaved version?"),
			                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		}
		else
		{
			ret = QMessageBox::warning(this, APP_NAME,
			                           tr("The file has been modified.\n"
			                              "Do you want to save your changes?"),
			                           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		}
		
		switch (ret)
		{
		case QMessageBox::Cancel:
			return false;
			
		case QMessageBox::Discard:
			if (has_autosave_conflict)
				setHasAutosaveConflict(false);
			else
				removeAutosaveFile();
			break;
			
		case QMessageBox::Save:
			if (!save())
				return false;
			// fall through
			
		case QMessageBox::Yes:
			setHasAutosaveConflict(false);
			removeAutosaveFile();
			break;
			
		case QMessageBox::No:
			setHasAutosaveConflict(false);
			break;
			
		default:
			Q_ASSERT(false && "Unsupported return value from message box");
			break;
		}
		
	}
	
	return true;
}

void MainWindow::saveWindowSettings()
{
#if !defined(Q_OS_ANDROID)
	QSettings settings;
	
	settings.beginGroup("MainWindow");
	settings.setValue("pos", pos());
	settings.setValue("size", size());
	settings.setValue("maximized", isMaximized());
	settings.endGroup();
#endif
}

void MainWindow::loadWindowSettings()
{
#if defined(Q_OS_ANDROID)
	// Always show the window on the whole available area on Android
	resize(QApplication::desktop()->availableGeometry().size());
#else
	QSettings settings;
	
	settings.beginGroup("MainWindow");
	QPoint pos = settings.value("pos", QPoint(100, 100)).toPoint();
	QSize size = settings.value("size", QSize(800, 600)).toSize();
	bool maximized = settings.value("maximized", false).toBool();
	settings.endGroup();
	
	move(pos);
	resize(size);
	if (maximized)
		showMaximized();
#endif
}

MainWindow* MainWindow::findMainWindow(const QString& file_name)
{
	QString canonical_file_path = QFileInfo(file_name).canonicalFilePath();
	if (canonical_file_path.isEmpty())
		return NULL;
	
	for (auto widget : qApp->topLevelWidgets())
	{
		MainWindow* other = qobject_cast<MainWindow*>(widget);
		if (other && other->currentPath() == canonical_file_path)
			return other;
	}
	
	return NULL;
}

void MainWindow::updateWindowTitle()
{
	QString window_title = "";
	
	if (has_unsaved_changes)
		window_title += "(*)";
	
	if (has_opened_file)
	{
		const QString current_file_path = currentPath();
		if (current_file_path.isEmpty())
			window_title += tr("Unsaved file") + " - ";
		else
			window_title += QFileInfo(current_file_path).fileName() + " - ";
	}
	
	window_title += APP_NAME + " " + APP_VERSION;
	
	setWindowTitle(window_title);
}

void MainWindow::showNewMapWizard()
{
	NewMapDialog newMapDialog(this);
	newMapDialog.setWindowModality(Qt::WindowModal);
	newMapDialog.exec();
	
	if (newMapDialog.result() == QDialog::Rejected)
		return;
	
	Map* new_map = new Map();
	QString symbol_set_path = newMapDialog.getSelectedSymbolSetPath();
	if (symbol_set_path.isEmpty())
		new_map->setScaleDenominator(newMapDialog.getSelectedScale());
	else
	{
		new_map->loadFrom(symbol_set_path, this, NULL, true);
		if (new_map->getScaleDenominator() != newMapDialog.getSelectedScale())
		{
			if (QMessageBox::question(this, tr("Warning"), tr("The selected map scale is 1:%1, but the chosen symbol set has a nominal scale of 1:%2.\n\nDo you want to scale the symbols to the selected scale?").arg(newMapDialog.getSelectedScale()).arg(new_map->getScaleDenominator()),  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				double factor = new_map->getScaleDenominator() / (double)newMapDialog.getSelectedScale();
				new_map->scaleAllSymbols(factor);
			}
			
			new_map->setScaleDenominator(newMapDialog.getSelectedScale());
		}
	}
	
	MainWindow* new_window;
	if (has_opened_file)
		new_window = new MainWindow(true);
	else
		new_window = this;
	new_window->setController(new MapEditorController(MapEditorController::MapEditor, new_map));
	
	new_window->show();
	new_window->raise();
	new_window->activateWindow();
	num_open_files++;
}

void MainWindow::showOpenDialog()
{
	QString path = getOpenFileName(this, tr("Open file"), FileFormat::AllFiles);
	
	if (path.isEmpty())
		return;
	
	openPath(path);
}

bool MainWindow::openPath(const QString &path)
{
#ifndef Q_OS_ANDROID
	// Empty path does nothing. This also helps with the single instance application code.
	if (path.isEmpty())
		return true;
	
	MainWindow* const existing = findMainWindow(path);
	if (existing)
	{
		existing->show();
		existing->raise();
		existing->activateWindow();
		return true;
	}
#endif
	
	// Check a blocker that prevents immediate re-opening of crashing files.
	// Needed for stopping auto-loading a crashing file on startup.
	static const QString reopen_blocker = "open_in_progress";
	QSettings settings;
	const QString open_in_progress(settings.value(reopen_blocker).toString());
	if (open_in_progress == path)
	{
		int result = QMessageBox::warning(this, tr("Crash warning"), 
		  tr("It seems that %1 crashed the last time this file was opened:<br /><tt>%2</tt><br /><br />Really retry to open it?").arg(appName()).arg(path),
		  QMessageBox::Yes | QMessageBox::No);
		settings.remove(reopen_blocker);
		if (result == QMessageBox::No)
			return false;
	}
	
	settings.setValue(reopen_blocker, path);
	settings.sync();
	
	MainWindowController* const new_controller = MainWindowController::controllerForFile(path);
	if (!new_controller)
	{
		QMessageBox::warning(this, tr("Error"), tr("Cannot open file:\n%1\n\nFile format not recognized.").arg(path));
		settings.remove(reopen_blocker);
		return false;
	}
	
	QString new_actual_path = path;
	QString autosave_path = Autosave::autosavePath(path);
	bool new_autosave_conflict = QFileInfo(autosave_path).exists();
	if (new_autosave_conflict)
	{
#if defined(Q_OS_ANDROID)
		// Assuming small screen, showing dialog before opening the file
		AutosaveDialog* autosave_dialog = new AutosaveDialog(path, autosave_path, autosave_path, this);
		int result = autosave_dialog->exec();
		new_actual_path = (result == QDialog::Accepted) ? autosave_dialog->selectedPath() : QString();
		delete autosave_dialog;
#else
		// Assuming large screen, dialog will be shown while the autosaved file is open
		new_actual_path = autosave_path;
#endif
	}
	
	if (new_actual_path.isEmpty() || !new_controller->load(new_actual_path, this))
	{
		delete new_controller;
		settings.remove(reopen_blocker);
		return false;
	}
	
	MainWindow* open_window = this;
#if !defined(Q_OS_ANDROID)
	if (has_opened_file)
		open_window = new MainWindow(true);
#endif
	
	open_window->setController(new_controller, path);
	open_window->actual_path = new_actual_path;
	open_window->setHasAutosaveConflict(new_autosave_conflict);
	open_window->setHasUnsavedChanges(false);
	
	open_window->setVisible(true); // Respect the window flags set by new_controller.
	open_window->raise();
	num_open_files++;
	settings.remove(reopen_blocker);
	setMostRecentlyUsedFile(path);
	
#if !defined(Q_OS_ANDROID)
	// Assuming large screen. Android handled above.
	if (new_autosave_conflict)
	{
		QDialog* autosave_dialog = new AutosaveDialog(path, autosave_path, new_actual_path, open_window, Qt::WindowTitleHint | Qt::CustomizeWindowHint);
		autosave_dialog->move(open_window->rect().right() - autosave_dialog->width(), open_window->rect().top());
		autosave_dialog->show();
		autosave_dialog->raise();
		
		connect(autosave_dialog, SIGNAL(pathSelected(QString)), open_window, SLOT(switchActualPath(QString)));
		connect(open_window, SIGNAL(actualPathChanged(QString)), autosave_dialog, SLOT(setSelectedPath(QString)));
		connect(open_window, SIGNAL(autosaveConflictResolved()), autosave_dialog, SLOT(autosaveConflictResolved()));
	}
#endif
	
	open_window->activateWindow();
	
	return true;
}

void MainWindow::switchActualPath(const QString& path)
{
	if (path == actual_path)
	{
		return;
	}
	
	int ret = QMessageBox::Ok;
	if (has_unsaved_changes)
	{
		ret = QMessageBox::warning(this, APP_NAME,
		                           tr("The file has been modified.\n"
		                              "Do you want to discard your changes?"),
		                           QMessageBox::Discard | QMessageBox::Cancel);
	}
	
	if (ret != QMessageBox::Cancel)
	{
		const QString& current_path = currentPath();
		MainWindowController* const new_controller = MainWindowController::controllerForFile(current_path);
		if (new_controller && new_controller->load(path, this))
		{
			setController(new_controller, current_path);
			actual_path = path;
			setHasUnsavedChanges(false);
		}
	}
	
	emit actualPathChanged(actual_path);
	activateWindow();
}

void MainWindow::openPathLater(const QString& path)
{
	path_backlog.push_back(path);
	QTimer::singleShot(0, this, SLOT(openPathBacklog()));
}

void MainWindow::openPathBacklog()
{
	for (auto&& path : path_backlog)
		openPath(path);
	path_backlog.clear();
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction*>(sender());
	if (action)
		openPath(action->data().toString());
}

void MainWindow::updateRecentFileActions()
{
	if (! create_menu)
		return;
	
	QStringList files = Settings::getInstance().getSettingCached(Settings::General_RecentFilesList).toStringList();
	
	int num_recent_files = qMin(files.size(), (int)max_recent_files);
	
	open_recent_menu->clear();
	for (int i = 0; i < num_recent_files; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		recent_file_act[i]->setText(text);
		recent_file_act[i]->setData(files[i]);
		open_recent_menu->addAction(recent_file_act[i]);
	}
	
	if (num_recent_files > 0 && !open_recent_menu_inserted)
		file_menu->insertMenu(save_act, open_recent_menu);
	else if (!(num_recent_files > 0) && open_recent_menu_inserted)
		file_menu->removeAction(open_recent_menu->menuAction());
	open_recent_menu_inserted = num_recent_files > 0;
}

void MainWindow::setHasAutosaveConflict(bool value)
{
	if (has_autosave_conflict != value)
	{
		has_autosave_conflict = value;
		setAutosaveNeeded(has_unsaved_changes && !has_autosave_conflict);
		if (!has_autosave_conflict)
			emit autosaveConflictResolved();
	}
}

bool MainWindow::removeAutosaveFile() const
{
	if (!currentPath().isEmpty() && !has_autosave_conflict)
	{
		QFile autosave_file(autosavePath(currentPath()));
		return !autosave_file.exists() || autosave_file.remove();
	}
	return false;
}

Autosave::AutosaveResult MainWindow::autosave()
{
	QString path = currentPath();
	if (path.isEmpty() || !controller)
	{
		return Autosave::PermanentFailure;
	}
	else if (controller->isEditingInProgress())
	{
		return Autosave::TemporaryFailure;
	}
	else
	{
		showStatusBarMessage(tr("Autosaving..."), 0);
		if (controller->exportTo(autosavePath(currentPath())))
		{
			// Success
			clearStatusBarMessage();
			return Autosave::Success;
		}
		else
		{
			// Failure
			showStatusBarMessage(tr("Autosaving failed!"), 6000);
			return Autosave::PermanentFailure;
		}
	}
}

bool MainWindow::save()
{
	return savePath(currentPath());
}

bool MainWindow::savePath(const QString &path)
{
	if (!controller)
		return false;
	
	if (path.isEmpty())
		return showSaveAsDialog();
	
	const FileFormat *format = FileFormats.findFormatForFilename(path);
	if (format->isExportLossy())
	{
		QString message = tr("This map is being saved as a \"%1\" file. Information may be lost.\n\nPress Yes to save in this format.\nPress No to choose a different format.").arg(format->description());
		int result = QMessageBox::warning(this, tr("Warning"), message, QMessageBox::Yes, QMessageBox::No);
		if (result != QMessageBox::Yes)
			return showSaveAsDialog();
	}
	
	if (!controller->save(path))
		return false;
	
	setMostRecentlyUsedFile(path);
	
	setHasAutosaveConflict(false);
	removeAutosaveFile();
	
	if (path != current_path)
	{
		setCurrentPath(path);
		removeAutosaveFile();
	}
	
	setHasUnsavedChanges(false);
	
	return true;
}

QString MainWindow::getOpenFileName(QWidget* parent, const QString& title, FileFormat::FileTypes types)
{
	// Get the saved directory to start in, defaulting to the user's home directory.
	QSettings settings;
	QString open_directory = settings.value("openFileDirectory", QDir::homePath()).toString();
	
	// Build the list of supported file filters based on the file format registry
	QString filters, extensions;
	
	if (types.testFlag(FileFormat::MapFile))
	{
		for (auto format : FileFormats.formats())
		{
			if (format->supportsImport())
			{
				if (filters.isEmpty())
				{
					filters    = format->filter();
					extensions = "*." % format->fileExtensions().join(" *.");
				}
				else
				{
					filters    = filters    % ";;"  % format->filter();
					extensions = extensions % " *." % format->fileExtensions().join(" *.");
				}
			}
		}
		filters = 
			tr("All maps")  % " (" % extensions % ");;" %
			filters         % ";;";
	}
	
	filters += tr("All files") % " (*.*)";
	
	QString path = QFileDialog::getOpenFileName(parent, title, open_directory, filters);
	QFileInfo info(path);
	return info.canonicalFilePath();
}

bool MainWindow::showSaveAsDialog()
{
	if (!controller)
		return false;
	
	// Try current directory first
	QFileInfo current(currentPath());
	QString save_directory = current.canonicalPath();
	if (save_directory.isEmpty())
	{
		// revert to least recently used directory or home directory.
		QSettings settings;
		save_directory = settings.value("openFileDirectory", QDir::homePath()).toString();
	}
	
	// Build the list of supported file filters based on the file format registry
	QString filters;
	for (auto format : FileFormats.formats())
	{
		if (format->supportsExport())
		{
			if (filters.isEmpty()) 
				filters = format->filter();
			else
				filters = filters % ";;" % format->filter();
		}
	}
	
	QString filter = NULL; // will be set to the selected filter by QFileDialog
	QString path = QFileDialog::getSaveFileName(this, tr("Save file"), save_directory, filters, &filter);
	
	// On Windows, when the user enters "sample", we get "sample.omap *.xmap".
	// (Fixed in upstream qtbase/src/plugins/platforms/windows/qwindowsdialoghelpers.cpp
	// Wednesday March 20 2013 in commit 426f2cc.)
	// This results in an error later, because "*" is not a valid character.
	// But it is reasonable to apply the workaround to all platforms, 
	// due to the special meaning of "*" in shell patterns.
	const int extensions_quirk = path.indexOf(" *.");
	if (extensions_quirk >= 0)
	{
		path.truncate(extensions_quirk);
	}
	
	if (path.isEmpty())
		return false;
	
	const FileFormat *format = FileFormats.findFormatByFilter(filter);
	if (NULL == format)
	{
		QMessageBox::information(this, tr("Error"), 
		  tr("File could not be saved:") % "\n" %
		  tr("There was a problem in determining the file format.") % "\n\n" %
		  tr("Please report this as a bug.") );
		return false;
	}
	
	// Ensure that the provided filename has a correct file extension.
	// Among other things, this will ensure that FileFormats.formatForFilename()
	// returns the same thing the user selected in the dialog.
// 	QString selected_extension = "." % format->primaryExtension();
	QStringList selected_extensions(format->fileExtensions());
	selected_extensions.replaceInStrings(QRegExp("^"), ".");
	bool has_extension = false;
	for (auto selected_extension : selected_extensions)
	{
		if (path.endsWith(selected_extension, Qt::CaseInsensitive))
		{
			has_extension = true;
			break;
		}
	}
	if (!has_extension)
		path.append(".").append(format->primaryExtension());
	// Ensure that the file name matches the format.
	Q_ASSERT(format->fileExtensions().contains(QFileInfo(path).suffix()));
	// Fails when using different formats for import and export:
	//	Q_ASSERT(FileFormats.findFormatForFilename(path) == format);
	
	return savePath(path);
}

void MainWindow::toggleFullscreenMode()
{
	if (isFullScreen())
	{
		if (maximized_before_fullscreen)
		{
			showNormal();
			showMaximized();
		}
		else
			showNormal();
	}
	else
	{
		maximized_before_fullscreen = isMaximized();
		showFullScreen();
	}
}

void MainWindow::showSettings()
{
	SettingsDialog dialog(this);
	dialog.exec();
}

void MainWindow::showAbout()
{
	AboutDialog about_dialog(this);
	about_dialog.exec();
}

void MainWindow::showHelp()
{
#ifdef Q_OS_ANDROID
	const QString manual_path = MapperResource::locate(MapperResource::MANUAL, "index.html");
	const QUrl help_url = QUrl::fromLocalFile(manual_path);
	TextBrowserDialog help_dialog(help_url, this);
	help_dialog.exec();
#else
	Util::showHelp(this);
#endif
}

void MainWindow::linkClicked(const QString &link)
{
	if (link.compare("settings:", Qt::CaseInsensitive) == 0)
		showSettings();
	else if (link.compare("help:", Qt::CaseInsensitive) == 0)
		showHelp();
	else if (link.compare("about:", Qt::CaseInsensitive) == 0)
		showAbout();
	else if (link.startsWith("examples:", Qt::CaseInsensitive))
	{
		auto example = link.midRef(9);
		openPathLater(MapperResource::locate(MapperResource::EXAMPLE) % '/' % example);
	}
	else
		QDesktopServices::openUrl(link);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	Q_UNUSED(object)
	
	switch(event->type())
	{
	case QEvent::WhatsThisClicked:
		{
			QWhatsThisClickedEvent* e = static_cast<QWhatsThisClickedEvent*>(event);
			QStringList parts = e->href().split("#");
			if(parts.size() == 0)
				Util::showHelp(this);
			else if(parts.size() == 1)
				Util::showHelp(this, parts.at(0));
			else if(parts.size() == 2)
				Util::showHelp(this, parts.at(0), parts.at(1));
		};
		break;
#if defined(Q_OS_ANDROID)
	case QEvent::KeyRelease:
		if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Back && hasOpenedFile())
		{
			/* Don't let Qt close the application in
			 * QGuiApplicationPrivate::processKeyEvent() while a file is opened.
			 * 
			 * This must be the application-wide event filter in order to
			 * catch Qt::Key_Back from popup menus (such as template list,
			 * overflow actions).
			 * 
			 * Any widgets that want to handle Qt::Key_Back need to watch
			 * for QEvent::KeyPress.
			 */
			event->accept();
			return true;
		}
		break;
#endif
	default:
		; // nothing
	}
	
	return false;
}
