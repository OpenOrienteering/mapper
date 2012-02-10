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
#include "map_dialog_scale.h"
#include "print_dock_widget.h"
#include "color_dock_widget.h"
#include "symbol_dock_widget.h"
#include "template_dock_widget.h"
#include "template.h"
#include "paint_on_template.h"
#include "gps_coordinates.h"
#include "symbol.h"
#include "draw_point.h"
#include "draw_path.h"
#include "edit_tool.h"
#include "util.h"
#include "map_undo.h"

// ### MapEditorController ###

MapEditorController::MapEditorController(OperatingMode mode, Map* map)
{
	this->mode = mode;
	this->map = NULL;
	main_view = NULL;
	symbol_widget = NULL;
	window = NULL;
	editing_in_progress = false;
	
	if (map)
		setMap(map, true);
	
	editor_activity = NULL;
	current_tool = NULL;
	override_tool = NULL;
	last_painted_on_template = NULL;
}
MapEditorController::~MapEditorController()
{
	delete current_tool;
	delete override_tool;
	delete editor_activity;
	delete main_view;
	delete map;
}

void MapEditorController::setTool(MapEditorTool* new_tool)
{
	delete current_tool;
	if (!override_tool)
	{
		map->clearDrawingBoundingBox();
		window->setStatusBarText("");
	}
	
	current_tool = new_tool;
	if (current_tool && !override_tool)
	{
		current_tool->init();
		if (current_tool->getAction())
			current_tool->getAction()->setChecked(true);
	}
	
	if (!override_tool)
		map_widget->setTool(current_tool);
}
void MapEditorController::setEditTool()
{
	if (!current_tool || !current_tool->getType() == MapEditorTool::Edit)
		setTool(new EditTool(this, edit_tool_act));
}
void MapEditorController::setOverrideTool(MapEditorTool* new_override_tool)
{
	delete override_tool;
	map->clearDrawingBoundingBox();
	window->setStatusBarText("");
	
	override_tool = new_override_tool;
	if (override_tool)
	{
		override_tool->init();
		if (override_tool->getAction())
			override_tool->getAction()->setChecked(true);
	}
	else if (current_tool)
	{
		current_tool->init();
		if (current_tool->getAction())
			current_tool->getAction()->setChecked(true);
	}
	
	map_widget->setTool(override_tool ? override_tool : current_tool);
}
MapEditorTool* MapEditorController::getDefaultDrawToolForSymbol(Symbol* symbol)
{
	if (!symbol)
		return NULL;
	else if (symbol->getType() == Symbol::Point)
		return new DrawPointTool(this, draw_point_act, symbol_widget);
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined)
		return new DrawPathTool(this, draw_path_act, symbol_widget);
	else
		assert(false);
	return NULL;
}

void MapEditorController::setEditingInProgress(bool value)
{
	if (value != editing_in_progress)
	{
		editing_in_progress = value;
		
		undo_act->setEnabled(!editing_in_progress && map->objectUndoManager().getNumUndoSteps() > 0);
		redo_act->setEnabled(!editing_in_progress && map->objectUndoManager().getNumRedoSteps() > 0);
	}
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
	{
		delete map;
		map = NULL;
	}
	
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
	print_dock_widget = NULL;
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
	map_widget = new MapWidget(mode == MapEditor, true); //mode == SymbolEditor);	TODO: Make antialiasing configurable
	connect(window, SIGNAL(keyPressed(QKeyEvent*)), map_widget, SLOT(keyPressed(QKeyEvent*)));
	connect(window, SIGNAL(keyReleased(QKeyEvent*)), map_widget, SLOT(keyReleased(QKeyEvent*)));
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
	
	// Auto-select the edit tool
	if (mode == MapEditor)
	{
		edit_tool_act->setChecked(true);
		setEditTool();
	}
	
	// Update enabled/disabled state for the tools ...
	selectedObjectsChanged();
	// ... and for undo
	undoStepAvailabilityChanged();
	
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
	// Extend file menu
	print_act = new QAction(QIcon("images/print.png"), tr("Print..."), this);
	print_act->setShortcuts(QKeySequence::Print);
	connect(print_act, SIGNAL(triggered()), this, SLOT(printClicked()));
	
	QMenu* file_menu = window->getFileMenu();
	file_menu->insertAction(window->getCloseAct(), print_act);
	file_menu->insertSeparator(window->getCloseAct());
	
	// Edit menu
	undo_act = new QAction(QIcon("images/undo.png"), tr("Undo"), this);	// TODO: update this with a desc. of what will be undone
	undo_act->setShortcuts(QKeySequence::Undo);
	undo_act->setStatusTip(tr("Undo the last step"));
	connect(undo_act, SIGNAL(triggered()), this, SLOT(undo()));
	
	redo_act = new QAction(QIcon("images/redo.png"), tr("Redo"), this);	// TODO: update this with a desc. of what will be redone
	redo_act->setShortcuts(QKeySequence::Redo);
	redo_act->setStatusTip(tr("Redo the next step"));
	connect(redo_act, SIGNAL(triggered()), this, SLOT(redo()));
	
	// TODO
	/*QAction* cut_act = new QAction(QIcon("images/cut.png"), tr("Cu&t"), this);
	cut_act->setShortcuts(QKeySequence::Cut);
	connect(cut_act, SIGNAL(triggered()), this, SLOT(cut()));
	
	QAction* copy_act = new QAction(QIcon("images/copy.png"), tr("&Copy"), this);
	copy_act->setShortcuts(QKeySequence::Copy);
	connect(copy_act, SIGNAL(triggered()), this, SLOT(copy()));
	
	QAction* paste_act = new QAction(QIcon("images/paste.png"), tr("&Paste"), this);
	paste_act->setShortcuts(QKeySequence::Paste);
	connect(paste_act, SIGNAL(triggered()), this, SLOT(paste()));*/
	
	QMenu* edit_menu = window->menuBar()->addMenu(tr("&Edit"));
	edit_menu->addAction(undo_act);
	edit_menu->addAction(redo_act);
	/*edit_menu->addSeparator();
	edit_menu->addAction(cut_act);
	edit_menu->addAction(copy_act);
	edit_menu->addAction(paste_act);*/
	
	// View menu
	QAction* zoom_in_act = new QAction(QIcon("images/view-zoom-in.png"), tr("Zoom in"), this);
	zoom_in_act->setShortcut(tr("F7"));
	connect(zoom_in_act, SIGNAL(triggered()), this, SLOT(zoomIn()));
	
	QAction* zoom_out_act = new QAction(QIcon("images/view-zoom-out.png"), tr("Zoom in"), this);
	zoom_out_act->setShortcut(tr("F8"));
	connect(zoom_out_act, SIGNAL(triggered()), this, SLOT(zoomOut()));
	
	QMenu* view_menu = window->menuBar()->addMenu(tr("&View"));
	view_menu->addAction(zoom_in_act);
	view_menu->addAction(zoom_out_act);
	
	// Symbols menu
	symbol_window_act = new QAction(QIcon("images/window-new.png"), tr("Symbol window"), this);
	symbol_window_act->setCheckable(true);
	symbol_window_act->setStatusTip(tr("Show/Hide the symbol window"));
	connect(symbol_window_act, SIGNAL(triggered(bool)), this, SLOT(showSymbolWindow(bool)));
	
	color_window_act = new QAction(QIcon("images/window-new.png"), tr("Color window"), this);
	color_window_act->setCheckable(true);
	color_window_act->setStatusTip(tr("Show/Hide the color window"));
	connect(color_window_act, SIGNAL(triggered(bool)), this, SLOT(showColorWindow(bool)));
	
	// TODO
	/*QAction* load_symbols_from_act = new QAction(tr("Load symbols from..."), this);
	load_symbols_from_act->setStatusTip(tr("Replace the symbols with those from another map file"));
	connect(load_symbols_from_act, SIGNAL(triggered()), this, SLOT(loadSymbolsFromClicked()));
	
	QAction* load_colors_from_act = new QAction(tr("Load colors from..."), this);
	load_colors_from_act->setStatusTip(tr("Replace the colors with those from another map file"));
	connect(load_colors_from_act, SIGNAL(triggered()), this, SLOT(loadColorsFromClicked()));*/
	
	QAction* scale_all_symbols_act = new QAction(tr("Scale all symbols..."), this);
	scale_all_symbols_act->setStatusTip(tr("Scale the whole symbol set"));
	connect(scale_all_symbols_act, SIGNAL(triggered()), this, SLOT(scaleAllSymbolsClicked()));
	
	QMenu* symbols_menu = window->menuBar()->addMenu(tr("Sy&mbols"));
	symbols_menu->addAction(symbol_window_act);
	symbols_menu->addAction(color_window_act);
	symbols_menu->addSeparator();
	/*symbols_menu->addAction(load_symbols_from_act);
	symbols_menu->addAction(load_colors_from_act);*/
	symbols_menu->addAction(scale_all_symbols_act);
	
	// Map menu
	QAction* scale_map_act = new QAction(tr("Change map scale..."), this);
	scale_map_act->setStatusTip(tr("Change the map scale and adjust map objects and symbol sizes"));
	connect(scale_map_act, SIGNAL(triggered(bool)), this, SLOT(scaleMapClicked()));
	
	QMenu* map_menu = window->menuBar()->addMenu(tr("M&ap"));
	map_menu->addAction(scale_map_act);
	
	// Templates menu
	template_window_act = new QAction(QIcon("images/window-new.png"), tr("Template setup window"), this);
	template_window_act->setCheckable(true);
	template_window_act->setStatusTip(tr("Show/Hide the template window"));
	connect(template_window_act, SIGNAL(triggered(bool)), this, SLOT(showTemplateWindow(bool)));
	
	/* TODO
	QAction* template_config_window_act = new QAction(QIcon("images/window-new.png"), tr("Template configurations window"), this);
	template_config_window_act->setCheckable(true);
	template_config_window_act->setStatusTip(tr("Show/Hide the template configurations window"));
	connect(template_config_window_act, SIGNAL(triggered(bool)), this, SLOT(showTemplateConfigurationsWindow(bool)));
	
	QAction* template_visibilities_window_act = new QAction(QIcon("images/window-new.png"), tr("Template visibilities window"), this);
	template_visibilities_window_act->setCheckable(true);
	template_visibilities_window_act->setStatusTip(tr("Show/Hide the template visibilities window"));
	connect(template_visibilities_window_act, SIGNAL(triggered(bool)), this, SLOT(showTemplateVisibilitiesWindow(bool)));
	*/
	
	QAction* open_template_act = new QAction(tr("Open template..."), this);
	connect(open_template_act, SIGNAL(triggered()), this, SLOT(openTemplateClicked()));
	
	QMenu* template_menu = window->menuBar()->addMenu(tr("&Templates"));
	template_menu->addAction(template_window_act);
	/*template_menu->addAction(template_config_window_act);
	template_menu->addAction(template_visibilities_window_act);*/
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
	// View toolbar
	QToolBar* toolbar_view = window->addToolBar(tr("View"));
	
	QAction* show_all_act = new QAction(QIcon("images/view-show-all.png"), tr("Show whole map"), this);
	connect(show_all_act, SIGNAL(triggered(bool)), this, SLOT(showWholeMap()));
	toolbar_view->addAction(show_all_act);
	
	// Drawing toolbar
	QToolBar* toolbar_drawing = window->addToolBar(tr("Drawing"));
	
	edit_tool_act = new QAction(QIcon("images/tool-edit.png"), tr("Edit objects"), this);
	edit_tool_act->setCheckable(true);
	connect(edit_tool_act, SIGNAL(triggered(bool)), this, SLOT(editToolClicked(bool)));
	toolbar_drawing->addAction(edit_tool_act);
	
	draw_point_act = new QAction(QIcon("images/draw-point.png"), tr("Set point objects"), this);
	draw_point_act->setCheckable(true);
	connect(draw_point_act, SIGNAL(triggered(bool)), this, SLOT(drawPointClicked(bool)));
	toolbar_drawing->addAction(draw_point_act);
	
	draw_path_act = new QAction(QIcon("images/draw-path.png"), tr("Draw paths"), this);
	draw_path_act->setCheckable(true);
	connect(draw_path_act, SIGNAL(triggered(bool)), this, SLOT(drawPathClicked(bool)));
	toolbar_drawing->addAction(draw_path_act);
	
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
	
	// Editing toolbar
	QToolBar* toolbar_editing = window->addToolBar(tr("Editing"));
	
	duplicate_act = new QAction(QIcon("images/tool-duplicate.png"), tr("Duplicate"), this);
	duplicate_act->setShortcut(tr("D"));
	connect(duplicate_act, SIGNAL(triggered(bool)), this, SLOT(duplicateClicked()));
	toolbar_editing->addAction(duplicate_act);
	
	switch_symbol_act = new QAction(QIcon("images/tool-switch-symbol.png"), tr("Switch symbol"), this);
	connect(switch_symbol_act, SIGNAL(triggered(bool)), this, SLOT(switchSymbolClicked()));
	toolbar_editing->addAction(switch_symbol_act);
	
	fill_border_act = new QAction(QIcon("images/tool-fill-border.png"), tr("Fill / Create border"), this);
	connect(fill_border_act, SIGNAL(triggered(bool)), this, SLOT(fillBorderClicked()));
	toolbar_editing->addAction(fill_border_act);
	
	switch_dashes_act = new QAction(QIcon("images/tool-switch-dashes.png"), tr("Switch dash direction"), this);
	switch_dashes_act->setShortcut(tr("Ctrl+D"));
	connect(switch_dashes_act, SIGNAL(triggered(bool)), this, SLOT(switchDashesClicked()));
	toolbar_editing->addAction(switch_dashes_act);
}
void MapEditorController::detach()
{
	QWidget* widget = window->centralWidget();
	window->setCentralWidget(NULL);
	delete widget;
	
	delete statusbar_zoom_label;
	delete statusbar_cursorpos_label;
}

void MapEditorController::printClicked()
{
	if (print_dock_widget)
	{
		if (!print_dock_widget->isVisible())
			print_widget->activate();
		print_dock_widget->setVisible(!print_dock_widget->isVisible());
	}
	else
	{
		print_dock_widget = new EditorDockWidget(tr("Print or Export"), print_act, window);
		print_widget = new PrintWidget(map, window, main_view, this, print_dock_widget);
		print_dock_widget->setChild(print_widget);
		
		// Show dock in floating state
		print_dock_widget->setFloating(true);
		print_dock_widget->show();
		print_dock_widget->setGeometry(getWindow()->geometry().left() + 40, getWindow()->geometry().top() + 100, print_dock_widget->width(), print_dock_widget->height());
		//window->addDockWidget(Qt::LeftDockWidgetArea, print_dock_widget, Qt::Vertical);
	}	
}

void MapEditorController::undo()
{
	doUndo(false);
}
void MapEditorController::redo()
{
	doUndo(true);
}
void MapEditorController::doUndo(bool redo)
{
	MapUndoStep* step = reinterpret_cast<MapUndoStep*>(redo ? map->objectUndoManager().getLastRedoStep() : map->objectUndoManager().getLastUndoStep());
	MapLayer* affected_layer = map->getLayer(step->getLayer());
	std::vector<int> affected_objects;
	step->getAffectedOutcome(affected_objects);
	
	bool in_saved_state, done;
	if (redo)
		in_saved_state = map->objectUndoManager().redo(window, &done);
	else
		in_saved_state = map->objectUndoManager().undo(window, &done);
	
	if (!done)
		return;
	
	if (map->hasUnsavedChanged() && in_saved_state)
	{
		map->setHasUnsavedChanges(false);
		window->setHasUnsavedChanges(false);
	}
	else if (!map->hasUnsavedChanged() && !in_saved_state)
		map->setHasUnsavedChanges(true);
	
	// Select affected objects and ensure that they are visible
	map->clearObjectSelection((int)affected_objects.size() == 0);
	QRectF object_rect;
	for (int i = 0; i < (int)affected_objects.size(); ++i)
	{
		Object* object = affected_layer->getObject(affected_objects[i]);
		rectIncludeSafe(object_rect, object->getExtent());
		map->addObjectToSelection(object, i == (int)affected_objects.size() - 1);
	}
	if (object_rect.isValid())
		map_widget->ensureVisibilityOfRect(object_rect);
}

void MapEditorController::cut()
{
	// TODO
}
void MapEditorController::copy()
{
	// TODO
}
void MapEditorController::paste()
{
	// TODO
}

void MapEditorController::zoomIn()
{
	main_view->zoomSteps(1, false);
}
void MapEditorController::zoomOut()
{
	main_view->zoomSteps(-1, false);
}

void MapEditorController::showSymbolWindow(bool show)
{
	if (symbol_dock_widget)
		symbol_dock_widget->setVisible(!symbol_dock_widget->isVisible());
	else
	{
		symbol_dock_widget = new EditorDockWidget(tr("Symbols"), symbol_window_act, window);
		symbol_widget = new SymbolWidget(map, symbol_dock_widget);
		connect(window, SIGNAL(keyPressed(QKeyEvent*)), symbol_widget, SLOT(keyPressed(QKeyEvent*)));
		connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), symbol_widget, SLOT(symbolChanged(int,Symbol*,Symbol*)));	// NOTE: adjust setMap() if changing this!
		symbol_dock_widget->setChild(symbol_widget);
		window->addDockWidget(Qt::RightDockWidgetArea, symbol_dock_widget, Qt::Vertical);
		
		connect(symbol_widget, SIGNAL(switchSymbolClicked()), this, SLOT(switchSymbolClicked()));
		connect(symbol_widget, SIGNAL(fillBorderClicked()), this, SLOT(fillBorderClicked()));
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
		color_dock_widget->setChild(new ColorWidget(map, color_dock_widget));
		window->addDockWidget(Qt::LeftDockWidgetArea, color_dock_widget, Qt::Vertical);
	}
}

void MapEditorController::loadSymbolsFromClicked()
{
	// TODO
}
void MapEditorController::loadColorsFromClicked()
{
	// TODO
}
void MapEditorController::scaleAllSymbolsClicked()
{
	bool ok;
	double percent = QInputDialog::getDouble(window, tr("Scale all symbols"), tr("Scale to percentage:"), 100, 0, 999999, 6, &ok);
	if (!ok || percent == 100)
		return;
	
	map->scaleAllSymbols(percent / 100.0);
}

void MapEditorController::scaleMapClicked()
{
	ScaleMapDialog dialog(window, map);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
}

void MapEditorController::showTemplateWindow(bool show)
{
	if (template_dock_widget)
		template_dock_widget->setVisible(!template_dock_widget->isVisible());
	else
	{
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, window);
		template_dock_widget->setChild(new TemplateWidget(map, main_view, this, template_dock_widget));
		window->addDockWidget(Qt::RightDockWidgetArea, template_dock_widget, Qt::Vertical);
	}
}
void MapEditorController::openTemplateClicked()
{
	Template* new_template = TemplateWidget::showOpenTemplateDialog(window, main_view);
	if (!new_template)
		return;
	
	if (template_dock_widget)
	{
		if (!template_dock_widget->isVisible())
			template_dock_widget->setVisible(true);
	}
	else
	{
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, window);
		template_dock_widget->setChild(new TemplateWidget(map, main_view, this, template_dock_widget));
		window->addDockWidget(Qt::RightDockWidgetArea, template_dock_widget, Qt::Vertical);
	}
	template_window_act->setChecked(true);
	
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
	draw_point_act->setStatusTip(tr("Place point objects on the map.") + (draw_point_act->isEnabled() ? "" : (" " + tr("Select a point symbol to be able to use this tool."))));
	draw_path_act->setEnabled(type == Symbol::Line || type == Symbol::Area || type == Symbol::Combined);
	draw_path_act->setStatusTip(tr("Draw polygonal and curved lines.") + (draw_path_act->isEnabled() ? "" : (" " + tr("Select a line, area or combined symbol to be able to use this tool."))));

	selectedSymbolsOrObjectsChanged();
}
void MapEditorController::selectedObjectsChanged()
{
	if (mode != MapEditor)
		return;
	
	bool have_selection = map->getNumSelectedObjects() > 0;
	bool have_line = false;
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line)
		{
			have_line = true;
			break;
		}
	}
	
	duplicate_act->setEnabled(have_selection);
	duplicate_act->setStatusTip(tr("Duplicate the selected object(s).") + (duplicate_act->isEnabled() ? "" : (" " + tr("Select at least one object to activate this tool."))));
	switch_dashes_act->setEnabled(have_line);
	switch_dashes_act->setStatusTip(tr("Switch the direction of symbols on line objects.") + (switch_dashes_act->isEnabled() ? "" : (" " + tr("Select at least one line object to activate this tool."))));
	
	selectedSymbolsOrObjectsChanged();
}
void MapEditorController::selectedSymbolsOrObjectsChanged()
{
	Symbol::Type single_symbol_type = Symbol::NoSymbol;
	Symbol* single_symbol = symbol_widget ? symbol_widget->getSingleSelectedSymbol() : NULL;
	if (single_symbol)
		single_symbol_type = single_symbol->getType();
	
	bool single_symbol_compatible;
	bool single_symbol_different;
	map->getSelectionToSymbolCompatibility(single_symbol, single_symbol_compatible, single_symbol_different);
	
	switch_symbol_act->setEnabled(single_symbol_compatible && single_symbol_different);
	switch_symbol_act->setStatusTip(tr("Switches the symbol of the selected object(s) to the selected symbol.") + (switch_symbol_act->isEnabled() ? "" : (" " + tr("Select at least one object and a fitting, different symbol to activate this tool."))));
	fill_border_act->setEnabled(single_symbol_compatible && single_symbol_different);
	fill_border_act->setStatusTip(tr("Fill the selected line(s) or create a border for the selected area(s).") + (fill_border_act->isEnabled() ? "" : (" " + tr("Select at least one object and a fitting, different symbol to activate this tool."))));
}
void MapEditorController::undoStepAvailabilityChanged()
{
	if (mode != MapEditor)
		return;
	
	undo_act->setEnabled(map->objectUndoManager().getNumUndoSteps() > 0);
	redo_act->setEnabled(map->objectUndoManager().getNumRedoSteps() > 0);
}

void MapEditorController::showWholeMap()
{
	QRectF map_extent = map->calculateExtent(true, main_view);
	map_widget->adjustViewToRect(map_extent);
}

void MapEditorController::editToolClicked(bool checked)
{
	if (checked)
		setEditTool();
	else
		setTool(NULL);
}
void MapEditorController::drawPointClicked(bool checked)
{
	setTool(checked ? new DrawPointTool(this, draw_point_act, symbol_widget) : NULL);
}
void MapEditorController::drawPathClicked(bool checked)
{
	setTool(checked ? new DrawPathTool(this, draw_path_act, symbol_widget) : NULL);
}

void MapEditorController::duplicateClicked()
{
	std::vector<Object*> new_objects;
	new_objects.reserve(map->getNumSelectedObjects());
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* duplicate = (*it)->duplicate();
		map->addObject(duplicate);
		new_objects.push_back(duplicate);
	}
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapLayer* layer = map->getCurrentLayer();
	
	map->clearObjectSelection(false);
	for (int i = 0; i < (int)new_objects.size(); ++i)
	{
		undo_step->addObject(layer->findObjectIndex(new_objects[i]));
		map->addObjectToSelection(new_objects[i], i == (int)new_objects.size() - 1);
	}
	
	map->objectUndoManager().addNewUndoStep(undo_step);
	setEditTool();
	window->statusBar()->showMessage(tr("%1 object(s) duplicated").arg((int)new_objects.size()), 2000);
}
void MapEditorController::switchSymbolClicked()
{
	SwitchSymbolUndoStep* undo_step = new SwitchSymbolUndoStep(map);
	MapLayer* layer = map->getCurrentLayer();
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		undo_step->addObject(layer->findObjectIndex(*it), (*it)->getSymbol());
		(*it)->setSymbol(symbol, true);
		(*it)->update(true);
	}
	
	map->setObjectsDirty();
	map->objectUndoManager().addNewUndoStep(undo_step);
}
void MapEditorController::fillBorderClicked()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	std::vector<Object*> new_objects;
	new_objects.reserve(map->getNumSelectedObjects());
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapLayer* layer = map->getCurrentLayer();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* duplicate = (*it)->duplicate();
		duplicate->setSymbol(symbol, true);
		map->addObject(duplicate);
		new_objects.push_back(duplicate);
	}
	
	map->clearObjectSelection(false);
	for (int i = 0; i < (int)new_objects.size(); ++i)
	{
		map->addObjectToSelection(new_objects[i], i == (int)new_objects.size() - 1);
		undo_step->addObject(layer->findObjectIndex(new_objects[i]));
	}
	map->objectUndoManager().addNewUndoStep(undo_step);
}
void MapEditorController::switchDashesClicked()
{
	SwitchDashesUndoStep* undo_step = new SwitchDashesUndoStep(map);
	MapLayer* layer = map->getCurrentLayer();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line)
		{
			(*it)->reverse();
			(*it)->update(true);
			
			undo_step->addObject(layer->findObjectIndex(*it));
		}
	}
	
	map->setObjectsDirty();
	map->objectUndoManager().addNewUndoStep(undo_step);
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
		map->disconnect(this);
		map->disconnect(symbol_widget);
	}
	
	this->map = map;
	if (create_new_map_view)
		main_view = new MapView(map);
	
	connect(&map->objectUndoManager(), SIGNAL(undoStepAvailabilityChanged()), this, SLOT(undoStepAvailabilityChanged()));
	connect(map, SIGNAL(selectedObjectsChanged()), this, SLOT(selectedObjectsChanged()));
	connect(map, SIGNAL(templateAdded(int,Template*)), this, SLOT(templateAdded(int,Template*)));
	connect(map, SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
	if (symbol_widget)
		connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), symbol_widget, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	
	if (window)
	{
		undoStepAvailabilityChanged();
		selectedObjectsChanged();
	}
}

// ### EditorDockWidget ###

EditorDockWidget::EditorDockWidget(const QString title, QAction* action, QWidget* parent): QDockWidget(title, parent), action(action)
{
}
void EditorDockWidget::setChild(EditorDockWidgetChild* child)
{
	this->child = child;
	setWidget(child);
}
void EditorDockWidget::closeEvent(QCloseEvent* event)
{
	if (action)
		action->setChecked(false);
	child->closed();
    QDockWidget::closeEvent(event);
}

// ### MapEditorTool ###

const int MapEditorTool::click_tolerance = 5;
const QRgb MapEditorTool::inactive_color = qRgb(0, 0, 255);
const QRgb MapEditorTool::active_color = qRgb(255, 150, 0);
const QRgb MapEditorTool::selection_color = qRgb(210, 0, 229);

MapEditorTool::MapEditorTool(MapEditorController* editor, Type type, QAction* tool_button): QObject(NULL), tool_button(tool_button), type(type), editor(editor)
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
