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


#include "symbol_tooltip.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QHideEvent>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QShortcut>
#include <QShowEvent>
#include <QStyleOption>
#include <QVBoxLayout>

#include "../../symbol.h"


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
		Q_ASSERT(hasRoomToLeft || hasRoomToRight);
		x = hasRoomToRight ? icon_rect.right() + margin : icon_rect.left() - size.width() - margin;
		y = qMin(qMax(desktop.top() + margin, icon_rect.center().y() - size.height() / 2), desktop.bottom() - size.height() - margin);
	}
	
	move(QPoint(x, y));
}

void SymbolToolTip::scheduleShow(const Symbol* symbol, QRect icon_rect)
{
	this->icon_rect = icon_rect;
	this->symbol = symbol;
	
	name_label->setText(symbol->getNumberAsString() + QLatin1String(" <b>") + symbol->getName() + QLatin1String("</b>"));
	
	QString help_text(symbol->getDescription());
	if (help_text.isEmpty())
	{
		help_text = tr("No description!");
	}
	else
	{
		help_text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
		help_text.remove(QLatin1Char('\r'));
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
