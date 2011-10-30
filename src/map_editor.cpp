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
#include "template_dock_widget.h"
#include "template.h"

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
	delete map;
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
	template_dock_widget = NULL;
	
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
		template_dock_widget->setWidget(new TemplateWidget(map, main_view, template_dock_widget));
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
		template_dock_widget->setWidget(new TemplateWidget(map, main_view, template_dock_widget));
		window->addDockWidget(Qt::RightDockWidgetArea, template_dock_widget, Qt::Vertical);
	}
	
	TemplateWidget* template_widget = reinterpret_cast<TemplateWidget*>(template_dock_widget->widget());
	template_widget->addTemplateAt(new_template, -1);
}

// ### MapWidget ###

MapWidget::MapWidget(QWidget* parent) : QWidget(parent)
{
	view = NULL;
	below_template_cache = NULL;
	above_template_cache = NULL;
	
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
}
MapWidget::~MapWidget()
{
	if (view)
		view->getMap()->removeMapWidget(this);
	
	delete below_template_cache;
	delete above_template_cache;
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

QRectF MapWidget::viewportToView(const QRect& input)
{
	return QRectF(input.left() - 0.5*width(), input.top() - 0.5*height(), input.width(), input.height());
}
QPointF MapWidget::viewportToView(QPoint input)
{
	return QPointF(input.x() - 0.5*width(), input.y() - 0.5*height());
}
QRectF MapWidget::viewToViewport(const QRectF& input)
{
	return QRectF(input.left() + 0.5*width(), input.top() + 0.5*height(), input.width(), input.height());
}
QRectF MapWidget::viewToViewport(const QRect& input)
{
	return QRectF(input.left() + 0.5*width(), input.top() + 0.5*height(), input.width(), input.height());
}
QPointF MapWidget::viewToViewport(QPoint input)
{
	return QPointF(input.x() + 0.5*width(), input.y() + 0.5*height());
}

void MapWidget::setTemplateCacheDirty(QRectF view_rect, bool front_cache)
{
	QRect* cache_dirty_rect = front_cache ? &above_template_cache_dirty_rect : &below_template_cache_dirty_rect;
	QRectF viewport_rect = viewToViewport(view_rect);
	QRect integer_rect = QRect(viewport_rect.left() - 1, viewport_rect.top() - 1, viewport_rect.width() + 2, viewport_rect.height() + 2);
	
	if (cache_dirty_rect->isValid())
		*cache_dirty_rect = cache_dirty_rect->united(integer_rect);
	else
		*cache_dirty_rect = integer_rect;
	
	update();
}

void MapWidget::paintEvent(QPaintEvent* event)
{
	// Draw on the widget
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	// Background color
	painter.fillRect(event->rect(), QColor(Qt::gray));	// TODO: do this more intelligently based on drag x/y
	
	// No colors defined? Provide a litte help message ... TODO: reactivate
	/*if (view && view->getMap()->getNumColors() == 0)
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
	else if (view && view->getMap()->getNumTemplates() == 0 && NoObjectsTODO)	// No templates defined?
	{
		QFont font = painter.font();
		font.setPointSize(2 * font.pointSize());
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tr("- Ready to draw -\nStart drawing or load a base map.\nTo load a base map, click\nTemplates -> Open template..."));
	}
	else if (view)
	{*/
		// Update all dirty caches
		// TODO: It would be an idea to do these updates in a background thread and use the old caches in the meantime
		if (below_template_cache_dirty_rect.isValid())
			updateTemplateCache(below_template_cache, below_template_cache_dirty_rect, 0, view->getMap()->getFirstFrontTemplate() - 1, true);
		if (above_template_cache_dirty_rect.isValid())
			updateTemplateCache(above_template_cache, above_template_cache_dirty_rect, view->getMap()->getFirstFrontTemplate(), view->getMap()->getNumTemplates() - 1, false);
		
		// Draw caches
		if (below_template_cache && view->getMap()->getFirstFrontTemplate() > 0)
			painter.drawImage(QPoint(0, 0), *below_template_cache, rect());
			//painter.drawPixmap(QPoint(view->getCameraDragX(), view->getCameraDragY()), *below_template_cache, rect());
		
		// TODO: Map cache
		
		if (above_template_cache && view->getMap()->getNumTemplates() - view->getMap()->getFirstFrontTemplate() > 0)
			painter.drawImage(QPoint(0, 0), *above_template_cache, rect());
			//painter.drawPixmap(QPoint(view->getCameraDragX(), view->getCameraDragY()), *above_template_cache, rect());
	//}
	
	painter.end();
}
void MapWidget::resizeEvent(QResizeEvent* event)
{
	if (below_template_cache && below_template_cache->size() != event->size())
	{
		delete below_template_cache;
		below_template_cache = NULL;
		below_template_cache_dirty_rect = rect();
	}
	if (above_template_cache && above_template_cache->size() != event->size())
	{
		delete above_template_cache;
		above_template_cache = NULL;
		above_template_cache_dirty_rect = rect();
	}
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

void MapWidget::updateTemplateCache(QImage*& cache, QRect& dirty_rect, int first_template, int last_template, bool use_background)
{
	if (first_template > last_template)
		return;	// no template visible
	
	if (!cache)
	{
		// Lazy allocation of cache image
		cache = new QImage(size(), QImage::Format_ARGB32_Premultiplied);
		dirty_rect = rect();
		
		//if (!use_background)
		//	cache->fill(QColor(Qt::transparent));
	}
	
	// Make sure not to use a bigger draw rect than necessary
	dirty_rect = dirty_rect.intersected(rect());
	
	// Start drawing
	QPainter painter;
	painter.begin(cache);
	painter.setClipRect(dirty_rect);
	
	// Fill with background color (TODO: make configurable)
	if (use_background)
		painter.fillRect(dirty_rect, Qt::white);
	else
	{
		QPainter::CompositionMode mode = painter.compositionMode();
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
		painter.fillRect(dirty_rect, Qt::transparent);
		painter.setCompositionMode(mode);
	}
	
	// Draw templates
	Map* map = view->getMap();
	
	painter.translate(width() / 2.0, height() / 2.0);
	view->applyTransform(&painter);
	QRectF base_view_rect = view->calculateViewedRect(viewportToView(dirty_rect));
	
	for (int i = first_template; i <= last_template; ++i)
	{
		Template* templ = map->getTemplate(i);
		float scale = view->getZoom() * std::max(templ->getTemplateScaleX(), templ->getTemplateScaleY());
		
		QRectF view_rect;
		if (templ->getTemplateRotation() != 0)
			view_rect = QRectF(-9e42, -9e42, 9e42, 9e42);	// TODO: transform base_view_rect (map coords) using template transform to template coords
		else
		{
			view_rect.setLeft((base_view_rect.x() / templ->getTemplateScaleX()) - templ->getTemplateX());
			view_rect.setTop((base_view_rect.y() / templ->getTemplateScaleY()) - templ->getTemplateY());
			view_rect.setRight((base_view_rect.right() / templ->getTemplateScaleX()) - templ->getTemplateX());
			view_rect.setBottom((base_view_rect.bottom() / templ->getTemplateScaleY()) - templ->getTemplateY());
		}
		
		painter.save();
		templ->applyTemplateTransform(&painter);
		templ->drawTemplate(&painter, view_rect, scale);
		painter.restore();
	}
	
	painter.end();
	
	dirty_rect.setWidth(-1);
	assert(!dirty_rect.isValid());
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
