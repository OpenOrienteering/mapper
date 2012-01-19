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


#include "map_editor.h"

#include <QtGui>

#include "map.h"
#include "map_widget.h"
#include "color_dock_widget.h"
#include "symbol_dock_widget.h"
#include "template_dock_widget.h"
#include "template.h"
#include "paint_on_template.h"
#include "gps_coordinates.h"
#include "symbol.h"
#include "draw_point.h"

// ### MapEditorController ###

MapEditorController::MapEditorController(OperatingMode mode)
{
	this->mode = mode;
	map = NULL;
	main_view = NULL;
	
	symbol_widget = NULL;
	editor_activity = NULL;
	current_tool = NULL;	// TODO: default tool?
	last_painted_on_template = NULL;
}
MapEditorController::MapEditorController(OperatingMode mode, Map* map)
{
	this->mode = mode;
	this->map = NULL;
	main_view = NULL;
	
	setMap(map, true);
	
	editor_activity = NULL;
	current_tool = NULL;	// TODO: default tool?
	last_painted_on_template = NULL;
}
MapEditorController::~MapEditorController()
{
	delete current_tool;
	delete editor_activity;
	delete main_view;
	delete map;
}

void MapEditorController::setTool(MapEditorTool* new_tool)
{
	delete current_tool;
	map->clearDrawingBoundingBox();
	window->setStatusBarText("");
	
	current_tool = new_tool;
	if (current_tool)
		current_tool->init();
	
	map_widget->setTool(current_tool);
}

void MapEditorController::setEditorActivity(MapEditorActivity* new_activity)
{
	delete editor_activity;
	map->clearActivityBoundingBox();
	
	editor_activity = new_activity;
	if (editor_activity)
		editor_activity->init();
	
	map_widget->setActivity(editor_activity);
}

bool MapEditorController::save(const QString& path)
{
	if (map)
		return map->saveTo(path, this);
	else
		return false;
}
bool MapEditorController::load(const QString& path)
{
	if (!map)
		map = new Map();
	
	bool result = map->loadFrom(path, this);
	if (result)
		setMap(map, false);
	else
		delete map;
	
	return result;
}

void MapEditorController::saveWidgetsAndViews(QFile* file)
{
	// TODO: currently, this just saves/loads the main view
	
	main_view->save(file);
}
void MapEditorController::loadWidgetsAndViews(QFile* file)
{
	main_view = new MapView(map);
	main_view->load(file);
}

void MapEditorController::attach(MainWindow* window)
{
	color_dock_widget = NULL;
	symbol_dock_widget = NULL;
	template_dock_widget = NULL;
	
	this->window = window;
	if (mode == MapEditor)
		window->setHasOpenedFile(true);
	connect(map, SIGNAL(gotUnsavedChanges()), window, SLOT(gotUnsavedChanges()));
	
	// Add zoom / cursor position field to status bar
	statusbar_zoom_label = new QLabel();
	statusbar_zoom_label->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	statusbar_zoom_label->setFixedWidth(90);
	statusbar_zoom_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	statusbar_cursorpos_label = new QLabel();
	statusbar_cursorpos_label->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	statusbar_cursorpos_label->setFixedWidth(100);
	statusbar_cursorpos_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	window->statusBar()->addPermanentWidget(statusbar_zoom_label);
	window->statusBar()->addPermanentWidget(statusbar_cursorpos_label);
	
	// Create map widget
	map_widget = new MapWidget(mode == MapEditor, mode == SymbolEditor);
	map_widget->setMapView(main_view);
	map_widget->setZoomLabel(statusbar_zoom_label);
	map_widget->setCursorposLabel(statusbar_cursorpos_label);
	window->setCentralWidget(map_widget);
	
	// Create menu
	if (mode == MapEditor)
		createMenu();
	
	// Create toolbar
	if (mode == MapEditor)
		createToolbar();
	
	// Check if there is an invalid template and if so, output a warning
	bool has_invalid_template = false;
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		if (!map->getTemplate(i)->isTemplateValid())
		{
			has_invalid_template = true;
			break;
		}
	}
	if (has_invalid_template)
		window->setStatusBarText("<font color=\"#c00\">" + tr("One or more templates could not be loaded. Use the Templates -> Template setup window to resolve the issue(s) by clicking on the red template file name(s).") + "</font>");

	// Show the symbol window
	if (mode == MapEditor)
		symbol_window_act->trigger();
}
void MapEditorController::createMenu()
{
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
	symbol_window_act = new QAction(QIcon("images/window-new.png"), tr("Symbol window"), this);
	symbol_window_act->setCheckable(true);
	symbol_window_act->setStatusTip(tr("Show/Hide the symbol window"));
	connect(symbol_window_act, SIGNAL(triggered(bool)), this, SLOT(showSymbolWindow(bool)));
	
	color_window_act = new QAction(QIcon("images/window-new.png"), tr("Color window"), this);
	color_window_act->setCheckable(true);
	color_window_act->setStatusTip(tr("Show/Hide the color window"));
	connect(color_window_act, SIGNAL(triggered(bool)), this, SLOT(showColorWindow(bool)));
	
	QAction* load_symbols_from_act = new QAction(tr("Load symbols from..."), this);
	load_symbols_from_act->setStatusTip(tr("Replace the symbols with those from another map file"));
	connect(load_symbols_from_act, SIGNAL(triggered()), this, SLOT(loadSymbolsFromClicked()));
	
	QAction* load_colors_from_act = new QAction(tr("Load colors from..."), this);
	load_colors_from_act->setStatusTip(tr("Replace the colors with those from another map file"));
	connect(load_colors_from_act, SIGNAL(triggered()), this, SLOT(loadColorsFromClicked()));
	
	QMenu* symbols_menu = window->menuBar()->addMenu(tr("Sy&mbols"));
	symbols_menu->addAction(symbol_window_act);
	symbols_menu->addAction(color_window_act);
	symbols_menu->addSeparator();
	symbols_menu->addAction(load_symbols_from_act);
	symbols_menu->addAction(load_colors_from_act);
	
	// Templates menu
	template_window_act = new QAction(QIcon("images/window-new.png"), tr("Template setup window"), this);
	template_window_act->setCheckable(true);
	template_window_act->setStatusTip(tr("Show/Hide the template window"));
	connect(template_window_act, SIGNAL(triggered(bool)), this, SLOT(showTemplateWindow(bool)));
	
	QAction* template_config_window_act = new QAction(QIcon("images/window-new.png"), tr("Template configurations window"), this);
	template_config_window_act->setCheckable(true);
	template_config_window_act->setStatusTip(tr("Show/Hide the template configurations window"));
	//connect(template_config_window_act, SIGNAL(triggered(bool)), this, SLOT(showTemplateConfigurationsWindow(bool))); TODO
	
	QAction* template_visibilities_window_act = new QAction(QIcon("images/window-new.png"), tr("Template visibilities window"), this);
	template_visibilities_window_act->setCheckable(true);
	template_visibilities_window_act->setStatusTip(tr("Show/Hide the template visibilities window"));
	//connect(template_visibilities_window_act, SIGNAL(triggered(bool)), this, SLOT(showTemplateVisibilitiesWindow(bool))); TODO
	
	QAction* open_template_act = new QAction(tr("Open template..."), this);
	connect(open_template_act, SIGNAL(triggered()), this, SLOT(openTemplateClicked()));
	
	QMenu* template_menu = window->menuBar()->addMenu(tr("&Templates"));
	template_menu->addAction(template_window_act);
	template_menu->addAction(template_config_window_act);
	template_menu->addAction(template_visibilities_window_act);
	template_menu->addSeparator();
	template_menu->addAction(open_template_act);
	
	// TODO: Map menu
	
	// GPS menu
	QAction* edit_gps_projection_parameters_act = new QAction(tr("Edit projection parameters..."), this);
	connect(edit_gps_projection_parameters_act, SIGNAL(triggered()), this, SLOT(editGPSProjectionParameters()));
	
	QMenu* gps_menu = window->menuBar()->addMenu(tr("&GPS"));
	gps_menu->addAction(edit_gps_projection_parameters_act);
}
void MapEditorController::createToolbar()
{
	QToolBar* toolbar_drawing = window->addToolBar(tr("Drawing"));
	
	draw_point_act = new QAction(QIcon("images/draw-point.png"), tr("Set point objects"), this);
	draw_point_act->setCheckable(true);
	connect(draw_point_act, SIGNAL(triggered(bool)), this, SLOT(drawPointClicked(bool)));
	toolbar_drawing->addAction(draw_point_act);
	
	toolbar_drawing->addSeparator();
	
	paint_on_template_act = new QAction(QIcon("images/pencil.png"), tr("Paint on template"), this);
	paint_on_template_act->setCheckable(true);
	updatePaintOnTemplateAction();
	connect(paint_on_template_act, SIGNAL(triggered(bool)), this, SLOT(paintOnTemplateClicked(bool)));
	
	QToolButton* paint_on_template_button = new QToolButton();
	paint_on_template_button->setCheckable(true);
	paint_on_template_button->setDefaultAction(paint_on_template_act);
	paint_on_template_button->setPopupMode(QToolButton::MenuButtonPopup);
	QMenu* paint_on_template_menu = new QMenu(paint_on_template_button);
	paint_on_template_menu->addAction(tr("Select template..."));
	paint_on_template_button->setMenu(paint_on_template_menu);
	toolbar_drawing->addWidget(paint_on_template_button);
	connect(paint_on_template_menu, SIGNAL(triggered(QAction*)), this, SLOT(paintOnTemplateSelectClicked()));
}
void MapEditorController::detach()
{
	QWidget* widget = window->centralWidget();
	window->setCentralWidget(NULL);
	delete widget;
	
	delete statusbar_zoom_label;
	delete statusbar_cursorpos_label;
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
	if (symbol_dock_widget)
		symbol_dock_widget->setVisible(!symbol_dock_widget->isVisible());
	else
	{
		symbol_dock_widget = new EditorDockWidget(tr("Symbols"), symbol_window_act, window);
		symbol_widget = new SymbolWidget(map, symbol_dock_widget);
		symbol_dock_widget->setWidget(symbol_widget);
		window->addDockWidget(Qt::RightDockWidgetArea, symbol_dock_widget, Qt::Vertical);
		
		connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
		selectedSymbolsChanged();
	}
}
void MapEditorController::showColorWindow(bool show)
{
	if (color_dock_widget)
		color_dock_widget->setVisible(!color_dock_widget->isVisible());
	else
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

void MapEditorController::showTemplateWindow(bool show)
{
	if (template_dock_widget)
		template_dock_widget->setVisible(!template_dock_widget->isVisible());
	else
	{
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, window);
		template_dock_widget->setWidget(new TemplateWidget(map, main_view, this, template_dock_widget));
		window->addDockWidget(Qt::RightDockWidgetArea, template_dock_widget, Qt::Vertical);
	}
}
void MapEditorController::openTemplateClicked()
{
	Template* new_template = TemplateWidget::showOpenTemplateDialog(window, main_view);
	if (!new_template)
		return;
	
	if (template_dock_widget && !template_dock_widget->isVisible())
		template_dock_widget->setVisible(true);
	else
	{
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, window);
		template_dock_widget->setWidget(new TemplateWidget(map, main_view, this, template_dock_widget));
		window->addDockWidget(Qt::RightDockWidgetArea, template_dock_widget, Qt::Vertical);
	}
	
	TemplateWidget* template_widget = reinterpret_cast<TemplateWidget*>(template_dock_widget->widget());
	template_widget->addTemplateAt(new_template, -1);
}

void MapEditorController::editGPSProjectionParameters()
{
	GPSProjectionParametersDialog dialog(window, &map->getGPSProjectionParameters());
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Rejected)
		return;
	
	map->setGPSProjectionParameters(dialog.getParameters());
}

void MapEditorController::selectedSymbolsChanged()
{
	Symbol::Type type = Symbol::NoSymbol;
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol)
		type = symbol->getType();
	
	draw_point_act->setEnabled(type == Symbol::Point);
	draw_point_act->setToolTip(tr("This tool places point objects on the map.") + ((type == Symbol::Point) ? (" " + tr("Select a point symbol to be able to use this tool.")) : ""));
}
void MapEditorController::drawPointClicked(bool checked)
{
	setTool(checked ? new DrawPointTool(this, draw_point_act, symbol_widget) : NULL);
}

void MapEditorController::paintOnTemplateClicked(bool checked)
{
	if (checked)
	{
		if (!last_painted_on_template)
			paintOnTemplateSelectClicked();
		else
			paintOnTemplate(last_painted_on_template);
	}
	else
		setTool(NULL);
}
void MapEditorController::paintOnTemplateSelectClicked()
{
	Template* only_paintable_template = NULL;
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		if (map->getTemplate(i)->canBeDrawnOnto())
		{
			if (!only_paintable_template)
				only_paintable_template = map->getTemplate(i);
			else
			{
				only_paintable_template = NULL;
				break;
			}
		}
	}
	
	if (only_paintable_template)
		last_painted_on_template = only_paintable_template;
	else
	{
		PaintOnTemplateSelectDialog paintDialog(map, window);
		paintDialog.setWindowModality(Qt::WindowModal);
		if (paintDialog.exec() == QDialog::Rejected)
		{
			paint_on_template_act->setChecked(false);
			return;
		}
		
		last_painted_on_template = paintDialog.getSelectedTemplate();
	}
	paintOnTemplate(last_painted_on_template);
}
void MapEditorController::paintOnTemplate(Template* temp)
{
	setTool(new PaintOnTemplateTool(this, paint_on_template_act, temp));
	paint_on_template_act->setChecked(true);
}
void MapEditorController::updatePaintOnTemplateAction()
{
	if (map)
	{
		int i;
		for (i = 0; i < map->getNumTemplates(); ++i)
		{
			if (map->getTemplate(i)->canBeDrawnOnto() && map->getTemplate(i)->isTemplateValid())
				break;
		}
		paint_on_template_act->setEnabled(i != map->getNumTemplates());
	}
	else
		paint_on_template_act->setEnabled(false);
	
	if (paint_on_template_act->isEnabled())
		paint_on_template_act->setStatusTip(tr("Paint free-handedly on a template"));
	else
		paint_on_template_act->setStatusTip(tr("Paint free-handedly on a template. Create or load a template which can be drawn onto to activate this button"));
}

void MapEditorController::templateAdded(int pos, Template* temp)
{
	if (mode == MapEditor && temp->canBeDrawnOnto())
		updatePaintOnTemplateAction();
}
void MapEditorController::templateDeleted(int pos, Template* temp)
{
	if (mode == MapEditor && temp->canBeDrawnOnto())
		updatePaintOnTemplateAction();
}

void MapEditorController::setMap(Map* map, bool create_new_map_view)
{
	if (this->map)
	{
		disconnect(this->map, SIGNAL(templateAdded(int,Template*)), this, SLOT(templateAdded(int,Template*)));
		disconnect(this->map, SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
	}
	
	this->map = map;
	if (create_new_map_view)
		main_view = new MapView(map);
	
	connect(map, SIGNAL(templateAdded(int,Template*)), this, SLOT(templateAdded(int,Template*)));
	connect(map, SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
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

// ### MapEditorTool ###

MapEditorTool::MapEditorTool(MapEditorController* editor, QAction* tool_button): QObject(NULL), tool_button(tool_button), editor(editor)
{
}
MapEditorTool::~MapEditorTool()
{
	if (tool_button)
		tool_button->setChecked(false);
}

void MapEditorTool::setStatusBarText(const QString& text)
{
	editor->getWindow()->setStatusBarText(text);
}

#include "map_editor.moc"
