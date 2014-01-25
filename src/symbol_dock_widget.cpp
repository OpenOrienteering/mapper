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


#include "symbol_dock_widget.h"

#include <cassert>

#include <QDebug>
#include <QHash>
#include <QtWidgets>

#include "core/map_color.h"
#include "map.h"
#include "map_editor.h"
#include "file_format.h"
#include "settings.h"
#include "symbol_area.h"
#include "symbol_combined.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_setting_dialog.h"
#include "symbol_text.h"
#include "util/overriding_shortcut.h"
#include "util.h"


// STL comparison function for sorting symbols by number
static bool Compare_symbolByNumber(Symbol* s1, Symbol* s2)
{
	int n1 = s1->number_components, n2 = s2->number_components;
	for (int i = 0; i < n1 && i < n2; i++)
	{
		if (s1->getNumberComponent(i) < s2->getNumberComponent(i)) return true;  // s1 < s2
		if (s1->getNumberComponent(i) > s2->getNumberComponent(i)) return false; // s1 > s2
		// if s1 == s2, loop to the next component
	}
	return false; // s1 == s2
}

struct Compare_symbolByColor
{
	bool operator() (Symbol* s1, Symbol* s2)
	{
		const MapColor* c1 = s1->getDominantColorGuess();
		const MapColor* c2 = s2->getDominantColorGuess();
		
		if (c1 && c2)
			return color_map.value(QRgb(*c1)) < color_map.value(QRgb(*c2));
		else if (c2)
			return true;
		else if (c1)
			return false;
		
		return false; // s1 == s2
	}
	
	// Maps color code to priority
	QHash<QRgb, int> color_map;
};

// STL comparison function for sorting symbols by color priority
static bool Compare_symbolByColorPriority(Symbol* s1, Symbol* s2)
{
	const MapColor* c1 = s1->getDominantColorGuess();
	const MapColor* c2 = s2->getDominantColorGuess();
	
	if (c1 && c2)
		return c1->comparePriority(*c2);
	else if (c2)
		return true;
	else if (c1)
		return false;
	
	return false; // s1 == s2
}


// ### SymbolRenderWidget ###

SymbolRenderWidget::SymbolRenderWidget(Map* map, bool mobile_mode, QScrollBar* scroll_bar, SymbolWidget* parent)
: QWidget(parent)
, scroll_bar(scroll_bar)
, symbol_widget(parent)
, mobile_mode(mobile_mode)
, map(map)
{
	current_symbol_index = -1;
	hover_symbol_index = -1;
	
	last_drop_row = -1;
	last_drop_pos = -1;
	
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setAcceptDrops(true);
	
	QShortcut* description_shortcut = new OverridingShortcut(
	  QKeySequence(tr("F1", "Shortcut for displaying the symbol's description")),
	  window()
	);
	tooltip = new SymbolToolTip(this, description_shortcut);
	// TODO: Use a placeholder in the literal and pass the actual shortcut's string representation.
	setStatusTip(tr("For symbols with description, press F1 while the tooltip is visible to show it"));
	
	context_menu = new QMenu(this);
	
	QMenu* new_menu = new QMenu(tr("New symbol"), context_menu);
	new_menu->setIcon(QIcon(":/images/plus.png"));
	/*QAction* new_point_action =*/ new_menu->addAction(tr("Point"), this, SLOT(newPointSymbol()));
	/*QAction* new_line_action =*/ new_menu->addAction(tr("Line"), this, SLOT(newLineSymbol()));
	/*QAction* new_area_action =*/ new_menu->addAction(tr("Area"), this, SLOT(newAreaSymbol()));
	/*QAction* new_text_action =*/ new_menu->addAction(tr("Text"), this, SLOT(newTextSymbol()));
	/*QAction* new_combined_action =*/ new_menu->addAction(tr("Combined"), this, SLOT(newCombinedSymbol()));
	context_menu->addMenu(new_menu);
	
	edit_action = context_menu->addAction(tr("Edit"), this, SLOT(editSymbol()));
	duplicate_action = context_menu->addAction(QIcon(":/images/tool-duplicate.png"), tr("Duplicate"), this, SLOT(duplicateSymbol()));
	delete_action = context_menu->addAction(QIcon(":/images/minus.png"), tr("Delete"), this, SLOT(deleteSymbols()));
	scale_action = context_menu->addAction(QIcon(":/images/tool-scale.png"), tr("Scale..."), this, SLOT(scaleSymbol()));
	context_menu->addSeparator();
	copy_action = context_menu->addAction(QIcon(":/images/copy.png"), tr("Copy"), this, SLOT(copySymbols()));
	paste_action = context_menu->addAction(QIcon(":/images/paste.png"), tr("Paste"), this, SLOT(pasteSymbols()));
	context_menu->addSeparator();
	switch_symbol_action = context_menu->addAction(QIcon(":/images/tool-switch-symbol.png"), tr("Switch symbol of selected object(s)"), parent, SLOT(emitSwitchSymbolClicked()));
	fill_border_action = context_menu->addAction(QIcon(":/images/tool-fill-border.png"), tr("Fill / Create border for selected object(s)"), parent, SLOT(emitFillBorderClicked()));
	// text will be filled in by updateContextMenuState()
	select_objects_action = context_menu->addAction(QIcon(":/images/tool-edit.png"), "", parent, SLOT(emitSelectObjectsClicked()));
	select_objects_additionally_action = context_menu->addAction(QIcon(":/images/tool-edit.png"), "", parent, SLOT(emitSelectObjectsAdditionallyClicked()));
	context_menu->addSeparator();
	hide_action = context_menu->addAction("", this, SLOT(setSelectedSymbolVisibility(bool)));
	hide_action->setCheckable(true);
	protect_action = context_menu->addAction("", this, SLOT(setSelectedSymbolProtection(bool)));
	protect_action->setCheckable(true);
	context_menu->addSeparator();
	
	QMenu* select_menu = new QMenu(tr("Select symbols"), context_menu);
	select_menu->addAction(tr("Select all"), this, SLOT(selectAll()));
	select_menu->addAction(tr("Select unused"), this, SLOT(selectUnused()));
	select_menu->addSeparator();
	select_menu->addAction(tr("Invert selection"), this, SLOT(invertSelection()));
	context_menu->addMenu(select_menu);
	
	QMenu* sort_menu = new QMenu(tr("Sort symbols"), context_menu);
	sort_menu->addAction(tr("Sort by number"), this, SLOT(sortByNumber()));
	sort_menu->addAction(tr("Sort by primary color"), this, SLOT(sortByColor()));
	sort_menu->addAction(tr("Sort by primary color priority"), this, SLOT(sortByColorPriority()));
	context_menu->addMenu(sort_menu);

	connect(map, SIGNAL(colorDeleted(int,const MapColor*)), this, SLOT(update()));
	connect(map, SIGNAL(symbolAdded(int,Symbol*)), symbol_widget, SLOT(adjustContents()));
}

SymbolToolTip* SymbolRenderWidget::getSymbolToolTip() const
{
	return tooltip;
}

bool SymbolRenderWidget::scrollBarNeeded(int width, int height)
{
	int icons_per_row, num_rows;
	getRowInfo(width, height, icons_per_row, num_rows);
	
	return (num_rows * Settings::getInstance().getSymbolWidgetIconSizePx()) > height;
}
void SymbolRenderWidget::setScrollBar(QScrollBar* new_scroll_bar)
{
	if (scroll_bar)
		disconnect(scroll_bar, SIGNAL(valueChanged(int)), this, SLOT(setScroll(int)));
	else
		new_scroll_bar->setValue(0);
	
	scroll_bar = new_scroll_bar;
	updateScrollRange();
	
	if (scroll_bar)
		connect(scroll_bar, SIGNAL(valueChanged(int)), this, SLOT(setScroll(int)));
}

Symbol* SymbolRenderWidget::getSingleSelectedSymbol() const
{
	if (selected_symbols.size() != 1)
		return NULL;
	return map->getSymbol(*(selected_symbols.begin()));
}
bool SymbolRenderWidget::isSymbolSelected(Symbol* symbol) const
{
	return isSymbolSelected(map->findSymbolIndex(symbol));
}

void SymbolRenderWidget::setScroll(int new_scroll)
{
	Q_UNUSED(new_scroll);
	update();
}

void SymbolRenderWidget::selectSingleSymbol(int i)
{
	if (selected_symbols.size() == 1 && *selected_symbols.begin() == i)
		return;	// Symbol already selected as only selection
	
	updateSelectedIcons();
	selected_symbols.clear();
	if (i >= 0)
	{
		selected_symbols.insert(i);
		updateIcon(i);
	}
	
	// Deactivate the change symbol on object selection setting temporarily while emitting the selected symbols changes signal.
	// If not doing this, a currently used tool could catch the signal, finish its editing as the selected symbol has changed,
	// and the selection of the newly inserted object triggers the selection of a different symbol, leading to bugs.
	bool change_symbol_setting = Settings::getInstance().getSettingCached(Settings::MapEditor_ChangeSymbolWhenSelecting).toBool();
	Settings::getInstance().setSettingInCache(Settings::MapEditor_ChangeSymbolWhenSelecting, false);
	symbol_widget->emitSelectedSymbolsChanged();
	Settings::getInstance().setSettingInCache(Settings::MapEditor_ChangeSymbolWhenSelecting, change_symbol_setting);
}
bool SymbolRenderWidget::isSymbolSelected(int i) const
{
	return selected_symbols.find(i) != selected_symbols.end();
}
void SymbolRenderWidget::getSelectionBitfield(std::vector< bool >& out) const
{
	out.assign(map->getNumSymbols(), false);
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it)
		out[*it] = true;
}
void SymbolRenderWidget::setSelectionBitfield(std::vector< bool >& in)
{
	updateSelectedIcons();
	selected_symbols.clear();
	for (size_t i = 0, end = in.size(); i < end; ++i)
	{
		if (in[i])
		{
			selected_symbols.insert(i);
			updateIcon(i);
		}
	}
	symbol_widget->emitSelectedSymbolsChanged();
}
int SymbolRenderWidget::getNumSelectedSymbols() const
{
	return (int)selected_symbols.size();
}

void SymbolRenderWidget::mouseMove(int x, int y)
{
	int i = getSymbolIndexAt(x, y);
	
	if (i != hover_symbol_index)
	{
		updateIcon(hover_symbol_index);
		hover_symbol_index = i;
		updateIcon(hover_symbol_index);
		
		if (hover_symbol_index >= 0)
		{
			Symbol* symbol = map->getSymbol(hover_symbol_index);
			if (tooltip->getSymbol() != symbol)
			{
				int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
				const QRect icon_rect(mapToGlobal(getIconRect(hover_symbol_index).topLeft()), QSize(icon_size, icon_size));
				tooltip->scheduleShow(symbol, icon_rect);
			}
		}
		else
		{
			tooltip->reset();
		}
	}
}
int SymbolRenderWidget::getSymbolIndexAt(int x, int y)
{
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	
	int icon_in_row = floor(x / (float)icon_size);
	if (icon_in_row >= icons_per_row)
		return -1;
	int i = icon_in_row + icons_per_row * floor((y + scroll) / (float)icon_size);
	if (i >= map->getNumSymbols())
		return -1;
	
	return i;
}
QRect SymbolRenderWidget::getIconRect(int i)
{
	if (i < 0)
		return QRect();
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	
	int x = i % icons_per_row;
	int y = i / icons_per_row;
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	return QRect(x * icon_size, y * icon_size - scroll, icon_size, icon_size);
}
void SymbolRenderWidget::updateIcon(int i)
{
	if (i < 0)
		return;
	update(getIconRect(i));
}
void SymbolRenderWidget::updateSelectedIcons()
{
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it)
		updateIcon(*it);
}
void SymbolRenderWidget::getRowInfo(int width, int height, int& icons_per_row, int& num_rows)
{
	Q_UNUSED(height);
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	icons_per_row = qMax(1, (int)floor(width / (float)icon_size));
	num_rows = (int)ceil(map->getNumSymbols() / (float)icons_per_row);
}
bool SymbolRenderWidget::getDropPosition(QPoint pos, int& row, int& pos_in_row)
{
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	
	row = floor((pos.y() + scroll) / (float)icon_size);
	if (row >= num_rows)
	{
		row = -1;
		pos_in_row = -1;
		return false;
	}
	
	pos_in_row = floor((pos.x() + icon_size / 2) / (float)icon_size);
	if (pos_in_row > icons_per_row)
		pos_in_row = icons_per_row;
	
	int global_pos = row * icons_per_row + pos_in_row;
	if (global_pos > map->getNumSymbols())
	{
		row = -1;
		pos_in_row = -1;
		return false;
	}
	
	return true;
}
QRect SymbolRenderWidget::getDragIndicatorRect(int row, int pos_in_row)
{
	const int rect_width = 3;
	
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	return QRect(pos_in_row * icon_size - rect_width / 2 - 1, row * icon_size - scroll - 1, rect_width, icon_size + 2);
}
void SymbolRenderWidget::updateScrollRange()
{
	if (!scroll_bar)
		return;
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	int pixels_too_much = qMax(0, (num_rows * icon_size) - height());
	if (pixels_too_much > 0)
	{
		scroll_bar->setEnabled(true);
		scroll_bar->setMinimum(0);
		scroll_bar->setMaximum(pixels_too_much);
	}
	else
		scroll_bar->setEnabled(false);
}

void SymbolRenderWidget::paintEvent(QPaintEvent* event)
{
	int icon_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	QPainter painter;
	painter.begin(this);
	
	QRect rect = event->rect().intersected(QRect(0, 0, width(), height()));
	painter.setClipRect(rect);
	
	// White background
	painter.fillRect(rect, Qt::white);
	
	// Cells
	painter.setPen(Qt::gray);
	
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	int x = 0;
	int y = floor(scroll / (float)icon_size);
	for (int i = icons_per_row * y; i < map->getNumSymbols(); ++i)
	{
		QImage* icon = map->getSymbol(i)->getIcon(map);
		
		QPoint corner(x * icon_size, y * icon_size - scroll);
		painter.drawImage(corner, *icon);
		
		if (i == hover_symbol_index || isSymbolSelected(i))
		{
			painter.setPen(Qt::white);
			painter.drawRect(corner.x() + 2, corner.y() + 2, icon_size - 6, icon_size - 6);
			
			QPen pen(isSymbolSelected(i) ? qRgb(12, 0, 255) : qRgb(255, 150, 0));
			pen.setWidth(2);
			pen.setJoinStyle(Qt::MiterJoin);
			painter.setPen(pen);
			painter.drawRect(QRectF(corner.x() + 0.5f, corner.y() + 0.5f, icon_size - 3, icon_size - 3));
			
			if (i == current_symbol_index && isSymbolSelected(i))
			{
				QPen pen(Qt::white);
				pen.setDashPattern(QVector<qreal>() << 2 << 2);
				painter.setPen(pen);
				painter.drawRect(corner.x() + 1, corner.y() + 1, icon_size - 4, icon_size - 4);
			}
			
			painter.setPen(Qt::gray);
		}
		
		if (map->getSymbol(i)->isHidden() || map->getSymbol(i)->isProtected())
		{
			QPen pen(Qt::white);
			pen.setWidth(3);
			painter.setPen(pen);
			painter.drawLine(corner + QPoint(1, 1), corner + QPoint(icon_size - 2, icon_size - 2));
			painter.drawLine(corner + QPoint(icon_size - 3, 1), corner + QPoint(1, icon_size - 3));
			
			painter.setPen(QPen(map->getSymbol(i)->isHidden() ? Qt::red : Qt::blue));
			painter.drawLine(corner + QPoint(0, 0), corner + QPoint(icon_size - 2, icon_size - 2));
			painter.drawLine(corner + QPoint(icon_size - 2, 0), corner + QPoint(0, icon_size - 2));
			
			painter.setPen(Qt::gray);
		}
		
		painter.drawLine(corner + QPoint(0, icon_size - 1), corner + QPoint(icon_size - 1, icon_size - 1));
		painter.drawLine(corner + QPoint(icon_size - 1, 0), corner + QPoint(icon_size - 1, icon_size - 2));
		
		++x;
		if (x >= icons_per_row)
		{
			x = 0;
			++y;
			if (y * icon_size - scroll >= height())
				break;
		}
	}
	
	// Drop indicator?
	if (last_drop_pos >= 0 && last_drop_row >= 0)
	{
		QRect drop_rect = getDragIndicatorRect(last_drop_row, last_drop_pos);
		painter.setPen(qRgb(255, 150, 0));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(drop_rect.left(), drop_rect.top(), drop_rect.width() - 1, drop_rect.height() - 1);
	}
	
	painter.end();
}
void SymbolRenderWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
    updateScrollRange();
}
void SymbolRenderWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (mobile_mode)
	{
		if (event->buttons() & Qt::LeftButton)
		{
			if ((event->pos() - last_click_pos).manhattanLength() < Settings::getInstance().getStartDragDistancePx())
				return;
			dragging = true;
			
			// set scroll
			if (scroll_bar && scroll_bar->isEnabled())
			{
				int new_scroll_value = last_click_scroll_value - (event->y() - last_click_pos.y());
				if (new_scroll_value != scroll_bar->value())
					scroll_bar->setValue(new_scroll_value);
			}
		}
	}
	else
	{
		if (event->buttons() & Qt::LeftButton && current_symbol_index >= 0)
		{
			if ((event->pos() - last_click_pos).manhattanLength() < Settings::getInstance().getStartDragDistancePx())
				return;
			
			tooltip->hide();
			
			QDrag* drag = new QDrag(this);
			QMimeData* mime_data = new QMimeData();
			
			QByteArray data;
			data.append((const char*)&current_symbol_index, sizeof(int));
			mime_data->setData("openorienteering/symbol_index", data);
			drag->setMimeData(mime_data);
			
			drag->exec(Qt::MoveAction);
		}
		else if (event->button() == Qt::NoButton)
			mouseMove(event->x(), event->y());
	}
	event->accept();
}
void SymbolRenderWidget::mousePressEvent(QMouseEvent* event)
{
	dragging = false;
	
	if (mobile_mode)
	{
		last_click_pos = event->pos();
		last_click_scroll_value = scroll_bar->value();
		return;
	}
	
	updateIcon(current_symbol_index);
	int old_symbol_index = current_symbol_index;
	current_symbol_index = getSymbolIndexAt(event->x(), event->y());
	updateIcon(current_symbol_index);
	
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		if (event->modifiers() & Qt::ControlModifier)
		{
			if (current_symbol_index >= 0)
			{
				if (!isSymbolSelected(current_symbol_index))
					selected_symbols.insert(current_symbol_index);
				else
					selected_symbols.erase(current_symbol_index);
				symbol_widget->emitSelectedSymbolsChanged();
			}
		}
		else if (event->modifiers() & Qt::ShiftModifier)
		{
			if (current_symbol_index >= 0)
			{
				bool insert = !isSymbolSelected(current_symbol_index);
				int i = (old_symbol_index >= 0) ? old_symbol_index : current_symbol_index;
				while (true)
				{
					if (insert)
						selected_symbols.insert(i);
					else
						selected_symbols.erase(i);
					updateIcon(i);
					
					if (current_symbol_index > i)
						++i;
					else if (current_symbol_index < i)
						--i;
					else
						break;
				}
				symbol_widget->emitSelectedSymbolsChanged();
			}
		}
		else
		{
			if (event->button() == Qt::LeftButton ||
				(event->button() == Qt::RightButton &&
				current_symbol_index >= 0 && !isSymbolSelected(current_symbol_index)))
				selectSingleSymbol(current_symbol_index);
		}
	}
	
	if (event->button() == Qt::RightButton)
	{
		updateContextMenuState();
		context_menu->popup(event->globalPos());
		event->accept();
	}
	else if (event->button() == Qt::LeftButton && current_symbol_index >= 0 && !(event->modifiers() & Qt::ShiftModifier))
	{
		last_click_pos = event->pos();
		last_drop_pos = -1;
		last_drop_row = -1;
	}
}
void SymbolRenderWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (mobile_mode)
	{
		if (dragging)
		{
			dragging = false;
			return;
		}
		
		updateIcon(current_symbol_index);
		current_symbol_index = getSymbolIndexAt(event->x(), event->y());
		updateIcon(current_symbol_index);
		
		if (current_symbol_index >= 0 && !isSymbolSelected(current_symbol_index))
			selectSingleSymbol(current_symbol_index);
		else
			symbol_widget->emitSelectedSymbolsChanged(); // HACK, will close the symbol selection screen
	}
}
void SymbolRenderWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (mobile_mode)
		return;
	
    int i = getSymbolIndexAt(event->x(), event->y());
	if (i < 0)
		return;
	
	updateIcon(current_symbol_index);
	current_symbol_index = i;
	updateIcon(current_symbol_index);
	editSymbol();
}
void SymbolRenderWidget::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	updateIcon(hover_symbol_index);
	hover_symbol_index = -1;
	
	tooltip->reset();
}
void SymbolRenderWidget::wheelEvent(QWheelEvent* event)
{
	if (scroll_bar && scroll_bar->isEnabled())
	{
		scroll_bar->event(event);
		mouseMove(event->x(), event->y());
		if (event->isAccepted())
			return;
	}
	QWidget::wheelEvent(event);
}

void SymbolRenderWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat("openorienteering/symbol_index"))
		event->acceptProposedAction();
}
void SymbolRenderWidget::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat("openorienteering/symbol_index"))
	{
		int row, pos_in_row;
		if (!getDropPosition(event->pos(), row, pos_in_row))
		{
			if (last_drop_pos >= 0 && last_drop_row >= 0)
			{
				update(getDragIndicatorRect(last_drop_row, last_drop_pos));
				last_drop_pos = -1;
				last_drop_row = -1;
			}
			return;
		}
		
		if (row != last_drop_row || pos_in_row != last_drop_pos)
		{
			if (last_drop_row >= 0 && last_drop_pos >= 0)
				update(getDragIndicatorRect(last_drop_row, last_drop_pos));
			
			last_drop_row = row;
			last_drop_pos = pos_in_row;
			
			if (last_drop_row >= 0 && last_drop_pos >= 0)
				update(getDragIndicatorRect(last_drop_row, last_drop_pos));
		}
		
		event->acceptProposedAction();
	}
}
void SymbolRenderWidget::dropEvent(QDropEvent* event)
{
	last_drop_row = -1;
	last_drop_pos = -1;
	
	if (event->source() != this || current_symbol_index < 0)
		return;
	
	if (event->proposedAction() == Qt::MoveAction)
	{
		int row, pos_in_row;
		if (!getDropPosition(event->pos(), row, pos_in_row))
			return;
		
		int icons_per_row, num_rows;
		getRowInfo(width(), height(), icons_per_row, num_rows);
		
		int pos = row * icons_per_row + pos_in_row;
		if (pos == current_symbol_index)
			return;
		
		event->acceptProposedAction();
		
        // save selection
        std::set<Symbol *> sel;
        for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it) {
            sel.insert(map->getSymbol(*it));
        }

		map->moveSymbol(current_symbol_index, pos);


		if (pos > current_symbol_index)
			--pos;
		current_symbol_index = pos;
		selectSingleSymbol(pos);
		update();
	}
}

void SymbolRenderWidget::newPointSymbol()
{
	newSymbol(new PointSymbol());
}
void SymbolRenderWidget::newLineSymbol()
{
	newSymbol(new LineSymbol());
}
void SymbolRenderWidget::newAreaSymbol()
{
	newSymbol(new AreaSymbol());
}
void SymbolRenderWidget::newTextSymbol()
{
	newSymbol(new TextSymbol());
}
void SymbolRenderWidget::newCombinedSymbol()
{
	newSymbol(new CombinedSymbol());
}
void SymbolRenderWidget::editSymbol()
{
	assert(current_symbol_index >= 0);
	
	Symbol* symbol = map->getSymbol(current_symbol_index);
	SymbolSettingDialog dialog(symbol, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Accepted)
	{
		symbol = dialog.getNewSymbol();
		map->setSymbol(symbol, current_symbol_index);
	}
}
void SymbolRenderWidget::scaleSymbol()
{
	assert(!selected_symbols.empty());
	
	bool ok;
	double percent = QInputDialog::getDouble(this, tr("Scale symbol(s)"), tr("Scale to percentage:"), 100, 0, 999999, 6, &ok);
	if (!ok || percent == 100)
		return;
	
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it)
	{
		Symbol* symbol = map->getSymbol(*it);
		
		symbol->scale(percent / 100.0);
		updateIcon(current_symbol_index);
		map->changeSymbolForAllObjects(symbol, symbol);	// update the objects
	}
	
	map->setSymbolsDirty();
}
void SymbolRenderWidget::deleteSymbols()
{
	for (std::set<int>::const_reverse_iterator it = selected_symbols.rbegin(); it != selected_symbols.rend(); ++it)
	{
		if (map->doObjectsExistWithSymbol(map->getSymbol(*it)))
		{
			if (QMessageBox::warning(this, tr("Confirmation"), tr("The map contains objects with the symbol \"%1\". Deleting it will delete those objects and clear the undo history! Do you really want to do that?").arg(map->getSymbol(*it)->getName()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
				continue;
		}
		map->deleteSymbol(*it);
	}
	selected_symbols.clear();
	symbol_widget->emitSelectedSymbolsChanged();
	update();
	
	symbol_widget->adjustContents();
	map->setSymbolsDirty();
}
void SymbolRenderWidget::duplicateSymbol()
{
	assert(current_symbol_index >= 0);
	
	map->addSymbol(map->getSymbol(current_symbol_index)->duplicate(), current_symbol_index + 1);
	selectSingleSymbol(current_symbol_index + 1);
	
	symbol_widget->adjustContents();
	map->setSymbolsDirty();
}

void SymbolRenderWidget::copySymbols()
{
	// Create map containing all colors and the selected symbols.
	// Copying all colors improves preservation of relative order during paste.
	Map* copy_map = new Map();
	copy_map->setScaleDenominator(map->getScaleDenominator());

	copy_map->importMap(map, Map::ColorImport, this);
	
	std::vector<bool> selection;
	getSelectionBitfield(selection);
	copy_map->importMap(map, Map::MinimalSymbolImport, this, &selection);
	
	// Save map to memory
	QBuffer buffer;
	if (!copy_map->exportToIODevice(&buffer))
	{
		delete copy_map;
		QMessageBox::warning(NULL, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	delete copy_map;
	
	// Put buffer into clipboard
	QMimeData* mime_data = new QMimeData();
	mime_data->setData("openorienteering/symbols", buffer.data());
	QApplication::clipboard()->setMimeData(mime_data);
}
void SymbolRenderWidget::pasteSymbols()
{
	if (!QApplication::clipboard()->mimeData()->hasFormat("openorienteering/symbols"))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("There are no symbols in clipboard which could be pasted!"));
		return;
	}
	
	// Get buffer from clipboard
	QByteArray byte_array = QApplication::clipboard()->mimeData()->data("openorienteering/symbols");
	QBuffer buffer(&byte_array);
	buffer.open(QIODevice::ReadOnly);
	
	// Create map from buffer
	Map* paste_map = new Map();
	if (!paste_map->importFromIODevice(&buffer))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	
	// Import pasted map
	map->importMap(paste_map, Map::MinimalSymbolImport, this, NULL, currentSymbolIndex(), false);
	delete paste_map;
	
	symbol_widget->emitSelectedSymbolsChanged();
}


void SymbolRenderWidget::setSelectedSymbolVisibility(bool checked)
{
	bool selection_changed = false;
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it)
	{
		Symbol* symbol = map->getSymbol(*it);
		if (symbol->isHidden() != checked)
		{
			symbol->setHidden(checked);
			updateIcon(*it);
			if (checked)
				selection_changed |= map->removeSymbolFromSelection(symbol, false);
		}
	}
	if (selection_changed)
		map->emitSelectionChanged();
	map->updateAllMapWidgets();
	map->setSymbolsDirty();
	symbol_widget->emitSelectedSymbolsChanged();
}
void SymbolRenderWidget::setSelectedSymbolProtection(bool checked)
{
	bool selection_changed = false;
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it)
	{
		Symbol* symbol = map->getSymbol(*it);
		if (symbol->isProtected() != checked)
		{
			symbol->setProtected(checked);
			updateIcon(*it);
			if (checked)
				selection_changed |= map->removeSymbolFromSelection(symbol, false);
		}
	}
	if (selection_changed)
		map->emitSelectionChanged();
	map->setSymbolsDirty();
	symbol_widget->emitSelectedSymbolsChanged();
}
void SymbolRenderWidget::selectAll()
{
	for (int i = 0; i < map->getNumSymbols(); ++i)
		selected_symbols.insert(i);
	symbol_widget->emitSelectedSymbolsChanged();
	update();
}
void SymbolRenderWidget::selectUnused()
{
	std::vector<bool> bitfield;
	map->determineSymbolsInUse(bitfield);
	for (size_t i = 0, end = bitfield.size(); i < end; ++i)
		bitfield[i] = !bitfield[i];
	setSelectionBitfield(bitfield);
}
void SymbolRenderWidget::invertSelection()
{
	std::set<int> new_set;
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		if (!isSymbolSelected(i))
			new_set.insert(i);
	}
	selected_symbols = new_set;
	symbol_widget->emitSelectedSymbolsChanged();
	update();
}

void SymbolRenderWidget::sortByNumber()
{
    sort(Compare_symbolByNumber);
}
void SymbolRenderWidget::sortByColor()
{
	Compare_symbolByColor compare;
	int next_priority = map->getNumColors() - 1;
	// Iterating in reverse order so identical colors are at the position where they appear with lowest priority.
	for (int i = map->getNumColors() - 1; i >= 0; --i)
	{
		QRgb color_code = QRgb(*map->getColor(i));
		if (!compare.color_map.contains(color_code))
		{
			compare.color_map.insert(color_code, next_priority);
			--next_priority;
		}
	}
	
	sort(compare);
}
void SymbolRenderWidget::sortByColorPriority()
{
	sort(Compare_symbolByColorPriority);
}

void SymbolRenderWidget::updateContextMenuState()
{
	bool have_selection = getNumSelectedSymbols() > 0;
	bool single_selection = getNumSelectedSymbols() == 1 && current_symbol_index >= 0;
	Symbol* single_symbol = getSingleSelectedSymbol();
	bool all_symbols_hidden = have_selection;
	bool all_symbols_protected = have_selection;
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it)
	{
		if (!map->getSymbol(*it)->isHidden())
			all_symbols_hidden = false;
		if (!map->getSymbol(*it)->isProtected())
			all_symbols_protected = false;
		if (!all_symbols_hidden && !all_symbols_protected)
			break;
	}
	
	bool single_symbol_compatible;
	bool single_symbol_different;
	map->getSelectionToSymbolCompatibility(single_symbol, single_symbol_compatible, single_symbol_different);
	
	edit_action->setEnabled(single_selection);
	scale_action->setEnabled(have_selection);
	copy_action->setEnabled(have_selection);
	paste_action->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("openorienteering/symbols"));
	switch_symbol_action->setEnabled(single_symbol_compatible && single_symbol_different);
	fill_border_action->setEnabled(single_symbol_compatible && single_symbol_different);
	hide_action->setEnabled(have_selection);
	hide_action->setChecked(all_symbols_hidden);
	protect_action->setEnabled(have_selection);
	protect_action->setChecked(all_symbols_protected);
	duplicate_action->setEnabled(single_selection);
	delete_action->setEnabled(have_selection);

	if (single_selection)
	{
		select_objects_action->setText(tr("Select all objects with this symbol"));
		select_objects_additionally_action->setText(tr("Add all objects with this symbol to selection"));
		hide_action->setText(tr("Hide objects with this symbol"));
		protect_action->setText(tr("Protect objects with this symbol"));
	}
	else
	{
		select_objects_action->setText(tr("Select all objects with selected symbols"));
		select_objects_additionally_action->setText(tr("Add all objects with selected symbols to selection"));
		hide_action->setText(tr("Hide objects with selected symbols"));
		protect_action->setText(tr("Protect objects with selected symbols"));
	}
    select_objects_action->setEnabled(have_selection);
	select_objects_additionally_action->setEnabled(have_selection);
}

bool SymbolRenderWidget::newSymbol(Symbol* prototype)
{
	SymbolSettingDialog dialog(prototype, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	delete prototype;
	if (dialog.exec() == QDialog::Rejected)
		return false;
	
	Symbol* new_symbol = dialog.getNewSymbol();
	int pos = (currentSymbolIndex() >= 0) ? currentSymbolIndex() : map->getNumSymbols();
	map->addSymbol(new_symbol, pos);
	selectSingleSymbol(pos);
	
	// Normally selectSingleSymbol() calls this already but here we must do it manually,
	// as the changing symbol indices can make selectSingleSymbol() think that the selection did not change.
	symbol_widget->emitSelectedSymbolsChanged();
	
	symbol_widget->adjustContents();
	map->setSymbolsDirty();
	return true;
}

// ### SymbolWidget ###

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



// ### SymbolToolTip ###

SymbolToolTip::SymbolToolTip(QWidget* parent, QShortcut* shortcut)
 : QWidget(parent),
   shortcut(shortcut),
   symbol(NULL),
   description_shown(false)
{
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_OpaquePaintEvent);
	
	QPalette text_palette;
	text_palette.setColor(QPalette::Window, text_palette.color(QPalette::Base));
	text_palette.setColor(QPalette::WindowText, text_palette.color(QPalette::Text));
	setPalette(text_palette);
	
	name_label = new QLabel();
	description_label = new QLabel();
	description_label->setWordWrap(true);
	description_label->hide();
	
	QVBoxLayout* layout = new QVBoxLayout();
	QStyleOption style_option;
	layout->setContentsMargins(
	  style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
	  style()->pixelMetric(QStyle::PM_LayoutTopMargin, &style_option) / 2,
	  style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
	  style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	layout->addWidget(name_label);
	layout->addWidget(description_label);
	setLayout(layout);
	
	tooltip_timer.setSingleShot(true);
	connect(&tooltip_timer, SIGNAL(timeout()), this, SLOT(show()));
	
	if (shortcut)
	{
		shortcut->setEnabled(false);
		connect(shortcut, SIGNAL(activated()), this, SLOT(showDescription()));
	}
}

void SymbolToolTip::showDescription()
{
	if (symbol && !description_shown && isVisible())
	{
		description_label->show();
		adjustSize();
		adjustPosition();
		description_shown = true;
	}
	
	if (shortcut)
		shortcut->setEnabled(false);
}

void SymbolToolTip::reset()
{
	symbol = NULL;
	tooltip_timer.stop();
	hide();
	description_label->hide();
	description_shown = false;
	
	if (shortcut)
		shortcut->setEnabled(false);
}

void SymbolToolTip::enterEvent(QEvent* event)
{
	Q_UNUSED(event);
    hide();
}

void SymbolToolTip::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	
	QPainter painter(this);
	
	painter.setBrush(palette().color(QPalette::Window));
	painter.setPen(palette().color(QPalette::WindowText));
	
	QRect rect(0, 0, width() - 1, height() - 1);
	painter.drawRect(rect);
	
	painter.end();
}

void SymbolToolTip::adjustPosition()
{
	QSize size = this->size();
	QRect desktop = QApplication::desktop()->screenGeometry(QCursor::pos());
	
	const int margin = 3;
	const bool hasRoomToLeft  = (icon_rect.left()   - size.width()  - margin >= desktop.left());
	const bool hasRoomToRight = (icon_rect.right()  + size.width()  + margin <= desktop.right());
	const bool hasRoomAbove   = (icon_rect.top()    - size.height() - margin >= desktop.top());
	const bool hasRoomBelow   = (icon_rect.bottom() + size.height() + margin <= desktop.bottom());
	if (!hasRoomAbove && !hasRoomBelow && !hasRoomToLeft && !hasRoomToRight) {
		return;
	}
	
	int x = 0;
	int y = 0;
	
	if (hasRoomBelow || hasRoomAbove) {
		y = hasRoomBelow ? icon_rect.bottom() + margin : icon_rect.top() - size.height() - margin;
		x = qMin(qMax(desktop.left() + margin, icon_rect.center().x() - size.width() / 2), desktop.right() - size.width() - margin);
	} else {
		assert(hasRoomToLeft || hasRoomToRight);
		x = hasRoomToRight ? icon_rect.right() + margin : icon_rect.left() - size.width() - margin;
		y = qMin(qMax(desktop.top() + margin, icon_rect.center().y() - size.height() / 2), desktop.bottom() - size.height() - margin);
	}
	
	move(QPoint(x, y));
}

void SymbolToolTip::scheduleShow(const Symbol* symbol, QRect icon_rect)
{
	this->icon_rect = icon_rect;
	this->symbol = symbol;
	
	name_label->setText(symbol->getNumberAsString() + " <b>" + symbol->getName() + "</b>");
	
	QString help_text(symbol->getDescription());
	if (help_text.isEmpty())
	{
		help_text = tr("No description!");
	}
	else
	{
		help_text.replace("\n", "<br>");
		help_text.remove('\r');
	}
	description_label->setText(help_text);
	description_label->hide();
	description_shown = false;
	shortcut->setEnabled(isVisible());
	
	adjustSize();
	adjustPosition();
	
	static const int delay = 150;
	tooltip_timer.start(delay);
}

void SymbolToolTip::showEvent(QShowEvent* event)
{
	if (shortcut && !description_shown && !event->spontaneous())
		shortcut->setEnabled(true);
	
	QWidget::showEvent(event);
}

void SymbolToolTip::hideEvent(QHideEvent* event)
{
	if (!event->spontaneous())
		reset();
	
	QWidget::hideEvent(event);
}

const Symbol* SymbolToolTip::getSymbol() const
{
	return symbol;
}



template<typename T>
void SymbolRenderWidget::sort(T compare)
{
	// save selection
	std::set<Symbol *> sel;
	for (std::set<int>::const_iterator it = selected_symbols.begin(); it != selected_symbols.end(); ++it) {
		sel.insert(map->getSymbol(*it));
	}
	
	map->sortSymbols(compare);
	
	//restore selection
	selected_symbols.clear();
	for (int i = 0; i < map->getNumSymbols(); i++) {
		if (sel.find(map->getSymbol(i)) != sel.end()) selected_symbols.insert(i);
	}
	
	update();
}
