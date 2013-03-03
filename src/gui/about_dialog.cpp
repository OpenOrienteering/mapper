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

AboutDialog::AboutDialog(QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle(tr("About %1").arg(APP_NAME));
	
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
	    arg("<tt>COPYING</tt>") %
	  tr("Developers in alphabetical order:<br/>%1<br/>"
	     "For contributions, thanks to:<br/>%2"
		).
		  arg(QString("Peter Curtis<br/>Kai Pastor<br/>Thomas Sch&ouml;ps %1<br/>").arg(tr("(project initiator)"))).
		  arg("Jon Cundill<br/>Jan Dalheimer<br/>Eugeniy Fedirets<br/>Anders Gressli<br/>Peter Hoban<br/>"
		      "Henrik Johansson<br/>Oskar Karlin<br/>Tojo Masaya<br/>Vincent Poinsignon<br/>Russell Porter<br/>"
			  "Christopher Schive<br/>Aivars Zogla<br/>");
	
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
	
	QLabel* mapper_icon_label = new QLabel("<img src=\":/images/mapper-icon/Mapper-128.png\"/>");
	QLabel* oo_icon_label     = new QLabel(
	  "<a href=\"http://openorienteering.org\"><img src=\":/images/open-orienteering.png\"/></a>" );
	QTabWidget* stack = new QTabWidget();
	QLabel* about_label = new QLabel(mapper_about);
	stack->addTab(about_label, tr("About %1").arg(APP_NAME));
	QLabel* additional_info_label = new QLabel( 
	  clipper_about %
	  "<br/><br/>" % QString("_").repeated(80) % "<br/><br/>" %
	  proj_about 
	);
	QScrollArea* additional_info_widget = new QScrollArea();
	additional_info_widget->setWidget(additional_info_label);
	additional_info_widget->setFrameStyle(QFrame::NoFrame);
	stack->addTab(additional_info_widget, tr("Additional information"));
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
	
	QGridLayout* layout = new QGridLayout();
	setLayout(layout);
	
	int row = 0;
	layout->addWidget(mapper_icon_label, row, 0);
	layout->addWidget(oo_icon_label,     row, 1);
	row++;
	layout->addWidget(stack,             row, 0, 1, 2);
	row++;
	layout->addWidget(buttons,           row, 0, 1, 2);
	layout->setColumnStretch(1, 1);
	
	int left, top, right, bottom;
	layout->getContentsMargins(&left, &top, &right, &bottom);
	about_label->setContentsMargins(left, top, right, bottom);
	additional_info_label->setContentsMargins(left, top, right, bottom);
	
	connect(oo_icon_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	connect(about_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	connect(additional_info_label, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));
	connect(buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(accept()));
	
	if (parent)
	{
		setWindowModality(Qt::WindowModal);
	}
}
