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

#include "about_dialog.h"

#include <cmath>

#include <QApplication>
#include <QTextBrowser>

#include <mapper_config.h>


/**
 * @brief An URL identifying the main "about" page.
 * 
 * The main page's text will be set directly, thus not having a valid URL.
 * But an empty URL will be ignored by QTextBrowser's history, leading to
 * unexpected behaviour of backward navigation.
 */
const QUrl about_page_url = QUrl(QStringLiteral("#ABOUT"));

/**
 * Puts the items of a QStringList into an HTML block or a sequence of blocks.
 */
static QString formatBlock(const QStringList& items)
{
#if defined(Q_OS_ANDROID) // or any other small-screen device
	QString block = QStringLiteral("<p>");
	block.append(items.join(QStringLiteral(", ")));
	block.append(QStringLiteral("</p>"));
	return block;
#else
	const int columns = 3;
	const int rows = (int)ceil((double)items.size() / columns);
	QString table(QStringLiteral("<table><tr><td>"));
	for(int i = 0, row = 1; i < items.size(); ++i)
	{
		table.append(items[i]);
		if (rows != row)
		{
			table.append(QStringLiteral("<br/>"));
			++row;
		}
		else if (i < items.size())
		{
			table.append(QStringLiteral("</td><td>&nbsp;&nbsp;&nbsp;</td><td>"));
			row = 1;
		}
	}
	table.append(QStringLiteral("</tr></table>"));
	return table;
#endif
}


AboutDialog::AboutDialog(QWidget* parent)
 : TextBrowserDialog(about_page_url, parent)
{
	text_browser->setSearchPaths(text_browser->searchPaths() << QStringLiteral(":/doc/licensing/html/"));
	text_browser->setHtml(about());
	text_browser->document()->adjustSize();
	updateWindowTitle();
}

void AboutDialog::sourceChanged(const QUrl& url)
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

QString AboutDialog::about()
{
	static QStringList developers_list( QStringList()
	  << QStringLiteral("Peter Curtis")
	  << QStringLiteral("Kai Pastor")
	  << QStringLiteral("Thomas Schöps %1")
	);
	
	static QStringList contributors_list( QStringList()
	  << QStringLiteral("Javier Arufe")
	  << QStringLiteral("Jon Cundill")
	  << QStringLiteral("Sławomir Cygler")
	  << QStringLiteral("Jan Dalheimer")
	  << QStringLiteral("Davide De Nardis")
	  << QStringLiteral("Eugeniy Fedirets")
	  << QStringLiteral("Pavel Fric")
	  << QStringLiteral("Anders Gressli")
	  << QStringLiteral("Peter Hoban")
	  << QStringLiteral("Henrik Johansson")
	  << QStringLiteral("Panu Karhu")
	  << QStringLiteral("Oskar Karlin")
	  << QStringLiteral("Matthias Kühlewein")
	  << QStringLiteral("Albin Larsson")
	  << QStringLiteral("Tojo Masaya")
	  << QStringLiteral("Yevhen Mazur")
	  << QStringLiteral("Fraser Mills")
	  << QStringLiteral("Vincent Poinsignon")
	  << QStringLiteral("Russell Porter")
	  << QStringLiteral("Christopher Schive")
	  << QStringLiteral("Semyon Yakimov")
	  << QStringLiteral("Aivars Zogla")
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
	          "<p>Copyright (C) 2015 The OpenOrienteering developers</p>"
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
	           arg(QStringLiteral("href=\"licensing.html\""))).
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
