/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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

#include "symbol_render_widget.h"


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
	connect(render_widget, SIGNAL(selectedSymbolsChanged()), this, SIGNAL(selectedSymbolsChanged()));
	connect(render_widget, SIGNAL(fillBorderClicked()), this, SIGNAL(fillBorderClicked()));
	connect(render_widget, SIGNAL(switchSymbolClicked()), this, SIGNAL(switchSymbolClicked()));
	connect(render_widget, SIGNAL(selectObjectsClicked(bool)), this, SIGNAL(selectObjectsClicked(bool)));
}

SymbolWidget::~SymbolWidget()
{
	; // nothing
}

Symbol* SymbolWidget::getSingleSelectedSymbol() const
{
	return render_widget->singleSelectedSymbol();
}

int SymbolWidget::selectedSymbolsCount() const
{
	return render_widget->selectedSymbolsCount();
}

bool SymbolWidget::isSymbolSelected(Symbol* symbol) const
{
	return render_widget->isSymbolSelected(symbol);
}

void SymbolWidget::selectSingleSymbol(Symbol *symbol)
{
    render_widget->selectSingleSymbol(symbol);
}
