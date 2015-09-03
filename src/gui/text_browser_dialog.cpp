/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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

#include "text_browser_dialog.h"

#include <cmath>

#include <QApplication>
#include <QPushButton>
#include <QScrollBar>
#include <QScroller>
#include <QTextBrowser>
#include <QToolTip>
#include <QVBoxLayout>

#include <mapper_config.h>


TextBrowserDialog::TextBrowserDialog(const QUrl& initial_url, QWidget* parent)
 : QDialog(parent)
 , text_browser(new QTextBrowser())
{
	if (parent)
	{
		setWindowModality(Qt::WindowModal);
	}
	
	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);
	
	int left, top, right, bottom;
	layout->getContentsMargins(&left, &top, &right, &bottom);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	
	text_browser->setOpenExternalLinks(true);
	layout->addWidget(text_browser);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->setContentsMargins(left, top, right, bottom);
	
	QPushButton* back_button  = new QPushButton(QIcon(QStringLiteral(":/images/arrow-left.png")), QApplication::translate("QFileDialog", "Back"));
	buttons_layout->addWidget(back_button);
	
	buttons_layout->addStretch(1);
	
	QPushButton* close_button  = new QPushButton(QApplication::translate("QDialogButtonBox", "&Close"));
	close_button->setDefault(true);
	buttons_layout->addWidget(close_button);
	
	layout->addLayout(buttons_layout);
	
	connect(text_browser, &QTextBrowser::sourceChanged, this, &TextBrowserDialog::sourceChanged);
	connect(text_browser, &QTextBrowser::textChanged, this, &TextBrowserDialog::updateWindowTitle);
	connect(text_browser, &QTextBrowser::backwardAvailable, back_button, &TextBrowserDialog::setEnabled);
// Android: Crash in Qt 5.3.0 beta, Qt 5.4
// cf. https://bugreports.qt-project.org/browse/QTBUG-38434
// and highlighting won't work anyway due to missing (stylus) hover events,
// cf. https://bugreports.qt-project.org/browse/QTBUG-36105
#if !defined(Q_OS_ANDROID)
	connect(text_browser, (void (QTextBrowser::*)(const QString &))&QTextBrowser::highlighted, this, &TextBrowserDialog::highlighted);
#endif
	connect(back_button,  &QPushButton::clicked, text_browser, &QTextBrowser::backward);
	connect(close_button, &QPushButton::clicked, this, &TextBrowserDialog::accept);
	
	text_browser->setSource(initial_url);
	text_browser->document()->adjustSize();  // needed for sizeHint()
	
#if defined(Q_OS_ANDROID)
	QScroller::grabGesture(text_browser->viewport(), QScroller::LeftMouseButtonGesture);
	// Disable selection, so that it doesn't interfere with scrolling
	text_browser->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextBrowserInteraction) & ~Qt::TextSelectableByMouse);
	// Note: Only the above combination of QScroller::LeftMouseButtonGesture
	// and ~Qt::TextSelectableByMouse seems to achieve the desired behaviour
	// (touch-scrolling without selecting text.)
	
	setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
#endif
}

QSize TextBrowserDialog::sizeHint() const
{
	QSize size = text_browser->document()->size().toSize();
	if (text_browser->verticalScrollBar())
		size.rwidth() += text_browser->verticalScrollBar()->width();
	return size;
}

void TextBrowserDialog::sourceChanged(const QUrl&)
{
	; // Nothing, to be overriden in subclasses
}

void TextBrowserDialog::updateWindowTitle()
{
	setWindowTitle(text_browser->documentTitle());
}

void TextBrowserDialog::highlighted(const QString& link)
{
	if (link.isEmpty())
	{
		QToolTip::hideText();
		return;
	}
	
	QString tooltip_text;
	if (link.contains(QLatin1String("://")))
	{
		tooltip_text = tr("External link: %1").arg(link);
	}
	else
	{
		tooltip_text = tr("Click to view");
	}
	/// @todo: Position near mouse pointer
	QPoint tooltip_pos   = pos() + text_browser->pos();
	tooltip_pos.ry()    += text_browser->height();
	QToolTip::showText(tooltip_pos, tooltip_text, this, QRect());
}
