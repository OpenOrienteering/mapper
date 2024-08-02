/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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

#include <Qt>
#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QScrollBar>
#include <QScroller>  // IWYU pragma: keep
#include <QString>
#include <QTextBrowser>
#include <QToolTip>
#include <QVBoxLayout>

#include "gui/widgets/text_browser.h"
#include "util/backports.h"  // IWYU pragma: keep


namespace OpenOrienteering {

TextBrowserDialog::TextBrowserDialog(QWidget* parent)
: QDialog(parent)
, text_browser(new TextBrowser())
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
	back_button->setEnabled(false);
	buttons_layout->addWidget(back_button);
	
	buttons_layout->addStretch(1);
	
	QPushButton* close_button  = new QPushButton(QApplication::translate("QPlatformTheme", "Close"));
	close_button->setDefault(true);
	buttons_layout->addWidget(close_button);
	
	layout->addLayout(buttons_layout);
	
	connect(text_browser, &QTextBrowser::sourceChanged, this, &TextBrowserDialog::sourceChanged);
	connect(text_browser, &QTextBrowser::textChanged, this, &TextBrowserDialog::updateWindowTitle);
	connect(text_browser, &QTextBrowser::backwardAvailable, back_button, &TextBrowserDialog::setEnabled);
	connect(text_browser, QOverload<const QString&>::of(&QTextBrowser::highlighted), this, &TextBrowserDialog::highlighted);
	connect(back_button,  &QPushButton::clicked, text_browser, &QTextBrowser::backward);
	connect(close_button, &QPushButton::clicked, this, &TextBrowserDialog::accept);
	
#if defined(Q_OS_ANDROID)
	QScroller::grabGesture(text_browser->viewport(), QScroller::TouchGesture);
	// Disable selection, so that it doesn't interfere with scrolling
	text_browser->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextBrowserInteraction) & ~Qt::TextSelectableByMouse);
	// Note: Only the above combination of QScroller::LeftMouseButtonGesture
	// and ~Qt::TextSelectableByMouse seems to achieve the desired behaviour
	// (touch-scrolling without selecting text.)
	
	setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
#endif
}


TextBrowserDialog::TextBrowserDialog(const QUrl& initial_url, QWidget* parent)
: TextBrowserDialog(parent)
{
	text_browser->setSource(initial_url);
	text_browser->document()->adjustSize();  // needed for sizeHint()
}


TextBrowserDialog::TextBrowserDialog(const QString& text, QWidget* parent)
 : TextBrowserDialog(parent)
{
	text_browser->setText(text);
	text_browser->document()->adjustSize();  // needed for sizeHint()
}


TextBrowserDialog::~TextBrowserDialog() = default;



QSize TextBrowserDialog::sizeHint() const
{
	QSize size = text_browser->document()->size().toSize();
	if (text_browser->verticalScrollBar())
		size.rwidth() += text_browser->verticalScrollBar()->width();
	return size;
}

void TextBrowserDialog::sourceChanged(const QUrl&)
{
	; // Nothing, to be overridden in subclasses
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
	}
	else
	{
		/// @todo: Position near mouse pointer
		auto tooltip_pos  = pos() + text_browser->pos();
		tooltip_pos.ry() += text_browser->height();
		QToolTip::showText(tooltip_pos, link, this, {});
	}
}


}  // namespace OpenOrienteering
