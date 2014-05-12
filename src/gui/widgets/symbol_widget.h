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


#ifndef _OPENORIENTEERING_SYMBOL_WIDGET_H_
#define _OPENORIENTEERING_SYMBOL_WIDGET_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QHBoxLayout;
QT_END_NAMESPACE

class Map;
class Symbol;
class SymbolRenderWidget;

/**
 * @brief Combines SymbolRenderWidget and a scroll bar.
 * 
 * SymbolWidget shows all symbols from a map's symbol set
 * and lets the user select symbols.
 * 
 * Normally it is used inside a dock widget.
 */
class SymbolWidget : public QWidget
{
Q_OBJECT
public:
	SymbolWidget(Map* map, bool mobile_mode, QWidget* parent = NULL);
	virtual ~SymbolWidget();
	
	/**
	 * If exactly one symbol is selected, returns this symbol,
	 * otherwise returns NULL.
	 */
	Symbol* getSingleSelectedSymbol() const;
	
	/** Returns the number of selected symbols. */
	int getNumSelectedSymbols() const;
	
	/** Checks if the symbol is selected. */
	bool isSymbolSelected(Symbol* symbol) const;
	
	/** Selects the symbol exclusively, deselecting all other symbols. */
	void selectSingleSymbol(Symbol *symbol);
	
	virtual QSize sizeHint() const;
	
	inline void emitSelectedSymbolsChanged() {emit selectedSymbolsChanged();}
	inline SymbolRenderWidget* getRenderWidget() const {return render_widget;}
	
public slots:
	/** Adjusts the widget contents to its size. */
	void adjustContents();
	
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol = NULL);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
	void emitSwitchSymbolClicked() {emit switchSymbolClicked();}
	void emitFillBorderClicked() {emit fillBorderClicked();}
	void emitSelectObjectsClicked() {emit selectObjectsClicked(true);}
	void emitSelectObjectsAdditionallyClicked() {emit selectObjectsClicked(false);}
	
signals:
	void selectedSymbolsChanged();
	
	void switchSymbolClicked();
	void fillBorderClicked();
	void selectObjectsClicked(bool select_exclusively);
	
protected:
	virtual void resizeEvent(QResizeEvent* event);
	
private:
	SymbolRenderWidget* render_widget;
	QScrollBar* scroll_bar;
	
	QHBoxLayout* layout;
	QSize preferred_size;
	
	Map* map;
};

#endif
