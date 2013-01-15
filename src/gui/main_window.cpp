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


#include "main_window.h"

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QSettings>

#include <proj_api.h>
#include <fcntl.h>

#include <mapper_config.h>

#include "home_screen_controller.h"
#include "../map.h"
#include "../map_dialog_new.h"
#include "../map_editor.h"
#include "../mapper_resource.h"
#include "../file_format.h"
#include "../settings.h"
#include "../settings_dialog.h"


int MainWindow::num_open_files = 0;

MainWindow::MainWindow(bool as_main_window)
{
	controller = NULL;
	has_unsaved_changes = false;
	has_opened_file = false;
	show_menu = as_main_window;
	disable_shortcuts = false;
	setCurrentFile("");
	maximized_before_fullscreen = false;
	general_toolbar = NULL;
	
	setWindowIcon(QIcon(":/images/mapper.png"));
	setAttribute(Qt::WA_DeleteOnClose);
	
	status_label = new QLabel();
	statusBar()->addWidget(status_label, 1);
	statusBar()->setSizeGripEnabled(as_main_window);
	
	central_widget = new QStackedWidget(this);
	QMainWindow::setCentralWidget(central_widget);
	
	if (as_main_window)
		loadWindowSettings();
	
	installEventFilter(this);
	
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(updateRecentFileActions()));
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

const QString& MainWindow::appName() const
{
	static QString app_name(APP_NAME);
	return app_name;
}

void MainWindow::setCentralWidget(QWidget* widget)
{
	if (widget != NULL)
	{
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

void MainWindow::setController(MainWindowController* new_controller)
{
	if (controller)
	{
		controller->detach();
		delete controller;
		controller = NULL;
		
		// Just to make sure ...
		menuBar()->clear();
		delete general_toolbar;
		general_toolbar = NULL;
	}
	
	has_opened_file = false;
	has_unsaved_changes = false;
	disable_shortcuts = false;
	setCurrentFile("");
	
	if (show_menu)
		createFileMenu();
	
	controller = new_controller;
	controller->attach(this);
	
	if (show_menu)
		createHelpMenu();
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
	connect(settings_act, SIGNAL(triggered()), this, SLOT(showSettings()));
	
	close_act = new QAction(tr("Close"), this);
	close_act->setShortcut(QKeySequence::Close);
	close_act->setStatusTip(tr("Close this file"));
	close_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(close_act, SIGNAL(triggered()), this, SLOT(closeFile()));
	
	QAction* exit_act = new QAction(tr("E&xit"), this);
	exit_act->setShortcuts(QKeySequence::Quit);
	exit_act->setStatusTip(tr("Exit the application"));
	close_act->setWhatsThis("<a href=\"file_menu.html\">See more</a>");
	connect(exit_act, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	
	file_menu = menuBar()->addMenu(tr("&File"));
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
	connect(manualAct, SIGNAL(triggered()), this, SLOT(showHelp()));
	
	QAction* aboutAct = new QAction(tr("&About %1").arg(APP_NAME), this);
	aboutAct->setStatusTip(tr("Show information about this application"));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(showAbout()));
	
	QAction* aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setStatusTip(tr("Show information about Qt"));
	connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	
	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(manualAct);
	helpMenu->addAction(QWhatsThis::createAction(this));
	helpMenu->addSeparator();
	helpMenu->addAction(aboutAct);
	helpMenu->addAction(aboutQtAct);
}

void MainWindow::setCurrentFile(const QString& path)
{
	QFileInfo info(path);
	current_path = info.canonicalFilePath();
	updateWindowTitle();
	
	if (current_path.isEmpty())
		return;
	
	Settings& settings = Settings::getInstance();
	
	// Update least recently used directory
	QString open_directory = info.canonicalPath();
	QSettings().setValue("openFileDirectory", open_directory);
	
	// Update recent file lists
	QStringList files = settings.getSettingCached(Settings::General_RecentFilesList).toStringList();
	files.removeAll(current_path);
	files.prepend(current_path);
	while (files.size() > max_recent_files)
		files.removeLast();
	settings.setSetting(Settings::General_RecentFilesList, files);
}

void MainWindow::setHasOpenedFile(bool value)
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
	has_opened_file = value;
	updateWindowTitle();
}

void MainWindow::setHasUnsavedChanges(bool value)
{
	has_unsaved_changes = value;
	updateWindowTitle();
}

void MainWindow::setStatusBarText(const QString& text)
{
	status_label->setText(text);
	status_label->setToolTip(text);
}

void MainWindow::closeFile()
{
	if (num_open_files > 1)
		close();
	else if (showSaveOnCloseDialog())
	{
		num_open_files--;
		setController(new HomeScreenController());
	}
}

bool MainWindow::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && disable_shortcuts)
		event->accept();
	
	return QMainWindow::event(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	if (showSaveOnCloseDialog())
	{
		if (has_opened_file)
		{
			saveWindowSettings();
			num_open_files--;
		}
		event->accept();
	}
	else
		event->ignore();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if (controller)
		controller->keyPressEvent(event);
	if (!event->isAccepted())
	{
		emit(keyPressed(event));
		QWidget::keyPressEvent(event);
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
	if (controller)
		controller->keyReleaseEvent(event);
	if (!event->isAccepted())
	{
		emit(keyReleased(event));
		QWidget::keyReleaseEvent(event);
	}
}

bool MainWindow::showSaveOnCloseDialog()
{
	if (has_opened_file && has_unsaved_changes)
	{
		// Show the window in case it is minimized
		setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		raise();
		activateWindow();
		
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, APP_NAME,
								   tr("The file has been modified.\n"
		                           "Do you want to save your changes?"),
								   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		
		if (ret == QMessageBox::Save)
			return save();
		else if (ret == QMessageBox::Cancel)
			return false;
	}
	
	return true;
}

void MainWindow::saveWindowSettings()
{
	QSettings settings;
	
	settings.beginGroup("MainWindow");
	settings.setValue("pos", pos());
	settings.setValue("size", size());
	settings.setValue("maximized", isMaximized());
	settings.endGroup();
}

void MainWindow::loadWindowSettings()
{
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
}

MainWindow* MainWindow::findMainWindow(const QString& file_name)
{
	QString canonical_file_path = QFileInfo(file_name).canonicalFilePath();
	if (canonical_file_path.isEmpty())
		return NULL;
	
	foreach (QWidget *widget, qApp->topLevelWidgets())
	{
		MainWindow* other = qobject_cast<MainWindow*>(widget);
		if (other && other->getCurrentFilePath() == canonical_file_path)
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
		if (current_path.isEmpty())
			window_title += tr("Unsaved file") + " - ";
		else
			window_title += QFileInfo(current_path).fileName() + " - ";
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
		new_map->loadFrom(symbol_set_path, NULL, true);
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
}

void MainWindow::showOpenDialog()
{
	QString path = getOpenFileName(this, tr("Open file"), FileType::All);
	
	if (path.isEmpty())
		return;
	
	openPath(path);
}

bool MainWindow::openPath(const QString &path)
{
	// Empty path does nothing. This also helps with the single instance application code.
	if (path.isEmpty()) return true;
	
	// Check a blocker that prevents immediate re-opening of crashing files.
	// Needed for stopping auto-loading a crashing file on startup.
	static const QString reopen_blocker = "open_in_progress";
	QSettings settings;
	QString open_in_progress = settings.value(reopen_blocker, QString("")).toString();
	if (open_in_progress == path)
	{
		QMessageBox::warning(this, tr("Crash warning"), 
		  tr("It seems that %1 crashed the last time this file was opened:<br /><tt>%2</tt><br /><br />Opening aborted.").arg(appName()).arg(path),
		  QMessageBox::Ok);
		settings.remove(reopen_blocker);
		return false;
	}
	
	settings.setValue(reopen_blocker, path);
	settings.sync();
	
	MainWindow* existing = findMainWindow(path);
	if (existing)
	{
		existing->show();
		existing->raise();
		existing->activateWindow();
		return true;
	}
	
	MainWindowController* new_controller = MainWindowController::controllerForFile(path);
	if (!new_controller)
	{
		QMessageBox::warning(this, tr("Error"), tr("Cannot open file:\n%1\n\nFile format not recognized.").arg(path));
		return false;
	}
	if (!new_controller->load(path))
	{
		delete new_controller;
		return false;
	}
	
	MainWindow* open_window;
	if (has_opened_file)
		open_window = new MainWindow(true);
	else
		open_window = this;
	open_window->setController(new_controller);
	open_window->setCurrentFile(path);		// must be after setController()
	open_window->setHasUnsavedChanges(false);
	
	open_window->show();
	open_window->raise();
	open_window->activateWindow();
	num_open_files++;
	settings.remove(reopen_blocker);
	return true;
}

void MainWindow::openPathLater(const QString& path)
{
	path_backlog.push_back(path);
	QTimer::singleShot(0, this, SLOT(openPathBacklog()));
}

void MainWindow::openPathBacklog()
{
	Q_FOREACH(QString path, path_backlog)
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

bool MainWindow::save()
{
	return savePath(current_path);
}

bool MainWindow::savePath(const QString &path)
{
	if (!controller)
		return false;
	
	if (path.isEmpty())
		return showSaveAsDialog();
	
	const Format *format = FileFormats.findFormatForFilename(path);
	if (format->isExportLossy())
	{
		QString message = tr("This map is being saved as a \"%1\" file. Information may be lost.\n\nPress Yes to save in this format.\nPress No to choose a different format.").arg(format->description());
		int result = QMessageBox::warning(this, tr("Warning"), message, QMessageBox::Yes, QMessageBox::No);
		if (result != QMessageBox::Yes)
			return showSaveAsDialog();
	}
	
	if (!controller->save(path))
		return false;
	
	setCurrentFile(path);
	setHasUnsavedChanges(false);
	return true;
}

QString MainWindow::getOpenFileName(QWidget* parent, const QString& title, FileType::Enum types)
{
	// Get the saved directory to start in, defaulting to the user's home directory.
	QSettings settings;
	QString open_directory = settings.value("openFileDirectory", QDir::homePath()).toString();
	
	// Build the list of supported file filters based on the file format registry
	QString filters, extensions;
	
	if (types & FileType::Map)
	{
		Q_FOREACH(const Format *format, FileFormats.formats())
		{
			if (format->supportsImport())
			{
				if (filters.isEmpty())
				{
					filters    = format->filter();
					extensions = "*." % format->fileExtension();
				}
				else
				{
					filters    = filters    % ";;"  % format->filter();
					extensions = extensions % " *." % format->fileExtension();
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
	QFileInfo current(current_path);
	QString save_directory = current.canonicalPath();
	if (save_directory.isEmpty())
	{
		// revert to least recently used directory or home directory.
		QSettings settings;
		save_directory = settings.value("openFileDirectory", QDir::homePath()).toString();
	}
	
	// Build the list of supported file filters based on the file format registry
	QString filters;
	Q_FOREACH(const Format *format, FileFormats.formats())
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
	if (path.isEmpty())
		return false;
	
	const Format *format = FileFormats.findFormatByFilter(filter);
	if (NULL == format)
	{
		QMessageBox::information(this, tr("Error"), 
		  tr("File could not be saved:") % "\n" %
		  tr("There was a problem in determining the file format.") % "\n\n" %
		  tr("Please report this as a bug.") );
		return false;
	}
	
	// Ensure the provided filename has the correct file extension.
	// Among other things, this will ensure that FileFormats.formatForFilename()
	// returns the same thing the user selected in the dialog.
	QString selected_extension = "." % format->fileExtension();
	if (!path.endsWith(selected_extension, Qt::CaseInsensitive))
	{
		path.append(selected_extension);
	}
	Q_ASSERT(FileFormats.findFormatForFilename(path) == format);
	
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
	QDialog about_dialog(this);
	about_dialog.setWindowTitle(tr("About %1").arg(APP_NAME));
	
	QString clipper_about(tr("This program uses the <b>Clipper library</b> by Angus Johnson.") % "<br/><br/>");
	QFile clipper_about_file(":/3rd-party/clipper/License.txt");
	if (clipper_about_file.open(QIODevice::ReadOnly))
	{
		clipper_about.append(clipper_about_file.readAll().replace('\n', "<br/>"));
		clipper_about.append("<br/>");
	}
	clipper_about.append(tr("See <a href=\"%1\">%1</a> for more information.").arg("http://www.angusj.com/delphi/clipper.php"));	
	
	QString proj_about(tr("This program uses the <b>PROJ.4 Cartographic Projections Library</b> by Frank Warmerdam.") % "<br/>");
    proj_about.append(pj_get_release()).append("<br/><br/>");
	QFile proj_about_file(":/3rd-party/proj/COPYING");
	if (proj_about_file.open(QIODevice::ReadOnly))
	{
		proj_about.append(proj_about_file.readAll().replace('\n', "<br/>"));
		proj_about.append("<br/>");
	}
	proj_about.append(tr("See <a href=\"%1\">%1</a> for more information.").arg("http://trac.osgeo.org/proj/"));	
	
	QLabel* about_label = new QLabel(
		QString(
		     "<a href=\"http://openorienteering.org\"><img src=\":/images/open-orienteering.png\"/></a><br/><br/>"
		     "OpenOrienteering Mapper %1<br/>"
		     "Copyright (C) 2012, 2013  Thomas Sch&ouml;ps<br/>"
		     "This program comes with ABSOLUTELY NO WARRANTY;<br/>"
		     "This is free software, and you are welcome to redistribute it<br/>"
		     "under certain conditions; see the file COPYING for details.<br/><br/>").
		   arg(APP_VERSION)
		
		% tr("Developers in alphabetical order:<br/>%1<br/>"
		     "For contributions, thanks to:<br/>%2<br/>"
		     "Additional information:").
		  arg(QString("Peter Curtis<br/>Kai Pastor<br/>Russell Porter<br/>Thomas Sch&ouml;ps %1<br/>").arg(tr("(project leader)"))).
		  arg("Jon Cundill<br/>Jan Dalheimer<br/>Eugeniy Fedirets<br/>Anders Gressli<br/>Peter Hoban<br/>Henrik Johansson<br/>Oskar Karlin<br/>Tojo Masaya<br/>Vincent Poinsignon<br/>Christopher Schive<br/>Aivars Zogla<br/>")
		 );
	QTextEdit* additional_text = new QTextEdit( 
	  clipper_about % "<br/><br/>" %
	  QString("_").repeated(80) % "<br/><br/>" %
	  proj_about 
	);
	additional_text->setReadOnly(true);
	additional_text->setLineWrapMode(QTextEdit::NoWrap);
	QPushButton* about_ok = new QPushButton(tr("OK"));
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(about_label, 0, 0, 1, 2);
	layout->addWidget(additional_text, 1, 0, 1, 2);
	layout->addWidget(about_ok, 2, 1);
	layout->setColumnStretch(0, 1);
	about_dialog.setLayout(layout);
	
	connect(about_ok, SIGNAL(clicked(bool)), &about_dialog, SLOT(accept()));
	connect(about_label, SIGNAL(linkActivated(QString)), this, SLOT(linkClicked(QString)));
	
	about_dialog.setWindowModality(Qt::WindowModal);
	about_dialog.exec();
}


void MainWindow::showHelp(QString filename, QString fragment)
{
	static QProcess assistant_process;
	if (assistant_process.state() == QProcess::Running)
	{
		QString command("setSource " + makeHelpUrl(filename, fragment) + "\n");
		assistant_process.write(command.toLatin1());
	}
	else
	{
		QString manual_path = MapperResource::locate(MapperResource::MANUAL);
		if (manual_path.isEmpty())
		{
			QMessageBox::warning(this, tr("Error"), tr("Failed to locate the help files."));
			return;
		}
		
		QString assistant_path = MapperResource::locate(MapperResource::ASSISTANT);
		if (assistant_path.isEmpty())
		{
			QMessageBox::warning(this, tr("Error"), tr("Failed to locate the help browser (\"Qt Assistant\")."));
			return;
		}
		
		// Try to start the Qt Assistant process
		QStringList args;
		args << QLatin1String("-collectionFile")
			 << QDir::toNativeSeparators(manual_path)
			 << QLatin1String("-showUrl")
			 << makeHelpUrl(filename, fragment)
			 << QLatin1String("-enableRemoteControl");
		
		assistant_process.start(assistant_path, args);
		
		// FIXME: Calling waitForStarted() from the main thread might cause the user interface to freeze.
		if (!assistant_process.waitForStarted())
		{
			QMessageBox msg_box;
			msg_box.setIcon(QMessageBox::Warning);
			msg_box.setWindowTitle(tr("Error"));
			
			QString assistant_install_cmd;
#ifdef MAPPER_DEBIAN_PACKAGE_NAME
			QDir usr_bin("/usr/bin");
			if (!usr_bin.exists("assistant") && usr_bin.exists("software-center"))
				assistant_install_cmd = "/usr/bin/software-center qt4-dev-tools";
#endif
			if (!assistant_install_cmd.isEmpty())
			{
				msg_box.setText(tr("The help browser (\"Qt Assistant\") is not installed.") + "\n" +
				                tr("Do you want to install it now?"));
				msg_box.setStandardButtons(QMessageBox::Cancel);
				msg_box.addButton(tr("Install..."), QMessageBox::ActionRole);
			}
			else
			{
				msg_box.setText(tr("Failed to launch the help browser (\"Qt Assistant\")."));
				msg_box.setStandardButtons(QMessageBox::Ok);
				QString details = assistant_process.readAllStandardError();
				if (! details.isEmpty())
					msg_box.setDetailedText(details);
			}
			
			int result = msg_box.exec();
			if ( result != QMessageBox::Ok && result != QMessageBox::Cancel &&
			     !assistant_install_cmd.isEmpty() )
			{
				QProcess::startDetached(assistant_install_cmd);
			}
		}
	}
}

QString MainWindow::makeHelpUrl(QString filename, QString fragment)
{
	return "qthelp://openorienteering.mapper.help/oohelpdoc/help/html_en/" + filename + (fragment.isEmpty() ? "" : ("#" + fragment));
}

void MainWindow::linkClicked(const QString &link)
{
	if (link.compare("settings:", Qt::CaseInsensitive) == 0)
		showSettings();
	else if (link.compare("help:", Qt::CaseInsensitive) == 0)
		showHelp();
	else if (link.compare("about:", Qt::CaseInsensitive) == 0)
		showAbout();
	else
		QDesktopServices::openUrl(link);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event){
	Q_UNUSED(object)
	if(event->type() == QEvent::WhatsThisClicked){
		QWhatsThisClickedEvent* e = static_cast<QWhatsThisClickedEvent*>(event);
		QStringList parts = e->href().split("#");
		if(parts.size() == 0)
			this->showHelp();
		else if(parts.size() == 1)
			this->showHelp(parts.at(0));
		else if(parts.size() == 2)
			this->showHelp(parts.at(0), parts.at(1));
	}
	return false;
}

void MainWindow::gotUnsavedChanges()
{
	setHasUnsavedChanges(true);
}
