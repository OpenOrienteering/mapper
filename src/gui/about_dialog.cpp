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

#include "../mapper_resource.h"

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
	QString mapper_about = 
	  QString("%1 %2<br/>"
	          "Copyright (C) 2012, 2013 Thomas Sch&ouml;ps<br/>"
	          "<a href=\"%3\">%3</a><br/>" ).
	    arg(APP_NAME).
	    arg(APP_VERSION).
	    arg("http://openorienteering.org") %
	  tr("This program comes with ABSOLUTELY NO WARRANTY;<br/>"
	     "This is free software, and you are welcome to redistribute it<br/>"
	     "under certain conditions; see the file %1 for details.<br/><br/>").
	    arg("COPYING") %
	  tr("Developers in alphabetical order:<br/>%1<br/>"
	     "For contributions, thanks to:<br/>%2" ).
	    arg(QString("Peter Curtis<br/>Kai Pastor<br/>Thomas Sch&ouml;ps %1<br/>").arg(tr("(project initiator)"))).
	    arg("Jon Cundill<br/>Jan Dalheimer<br/>Eugeniy Fedirets<br/>Anders Gressli<br/>Peter Hoban<br/>"
	        "Henrik Johansson<br/>Oskar Karlin<br/>Tojo Masaya<br/>Vincent Poinsignon<br/>Russell Porter<br/>"
	        "Christopher Schive<br/>Aivars Zogla<br/>");
	QLabel* about_label = new QLabel(mapper_about);
	about_label->setContentsMargins(left, top, right, bottom);
	info_tabs->addTab(about_label, tr("About %1").arg(APP_NAME));
	connect(about_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	
	// Tab: License (COPYING)
	QString mapper_copying;
	QFile mapper_copying_file(MapperResource::locate(MapperResource::ABOUT, "COPYING"));
	if (mapper_copying_file.open(QIODevice::ReadOnly))
	{
		mapper_copying = mapper_copying_file.readAll();
		mapper_copying.replace("<", "&lt;");
		mapper_copying.replace(">", "&gt;");
		mapper_copying.replace('\n', "<br/>");
		mapper_copying.replace(QRegExp("&lt;(http[^;]*)&gt;"), "&lt;<a href=\"\\1\">\\1</a>&gt;");
		
		QLabel* mapper_copying_label = new QLabel(mapper_copying);
		mapper_copying_label->setContentsMargins(left, top, right, bottom);
		QScrollArea* mapper_copying_widget = new QScrollArea();
		mapper_copying_widget->setWidget(mapper_copying_label);
		mapper_copying_widget->setFrameStyle(QFrame::NoFrame);
		info_tabs->addTab(mapper_copying_widget, tr("License (%1)").arg("COPYING"));
		connect(mapper_copying_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	}
	
	// Tab: Additional information
	QString clipper_about(tr("This program uses the <b>Clipper library</b> by Angus Johnson.") % "<br/><br/>");
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
	
	QLabel* additional_info_label = new QLabel( 
	  clipper_about %
	  "<br/><br/>" % QString("_").repeated(80) % "<br/><br/>" %
	  proj_about 
	);
	additional_info_label->setContentsMargins(left, top, right, bottom);
	QScrollArea* additional_info_widget = new QScrollArea();
	additional_info_widget->setWidget(additional_info_label);
	additional_info_widget->setFrameStyle(QFrame::NoFrame);
	info_tabs->addTab(additional_info_widget, tr("Additional information"));
	connect(additional_info_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
}
