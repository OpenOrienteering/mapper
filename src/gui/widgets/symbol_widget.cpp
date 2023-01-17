/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2023 Kai Pastor
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

#include <Qt>
#include <QPalette>
#include <QContextMenuEvent>

#include "symbol_render_widget.h"

// IWYU pragma: no_forward_declare QContextMenuEvent
// IWYU pragma: no_forward_declare QWidget


namespace OpenOrienteering {

SymbolWidget::SymbolWidget(Map* map, bool mobile_mode, QWidget* parent)
: QScrollArea(parent)
{
	render_widget = new SymbolRenderWidget(map, mobile_mode, this);
	setWidget(render_widget);
	
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setWidgetResizable(true);
	setBackgroundRole(QPalette::Base);
	
	// Relay render_widget signals
	connect(render_widget, &SymbolRenderWidget::selectedSymbolsChanged, this, &SymbolWidget::selectedSymbolsChanged);
	connect(render_widget, &SymbolRenderWidget::fillBorderClicked, this, &SymbolWidget::fillBorderClicked);
	connect(render_widget, &SymbolRenderWidget::switchSymbolClicked, this, &SymbolWidget::switchSymbolClicked);
	connect(render_widget, &SymbolRenderWidget::selectObjectsClicked, this, &SymbolWidget::selectObjectsClicked);
	connect(render_widget, &SymbolRenderWidget::deselectObjectsClicked, this, &SymbolWidget::deselectObjectsClicked);
}

SymbolWidget::~SymbolWidget()
{
	; // nothing
}

const Symbol* SymbolWidget::getSingleSelectedSymbol() const
{
	return static_cast<const SymbolRenderWidget*>(render_widget)->singleSelectedSymbol();
}

Symbol* SymbolWidget::getSingleSelectedSymbol()
{
	return render_widget->singleSelectedSymbol();
}

int SymbolWidget::selectedSymbolsCount() const
{
	return render_widget->selectedSymbolsCount();
}

bool SymbolWidget::isSymbolSelected(const Symbol* symbol) const
{
	return render_widget->isSymbolSelected(symbol);
}

void SymbolWidget::selectSingleSymbol(const Symbol* symbol)
{
    render_widget->selectSingleSymbol(symbol);
}

void SymbolWidget::selectMultipleSymbols(std::vector<int> const &symbols)
{
	render_widget->selectMultipleSymbols(symbols);
}

void SymbolWidget::contextMenuEvent(QContextMenuEvent* event)
{
	render_widget->showContextMenu(event->globalPos());
	event->accept();
}


}  // namespace OpenOrienteering
