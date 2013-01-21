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


#include "map_editor.h"

#include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QSignalMapper>

#include "color_dock_widget.h"
#include "file_format_registry.h"
#include "georeferencing.h"
#include "georeferencing_dialog.h"
#include "map.h"
#include "map_dialog_rotate.h"
#include "map_dialog_scale.h"
#include "map_editor_activity.h"
#include "map_grid.h"
#include "map_undo.h"
#include "map_widget.h"
#include "gui/main_window.h"
#include "gui/print_widget.h"
#include "gui/widgets/measure_widget.h"
#include "object_operations.h"
#include "object_text.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_area.h"
#include "symbol_dialog_replace.h"
#include "symbol_dock_widget.h"
#include "symbol_point.h"
#include "template.h"
#include "template_dialog_reopen.h"
#include "template_dock_widget.h"
#include "template_gps.h"
#include "template_position_dock_widget.h"
#include "template_tool_paint.h"
#include "tool.h"
#include "tool_boolean.h"
#include "tool_cut.h"
#include "tool_cut_hole.h"
#include "tool_draw_circle.h"
#include "tool_draw_path.h"
#include "tool_draw_point.h"
#include "tool_draw_rectangle.h"
#include "tool_draw_text.h"
#include "tool_edit_point.h"
#include "tool_edit_line.h"
#include "tool_pan.h"
#include "tool_rotate.h"
#include "tool_rotate_pattern.h"
#include "tool_scale.h"
#include "util.h"

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
	
	paste_act = NULL;
	reopen_template_act = NULL;
	
	toolbar_view = NULL;
	toolbar_drawing = NULL;
	toolbar_editing = NULL;
	toolbar_advanced_editing = NULL;
	print_dock_widget = NULL;
	measure_dock_widget = NULL;
	color_dock_widget = NULL;
	symbol_dock_widget = NULL;
	template_dock_widget = NULL;
	
	actionsById[""] = new QAction(this); // dummy action
}

MapEditorController::~MapEditorController()
{
	paste_act = NULL;
	delete current_tool;
	delete override_tool;
	delete editor_activity;
	delete toolbar_view;
	delete toolbar_drawing;
	delete toolbar_editing;
	delete toolbar_advanced_editing;
	delete print_dock_widget;
	delete measure_dock_widget;
	delete color_dock_widget;
	delete symbol_dock_widget;
	delete template_dock_widget;
	for (QHash<Template*, TemplatePositionDockWidget*>::iterator it = template_position_widgets.begin(); it != template_position_widgets.end(); ++it)
		delete it.value();
	delete main_view;
	delete map;
}

void MapEditorController::setTool(MapEditorTool* new_tool)
{
	if (current_tool)
		current_tool->deleteLater();
	
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
	if (!current_tool || current_tool->getType() != MapEditorTool::EditPoint)
		setTool(new EditPointTool(this, edit_tool_act, symbol_widget));
}

void MapEditorController::setOverrideTool(MapEditorTool* new_override_tool)
{
	if (override_tool == new_override_tool)
		return;
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
		return new EditPointTool(this, edit_tool_act, symbol_widget);
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
		updatePasteAvailability();
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

void MapEditorController::addTemplatePositionDockWidget(Template* temp)
{
	assert(!existsTemplatePositionDockWidget(temp));
	TemplatePositionDockWidget* dock_widget = new TemplatePositionDockWidget(temp, this, window);
	addFloatingDockWidget(dock_widget);
	template_position_widgets.insert(temp, dock_widget);
}

void MapEditorController::removeTemplatePositionDockWidget(Template* temp)
{
	emit(templatePositionDockWidgetClosed(temp));
	
	delete getTemplatePositionDockWidget(temp);
	int num_deleted = template_position_widgets.remove(temp);
	assert(num_deleted == 1);
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
	measure_dock_widget = NULL;
	color_dock_widget = NULL;
	symbol_dock_widget = NULL;
	template_dock_widget = NULL;
	
	this->window = window;
	if (mode == MapEditor)
		window->setHasOpenedFile(true);
	connect(map, SIGNAL(gotUnsavedChanges()), window, SLOT(gotUnsavedChanges()));
	
	// Add zoom / cursor position field to status bar
	QLabel* statusbar_zoom_icon = new QLabel();
	statusbar_zoom_icon->setPixmap(QPixmap(":/images/magnifying-glass-12.png"));
	
	QLabel* statusbar_zoom_label = new QLabel();
	statusbar_zoom_label->setFixedWidth(51);
	statusbar_zoom_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	
	statusbar_zoom_frame = new QFrame();
	statusbar_zoom_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	statusbar_zoom_frame->setLineWidth(1);
	QHBoxLayout* statusbar_zoom_frame_layout = new QHBoxLayout();
	statusbar_zoom_frame_layout->setMargin(0);
	statusbar_zoom_frame_layout->setSpacing(0);
	statusbar_zoom_frame_layout->addSpacing(1);
	statusbar_zoom_frame_layout->addWidget(statusbar_zoom_icon);
	//statusbar_zoom_frame_layout->addStretch(1);
	statusbar_zoom_frame_layout->addWidget(statusbar_zoom_label);
	statusbar_zoom_frame->setLayout(statusbar_zoom_frame_layout);
	
	statusbar_cursorpos_label = new QLabel();
	statusbar_cursorpos_label->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	statusbar_cursorpos_label->setFixedWidth(160);
	statusbar_cursorpos_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	
	window->statusBar()->addPermanentWidget(statusbar_zoom_frame);
	window->statusBar()->addPermanentWidget(statusbar_cursorpos_label);
	
	// Create map widget
	map_widget = new MapWidget(mode == MapEditor, mode == SymbolEditor);
	connect(window, SIGNAL(keyPressed(QKeyEvent*)), map_widget, SLOT(keyPressed(QKeyEvent*)));
	connect(window, SIGNAL(keyReleased(QKeyEvent*)), map_widget, SLOT(keyReleased(QKeyEvent*)));
	map_widget->setMapView(main_view);
	map_widget->setZoomLabel(statusbar_zoom_label);
	map_widget->setCursorposLabel(statusbar_cursorpos_label);
	window->setCentralWidget(map_widget);
	
	// Create menu and toolbar together, so actions can be inserted into one or both
	if (mode == MapEditor)
	{
		createMenuAndToolbars();
		createPieMenu(&map_widget->getPieMenu());
		restoreWindowState();
	}
	
	// Update enabled/disabled state for the tools ...
	updateWidgets();
	// ... and make sure it is kept up to date for copy/paste
	connect(QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));
	clipboardChanged(QClipboard::Clipboard);
	
	if (mode == MapEditor)
	{
		// Show the symbol window
		symbol_window_act->trigger();
		
		// Auto-select the edit tool
		edit_tool_act->setChecked(true);
		setEditTool();
		
		// Set the coordinates display mode
		coordsDisplayChanged();
	}
	else
	{
		// Set the coordinates display mode
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
	}
}

QAction* MapEditorController::newAction(const char* id, const QString &tr_text, QObject* receiver, const char* slot, const char* icon, const QString& tr_tip, const QString& whatsThisLink)
{
	QAction* action = new QAction(icon ? QIcon(QString(":/images/%1").arg(icon)) : QIcon(), tr_text, this);
	if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
	if (!whatsThisLink.isEmpty()) action->setWhatsThis("<a href=\"" + whatsThisLink + "\">See more</a>");
	if (receiver) QObject::connect(action, SIGNAL(triggered()), receiver, slot);
	actionsById[id] = action;
	return action;
}

QAction* MapEditorController::newCheckAction(const char* id, const QString &tr_text, QObject* receiver, const char* slot, const char* icon, const QString& tr_tip, const QString& whatsThisLink)
{
	QAction* action = new QAction(icon ? QIcon(QString(":/images/%1").arg(icon)) : QIcon(), tr_text, this);
	action->setCheckable(true);
	if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
	if (!whatsThisLink.isEmpty()) action->setWhatsThis("<a href=\"" + whatsThisLink + "\">See more</a>");
	if (receiver) QObject::connect(action, SIGNAL(triggered(bool)), receiver, slot);
	actionsById[id] = action;
	return action;
}

QAction* MapEditorController::newToolAction(const char* id, const QString &tr_text, QObject* receiver, const char* slot, const char* icon, const QString& tr_tip, const QString& whatsThisLink)
{
	QAction* action = new MapEditorToolAction(icon ? QIcon(QString(":/images/%1").arg(icon)) : QIcon(), tr_text, this);
	if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
	if (!whatsThisLink.isEmpty()) action->setWhatsThis("<a href=\"" + whatsThisLink + "\">See more</a>");
	if (receiver) QObject::connect(action, SIGNAL(activated()), receiver, slot);
	actionsById[id] = action;
	return action;
}

QAction* MapEditorController::findAction(const char* id)
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
	findAction("showgrid")->setShortcut(QKeySequence("G"));
	findAction("zoomin")->setShortcut(QKeySequence("F7"));
	findAction("zoomout")->setShortcut(QKeySequence("F8"));
	findAction("hatchareasview")->setShortcut(QKeySequence("F2"));
	findAction("baselineview")->setShortcut(QKeySequence("F3"));
	findAction("hidealltemplates")->setShortcut(QKeySequence("F10"));
	findAction("fullscreen")->setShortcut(QKeySequence("F11"));
	color_window_act->setShortcut(QKeySequence("Ctrl+Shift+7"));
	symbol_window_act->setShortcut(QKeySequence("Ctrl+Shift+8"));
	template_window_act->setShortcut(QKeySequence("Ctrl+Shift+9"));
	
	findAction("editobjects")->setShortcut(QKeySequence("E"));
	findAction("editlines")->setShortcut(QKeySequence("L"));
	findAction("drawpoint")->setShortcut(QKeySequence("S"));
	findAction("drawpath")->setShortcut(QKeySequence("P"));
	findAction("drawcircle")->setShortcut(QKeySequence("O"));
	findAction("drawrectangle")->setShortcut(QKeySequence("Ctrl+R"));
	findAction("drawtext")->setShortcut(QKeySequence("T"));
	
	findAction("duplicate")->setShortcut(QKeySequence("D"));
	findAction("switchdashes")->setShortcut(QKeySequence("Ctrl+D"));
	findAction("connectpaths")->setShortcut(QKeySequence("C"));
	findAction("rotateobjects")->setShortcut(QKeySequence("R"));
	findAction("scaleobjects")->setShortcut(QKeySequence("Z"));
	findAction("cutobject")->setShortcut(QKeySequence("K"));
	findAction("cuthole")->setShortcut(QKeySequence("H"));
	findAction("measure")->setShortcut(QKeySequence("M"));
	findAction("booleanunion")->setShortcut(QKeySequence("U"));
	findAction("converttocurves")->setShortcut(QKeySequence("N"));
}

void MapEditorController::createMenuAndToolbars()
{
	// Define all the actions, saving them into variables as necessary. Can also get them by ID.
	QSignalMapper* print_act_mapper = new QSignalMapper(this);
	connect(print_act_mapper, SIGNAL(mapped(int)), this, SLOT(printClicked(int)));
	QAction* print_act = newAction("print", tr("Print..."), print_act_mapper, SLOT(map()), "print.png", QString::null, "file_menu.html");
	print_act_mapper->setMapping(print_act, PrintWidget::PRINT_TASK);
	QAction* export_image_act = newAction("export-image", tr("&Image..."), print_act_mapper, SLOT(map()), NULL, QString::null, "file_menu.html");
	print_act_mapper->setMapping(export_image_act, PrintWidget::EXPORT_IMAGE_TASK);
	QAction* export_pdf_act = newAction("export-pdf", tr("&PDF..."), print_act_mapper, SLOT(map()), NULL, QString::null, "file_menu.html");
	print_act_mapper->setMapping(export_pdf_act, PrintWidget::EXPORT_PDF_TASK);
	
	undo_act = newAction("undo", tr("Undo"), this, SLOT(undo()), "undo.png", tr("Undo the last step"), "edit_menu.html");
	redo_act = newAction("redo", tr("Redo"), this, SLOT(redo()), "redo.png", tr("Redo the last step"), "edit_menu.html");
	cut_act = newAction("cut", tr("Cu&t"), this, SLOT(cut()), "cut.png");
	copy_act = newAction("copy", tr("C&opy"), this, SLOT(copy()), "copy.png");
	paste_act = newAction("paste", tr("&Paste"), this, SLOT(paste()), "paste");
	clear_undo_redo_history_act = newAction("clearundoredohistory", tr("Clear undo / redo history"), this, SLOT(clearUndoRedoHistory()), NULL, tr("Clear the undo / redo history to reduce map file size."));
	
	show_grid_act = newCheckAction("showgrid", tr("Show grid"), this, SLOT(showGrid()), "grid.png", QString::null, QString::null);	// TODO: link to manual
	QAction* configure_grid_act = newAction("configuregrid", tr("Configure grid..."), this, SLOT(configureGrid()), "grid.png", QString::null, QString::null);	// TODO: link to manual
	pan_act = newCheckAction("panmap", tr("Pan"), this, SLOT(pan()), "move.png", QString::null, "view_menu.html");
	QAction* zoom_in_act = newAction("zoomin", tr("Zoom in"), this, SLOT(zoomIn()), "view-zoom-in.png", QString::null, "view_menu.html");
	QAction* zoom_out_act = newAction("zoomout", tr("Zoom out"), this, SLOT(zoomOut()), "view-zoom-out.png", QString::null, "view_menu.html");
	QAction* show_all_act = newAction("showall", tr("Show whole map"), this, SLOT(showWholeMap()), "view-show-all.png");
	QAction* fullscreen_act = newAction("fullscreen", tr("Toggle fullscreen mode"), window, SLOT(toggleFullscreenMode()));
	QAction* custom_zoom_act = newAction("setzoom", tr("Set custom zoom factor..."), this, SLOT(setCustomZoomFactorClicked()));
	
	hatch_areas_view_act = newCheckAction("hatchareasview", tr("Hatch areas"), this, SLOT(hatchAreas(bool)), NULL, QString::null, QString::null);	// TODO: link to manual; icon?
	baseline_view_act = newCheckAction("baselineview", tr("Baseline view"), this, SLOT(baselineView(bool)), NULL, QString::null, QString::null);	// TODO: link to manual; icon?
	hide_all_templates_act = newCheckAction("hidealltemplates", tr("Hide all templates"), this, SLOT(hideAllTemplates(bool)), NULL, QString::null, QString::null);	// TODO: link to manual 
	overprinting_simulation_act = newCheckAction("overprintsimulation", tr("Overprinting simulation"), this, SLOT(overprintingSimulation(bool)), NULL, QString::null, QString::null);	// TODO: link to manual 
	
	symbol_window_act = newCheckAction("symbolwindow", tr("Symbol window"), this, SLOT(showSymbolWindow(bool)), "window-new.png", tr("Show/Hide the symbol window"), "symbols.html#symbols");
	color_window_act = newCheckAction("colorwindow", tr("Color window"), this, SLOT(showColorWindow(bool)), "window-new.png", tr("Show/Hide the color window"), "symbols.html#colors");
	QAction* load_symbols_from_act = newAction("loadsymbols", tr("Replace symbol set..."), this, SLOT(loadSymbolsFromClicked()), NULL, tr("Replace the symbols with those from another map file"));
	/*QAction* load_colors_from_act = newAction("loadcolors", tr("Load colors from..."), this, SLOT(loadColorsFromClicked()), NULL, tr("Replace the colors with those from another map file"));*/
	
	QAction *scale_all_symbols_act = newAction("scaleall", tr("Scale all symbols..."), this, SLOT(scaleAllSymbolsClicked()), NULL, tr("Scale the whole symbol set"));
	QAction* georeferencing_act = newAction("georef", tr("Georeferencing..."), this, SLOT(editGeoreferencing()));
	QAction *scale_map_act = newAction("scalemap", tr("Change map scale..."), this, SLOT(scaleMapClicked()), "tool-scale.png", tr("Change the map scale and adjust map objects and symbol sizes"), "map_menu.html");
	QAction *rotate_map_act = newAction("rotatemap", tr("Rotate map..."), this, SLOT(rotateMapClicked()), "tool-rotate.png", tr("Rotate the whole map"), "map_menu.html");
	QAction *map_notes_act = newAction("mapnotes", tr("Map notes..."), this, SLOT(mapNotesClicked()));
	
	template_window_act = newCheckAction("templatewindow", tr("Template setup window"), this, SLOT(showTemplateWindow(bool)), "window-new", tr("Show/Hide the template window"), "template_menu.html");
	//QAction* template_config_window_act = newCheckAction("templateconfigwindow", tr("Template configurations window"), this, SLOT(showTemplateConfigurationsWindow(bool)), "window-new", tr("Show/Hide the template configurations window"));
	//QAction* template_visibilities_window_act = newCheckAction("templatevisibilitieswindow", tr("Template visibilities window"), this, SLOT(showTemplateVisbilitiesWindow(bool)), "window-new", tr("Show/Hide the template visibilities window"));
	QAction* open_template_act = newAction("opentemplate", tr("Open template..."), this, SLOT(openTemplateClicked()), NULL, QString::null, "template_menu.html");
	reopen_template_act = newAction("reopentemplate", tr("Reopen template..."), this, SLOT(reopenTemplateClicked()), NULL, QString::null, "template_menu.html");
	
	edit_tool_act = newToolAction("editobjects", tr("Edit objects"), this, SLOT(editToolClicked()), "tool-edit.png", QString::null, "drawing_toolbar.html#selector");
	edit_line_tool_act = newToolAction("editlines", tr("Edit lines"), this, SLOT(editLineToolClicked()), "tool-edit-line.png", QString::null, "drawing_toolbar.html#selector"); // TODO: adjust help reference
	draw_point_act = newToolAction("drawpoint", tr("Set point objects"), this, SLOT(drawPointClicked()), "draw-point.png", QString::null, "drawing_toolbar.html#point");
	draw_path_act = newToolAction("drawpath", tr("Draw paths"), this, SLOT(drawPathClicked()), "draw-path.png", QString::null, "drawing_toolbar.html#line");
	draw_circle_act = newToolAction("drawcircle", tr("Draw circles and ellipses"), this, SLOT(drawCircleClicked()), "draw-circle.png", QString::null, "drawing_toolbar.html#circle");
	draw_rectangle_act = newToolAction("drawrectangle", tr("Draw rectangles"), this, SLOT(drawRectangleClicked()), "draw-rectangle.png", QString::null, "drawing_toolbar.html#rectangle");
	draw_text_act = newToolAction("drawtext", tr("Write text"), this, SLOT(drawTextClicked()), "draw-text.png", QString::null, "drawing_toolbar.html#text");
	duplicate_act = newAction("duplicate", tr("Duplicate"), this, SLOT(duplicateClicked()), "tool-duplicate.png", QString::null, "drawing_toolbar.html#duplicate");
	switch_symbol_act = newAction("switchsymbol", tr("Switch symbol"), this, SLOT(switchSymbolClicked()), "tool-switch-symbol.png", QString::null, "drawing_toolbar.html#change");
	fill_border_act = newAction("fillborder", tr("Fill / Create border"), this, SLOT(fillBorderClicked()), "tool-fill-border.png", QString::null, "drawing_toolbar.html#fill");
	switch_dashes_act = newAction("switchdashes", tr("Switch dash direction"), this, SLOT(switchDashesClicked()), "tool-switch-dashes", QString::null, "drawing_toolbar.html#reverse");
	connect_paths_act = newAction("connectpaths", tr("Connect paths"), this, SLOT(connectPathsClicked()), "tool-connect-paths.png", QString::null, "drawing_toolbar.html#connect");
	cut_tool_act = newToolAction("cutobject", tr("Cut object"), this, SLOT(cutClicked()), "tool-cut.png", QString::null, "drawing_toolbar.html#cut");
	cut_hole_act = newToolAction("cuthole", tr("Cut free form hole"), this, SLOT(cutHoleClicked()), "tool-cut-hole.png", QString::null, "drawing_toolbar.html#cut_hole"); // cut hole using a path
	cut_hole_circle_act = new MapEditorToolAction(QIcon(":/images/tool-cut-hole.png"), tr("Cut round hole"), this);
	cut_hole_circle_act->setWhatsThis("<a href=\"drawing_toolbar.html#cut_hole_circle\">See more</a>");
	cut_hole_circle_act->setCheckable(true);
	QObject::connect(cut_hole_circle_act, SIGNAL(activated()), this, SLOT(cutHoleCircleClicked()));
	cut_hole_rectangle_act = new MapEditorToolAction(QIcon(":/images/tool-cut-hole.png"), tr("Cut rectangular hole"), this);
	cut_hole_rectangle_act->setWhatsThis("<a href=\"drawing_toolbar.html#cut_hole_rectangle\">See more</a>");
	cut_hole_rectangle_act->setCheckable(true);
	QObject::connect(cut_hole_rectangle_act, SIGNAL(activated()), this, SLOT(cutHoleRectangleClicked()));
	rotate_act = newToolAction("rotateobjects", tr("Rotate object(s)"), this, SLOT(rotateClicked()), "tool-rotate.png", QString::null, "drawing_toolbar.html#rotate");
	rotate_pattern_act = newToolAction("rotatepatterns", tr("Rotate pattern"), this, SLOT(rotatePatternClicked()), "tool-rotate-pattern.png", QString::null, "drawing_toolbar.html#rotatepatterns");	// TODO: Write help file entry
	scale_act = newToolAction("scaleobjects", tr("Scale object(s)"), this, SLOT(scaleClicked()), "tool-scale.png", QString::null, "drawing_toolbar.html#scale");
	measure_act = newCheckAction("measure", tr("Measure lengths and areas"), this, SLOT(measureClicked(bool)), "tool-measure.png", QString::null, "drawing_toolbar.html#measure");
	boolean_union_act = newAction("booleanunion", tr("Unify areas"), this, SLOT(booleanUnionClicked()), "tool-boolean-union.png");
	boolean_intersection_act = newAction("booleanintersection", tr("Intersect areas"), this, SLOT(booleanIntersectionClicked()), "tool-boolean-intersection.png");
	boolean_difference_act = newAction("booleandifference", tr("Area difference"), this, SLOT(booleanDifferenceClicked()), "tool-boolean-difference.png");
	boolean_xor_act = newAction("booleanxor", tr("Area XOr"), this, SLOT(booleanXOrClicked()), "tool-boolean-xor.png");
	convert_to_curves_act = newAction("converttocurves", tr("Convert to curves"), this, SLOT(convertToCurvesClicked()), "tool-convert-to-curves.png");
	simplify_path_act = newAction("simplify", tr("Simplify path"), this, SLOT(simplifyPathClicked()), "tool-simplify-path.png");
	
	paint_on_template_act = new QAction(QIcon(":/images/pencil.png"), tr("Paint on template"), this);
	paint_on_template_act->setCheckable(true);
	updatePaintOnTemplateAction();
	connect(paint_on_template_act, SIGNAL(triggered(bool)), this, SLOT(paintOnTemplateClicked(bool)));
	
	QAction *import_act = newAction("import", tr("Import..."), this, SLOT(importClicked()));
	
	map_coordinates_act = new QAction(tr("Map coordinates"), this);
	map_coordinates_act->setCheckable(true);
	projected_coordinates_act = new QAction(tr("Projected coordinates"), this);
	projected_coordinates_act->setCheckable(true);
	geographic_coordinates_act = new QAction(tr("Latitude/Longitude (Dec)"), this);
	geographic_coordinates_act->setCheckable(true);
	geographic_coordinates_dms_act = new QAction(tr("Latitude/Longitude (DMS)"), this);
	geographic_coordinates_dms_act->setCheckable(true);
	QActionGroup* coordinates_group = new QActionGroup(this);
	coordinates_group->addAction(map_coordinates_act);
	coordinates_group->addAction(projected_coordinates_act);
	coordinates_group->addAction(geographic_coordinates_act);
	coordinates_group->addAction(geographic_coordinates_dms_act);
	QObject::connect(coordinates_group, SIGNAL(triggered(QAction*)), this, SLOT(coordsDisplayChanged()));
	map_coordinates_act->setChecked(true);
	QObject::connect(&map->getGeoreferencing(), SIGNAL(projectionChanged()), this, SLOT(projectionChanged()));
	projectionChanged();
	
	statusbar_cursorpos_label->setContextMenuPolicy(Qt::ActionsContextMenu);
	statusbar_cursorpos_label->addAction(map_coordinates_act);
	statusbar_cursorpos_label->addAction(projected_coordinates_act);
	statusbar_cursorpos_label->addAction(geographic_coordinates_act);
	statusbar_cursorpos_label->addAction(geographic_coordinates_dms_act);
	
	QMenu* coordinates_menu = new QMenu(tr("Display coordinates as..."));
	coordinates_menu->addAction(map_coordinates_act);
	coordinates_menu->addAction(projected_coordinates_act);
	coordinates_menu->addAction(geographic_coordinates_act);
	coordinates_menu->addAction(geographic_coordinates_dms_act);
	
	// Refactored so we can do custom key bindings in the future
	assignKeyboardShortcuts();
	
	// Export menu
	QMenu* export_menu = new QMenu(tr("&Export"));
	export_menu->addAction(export_image_act);
	export_menu->addAction(export_pdf_act);
	
	// Extend file menu
	QMenu* file_menu = window->getFileMenu();
	file_menu->insertAction(window->getFileMenuExtensionAct(), print_act);
	file_menu->insertSeparator(window->getFileMenuExtensionAct());
	file_menu->insertAction(print_act, import_act);
	file_menu->insertMenu(print_act, export_menu);
	file_menu->insertSeparator(print_act);
		
	// Edit menu
	QMenu* edit_menu = window->menuBar()->addMenu(tr("&Edit"));
	edit_menu->setWhatsThis("<a href=\"edit_menu.html\">See more</a>");
	edit_menu->addAction(undo_act);
	edit_menu->addAction(redo_act);
	edit_menu->addSeparator();
	edit_menu->addAction(cut_act);
	edit_menu->addAction(copy_act);
	edit_menu->addAction(paste_act);
	edit_menu->addSeparator();
	edit_menu->addAction(clear_undo_redo_history_act);
	
	// View menu
	QMenu* view_menu = window->menuBar()->addMenu(tr("&View"));
	view_menu->setWhatsThis("<a href=\"view_menu.html\">See more</a>");
	view_menu->addAction(pan_act);
	view_menu->addAction(zoom_in_act);
	view_menu->addAction(zoom_out_act);
	view_menu->addAction(show_all_act);
	view_menu->addAction(custom_zoom_act);
	view_menu->addSeparator();
	view_menu->addAction(hatch_areas_view_act);
	view_menu->addAction(baseline_view_act);
	view_menu->addAction(hide_all_templates_act);
	view_menu->addAction(overprinting_simulation_act);
	view_menu->addSeparator();
	view_menu->addMenu(coordinates_menu);
	view_menu->addSeparator();
	view_menu->addAction(fullscreen_act);
	view_menu->addSeparator();
	view_menu->addAction(color_window_act);
	view_menu->addAction(symbol_window_act);
	view_menu->addAction(template_window_act);
	
	// Tools menu
	QMenu *tools_menu = window->menuBar()->addMenu(tr("&Tools"));
	tools_menu->setWhatsThis("<a href=\"tools_menu.html\">See more</a>");
	tools_menu->addAction(edit_tool_act);
	tools_menu->addAction(edit_line_tool_act);
	tools_menu->addAction(draw_point_act);
	tools_menu->addAction(draw_path_act);
	tools_menu->addAction(draw_circle_act);
	tools_menu->addAction(draw_rectangle_act);
	tools_menu->addAction(draw_text_act);
	tools_menu->addAction(duplicate_act);
	tools_menu->addAction(switch_symbol_act);
	tools_menu->addAction(fill_border_act);
	tools_menu->addAction(switch_dashes_act);
	tools_menu->addAction(connect_paths_act);
	tools_menu->addAction(boolean_union_act);
	tools_menu->addAction(boolean_intersection_act);
	tools_menu->addAction(boolean_difference_act);
	tools_menu->addAction(boolean_xor_act);
	tools_menu->addAction(convert_to_curves_act);
	tools_menu->addAction(simplify_path_act);
	tools_menu->addAction(cut_tool_act);
	cut_hole_menu = new QMenu(tr("Cut hole"));
	cut_hole_menu->setIcon(QIcon(":/images/tool-cut-hole.png"));
	cut_hole_menu->addAction(cut_hole_act);
	cut_hole_menu->addAction(cut_hole_circle_act);
	cut_hole_menu->addAction(cut_hole_rectangle_act);
	tools_menu->addMenu(cut_hole_menu);
	tools_menu->addAction(rotate_act);
	tools_menu->addAction(rotate_pattern_act);
	tools_menu->addAction(scale_act);
	tools_menu->addAction(measure_act);
	
	// Map menu
	QMenu* map_menu = window->menuBar()->addMenu(tr("M&ap"));
	map_menu->setWhatsThis("<a href=\"map_menu.html\">See more</a>");
	map_menu->addAction(georeferencing_act);
	map_menu->addAction(configure_grid_act);
	map_menu->addSeparator();
	map_menu->addAction(scale_map_act);
	map_menu->addAction(rotate_map_act);
	map_menu->addAction(map_notes_act);
	
	// Symbols menu
	QMenu* symbols_menu = window->menuBar()->addMenu(tr("Sy&mbols"));
	symbols_menu->setWhatsThis("<a href=\"symbols_menu.html\">See more</a>");
	symbols_menu->addAction(symbol_window_act);
	symbols_menu->addAction(color_window_act);
	symbols_menu->addSeparator();
	symbols_menu->addAction(scale_all_symbols_act);
	symbols_menu->addAction(load_symbols_from_act);
	/*symbols_menu->addAction(load_colors_from_act);*/
	
	// Templates menu
	QMenu* template_menu = window->menuBar()->addMenu(tr("&Templates"));
	template_menu->setWhatsThis("<a href=\"template_menu.html\">See more</a>");
	template_menu->addAction(template_window_act);
	/*template_menu->addAction(template_config_window_act);
	template_menu->addAction(template_visibilities_window_act);*/
	template_menu->addSeparator();
	template_menu->addAction(open_template_act);
	template_menu->addAction(reopen_template_act);
	
#if defined(Q_WS_MAC) && QT_VERSION < 0x050000
	// Mac toolbars are still a little screwed up, turns out we have to insert a
	// "dummy" toolbar first and hide it, then the others show up
	window->addToolBar(tr("Dummy"))->hide();
#endif
	
	// Extend and activate general toolbar
	QToolBar* main_toolbar = window->getGeneralToolBar();
	main_toolbar->addAction(print_act);
	main_toolbar->addSeparator();
	main_toolbar->addAction(cut_act);
	main_toolbar->addAction(copy_act);
	main_toolbar->addAction(paste_act);
	main_toolbar->addSeparator();
	main_toolbar->addAction(undo_act);
	main_toolbar->addAction(redo_act);
	window->addToolBar(main_toolbar);
	
	// View toolbar
	toolbar_view = window->addToolBar(tr("View"));
	toolbar_view->setObjectName("View toolbar");
	QToolButton* grid_button = new QToolButton();
	grid_button->setCheckable(true);
	grid_button->setDefaultAction(show_grid_act);
	grid_button->setPopupMode(QToolButton::MenuButtonPopup);
	QMenu* grid_menu = new QMenu(grid_button);
	grid_menu->addAction(tr("Configure grid..."));
	grid_button->setMenu(grid_menu);
	connect(grid_menu, SIGNAL(triggered(QAction*)), this, SLOT(configureGrid()));
	toolbar_view->addWidget(grid_button);
	toolbar_view->addSeparator();
	toolbar_view->addAction(pan_act);
	toolbar_view->addAction(zoom_in_act);
	toolbar_view->addAction(zoom_out_act);
	toolbar_view->addAction(show_all_act);
	
	// Drawing toolbar
	toolbar_drawing = window->addToolBar(tr("Drawing"));
	toolbar_drawing->setObjectName("Drawing toolbar");
	toolbar_drawing->addAction(edit_tool_act);
	toolbar_drawing->addAction(edit_line_tool_act);
	toolbar_drawing->addAction(draw_point_act);
	toolbar_drawing->addAction(draw_path_act);
	toolbar_drawing->addAction(draw_circle_act);
	toolbar_drawing->addAction(draw_rectangle_act);
	toolbar_drawing->addAction(draw_text_act);
	toolbar_drawing->addSeparator();
	
	QToolButton* paint_on_template_button = new QToolButton();
	paint_on_template_button->setCheckable(true);
	paint_on_template_button->setDefaultAction(paint_on_template_act);
	paint_on_template_button->setPopupMode(QToolButton::MenuButtonPopup);
	QMenu* paint_on_template_menu = new QMenu(paint_on_template_button);
	paint_on_template_menu->addAction(tr("Select template..."));
	paint_on_template_button->setMenu(paint_on_template_menu);
	connect(paint_on_template_menu, SIGNAL(triggered(QAction*)), this, SLOT(paintOnTemplateSelectClicked()));
	toolbar_drawing->addWidget(paint_on_template_button);
	
	// Editing toolbar
	toolbar_editing = window->addToolBar(tr("Editing"));
	toolbar_editing->setObjectName("Editing toolbar");
	toolbar_editing->addAction(duplicate_act);
	toolbar_editing->addAction(switch_symbol_act);
	toolbar_editing->addAction(fill_border_act);
	toolbar_editing->addAction(switch_dashes_act);
	toolbar_editing->addAction(connect_paths_act);
	toolbar_editing->addAction(boolean_union_act);
	toolbar_editing->addAction(cut_tool_act);
	
	QToolButton* cut_hole_button = new QToolButton();
	cut_hole_button->setCheckable(true);
	cut_hole_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	cut_hole_button->setDefaultAction(cut_hole_act);
	cut_hole_button->setPopupMode(QToolButton::MenuButtonPopup);
	cut_hole_button->setMenu(cut_hole_menu);
	toolbar_editing->addWidget(cut_hole_button);
	
	toolbar_editing->addAction(rotate_act);
	toolbar_editing->addAction(rotate_pattern_act);
	toolbar_editing->addAction(scale_act);
	toolbar_editing->addAction(measure_act);
	
	// Advanced editing toolbar
	toolbar_advanced_editing = window->addToolBar(tr("Advanced editing"));
	toolbar_advanced_editing->setObjectName("Advanved editing toolbar");
	toolbar_advanced_editing->addAction(convert_to_curves_act);
	toolbar_advanced_editing->addAction(simplify_path_act);
	toolbar_advanced_editing->addAction(boolean_intersection_act);
	toolbar_advanced_editing->addAction(boolean_difference_act);
	toolbar_advanced_editing->addAction(boolean_xor_act);
}

void MapEditorController::createPieMenu(PieMenu* menu)
{
	int i = 0;
	menu->setAction(i++, edit_tool_act);
	menu->setAction(i++, draw_point_act);
	menu->setAction(i++, draw_path_act);
	menu->setAction(i++, draw_rectangle_act);
	menu->setAction(i++, cut_tool_act);
	menu->setAction(i++, cut_hole_act);
	menu->setAction(i++, switch_dashes_act);
	menu->setAction(i++, connect_paths_act);
}

void MapEditorController::detach()
{
	saveWindowState();
	
	window->setCentralWidget(NULL);
	delete map_widget;
	
	delete statusbar_zoom_frame;
	delete statusbar_cursorpos_label;
}

void MapEditorController::saveWindowState()
{
	QSettings settings;
	settings.beginGroup(metaObject()->className());
	settings.setValue("state", window->saveState());
}

void MapEditorController::restoreWindowState()
{
	QSettings settings;
	settings.beginGroup(metaObject()->className());
	window->restoreState(settings.value("state").toByteArray());
}

void MapEditorController::keyPressEvent(QKeyEvent* event)
{
	map_widget->keyPressed(event);
}

void MapEditorController::keyReleaseEvent(QKeyEvent* event)
{
	map_widget->keyReleased(event);
}

void MapEditorController::printClicked(int task)
{
	if (!print_dock_widget)
	{
		print_dock_widget = new EditorDockWidget(QString::null, NULL, this, window);
		print_dock_widget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
		print_widget = new PrintWidget(map, window, main_view, this, print_dock_widget);
		connect(print_dock_widget, SIGNAL(visibilityChanged(bool)), print_widget, SLOT(setActive(bool)));
		connect(print_widget, SIGNAL(finished(int)), print_dock_widget, SLOT(close()));
		connect(print_widget, SIGNAL(taskChanged(QString)), print_dock_widget, SLOT(setWindowTitle(QString)));
		print_dock_widget->setWidget(print_widget);
		print_dock_widget->setObjectName("Print dock widget");
		if (!window->restoreDockWidget(print_dock_widget))
			addFloatingDockWidget(print_dock_widget);
	}
	
	print_widget->setTask((PrintWidget::TaskFlags)task);
	print_dock_widget->show();
	QTimer::singleShot(0, print_dock_widget, SLOT(raise()));
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
	if ((!redo && map->objectUndoManager().getNumUndoSteps() == 0) ||
		(redo && map->objectUndoManager().getNumRedoSteps() == 0))
	{
		// This should not happen as the action should be deactivated in this case!
		QMessageBox::critical(window, tr("Error"), tr("No undo steps available."));
		return;
	}
	
	UndoStep* generic_step = redo ? map->objectUndoManager().getLastRedoStep() : map->objectUndoManager().getLastUndoStep();
	MapPart* affected_part = NULL;
	std::vector<Object*> affected_objects;
	
	if (generic_step->getType() == UndoStep::CombinedUndoStepType)
	{
		CombinedUndoStep* combined_step = reinterpret_cast<CombinedUndoStep*>(generic_step);
		std::set<Object*> affected_objects_set;
		for (int i = 0; i < combined_step->getNumSubSteps(); ++i)
		{
			std::vector<Object*> affected_objects_temp;
			MapUndoStep* map_step = reinterpret_cast<MapUndoStep*>(combined_step->getSubStep(i));
			if (!affected_part)
				affected_part = map->getPart(map_step->getPart());
			else
				assert(affected_part == map->getPart(map_step->getPart()));
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
		affected_part = map->getPart(map_step->getPart());
		map_step->getAffectedOutcome(affected_objects);
	}
	
	bool in_saved_state, done;
	if (redo)
		in_saved_state = map->objectUndoManager().redo(window, &done);
	else
		in_saved_state = map->objectUndoManager().undo(window, &done);
	
	if (!done)
		return;
	
	if (map->hasUnsavedChanged() && in_saved_state && !(map->areColorsDirty() || map->areSymbolsDirty() || map->areTemplatesDirty() || map->isOtherDirty()))
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
		map_widget->ensureVisibilityOfRect(object_rect, false, true);
}

void MapEditorController::cut()
{
	copy();
	window->statusBar()->showMessage(tr("Cut %1 object(s)").arg(map->getNumSelectedObjects()), 2000);
	map->deleteSelectedObjects();
}

void MapEditorController::copy()
{
	if (map->getNumSelectedObjects() == 0)
		return;
	
	// Create map containing required objects and their symbol and color dependencies
	Map* copy_map = new Map();
	copy_map->setScaleDenominator(map->getScaleDenominator());
	
	std::vector<bool> symbol_filter;
	symbol_filter.assign(map->getNumSymbols(), false);
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(), end = map->selectedObjectsEnd(); it != end; ++it)
	{
		int symbol_index = map->findSymbolIndex((*it)->getSymbol());
		if (symbol_index >= 0)
			symbol_filter[symbol_index] = true;
	}
	
	// Export symbols and colors into copy_map
	QHash<Symbol*, Symbol*> symbol_map;
	copy_map->importMap(map, Map::MinimalSymbolImport, window, &symbol_filter, -1, true, &symbol_map);
	
	// Duplicate all selected objects into copy map
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(), end = map->selectedObjectsEnd(); it != end; ++it)
	{
		Object* new_object = (*it)->duplicate();
		if (symbol_map.contains(new_object->getSymbol()))
			new_object->setSymbol(symbol_map.value(new_object->getSymbol()), true);
		
		copy_map->addObject(new_object);
	}
	
	// Save map to memory
	QBuffer buffer;
	if (!copy_map->exportToIODevice(&buffer))
	{
		delete copy_map;
		QMessageBox::warning(NULL, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	delete copy_map;
	
	// Put buffer into clipboard
	QMimeData* mime_data = new QMimeData();
	mime_data->setData("openorienteering/objects", buffer.data());
	QApplication::clipboard()->setMimeData(mime_data);
	
	// Show message
	window->statusBar()->showMessage(tr("Copied %1 object(s)").arg(map->getNumSelectedObjects()), 2000);
}

void MapEditorController::paste()
{
	if (editing_in_progress)
		return;
	if (!QApplication::clipboard()->mimeData()->hasFormat("openorienteering/objects"))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("There are no objects in clipboard which could be pasted!"));
		return;
	}
	
	// Get buffer from clipboard
	QByteArray byte_array = QApplication::clipboard()->mimeData()->data("openorienteering/objects");
	QBuffer buffer(&byte_array);
	buffer.open(QIODevice::ReadOnly);
	
	// Create map from buffer
	Map* paste_map = new Map();
	if (!paste_map->importFromIODevice(&buffer))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	
	// Move objects in paste_map so their bounding box center is at this map's viewport center.
	// This makes the pasted objects appear at the center of the viewport.
	QRectF paste_extent = paste_map->calculateExtent(true, false, NULL);
	qint64 dx = main_view->getPositionX() - paste_extent.center().x() * 1000;
	qint64 dy = main_view->getPositionY() - paste_extent.center().y() * 1000;
	
	MapPart* part = paste_map->getCurrentPart();
	for (int i = 0; i < part->getNumObjects(); ++i)
		part->getObject(i)->move(dx, dy);
	
	// Import pasted map
	map->importMap(paste_map, Map::CompleteImport, window);
	
	// Show message
	window->statusBar()->showMessage(tr("Pasted %1 object(s)").arg(paste_map->getNumObjects()), 2000);
	delete paste_map;
}

void MapEditorController::clearUndoRedoHistory()
{
	map->objectUndoManager().clear(false);
	map->setOtherDirty();
}

void MapEditorController::showGrid()
{
	main_view->setGridVisible(show_grid_act->isChecked());
	map->updateAllMapWidgets();
}

void MapEditorController::configureGrid()
{
	ConfigureGridDialog dialog(window, map, main_view);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Accepted)
	{
		show_grid_act->setChecked(main_view->isGridVisible());
		map->updateAllMapWidgets();
		map->setOtherDirty(true);
	}
}

void MapEditorController::pan()
{
	setTool(new PanTool(this, pan_act));
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

void MapEditorController::hatchAreas(bool checked)
{
	map->setAreaHatchingEnabled(checked);
	// Update all areas
	map->operationOnAllObjects(ObjectOp::Update(), ObjectOp::ContainsSymbolType(Symbol::Area));
}

void MapEditorController::baselineView(bool checked)
{
	map->setBaselineViewEnabled(checked);
	map->updateAllObjects();
}

void MapEditorController::hideAllTemplates(bool checked)
{
	main_view->setHideAllTemplates(checked);
}

void MapEditorController::overprintingSimulation(bool checked)
{
	main_view->setOverprintingSimulationEnabled(checked);
	map->updateAllMapWidgets();
}

void MapEditorController::coordsDisplayChanged()
{
	if (geographic_coordinates_dms_act->isChecked())
		map_widget->setCoordsDisplay(MapWidget::GEOGRAPHIC_COORDS_DMS);
	else if (geographic_coordinates_act->isChecked())
		map_widget->setCoordsDisplay(MapWidget::GEOGRAPHIC_COORDS);
	else if (projected_coordinates_act->isChecked())
		map_widget->setCoordsDisplay(MapWidget::PROJECTED_COORDS);	
	else
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
}

void MapEditorController::projectionChanged()
{
	const Georeferencing& geo(map->getGeoreferencing());
	
	bool enable_projected = geo.getState() != Georeferencing::ScaleOnly;
	projected_coordinates_act->setEnabled(enable_projected);
	projected_coordinates_act->setText(geo.getProjectedCRSName());
	if (!enable_projected &&
		map_widget->getCoordsDisplay() == MapWidget::PROJECTED_COORDS)
	{
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
		map_coordinates_act->setChecked(true);
	}
	
	bool enable_geographic = geo.getState() != Georeferencing::ScaleOnly && !geo.isLocal();
	geographic_coordinates_act->setEnabled(enable_geographic);
	geographic_coordinates_dms_act->setEnabled(enable_geographic);
	if (!enable_geographic &&
		(map_widget->getCoordsDisplay() == MapWidget::GEOGRAPHIC_COORDS ||
		map_widget->getCoordsDisplay() == MapWidget::GEOGRAPHIC_COORDS_DMS))
	{
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
		map_coordinates_act->setChecked(true);
	}
}

void MapEditorController::showSymbolWindow(bool show)
{
	if (!symbol_dock_widget)
	{
		symbol_dock_widget = new EditorDockWidget(tr("Symbols"), symbol_window_act, this, window);
		symbol_widget = new SymbolWidget(map, symbol_dock_widget);
		connect(window, SIGNAL(keyPressed(QKeyEvent*)), symbol_widget, SLOT(keyPressed(QKeyEvent*)));
		connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), symbol_widget, SLOT(symbolChanged(int,Symbol*,Symbol*)));	// NOTE: adjust setMap() if changing this!
		connect(map, SIGNAL(symbolIconChanged(int)), symbol_widget->getRenderWidget(), SLOT(updateIcon(int)));
		symbol_dock_widget->setWidget(symbol_widget);
		symbol_dock_widget->setObjectName("Symbol dock widget");
		if (!window->restoreDockWidget(symbol_dock_widget))
			window->addDockWidget(Qt::RightDockWidgetArea, symbol_dock_widget, Qt::Vertical);
		
		connect(symbol_widget, SIGNAL(switchSymbolClicked()), this, SLOT(switchSymbolClicked()));
		connect(symbol_widget, SIGNAL(fillBorderClicked()), this, SLOT(fillBorderClicked()));
		connect(symbol_widget, SIGNAL(selectObjectsClicked()), this, SLOT(selectObjectsClicked()));
		connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
		selectedSymbolsChanged();
	}
	
	symbol_dock_widget->setVisible(show);
	if (show)
		QTimer::singleShot(0, symbol_dock_widget, SLOT(raise()));
}

void MapEditorController::showColorWindow(bool show)
{
	if (!color_dock_widget)
	{
		color_dock_widget = new EditorDockWidget(tr("Colors"), color_window_act, this, window);
		color_dock_widget->setWidget(new ColorWidget(map, window, color_dock_widget));
		color_dock_widget->setObjectName("Color dock widget");
		if (!window->restoreDockWidget(color_dock_widget))
			window->addDockWidget(Qt::LeftDockWidgetArea, color_dock_widget, Qt::Vertical);
	}
	
	color_dock_widget->setVisible(show);
	if (show)
		QTimer::singleShot(0, color_dock_widget, SLOT(raise()));
}

void MapEditorController::loadSymbolsFromClicked()
{
	ReplaceSymbolSetDialog::showDialog(window, map);
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

void MapEditorController::rotateMapClicked()
{
	RotateMapDialog dialog(window, map);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
}

void MapEditorController::mapNotesClicked()
{
	QDialog dialog(window, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	dialog.setWindowTitle(tr("Map notes"));
	dialog.setWindowModality(Qt::WindowModal);
	
	QTextEdit* text_edit = new QTextEdit();
	text_edit->setPlainText(map->getMapNotes());
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	QPushButton* ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("OK"));
	ok_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(text_edit);
	layout->addLayout(buttons_layout);
	dialog.setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), &dialog, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), &dialog, SLOT(accept()));
	
	if (dialog.exec() == QDialog::Accepted)
	{
		if (text_edit->toPlainText() != map->getMapNotes())
		{
			map->setMapNotes(text_edit->toPlainText());
			map->setHasUnsavedChanges();
		}
	}
}

void MapEditorController::showTemplateWindow(bool show)
{
	if (!template_dock_widget)
	{
		template_dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, this, window);
		template_dock_widget->setWidget(new TemplateWidget(map, main_view, this, template_dock_widget));
		template_dock_widget->setObjectName("Templates dock widget");
		if (!window->restoreDockWidget(template_dock_widget))
			window->addDockWidget(Qt::RightDockWidgetArea, template_dock_widget, Qt::Vertical);
	}
	
	template_dock_widget->setVisible(show);
	if (show)
		QTimer::singleShot(0, template_dock_widget, SLOT(raise()));
}

void MapEditorController::openTemplateClicked()
{
	Template* new_template = TemplateWidget::showOpenTemplateDialog(window, main_view);
	if (!new_template)
		return;
	
	template_window_act->setChecked(true);
	showTemplateWindow(true);
	
	TemplateWidget* template_widget = reinterpret_cast<TemplateWidget*>(template_dock_widget->widget());
	template_widget->addTemplateAt(new_template, -1);
}

void MapEditorController::reopenTemplateClicked()
{
	QString map_directory = window->getCurrentFilePath();
	if (!map_directory.isEmpty())
		map_directory = QFileInfo(map_directory).canonicalPath();
	ReopenTemplateDialog* dialog = new ReopenTemplateDialog(window, map, main_view, map_directory); 
	dialog->setWindowModality(Qt::WindowModal);
	dialog->exec();
	delete dialog;
}

void MapEditorController::closedTemplateAvailabilityChanged()
{
	if (reopen_template_act)
		reopen_template_act->setEnabled(map->getNumClosedTemplates() > 0);
}

void MapEditorController::editGeoreferencing()
{
	if (georeferencing_dialog.isNull())
	{
		GeoreferencingDialog* dialog = new GeoreferencingDialog(this); 
		georeferencing_dialog.reset(dialog);
		connect(dialog, SIGNAL(finished(int)), this, SLOT(georeferencingDialogFinished()));
	}
	georeferencing_dialog->exec();
}

void MapEditorController::georeferencingDialogFinished()
{
	georeferencing_dialog.take()->deleteLater();
	map->updateAllMapWidgets();
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
	draw_circle_act->setEnabled(draw_path_act->isEnabled());
	draw_circle_act->setStatusTip(tr("Draw circles and ellipses.") + (draw_circle_act->isEnabled() ? "" : (" " + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_rectangle_act->setEnabled(draw_path_act->isEnabled());
	draw_rectangle_act->setStatusTip(tr("Draw rectangles.") + (draw_rectangle_act->isEnabled() ? "" : (" " + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_text_act->setEnabled(type == Symbol::Text && !symbol->isHidden());
	draw_text_act->setStatusTip(tr("Write text on the map.") + (draw_text_act->isEnabled() ? "" : (" " + tr("Select a text symbol to be able to use this tool."))));
	
	selectedSymbolsOrObjectsChanged();
}

void MapEditorController::objectSelectionChanged()
{
	if (mode != MapEditor)
		return;
	
	bool have_selection = map->getNumSelectedObjects() > 0;
	bool single_object_selected = map->getNumSelectedObjects() == 1;
	bool have_line = false;
	bool have_area = false;
	bool have_rotatable_pattern = false;
	bool have_rotatable_point = false;
	int num_selected_areas = 0;
	bool have_two_same_symbol_areas = false;
	bool uniform_symbol_selected = true;
	Symbol* uniform_symbol = NULL;
	std::vector< bool > symbols_in_selection;
	symbols_in_selection.assign(map->getNumSymbols(), false);
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Symbol* symbol = (*it)->getSymbol();
		int symbol_index = map->findSymbolIndex(symbol);
		if (symbol_index >= 0 && symbol_index < (int)symbols_in_selection.size())
			symbols_in_selection[symbol_index] = true;
		
		if (uniform_symbol_selected)
		{
			if (!uniform_symbol)
				uniform_symbol = symbol;
			else if (uniform_symbol != symbol)
			{
				uniform_symbol = NULL;
				uniform_symbol_selected = false;
			}
		}
		
		if (symbol->getType() == Symbol::Point)
		{
			PointSymbol* point_symbol = static_cast<PointSymbol*>(symbol);
			if (point_symbol->isRotatable())
				have_rotatable_point = true;
		}
		
		if (symbol->getContainedTypes() & Symbol::Line)
			have_line = true;
		if (symbol->getContainedTypes() & Symbol::Area)
		{
			have_area = true;
			++num_selected_areas;
			if (!have_two_same_symbol_areas)
			{
				for (Map::ObjectSelection::const_iterator it2 = map->selectedObjectsBegin(); it2 != it; ++it2)
				{
					if ((*it2)->getSymbol() == symbol)
					{
						have_two_same_symbol_areas = true;
						break;
					}
				}
			}
		}
	}
	
	map->determineSymbolUseClosure(symbols_in_selection);
	for (size_t i = 0, end = symbols_in_selection.size(); i < end; ++i)
	{
		if (!symbols_in_selection[i])
			continue;
		
		Symbol* symbol = map->getSymbol(i);
		if (symbol->getType() == Symbol::Area)
		{
			AreaSymbol* area_symbol = static_cast<AreaSymbol*>(symbol);
			if (area_symbol->hasRotatableFillPattern())
			{
				have_rotatable_pattern = true;
				break;
			}
		}
	}
	
	cut_act->setEnabled(have_selection);
	copy_act->setEnabled(have_selection);
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
	cut_hole_circle_act->setEnabled(cut_hole_act->isEnabled());
	cut_hole_circle_act->setStatusTip(cut_hole_act->statusTip());
	cut_hole_rectangle_act->setEnabled(cut_hole_act->isEnabled());
	cut_hole_rectangle_act->setStatusTip(cut_hole_act->statusTip());
	cut_hole_menu->setEnabled(cut_hole_act->isEnabled());
	rotate_act->setEnabled(have_selection);
	rotate_act->setStatusTip(tr("Rotate the selected object(s).") + (rotate_act->isEnabled() ? "" : (" " + tr("Select at least one object to activate this tool."))));
	rotate_pattern_act->setEnabled(have_rotatable_pattern || have_rotatable_point);
	rotate_pattern_act->setStatusTip(tr("Set the direction of area fill patterns or point objects.") + (rotate_pattern_act->isEnabled() ? "" : (" " + tr("Select an area object with rotatable fill pattern or a rotatable point object to activate this tool."))));
	scale_act->setEnabled(have_selection);
	scale_act->setStatusTip(tr("Scale the selected object(s).") + (scale_act->isEnabled() ? "" : (" " + tr("Select at least one object to activate this tool."))));
	boolean_union_act->setEnabled(have_two_same_symbol_areas);
	boolean_union_act->setStatusTip(tr("Unify overlapping areas.") + (boolean_union_act->isEnabled() ? "" : (" " + tr("Select at least two area objects with the same symbol to activate this tool."))));
	boolean_intersection_act->setEnabled(have_two_same_symbol_areas && uniform_symbol_selected);
	boolean_intersection_act->setStatusTip(tr("Intersect the first selected area object with all other selected overlapping areas.") + (boolean_intersection_act->isEnabled() ? "" : (" " + tr("Select at least two area objects with the same symbol to activate this tool."))));
	boolean_difference_act->setEnabled(num_selected_areas >= 2);
	boolean_difference_act->setStatusTip(tr("Subtract all other selected area objects from the first selected area object.") + (boolean_difference_act->isEnabled() ? "" : (" " + tr("Select at least two area objects to activate this tool."))));
	boolean_xor_act->setEnabled(have_two_same_symbol_areas && uniform_symbol_selected);
	boolean_xor_act->setStatusTip(tr("Calculate nonoverlapping parts of areas.") + (boolean_xor_act->isEnabled() ? "" : (" " + tr("Select at least two area objects with the same symbol to activate this tool."))));
	convert_to_curves_act->setEnabled(have_area || have_line);
	convert_to_curves_act->setStatusTip(tr("Turn paths made of straight segments into smooth bezier splines.") + (convert_to_curves_act->isEnabled() ? "" : (" " + tr("Select a path object to activate this tool."))));
	simplify_path_act->setEnabled(have_area || have_line);
	simplify_path_act->setStatusTip(tr("Reduce the number of points in path objects while trying to retain their shape.") + (convert_to_curves_act->isEnabled() ? "" : (" " + tr("Select a path object to activate this tool."))));
	
	// Automatic symbol selection of selected objects
	if (symbol_widget && uniform_symbol_selected && Settings::getInstance().getSettingCached(Settings::MapEditor_ChangeSymbolWhenSelecting).toBool())
		symbol_widget->selectSingleSymbol(uniform_symbol);

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
	clear_undo_redo_history_act->setEnabled(undo_act->isEnabled() || redo_act->isEnabled());
}

void MapEditorController::clipboardChanged(QClipboard::Mode mode)
{
	if (mode == QClipboard::Clipboard)
		updatePasteAvailability();
}

void MapEditorController::updatePasteAvailability()
{
	if (paste_act)
		paste_act->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("openorienteering/objects") && !editing_in_progress);
}

void MapEditorController::showWholeMap()
{
	QRectF map_extent = map->calculateExtent(true, true, main_view);
	map_widget->adjustViewToRect(map_extent, false);
}

void MapEditorController::editToolClicked()
{
	setEditTool();
}

void MapEditorController::editLineToolClicked()
{
	if (!current_tool || current_tool->getType() != MapEditorTool::EditLine)
		setTool(new EditLineTool(this, edit_line_tool_act, symbol_widget));
}

void MapEditorController::drawPointClicked()
{
	setTool(new DrawPointTool(this, draw_point_act, symbol_widget));
}

void MapEditorController::drawPathClicked()
{
	setTool(new DrawPathTool(this, draw_path_act, symbol_widget, true));
}

void MapEditorController::drawCircleClicked()
{
	setTool(new DrawCircleTool(this, draw_circle_act, symbol_widget));
}

void MapEditorController::drawRectangleClicked()
{
	setTool(new DrawRectangleTool(this, draw_rectangle_act, symbol_widget));
}

void MapEditorController::drawTextClicked()
{
	setTool(new DrawTextTool(this, draw_text_act, symbol_widget));
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
	MapPart* part = map->getCurrentPart();
	
	map->clearObjectSelection(false);
	for (int i = 0; i < (int)new_objects.size(); ++i)
	{
		undo_step->addObject(part->findObjectIndex(new_objects[i]));
		map->addObjectToSelection(new_objects[i], i == (int)new_objects.size() - 1);
	}
	
	map->objectUndoManager().addNewUndoStep(undo_step);
	setEditTool();
	window->statusBar()->showMessage(tr("%1 object(s) duplicated").arg((int)new_objects.size()), 2000);
}

void MapEditorController::switchSymbolClicked()
{
	SwitchSymbolUndoStep* switch_step = NULL;
	ReplaceObjectsUndoStep* replace_step = NULL;
	AddObjectsUndoStep* add_step = NULL;
	DeleteObjectsUndoStep* delete_step = NULL;
	std::vector<Object*> old_objects;
	std::vector<Object*> new_objects;
	MapPart* part = map->getCurrentPart();
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	
	bool close_paths = false, split_up = false;
	Symbol::Type contained_types = symbol->getContainedTypes();
	if (contained_types & Symbol::Area && !(contained_types & Symbol::Line))
		close_paths = true;
	else if (contained_types & Symbol::Line && !(contained_types & Symbol::Area))
		split_up = true;
	
	if (close_paths)
		replace_step = new ReplaceObjectsUndoStep(map);
	else if (split_up)
	{
		add_step = new AddObjectsUndoStep(map);
		delete_step = new DeleteObjectsUndoStep(map);
	}
	else
		switch_step = new SwitchSymbolUndoStep(map);
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* object = *it;
		
		if (close_paths)
			replace_step->addObject(part->findObjectIndex(object), object->duplicate());
		else if (split_up)
		{
			add_step->addObject(part->findObjectIndex(object), object);
			old_objects.push_back(object);
		}
		else
			switch_step->addObject(part->findObjectIndex(object), (object)->getSymbol());
		
		if (!split_up)
			object->setSymbol(symbol, true);
		if (object->getType() == Object::Path)
		{
			PathObject* path_object = object->asPath();
			if (close_paths)
				path_object->closeAllParts();
			else if (split_up)
			{
				for (int path_part = 0; path_part < path_object->getNumParts(); ++path_part)
				{
					PathObject* new_object = path_object->duplicatePart(path_part);
					new_object->setSymbol(symbol, true);
					new_objects.push_back(new_object);
				}
			}
		}
		object->update(true);
	}
	
	if (split_up)	
	{
		map->clearObjectSelection(false);
		for (size_t i = 0; i < old_objects.size(); ++i)
		{
			Object* object = old_objects[i];
			map->deleteObject(object, true);
		}
		for (size_t i = 0; i < new_objects.size(); ++i)
		{
			Object* object = new_objects[i];
			map->addObject(object);
			map->addObjectToSelection(object, i == new_objects.size() - 1);
		}
		// Do not merge this loop into the upper one;
		// theoretically undo step indices could be wrong this way.
		for (size_t i = 0; i < new_objects.size(); ++i)
		{
			Object* object = new_objects[i];
			delete_step->addObject(part->findObjectIndex(object));
		}
	}
	
	map->setObjectsDirty();
	if (close_paths)
		map->objectUndoManager().addNewUndoStep(replace_step);
	else if (split_up)
	{
		CombinedUndoStep* combined_step = new CombinedUndoStep((void*)map);
		combined_step->addSubStep(delete_step);
		combined_step->addSubStep(add_step);
		map->objectUndoManager().addNewUndoStep(combined_step);
	}
	else
		map->objectUndoManager().addNewUndoStep(switch_step);
	map->emitSelectionEdited();
}

void MapEditorController::fillBorderClicked()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	std::vector<Object*> new_objects;
	new_objects.reserve(map->getNumSelectedObjects());
	
	bool close_paths = false, split_up = false;
	Symbol::Type contained_types = symbol->getContainedTypes();
	if (contained_types & Symbol::Area && !(contained_types & Symbol::Line))
		close_paths = true;
	else if (contained_types & Symbol::Line && !(contained_types & Symbol::Area))
		split_up = true;
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* object = *it;
		if (split_up && object->getType() == Object::Path)
		{
			PathObject* path_object = object->asPath();
			for (int path_part = 0; path_part < path_object->getNumParts(); ++path_part)
			{
				PathObject* new_object = path_object->duplicatePart(path_part);
				new_object->setSymbol(symbol, true);
				map->addObject(new_object);
				new_objects.push_back(new_object);
			}
		}
		else
		{
			Object* duplicate = object->duplicate();
			duplicate->setSymbol(symbol, true);
			if (close_paths && duplicate->getType() == Object::Path)
			{
				PathObject* path_object = duplicate->asPath();
				path_object->closeAllParts();
			}
			map->addObject(duplicate);
			new_objects.push_back(duplicate);
		}
	}
	
	map->clearObjectSelection(false);
	for (int i = 0; i < (int)new_objects.size(); ++i)
	{
		map->addObjectToSelection(new_objects[i], i == (int)new_objects.size() - 1);
		undo_step->addObject(part->findObjectIndex(new_objects[i]));
	}
	map->objectUndoManager().addNewUndoStep(undo_step);
}
void MapEditorController::selectObjectsClicked()
{
	map->clearObjectSelection(false);
	
	MapPart* part = map->getCurrentPart();
	for (int i = 0, size = map->getNumObjects(); i < size; ++i)
	{
		Object* object = part->getObject(i);
		if (symbol_widget->isSymbolSelected(object->getSymbol()))
			map->addObjectToSelection(object, false);
	}
	
	map->emitSelectionChanged();
	
	if (map->getNumSelectedObjects() > 0)
		setEditTool();
	else
		QMessageBox::warning(window, QObject::tr("Object selection"), QObject::tr("No objects were selected because there are no objects with the selected symbol(s)"));
}

void MapEditorController::switchDashesClicked()
{
	SwitchDashesUndoStep* undo_step = new SwitchDashesUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line)
		{
			PathObject* path = reinterpret_cast<PathObject*>(*it);
			path->reverse();
			(*it)->update(true);
			
			undo_step->addObject(part->findObjectIndex(*it));
		}
	}
	
	map->setObjectsDirty();
	map->objectUndoManager().addNewUndoStep(undo_step);
	map->emitSelectionEdited();
}

float connectPaths_FindClosestEnd(const std::vector<Object*>& objects, PathObject* a, int a_index, int path_part_a, bool path_part_a_begin, PathObject** out_b, int* out_b_index, int* out_path_part_b, bool* out_path_part_b_begin)
{
	float best_dist_sq = std::numeric_limits<float>::max();
	for (int i = a_index; i < (int)objects.size(); ++i)
	{
		PathObject* b = reinterpret_cast<PathObject*>(objects[i]);
		if (b->getSymbol() != a->getSymbol())
			continue;
		
		int num_parts = b->getNumParts();
		for (int path_part_b = (a == b) ? path_part_a : 0; path_part_b < num_parts; ++path_part_b)
		{
			PathObject::PathPart& part = b->getPart(path_part_b);
			if (!part.isClosed())
			{
				for (int begin = 0; begin < 2; ++begin)
				{
					bool path_part_b_begin = (begin == 0);
					if (a == b && path_part_a == path_part_b && path_part_a_begin == path_part_b_begin)
						continue;
					
					MapCoord& coord_a = a->getCoordinate(path_part_a_begin ? a->getPart(path_part_a).start_index : a->getPart(path_part_a).end_index);
					MapCoord& coord_b = b->getCoordinate(path_part_b_begin ? b->getPart(path_part_b).start_index : b->getPart(path_part_b).end_index);
					float distance_sq = coord_a.lengthSquaredTo(coord_b);
					if (distance_sq < best_dist_sq)
					{
						best_dist_sq = distance_sq;
						*out_b = b;
						*out_b_index = i;
						*out_path_part_b = path_part_b;
						*out_path_part_b_begin = path_part_b_begin;
					}
				}
			}
		}
	}
	return best_dist_sq;
}

void MapEditorController::connectPathsClicked()
{
	std::vector<Object*> objects;
	std::vector<Object*> undo_objects;
	std::vector<Object*> deleted_objects;
	
	// Collect all objects in question
	objects.reserve(map->getNumSelectedObjects());
	undo_objects.reserve(map->getNumSelectedObjects());
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line && (*it)->getType() == Object::Path)
		{
			(*it)->update(false);
			objects.push_back(*it);
			undo_objects.push_back(NULL);
		}
	}
	
	AddObjectsUndoStep* add_step = NULL;
	MapPart* part = map->getCurrentPart();
	
	while (true)
	{
		// Find the closest pair of open ends of objects with the same symbol,
		// which is closer than a threshold
		PathObject* best_object_a;
		PathObject* best_object_b;
		int best_object_a_index;
		int best_object_b_index;
		int best_part_a;
		int best_part_b;
		bool best_part_a_begin;
		bool best_part_b_begin;
		float best_dist_sq = std::numeric_limits<float>::max();
		
		for (int i = 0; i < (int)objects.size(); ++i)
		{
			PathObject* a = reinterpret_cast<PathObject*>(objects[i]);
			
			// Choose connection threshold as maximum of 0.35mm, 1.5 * largest line extent, and 6 pixels
			// TODO: instead of 6 pixels, use a physical size as soon as screen dpi is in the settings
			float close_distance_sq = qMax(0.35f, 1.5f * a->getSymbol()->calculateLargestLineExtent(map));
			close_distance_sq = qMax(close_distance_sq, 0.001f * main_view->pixelToLength(6));
			close_distance_sq = qPow(close_distance_sq, 2);
			
			int num_parts = a->getNumParts();
			for (int path_part_a = 0; path_part_a < num_parts; ++path_part_a)
			{
				PathObject::PathPart& part = a->getPart(path_part_a);
				if (!part.isClosed())
				{
					PathObject* b;
					int b_index;
					int path_part_b;
					bool path_part_b_begin;
					float distance_sq;
					
					for (int begin = 0; begin < 2; ++begin)
					{
						bool path_part_a_begin = (begin == 0);
						distance_sq = connectPaths_FindClosestEnd(objects, a, i, path_part_a, path_part_a_begin, &b, &b_index, &path_part_b, &path_part_b_begin);
						if (distance_sq <= close_distance_sq && distance_sq < best_dist_sq)
						{
							best_dist_sq = distance_sq;
							best_object_a = a;
							best_object_b = b;
							best_object_a_index = i;
							best_object_b_index = b_index;
							best_part_a = path_part_a;
							best_part_b = path_part_b;
							best_part_a_begin = path_part_a_begin;
							best_part_b_begin = path_part_b_begin;
						}
					}
				}
			}
		}
		
		// Abort if no possible connections found
		if (best_dist_sq == std::numeric_limits<float>::max())
			break;
		
		// Create undo objects for a and b
		if (!undo_objects[best_object_a_index])
			undo_objects[best_object_a_index] = best_object_a->duplicate();
		if (!undo_objects[best_object_b_index])
			undo_objects[best_object_b_index] = best_object_b->duplicate();
		
		// Connect the best parts
		if (best_part_a_begin && best_part_b_begin)
		{
			best_object_b->reversePart(best_part_b);
			best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, true);
		}
		else if (best_part_a_begin && !best_part_b_begin)
		{
			if (best_object_a == best_object_b && best_part_a == best_part_b)
				best_object_a->getPart(best_part_a).connectEnds();
			else
				best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, true);
		}
		else if (!best_part_a_begin && best_part_b_begin)
		{
			if (best_object_a == best_object_b && best_part_a == best_part_b)
				best_object_a->getPart(best_part_a).connectEnds();
			else
				best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, false);
		}
		else //if (!best_part_a_begin && !best_part_b_begin)
		{
			best_object_b->reversePart(best_part_b);
			best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, false);
		}
		
		if (best_object_a != best_object_b)
		{
			// Copy all remaining parts of object b over to a
			best_object_a->getCoordinate(best_object_a->getCoordinateCount() - 1).setHolePoint(true);
			for (int i = 0; i < best_object_b->getNumParts(); ++i)
			{
				if (i != best_part_b)
					best_object_a->appendPathPart(best_object_b, i);
			}
			
			// Create an add step for b
			int b_index_in_part = part->findObjectIndex(best_object_b);
			deleted_objects.push_back(best_object_b);
			if (!add_step)
				add_step = new AddObjectsUndoStep(map);
			add_step->addObject(b_index_in_part, undo_objects[best_object_b_index]);
			undo_objects[best_object_b_index] = NULL;
			
			// Delete b from the active list
			objects.erase(objects.begin() + best_object_b_index);
			undo_objects.erase(undo_objects.begin() + best_object_b_index);
		}
		else
		{
			// Delete the part which has been merged with another part
			if (best_part_a != best_part_b)
				best_object_a->deletePart(best_part_b);
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
			replace_step->addObject(part->findObjectIndex(objects[i]), undo_objects[i]);
		}
	}
	
	if (add_step)
	{
		for (int i = 0; i < (int)deleted_objects.size(); ++i)
		{
			map->removeObjectFromSelection(deleted_objects[i], false);
			map->getCurrentPart()->deleteObject(deleted_objects[i], false);
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
	map->emitSelectionEdited();
}
void MapEditorController::cutClicked()
{
	setTool(new CutTool(this, cut_tool_act));
}
void MapEditorController::cutHoleClicked()
{
	setTool(new CutHoleTool(this, cut_hole_act, PathObject::Path));
}

void MapEditorController::cutHoleCircleClicked()
{
	setTool(new CutHoleTool(this, cut_hole_circle_act, PathObject::Circle));
}

void MapEditorController::cutHoleRectangleClicked()
{
	setTool(new CutHoleTool(this, cut_hole_rectangle_act, PathObject::Rect));
}

void MapEditorController::rotateClicked()
{
	setTool(new RotateTool(this, rotate_act));
}

void MapEditorController::rotatePatternClicked()
{
	setTool(new RotatePatternTool(this, rotate_pattern_act));
}

void MapEditorController::scaleClicked()
{
	setTool(new ScaleTool(this, scale_act));
}

void MapEditorController::measureClicked(bool checked)
{
	if (!measure_dock_widget)
	{
		measure_dock_widget = new EditorDockWidget(tr("Measure"), measure_act, this, window);
		MeasureWidget* measure_widget = new MeasureWidget(map);
		measure_dock_widget->setWidget(measure_widget);
		measure_dock_widget->setObjectName("Measure dock widget");
		if (!window->restoreDockWidget(measure_dock_widget))
			addFloatingDockWidget(measure_dock_widget);
	}
	
	measure_dock_widget->setVisible(checked);
	if (checked)
		QTimer::singleShot(0, measure_dock_widget, SLOT(raise()));
}

void MapEditorController::booleanUnionClicked()
{
	BooleanTool tool(map);
	if (!tool.execute(BooleanTool::Union))
		QMessageBox::warning(window, QObject::tr("Error"), QObject::tr("Unification failed."));
}

void MapEditorController::booleanIntersectionClicked()
{
	BooleanTool tool(map);
	if (!tool.execute(BooleanTool::Intersection))
		QMessageBox::warning(window, QObject::tr("Error"), QObject::tr("Intersection failed."));
}

void MapEditorController::booleanDifferenceClicked()
{
	BooleanTool tool(map);
	if (!tool.execute(BooleanTool::Difference))
		QMessageBox::warning(window, QObject::tr("Error"), QObject::tr("Difference failed."));
}

void MapEditorController::booleanXOrClicked()
{
	BooleanTool tool(map);
	if (!tool.execute(BooleanTool::XOr))
		QMessageBox::warning(window, QObject::tr("Error"), QObject::tr("XOr failed."));
}

void MapEditorController::convertToCurvesClicked()
{
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getType() != Object::Path)
			continue;
		
		PathObject* path = (*it)->asPath();
		PathObject* undo_duplicate = NULL;
		if (path->convertToCurves(&undo_duplicate))
		{
			undo_step->addObject(part->findObjectIndex(path), undo_duplicate);
			path->simplify();
		}
		path->update(true);
	}
	
	if (undo_step->isEmpty())
		delete undo_step;
	else
	{
		map->setObjectsDirty();
		map->objectUndoManager().addNewUndoStep(undo_step);
		map->emitSelectionEdited();
	}
}

void MapEditorController::simplifyPathClicked()
{
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getType() != Object::Path)
			continue;
		
		PathObject* path = (*it)->asPath();
		PathObject* undo_duplicate = NULL;
		if (path->simplify(&undo_duplicate))
			undo_step->addObject(part->findObjectIndex(path), undo_duplicate);
		path->update(true);
	}
	
	if (undo_step->isEmpty())
		delete undo_step;
	else
	{
		map->setObjectsDirty();
		map->objectUndoManager().addNewUndoStep(undo_step);
		map->emitSelectionEdited();
	}
}

void MapEditorController::addFloatingDockWidget(QDockWidget* dock_widget)
{
	if (!window->restoreDockWidget(dock_widget))
	{
		dock_widget->setFloating(true);
		dock_widget->move(window->geometry().left() + 40, window->geometry().top() + 100);
		connect(dock_widget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), SLOT(saveWindowState()));
	}
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
			// TODO: check for visibility too?!
			if (map->getTemplate(i)->canBeDrawnOnto() && map->getTemplate(i)->getTemplateState() != Template::Invalid)
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
		if (symbol_widget)
			map->disconnect(symbol_widget);
	}
	
	this->map = map;
	if (create_new_map_view)
		main_view = new MapView(map);
	
	connect(&map->objectUndoManager(), SIGNAL(undoStepAvailabilityChanged()), this, SLOT(undoStepAvailabilityChanged()));
	connect(map, SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	connect(map, SIGNAL(templateAdded(int,Template*)), this, SLOT(templateAdded(int,Template*)));
	connect(map, SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
	connect(map, SIGNAL(closedTemplateAvailabilityChanged()), this, SLOT(closedTemplateAvailabilityChanged()));
	if (symbol_widget)
		connect(map, SIGNAL(symbolChanged(int,Symbol*,Symbol*)), symbol_widget, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	
	if (window)
		updateWidgets();
}

void MapEditorController::updateWidgets()
{
	undoStepAvailabilityChanged();
	objectSelectionChanged();
	if (mode == MapEditor)
	{
		if (main_view)
		{
			show_grid_act->setChecked(main_view->isGridVisible());
			hide_all_templates_act->setChecked(main_view->areAllTemplatesHidden());
			overprinting_simulation_act->setChecked(main_view->isOverprintingSimulationEnabled());
		}
		if (map)
		{
			hatch_areas_view_act->setChecked(map->isAreaHatchingEnabled());
			baseline_view_act->setChecked(map->isBaselineViewEnabled());
			closedTemplateAvailabilityChanged();
		}
	}
}

void MapEditorController::importClicked()
{
	QSettings settings;
	QString import_directory = settings.value("importFileDirectory", QDir::homePath()).toString();
	
	QString map_names = "";
	QString map_extensions = "";
	Q_FOREACH(const FileFormat *format, FileFormats.formats())
	{
		if (!format->supportsImport())
			continue;
		
		if (!map_extensions.isEmpty())
		{
			map_names = map_names + ", ";
			map_extensions = map_extensions + " ";
		}
		
		// FIXME: primaryExtension is incomplete, but fileExtensions may produce redundant entries
		map_names = map_names + format->primaryExtension().toUpper();
		map_extensions = map_extensions + "*." % format->fileExtensions().join(" *.");
	}
	
	QString filename = QFileDialog::getOpenFileName(window, tr("Import %1, GPX, OSM or DXF file").arg(map_names), import_directory, QString("%1 (%2 *.gpx *.osm *.dxf);;%3 (*.*)").arg(tr("Importable files")).arg(map_extensions).arg(tr("All files")));
	if (filename.isEmpty() || filename.isNull())
		return;
	
	settings.setValue("importFileDirectory", QFileInfo(filename).canonicalPath());
	
	if ( filename.endsWith(".dxf", Qt::CaseInsensitive) || 
	     filename.endsWith(".gpx", Qt::CaseInsensitive) ||
	     filename.endsWith(".osm", Qt::CaseInsensitive) )
	{
		importGeoFile(filename);
	}
	else if (filename.endsWith(".ocd", Qt::CaseInsensitive) || 
			 filename.endsWith(".omap", Qt::CaseInsensitive) ||
			 filename.endsWith(".xmap", Qt::CaseInsensitive))
	{
		importMapFile(filename);
	}
	else
	{
		QMessageBox::critical(window, tr("Error"), tr("Cannot import the selected file because its file format is not supported."));
	}
}

void MapEditorController::importGeoFile(const QString& filename)
{
	TemplateTrack temp(filename, map);
	if (!temp.configureAndLoad(window, main_view))
		return;
	temp.import(window);
}

bool MapEditorController::importMapFile(const QString& filename)
{
	Map* imported_map = new Map();
	bool result = imported_map->loadFrom(filename);
	if (!result)
	{
		QMessageBox::critical(window, tr("Error"), tr("Cannot import the selected map file because it could not be loaded."));
		return false;
	}
	
	map->importMap(imported_map, Map::MinimalObjectImport, window);
	
	delete imported_map;
	return true;
}


// ### EditorDockWidget ###

EditorDockWidget::EditorDockWidget(const QString title, QAction* action, MapEditorController* editor, QWidget* parent): QDockWidget(title, parent), action(action), editor(editor)
{
	if (editor)
		connect(this, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), editor, SLOT(saveWindowState()));
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
	QDockWidget::closeEvent(event);
}


// ### MapEditorToolAction ###

MapEditorToolAction::MapEditorToolAction(const QIcon& icon, const QString& text, QObject* parent)
 : QAction(icon, text, parent)
{
	setCheckable(true);
	connect(this, SIGNAL(triggered(bool)), this, SLOT(triggeredImpl(bool)));
}

void MapEditorToolAction::triggeredImpl(bool checked)
{
	if (checked)
		emit(activated());
	else
		setChecked(true);
}
