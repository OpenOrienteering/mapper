/*
 *    Copyright 2011 Thomas Schöps
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


#include "template.h"

#include <QtGui>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>

#include "util.h"
#include "map.h"

// ### Template ###

Template::Template(const QString& filename, Map* map) : map(map)
{
	template_path = filename;
	template_file = QFileInfo(filename).fileName();
	
	template_x = 0;
	template_y = 0;
	template_scale_x = 1;
	template_scale_y = 1;
	template_rotation = 0;
	
	template_group = -1;
	
	//templateCenterX = 0;
	//templateCenterY = 0;
	template_valid = false;
}
Template::Template(const Template& other)
{
	map = other.map;
	
	template_path = other.template_path;
	template_file = other.template_file;
	
	template_x = other.template_x;
	template_y = other.template_y;
	template_scale_x = other.template_scale_x;
	template_scale_y = other.template_scale_y;
	template_rotation = other.template_rotation;
	
	template_group = other.template_group;
	
	//templateCenterX = other.templateCenterX;
	//templateCenterY = other.templateCenterY;
	template_valid = other.template_valid;
}
Template::~Template()
{	
}

void Template::applyTemplateTransform(QPainter* painter)
{
	painter->translate(template_x / 1000.0, template_y / 1000.0);
	painter->rotate(-template_rotation);
	painter->scale(getTemplateFinalScaleX(), getTemplateFinalScaleY());
	//painter->translate(-templateCenterX, -templateCenterY);
}

void Template::setTemplateAreaDirty()
{
	QRectF template_area = calculateBoundingBox();
	map->setTemplateAreaDirty(this, template_area);	// TODO: Would be better to do this with the corner points, instead of the bounding box
}

QRectF Template::calculateBoundingBox()
{
	// Calculate transformation matrix elements
	double cosr = cos(-template_rotation);
	double sinr = sin(-template_rotation);
	double scale_x = getTemplateFinalScaleX();
	double scale_y = getTemplateFinalScaleY();
	
	double t00 = scale_x * cosr;
	double t01 = scale_y * (-sinr);
	double t10 = scale_x * sinr;
	double t11 = scale_y * cosr;
	double t02 = template_x / 1000.0;
	double t12 = template_y / 1000.0;
	
	// TODO: Test
	QRectF testA(-10, -10, 0, 0);
	QRectF testB(20, 20, 0, 0);
	QRectF test = testA.united(testB);
	
	// Create bounding box by calculating the positions of all corners of the transformed extent rect
	QRectF extent = getExtent();
	QRectF bbox(extent.left() * t00 + extent.top() * t01 + t02, extent.left() * t10 + extent.top() * t11 + t12, 0, 0);
	
	QPointF top_right(extent.right() * t00 + extent.top() * t01 + t02, extent.right() * t10 + extent.top() * t11 + t12);
	rectInclude(bbox, top_right);
	
	QPointF bottom_right(extent.right() * t00 + extent.bottom() * t01 + t02, extent.right() * t10 + extent.bottom() * t11 + t12);
	rectInclude(bbox, bottom_right);
	
	QPointF bottom_left(extent.left() * t00 + extent.bottom() * t01 + t02, extent.left() * t10 + extent.bottom() * t11 + t12);
	rectInclude(bbox, bottom_left);
	
	return bbox;
}

Template* Template::templateForFile(const QString& path, Map* map)
{
	if (path.endsWith(".png", Qt::CaseInsensitive) || path.endsWith(".bmp", Qt::CaseInsensitive) || path.endsWith(".jpg", Qt::CaseInsensitive) ||
		path.endsWith(".gif", Qt::CaseInsensitive) || path.endsWith(".jpeg", Qt::CaseInsensitive) || path.endsWith(".tif", Qt::CaseInsensitive) ||
		path.endsWith(".tiff", Qt::CaseInsensitive))
		return new TemplateImage(path, map);
	else if (path.endsWith(".ocd", Qt::CaseInsensitive) || path.endsWith(".omap", Qt::CaseInsensitive))
		return NULL;	// TODO
	else
		return NULL;
}

// ### TemplateImage ###

TemplateImage::TemplateImage(const QString& filename, Map* map): Template(filename, map)
{
	pixmap = new QPixmap(filename);
	if (pixmap->isNull())
	{
		delete pixmap;
		pixmap = NULL;
		return;
	}
	
	template_valid = true;
	//templateCenterX = 0.5f * pixmap->width();
	//templateCenterY = 0.5f * pixmap->height();
}
TemplateImage::TemplateImage(const TemplateImage& other): Template(other)
{
	pixmap = new QPixmap(*other.pixmap);
}
TemplateImage::~TemplateImage()
{
	delete pixmap;
}
Template* TemplateImage::duplicate()
{
	TemplateImage* copy = new TemplateImage(*this);
	return copy;
}

bool TemplateImage::open(QWidget* dialog_parent, MapView* main_view)
{
	TemplateImageOpenDialog open_dialog(dialog_parent);
	open_dialog.setWindowModality(Qt::WindowModal);
	if (open_dialog.exec() == QDialog::Rejected)
		return false;
	
	template_scale_x = open_dialog.getMpp();
	template_scale_y = open_dialog.getMpp();
	
	template_x = main_view->getPositionX();
	template_y = main_view->getPositionY();
	
	return true;
}
void TemplateImage::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale)
{
	painter->drawPixmap(QPointF(-pixmap->width() * 0.5, -pixmap->height() * 0.5), *pixmap);
}
QRectF TemplateImage::getExtent()
{
	return QRectF(-pixmap->width() * 0.5, -pixmap->height() * 0.5, pixmap->width(), pixmap->height());
}

double TemplateImage::getTemplateFinalScaleX() const
{
    return template_scale_x * 1000 / map->getScaleDenominator();
}
double TemplateImage::getTemplateFinalScaleY() const
{
	return template_scale_y * 1000 / map->getScaleDenominator();
}

TemplateImageOpenDialog::TemplateImageOpenDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Open image template"));
	
	QLabel* mpp_label = new QLabel(tr("Meters per pixel:"));
	mpp_edit = new QLineEdit("1");
	mpp_edit->setValidator(new DoubleValidator(0, 999999, mpp_edit));
	
	QHBoxLayout* mpp_layout = new QHBoxLayout();
	mpp_layout->addWidget(mpp_label);
	mpp_layout->addWidget(mpp_edit);
	mpp_layout->addStretch(1);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	open_button = new QPushButton(QIcon("images/arrow-right.png"), tr("Open"));
	open_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(open_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(mpp_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(open_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(mpp_edit, SIGNAL(textChanged(QString)), this, SLOT(mppChanged(QString)));
	
	mpp = 1;
}
void TemplateImageOpenDialog::mppChanged(const QString& new_text)
{
	bool ok = false;
	mpp = new_text.toDouble(&ok);
}

#include "template.moc"
