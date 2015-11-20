/*
 *    Copyright 2012, 2013, 2014 Kai Pastor, Thomas Schöps
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

#include "about_dialog.h"

#include <QtWidgets>

#include <mapper_config.h>


/**
 * @brief An URL identifying the main "about" page.
 * 
 * The main page's text will be set directly, thus not having a valid URL.
 * But an empty URL will be ignored by QTextBrowser's history, leading to
 * unexpected behaviour of backward navigation.
 */
const QUrl about_page_url = QUrl("#ABOUT");

/**
 * Puts the items of a QStringList into an HTML block or a sequence of blocks.
 */
static QString formatBlock(QStringList items)
{
#if defined(Q_OS_ANDROID) // or any other small-screen device
	QString block = "<p>";
	block.append(items.join(", "));
	block.append("</p>");
	return block;
#else
	const int columns = 3;
	const int rows = (int)ceil((double)items.size() / columns);
	QString table("<table><tr><td>");
	for(int i = 0, row = 1; i < items.size(); ++i)
	{
		table.append(items[i]);
		if (rows != row)
		{
			table.append("<br/>");
			++row;
		}
		else if (i < items.size())
		{
			table.append("</td><td>&nbsp;&nbsp;&nbsp;</td><td>");
			row = 1;
		}
	}
	table.append("</tr></table>");
	return table;
#endif
}


AboutDialog::AboutDialog(QWidget* parent)
 : QDialog(parent)
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
	
	text_browser = new QTextBrowser();
	text_browser->setOpenExternalLinks(true);
	text_browser->setSearchPaths(QStringList() << ":/" << ":/docs/licensing/html/");
	layout->addWidget(text_browser);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->setContentsMargins(left, top, right, bottom);
	
	QPushButton* back_button  = new QPushButton(QIcon(":/images/arrow-left.png"), QApplication::translate("QFileDialog", "Back"));
	buttons_layout->addWidget(back_button);
	
	buttons_layout->addStretch(1);
	
	QPushButton* close_button  = new QPushButton(QApplication::translate("QDialogButtonBox", "&Close"));
	close_button->setDefault(true);
	buttons_layout->addWidget(close_button);
	
	layout->addLayout(buttons_layout);
	
	connect(text_browser, SIGNAL(sourceChanged(QUrl)), this, SLOT(sourceChanged(QUrl)));
	connect(text_browser, SIGNAL(textChanged()), this, SLOT(updateWindowTitle()));
	connect(text_browser, SIGNAL(backwardAvailable(bool)), back_button, SLOT(setEnabled(bool)));
// Android: Crash in Qt 5.3.0 beta,
// cf. https://bugreports.qt-project.org/browse/QTBUG-38434
// and highlighting won't work anyway due to missing (stylus) hover events,
// cf. https://bugreports.qt-project.org/browse/QTBUG-36105
#ifndef Q_OS_ANDROID
	connect(text_browser, SIGNAL(highlighted(QString)), this, SLOT(highlighted(QString)));
#endif
	connect(back_button,  SIGNAL(clicked()), text_browser, SLOT(backward()));
	connect(close_button, SIGNAL(clicked()), this, SLOT(accept()));
	
	text_browser->setSource(about_page_url);
	text_browser->document()->adjustSize();  // needed for sizeHint()
	
#if defined(Q_OS_ANDROID)
	QScroller::grabGesture(text_browser, QScroller::LeftMouseButtonGesture);
	// Disable selection, so that it doesn't interfere with scrolling
	text_browser->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextBrowserInteraction) & ~Qt::TextSelectableByMouse);
	// Note: Only the above combination of QScroller::LeftMouseButtonGesture
	// and ~Qt::TextSelectableByMouse seems to achieve the desired behaviour
	// (touch-scrolling without selecting text.)
	
	setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
#endif
}

QSize AboutDialog::sizeHint() const
{
	QSize size = text_browser->document()->size().toSize();
	if (text_browser->verticalScrollBar())
		size.rwidth() += text_browser->verticalScrollBar()->width();
	return size;
}

void AboutDialog::sourceChanged(QUrl url)
{
	if (url == about_page_url)
		text_browser->setHtml(about());
}

void AboutDialog::updateWindowTitle()
{
	QString title = text_browser->documentTitle();
	if (title.isEmpty())
		title = tr("About %1").arg(APP_NAME);
	setWindowTitle(title);
}

void AboutDialog::highlighted(QString link)
{
	if (link.isEmpty())
	{
		QToolTip::hideText();
		return;
	}
	
	QString tooltip_text;
	if (link.contains("://"))
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

QString AboutDialog::about()
{
	static QStringList developers_list( QStringList()
	  << "Peter Curtis"
	  << "Kai Pastor"
	  << "Thomas Schöps %1"
	);
	
	static QStringList contributors_list( QStringList()
	  << "Javier Arufe"
	  << "Jon Cundill"
	  << "Sławomir Cygler"
	  << "Jan Dalheimer"
	  << "Eugeniy Fedirets"
	  << "Pavel Fric"
	  << "Anders Gressli"
	  << "Peter Hoban"
	  << "Henrik Johansson"
	  << "Panu Karhu"
	  << "Oskar Karlin"
	  << "Tojo Masaya"
	  << "Vincent Poinsignon"
	  << "Russell Porter"
	  << "Christopher Schive"
	  << "Aivars Zogla"
	);
	
	QString mapper_about(
	  QString("<html><head>"
	          "<title>%0</title>"
	          "</head><body>"
	          "<table><tr>"
	          "<td><img src=\":/images/mapper-icon/Mapper-128.png\"/></td>"
	          "<td><img src=\":/images/open-orienteering.png\"/></td>"
	          "</tr></table>"
	          "<h1>%1 %2</h1>"
	          "<p>"
	          "<em>%3</em><br/>"
	          "<a href=\"%4\">%4</a></p>"
	          "<p>Copyright (C) 2014 The OpenOrienteering developers</p>"
	          "<p>%5</p>"
	          "<p>%6</p>"
	          "<p>%7</p>"
	          "<p>%8</p>%9"
	          "<p>&nbsp;<br/>%10</p>%11"
	          "</body></html>"
	         ).
	    // %0
	    arg(tr("About %1")).
	    // %1 %2
	    arg(APP_NAME).arg(APP_VERSION).
	    // %3
	    arg(tr("A free software for drawing orienteering maps")).
	    // %4
	    arg("http://openorienteering.org").
	    // %5
	    arg(tr("This program is free software: you can redistribute it "
	           "and/or modify it under the terms of the "
	           "<a %1>GNU General Public License (GPL), version&nbsp;3</a>, "
	           "as published by the Free Software Foundation.").
	           arg("href=\"gpl-3-0.html\"")).
	    // %6
	    arg(tr("This program is distributed in the hope that it will be useful, "
	           "but WITHOUT ANY WARRANTY; without even the implied warranty of "
	           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
	           "GNU General Public License (GPL), version&nbsp;3, for "
	           "<a %1>more details</a>.").
	           arg("href=\"gpl-3-0.html#15-disclaimer-of-warranty\"")).
	    // %7
	    arg(tr("<a %1>All about licenses, copyright notices, conditions and "
	           "disclaimers.</a>").
	           arg("href=\"licensing.html\"")).
	    // %8
	    arg(tr("The OpenOrienteering developers in alphabetical order:")).
	    // %9
	    arg(formatBlock(developers_list).arg(tr("(project initiator)"))).
	    // %10
	    arg(tr("For contributions, thanks to:")).
	    // %11
	    arg(formatBlock(contributors_list)) );
	
	return mapper_about;
}
