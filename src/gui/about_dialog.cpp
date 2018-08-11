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

#include "about_dialog.h"

#include <cmath>

#include <QApplication>
#include <QTextBrowser>

#include "mapper_config.h"


namespace OpenOrienteering {

namespace {

/**
 * @brief An URL identifying the main "about" page.
 * 
 * The main page's text will be set directly, thus not having a valid URL.
 * But an empty URL will be ignored by QTextBrowser's history, leading to
 * unexpected behaviour of backward navigation.
 */
const QUrl& aboutPageUrl()
{
	static auto url = QUrl(QString::fromLatin1("#ABOUT"));
	return url;
}

/**
 * Puts the items of a QStringList into an HTML block or a sequence of blocks.
 */
QString formatBlock(const QStringList& items)
{
#if defined(Q_OS_ANDROID) // or any other small-screen device
	QString block = QLatin1String("<p>")
	                + items.join(QString::fromLatin1(", "))
	                + QLatin1String("</p>");
#else
	QString block;
	block.reserve(100 + 30 * items.size());
	block.append(QLatin1String("<table><tr><td>"));
	constexpr int columns = 3;
	const int rows = (int)ceil((double)items.size() / columns);
	for (int i = 0, row = 1; i < items.size(); ++i)
	{
		block.append(items[i]);
		if (rows != row)
		{
			block.append(QString::fromLatin1("<br/>"));
			++row;
		}
		else if (i < items.size())
		{
			block.append(QString::fromLatin1("</td><td>&nbsp;&nbsp;&nbsp;</td><td>"));
			row = 1;
		}
	}
	block.append(QString::fromLatin1("</td></tr></table>"));
#endif
	return block;
}


}  // namespace



AboutDialog::AboutDialog(QWidget* parent)
 : TextBrowserDialog(aboutPageUrl(), parent)
{
	text_browser->setHtml(about());
	text_browser->document()->adjustSize();
	updateWindowTitle();
}

void AboutDialog::sourceChanged(const QUrl& url)
{
	if (url == aboutPageUrl())
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
	  << QString::fromLatin1("Peter Curtis (2012-2013)")
	  << QString::fromLatin1("<b>Kai Pastor</b>")
	  << QString::fromUtf8("Thomas Schöps (2012-2014, %1)")
	);
	
	static QStringList contributors_list( QStringList()
	  << QString::fromLatin1("Arrizal Amin")
	  << QString::fromLatin1("Javier Arufe")
	  << QString::fromLatin1("Eric Boulet")
	  << QString::fromLatin1("Jon Cundill")
	  << QString::fromUtf8("Sławomir Cygler")
	  << QString::fromLatin1("Jan Dalheimer")
	  << QString::fromLatin1("Davide De Nardis")
	  << QString::fromLatin1("Eugeniy Fedirets")
	  << QString::fromLatin1("Joao Franco")
	  << QString::fromLatin1("Pavel Fric")
	  << QString::fromLatin1("Naofumi Fukue")
	  << QString::fromLatin1("Anders Gressli")
	  << QString::fromLatin1("Peter Hoban")
	  << QString::fromLatin1("Henrik Johansson")
	  << QString::fromLatin1("Panu Karhu")
	  << QString::fromLatin1("Oskar Karlin")
	  << QString::fromLatin1("Nikolay Korotkiy")
	  << QString::fromLatin1("Mitchell Krome")
	  << QString::fromUtf8("Matthias Kühlewein")
	  << QString::fromLatin1("Albin Larsson")
	  << QString::fromUtf8("István Marczis")
	  << QString::fromLatin1("Tojo Masaya")
	  << QString::fromLatin1("Yevhen Mazur")
	  << QString::fromLatin1("Fraser Mills")
	  << QString::fromLatin1("Vincent Poinsignon")
	  << QString::fromLatin1("Russell Porter")
	  << QString::fromLatin1("Adhika Setya Pramudita")
	  << QString::fromLatin1("Christopher Schive")
	  << QString::fromLatin1("Arif Suryawan")
	  << QString::fromLatin1("Jan-Gerard van der Toorn")
	  << QString::fromLatin1("Semyon Yakimov")
	  << QString::fromLatin1("Aivars Zogla")
	);
	
	QString mapper_about = QString::fromLatin1(
	  "<html><head>"
	  "<title>%0</title>"
	  "</head><body>"
	  "<table><tr>"
	  "<td><img src=\":/images/mapper-icon/Mapper-128.png\"/></td>"
	  "<td><img src=\":/images/open-orienteering.png\"/></td>"
	  "</tr></table>"
	  "<h1>%1</h1>"
	  "<p>"
	  "<em>%3</em><br/>"
	  "<a href=\"%4\">%4</a></p>"
	  "<p>Copyright (C) 2018 The OpenOrienteering developers</p>"
	  "<p>%5</p>"
	  "<p>%6</p>"
	  "<p>%7</p>"
	  "<p>%8</p>%9"
	  "<p>&nbsp;<br/>%10</p>%11"
	  "</body></html>"
	).arg(
	  tr("About %1").arg(APP_NAME), // %0
	  qApp->applicationDisplayName(),   // %1
	  tr("A free software for drawing orienteering maps"), // %3
	  QString::fromLatin1("https://www.openorienteering.org/apps/mapper/"), // %4
	  tr("This program is free software: you can redistribute it "
	     "and/or modify it under the terms of the "
	     "<a %1>GNU General Public License (GPL), version&nbsp;3</a>, "
	     "as published by the Free Software Foundation.").arg(QString::fromLatin1("href=\"file:doc:/common-licenses/GPL-3.txt\"")), // %5
	    // %6
	  tr("This program is distributed in the hope that it will be useful, "
	     "but WITHOUT ANY WARRANTY; without even the implied warranty of "
	     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
	     "GNU General Public License (GPL), version&nbsp;3, for "
	     "<a %1>more details</a>.").arg(QString::fromLatin1("href=\"file:doc:/common-licenses/GPL-3.txt#L589\"")), // %6
	  tr("<a %1>All about licenses, copyright notices, conditions and disclaimers.</a>").arg(QString::fromLatin1("href=\"file:doc:/licensing.html\"")) // %7
	).arg(
	  tr("The OpenOrienteering developers in alphabetical order:"), // %8
	  formatBlock(developers_list).arg(tr("(project initiator)").replace(QLatin1Char('('), QString{}).replace(QLatin1Char(')'), QString{})), // %9
	  tr("For contributions, thanks to:"), // %10
	  formatBlock(contributors_list) // %11
	);
	
	return mapper_about;
}


}  // namespace OpenOrienteering
