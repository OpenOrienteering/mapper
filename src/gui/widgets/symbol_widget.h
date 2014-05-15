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


#ifndef _OPENORIENTEERING_SYMBOL_WIDGET_H_
#define _OPENORIENTEERING_SYMBOL_WIDGET_H_

#include <QScrollArea>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QHBoxLayout;
QT_END_NAMESPACE

class Map;
class Symbol;
class SymbolRenderWidget;

/**
 * @brief Shows all symbols from a map.
 * 
 * SymbolWidget shows all symbols from a map's symbol set.
 * It lets the user select symbols and perform actions on symbols.
 * 
 * The implementation is based on SymbolRenderWidget and
 * adds a scroll bar as needed.
 * 
 * Normally it is used inside a dock widget.
 */
class SymbolWidget : public QScrollArea
{
Q_OBJECT
public:
	/**
	 * @brief Constructs a new SymbolWidget.
	 * @param map The map which provides the symbols. Must not be NULL.
	 * @param mobile_mode If true, enables a special mode for mobile devices.
	 * @param parent The parent QWidget.
	 */
	SymbolWidget(Map* map, bool mobile_mode, QWidget* parent = NULL);
	
	/**
	 * @brief Destroys the SymbolWidget.
	 */
	virtual ~SymbolWidget();
	
	/**
	 * @brief If exactly one symbol is selected, returns this symbol.
	 * 
	 * Otherwise returns NULL.
	 */
	Symbol* getSingleSelectedSymbol() const;
	
	/**
	 * @brief Returns the number of selected symbols.
	 */
	int getNumSelectedSymbols() const;
	
	/**
	 * @brief Checks if the symbol is selected.
	 */
	bool isSymbolSelected(Symbol* symbol) const;
	
	/**
	 * @brief Selects the symbol exclusively, deselecting all other symbols.
	 */
	void selectSingleSymbol(Symbol *symbol);
	
	/**
	 * @brief Returns the recommended size for the widget.
	 * 
	 * Reimplementation of QWidget::sizeHint().
	 */
//	virtual QSize sizeHint() const;
	
public slots:
	/**
	 * @brief Adjusts the widget contents to its size.
	 */
//	void adjustContents();
	
	/**
	 * @brief Listens to changes of map symols.
	 */
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol = NULL);
	
	/**
	 * @brief Listens to deletion of map symbols.
	 */
	void symbolDeleted(int pos, Symbol* old_symbol);
	
	/**
	 * @brief Listens to changes of map symbol icons.
	 */
	void symbolIconChanged(int pos);
	
	/**
	 * @brief Emits selectObjectsClicked(true).
	 */
	void emitSelectObjectsExclusivelyClicked() {emit selectObjectsClicked(true);}
	
	/**
	 * @brief Emits selectObjectsClicked(false).
	 */
	void emitSelectObjectsAdditionallyClicked() {emit selectObjectsClicked(false);}
	
signals:
	/**
	 * @brief This signal indicicates a change of the selected symbol(s).
	 */
	void selectedSymbolsChanged();
	
	/**
	 * @brief This signal is triggered when the user activates "Switch symbol".
	 * @todo  Merge with/Reuse corresponding action in MapEditorController.
	 */
	void switchSymbolClicked();
	
	/**
	 * @brief This signal is triggered when the user activates "Fill/Create border".
	 * @todo  Merge with/Reuse corresponding action in MapEditorController.
	 */
	void fillBorderClicked();
	
	/**
	 * @brief This signal is triggered when the user selects symbols.
	 * @param select_exclusively It true, the new selection is a single symbol.
	 */
	void selectObjectsClicked(bool select_exclusively);
	
protected:
	/**
	 * @brief Receives widget resize events.
	 * 
	 * Reimplementation of QWidget::resizeEvent().
	 */
//	virtual void resizeEvent(QResizeEvent* event);
	
private:
	SymbolRenderWidget* render_widget;
//	QScrollBar* scroll_bar;
//	QSize preferred_size;
};

#endif
