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


#ifndef _OPENORIENTEERING_SYMBOL_DOCK_WIDGET_H_
#define _OPENORIENTEERING_SYMBOL_DOCK_WIDGET_H_

#include <set>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QHBoxLayout;
class QMenu;
class QLabel;
QT_END_NAMESPACE

class Map;
class Symbol;
class SymbolWidget;

class SymbolRenderWidget : public QWidget
{
Q_OBJECT
public:
	SymbolRenderWidget(Map* map, QScrollBar* scroll_bar, SymbolWidget* parent);
	
	inline bool scrollBarNeeded(int width, int height);
	void setScrollBar(QScrollBar* new_scroll_bar);
	
	int getNumSelectedSymbols() const;
	Symbol* getSingleSelectedSymbol() const;
	bool isSymbolSelected(Symbol* symbol) const;
	
	/// Returns the single "current" symbol (the symbol which was clicked last). Can be -1 if no symbol selected
	inline int currentSymbolIndex() const {return current_symbol_index;}
	
	/// Selects the symbol with the given number. Deselects other symbols, if there was a different selection before.
	void selectSingleSymbol(int i);
	
public slots:
	void updateIcon(int i);
	
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
	
	void setScroll(int new_scroll);
	
protected:
	int current_symbol_index;
	int hover_symbol_index;
	std::set<int> selected_symbols;
	
	QPoint last_click_pos;
	int last_drop_pos;
	int last_drop_row;
	
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
	
	Map* map;
	
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
	void updateScrollRange();
	
	template<typename T> void sort(T compare);
	
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void leaveEvent(QEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dragMoveEvent(QDragMoveEvent* event);
	virtual void dropEvent(QDropEvent* event);
};

/// Combines SymbolRenderWidget and a scroll bar to a symbol widget
class SymbolWidget : public QWidget
{
Q_OBJECT
public:
	SymbolWidget(Map* map, QWidget* parent = NULL);
	virtual ~SymbolWidget();
	
	/// Returns the selected symbol IF EXACTLY ONE symbol is selected, otherwise returns NULL
	Symbol* getSingleSelectedSymbol() const;
	int getNumSelectedSymbols() const;
	bool isSymbolSelected(Symbol* symbol) const;
	
	// Programmatic select of a symbol
	void selectSingleSymbol(Symbol *symbol);
	
	void adjustContents();
	virtual QSize sizeHint() const;
	
	inline void emitSelectedSymbolsChanged() {emit selectedSymbolsChanged();}
	inline SymbolRenderWidget* getRenderWidget() const {return render_widget;}
	
public slots:
	virtual void keyPressed(QKeyEvent* event);
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

class SymbolToolTip : public QWidget
{
Q_OBJECT
public:
	SymbolToolTip(Symbol* symbol, QRect icon_rect, QWidget* parent);
	void showDescription();
	
	static void showTip(QRect rect, Symbol* symbol, QWidget* parent);
	static void hideTip();
	static SymbolToolTip* getTip();
	static Symbol* getCurrentTipSymbol();
	
protected:
	virtual void enterEvent(QEvent* event);
	virtual void paintEvent(QPaintEvent* event);
	
private:
	void setPosition();
	
	bool help_shown;
	QLabel* help_label;
	Symbol* symbol;
	QRect icon_rect;
	
	static SymbolToolTip* tooltip;
	static QTimer* tooltip_timer;
};

#endif
