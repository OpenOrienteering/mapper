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

// ### TemplateTransform ###

Template::TemplateTransform::TemplateTransform()
{
	template_x = 0;
	template_y = 0;
	template_scale_x = 1;
	template_scale_y = 1;
	template_rotation = 0;
}

// ### Template ###

Template::Template(const QString& filename, Map* map) : map(map)
{
	template_path = filename;
	template_file = QFileInfo(filename).fileName();
	template_valid = false;
	
	has_unsaved_changes = false;
	
	georeferenced = false;
	georeferencing_dirty = true;
	
	template_group = -1;
	
	updateTransformationMatrices();
}
Template::Template(const Template& other)
{
	map = other.map;
	
	template_path = other.template_path;
	template_file = other.template_file;
	template_valid = other.template_valid;
	
	has_unsaved_changes = other.has_unsaved_changes;	// TODO: = false to prevent saving the changes twice?
	
	cur_trans = other.cur_trans;
	other_trans = other.other_trans;
	georeferenced = other.georeferenced;
	georeferencing_dirty = other.georeferencing_dirty;
	
	map_to_template = other.map_to_template;
	template_to_map = other.template_to_map;
	template_to_map_other = other.template_to_map_other;
	
	template_group = other.template_group;
}
Template::~Template()
{	
}

void Template::applyTemplateTransform(QPainter* painter)
{
	painter->translate(cur_trans.template_x / 1000.0, cur_trans.template_y / 1000.0);
	painter->rotate(-cur_trans.template_rotation * (180 / M_PI));
	painter->scale(getTemplateFinalScaleX(), getTemplateFinalScaleY());
	//painter->translate(-templateCenterX, -templateCenterY);
}

void Template::setTemplateAreaDirty()
{
	QRectF template_area = calculateBoundingBox();
	map->setTemplateAreaDirty(this, template_area, 0);	// TODO: Would be better to do this with the corner points, instead of the bounding box
}

QRectF Template::calculateBoundingBox()
{
	// Create bounding box by calculating the positions of all corners of the transformed extent rect
	QRectF extent = getExtent();
	
	QRectF bbox(templateToMap(extent.topLeft()).toQPointF(), QSizeF(0, 0));
	rectInclude(bbox, templateToMap(extent.topRight()).toQPointF());
	rectInclude(bbox, templateToMap(extent.bottomRight()).toQPointF());
	rectInclude(bbox, templateToMap(extent.bottomLeft()).toQPointF());
	return bbox;
}

void Template::drawOntoTemplate(MapCoordF* coords, int num_coords, QColor color, float width, QRectF map_bbox)
{
	assert(canBeDrawnOnto());
	assert(num_coords > 1);
	
	if (!map_bbox.isValid())
	{
		map_bbox = QRectF(coords[0].getX(), coords[0].getY(), 0, 0);
		for (int i = 1; i < num_coords; ++i)
			rectInclude(map_bbox, coords[i].toQPointF());
	}
	float radius = qMin(getTemplateFinalScaleX(), getTemplateFinalScaleY()) * qMax((width+1) / 2, 1.0f);
	QRectF radius_bbox = QRectF(map_bbox.left() - radius, map_bbox.top() - radius, map_bbox.width() + 2*radius, map_bbox.height() + 2*radius);
	
	QPointF* points = new QPointF[num_coords];
	for (int i = 0; i < num_coords; ++i)
		points[i] = mapToTemplateQPoint(coords[i]);
	
	drawOntoTemplateImpl(points, num_coords, color, width);
	map->setTemplateAreaDirty(this, radius_bbox, 0);
	
	delete[] points;
	
	setHasUnsavedChanges(true);
}

void Template::addPassPoint(const Template::PassPoint& point, int pos)
{
	passpoints.insert(passpoints.begin() + pos, point);
}
void Template::deletePassPoint(int pos)
{
	passpoints.erase(passpoints.begin() + pos);
}
void Template::clearPassPoints()
{
	passpoints.clear();
	setGeoreferencingDirty(true);
	georeferenced = false;
}

void Template::switchTransforms()
{
	setTemplateAreaDirty();
	
	TemplateTransform temp = cur_trans;
	cur_trans = other_trans;
	other_trans = temp;
	template_to_map_other = template_to_map;
	updateTransformationMatrices();
	
	georeferenced = !georeferenced;
	setTemplateAreaDirty();
	map->setTemplatesDirty();
}
void Template::getTransform(Template::TemplateTransform& out)
{
	out = cur_trans;
}
void Template::setTransform(const Template::TemplateTransform& transform)
{
	setTemplateAreaDirty();
	
	cur_trans = transform;
	updateTransformationMatrices();
	
	setTemplateAreaDirty();
	map->setTemplatesDirty();
}
void Template::getOtherTransform(Template::TemplateTransform& out)
{
	out = other_trans;
}
void Template::setOtherTransform(const Template::TemplateTransform& transform)
{
	other_trans = transform;
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

void Template::updateTransformationMatrices()
{
	double cosr = cos(-cur_trans.template_rotation);
	double sinr = sin(-cur_trans.template_rotation);
	double scale_x = getTemplateFinalScaleX();
	double scale_y = getTemplateFinalScaleY();
	
	template_to_map.setSize(3, 3);
	template_to_map.set(0, 0, scale_x * cosr);
	template_to_map.set(0, 1, scale_y * (-sinr));
	template_to_map.set(1, 0, scale_x * sinr);
	template_to_map.set(1, 1, scale_y * cosr);
	template_to_map.set(0, 2, cur_trans.template_x / 1000.0);
	template_to_map.set(1, 2, cur_trans.template_y / 1000.0);
	template_to_map.set(2, 0, 0);
	template_to_map.set(2, 1, 0);
	template_to_map.set(2, 2, 1);
	
	template_to_map.invert(map_to_template);
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
	
	cur_trans.template_scale_x = open_dialog.getMpp();
	cur_trans.template_scale_y = open_dialog.getMpp();
	
	cur_trans.template_x = main_view->getPositionX();
	cur_trans.template_y = main_view->getPositionY();
	
	updateTransformationMatrices();
	
	return true;
}
void TemplateImage::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	painter->setOpacity(opacity);
	painter->drawPixmap(QPointF(-pixmap->width() * 0.5, -pixmap->height() * 0.5), *pixmap);
}
QRectF TemplateImage::getExtent()
{
	return QRectF(-pixmap->width() * 0.5, -pixmap->height() * 0.5, pixmap->width(), pixmap->height());
}

double TemplateImage::getTemplateFinalScaleX() const
{
	return cur_trans.template_scale_x * 1000 / map->getScaleDenominator();
}
double TemplateImage::getTemplateFinalScaleY() const
{
	return cur_trans.template_scale_y * 1000 / map->getScaleDenominator();
}

void TemplateImage::drawOntoTemplateImpl(QPointF* points, int num_points, QColor color, float width)
{
	for (int i = 0; i < num_points; ++i)
		points[i] = QPointF(points[i].x() + pixmap->width() * 0.5, points[i].y() + pixmap->height() * 0.5);
	
    QPainter painter;
	painter.begin(pixmap);
	
	QPen pen(color);
	pen.setWidthF(width);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing);
	
	painter.drawPolyline(points, num_points);
	
	painter.end();
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
