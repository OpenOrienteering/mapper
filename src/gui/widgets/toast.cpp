/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017, 2019 Kai Pastor
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


#include "toast.h"

#include <utility>

#include <Qt>
#include <QtGlobal>
#include <QColor>
#include <QHideEvent>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRgb>
#include <QString>
#include <QStyle>
#include <QStyleOption>
#include <QTimerEvent>
#include <QVBoxLayout>


namespace OpenOrienteering {

namespace {

constexpr auto min_timeout = 500;  // ms
constexpr auto max_timeout = 5000; // ms

}  // namespace


Toast::Toast(QWidget* parent)
: QWidget{ parent }
{
	setAttribute(Qt::WA_TranslucentBackground);
	if (!parent)
		setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
	
	QPalette text_palette;
	auto foreground = Qt::white;
	auto background = Qt::black;
	if (qGray(text_palette.color(QPalette::Window).rgb()) < 128)
	    std::swap(foreground, background);
	text_palette.setColor(QPalette::WindowText, foreground);
	text_palette.setColor(QPalette::Window, background);
	setPalette(text_palette);
	
	label = new QLabel();
	label->setAttribute(Qt::WA_TranslucentBackground);
	label->setWordWrap(true);
	connect(label, &QLabel::linkActivated, this, &Toast::linkActivated);
	
	auto* layout = new QVBoxLayout();
	layout->addWidget(label);
	setLayout(layout);
	
	setVisible(false);
}


void Toast::showText(const QString& text, int timeout)
{
	label->setText(text);
	adjustSize();
	if (!isWindow())
		adjustPosition(window()->frameGeometry());
	show();
	raise();
	startTimer(timeout);
}

QString Toast::text() const
{
	return label->text();
}


void Toast::adjustPosition(const QRect& region)
{
	QStyleOption style_option;
	auto const margin = style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option);
	auto const x = region.left() + (region.width() - width()) / 2;
	auto const y = region.bottom() - height() - margin;
	move(x, y);
}


void Toast::startTimer(int timeout)
{
	if (Q_UNLIKELY(timeout < min_timeout))
	{
		qWarning("Minimum toast timeout is %d, got: %d", min_timeout, timeout);
		timeout = min_timeout;
	}
	else if (Q_UNLIKELY(timeout > max_timeout))
	{
		qWarning("Maximum toast timeout is %d, got: %d", max_timeout, timeout);
		timeout = max_timeout;
	}
	
	timer_id = QObject::startTimer(timeout, Qt::PreciseTimer);
	
	if (Q_UNLIKELY(timer_id == 0))
	{
		// There is no way to hide the toast, except for doing it right now.
		qWarning("Toast suppressed: %s\n"
		         "Reason: Failed to start a timer for hiding the toast.",
		         qPrintable(text()));
		hide();
	}
}

void Toast::stopTimer()
{
	if (timer_id != 0)
	{
		killTimer(timer_id);
		timer_id = 0;
	}
}


void Toast::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == timer_id)
	{
		stopTimer();
		hide();
	}
}


void Toast::hideEvent(QHideEvent* event)
{
	if (!event->spontaneous())
	{
		stopTimer();
	}
	QWidget::hideEvent(event);
}


void Toast::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);
	auto color = QColor(palette().color(QPalette::Window));
	painter.setBrush(color);
	painter.setOpacity(0.7);
	QStyleOption style_option;
	auto const radius = style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option)
	                    + style()->pixelMetric(QStyle::PM_LayoutTopMargin, &style_option);
	painter.drawRoundedRect(0, 0, width(), height(), radius, radius, Qt::AbsoluteSize);
}


}  // namespace OpenOrienteering
