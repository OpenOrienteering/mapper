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


#include "symbol_render_widget.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QDrag>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScopedValueRollback>

#include "settings.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/symbol_icon_decorator.h"
#include "core/symbols/text_symbol.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/widgets/symbol_tooltip.h"
#include "util/backports.h"
#include "util/overriding_shortcut.h"


namespace OpenOrienteering {

namespace MimeType {

/// The index of a symbol during drag-and-drop
const QString OpenOrienteeringSymbolIndex()
{
	return QStringLiteral("openorienteering/symbol_index");
}

/// Symbol definitions
const QString OpenOrienteeringSymbols()
{
	return QStringLiteral("openorienteering/symbols");
}


}  // namespace MimeType



//### SymbolRenderWidget ###

SymbolRenderWidget::SymbolRenderWidget(Map* map, bool mobile_mode, QWidget* parent)
: QWidget(parent)
, map(map)
, mobile_mode(mobile_mode)
, selection_locked(false)
, dragging(false)
, current_symbol_index(-1)
, hover_symbol_index(-1)
, last_drop_pos(-1)
, last_drop_row(-1)
, icon_size(Settings::getInstance().getSymbolWidgetIconSizePx())
, icons_per_row(6)
, num_rows(5)
, preferred_size(icons_per_row * icon_size, num_rows * icon_size)
, hidden_symbol_decoration(new HiddenSymbolDecorator(icon_size))
, protected_symbol_decoration(new ProtectedSymbolDecorator(icon_size))
{	
	setBackgroundRole(QPalette::Base);
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
	new_menu->setIcon(QIcon(QStringLiteral(":/images/plus.png")));
	/*QAction* new_point_action =*/ new_menu->addAction(tr("Point"), this, SLOT(newPointSymbol()));
	/*QAction* new_line_action =*/ new_menu->addAction(tr("Line"), this, SLOT(newLineSymbol()));
	/*QAction* new_area_action =*/ new_menu->addAction(tr("Area"), this, SLOT(newAreaSymbol()));
	/*QAction* new_text_action =*/ new_menu->addAction(tr("Text"), this, SLOT(newTextSymbol()));
	/*QAction* new_combined_action =*/ new_menu->addAction(tr("Combined"), this, SLOT(newCombinedSymbol()));
	context_menu->addMenu(new_menu);
	
	edit_action = context_menu->addAction(tr("Edit"), this, SLOT(editSymbol()));
	duplicate_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-duplicate.png")), tr("Duplicate"), this, SLOT(duplicateSymbol()));
	delete_action = context_menu->addAction(QIcon(QStringLiteral(":/images/minus.png")), tr("Delete"), this, SLOT(deleteSymbols()));
	scale_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-scale.png")), tr("Scale..."), this, SLOT(scaleSymbol()));
	context_menu->addSeparator();
	copy_action = context_menu->addAction(QIcon(QStringLiteral(":/images/copy.png")), tr("Copy"), this, SLOT(copySymbols()));
	paste_action = context_menu->addAction(QIcon(QStringLiteral(":/images/paste.png")), tr("Paste"), this, SLOT(pasteSymbols()));
	context_menu->addSeparator();
	switch_symbol_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-switch-symbol.png")), tr("Switch symbol of selected objects"), this, SIGNAL(switchSymbolClicked()));
	fill_border_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-fill-border.png")), tr("Fill / Create border for selected objects"), this, SIGNAL(fillBorderClicked()));
	// text will be filled in by updateContextMenuState()
	select_objects_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-edit.png")), {}, this, SLOT(selectObjectsExclusively()));
	select_objects_additionally_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-edit.png")), {}, this, SLOT(selectObjectsAdditionally()));
	deselect_objects_action = context_menu->addAction(QIcon(QStringLiteral(":/images/tool-edit.png")), {}, this, SLOT(deselectObjects()));
	context_menu->addSeparator();
	hide_action = context_menu->addAction({}, this, SLOT(setSelectedSymbolVisibility(bool)));
	hide_action->setCheckable(true);
	protect_action = context_menu->addAction({}, this, SLOT(setSelectedSymbolProtection(bool)));
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
	sort_manual_action = sort_menu->addAction(tr("Enable drag and drop"));
	sort_manual_action->setCheckable(true);
	context_menu->addMenu(sort_menu);
	
	connect(map, &Map::colorDeleted, this, QOverload<>::of(&QWidget::update));
	connect(map, &Map::symbolAdded, this, &SymbolRenderWidget::updateAll);
	connect(map, &Map::symbolDeleted, this, &SymbolRenderWidget::symbolDeleted);
	connect(map, &Map::symbolChanged, this, &SymbolRenderWidget::symbolChanged);
	connect(map, &Map::symbolIconChanged, this, &SymbolRenderWidget::updateSingleIcon);
	connect(map, &Map::symbolIconZoomChanged, this, &SymbolRenderWidget::updateAll);
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &SymbolRenderWidget::settingsChanged);
}

SymbolRenderWidget::~SymbolRenderWidget()
{
	; // nothing
}

void SymbolRenderWidget::symbolDeleted(int pos, const Symbol *old_symbol)
{
	Q_UNUSED(old_symbol);
	
	std::set<int>::iterator it = selected_symbols.find(pos);
	if (it != selected_symbols.end())
	{
		// Remove pos
		std::set<int>::iterator new_it = it;
		++new_it;
		selected_symbols.erase(it);
		// Renumber successors
		for (it = new_it; it != selected_symbols.end(); it = ++new_it)
		{
				new_it = selected_symbols.insert(*it - 1).first;
				selected_symbols.erase(it);
		}
		emitGuarded_selectedSymbolsChanged();
	}
	
	adjustLayout();
	update();
}

void SymbolRenderWidget::symbolChanged(int pos, const Symbol* new_symbol, const Symbol* old_symbol)
{
	Q_UNUSED(new_symbol);
	Q_UNUSED(old_symbol);
	
	updateSingleIcon(pos);
	
	if (isSymbolSelected(pos))
		emitGuarded_selectedSymbolsChanged();
}

void SymbolRenderWidget::updateAll()
{
	adjustLayout();
	update();
}

void SymbolRenderWidget::updateSingleIcon(int i)
{
	if (i >= 0)
	{
		QPoint pos = iconPosition(i);
		update(pos.x(), pos.y(), icon_size, icon_size);
	}
}

void SymbolRenderWidget::settingsChanged()
{
	const auto new_size = Settings::getInstance().getSymbolWidgetIconSizePx();
	if (icon_size != new_size)
	{
		for (int i = 0; i < map->getNumSymbols(); ++i)
		{
			auto symbol = map->getSymbol(i);
			if (symbol->getIcon(map).width() != new_size)
				symbol->resetIcon();
		}
		updateAll();
	}
}



QSize SymbolRenderWidget::sizeHint() const
{
	return preferred_size;
}

void SymbolRenderWidget::adjustLayout()
{
	auto old_icon_size = icon_size;
	icon_size = Settings::getInstance().getSymbolWidgetIconSizePx() + 1;
	// Allow symbol widget to be that much wider than the viewport
	int overflow = icon_size / 3;
	icons_per_row = qMax(1, (width() + overflow) / icon_size);
	num_rows = qMax(1, (map->getNumSymbols() + icons_per_row -1) / icons_per_row);
	setFixedHeight(num_rows * icon_size);
	
	if (old_icon_size != icon_size)
	{
		hidden_symbol_decoration.reset(new HiddenSymbolDecorator(icon_size));
		protected_symbol_decoration.reset(new ProtectedSymbolDecorator(icon_size));
	}
}

void SymbolRenderWidget::emitGuarded_selectedSymbolsChanged()
{
	QScopedValueRollback<bool> save(selection_locked);
	selection_locked = true;
	emit selectedSymbolsChanged();
}

const Symbol* SymbolRenderWidget::singleSelectedSymbol() const
{
	if (selected_symbols.size() != 1)
		return nullptr;
	return static_cast<const Map*>(map)->getSymbol(*(selected_symbols.begin()));
}

Symbol* SymbolRenderWidget::singleSelectedSymbol()
{
	if (selected_symbols.size() != 1)
		return nullptr;
	return map->getSymbol(*(selected_symbols.begin()));
}

bool SymbolRenderWidget::isSymbolSelected(const Symbol* symbol) const
{
	return isSymbolSelected(map->findSymbolIndex(symbol));
}

bool SymbolRenderWidget::isSymbolSelected(int i) const
{
	return selected_symbols.find(i) != selected_symbols.end();
}

void SymbolRenderWidget::selectSingleSymbol(const Symbol *symbol)
{
	int index = map->findSymbolIndex(symbol);
    if (index >= 0)
		selectSingleSymbol(index);
}

void SymbolRenderWidget::selectSingleSymbol(int i)
{
	if (selection_locked)
		return;
	
	if (selected_symbols.size() == 1 && *selected_symbols.begin() == i)
		return;	// Symbol already selected as only selection
	
	updateSelectedIcons();
	selected_symbols.clear();
	if (i >= 0)
	{
		selected_symbols.insert(i);
		updateSingleIcon(i);
	}
	current_symbol_index = i;
	
	emitGuarded_selectedSymbolsChanged();
}

void SymbolRenderWidget::hover(QPoint pos)
{
	int i = symbolIndexAt(pos);
	
	if (i != hover_symbol_index)
	{
		updateSingleIcon(hover_symbol_index);
		hover_symbol_index = i;
		updateSingleIcon(hover_symbol_index);
		
		if (hover_symbol_index >= 0)
		{
			Symbol* symbol = map->getSymbol(hover_symbol_index);
			if (tooltip->getSymbol() != symbol)
			{
				const QRect icon_rect(mapToGlobal(iconPosition(hover_symbol_index)), QSize(icon_size, icon_size));
				tooltip->scheduleShow(symbol, map, icon_rect, mobile_mode);
			}
		}
		else
		{
			tooltip->reset();
		}
	}
}	

QPoint SymbolRenderWidget::iconPosition(int i) const
{
	return QPoint((i % icons_per_row) * icon_size, (i / icons_per_row) * icon_size);
}

int SymbolRenderWidget::symbolIndexAt(QPoint pos) const
{
	int i = -1;
	
	int icon_in_row = pos.x() / icon_size;
	if (icon_in_row < icons_per_row)
	{
		i = icon_in_row + icons_per_row * (pos.y() / icon_size);
		if (i >= map->getNumSymbols())
			i = -1;
	}
	
	return i;
}

void SymbolRenderWidget::updateSelectedIcons()
{
	for (auto symbol_index : selected_symbols)
		updateSingleIcon(symbol_index);
}

bool SymbolRenderWidget::dropPosition(QPoint pos, int& row, int& pos_in_row)
{
	row = pos.y() / icon_size;
	if (row >= num_rows)
	{
		row = -1;
		pos_in_row = -1;
		return false;
	}
	
	pos_in_row = (pos.x() + icon_size / 2) / icon_size;
	if (pos_in_row > icons_per_row)
	{
		pos_in_row = icons_per_row;
	}
	
	int global_pos = row * icons_per_row + pos_in_row;
	if (global_pos > map->getNumSymbols())
	{
		row = -1;
		pos_in_row = -1;
		return false;
	}
	
	return true;
}

QRect SymbolRenderWidget::dropIndicatorRect(int row, int pos_in_row)
{
	const int rect_width = 3;
	return QRect(pos_in_row * icon_size - rect_width / 2 - 1, row * icon_size - 1, rect_width, icon_size + 2);
}

void SymbolRenderWidget::drawIcon(QPainter &painter, int i) const
{
	painter.save();
	
	Symbol* symbol = map->getSymbol(i);
	painter.drawImage(0, 0, symbol->getIcon(map));
	
	if (isSymbolSelected(i) || i == current_symbol_index)
	{
		painter.setOpacity(0.5);
		QPen pen(Qt::white);
		pen.setWidth(2);
		pen.setJoinStyle(Qt::MiterJoin);
		painter.setPen(pen);
		painter.drawRect(QRectF(1.5f, 1.5f, icon_size - 5, icon_size - 5));
		
		painter.setOpacity(1.0);
	}
	
	if (symbol->isProtected())
		protected_symbol_decoration->draw(painter);
	
	if (symbol->isHidden())
		hidden_symbol_decoration->draw(painter);
	
	if (isSymbolSelected(i))
	{
		QPen pen(qRgb(12, 0, 255));
		pen.setWidth(2);
		pen.setJoinStyle(Qt::MiterJoin);
		painter.setPen(pen);
		painter.drawRect(QRectF(0.5f, 0.5f, icon_size - 3, icon_size - 3));
		
		if (i == hover_symbol_index)
		{
			QPen pen(qRgb(255, 150, 0));
			pen.setWidth(2);
			pen.setDashPattern(QVector<qreal>() << 1 << 2);
			pen.setJoinStyle(Qt::MiterJoin);
			painter.setPen(pen);
			painter.drawRect(QRectF(0.5f, 0.5f, icon_size - 3, icon_size - 3));
		}
		else if (i == current_symbol_index)
		{
			QPen pen(Qt::white);
			pen.setDashPattern(QVector<qreal>() << 2 << 2);
			painter.setPen(pen);
			painter.drawRect(1, 1, icon_size - 4, icon_size - 4);
		}
	}
	else if (i == hover_symbol_index)
	{
		QPen pen(qRgb(255, 150, 0));
		pen.setWidth(2);
		pen.setJoinStyle(Qt::MiterJoin);
		painter.setPen(pen);
		painter.drawRect(QRectF(0.5f, 0.5f, icon_size - 3, icon_size - 3));
	}
	
	painter.setPen(Qt::gray);
	painter.drawLine(QPoint(0, icon_size - 1), QPoint(icon_size - 1, icon_size - 1));
	painter.drawLine(QPoint(icon_size - 1, 0), QPoint(icon_size - 1, icon_size - 2));
	
	painter.restore();
}

void SymbolRenderWidget::paintEvent(QPaintEvent* event)
{
	QRect event_rect = event->rect().adjusted(-icon_size, -icon_size, 0, 0);
	
	QPainter painter(this);
	painter.setPen(Qt::gray);
	
	int x = 0;
	int y = 0;
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		if (event_rect.contains(x,y))
		{
			painter.save();
			painter.translate(x, y);
			drawIcon(painter, i);
			painter.restore();
		}
		
		x += icon_size;
		if (x >= icons_per_row * icon_size)
		{
			x = 0;
			y +=  icon_size;
		}
	}
	
	// Drop indicator?
	if (last_drop_pos >= 0 && last_drop_row >= 0)
	{
		QRect drop_rect = dropIndicatorRect(last_drop_row, last_drop_pos);
		painter.setPen(qRgb(255, 150, 0));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(drop_rect.left(), drop_rect.top(), drop_rect.width() - 1, drop_rect.height() - 1);
	}
	
	painter.end();
}

void SymbolRenderWidget::resizeEvent(QResizeEvent* event)
{
	if (width() != event->oldSize().width())
	{
		adjustLayout();
	}
}

void SymbolRenderWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (mobile_mode)
	{
		if (event->buttons() & Qt::LeftButton)
		{
			hover(event->pos());
			
			if ((event->pos() - last_click_pos).manhattanLength() < Settings::getInstance().getStartDragDistancePx())
				return;
			dragging = sort_manual_action->isChecked();
		}
	}
	else
	{
		if (event->buttons() & Qt::LeftButton
		    && current_symbol_index >= 0
		    && sort_manual_action->isChecked())
		{
			if ((event->pos() - last_click_pos).manhattanLength() < Settings::getInstance().getStartDragDistancePx())
				return;
			
			tooltip->hide();
			
			QDrag* drag = new QDrag(this);
			QMimeData* mime_data = new QMimeData();
			
			QByteArray data;
			data.append((const char*)&current_symbol_index, sizeof(int));
			mime_data->setData(MimeType::OpenOrienteeringSymbolIndex(), data);
			drag->setMimeData(mime_data);
			
			drag->exec(Qt::MoveAction);
		}
		else if (event->button() == Qt::NoButton)
		{
			hover(event->pos());
		}
	}
	event->accept();
}

void SymbolRenderWidget::mousePressEvent(QMouseEvent* event)
{
	dragging = false;
	
	if (mobile_mode)
	{
		tooltip->hide();
		
		last_click_pos = event->pos();
		hover(event->pos());
		return;
	}
	
	updateSingleIcon(current_symbol_index);
	int old_symbol_index = current_symbol_index;
	current_symbol_index = symbolIndexAt(event->pos());
	updateSingleIcon(current_symbol_index);
	
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
				emitGuarded_selectedSymbolsChanged();
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
					updateSingleIcon(i);
					
					if (current_symbol_index > i)
						++i;
					else if (current_symbol_index < i)
						--i;
					else
						break;
				}
				emitGuarded_selectedSymbolsChanged();
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
		
		hover(event->pos());
		if (tooltip->isVisible())
			return;
		
		updateSingleIcon(current_symbol_index);
		current_symbol_index = symbolIndexAt(event->pos());
		updateSingleIcon(current_symbol_index);
		
		if (current_symbol_index >= 0 && !isSymbolSelected(current_symbol_index))
			selectSingleSymbol(current_symbol_index);
		else
			emitGuarded_selectedSymbolsChanged(); // HACK, will close the symbol selection screen
	}
}

void SymbolRenderWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (mobile_mode)
		return;
	
    int i = symbolIndexAt(event->pos());
	if (i < 0)
		return;
	
	updateSingleIcon(current_symbol_index);
	current_symbol_index = i;
	updateSingleIcon(current_symbol_index);
	editSymbol();
}

void SymbolRenderWidget::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	updateSingleIcon(hover_symbol_index);
	hover_symbol_index = -1;
	
	tooltip->reset();
}

void SymbolRenderWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat(MimeType::OpenOrienteeringSymbolIndex()))
		event->acceptProposedAction();
}

void SymbolRenderWidget::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat(MimeType::OpenOrienteeringSymbolIndex()))
	{
		int row, pos_in_row;
		if (!dropPosition(event->pos(), row, pos_in_row))
		{
			if (last_drop_pos >= 0 && last_drop_row >= 0)
			{
				update(dropIndicatorRect(last_drop_row, last_drop_pos));
				last_drop_pos = -1;
				last_drop_row = -1;
			}
			return;
		}
		
		if (row != last_drop_row || pos_in_row != last_drop_pos)
		{
			if (last_drop_row >= 0 && last_drop_pos >= 0)
				update(dropIndicatorRect(last_drop_row, last_drop_pos));
			
			last_drop_row = row;
			last_drop_pos = pos_in_row;
			
			if (last_drop_row >= 0 && last_drop_pos >= 0)
				update(dropIndicatorRect(last_drop_row, last_drop_pos));
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
		if (!dropPosition(event->pos(), row, pos_in_row))
			return;
		
		int pos = row * icons_per_row + pos_in_row;
		if (pos == current_symbol_index)
			return;
		
		event->acceptProposedAction();
		
        // save selection
        std::set<Symbol *> sel;
        for (auto symbol_index : selected_symbols) {
            sel.insert(map->getSymbol(symbol_index));
        }

		map->moveSymbol(current_symbol_index, pos);
		update();


		if (pos > current_symbol_index)
			--pos;
		current_symbol_index = pos;
		selectSingleSymbol(pos);
	}
}

void SymbolRenderWidget::selectObjectsExclusively()
{
	emit selectObjectsClicked(true);
}

void SymbolRenderWidget::selectObjectsAdditionally()
{
	emit selectObjectsClicked(false);
}

void SymbolRenderWidget::deselectObjects()
{
	emit deselectObjectsClicked();
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
	Q_ASSERT(current_symbol_index >= 0);
	
	Symbol* symbol = map->getSymbol(current_symbol_index);
	SymbolSettingDialog dialog(symbol, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Accepted)
	{
		map->setSymbol(dialog.getNewSymbol().release(), current_symbol_index);
	}
}

void SymbolRenderWidget::scaleSymbol()
{
	Q_ASSERT(!selected_symbols.empty());
	
	bool ok;
	double percent = QInputDialog::getDouble(this, tr("Scale symbols"), tr("Scale to percentage:"), 100, 0, 999999, 6, &ok);
	if (!ok || percent == 100)
		return;
	
	for (auto symbol_index : selected_symbols)
	{
		auto symbol = map->getSymbol(symbol_index);
		symbol->scale(percent / 100.0);
		updateSingleIcon(current_symbol_index);
		map->changeSymbolForAllObjects(symbol, symbol);	// update the objects
	}
	
	map->setSymbolsDirty();
}

void SymbolRenderWidget::deleteSymbols()
{
	// save selected symbols
	std::vector<const Symbol*> saved_selection;
	saved_selection.reserve(selected_symbols.size());
	for (auto symbol_index : selected_symbols)
	{
		saved_selection.push_back(map->getSymbol(symbol_index));
	}
	
	// delete symbols in order
	for (const auto symbol : saved_selection)
	{
		if (map->existsObjectWithSymbol(symbol))
		{
			if (QMessageBox::warning(this, tr("Confirmation"), tr("The map contains objects with the symbol \"%1\". Deleting it will delete those objects and clear the undo history! Do you really want to do that?").arg(symbol->getName()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
				continue;
		}
		map->deleteSymbol(map->findSymbolIndex(symbol));
	}
	
	if (selected_symbols.empty() && map->getFirstSelectedObject())
		selectSingleSymbol(map->getFirstSelectedObject()->getSymbol());
}

void SymbolRenderWidget::duplicateSymbol()
{
	Q_ASSERT(current_symbol_index >= 0);
	
	map->addSymbol(duplicate(*map->getSymbol(current_symbol_index)).release(), current_symbol_index + 1);
	selectSingleSymbol(current_symbol_index + 1);
}

void SymbolRenderWidget::copySymbols()
{
	// Create map containing all colors and the selected symbols.
	// Copying all colors improves preservation of relative order during paste.
	Map copy_map;
	copy_map.setScaleDenominator(map->getScaleDenominator());
	copy_map.importMap(*map, Map::ColorImport);
	
	std::vector<bool> selection(map->getNumSymbols(), false);
	for (auto symbol_index : selected_symbols)
		selection[symbol_index] = true;
	
	copy_map.importMap(*map, Map::MinimalSymbolImport, &selection);
	
	// Save map to memory
	QBuffer buffer;
	if (!copy_map.exportToIODevice(buffer))
	{
		QMessageBox::warning(nullptr, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	
	// Put buffer into clipboard
	QMimeData* mime_data = new QMimeData();
	mime_data->setData(MimeType::OpenOrienteeringSymbols(), buffer.data());
	QApplication::clipboard()->setMimeData(mime_data);
}

void SymbolRenderWidget::pasteSymbols()
{
	if (!QApplication::clipboard()->mimeData()->hasFormat(MimeType::OpenOrienteeringSymbols()))
	{
		QMessageBox::warning(nullptr, tr("Error"), tr("There are no symbols in clipboard which could be pasted!"));
		return;
	}
	
	// Get buffer from clipboard
	QByteArray byte_array = QApplication::clipboard()->mimeData()->data(MimeType::OpenOrienteeringSymbols());
	QBuffer buffer(&byte_array);
	
	// Create map from buffer
	Map paste_map;
	if (!paste_map.importFromIODevice(buffer))
	{
		QMessageBox::warning(nullptr, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	
	if (paste_map.getScaleDenominator() != map->getScaleDenominator())
	{
		/// \todo Adjust message to this particular action, and remove Map context.
		///       See also MapEditor::importMap.
		int answer = QMessageBox::question(
		                 window(),
		                 Map::tr("Question"),
		                 Map::tr("The scale of the imported data is 1:%1 "
		                         "which is different from this map's scale of 1:%2.\n\n"
		                         "Rescale the imported data?")
		                 .arg(QLocale().toString(paste_map.getScaleDenominator()),
		                      QLocale().toString(map->getScaleDenominator())),
		                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
		if (answer == QMessageBox::Cancel)
			return;
		
		paste_map.changeScale(map->getScaleDenominator(), {0, 0}, answer == QMessageBox::Yes, false, false, false);
	}
	
	// Ensure that a change in selection is detected
	selectSingleSymbol(-1);
		
	// Import pasted map
	map->importMap(paste_map, Map::MinimalSymbolImport, nullptr, current_symbol_index, false);
	
	selectSingleSymbol(current_symbol_index);
}

void SymbolRenderWidget::setSelectedSymbolVisibility(bool checked)
{
	bool selection_changed = false;
	for (auto symbol_index : selected_symbols)
	{
		auto symbol = map->getSymbol(symbol_index);
		if (symbol->isHidden() != checked)
		{
			symbol->setHidden(checked);
			updateSingleIcon(symbol_index);
			if (checked)
				selection_changed |= map->removeSymbolFromSelection(symbol, false);
		}
	}
	if (selection_changed)
		map->emitSelectionChanged();
	map->updateAllMapWidgets();
	map->setSymbolsDirty();
	emitGuarded_selectedSymbolsChanged();
}

void SymbolRenderWidget::setSelectedSymbolProtection(bool checked)
{
	bool selection_changed = false;
	for (auto symbol_index : selected_symbols)
	{
		auto symbol = map->getSymbol(symbol_index);
		if (symbol->isProtected() != checked)
		{
			symbol->setProtected(checked);
			updateSingleIcon(symbol_index);
			if (checked)
				selection_changed |= map->removeSymbolFromSelection(symbol, false);
		}
	}
	if (selection_changed)
		map->emitSelectionChanged();
	map->setSymbolsDirty();
	emitGuarded_selectedSymbolsChanged();
}

void SymbolRenderWidget::selectAll()
{
	for (int i = 0; i < map->getNumSymbols(); ++i)
		selected_symbols.insert(i);
	emitGuarded_selectedSymbolsChanged();
	update();
}

void SymbolRenderWidget::selectUnused()
{
	std::vector<bool> symbols_in_use;
	map->determineSymbolsInUse(symbols_in_use);
	
	updateSelectedIcons();
	selected_symbols.clear();
	for (size_t i = 0, end = symbols_in_use.size(); i < end; ++i)
	{
		if (!symbols_in_use[i])
		{
			selected_symbols.insert(i);
			updateSingleIcon(i);
		}
	}
	emitGuarded_selectedSymbolsChanged();
}

void SymbolRenderWidget::invertSelection()
{
	std::set<int> new_set;
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		if (!isSymbolSelected(i))
			new_set.insert(i);
	}
	swap(selected_symbols, new_set);
	emitGuarded_selectedSymbolsChanged();
	update();
}

void SymbolRenderWidget::sortByNumber()
{
    sort(Symbol::lessByNumber);
}

void SymbolRenderWidget::sortByColor()
{
	sort(Symbol::lessByColor(map));
}

void SymbolRenderWidget::sortByColorPriority()
{
	sort(Symbol::lessByColorPriority);
}

void SymbolRenderWidget::showContextMenu(QPoint global_pos)
{
	updateContextMenuState();
	context_menu->popup(global_pos);
}

void SymbolRenderWidget::contextMenuEvent(QContextMenuEvent* event)
{
	showContextMenu(event->globalPos());
	event->accept();
}

void SymbolRenderWidget::updateContextMenuState()
{
	bool have_selection = selectedSymbolsCount() > 0;
	bool single_selection = selectedSymbolsCount() == 1 && current_symbol_index >= 0;
	const Symbol* single_symbol = singleSelectedSymbol();
	bool all_symbols_hidden = have_selection;
	bool all_symbols_protected = have_selection;
	for (auto symbol_index : selected_symbols)
	{
		if (!map->getSymbol(symbol_index)->isHidden())
			all_symbols_hidden = false;
		if (!map->getSymbol(symbol_index)->isProtected())
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
	paste_action->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(MimeType::OpenOrienteeringSymbols()));
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
		deselect_objects_action->setText(tr("Remove all objects with this symbol from selection"));
		hide_action->setText(tr("Hide objects with this symbol"));
		protect_action->setText(tr("Protect objects with this symbol"));
	}
	else
	{
		select_objects_action->setText(tr("Select all objects with selected symbols"));
		select_objects_additionally_action->setText(tr("Add all objects with selected symbols to selection"));
		deselect_objects_action->setText(tr("Remove all objects with selected symbols from selection"));
		hide_action->setText(tr("Hide objects with selected symbols"));
		protect_action->setText(tr("Protect objects with selected symbols"));
	}
	select_objects_action->setEnabled(have_selection);
	select_objects_additionally_action->setEnabled(have_selection);
	deselect_objects_action->setEnabled(have_selection);
}

bool SymbolRenderWidget::newSymbol(Symbol* prototype)
{
	SymbolSettingDialog dialog(prototype, map, this);
	dialog.setWindowModality(Qt::WindowModal);
	delete prototype;
	if (dialog.exec() == QDialog::Rejected)
		return false;
	
	int pos = (current_symbol_index >= 0) ? current_symbol_index : map->getNumSymbols();
	map->addSymbol(dialog.getNewSymbol().release(), pos);
	// Ensure that a change in selection is detected
	selectSingleSymbol(-1);
	selectSingleSymbol(pos);
	return true;
}

template<typename T>
void SymbolRenderWidget::sort(T compare)
{
	// save selection
	std::set<const Symbol*> saved_selection;
	for (auto symbol_index : selected_symbols)
	{
		saved_selection.insert(map->getSymbol(symbol_index));
	}
	
	map->sortSymbols(compare);
	
	// restore selection
	selected_symbols.clear();
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		if (saved_selection.find(map->getSymbol(i)) != saved_selection.end())
			selected_symbols.insert(i);
	}
	
	update();
}


}  // namespace OpenOrienteering
