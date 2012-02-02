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
#include "symbol_setting_dialog.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "symbol_combined.h"

// ### SymbolRenderWidget ###

SymbolRenderWidget::SymbolRenderWidget(Map* map, QScrollBar* scroll_bar, SymbolWidget* parent) : QWidget(parent), scroll_bar(scroll_bar), symbol_widget(parent), map(map)
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
	setStatusTip(tr("For symbols with description, press F1 while the tooltip is visible to show it"));
	
	context_menu = new QMenu(this);
	
	QMenu* new_menu = new QMenu(tr("New symbol"), context_menu);
	/*QAction* new_point_action =*/ new_menu->addAction(tr("Point"), this, SLOT(newPointSymbol()));
	/*QAction* new_line_action =*/ new_menu->addAction(tr("Line"), this, SLOT(newLineSymbol()));
	/*QAction* new_area_action =*/ new_menu->addAction(tr("Area"), this, SLOT(newAreaSymbol()));
	/*QAction* new_text_action =*/ new_menu->addAction(tr("Text"), this, SLOT(newTextSymbol()));
	/*QAction* new_combined_action =*/ new_menu->addAction(tr("Combined"), this, SLOT(newCombinedSymbol()));
	context_menu->addMenu(new_menu);
	
	edit_action = context_menu->addAction(tr("Edit"), this, SLOT(editSymbol()));
	scale_action = context_menu->addAction(tr("Scale..."), this, SLOT(scaleSymbol()));
	context_menu->addSeparator();
	duplicate_action = context_menu->addAction(tr("Duplicate"), this, SLOT(duplicateSymbol()));
	delete_action = context_menu->addAction(tr("Delete"), this, SLOT(deleteSymbols()));
	context_menu->addSeparator();
	
	QMenu* select_menu = new QMenu(tr("Select"), context_menu);
	select_menu->addAction(tr("All"), this, SLOT(selectAll()));
	select_menu->addAction(tr("Invert"), this, SLOT(invertSelection()));
	context_menu->addMenu(select_menu);
	
	connect(map, SIGNAL(colorDeleted(int,MapColor*)), this, SLOT(update()));
}

bool SymbolRenderWidget::scrollBarNeeded(int width, int height)
{
	int icons_per_row, num_rows;
	getRowInfo(width, height, icons_per_row, num_rows);
	
	return (num_rows * Symbol::icon_size) > height;
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

Symbol* SymbolRenderWidget::getSingleSelectedSymbol()
{
	if (selected_symbols.size() != 1)
		return NULL;
	return map->getSymbol(*(selected_symbols.begin()));
}

void SymbolRenderWidget::setScroll(int new_scroll)
{
	update();
}

void SymbolRenderWidget::selectSingleSymbol(int i)
{
	updateSelectedIcons();
	selected_symbols.clear();
	if (i >= 0)
	{
		selected_symbols.insert(i);
		updateIcon(i);
	}
	symbol_widget->emitSelectedSymbolsChanged();
}
bool SymbolRenderWidget::isSymbolSelected(int i)
{
	return selected_symbols.find(i) != selected_symbols.end();
}
int SymbolRenderWidget::getNumSelectedSymbols()
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
			if (SymbolToolTip::getCurrentTipSymbol() != symbol)
				SymbolToolTip::showTip(QRect(mapToGlobal(getIconRect(hover_symbol_index).topLeft()), QSize(Symbol::icon_size, Symbol::icon_size)), symbol, this);
		}
		else
			SymbolToolTip::hideTip();
	}
}
int SymbolRenderWidget::getSymbolIndexAt(int x, int y)
{
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	
	int icon_in_row = floor(x / (float)Symbol::icon_size);
	if (icon_in_row >= icons_per_row)
		return -1;
	int i = icon_in_row + icons_per_row * floor((y + scroll) / (float)Symbol::icon_size);
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
	return QRect(x * Symbol::icon_size, y * Symbol::icon_size - scroll, Symbol::icon_size, Symbol::icon_size);
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
	icons_per_row = qMax(1, qRound(width / (float)Symbol::icon_size));
	num_rows = ceil(map->getNumSymbols() / (float)icons_per_row);
}
bool SymbolRenderWidget::getDropPosition(QPoint pos, int& row, int& pos_in_row)
{
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	int scroll = scroll_bar ? scroll_bar->value() : 0;
	
	row = floor((pos.y() + scroll) / (float)Symbol::icon_size);
	if (row >= num_rows)
	{
		row = -1;
		pos_in_row = -1;
		return false;
	}
	
	pos_in_row = floor((pos.x() + Symbol::icon_size / 2) / (float)Symbol::icon_size);
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
	
	return QRect(pos_in_row * Symbol::icon_size - rect_width / 2 - 1, row * Symbol::icon_size - scroll - 1, rect_width, Symbol::icon_size + 2);
}
void SymbolRenderWidget::updateScrollRange()
{
	if (!scroll_bar)
		return;
	int icons_per_row, num_rows;
	getRowInfo(width(), height(), icons_per_row, num_rows);
	
	int pixels_too_much = qMax(0, (num_rows * Symbol::icon_size) - height());
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
	int y = floor(scroll / (float)Symbol::icon_size);
	for (int i = icons_per_row * y; i < map->getNumSymbols(); ++i)
	{
		QImage* icon = map->getSymbol(i)->getIcon(map);
		
		QPoint corner(x * Symbol::icon_size, y * Symbol::icon_size - scroll);
		painter.drawImage(corner, *icon);
		
		if (i == hover_symbol_index || isSymbolSelected(i))
		{
			painter.setPen(Qt::white);
			painter.drawRect(corner.x() + 1, corner.y() + 1, Symbol::icon_size - 4, Symbol::icon_size - 4);
			
			QPen pen(isSymbolSelected(i) ? qRgb(12, 0, 255) : qRgb(255, 150, 0));
			painter.setPen(pen);
			painter.drawRect(corner.x() + 0, corner.y() + 0, Symbol::icon_size - 2, Symbol::icon_size - 2);
			
			if (i == current_symbol_index && isSymbolSelected(i))
			{
				QPen pen(Qt::white);
				pen.setStyle(Qt::DotLine);
				painter.setPen(pen);
				painter.drawRect(corner.x() + 0, corner.y() + 0, Symbol::icon_size - 2, Symbol::icon_size - 2);
			}
			
			painter.setPen(Qt::gray);
		}
		
		painter.drawLine(corner + QPoint(0, Symbol::icon_size - 1), corner + QPoint(Symbol::icon_size - 1, Symbol::icon_size - 1));
		painter.drawLine(corner + QPoint(Symbol::icon_size - 1, 0), corner + QPoint(Symbol::icon_size - 1, Symbol::icon_size - 2));
		
		++x;
		if (x >= icons_per_row)
		{
			x = 0;
			++y;
			if (y * Symbol::icon_size - scroll >= height())
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
    updateScrollRange();
}
void SymbolRenderWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton && current_symbol_index >= 0)
	{
		if ((event->pos() - last_click_pos).manhattanLength() < QApplication::startDragDistance())
			return;
		
		SymbolToolTip::hideTip();
		
		QDrag* drag = new QDrag(this);
		QMimeData* mime_data = new QMimeData();
		
		QByteArray data;
		data.append((const char*)&current_symbol_index, sizeof(int));
		mime_data->setData("openorienteering/symbol", data);
		drag->setMimeData(mime_data);
		
		drag->exec(Qt::MoveAction);
	}
	else if (event->button() == Qt::NoButton)
		mouseMove(event->x(), event->y());
	event->accept();
}
void SymbolRenderWidget::mousePressEvent(QMouseEvent* event)
{
	updateIcon(current_symbol_index);
	current_symbol_index = getSymbolIndexAt(event->x(), event->y());
	updateIcon(current_symbol_index);
	
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		if (event->modifiers() & Qt::ShiftModifier)
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
		else
		{
			if (!isSymbolSelected(current_symbol_index) && !(event->button() == Qt::RightButton && current_symbol_index < 0))
				selectSingleSymbol(current_symbol_index);
		}
	}
	
	if (event->button() == Qt::RightButton)
	{
		bool have_selection = getNumSelectedSymbols() > 0;
		bool single_selection = getNumSelectedSymbols() == 1 && current_symbol_index >= 0;
		
		edit_action->setEnabled(single_selection);
		scale_action->setEnabled(single_selection);
		duplicate_action->setEnabled(single_selection);
		delete_action->setEnabled(have_selection);
		
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
void SymbolRenderWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
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
	updateIcon(hover_symbol_index);
	hover_symbol_index = -1;
	
	SymbolToolTip::hideTip();
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

void SymbolRenderWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat("openorienteering/symbol"))
		event->acceptProposedAction();
}
void SymbolRenderWidget::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat("openorienteering/symbol"))
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
	
	Symbol* in_map_symbol = map->getSymbol(current_symbol_index);
	Symbol* edit_symbol = in_map_symbol->duplicate();

	SymbolSettingDialog dialog(edit_symbol, in_map_symbol, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Rejected)
	{
		delete edit_symbol;
		return;
	}
	
	edit_symbol->getIcon(map, true);
	map->setSymbol(edit_symbol, current_symbol_index);
}
void SymbolRenderWidget::scaleSymbol()
{
	assert(current_symbol_index >= 0);
	Symbol* symbol = map->getSymbol(current_symbol_index);
	
	bool ok;
	double percent = QInputDialog::getDouble(this, tr("Scale symbol %1").arg(symbol->getName()), tr("Scale to percentage:"), 100, 0, 999999, 6, &ok);
	if (!ok || percent == 100)
		return;
	
	symbol->scale(percent / 100.0);
	symbol->getIcon(map, true);
	updateIcon(current_symbol_index);
	map->changeSymbolForAllObjects(symbol, symbol);	// update the objects
	
	map->setSymbolsDirty();
}
void SymbolRenderWidget::deleteSymbols()
{
	for (std::set<int>::const_reverse_iterator it = selected_symbols.rbegin(); it != selected_symbols.rend(); ++it)
	{
		if (map->doObjectsExistWithSymbol(map->getSymbol(*it)))
		{
			if (QMessageBox::warning(this, tr("Confirmation"), tr("The map contains objects with the symbol \"%1\". Deleting it will delete those objects! Do you really want to do that?").arg(map->getSymbol(*it)->getName()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
				continue;
		}
		map->deleteSymbol(*it);
	}
	selected_symbols.clear();
	symbol_widget->emitSelectedSymbolsChanged();
	update();
	
	symbol_widget->adjustSize();
	map->setSymbolsDirty();
}
void SymbolRenderWidget::duplicateSymbol()
{
	assert(current_symbol_index >= 0);
	
	map->addSymbol(map->getSymbol(current_symbol_index)->duplicate(), current_symbol_index + 1);
	selectSingleSymbol(current_symbol_index + 1);
	
	symbol_widget->adjustSize();
	map->setSymbolsDirty();
}
void SymbolRenderWidget::selectAll()
{
	for (int i = 0; i < map->getNumSymbols(); ++i)
		selected_symbols.insert(i);
	symbol_widget->emitSelectedSymbolsChanged();
	update();
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

bool SymbolRenderWidget::newSymbol(Symbol* new_symbol)
{
	SymbolSettingDialog dialog(new_symbol, NULL, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Rejected)
	{
		delete new_symbol;
		return false;
	}
	
	int pos = currentSymbolIndex();
	map->addSymbol(new_symbol, (pos >= 0) ? pos : map->getNumSymbols());
	selectSingleSymbol(pos);
	
	symbol_widget->adjustSize();
	map->setSymbolsDirty();
	return true;
}

// ### SymbolWidget ###

SymbolWidget::SymbolWidget(Map* map, QWidget* parent): QWidget(parent), map(map)
{
	no_resize_handling = false;
	
	scroll_bar = new QScrollBar();
	scroll_bar->hide();
	scroll_bar->setOrientation(Qt::Vertical);
	scroll_bar->setSingleStep(Symbol::icon_size);
	scroll_bar->setPageStep(3 * Symbol::icon_size);
	render_widget = new SymbolRenderWidget(map, scroll_bar, this);
	
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

Symbol* SymbolWidget::getSingleSelectedSymbol()
{
	return render_widget->getSingleSelectedSymbol();
}
int SymbolWidget::getNumSelectedSymbols()
{
	return render_widget->getNumSelectedSymbols();
}

void SymbolWidget::adjustSize(int width, int height)
{
	if (width < 0) width = this->width();
	if (height < 0) height = this->height();
	
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
	
	// Determine optimal width
	int new_width;
	if (scroll_needed)
	{
		int icons_in_row = floor(width / (float)Symbol::icon_size);
		new_width = icons_in_row * Symbol::icon_size + scroll_bar->width();
	}
	else
		new_width = width;
	
	no_resize_handling = true;
	resize(new_width, height);
	no_resize_handling = false;
}

void SymbolWidget::resizeEvent(QResizeEvent* event)
{
	if (no_resize_handling)
		return;
	
	adjustSize(event->size().width(), event->size().height());
	
    event->accept();
}
void SymbolWidget::keyPressed(QKeyEvent* event)
{
	if (event->key() == Qt::Key_F1 && SymbolToolTip::getTip() != NULL)
		SymbolToolTip::getTip()->showDescription();
}
void SymbolWidget::symbolChanged(int pos, Symbol* symbol)
{
	render_widget->updateIcon(pos);
}

// ### SymbolToolTip ###

SymbolToolTip* SymbolToolTip::tooltip = NULL;

SymbolToolTip::SymbolToolTip(Symbol* symbol, QRect icon_rect, QWidget* parent) : QWidget(parent), symbol(symbol), icon_rect(icon_rect)
{
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_OpaquePaintEvent);
	
	QPalette text_palette;
	text_palette.setColor(QPalette::WindowText, qRgb(0, 0, 0));
	
	QLabel* upper_label = new QLabel(symbol->getNumberAsString() + " <b>" + symbol->getName() + "</b>");
	upper_label->setPalette(text_palette);
	help_shown = false;

	help_label = new QLabel("<i>" + (symbol->getDescription().isEmpty() ? tr("No description!") : symbol->getDescription()) + "</i>");
	help_label->setPalette(text_palette);
	//help_label->setMaximumWidth(500);
	help_label->setWordWrap(true);
	help_label->hide();

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(4, 4, 4, 4);	// NOTE: Tried to getContentsMargins() and set to half of that, but this returned zero
	layout->addWidget(upper_label);
	layout->addWidget(help_label);
	setLayout(layout);
}
void SymbolToolTip::showDescription()
{
	if (help_shown)
		return;
	
	help_label->show();
	adjustSize();
	setPosition();
	help_shown = true;
}

void SymbolToolTip::enterEvent(QEvent* event)
{
    hideTip();
}
void SymbolToolTip::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	
	painter.setPen(Qt::gray);
	painter.setBrush(Qt::white);
	
	QRect rect(0, 0, width() - 1, height() - 1);
	painter.drawRect(rect);
	
	painter.end();
}

void SymbolToolTip::setPosition()
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

void SymbolToolTip::showTip(QRect rect, Symbol* symbol, QWidget* parent)
{
	hideTip();
	
	tooltip = new SymbolToolTip(symbol, rect, parent);
	tooltip->adjustSize();
	tooltip->setPosition();
	tooltip->show();
}
void SymbolToolTip::hideTip()
{
	if (tooltip)
	{
		tooltip->hide();
		tooltip->deleteLater();
		tooltip = NULL;
	}
}
SymbolToolTip* SymbolToolTip::getTip()
{
	return tooltip;
}
Symbol* SymbolToolTip::getCurrentTipSymbol()
{
	if (!tooltip)
		return NULL;
	return tooltip->symbol;
}

#include "symbol_dock_widget.moc"
