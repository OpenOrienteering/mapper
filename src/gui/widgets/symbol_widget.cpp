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


#include "symbol_widget.h"

#include <QtWidgets>

#include "../../map.h"
#include "../../settings.h"
#include "../../symbol.h"
#include "symbol_render_widget.h"


SymbolWidget::SymbolWidget(Map* map, bool mobile_mode, QWidget* parent)
: QWidget(parent),
  map(map)
{
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	
	scroll_bar = new QScrollBar();
	scroll_bar->setEnabled(false);
	scroll_bar->hide();
	scroll_bar->setOrientation(Qt::Vertical);
	scroll_bar->setSingleStep(icon_size);
	scroll_bar->setPageStep(3 * icon_size);
	render_widget = new SymbolRenderWidget(map, mobile_mode, scroll_bar, this);
	
// 	// Load settings
// 	QSettings settings;
// 	settings.beginGroup("SymbolWidget");
// 	preferred_size = settings.value("size", QSize(200, 500)).toSize();
// 	settings.endGroup();
	preferred_size = QSize(6 * icon_size, 5 * icon_size);
	
	// Create layout
	layout = new QHBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
    layout->addWidget(render_widget);
	layout->addWidget(scroll_bar);
	setLayout(layout);
}

SymbolWidget::~SymbolWidget()
{
// 	// Save settings
// 	QSettings settings;
// 	settings.beginGroup("SymbolWidget");
// 	settings.setValue("size", size());
// 	settings.endGroup();
}

QSize SymbolWidget::sizeHint() const
{
	return preferred_size;
}

Symbol* SymbolWidget::getSingleSelectedSymbol() const
{
	return render_widget->getSingleSelectedSymbol();
}

int SymbolWidget::getNumSelectedSymbols() const
{
	return render_widget->getNumSelectedSymbols();
}

bool SymbolWidget::isSymbolSelected(Symbol* symbol) const
{
	return render_widget->isSymbolSelected(symbol);
}

void SymbolWidget::selectSingleSymbol(Symbol *symbol)
{
    int index = map->findSymbolIndex(symbol);
    if (index >= 0) render_widget->selectSingleSymbol(index);
}

void SymbolWidget::adjustContents()
{
	int width = this->width();
	int height = this->height();
	
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
	
	render_widget->updateScrollRange();
}

void SymbolWidget::resizeEvent(QResizeEvent* event)
{
	adjustContents();
	event->accept();
}

void SymbolWidget::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	Q_UNUSED(new_symbol);
	Q_UNUSED(old_symbol);
	render_widget->updateIcon(pos);
}

void SymbolWidget::symbolDeleted(int pos, Symbol* old_symbol)
{
	Q_UNUSED(pos);
	Q_UNUSED(old_symbol);
	render_widget->update();
}
