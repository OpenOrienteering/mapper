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


#ifndef _OPENORIENTEERING_SYMBOL_RENDER_WIDGET_H_
#define _OPENORIENTEERING_SYMBOL_RENDER_WIDGET_H_

#include <set>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
class QMenu;
QT_END_NAMESPACE

class Map;
class Symbol;
class SymbolToolTip;
class SymbolWidget;

/**
 * @brief Displays a list of symbols.
 * 
 * Internal class used in SymbolWidget.
 */
class SymbolRenderWidget : public QWidget
{
Q_OBJECT
public:
	SymbolRenderWidget(Map* map, bool mobile_mode, QScrollBar* scroll_bar, SymbolWidget* parent);
	
	QScrollArea* makeScrollArea(QWidget* parent = NULL);
	
	SymbolToolTip* getSymbolToolTip() const;
	
	/** Returns the number of selected symbols. */
	int getNumSelectedSymbols() const;
	
	/**
	 * If exactly one symbol is selected, returns this symbol,
	 * otherwise returns NULL.
	 */
	Symbol* getSingleSelectedSymbol() const;
	
	/** Checks if the symbol is selected. */
	bool isSymbolSelected(Symbol* symbol) const;
	
	/**
	 * Returns the single "current" symbol (the symbol which was clicked last).
	 * Can be -1 if no symbol selected.
	 */
	inline int currentSymbolIndex() const {return current_symbol_index;}
	
	/**
	 * @brief Selects the given symbol.
	 * 
	 * Deselects other symbols, if there was a different selection before.
	 */
	void selectSingleSymbol(Symbol *symbol);
	
	/**
	 * Selects the symbol with the given number. Deselects other symbols,
	 * if there was a different selection before.
	 */
	void selectSingleSymbol(int i);
	
	/**
	 * @brief Returns the recommended size for the widget.
	 * 
	 * Reimplementation of QWidget::sizeHint().
	 */
	virtual QSize sizeHint() const;
	
public slots:
	/** Repaints the icon with the given index. */
	void updateIcon(int i);
	
	/** Updates the range of the scroll bar, if there is one. */
	void adjustHeight();
	
protected:
	// Used to update actions in the context menu
	void updateContextMenuState();
	
protected slots:
	void newPointSymbol();
	void newLineSymbol();
	void newAreaSymbol();
	void newTextSymbol();
	void newCombinedSymbol();
	void editSymbol();
	void scaleSymbol();
	void deleteSymbols();
	void duplicateSymbol();
	void copySymbols();
	void pasteSymbols();
	void setSelectedSymbolVisibility(bool checked);
	void setSelectedSymbolProtection(bool checked);
	void selectAll();
	void selectUnused();
	void invertSelection();
	void sortByNumber();
	void sortByColor();
	void sortByColorPriority();
	
protected:
	int current_symbol_index;
	int hover_symbol_index;
	std::set<int> selected_symbols;
	
	QPoint last_click_pos;
	int last_drop_pos;
	int last_drop_row;
	int last_click_scroll_value;
	bool dragging;
	
	QScrollBar* scroll_bar;
	SymbolWidget* symbol_widget;
	QMenu* context_menu;
	QAction* edit_action;
	QAction* scale_action;
	QAction* copy_action;
	QAction* paste_action;
	QAction* switch_symbol_action;
	QAction* fill_border_action;
	QAction* hide_action;
	QAction* protect_action;
	QAction* duplicate_action;
	QAction* delete_action;
	QAction* select_objects_action;
	QAction* select_objects_additionally_action;
	
	bool mobile_mode;
	SymbolToolTip* tooltip;
	Map* map;
	QSize preferred_size;
	
	bool isSymbolSelected(int i) const;
	void getSelectionBitfield(std::vector<bool>& out) const;
	void setSelectionBitfield(std::vector<bool>& in);
	
	bool newSymbol(Symbol* prototype);
	
	void mouseMove(int x, int y);
	int getSymbolIndexAt(int x, int y);
	QRect getIconRect(int i);
	void updateSelectedIcons();
	void getRowInfo(int width, int height, int& icons_per_row, int& num_rows);
	bool getDropPosition(QPoint pos, int& row, int& pos_in_row);
	QRect getDragIndicatorRect(int row, int pos_in_row);
	
	template<typename T> void sort(T compare);
	
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void leaveEvent(QEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dragMoveEvent(QDragMoveEvent* event);
	virtual void dropEvent(QDropEvent* event);
};

#endif
