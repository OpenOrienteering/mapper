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


#include "map_editor.h"

#include <QtGui>

#include "map.h"
#include "color_dock_widget.h"

// ### MapEditorController ###

MapEditorController::MapEditorController()
{
	map = NULL;
	main_view = NULL;
}
MapEditorController::MapEditorController(Map* map) : map(map)
{
	main_view = new MapView(map);
}
MapEditorController::~MapEditorController()
{
	delete main_view;
}

bool MapEditorController::save(const QString& path)
{
	if (map)
		return map->saveTo(path);
	else
		return false;
}
bool MapEditorController::load(const QString& path)
{
	if (!map)
		map = new Map();
	
	bool result = map->loadFrom(path);
	if (result)
		main_view = new MapView(map);
	else
		delete map;
	
	return result;
}

void MapEditorController::attach(MainWindow* window)
{
	this->window = window;
	window->setHasOpenedFile(true);
	connect(map, SIGNAL(gotUnsavedChanges()), window, SLOT(gotUnsavedChanges()));
	
	map_widget = new MapWidget();
	map_widget->setMapView(main_view);
	window->setCentralWidget(map_widget);
	
	color_dock_widget = NULL;
	
	// Edit menu
	QAction* undo_act = new QAction(QIcon("images/undo.png"), tr("Undo"), this);	// TODO: update this with a desc. of what will be undone
	undo_act->setShortcuts(QKeySequence::Undo);
	undo_act->setStatusTip(tr("Undo the last step"));
	connect(undo_act, SIGNAL(triggered()), this, SLOT(undo()));
	
	QAction* redo_act = new QAction(QIcon("images/redo.png"), tr("Redo"), this);	// TODO: update this with a desc. of what will be redone
	redo_act->setShortcuts(QKeySequence::Redo);
	redo_act->setStatusTip(tr("Redo the next step"));
	connect(redo_act, SIGNAL(triggered()), this, SLOT(redo()));
	
	undo_act->setEnabled(false);
	redo_act->setEnabled(false);
	
	QAction* cut_act = new QAction(QIcon("images/cut.png"), tr("Cu&t"), this);
	cut_act->setShortcuts(QKeySequence::Cut);
	connect(cut_act, SIGNAL(triggered()), this, SLOT(cut()));
	
	QAction* copy_act = new QAction(QIcon("images/copy.png"), tr("&Copy"), this);
	copy_act->setShortcuts(QKeySequence::Copy);
	connect(copy_act, SIGNAL(triggered()), this, SLOT(copy()));
	
	QAction* paste_act = new QAction(QIcon("images/paste.png"), tr("&Paste"), this);
	paste_act->setShortcuts(QKeySequence::Paste);
	connect(paste_act, SIGNAL(triggered()), this, SLOT(paste()));
	
	QMenu* edit_menu = window->menuBar()->addMenu(tr("&Edit"));
	edit_menu->addAction(undo_act);
	edit_menu->addAction(redo_act);
	edit_menu->addSeparator();
	edit_menu->addAction(cut_act);
	edit_menu->addAction(copy_act);
	edit_menu->addAction(paste_act);
	
	// Symbols menu
	QAction* symbol_window_act = new QAction(QIcon("images/window-new.png"), tr("Symbol window"), this);
	symbol_window_act->setCheckable(true);
	symbol_window_act->setStatusTip(tr("Show/Hide the symbol window"));
	connect(symbol_window_act, SIGNAL(triggered(bool)), this, SLOT(showSymbolWindow(bool)));
	
	color_window_act = new QAction(QIcon("images/window-new.png"), tr("Color window"), this);
	color_window_act->setCheckable(true);
	color_window_act->setStatusTip(tr("Show/Hide the color window"));
	connect(color_window_act, SIGNAL(triggered(bool)), this, SLOT(showColorWindow(bool)));
	
	QAction* load_symbols_from_act = new QAction(tr("Load symbols from ..."), this);
	load_symbols_from_act->setStatusTip(tr("Replace the symbols with those from another map file"));
	connect(load_symbols_from_act, SIGNAL(triggered()), this, SLOT(loadSymbolsFromClicked()));
	
	QAction* load_colors_from_act = new QAction(tr("Load colors from ..."), this);
	load_colors_from_act->setStatusTip(tr("Replace the colors with those from another map file"));
	connect(load_colors_from_act, SIGNAL(triggered()), this, SLOT(loadColorsFromClicked()));
	
	QMenu* symbols_menu = window->menuBar()->addMenu(tr("Sy&mbols"));
	symbols_menu->addAction(symbol_window_act);
	symbols_menu->addAction(color_window_act);
	symbols_menu->addSeparator();
	symbols_menu->addAction(load_symbols_from_act);
	symbols_menu->addAction(load_colors_from_act);
	
	// TODO: Templates & Map menu
}
void MapEditorController::detach()
{
	QWidget* widget = window->centralWidget();
	window->setCentralWidget(NULL);
	delete widget;
}

void MapEditorController::undo()
{

}
void MapEditorController::redo()
{

}

void MapEditorController::cut()
{

}
void MapEditorController::copy()
{

}
void MapEditorController::paste()
{

}

void MapEditorController::showSymbolWindow(bool show)
{
	
}
void MapEditorController::showColorWindow(bool show)
{
	if (color_dock_widget)
		color_dock_widget->setVisible(!color_dock_widget->isVisible());
	else //if (show)
	{
		color_dock_widget = new EditorDockWidget(tr("Colors"), color_window_act, window);
		color_dock_widget->setWidget(new ColorWidget(map, color_dock_widget));
		window->addDockWidget(Qt::LeftDockWidgetArea, color_dock_widget, Qt::Vertical);
	}
}

void MapEditorController::loadSymbolsFromClicked()
{

}
void MapEditorController::loadColorsFromClicked()
{

}

// ### MapWidget ###

MapWidget::MapWidget(QWidget* parent) : QWidget(parent)
{
	view = NULL;
	
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
}
MapWidget::~MapWidget()
{
	if (view)
		view->getMap()->removeMapWidget(this);
}

void MapWidget::setMapView(MapView* view)
{
	if (this->view != view)
	{
		if (this->view)
			this->view->getMap()->removeMapWidget(this);
		
		this->view = view;
		
		if (view)
			view->getMap()->addMapWidget(this);
		
		update();
	}
}

void MapWidget::paintEvent(QPaintEvent* event)
{
	// Draw on the widget
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	// Background color
	painter.fillRect(event->rect(), QColor(Qt::gray));
	
	// No colors defined? Provide a litte help message ...
	if (view && view->getMap()->getNumColors() == 0)
	{
		QFont font = painter.font();
		font.setPointSize(2 * font.pointSize());
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tr("- Empty map -\nStart by defining some colors:\nSelect Symbols -> Color window to\nopen the color dialog and\ndefine the colors there."));
	}
	else if (true) // TODO; No symbols defined?
	{
		QFont font = painter.font();
		font.setPointSize(2 * font.pointSize());
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tr("- No symbols -\nNow define some symbols:\nRight-click in the symbol bar\nand select \"New\" to create\na new symbol."));
	}
	
	painter.end();
}
void MapWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}
void MapWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
}
void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
}
void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
}
void MapWidget::wheelEvent(QWheelEvent* event)
{
    QWidget::wheelEvent(event);
}

// ### EditorDockWidget ###

EditorDockWidget::EditorDockWidget(const QString title, QAction* action, QWidget* parent): QDockWidget(title, parent), action(action)
{
}
void EditorDockWidget::closeEvent(QCloseEvent* event)
{
	action->setChecked(false);
    QDockWidget::closeEvent(event);
}

#include "map_editor.moc"
