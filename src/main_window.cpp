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


#include "main_window.h"

#include <assert.h>

#include <QtGui>

#include "main_window_home_screen.h"
#include "global.h"
#include "map.h"
#include "map_dialog_new.h"
#include "map_editor.h"

// ### MainWindowController ###

MainWindowController* MainWindowController::controllerForFile(const QString& filename)
{
	// Check file extension and return fitting controller
	if (filename.endsWith(".omap", Qt::CaseInsensitive) || filename.endsWith(".ocd", Qt::CaseInsensitive))
		return new MapEditorController(MapEditorController::MapEditor);
	
	return NULL;
}

// ### MainWindow ###

MainWindow::MainWindow(bool as_main_window)
{
	controller = NULL;
	this->show_menu = as_main_window;
	setCurrentFile("");
	
	setWindowIcon(QIcon("images/control.png"));
	setAttribute(Qt::WA_DeleteOnClose);
	
	statusBar()->show();
	status_label = new QLabel();
	statusBar()->addWidget(status_label, 1);
	statusBar()->setSizeGripEnabled(as_main_window);
	
	loadWindowSettings();
}
MainWindow::~MainWindow()
{
	if (controller)
	{
		controller->detach();
		delete controller;
	}
}

void MainWindow::setController(MainWindowController* new_controller)
{
	if (controller)
	{
		controller->detach();
		delete controller;
		
		// Just to make sure ...
		menuBar()->clear();
	}
	
	has_opened_file = false;
	has_unsaved_changes = false;
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
	QAction* newAct = new QAction(QIcon("images/new.png"), tr("&New"), this);
	newAct->setShortcuts(QKeySequence::New);
	newAct->setStatusTip(tr("Create a new map"));
	connect(newAct, SIGNAL(triggered()), this, SLOT(showNewMapWizard()));
	
	QAction* openAct = new QAction(QIcon("images/open.png"), tr("&Open..."), this);
	openAct->setShortcuts(QKeySequence::Open);
	openAct->setStatusTip(tr("Open an existing file"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(showOpenDialog()));
	
	// TODO: importAct? Or better in the map menu?
	
	saveAct = new QAction(QIcon("images/save.png"), tr("&Save..."), this);
	saveAct->setShortcuts(QKeySequence::Save);
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));
	
	saveAsAct = new QAction(tr("Save &as..."), this);
	saveAsAct->setShortcuts(QKeySequence::SaveAs);
	connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
	
	QAction* closeAct = new QAction(tr("Close"), this);
	closeAct->setShortcut(tr("Ctrl+W"));
	closeAct->setStatusTip(tr("Close this window"));
	connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));
	
	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(newAct);
	fileMenu->addAction(openAct);
	fileMenu->addAction(saveAct);
	fileMenu->addAction(saveAsAct);
	fileMenu->addSeparator();
	fileMenu->addAction(closeAct);
	
	saveAct->setVisible(false);
	saveAsAct->setVisible(false);
}
void MainWindow::createHelpMenu()
{
	// Help menu
	QAction* manualAct = new QAction(QIcon("images/help.png"), tr("Open &Manual"), this);
	manualAct->setStatusTip(tr("Show the help file for this application"));
	connect(manualAct, SIGNAL(triggered()), this, SLOT(showHelp()));
	
	QAction* aboutAct = new QAction(tr("&About"), this);
	aboutAct->setStatusTip(tr("Show information about this application"));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(showAbout()));
	
	QAction* aboutQtAct = new QAction(tr("About &Qt"), this);
	aboutQtAct->setStatusTip(tr("Show information about QT"));
	connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	
	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(manualAct);
	helpMenu->addSeparator();
	helpMenu->addAction(aboutAct);
	helpMenu->addAction(aboutQtAct);
}

void MainWindow::setCurrentFile(const QString& path)
{
	current_path = path;
	updateWindowTitle();
	
	if (path.isEmpty())
		return;
	
	// Update recent file lists - TODO
	/*
	# define MAX_RECENT_FILES 10
	
	QSettings settings;
	
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(path);
	files.prepend(path);
	while (files.size() > MAX_RECENT_FILES)
		files.removeLast();
	
	settings.setValue("recentFileList", files);
	
	foreach (QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mainWin = qobject_cast<MainWindow*>(widget);
		if (mainWin)
			mainWin->updateRecentFileActions();
	}*/
}
void MainWindow::setHasOpenedFile(bool value)
{
	if (value && !has_opened_file)
	{
		saveAct->setVisible(true);
		saveAsAct->setVisible(true);
	}
	else if (!value && has_opened_file)
	{
		saveAct->setVisible(false);
		saveAsAct->setVisible(false);
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
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	if (showSaveOnCloseDialog())
	{
		saveWindowSettings();
		event->accept();
	}
	else
		event->ignore();
}

bool MainWindow::showSaveOnCloseDialog()
{
	if (has_opened_file && has_unsaved_changes)
	{
		/* TODO! Differ between untitled files (Save as..) and other ones
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, APP_NAME,
								   tr("The document has been modified.\n"
		                           "Do you want to save your changes?"),
								   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

	   if (ret == QMessageBox::Save)
		   return controller->save();
	   else if (ret == QMessageBox::Cancel)
		   return false;*/
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
	bool maximized = settings.value("maximized", true).toBool();
	settings.endGroup();
	
	move(pos);
	resize(size);
	if (maximized)
		showMaximized();
}
MainWindow* MainWindow::findMainWindow(const QString& fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
	
	foreach (QWidget *widget, qApp->topLevelWidgets())
	{
		MainWindow* other = qobject_cast<MainWindow*>(widget);
		if (other && other->getCurrentFilePath() == canonicalFilePath)
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
		new_map->loadFrom(symbol_set_path);
	
	// Open in current or new window?
	if (has_opened_file)
	{
		MainWindow* new_window = new MainWindow(true);
		new_window->setController(new MapEditorController(MapEditorController::MapEditor, new_map));
		new_window->show();
	}
	else
		setController(new MapEditorController(MapEditorController::MapEditor, new_map));
}
void MainWindow::showOpenDialog()
{
	// TODO: save directory
	QString path = QFileDialog::getOpenFileName(this, tr("Open file ..."), QString(), tr("Maps (*.omap *.ocd);;All files (*.*)"));
	path = QFileInfo(path).canonicalFilePath();
	
	if (path.isEmpty())
		return;
	
	openPath(path);
}
bool MainWindow::openPath(QString path)
{
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
	return true;
}
void MainWindow::save()
{
	if (!controller)
		return;
	if (current_path.isEmpty())
	{
		saveAs();
		return;
	}
	
	if (controller->save(current_path))
		setHasUnsavedChanges(false);
}
void MainWindow::saveAs()
{
	if (!controller)
		return;
	
	QString path = QFileDialog::getSaveFileName(this, tr("Save file ..."), QString(), tr("Maps (*.omap *.ocd);;All files (*.*)"));
	if (path.isEmpty())
		return;
	if (!path.endsWith(".omap", Qt::CaseInsensitive))
		path.append(".omap");
	
	setCurrentFile(path);
	save();
	//assert(current_path == QFileInfo(current_path).canonicalFilePath());
}

void MainWindow::showSettings()
{
	// TODO
}
void MainWindow::showAbout()
{
	QDialog about_dialog(this);
	about_dialog.setWindowTitle(tr("About %1").arg(APP_NAME));
	
	QLabel* about_label = new QLabel("<a href=\"http://openorienteering.org\"><img src=\"images/open-orienteering.png\"/></a><br/><br/>"
									 "OpenOrienteering Mapper<br/>"
									 "Copyright (C) 2011  Thomas Sch&ouml;ps<br/>"
									 "This program comes with ABSOLUTELY NO WARRANTY;<br/>"
									 "This is free software, and you are welcome to redistribute it<br/>"
									 "under certain conditions; see the file COPYING for details.");
	QPushButton* about_ok = new QPushButton(tr("Ok"));
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(about_label, 0, 0, 1, 2);
	layout->addWidget(about_ok, 1, 1);
	layout->setColumnStretch(0, 1);
	about_dialog.setLayout(layout);
	
	connect(about_ok, SIGNAL(clicked(bool)), &about_dialog, SLOT(accept()));
	connect(about_label, SIGNAL(linkActivated(QString)), this, SLOT(linkClicked(QString)));
	
	about_dialog.setWindowModality(Qt::WindowModal);
	about_dialog.exec();
}
void MainWindow::showHelp()
{
	// TODO
}
void MainWindow::linkClicked(QString link)
{
	QDesktopServices::openUrl(link);
}

void MainWindow::gotUnsavedChanges()
{
	setHasUnsavedChanges(true);
}

#include "main_window.moc"
