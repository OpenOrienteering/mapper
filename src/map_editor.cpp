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
#include "map_undo.h"
#include "map_dialog_scale.h"
#include "print_dock_widget.h"
#include "color_dock_widget.h"
#include "symbol_dock_widget.h"
#include "template_dock_widget.h"
#include "template.h"
#include "paint_on_template.h"
#include "gps_coordinates.h"
#include "symbol.h"
#include "tool_draw_point.h"
#include "tool_draw_path.h"
#include "tool_draw_text.h"
#include "tool_edit.h"
#include "util.h"
#include "tool_cut.h"
#include "tool_cut_hole.h"
#include "tool_rotate.h"
#include "object_text.h"

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
	
	toolbar_view = NULL;
	toolbar_drawing = NULL;
	toolbar_editing = NULL;
	print_dock_widget = NULL;
	color_dock_widget = NULL;
	symbol_dock_widget = NULL;
	template_dock_widget = NULL;

    actionsById[""] = new QAction(this); // dummy action
}
MapEditorController::~MapEditorController()
{
	delete toolbar_view;
	delete toolbar_drawing;
	delete toolbar_editing;
	delete print_dock_widget;
	delete color_dock_widget;
	delete symbol_dock_widget;
	delete template_dock_widget;
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
		setTool(new EditTool(this, edit_tool_act, symbol_widget));
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
		return new EditTool(this, edit_tool_act, symbol_widget);
	else if (symbol->getType() == Symbol::Point)
		return new DrawPointTool(this, draw_point_act, symbol_widget);
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined)
		return new DrawPathTool(this, draw_path_act, symbol_widget, true);
	else if (symbol->getType() == Symbol::Text)
		return new DrawTextTool(this, draw_text_act, symbol_widget);
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
		main_view = NULL;
	}
	
	return result;
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
	
    // Create menu and toolbar together, so actions can be inserted into one or both
	if (mode == MapEditor)
        createMenuAndToolbars();
	
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
	
	// Auto-select the edit tool
	if (mode == MapEditor)
	{
		edit_tool_act->setChecked(true);
		setEditTool();
	}
}

QAction *MapEditorController::newAction(const char *id, const QString &tr_text, QObject *receiver, const char *slot, const char *icon, const QString &tr_tip)
{
    QAction *action = new QAction(icon ? QIcon(QString(":/images/%1").arg(icon)) : QIcon(), tr_text, this);
    if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
    if (receiver) QObject::connect(action, SIGNAL(triggered()), receiver, slot);
    actionsById[id] = action;
    return action;
}

QAction *MapEditorController::newCheckAction(const char *id, const QString &tr_text, QObject *receiver, const char *slot, const char *icon, const QString &tr_tip)
{
    QAction *action = new QAction(icon ? QIcon(QString(":/images/%1").arg(icon)) : QIcon(), tr_text, this);
    action->setCheckable(true);
    if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
    if (receiver) QObject::connect(action, SIGNAL(triggered(bool)), receiver, slot);
    actionsById[id] = action;
    return action;
}

QAction *MapEditorController::findAction(const char *id)
{
    if (!actionsById.contains(id)) return actionsById[""];
    else return actionsById[id];
}

void MapEditorController::assignKeyboardShortcuts()
{
    // Standard keyboard shortcuts
    findAction("print")->setShortcut(QKeySequence::Print);
    findAction("undo")->setShortcut(QKeySequence::Undo);
    findAction("redo")->setShortcut(QKeySequence::Redo);
    findAction("cut")->setShortcut(QKeySequence::Cut);
    findAction("copy")->setShortcut(QKeySequence::Copy);
    findAction("paste")->setShortcut(QKeySequence::Paste);

    // Custom keyboard shortcuts
	findAction("zoomin")->setShortcut(QKeySequence("F7"));
	findAction("zoomout")->setShortcut(QKeySequence("F8"));
	findAction("fullscreen")->setShortcut(QKeySequence("F11"));
	
	findAction("editobjects")->setShortcut(QKeySequence("E"));
	findAction("drawpoint")->setShortcut(QKeySequence("S"));
	findAction("drawpath")->setShortcut(QKeySequence("P"));
	findAction("drawtext")->setShortcut(QKeySequence("T"));
	
    findAction("duplicate")->setShortcut(QKeySequence("D"));
    findAction("switchdashes")->setShortcut(QKeySequence("Ctrl+D"));
	findAction("connectpaths")->setShortcut(QKeySequence("C"));
	findAction("rotateobjects")->setShortcut(QKeySequence("R"));
	findAction("cutobject")->setShortcut(QKeySequence("K"));
	findAction("cuthole")->setShortcut(QKeySequence("H"));
}

void MapEditorController::createMenuAndToolbars()
{
    // Define all the actions, saving them into variables as necessary. Can also get them by ID.
    print_act = newAction("print", tr("Print..."), this, SLOT(printClicked()), "print.png");
    undo_act = newAction("undo", tr("Undo"), this, SLOT(undo()), "undo.png", tr("Undo the last step"));
    redo_act = newAction("redo", tr("Redo"), this, SLOT(redo()), "redo.png", tr("Redo the last step"));
    /*QAction* cut_act = */newAction("cut", tr("Cu&t"), this, SLOT(cut()), "cut.png");
    /*QAction* copy_act = */newAction("copy", tr("C&opy"), this, SLOT(copy()), "copy.png");
    /*QAction* paste_act = */newAction("paste", tr("&Paste"), this, SLOT(paste()), "paste");
    QAction* zoom_in_act = newAction("zoomin", tr("Zoom in"), this, SLOT(zoomIn()), "view-zoom-in.png"); // F7
    QAction* zoom_out_act = newAction("zoomout", tr("Zoom out"), this, SLOT(zoomOut()), "view-zoom-out.png"); // F8
    QAction* fullscreen_act = newAction("fullscreen", tr("Toggle fullscreen mode"), window, SLOT(toggleFullscreenMode()));
    QAction* custom_zoom_act = newAction("setzoom", tr("Set custom zoom factor..."), this, SLOT(setCustomZoomFactorClicked()));
    symbol_window_act = newCheckAction("symbolwindow", tr("Symbol window"), this, SLOT(showSymbolWindow(bool)), "window-new.png", tr("Show/Hide the symbol window"));
    color_window_act = newCheckAction("colorwindow", tr("Color window"), this, SLOT(showColorWindow(bool)), "window-new.png", tr("Show/Hide the color window"));
    /*QAction *load_symbols_from_act = */newAction("loadsymbols", tr("Load symbols from..."), this, SLOT(loadSymbolsFromClicked()), NULL, tr("Replace the symbols with those from another map file"));
    /*QAction *load_colors_from_act = */newAction("loadcolors", tr("Load colors from..."), this, SLOT(loadColorsFromClicked()), NULL, tr("Replace the colors with those from another map file"));
    QAction *scale_all_symbols_act = newAction("scaleall", tr("Scale all symbols..."), this, SLOT(scaleAllSymbolsClicked()), NULL, tr("Scale the whole symbol set"));
    QAction *scale_map_act = newAction("scalemap", tr("Change map scale..."), this, SLOT(scaleMapClicked()), NULL, tr("Change the map scale and adjust map objects and symbol sizes"));
    template_window_act = newCheckAction("templatewindow", tr("Template setup window"), this, SLOT(showTemplateWindow(bool)), "window-new", tr("Show/Hide the template window"));
    //QAction* template_config_window_act = newCheckAction("templateconfigwindow", tr("Template configurations window"), this, SLOT(showTemplateConfigurationsWindow(bool)), "window-new", tr("Show/Hide the template configurations window"));
    //QAction* template_visibilities_window_act = newCheckAction("templatevisibilitieswindow", tr("Template visibilities window"), this, SLOT(showTemplateVisbilitiesWindow(bool)), "window-new", tr("Show/Hide the template visibilities window"));
    QAction* open_template_act = newAction("opentemplate", tr("Open template..."), this, SLOT(openTemplateClicked()));
    QAction* edit_gps_projection_parameters_act = newAction("gpsproj", tr("Edit projection parameters..."), this, SLOT(editGPSProjectionParameters()));
    QAction* show_all_act = newAction("showall", tr("Show whole map"), this, SLOT(showWholeMap()), "view-show-all.png");
    edit_tool_act = newCheckAction("editobjects", tr("Edit objects"), this, SLOT(editToolClicked(bool)), "tool-edit.png");
    draw_point_act = newCheckAction("drawpoint", tr("Set point objects"), this, SLOT(drawPointClicked(bool)), "draw-point.png");
    draw_path_act = newCheckAction("drawpath", tr("Draw paths"), this, SLOT(drawPathClicked(bool)), "draw-path.png");
    draw_text_act = newCheckAction("drawtext", tr("Write text"), this, SLOT(drawTextClicked(bool)), "draw-text.png");
    duplicate_act = newAction("duplicate", tr("Duplicate"), this, SLOT(duplicateClicked()), "tool-duplicate.png"); // D
    switch_symbol_act = newAction("switchsymbol", tr("Switch symbol"), this, SLOT(switchSymbolClicked()), "tool-switch-symbol.png");
    fill_border_act = newAction("fillborder", tr("Fill / Create border"), this, SLOT(fillBorderClicked()), "tool-fill-border.png");
    switch_dashes_act = newAction("switchdashes", tr("Switch dash direction"), this, SLOT(switchDashesClicked()), "tool-switch-dashes"); // Ctrl+D
	connect_paths_act = newAction("connectpaths", tr("Connect paths"), this, SLOT(connectPathsClicked()), "tool-connect-paths.png");
	cut_tool_act = newCheckAction("cutobject", tr("Cut object"), this, SLOT(cutClicked(bool)), "tool-cut.png");
	cut_hole_act = newCheckAction("cuthole", tr("Cut holes"), this, SLOT(cutHoleClicked(bool)), "tool-cut-hole.png");
    rotate_act = newCheckAction("rotateobjects", tr("Rotate object(s)"), this, SLOT(rotateClicked(bool)), "tool-rotate.png");

    // Refactored so we can do custom key bindings in the future
    assignKeyboardShortcuts();

    // Extend file menu
	QMenu* file_menu = window->getFileMenu();
	file_menu->insertAction(window->getCloseAct(), print_act);
	file_menu->insertSeparator(window->getCloseAct());
		
    // Edit menu
	QMenu* edit_menu = window->menuBar()->addMenu(tr("&Edit"));
	edit_menu->addAction(undo_act);
	edit_menu->addAction(redo_act);
    edit_menu->addSeparator();
    //edit_menu->addAction(cut_act);
    //edit_menu->addAction(copy_act);
    //edit_menu->addAction(paste_act);
	
	// View menu
	QMenu* view_menu = window->menuBar()->addMenu(tr("&View"));
	view_menu->addAction(zoom_in_act);
	view_menu->addAction(zoom_out_act);
    view_menu->addAction(show_all_act);
    view_menu->addAction(custom_zoom_act);
    view_menu->addSeparator();
    view_menu->addAction(fullscreen_act);

    // Tools menu
    QMenu *tools_menu = window->menuBar()->addMenu(tr("&Tools"));
    tools_menu->addAction(edit_tool_act);
    tools_menu->addAction(draw_point_act);
    tools_menu->addAction(draw_path_act);
    tools_menu->addAction(draw_text_act);
    tools_menu->addAction(duplicate_act);
    tools_menu->addAction(switch_symbol_act);
    tools_menu->addAction(fill_border_act);
    tools_menu->addAction(switch_dashes_act);
	tools_menu->addAction(connect_paths_act);
	tools_menu->addAction(cut_tool_act);
	tools_menu->addAction(cut_hole_act);
	tools_menu->addAction(rotate_act);
	
	// Symbols menu
    QMenu* symbols_menu = window->menuBar()->addMenu(tr("Sy&mbols"));
    symbols_menu->addAction(symbol_window_act);
	symbols_menu->addAction(color_window_act);
	symbols_menu->addSeparator();
	/*symbols_menu->addAction(load_symbols_from_act);
	symbols_menu->addAction(load_colors_from_act);*/
	symbols_menu->addAction(scale_all_symbols_act);
	
	// Map menu
	QMenu* map_menu = window->menuBar()->addMenu(tr("M&ap"));
	map_menu->addAction(scale_map_act);
	
	// Templates menu
	QMenu* template_menu = window->menuBar()->addMenu(tr("&Templates"));
	template_menu->addAction(template_window_act);
	/*template_menu->addAction(template_config_window_act);
	template_menu->addAction(template_visibilities_window_act);*/
	template_menu->addSeparator();
	template_menu->addAction(open_template_act);
	
	// GPS menu
	QMenu* gps_menu = window->menuBar()->addMenu(tr("&GPS"));
	gps_menu->addAction(edit_gps_projection_parameters_act);



#ifndef Q_WS_MAC
    // disable toolbars on OS X for the moment - any call to addToolBar() appears to
    // torpedo the main window. The app is still running, but the window actually disappears
    // and all menu items contributed to the menu bar disappear, leaving only the Application
    // menu, (Quit, Services, etc.)

    // This is the main reason I refactored the action setup; made it easier to ensure all
    // actions made it into the menu system where I could use them.

    // View toolbar
    toolbar_view = window->addToolBar("View");
    toolbar_view->addAction(show_all_act);

	// Drawing toolbar
	toolbar_drawing = window->addToolBar(tr("Drawing"));
	toolbar_drawing->addAction(edit_tool_act);
    toolbar_drawing->addAction(draw_point_act);
	toolbar_drawing->addAction(draw_path_act);
    toolbar_drawing->addAction(draw_text_act);

	toolbar_drawing->addSeparator();

    // Leave this for the time being...
    paint_on_template_act = new QAction(QIcon(":/images/pencil.png"), tr("Paint on template"), this);
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
	toolbar_editing = window->addToolBar(tr("Editing"));
    toolbar_editing->addAction(duplicate_act);
    toolbar_editing->addAction(switch_symbol_act);
	toolbar_editing->addAction(fill_border_act);
    toolbar_editing->addAction(switch_dashes_act);
	toolbar_editing->addAction(connect_paths_act);
	toolbar_editing->addAction(cut_tool_act);
	toolbar_editing->addAction(cut_hole_act);
	toolbar_editing->addAction(rotate_act);
#endif

}
void MapEditorController::detach()
{
	QWidget* widget = window->centralWidget();
	window->setCentralWidget(NULL);
	delete widget;
	
	delete statusbar_zoom_label;
	delete statusbar_cursorpos_label;
}

void MapEditorController::keyPressEvent(QKeyEvent* event)
{
    map_widget->keyPressed(event);
}
void MapEditorController::keyReleaseEvent(QKeyEvent* event)
{
	map_widget->keyReleased(event);
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
		print_dock_widget = new EditorDockWidget(tr("Print or Export"), print_act, this, window);
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
	UndoStep* generic_step = redo ? map->objectUndoManager().getLastRedoStep() : map->objectUndoManager().getLastUndoStep();
	MapLayer* affected_layer = NULL;
	std::vector<Object*> affected_objects;
	
	if (generic_step->getType() == UndoStep::CombinedUndoStepType)
	{
		CombinedUndoStep* combined_step = reinterpret_cast<CombinedUndoStep*>(generic_step);
		std::set<Object*> affected_objects_set;
		for (int i = 0; i < combined_step->getNumSubSteps(); ++i)
		{
			std::vector<Object*> affected_objects_temp;
			MapUndoStep* map_step = reinterpret_cast<MapUndoStep*>(combined_step->getSubStep(i));
			if (!affected_layer)
				affected_layer = map->getLayer(map_step->getLayer());
			else
				assert(affected_layer == map->getLayer(map_step->getLayer()));
			map_step->getAffectedOutcome(affected_objects_temp);
			
			for (int o = 0; o < (int)affected_objects_temp.size(); ++o)
				affected_objects_set.insert(affected_objects_temp[o]);	// NOTE: This does only work as long as there is no combined step which combines editing objects with deleting them later
		}
		for (std::set<Object*>::const_iterator it = affected_objects_set.begin(); it != affected_objects_set.end(); ++it)
			affected_objects.push_back(*it);
	}
	else
	{
		MapUndoStep* map_step = reinterpret_cast<MapUndoStep*>(generic_step);
		affected_layer = map->getLayer(map_step->getLayer());
		map_step->getAffectedOutcome(affected_objects);
	}
	
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
		rectIncludeSafe(object_rect, affected_objects[i]->getExtent());
		map->addObjectToSelection(affected_objects[i], i == (int)affected_objects.size() - 1);
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
void MapEditorController::setCustomZoomFactorClicked()
{
	bool ok;
	double factor = QInputDialog::getDouble(window, tr("Set custom zoom factor"), tr("Zoom factor:"), main_view->getZoom(), MapView::zoom_out_limit, MapView::zoom_in_limit, 3, &ok);
	if (!ok || factor == main_view->getZoom())
		return;
	
	main_view->setZoom(factor);
}

void MapEditorController::showSymbolWindow(bool show)
{
	if (symbol_dock_widget)
		symbol_dock_widget->setVisible(!symbol_dock_widget->isVisible());
	else
	{
		symbol_dock_widget = new EditorDockWidget(tr("Symbols"), symbol_window_act, this, window);
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
		color_dock_widget = new EditorDockWidget(tr("Colors"), color_window_act, this, window);
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
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, this, window);
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
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, this, window);
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
	
	draw_point_act->setEnabled(type == Symbol::Point && !symbol->isHidden());
	draw_point_act->setStatusTip(tr("Place point objects on the map.") + (draw_point_act->isEnabled() ? "" : (" " + tr("Select a point symbol to be able to use this tool."))));
	draw_path_act->setEnabled((type == Symbol::Line || type == Symbol::Area || type == Symbol::Combined) && !symbol->isHidden());
	draw_path_act->setStatusTip(tr("Draw polygonal and curved lines.") + (draw_path_act->isEnabled() ? "" : (" " + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_text_act->setEnabled(type == Symbol::Text && !symbol->isHidden());
	draw_text_act->setStatusTip(tr("Write text on the map.") + (draw_text_act->isEnabled() ? "" : (" " + tr("Select a text symbol to be able to use this tool."))));
	
	selectedSymbolsOrObjectsChanged();
}
void MapEditorController::selectedObjectsChanged()
{
	if (mode != MapEditor)
		return;
	
	bool have_selection = map->getNumSelectedObjects() > 0;
	bool single_object_selected = map->getNumSelectedObjects() == 1;
	bool have_line = false;
	bool have_area = false;
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line)
			have_line = true;
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Area)
			have_area = true;
	}
	
	duplicate_act->setEnabled(have_selection);
	duplicate_act->setStatusTip(tr("Duplicate the selected object(s).") + (duplicate_act->isEnabled() ? "" : (" " + tr("Select at least one object to activate this tool."))));
	switch_dashes_act->setEnabled(have_line);
	switch_dashes_act->setStatusTip(tr("Switch the direction of symbols on line objects.") + (switch_dashes_act->isEnabled() ? "" : (" " + tr("Select at least one line object to activate this tool."))));
	connect_paths_act->setEnabled(have_line);
	connect_paths_act->setStatusTip(tr("Connect endpoints of paths which are close together.") + (connect_paths_act->isEnabled() ? "" : (" " + tr("Select at least one line object to activate this tool."))));
	cut_tool_act->setEnabled(have_area || have_line);
	cut_tool_act->setStatusTip(tr("Cut the selected object(s) into smaller parts.") + (cut_tool_act->isEnabled() ? "" : (" " + tr("Select at least one line or area object to activate this tool."))));
	cut_hole_act->setEnabled(single_object_selected && have_area);
	cut_hole_act->setStatusTip(tr("Cut a hole into the selected area object.") + (cut_hole_act->isEnabled() ? "" : (" " + tr("Select a single area object to activate this tool."))));
	rotate_act->setEnabled(have_selection);
	rotate_act->setStatusTip(tr("Rotate the selected object(s).") + (rotate_act->isEnabled() ? "" : (" " + tr("Select at least one object to activate this tool."))));
	
	selectedSymbolsOrObjectsChanged();
}
void MapEditorController::selectedSymbolsOrObjectsChanged()
{
	//Symbol::Type single_symbol_type = Symbol::NoSymbol;
	Symbol* single_symbol = symbol_widget ? symbol_widget->getSingleSelectedSymbol() : NULL;
	//if (single_symbol)
	//	single_symbol_type = single_symbol->getType();
	
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
	setTool(checked ? new DrawPathTool(this, draw_path_act, symbol_widget, true) : NULL);
}
void MapEditorController::drawTextClicked(bool checked)
{
	setTool(checked ? new DrawTextTool(this, draw_text_act, symbol_widget) : NULL);
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
			PathObject* path = reinterpret_cast<PathObject*>(*it);
			path->reverse();
			(*it)->update(true);
			
			undo_step->addObject(layer->findObjectIndex(*it));
		}
	}
	
	map->setObjectsDirty();
	map->objectUndoManager().addNewUndoStep(undo_step);
}
void MapEditorController::connectPathsClicked()
{
	std::vector<Object*> objects;
	std::vector<Object*> undo_objects;
	std::vector<Object*> deleted_objects;
	
	const float close_distance_sq = 0.35f * 0.35f;	// TODO: Should this depend on the width of the lines? But how to determine a sensible width for lines consisting only of objects?
	
	// Collect all objects in question
	objects.reserve(map->getNumSelectedObjects());
	undo_objects.reserve(map->getNumSelectedObjects());
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line && (*it)->getType() == Object::Path)
		{
			objects.push_back(*it);
			undo_objects.push_back(NULL);
		}
	}
	
	AddObjectsUndoStep* add_step = NULL;
	MapLayer* layer = map->getCurrentLayer();
	
	// Process all objects
	for (int i = 0; i < (int)objects.size(); ++i)
	{
		PathObject* a = reinterpret_cast<PathObject*>(objects[i]);
		a->update(false);
		
		// Loop over all parts and check if they should be closed
		int num_parts = a->getNumParts();
		bool all_paths_closed = true;
		for (int part_index = 0; part_index < num_parts; ++part_index)
		{
			PathObject::PathPart& part = a->getPart(part_index);
			if (!part.isClosed())
			{
				if (a->getCoordinate(part.start_index).lengthSquaredTo(a->getCoordinate(part.end_index)) <= close_distance_sq)
				{
					undo_objects[i] = a->duplicate();
					part.connectEnds();
				}
				else
					all_paths_closed = false;
			}
		}
		if (all_paths_closed)
			continue;
		
		// Check for other endpoints which are close enough together to be closed
		for (int k = i + 1; k < (int)objects.size(); ++k)
		{
			PathObject* b = reinterpret_cast<PathObject*>(objects[k]);
			if (b->getSymbol() != a->getSymbol())
				continue;
			
			if (a->canBeConnected(b, close_distance_sq))
			{
				// Duplicate objects if necessary and append b to a
				if (!undo_objects[i])
					undo_objects[i] = a->duplicate();
				if (!undo_objects[k])
					undo_objects[k] = b->duplicate();
				assert(a->connectIfClose(b, close_distance_sq));
				
				// Were all parts of b deleted?
				//if (b->getNumParts() == 0)
				//{
					// Create an add step for b
					int b_index = layer->findObjectIndex(b);
					deleted_objects.push_back(b);
					if (!add_step)
						add_step = new AddObjectsUndoStep(map);
					add_step->addObject(b_index, undo_objects[k]);
					undo_objects[k] = NULL;
					
					// Delete b from the active list
					objects.erase(objects.begin() + k);
					undo_objects.erase(undo_objects.begin() + k);
				//}
				
				k = i;	// have to check other objects again
			}
		}
	}
	
	// Create undo step?
	ReplaceObjectsUndoStep* replace_step = NULL;
	for (int i = 0; i < (int)undo_objects.size(); ++i)
	{
		if (undo_objects[i])
		{
			// The object was changed, update the new version
			objects[i]->update(true);
			
			// Add the old version to the undo step
			if (!replace_step)
				replace_step = new ReplaceObjectsUndoStep(map);
			replace_step->addObject(layer->findObjectIndex(objects[i]), undo_objects[i]);
		}
	}
	
	if (add_step)
	{
		for (int i = 0; i < (int)deleted_objects.size(); ++i)
		{
			map->removeObjectFromSelection(deleted_objects[i], false);
			map->getCurrentLayer()->deleteObject(deleted_objects[i], false);
		}
	}
	
	if (add_step || replace_step)
	{
		CombinedUndoStep* undo_step = new CombinedUndoStep((void*)map);
		if (add_step)
			undo_step->addSubStep(add_step);
		if (replace_step)
			undo_step->addSubStep(replace_step);
		map->objectUndoManager().addNewUndoStep(undo_step);
	}
	
	map->emitSelectionChanged();
}
void MapEditorController::cutClicked(bool checked)
{
	setTool(checked ? new CutTool(this, cut_tool_act) : NULL);
}
void MapEditorController::cutHoleClicked(bool checked)
{
	setTool(checked ? new CutHoleTool(this, cut_hole_act) : NULL);
}
void MapEditorController::rotateClicked(bool checked)
{
	setTool(checked ? new RotateTool(this, rotate_act) : NULL);
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

EditorDockWidget::EditorDockWidget(const QString title, QAction* action, MapEditorController* editor, QWidget* parent): QDockWidget(title, parent), action(action), editor(editor)
{
}
void EditorDockWidget::setChild(EditorDockWidgetChild* child)
{
	this->child = child;
	setWidget(child);
}
bool EditorDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && editor->getWindow()->areShortcutsDisabled())
		event->accept();
    return QDockWidget::event(event);
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
QImage* MapEditorTool::point_handles = NULL;

MapEditorTool::MapEditorTool(MapEditorController* editor, Type type, QAction* tool_button): QObject(NULL), tool_button(tool_button), type(type), editor(editor)
{
}
MapEditorTool::~MapEditorTool()
{
	if (tool_button)
		tool_button->setChecked(false);
}

void MapEditorTool::loadPointHandles()
{
	if (!point_handles)
		point_handles = new QImage(":/images/point-handles.png");
}

void MapEditorTool::setStatusBarText(const QString& text)
{
	editor->getWindow()->setStatusBarText(text);
}

void MapEditorTool::startEditingSelection(RenderableVector& old_renderables, std::vector<Object*>* undo_duplicates)
{
	Map* map = editor->getMap();
	
	// Temporarily take the edited objects out of the map so their map renderables are not updated, and make duplicates of them before for the edit step
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if (undo_duplicates)
			undo_duplicates->push_back((*it)->duplicate());
		
		(*it)->setMap(NULL);
		
		// Cache old renderables until the object is inserted into the map again
		old_renderables.reserve(old_renderables.size() + (*it)->getNumRenderables());
		RenderableVector::const_iterator rit_end = (*it)->endRenderables();
		for (RenderableVector::const_iterator rit = (*it)->beginRenderables(); rit != rit_end; ++rit)
			old_renderables.push_back(*rit);
		(*it)->takeRenderables();
	}
	
	editor->setEditingInProgress(true);
}
void MapEditorTool::finishEditingSelection(RenderableContainer& renderables, RenderableVector& old_renderables, bool create_undo_step, std::vector<Object*>* undo_duplicates, bool delete_objects)
{
	ReplaceObjectsUndoStep* undo_step = create_undo_step ? new ReplaceObjectsUndoStep(editor->getMap()) : NULL;
	
	int i = 0;
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		if (!delete_objects)
		{
			(*it)->setMap(editor->getMap());
			(*it)->update(true);
		}
		
		if (create_undo_step)
			undo_step->addObject(*it, undo_duplicates->at(i));
		else
			delete undo_duplicates->at(i);
		++i;
	}
	renderables.clear();
	deleteOldSelectionRenderables(old_renderables, true);
	
	undo_duplicates->clear();
	if (create_undo_step)
		editor->getMap()->objectUndoManager().addNewUndoStep(undo_step);
	
	editor->setEditingInProgress(false);
}
void MapEditorTool::updateSelectionEditPreview(RenderableContainer& renderables)
{
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		renderables.removeRenderablesOfObject(*it, false);
		(*it)->update(true);
		renderables.insertRenderablesOfObject(*it);
	}
}
void MapEditorTool::deleteOldSelectionRenderables(RenderableVector& old_renderables, bool set_area_dirty)
{
	int size = old_renderables.size();
	for (int i = 0; i < size; ++i)
	{
		if (set_area_dirty)
			editor->getMap()->setObjectAreaDirty(old_renderables[i]->getExtent());
		delete old_renderables[i];
	}
	old_renderables.clear();
}

void MapEditorTool::includeControlPointRect(QRectF& rect, Object* object, QPointF* box_text_handles)
{
	if (object->getType() == Object::Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		int size = path->getCoordinateCount();
		for (int i = 0; i < size; ++i)
			rectInclude(rect, path->getCoordinate(i).toQPointF());
	}
	else if (object->getType() == Object::Text)
	{
		TextObject* text_object = reinterpret_cast<TextObject*>(object);
		if (text_object->hasSingleAnchor())
			rectInclude(rect, text_object->getAnchorCoordF());
		else
		{
			for (int i = 0; i < 4; ++i)
				rectInclude(rect, box_text_handles[i]);
		}
	}
}
void MapEditorTool::drawPointHandles(int hover_point, QPainter* painter, Object* object, MapWidget* widget)
{
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		drawPointHandle(painter, widget->mapToViewport(point->getCoordF()), NormalHandle, hover_point == 0);
	}
	else if (object->getType() == Object::Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(object);
		if (text->hasSingleAnchor())
			drawPointHandle(painter, widget->mapToViewport(text->getAnchorCoordF()), NormalHandle, hover_point == 0);
		else
		{
			QPointF box_text_handles[4];
			calculateBoxTextHandles(box_text_handles);
			for (int i = 0; i < 4; ++i)
				drawPointHandle(painter, widget->mapToViewport(box_text_handles[i]), NormalHandle, hover_point == i);
		}
	}
	else if (object->getType() == Object::Path)
	{
		painter->setBrush(Qt::NoBrush); // for handle lines
		
		PathObject* path = reinterpret_cast<PathObject*>(object);
		
		int num_parts = path->getNumParts();
		for (int part_index = 0; part_index < num_parts; ++part_index)
		{
			PathObject::PathPart& part = path->getPart(part_index);
			bool have_curve = part.isClosed() && part.getNumCoords() > 3 && path->getCoordinate(part.end_index - 3).isCurveStart();
			
			for (int i = part.start_index; i <= part.end_index; ++i)
			{
				MapCoord coord = path->getCoordinate(i);
				if (coord.isClosePoint())
					continue;
				QPointF point = widget->mapToViewport(coord);
				
				if (have_curve)
				{
					int curve_index = (i == part.start_index) ? (part.end_index - 1) : (i - 1);
					QPointF curve_handle = widget->mapToViewport(path->getCoordinate(curve_index));
					drawCurveHandleLine(painter, point, curve_handle, hover_point == i);
					drawPointHandle(painter, curve_handle, CurveHandle, hover_point == i || hover_point == curve_index);
					have_curve = false;
				}
				
				/*if ((i == part.start_index && !part->isClosed()) || (i > part.start_index && path->getCoordinate(part.end_index-1).isHolePoint()))
					drawPointHandle(painter, point, StartHandle, widget);
				else if ((i == part.end_index - 1 && !part->isClosed()) || coord.isHolePoint())
					drawPointHandle(painter, point, EndHandle, widget);
				else*/
					drawPointHandle(painter, point, coord.isDashPoint() ? DashHandle : NormalHandle, hover_point == i);
				
				if (coord.isCurveStart())
				{
					QPointF curve_handle = widget->mapToViewport(path->getCoordinate(i+1));
					drawCurveHandleLine(painter, point, curve_handle, hover_point == i);
					drawPointHandle(painter, curve_handle, CurveHandle, hover_point == i || hover_point == i + 1);
					i += 2;
					have_curve = true;
				}
			}
		}
	}
	else
		assert(false);
}
void MapEditorTool::drawPointHandle(QPainter* painter, QPointF point, MapEditorTool::PointHandleType type, bool active)
{
	painter->drawImage(qRound(point.x()) - 5, qRound(point.y()) - 5, *point_handles, (int)type * 11, active ? 11 : 0, 11, 11);
}
void MapEditorTool::drawCurveHandleLine(QPainter* painter, QPointF point, QPointF curve_handle, bool active)
{
	const float handle_radius = 3;
	painter->setPen(active ? active_color : inactive_color);
	
	QPointF to_handle = curve_handle - point;
	float to_handle_len = to_handle.x()*to_handle.x() + to_handle.y()*to_handle.y();
	if (to_handle_len > 0.00001f)
	{
		to_handle_len = sqrt(to_handle_len);
		QPointF change = to_handle * (handle_radius / to_handle_len);
		
		point += change;
		curve_handle -= change;
	}
	
	painter->drawLine(point, curve_handle);
}
bool MapEditorTool::calculateBoxTextHandles(QPointF* out)
{
	Map* map = editor->getMap();
	Object* single_selected_object = (map->getNumSelectedObjects() == 1) ? *map->selectedObjectsBegin() : NULL;
	if (single_selected_object && single_selected_object->getType() == Object::Text)
	{
		TextObject* text_object = reinterpret_cast<TextObject*>(single_selected_object);
		if (!text_object->hasSingleAnchor())
		{
			TextObject* text_object = reinterpret_cast<TextObject*>(*editor->getMap()->selectedObjectsBegin());
			
			QTransform transform;
			transform.rotate(-text_object->getRotation() * 180 / M_PI);
			out[0] = transform.map(QPointF(text_object->getBoxWidth() / 2, -text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			out[1] = transform.map(QPointF(text_object->getBoxWidth() / 2, text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			out[2] = transform.map(QPointF(-text_object->getBoxWidth() / 2, text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			out[3] = transform.map(QPointF(-text_object->getBoxWidth() / 2, -text_object->getBoxHeight() / 2)) + text_object->getAnchorCoordF().toQPointF();
			return true;
		}
	}
	return false;
}

int MapEditorTool::findHoverPoint(QPointF cursor, Object* object, bool include_curve_handles, QPointF* box_text_handles, QRectF* selection_extent, MapWidget* widget)
{
	const float click_tolerance_squared = click_tolerance * click_tolerance;
	
	// Check object
	if (object)
	{
		if (object->getType() == Object::Point)
		{
			PointObject* point = reinterpret_cast<PointObject*>(object);
			if (distanceSquared(widget->mapToViewport(point->getCoordF()), cursor) <= click_tolerance_squared)
				return 0;
		}
		else if (object->getType() == Object::Text)
		{
			TextObject* text = reinterpret_cast<TextObject*>(object);
			if (text->hasSingleAnchor() && distanceSquared(widget->mapToViewport(text->getAnchorCoordF()), cursor) <= click_tolerance_squared)
				return 0;
			else if (!text->hasSingleAnchor())
			{
				for (int i = 0; i < 4; ++i)
				{
					if (box_text_handles && distanceSquared(widget->mapToViewport(box_text_handles[i]), cursor) <= click_tolerance_squared)
						return i;
				}
			}
		}
		else if (object->getType() == Object::Path)
		{
			PathObject* path = reinterpret_cast<PathObject*>(object);
			int size = path->getCoordinateCount();
			
			for (int i = size - 1; i >= 0; --i)
			{
				if (!path->getCoordinate(i).isClosePoint() &&
					distanceSquared(widget->mapToViewport(path->getCoordinate(i)), cursor) <= click_tolerance_squared)
					return i;
				if (!include_curve_handles && i >= 3 && path->getCoordinate(i - 3).isCurveStart())
					i -= 2;
			}
		}
	}
	
	// Check bounding box
	if (!selection_extent)
		return -2;
	QRectF selection_extent_viewport = widget->mapToViewport(*selection_extent);
	if (cursor.x() < selection_extent_viewport.left() - click_tolerance) return -2;
	if (cursor.y() < selection_extent_viewport.top() - click_tolerance) return -2;
	if (cursor.x() > selection_extent_viewport.right() + click_tolerance) return -2;
	if (cursor.y() > selection_extent_viewport.bottom() + click_tolerance) return -2;
	if (cursor.x() > selection_extent_viewport.left() + click_tolerance &&
		cursor.y() > selection_extent_viewport.top() + click_tolerance &&
		cursor.x() < selection_extent_viewport.right() - click_tolerance &&
		cursor.y() < selection_extent_viewport.bottom() - click_tolerance) return -2;
	return -1;
}

#include "map_editor.moc"
