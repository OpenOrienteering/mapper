/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMenuBar>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QWhatsThis>

#if defined(Q_OS_ANDROID)
#  include <QtAndroid>
#  include <QAndroidJniObject>
#  include <QDesktopWidget>
#  include <QTimer>
#  include <QUrl>
#endif

#include <mapper_config.h>

#include "settings.h"
#include "core/map.h"
#include "core/map_view.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_format_registry.h"
#include "fileformats/file_import_export.h"
#include "gui/about_dialog.h"
#include "gui/autosave_dialog.h"
#include "gui/file_dialog.h"
#include "gui/home_screen_controller.h"
#include "gui/settings_dialog.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/new_map_dialog.h"
#include "undo/undo_manager.h"
#include "util/util.h"
#include "util/backports.h"


namespace OpenOrienteering {

constexpr int MainWindow::max_recent_files;

int MainWindow::num_open_files = 0;

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
: MainWindow { true, parent, flags }
{
	// nothing else
}

MainWindow::MainWindow(bool as_main_window, QWidget* parent, Qt::WindowFlags flags)
: QMainWindow           { parent, flags }
, controller            { nullptr }
, create_menu           { as_main_window }
, show_menu             { create_menu && !mobileMode() }
, shortcuts_blocked     { false }
, general_toolbar       { nullptr }
, file_menu             { nullptr }
, has_opened_file       { false }
, has_unsaved_changes   { false }
, has_autosave_conflict { false }
, maximized_before_fullscreen { false }
, homescreen_disabled   { false }
{
	setWindowIcon(QIcon(QString::fromLatin1(":/images/mapper.png")));
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
	
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &MainWindow::settingsChanged);
	connect(qApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::applicationStateChanged);
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



void MainWindow::applicationStateChanged()
{
#ifdef Q_OS_ANDROID
	// The Android app may be started or resumed when the user triggers a suitable "intent".
	if (QGuiApplication::applicationState() == Qt::ApplicationActive)
	{
		auto activity = QtAndroid::androidActivity();
		auto intent_path = activity.callObjectMethod<jstring>("takeIntentPath").toString();
		if (!intent_path.isEmpty())
		{
			const auto local_file = QUrl(intent_path).toLocalFile();
			if (!hasOpenedFile())
			{
				openPathLater(local_file);
			}
			else if (currentPath() != local_file)
			{
				showStatusBarMessage(tr("You must close the current file before you can open another one."));
			}
			return;
		}
	}
#endif
	
	// Only on startup, we may need to load the most recently used file.
	static bool starting_up = true;
	if (starting_up)
	{
		starting_up = false;
		QSettings settings;
		if (path_backlog.isEmpty()
		    && settings.value(QLatin1String("openMRUFile")).toBool())
		{
			const auto files = settings.value(QLatin1String("recentFileList")).toStringList();
			if (!files.isEmpty())
				openPathLater(files[0]);
		}
	}
}



QString MainWindow::appName() const
{
	return APP_NAME;
}

#ifndef Q_OS_ANDROID
bool MainWindow::mobileMode()
{
	static bool mobile_mode = qEnvironmentVariableIsSet("MAPPER_MOBILE_GUI")
	                          ? (qgetenv("MAPPER_MOBILE_GUI") != "0")
	                          : 0;
	return mobile_mode;
}
#endif

void MainWindow::setCentralWidget(QWidget* widget)
{
	if (widget)
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

void MainWindow::setController(MainWindowController* new_controller)
{
	setController(new_controller, false);
	setCurrentFile({}, nullptr);
}

void MainWindow::setController(MainWindowController* new_controller, const QString& path, const FileFormat* format)
{
	setController(new_controller, true);
	setCurrentFile(path, format);
}

void MainWindow::setController(MainWindowController* new_controller, bool has_file)
{
	if (controller)
	{
		controller->detach();
		delete controller;
		controller = nullptr;
		
		if (show_menu)
			menuBar()->clear();
		delete general_toolbar;
		general_toolbar = nullptr;
	}
	
	has_opened_file = has_file;
	shortcuts_blocked = false;
	
	if (create_menu)
		createFileMenu();
	
	controller = new_controller;
	controller->attach(this);
	
	if (create_menu)
		createHelpMenu();
	
#if defined(Q_OS_MACOS)
	if (isVisible() && qApp->activeWindow() == this)
	{
		// Force a menu synchronisation,
		// QCocoaMenuBar::updateMenuBarImmediately(),
		// via QCocoaNativeInterface::onAppFocusWindowChanged().
		/// \todo Review in Qt > 5.6
		qApp->focusWindowChanged(qApp->focusWindow());
	}
	
# if defined(MAPPER_DEVELOPMENT_BUILD)
	{
		// Qt's menu text heuristic can assign unexpected platform specific roles,
		// which resulted in Mapper issue #1067. The only supported solution is
		// assigning QAction::NoRole) before adding items to the menubar.
		// Cf. QTBUG-30812.
		// However, the heuristic is required for some platform-specific items.
		// Cf. detectMenuRole() in qtbase/src/plugins/platforms/cocoa/messages.cpp
		const auto platform_keywords = {
			QCoreApplication::translate("QCocoaMenuItem", "Cut"),
			QCoreApplication::translate("QCocoaMenuItem", "Copy"),
			QCoreApplication::translate("QCocoaMenuItem", "Paste"),
			QCoreApplication::translate("QCocoaMenuItem", "Select All")
		};
		const auto menubar_actions = menuBar()->actions();
		for (const auto* menubar_action : menubar_actions)
		{
			if (const auto* menu = menubar_action->menu())
			{
				const auto menu_actions = menu->actions();
				for (const auto* action : menu_actions)
				{
					if (action->menuRole() != QAction::TextHeuristicRole
						|| action->isSeparator())
						continue;
					const auto text = action->text().remove(QLatin1Char('&'));
					if (std::none_of(begin(platform_keywords), end(platform_keywords), [&text](const auto& keyword) {
									return keyword.compare(text, Qt::CaseInsensitive) == 0;
						}))
					{
						// Such warnings may indiciate missing setting of QAction::NoRole
						// on a (new) item, or incomplete translations for Mapper or Qt.
						qDebug("Unexpected TextHeuristicRole for \"%s > %s\"",
						       qUtf8Printable(menubar_action->text()),
						       qUtf8Printable(action->text()));
					}
				}
			}
		}
	}
# endif  // MAPPER_DEVELOPMENT_BUILD
#endif  // Q_OS_MACOS
	
	setHasAutosaveConflict(false);
	setHasUnsavedChanges(false);
}

void MainWindow::createFileMenu()
{
	QAction* new_act = new QAction(QIcon(QString::fromLatin1(":/images/new.png")), tr("&New"), this);
	new_act->setMenuRole(QAction::NoRole);
	new_act->setShortcuts(QKeySequence::New);
	new_act->setStatusTip(tr("Create a new map"));
	new_act->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	connect(new_act, &QAction::triggered, this, &MainWindow::showNewMapWizard);
	
	QAction* open_act = new QAction(QIcon(QString::fromLatin1(":/images/open.png")), tr("&Open..."), this);
	open_act->setMenuRole(QAction::NoRole);
	open_act->setShortcuts(QKeySequence::Open);
	open_act->setStatusTip(tr("Open an existing file"));
	open_act->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	connect(open_act, &QAction::triggered, this, &MainWindow::showOpenDialog);
	
	open_recent_menu = new QMenu(tr("Open &recent"), this);
	open_recent_menu->menuAction()->setMenuRole(QAction::NoRole);
	open_recent_menu->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	for (auto& action : recent_file_act)
	{
		action = new QAction(this);
		connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
	}
	open_recent_menu_inserted = false;
	
	// NOTE: if you insert something between open_recent_menu and save_act, adjust updateRecentFileActions()!
	
	save_act = new QAction(QIcon(QString::fromLatin1(":/images/save.png")), tr("&Save"), this);
	save_act->setMenuRole(QAction::NoRole);
	save_act->setShortcuts(QKeySequence::Save);
	save_act->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	connect(save_act, &QAction::triggered, this, &MainWindow::save);
	
	auto save_as_act = new QAction(tr("Save &as..."), this);
	save_as_act->setMenuRole(QAction::NoRole);
	if (QKeySequence::keyBindings(QKeySequence::SaveAs).empty())
		save_as_act->setShortcut(tr("Ctrl+Shift+S"));
	else
		save_as_act->setShortcuts(QKeySequence::SaveAs);
	save_as_act->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	connect(save_as_act, &QAction::triggered, this, &MainWindow::showSaveAsDialog);
	
	settings_act = new QAction(tr("Settings..."), this);
	settings_act->setMenuRole(QAction::PreferencesRole);
	settings_act->setShortcut(QKeySequence::Preferences);
	connect(settings_act, &QAction::triggered, this, &MainWindow::showSettings);
	
	close_act = new QAction(QIcon(QString::fromLatin1(":/images/close.png")), tr("Close"), this);
	close_act->setMenuRole(QAction::NoRole);
	close_act->setShortcut(QKeySequence::Close);
	close_act->setStatusTip(tr("Close this file"));
	close_act->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	connect(close_act, &QAction::triggered, this, &MainWindow::closeFile);
	
	QAction* exit_act = new QAction(tr("E&xit"), this);
	exit_act->setMenuRole(QAction::QuitRole);
	exit_act->setShortcuts(QKeySequence::Quit);
	exit_act->setStatusTip(tr("Exit the application"));
	exit_act->setWhatsThis(Util::makeWhatThis("file_menu.html"));
	connect(exit_act, &QAction::triggered, qApp, &QApplication::closeAllWindows);
	
	if (show_menu)
	{
		file_menu = menuBar()->addMenu(tr("&File"));
	}
	else
	{
		delete file_menu;
		file_menu = new QMenu(this);
	}

	file_menu->setWhatsThis(Util::makeWhatThis("file_menu.html"));
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
	general_toolbar->setObjectName(QString::fromLatin1("General toolbar"));
	general_toolbar->addAction(new_act);
	general_toolbar->addAction(open_act);
	general_toolbar->addAction(save_act);
	
	save_act->setEnabled(has_opened_file);
	save_as_act->setEnabled(has_opened_file);
	close_act->setEnabled(has_opened_file);
	updateRecentFileActions();
}

void MainWindow::createHelpMenu()
{
	// Help menu
	QAction* manualAct = new QAction(QIcon(QString::fromLatin1(":/images/help.png")), tr("Open &Manual"), this);
	manualAct->setMenuRole(QAction::NoRole);
	manualAct->setStatusTip(tr("Show the help file for this application"));
	manualAct->setShortcut(QKeySequence::HelpContents);
	connect(manualAct, &QAction::triggered, this, &MainWindow::showHelp);
	
	QAction* aboutAct = new QAction(tr("&About %1").arg(appName()), this);
	aboutAct->setMenuRole(QAction::AboutRole);
	aboutAct->setStatusTip(tr("Show information about this application"));
	connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
	
	QAction* aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setMenuRole(QAction::AboutQtRole);
	aboutQtAct->setStatusTip(tr("Show information about Qt"));
	connect(aboutQtAct, &QAction::triggered, qApp, QApplication::aboutQt);
	
	if (show_menu)
	{
		QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
		helpMenu->addAction(manualAct);
		helpMenu->addAction([this] {
			auto action = QWhatsThis::createAction(this);
			action->setMenuRole(QAction::NoRole);
			return action;
		}());
		helpMenu->addSeparator();
		helpMenu->addAction(aboutAct);
		helpMenu->addAction(aboutQtAct);
	}
}

void MainWindow::setCurrentFile(const QString& path, const FileFormat* format)
{
	Q_ASSERT(has_opened_file || path.isEmpty());
	
	if (path != current_path)
	{
		QString window_file_path;
		current_path.clear();
		current_format = nullptr;
		if (has_opened_file)
		{
			window_file_path = QFileInfo(path).canonicalFilePath();
			if (window_file_path.isEmpty())
			{
				window_file_path = tr("Unsaved file");
			}
			else
			{
				current_path = window_file_path;
				current_format = format;
			}
		}
		setWindowFilePath(window_file_path);
	}
	else if (!windowFilePath().isEmpty() && !has_opened_file)
	{
		setWindowFilePath({});
	}
}

void MainWindow::setMostRecentlyUsedFile(const QString& path)
{
	if (!path.isEmpty())
	{
		Settings& settings = Settings::getInstance();
		
		// Update least recently used directory
		const QString open_directory = QFileInfo(path).canonicalPath();
		QSettings().setValue(QString::fromLatin1("openFileDirectory"), open_directory);
		
		// Update recent file lists
		QStringList files = settings.getSettingCached(Settings::General_RecentFilesList).toStringList();
		files.removeAll(path);
		files.prepend(path);
		if (files.size() > max_recent_files)
			files.erase(files.begin() + max_recent_files, files.end());
		settings.setSetting(Settings::General_RecentFilesList, files);
	}
}

void MainWindow::setHasUnsavedChanges(bool value)
{
	if (hasOpenedFile())
	{
		has_unsaved_changes = value;
		setAutosaveNeeded(has_unsaved_changes && !has_autosave_conflict);
	}
	setWindowModified(has_unsaved_changes);
}

void MainWindow::setStatusBarText(const QString& text)
{
	status_label->setText(text);
	status_label->setToolTip(text);
}

void MainWindow::showStatusBarMessage(const QString& text, int timeout)
{
#if defined(Q_OS_ANDROID)
	QAndroidJniObject java_string = QAndroidJniObject::fromString(text);
	QAndroidJniObject::callStaticMethod<void>(
		"org/openorienteering/mapper/MapperActivity",
		"showToast",
		"(Ljava/lang/String;I)V",
		java_string.object<jstring>(), timeout);
#else
	statusBar()->showMessage(text, timeout);
#endif
}

void MainWindow::clearStatusBarMessage()
{
#if defined(Q_OS_ANDROID)
	QAndroidJniObject::callStaticMethod<void>(
		"org/openorienteering/mapper/MapperActivity",
		"hideToast",
		"()V");
#else
	statusBar()->clearMessage();
#endif
}

void MainWindow::setShortcutsBlocked(bool blocked)
{
	shortcuts_blocked = blocked;
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
	if (event->type() == QEvent::ShortcutOverride && shortcutsBlocked())
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
			ret = QMessageBox::warning(this, appName(),
			                           tr("Do you want to remove the autosaved version?"),
			                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		}
		else
		{
			ret = QMessageBox::warning(this, appName(),
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
			qWarning("Unsupported return value from message box");
			break;
		}
		
	}
	
	return true;
}

void MainWindow::saveWindowSettings()
{
#if !defined(Q_OS_ANDROID)
	QSettings settings;
	
	settings.beginGroup(QString::fromLatin1("MainWindow"));
	settings.setValue(QString::fromLatin1("pos"), pos());
	settings.setValue(QString::fromLatin1("size"), size());
	settings.setValue(QString::fromLatin1("maximized"), isMaximized());
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
	
	settings.beginGroup(QString::fromLatin1("MainWindow"));
	QPoint pos = settings.value(QString::fromLatin1("pos"), QPoint(100, 100)).toPoint();
	QSize size = settings.value(QString::fromLatin1("size"), QSize(800, 600)).toSize();
	bool maximized = settings.value(QString::fromLatin1("maximized"), false).toBool();
	settings.endGroup();
	
	move(pos);
	resize(size);
	if (maximized)
		setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
		               | Qt::WindowMaximized); // Cf. QWidget::showMaximized()
#endif
}

MainWindow* MainWindow::findMainWindow(const QString& file_name)
{
	QString canonical_file_path = QFileInfo(file_name).canonicalFilePath();
	if (canonical_file_path.isEmpty())
		return nullptr;
	
	const auto top_level_widgets = qApp->topLevelWidgets();
	for (auto widget : top_level_widgets)
	{
		MainWindow* other = qobject_cast<MainWindow*>(widget);
		if (other && other->currentPath() == canonical_file_path)
			return other;
	}
	
	return nullptr;
}

void MainWindow::showNewMapWizard()
{
	NewMapDialog newMapDialog(this);
	newMapDialog.setWindowModality(Qt::WindowModal);
	newMapDialog.exec();
	
	if (newMapDialog.result() == QDialog::Rejected)
		return;
	
	Map* new_map = new Map();
	MapView tmp_view { nullptr, new_map };
	QString symbol_set_path = newMapDialog.getSelectedSymbolSetPath();
	if (symbol_set_path.isEmpty())
	{
		new_map->setScaleDenominator(newMapDialog.getSelectedScale());
	}
	else if (auto importer = FileFormats.makeImporter(symbol_set_path, *new_map, nullptr))
	{
		importer->setLoadSymbolsOnly(true);
		if (!importer->doImport())
		{
			QMessageBox::warning(this, tr("Error"),
			                     tr("Cannot open file:\n%1\n\n%2").
			                     arg(symbol_set_path, importer->warnings().back()));
			delete new_map;
			return;
		}
		if (!importer->warnings().empty())
			showMessageBox(this, tr("Warning"), tr("The symbol set import generated warnings."), importer->warnings());
		
		if (new_map->getScaleDenominator() != newMapDialog.getSelectedScale())
		{
			if (QMessageBox::question(this, tr("Warning"), tr("The selected map scale is 1:%1, but the chosen symbol set has a nominal scale of 1:%2.\n\nDo you want to scale the symbols to the selected scale?").arg(newMapDialog.getSelectedScale()).arg(new_map->getScaleDenominator()),  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				double factor = double(new_map->getScaleDenominator()) / newMapDialog.getSelectedScale();
				new_map->scaleAllSymbols(factor);
			}
			
			new_map->setScaleDenominator(newMapDialog.getSelectedScale());
		}
		
		for (int i = new_map->getNumSymbols(); i > 0; i = qMin(i, new_map->getNumSymbols()))
		{
			--i;
			auto symbol = new_map->getSymbol(i);
			if (symbol->isHidden()
			    && !new_map->existsObjectWithSymbol(symbol))
			{
				new_map->deleteSymbol(i);
			}
		}
	}
	else
	{
		;  /// \todo error message, cleanup
	}
	
	auto map_view = new MapView { new_map };
	map_view->setGridVisible(tmp_view.isGridVisible());
	
	new_map->setHasUnsavedChanges(false);
	new_map->undoManager().clear();
	
	MainWindow* new_window = hasOpenedFile() ? new MainWindow() : this;
	new_window->setWindowFilePath(tr("Unsaved file"));
	new_window->setController(new MapEditorController(MapEditorController::MapEditor, new_map, map_view), QString(), nullptr);
	
	new_window->show();
	new_window->raise();
	new_window->activateWindow();
	num_open_files++;
}

void MainWindow::showOpenDialog()
{
	if (auto selected = getOpenFileName(this, tr("Open file"), FileFormat::AllFiles))
	{
		openPath(selected.filePath(), selected.fileFormat());
	}
}

bool MainWindow::openPath(const QString &path)
{
	auto format = FileFormats.findFormatForFilename(path, &FileFormat::supportsFileOpen);
	if (!format)
		format = FileFormats.findFormatForData(path, FileFormat::AllFiles);
	return openPath(path, format);
}

bool MainWindow::openPath(const QString& path, const FileFormat* format)
{
	// Empty path does nothing. This also helps with the single instance application code.
	if (path.isEmpty())
		return true;
	
#ifdef Q_OS_ANDROID
	showStatusBarMessage(tr("Opening %1").arg(QFileInfo(path).fileName()));
#else
	MainWindow* const existing = findMainWindow(path);
	if (existing)
	{
		existing->show();
		existing->raise();
		existing->activateWindow();
		return true;
	}
#endif
	
	if (!format || !format->supportsReading())
		QMessageBox::warning(this, tr("Error"),
		                     tr("Cannot open file:\n%1\n\n%2").
		                     arg(path, tr("Invalid file type.")));
	
	// Check a blocker that prevents immediate re-opening of crashing files.
	// Needed for stopping auto-loading a crashing file on startup.
	static const QString reopen_blocker = QString::fromLatin1("open_in_progress");
	QSettings settings;
	const QString open_in_progress(settings.value(reopen_blocker).toString());
	if (open_in_progress == path)
	{
		int result = QMessageBox::warning(this, tr("Crash warning"), 
		  tr("It seems that %1 crashed the last time this file was opened:<br />"
		     "<tt>%2</tt><br /><br />"
		     "Really retry to open it?")
		  .arg(appName(), path),
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
	const FileFormat* new_actual_format = format;
	QString autosave_path = Autosave::autosavePath(path);
	bool new_autosave_conflict = QFileInfo::exists(autosave_path);
	if (new_autosave_conflict)
	{
#if defined(Q_OS_ANDROID)
		// Assuming small screen, showing dialog before opening the file
		AutosaveDialog* autosave_dialog = new AutosaveDialog(path, autosave_path, autosave_path, this);
		int result = autosave_dialog->exec();
		new_actual_path = (result == QDialog::Accepted) ? autosave_dialog->selectedPath() : QString();
		new_actual_format = (new_actual_path == path) ? format : FileFormats.findFormat(FileFormats.defaultFormat());
		delete autosave_dialog;
#else
		// Assuming large screen, dialog will be shown while the autosaved file is open
		new_actual_path = autosave_path;
		new_actual_format = FileFormats.findFormat(FileFormats.defaultFormat());
#endif
	}
	
	if (new_actual_path.isEmpty() || !new_controller->loadFrom(new_actual_path, *format, this))
	{
		delete new_controller;
		settings.remove(reopen_blocker);
		return false;
	}
	
	MainWindow* open_window = this;
#if !defined(Q_OS_ANDROID)
	if (has_opened_file)
		open_window = new MainWindow();
#endif
	
	open_window->setController(new_controller, path, format);
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
		auto autosave_dialog = new AutosaveDialog(path, autosave_path, new_actual_path, open_window, Qt::WindowTitleHint | Qt::CustomizeWindowHint);
		autosave_dialog->move(open_window->rect().right() - autosave_dialog->width(), open_window->rect().top());
		autosave_dialog->show();
		autosave_dialog->raise();
		
		connect(autosave_dialog, &AutosaveDialog::pathSelected, open_window, &MainWindow::switchActualPath);
		connect(open_window, &MainWindow::actualPathChanged, autosave_dialog, &AutosaveDialog::setSelectedPath);
		connect(open_window, &MainWindow::autosaveConflictResolved, autosave_dialog, &AutosaveDialog::autosaveConflictResolved);
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
		ret = QMessageBox::warning(this, appName(),
		                           tr("The file has been modified.\n"
		                              "Do you want to discard your changes?"),
		                           QMessageBox::Discard | QMessageBox::Cancel);
	}
	
	if (ret != QMessageBox::Cancel)
	{
		const QString& current_path = currentPath();
		MainWindowController* const new_controller = MainWindowController::controllerForFile(current_path);
		auto format = (path == current_path) ? current_format : FileFormats.findFormat(FileFormats.defaultFormat());
		if (new_controller && new_controller->loadFrom(path, *format, this))
		{
			setController(new_controller, current_path, format);
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
	QTimer::singleShot(10, this, SLOT(openPathBacklog()));  // clazy:exclude=old-style-connect
}

void MainWindow::openPathBacklog()
{
	for (const auto& path : qAsConst(path_backlog))
		openPath(path);
	path_backlog.clear();
}

void MainWindow::openRecentFile()
{
	if (auto action = qobject_cast<QAction*>(sender()))
		openPath(action->data().toString());
}

void MainWindow::updateRecentFileActions()
{
	if (! create_menu)
		return;
	
	QStringList files = Settings::getInstance().getSettingCached(Settings::General_RecentFilesList).toStringList();
	
	int num_recent_files = qMin(files.size(), max_recent_files);
	
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
	auto autosave_format = FileFormats.findFormat(FileFormats.defaultFormat());
	if (path.isEmpty() || !controller || !autosave_format)
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
		if (controller->exportTo(autosavePath(currentPath()), *autosave_format))
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
	auto path = currentPath();
	auto format = currentFormat();
	if (path.isEmpty()
	    || !format
	    || !format->supportsFileSave())
	{
		return showSaveAsDialog();
	}
	
	return saveTo(path, *currentFormat());
}

bool MainWindow::saveTo(const QString &path, const FileFormat& format)
{
	if (!controller || path.isEmpty())
	{
		qWarning("Unexpected call to MainWindow::saveTo(PATH, FORMAT)");
		return false;
	}

	if (format.isWritingLossy())
	{
		auto message = 
		        tr("This map is being saved as a \"%1\" file. "
		           "Information may be lost.\n\n"
		           "Press Yes to save in this format.\n"
		           "Press No to choose a different format.")
		        .arg(format.description());
		int result = QMessageBox::warning(this, tr("Warning"), message, QMessageBox::Yes, QMessageBox::No);
		if (result != QMessageBox::Yes)
			return showSaveAsDialog();
	}
	
	if (!controller->saveTo(path, format))
		return false;
	
	setMostRecentlyUsedFile(path);
	
	setHasAutosaveConflict(false);
	removeAutosaveFile();
	
	if (path != currentPath())
	{
		setCurrentFile(path, &format);
		removeAutosaveFile();
	}
	
	setHasUnsavedChanges(false);
	
	return true;
}

// static
MainWindow::FileInfo MainWindow::getOpenFileName(QWidget* parent, const QString& title, FileFormat::FileTypes types)
{
	// Get the saved directory to start in, defaulting to the user's home directory.
	QSettings settings;
	QString open_directory = settings.value(QString::fromLatin1("openFileDirectory"), QDir::homePath()).toString();
	
	// Build the list of supported file filters based on the file format registry
	QString filters, extensions;
	
	if (types.testFlag(FileFormat::MapFile) || types.testFlag(FileFormat::OgrFile))
	{
		for (auto format : FileFormats.formats())
		{
			if (format->supportsFileOpen())
			{
				if (filters.isEmpty())
				{
					filters    = format->filter();
					extensions = QLatin1String("*.") + format->fileExtensions().join(QString::fromLatin1(" *."));
				}
				else
				{
					filters    = filters    + QLatin1String(";;")  + format->filter();
					extensions = extensions + QLatin1String(" *.") + format->fileExtensions().join(QString::fromLatin1(" *."));
				}
			}
		}
		filters = 
			tr("All maps")  + QLatin1String(" (") + extensions + QLatin1String(");;") +
			filters         + QLatin1String(";;");
	}
	
	filters += tr("All files") + QLatin1String(" (*.*)");
	
	QString filter; // will be set to the selected filter by QFileDialog
	QString path = FileDialog::getOpenFileName(parent, title, open_directory, filters, &filter);
	
	const FileFormat* format = nullptr;
	if (!path.isEmpty())
	{
		path = QFileInfo(path).canonicalFilePath();
		format = FileFormats.findFormatByFilter(filter, &FileFormat::supportsFileOpen);
		if (!format)
			format = FileFormats.findFormatForFilename(path, &FileFormat::supportsFileOpen);
		if (!format)
			format = FileFormats.findFormatForData(path, types);
	}
	return { path, format };
}



// static
void MainWindow::showMessageBox(QWidget* parent, const QString& title, const QString& headline, const std::vector<QString>& messages)
{
	QString document;
	if (!headline.isEmpty())
		document += QLatin1String("<p><b>") + headline + QLatin1String("</b></p>");
	for (const auto& message : messages)
		document += Qt::convertFromPlainText(message, Qt::WhiteSpaceNormal);
	
	TextBrowserDialog dialog(document, parent);
	dialog.setWindowTitle(title);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
	// Let Android update the screen.
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
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
		save_directory = settings.value(QString::fromLatin1("openFileDirectory"), QDir::homePath()).toString();
	}
	
	// Build the list of supported file filters based on the file format registry
	QString filters;
	for (auto format : FileFormats.formats())
	{
		if (format->supportsFileSaveAs())
		{
			if (filters.isEmpty()) 
				filters = format->filter();
			else
				filters = filters + QLatin1String(";;") + format->filter();
		}
	}
	
	QString filter; // will be set to the selected filter by QFileDialog
	QString path = FileDialog::getSaveFileName(this, tr("Save file"), save_directory, filters, &filter);
	
	// On Windows, when the user enters "sample", we get "sample.omap *.xmap".
	// (Fixed in upstream qtbase/src/plugins/platforms/windows/qwindowsdialoghelpers.cpp
	// Wednesday March 20 2013 in commit 426f2cc.)
	// This results in an error later, because "*" is not a valid character.
	// But it is reasonable to apply the workaround to all platforms, 
	// due to the special meaning of "*" in shell patterns.
	const int extensions_quirk = path.indexOf(QLatin1String(" *."));
	if (extensions_quirk >= 0)
	{
		path.truncate(extensions_quirk);
	}
	
	if (path.isEmpty())
		return false;
	
	const FileFormat *format = FileFormats.findFormatByFilter(filter, &FileFormat::supportsFileSaveAs);
	if (!format)
	{
		QMessageBox::information(this, tr("Error"), 
		  tr("File could not be saved:") + QLatin1Char('\n') +
		  tr("There was a problem in determining the file format.") + QLatin1Char('\n') + QLatin1Char('\n') +
		  tr("Please report this as a bug.") );
		return false;
	}
	
	// Ensure that the provided filename has a correct file extension.
	// Among other things, this will ensure that FileFormats.formatForFilename()
	// returns the same thing the user selected in the dialog.
// 	QString selected_extension = "." + format->primaryExtension();
	QStringList selected_extensions(format->fileExtensions());
	selected_extensions.replaceInStrings(QRegExp(QString::fromLatin1("^")), QString::fromLatin1("."));
	bool has_extension = std::any_of(selected_extensions.constBegin(), selected_extensions.constEnd(), [&path](const auto& selected_extension) {
		return path.endsWith(selected_extension, Qt::CaseInsensitive);
	});
	if (!has_extension)
		path += QLatin1Char('.') + format->primaryExtension();
	// Ensure that the file name matches the format.
	Q_ASSERT(format->fileExtensions().contains(QFileInfo(path).suffix()));
	// Fails when using different formats for import and export:
	//	Q_ASSERT(FileFormats.findFormatForFilename(path, ***) == format);
	
	return saveTo(path, *format);
}

void MainWindow::toggleFullscreenMode()
{
	if (isFullScreen())
	{
		showNormal();
		if (maximized_before_fullscreen)
			showMaximized();
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
	Util::showHelp(this);
}

void MainWindow::linkClicked(const QString &link)
{
	if (link.compare(QLatin1String("settings:"), Qt::CaseInsensitive) == 0)
		showSettings();
	else if (link.compare(QLatin1String("help:"), Qt::CaseInsensitive) == 0)
		showHelp();
	else if (link.compare(QLatin1String("about:"), Qt::CaseInsensitive) == 0)
		showAbout();
	else if (link.startsWith(QLatin1String("examples:"), Qt::CaseInsensitive))
		openPathLater(QLatin1String("data:/examples/") + link.midRef(9));
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
			Util::showHelp(this, e->href());
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


}  // namespace OpenOrienteering
