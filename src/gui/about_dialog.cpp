/*
 *    Copyright 2012, 2013 Kai Pastor, Thomas Schöps
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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include <proj_api.h>

#include <mapper_config.h>


/** Puts QStringList items in an HTML table of the given number of columns. */
static QString formatTable(int columns, QStringList items)
{
	Q_ASSERT(columns > 0);
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
}


AboutDialog::AboutDialog(QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle(tr("About %1").arg(APP_NAME));
	if (parent)
	{
		setWindowModality(Qt::WindowModal);
	}
	
	// Layout and top level components.
	QGridLayout* layout = new QGridLayout();
	setLayout(layout);
	
	int left, top, right, bottom;
	layout->getContentsMargins(&left, &top, &right, &bottom);
	
	int row = 0;
	QLabel* mapper_icon_label = new QLabel("<img src=\":/images/mapper-icon/Mapper-128.png\"/>");
	QLabel* oo_icon_label     = new QLabel(
	  "<a href=\"http://openorienteering.org\"><img src=\":/images/open-orienteering.png\"/></a>" );
	layout->addWidget(mapper_icon_label, row, 0);
	layout->addWidget(oo_icon_label,     row, 1);
	connect(oo_icon_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	
	row++;
	QTabWidget* info_tabs = new QTabWidget();
	layout->addWidget(info_tabs,         row, 0, 1, 2);
	
	row++;
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
	layout->addWidget(buttons,           row, 0, 1, 2);
	connect(buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(accept()));
	
	layout->setColumnStretch(1, 1);
	
	// Tab: About OpenOrienteering Mapper
	QLabel* about_label = new QLabel(about());
	about_label->setContentsMargins(left, top, right, bottom);
	info_tabs->addTab(about_label, tr("About %1").arg(APP_NAME));
	connect(about_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	
	// Tab: License (COPYING)
	QLabel* mapper_copying_label = new QLabel(licenseText());
	mapper_copying_label->setContentsMargins(left, top, right, bottom);
	QScrollArea* mapper_copying_widget = new QScrollArea();
	mapper_copying_widget->setWidget(mapper_copying_label);
	mapper_copying_widget->setFrameStyle(QFrame::NoFrame);
	info_tabs->addTab(mapper_copying_widget, tr("License (%1)").arg("COPYING"));
	connect(mapper_copying_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	
	// Tab: Additional information
	QLabel* additional_info_label = new QLabel(additionalInformation());
	additional_info_label->setContentsMargins(left, top, right, bottom);
	QScrollArea* additional_info_widget = new QScrollArea();
	additional_info_widget->setWidget(additional_info_label);
	additional_info_widget->setFrameStyle(QFrame::NoFrame);
	info_tabs->addTab(additional_info_widget, tr("Additional information"));
	connect(additional_info_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
}

QString AboutDialog::about()
{
	static QStringList developers_list( QStringList()
	  << "Peter Curtis"
	  << "Kai Pastor"
	  << QString("Thomas Sch&ouml;ps %1").arg(tr("(project initiator)"))
	);
	
	static QStringList contributors_list( QStringList()
	  << "Jon Cundill"
	  << "Sławomir Cygler"
	  << "Jan Dalheimer"
	  << "Eugeniy Fedirets"
	  << "Anders Gressli"
	  << "Peter Hoban"
	  << "Henrik Johansson"
	  << "Oskar Karlin"
	  << "Tojo Masaya"
	  << "Vincent Poinsignon"
	  << "Russell Porter"
	  << "Christopher Schive"
	  << "Aivars Zogla"
	);
	
	static QString mapper_about(
	  QString("<h1>%1 %2</h1>"
	          "<p>"
	          "%3<br/>"
	          "<a href=\"%4\">%4</a></p>"
	          "<p>Copyright (C) 2012, 2013 The OpenOrienteering developers<br/>%5</p>"
	          "<p>%7</p>%8"
	          "<p>&nbsp;<br/>%9</p>%10"
	         ).
	    arg(APP_NAME).
	    arg(APP_VERSION).
	    arg(tr("A free software for drawing orienteering maps")).
	    arg("http://openorienteering.org").
	    arg(tr("This software is licensed under the term of the "
	           "GNU General Public License (GPL), version 3.<br/>"
	           "You are welcome to redistribute it under the terms of this license.<br/>"
	           "THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.<br>"
	           "The full license text is supplied in the file %1.").arg("COPYING")).
	    arg(tr("Developers in alphabetical order:")).
	    arg(formatTable(4, developers_list)).
	    arg(tr("For contributions, thanks to:")).
	    arg(formatTable(4, contributors_list)) );
	
	return mapper_about;
}

QString AboutDialog::licenseText()
{
	static QString mapper_copying;
	if (mapper_copying.isEmpty())
	{
		QFile mapper_copying_file(":/COPYING");
		if (mapper_copying_file.open(QIODevice::ReadOnly))
		{
			mapper_copying = mapper_copying_file.readAll();
			mapper_copying.replace("<", "&lt;");
			mapper_copying.replace(">", "&gt;");
			mapper_copying.replace('\n', "<br/>");
			mapper_copying.replace(QRegExp("&lt;(http[^;]*)&gt;"), "&lt;<a href=\"\\1\">\\1</a>&gt;");
		}
	}
	return mapper_copying;
}

QString AboutDialog::additionalInformation()
{
	static QString additional_info;
	if (additional_info.isEmpty())
	{
		QString clipper_about(tr("This program uses the <b>Clipper library</b> by Angus Johnson.") % "<br/>");
		clipper_about.append("Release ").append(CLIPPER_VERSION).append("<br><br/>");
		QFile clipper_about_file(":/3rd-party/clipper/License.txt");
		if (clipper_about_file.open(QIODevice::ReadOnly))
		{
			clipper_about.append(clipper_about_file.readAll().replace('\n', "<br/>"));
			clipper_about.append("<br/>");
		}
		clipper_about.append(tr("See <a href=\"%1\">%1</a> for more information.").arg("http://www.angusj.com/delphi/clipper.php"));	
		
		QString proj_about(tr("This program uses the <b>PROJ.4 Cartographic Projections Library</b> by Frank Warmerdam.") % "<br/>");
		proj_about.append(pj_get_release()).append("<br/><br/>");
		QFile proj_about_file(":/3rd-party/proj/COPYING");
		if (proj_about_file.open(QIODevice::ReadOnly))
		{
			proj_about.append(proj_about_file.readAll().replace('\n', "<br/>"));
			proj_about.append("<br/>");
		}
		proj_about.append(tr("See <a href=\"%1\">%1</a> for more information.").arg("http://trac.osgeo.org/proj/"));	
		
		additional_info =
		  clipper_about %
		  "<br/><br/>" % QString("_").repeated(80) % "<br/><br/>" %
		  proj_about;
	}
	
	return additional_info;
}
