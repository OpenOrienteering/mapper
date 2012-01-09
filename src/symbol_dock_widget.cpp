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


#include "symbol_dock_widget.h"

#include <assert.h>

#include <QtGui>

#include "map.h"
#include "symbol_point.h"
#include "symbol_setting_dialog.h"

// ### SymbolRenderWidget ###

SymbolRenderWidget::SymbolRenderWidget(QScrollBar* scroll_bar, SymbolWidget* parent) : QWidget(parent), scroll_bar(scroll_bar)
{
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	
	context_menu = new QMenu(this);
	
	QMenu* new_menu = new QMenu(tr("New"), context_menu);
	/*QAction* new_point_action =*/ new_menu->addAction(tr("Point symbol"), parent, SLOT(newPointSymbol()));
	QAction* new_line_action = new_menu->addAction(tr("Line symbol"));
	new_line_action->setEnabled(false);
	QAction* new_area_action = new_menu->addAction(tr("Area symbol"));
	new_area_action->setEnabled(false);
	QAction* new_text_action = new_menu->addAction(tr("Text symbol"));
	new_text_action->setEnabled(false);
	QAction* new_combined_action = new_menu->addAction(tr("Combined symbol"));
	new_combined_action->setEnabled(false);
	
	context_menu->addMenu(new_menu);
	/*QAction* duplicate_action =*/ context_menu->addAction(tr("Duplicate"), parent, SLOT(duplicateSymbol()));
	/*QAction* delete_action =*/ context_menu->addAction(tr("Delete"), parent, SLOT(deleteSymbol()));
}

bool SymbolRenderWidget::scrollBarNeeded(int width, int height)
{
	// TODO
	
	return false;
}
void SymbolRenderWidget::setScrollBar(QScrollBar* new_scroll_bar)
{
	// TODO
	
	scroll_bar = new_scroll_bar;
}

void SymbolRenderWidget::setScroll(int new_scroll)
{
	// TODO
}

void SymbolRenderWidget::mouseMove(int x, int y)
{

}

void SymbolRenderWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter;
	painter.begin(this);
	
	QRect rect = event->rect().intersected(QRect(0, 0, width(), height()));
	painter.setClipRect(rect);
	
	painter.fillRect(rect, Qt::white);
	// TODO
	
	painter.end();
}
void SymbolRenderWidget::mouseMoveEvent(QMouseEvent* event)
{
	mouseMove(event->x(), event->y());
	event->accept();
}
void SymbolRenderWidget::mousePressEvent(QMouseEvent* event)
{
	// TODO: set "current symbol"
	
	if (event->button() == Qt::RightButton)
	{
		context_menu->popup(event->globalPos());
		event->accept();
	}
	
	// event->modifiers() & Qt::ShiftModifier
	// TODO
}
void SymbolRenderWidget::leaveEvent(QEvent* event)
{
	// TODO
}
void SymbolRenderWidget::wheelEvent(QWheelEvent* event)
{
	if (scroll_bar)
	{
		scroll_bar->event(event);
		mouseMove(event->x(), event->y());
		if (event->isAccepted())
			return;
	}
	QWidget::wheelEvent(event);
}

// ### SymbolWidget ###

SymbolWidget::SymbolWidget(Map* map, QWidget* parent): QWidget(parent), map(map)
{
	no_resize_handling = false;
	
	scroll_bar = new QScrollBar();
	scroll_bar->hide();
	scroll_bar->setOrientation(Qt::Vertical);
	render_widget = new SymbolRenderWidget(scroll_bar, this);
	
	// Load settings
	QSettings settings;
	settings.beginGroup("SymbolWidget");
	preferred_size = settings.value("size", QSize(200, 500)).toSize();
	settings.endGroup();
	
	// Create layout
	layout = new QHBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(render_widget, 1);
	layout->addWidget(scroll_bar);
	setLayout(layout);
	
	// Connections
	connect(scroll_bar, SIGNAL(valueChanged(int)), render_widget, SLOT(setScroll(int)));
}
SymbolWidget::~SymbolWidget()
{
	// Save settings
	QSettings settings;
	settings.beginGroup("SymbolWidget");
	settings.setValue("size", size());
	settings.endGroup();
}
QSize SymbolWidget::sizeHint() const
{
	return preferred_size;
}

void SymbolWidget::resizeEvent(QResizeEvent* event)
{
	if (no_resize_handling)
		return;
	
	int width = event->size().width();
	int height = event->size().height();
	
	// Do we need a scroll bar?
	bool scroll_needed = render_widget->scrollBarNeeded(width, height);
	if (scroll_bar->isVisible() && !scroll_needed)
	{
		scroll_bar->hide();
		render_widget->setScrollBar(NULL);
	}
	else if (!scroll_bar->isVisible() && scroll_needed)
	{
		scroll_bar->show();
		render_widget->setScrollBar(scroll_bar);
	}
	
	// TODO
	int new_width = 200;
	no_resize_handling = true;
	resize(new_width, height);
	no_resize_handling = false;
/*
 width = renderWindow->width();
 
 rest = width % symbolEntryWidth;
 if (rest == 0)
	 return;
 
 width -= rest;
 if (rest >= symbolEntryWidth/2)
	 width += symbolEntryWidth;
 
 if (scrollBar->isVisible())
	 width += scrollBar->width();
 
 noResizeHandling = true;
 preferredSize.setWidth(width);
 resize(width, height);
*/
	
    event->accept();
}

void SymbolWidget::newPointSymbol()
{
	PointSymbol* new_symbol = new PointSymbol();
	
	SymbolSettingDialog dialog(new_symbol, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Rejected)
		return;
	
	map->addSymbol(new_symbol, 0); //render_widget->getCurrentSymbolIndex()); TODO
	render_widget->update();
}
void SymbolWidget::deleteSymbol()
{
	// TODO
	
	map->setSymbolsDirty();
}
void SymbolWidget::duplicateSymbol()
{
	// TODO
	
	map->setSymbolsDirty();
}

#include "symbol_dock_widget.moc"
