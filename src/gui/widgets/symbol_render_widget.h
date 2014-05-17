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


#ifndef _OPENORIENTEERING_SYMBOL_RENDER_WIDGET_H_
#define _OPENORIENTEERING_SYMBOL_RENDER_WIDGET_H_

#include <set>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

class Map;
class Symbol;
class SymbolToolTip;

/**
 * @brief Shows all symbols from a map in a size-constrained widget.
 * 
 * SymbolRenderWidget lets the user select symbols and perform actions on them.
 * It is a plain widget which does not implement scrolling but takes as much
 * height as neccessary for given width and number of symbols. Scrolling is
 * realized by SymbolWidget.
 */
class SymbolRenderWidget : public QWidget
{
Q_OBJECT
public:
	/**
	 * @brief Constructs a new SymbolRenderWidget.
	 * @param map         The map which provides the symbols. Must not be NULL.
	 * @param mobile_mode If true, enables a special mode for mobile devices.
	 * @param parent      The parent QWidget.
	 */
	SymbolRenderWidget(Map* map, bool mobile_mode, QWidget* parent = NULL);
	
	/**
	 * @brief Destroys the SymbolRenderWidget.
	 */
	virtual ~SymbolRenderWidget();

	/**
	 * @brief Returns the number of selected symbols.
	 */
	int selectedSymbolsCount() const;
	
	/**
	 * @brief Checks if the symbol is selected.
	 */
	bool isSymbolSelected(Symbol* symbol) const;
	
	/**
	 * @brief Checks if the symbol with given index is selected.
	 */
	bool isSymbolSelected(int i) const;
	
	/**
	 * @brief If exactly one symbol is selected, returns this symbol.
	 * 
	 * Otherwise returns NULL.
	 */
	Symbol* singleSelectedSymbol() const;
	
	/**
	 * @brief Selects the given symbol exclusively.
	 * 
	 * Deselects other symbols, if there was a different selection before.
	 */
	void selectSingleSymbol(Symbol *symbol);
	
	/**
	 * @brief Selects the given symbol exclusively.
	 * 
	 * Selects the symbol with the given number.
	 * Deselects other symbols, if there was a different selection before.
	 */
	void selectSingleSymbol(int i);
	
	/**
	 * @brief Returns the recommended size for the widget.
	 * 
	 * Reimplemented from QWidget::sizeHint().
	 */
	virtual QSize sizeHint() const;
	
public slots:
	/**
	 * @brief Updates the layout and marks all icons for repainting.
	 */
	void updateAll();
	
	/**
	 * @brief Marks the icon with the given index for repainting.
	 */
	void updateSingleIcon(int i);
	
signals:
	/**
	 * @brief The collection of selected symbols changed.
	 */
	void selectedSymbolsChanged();
	
	/**
	 * @brief The user triggered "Switch symbol".
	 * @todo  Merge with/Reuse corresponding action in MapEditorController.
	 */
	void switchSymbolClicked();
	
	/**
	 * @brief The user triggered "Fill/Create border".
	 * @todo  Merge with/Reuse corresponding action in MapEditorController.
	 */
	void fillBorderClicked();
	
	/**
	 * @brief The user triggered selecting objects with the active symbol.
	 * @param exclusively If true, an existing selection is replaced,
	 *                    otherwise it is extend.
	 */
	void selectObjectsClicked(bool exclusively);
	
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
	void selectObjectsExclusively();
	void selectObjectsAdditionally();
	void selectAll();
	void selectUnused();
	void invertSelection();
	void sortByNumber();
	void sortByColor();
	void sortByColorPriority();
	
	/**
	 * @brief Locks the current symbol selection against external changes.
	 * 
	 * A currently used tool could catch the selectedSymbolsChanged() and
	 * finish its editing for that reason. It would than insert a new object to
	 * the map and so trigger another change of selection. This not desired,
	 * and lockSelection(), being the first slot listening on
	 * selectedSymbolsChanged(), will block external changes to the selection.
	 * 
	 * It does take care of unlocking too, by scheduling a call to
	 * unlockSelection() for the next time control returns to the event loop.
	 */
	void lockSelection();
	
	/**
	 * @brief Unlocks the current symbol selection.
	 * 
	 * It should be rarely needed to call this actively because lockSelection
	 * already schedules a call to this function.
	 */
	void unlockSelection();
	
protected:
	virtual void resizeEvent(QResizeEvent* event);
	
	/**
	 * @brief Recalculates the layout and size.
	 * 
	 * This function reads the icon size from the settings and the widget's
	 * current width and adjusts the number of icons per row, the number of
	 * rows and the widget's height to fit the current number of symbols.
	 */
	void adjustLayout();
	
	/**
	 * @brief Returns the top-left coordinates of a symbol's icon.
	 * @param i The index of the symbol.
	 */
	QPoint iconPosition(int i) const;
	
	/**
	 * @brief Returns the index of the symbol represented at a particular location
	 * @param pos The location
	 */
	int symbolIndexAt(QPoint pos) const;
	
	
	virtual void paintEvent(QPaintEvent* event);
	
	/**
	 * @brief Draws the icon and its decoration (hidden, protected).
	 * @param painter The QPainter to be used (must be active).
	 * @param i       The index of the symbol.
	 * @param x       The x coordinate of the top left corner.
	 * @param y       The y coordinate of the top left corner.
	 */
	void drawIcon(QPainter& painter, int i, int x, int y);
	
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void leaveEvent(QEvent* event);
	
	/**
	 * @brief Handles hovering over the icons, i.e. controlling the tool tip.
	 * @param pos The current location of the pointing device.
	 */
	void hover(QPoint pos);
	
	
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dragMoveEvent(QDragMoveEvent* event);
	virtual void dropEvent(QDropEvent* event);
	
	/**
	 * @brief Determines the drop location for a given pointing device position.
	 * @param pos        The pointing device position.
	 * @param row        The icon row where to insert the dropped symbol.
	 * @param pos_in_row The position in the row where to insert the dropped symbol.
	 * @return If there is a valid drop position, returns true, otherwise false.
	 */
	bool dropPosition(QPoint pos, int& row, int& pos_in_row);
	
	/**
	 * @brief Determines a drop indicator rectangle for a given location in the icons.
	 * @param row        The icon row where to insert the dropped symbol.
	 * @param pos_in_row The position in the row where to insert the dropped symbol.
	 */
	QRect dropIndicatorRect(int row, int pos_in_row);
	
	
	/**
	 * @brief Takes care of editing and inserting a newly created symbol.
	 * @param prototype The symbol which will be edited and inserted in the map.
	 * @return If editing is cancelled, returns false. Otherwise returns true.
	 */
	bool newSymbol(Symbol* prototype);
	
	/**
	 * @brief Sorts the map's symbol set by arbitrary criteria.
	 */
	template<typename T> void sort(T compare);
	
	/**
	 * @brief Marks the icons of the selected symbols for repainting.
	 */
	void updateSelectedIcons();
	
	/** 
	 * @brief Updates the state of the actions in the context menu.
	 */
	void updateContextMenuState();
	
private:
	Map* map;
	bool mobile_mode;
	
	bool selection_locked;
	bool dragging;
	
	int current_symbol_index;
	int hover_symbol_index;
	std::set<int> selected_symbols;
	
	QPoint last_click_pos;
	int last_drop_pos;
	int last_drop_row;
	
	int icon_size;
	int icons_per_row;
	int num_rows;
	QSize preferred_size;
	
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
	
	SymbolToolTip* tooltip;
};

//### SymbolRenderWidget inline code ###

inline
int SymbolRenderWidget::selectedSymbolsCount() const
{
	return (int)selected_symbols.size();
}

#endif
