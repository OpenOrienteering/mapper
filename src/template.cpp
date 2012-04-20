/*
 *    Copyright 2012 Thomas Schöps
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
#include "template_image.h"
#include "template_gps.h"
#include "template_map.h"

// ### TemplateTransform ###

Template::TemplateTransform::TemplateTransform()
{
	template_x = 0;
	template_y = 0;
	template_scale_x = 1;
	template_scale_y = 1;
	template_rotation = 0;
}
void Template::TemplateTransform::save(QFile* file)
{
	file->write((const char*)&template_x, sizeof(qint64));
	file->write((const char*)&template_y, sizeof(qint64));
	
	file->write((const char*)&template_scale_x, sizeof(qint64));
	file->write((const char*)&template_scale_y, sizeof(qint64));
	file->write((const char*)&template_rotation, sizeof(qint64));
}
void Template::TemplateTransform::load(QFile* file)
{
	file->read((char*)&template_x, sizeof(qint64));
	file->read((char*)&template_y, sizeof(qint64));
	
	file->read((char*)&template_scale_x, sizeof(qint64));
	file->read((char*)&template_scale_y, sizeof(qint64));
	file->read((char*)&template_rotation, sizeof(qint64));
}

// ### PassPoint ###

void Template::PassPoint::save(QFile* file)
{
	file->write((const char*)&src_coords_template, sizeof(MapCoordF));
	file->write((const char*)&src_coords_map, sizeof(MapCoordF));
	file->write((const char*)&dest_coords_map, sizeof(MapCoordF));
	file->write((const char*)&calculated_coords_map, sizeof(MapCoordF));
	file->write((const char*)&error, sizeof(double));
}
void Template::PassPoint::load(QFile* file)
{
	file->read((char*)&src_coords_template, sizeof(MapCoordF));
	file->read((char*)&src_coords_map, sizeof(MapCoordF));
	file->read((char*)&dest_coords_map, sizeof(MapCoordF));
	file->read((char*)&calculated_coords_map, sizeof(MapCoordF));
	file->read((char*)&error, sizeof(double));
}

// ### Template ###

Template::Template(const QString& filename, Map* map) : map(map)
{
	template_path = filename;
	template_file = QFileInfo(filename).fileName();
	template_valid = false;
	
	has_unsaved_changes = false;
	
	adjusted = false;
	adjustment_dirty = true;
	
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
	adjusted = other.adjusted;
	adjustment_dirty = other.adjustment_dirty;
	
	map_to_template = other.map_to_template;
	template_to_map = other.template_to_map;
	template_to_map_other = other.template_to_map_other;
	
	template_group = other.template_group;
}
Template::~Template()
{	
}

void Template::saveTemplateParameters(QFile* file)
{
	saveString(file, template_file);
	
	cur_trans.save(file);
	other_trans.save(file);
	
	file->write((const char*)&adjusted, sizeof(bool));
	file->write((const char*)&adjustment_dirty, sizeof(bool));
	
	int num_passpoints = (int)passpoints.size();
	file->write((const char*)&num_passpoints, sizeof(int));
	for (int i = 0; i < num_passpoints; ++i)
		passpoints[i].save(file);
	
	map_to_template.save(file);
	template_to_map.save(file);
	template_to_map_other.save(file);
	
	file->write((const char*)&template_group, sizeof(int));
}
void Template::loadTemplateParameters(QFile* file)
{
	loadString(file, template_file);
	
	cur_trans.load(file);
	other_trans.load(file);
	
	file->read((char*)&adjusted, sizeof(bool));
	file->read((char*)&adjustment_dirty, sizeof(bool));
	
	int num_passpoints;
	file->read((char*)&num_passpoints, sizeof(int));
	passpoints.resize(num_passpoints);
	for (int i = 0; i < num_passpoints; ++i)
		passpoints[i].load(file);
	
	map_to_template.load(file);
	template_to_map.load(file);
	template_to_map_other.load(file);
	
	file->read((char*)&template_group, sizeof(int));
}

bool Template::changeTemplateFile(const QString& filename)
{
	bool ret_val = changeTemplateFileImpl(filename);
	if (ret_val)
	{
		template_path = filename;
		template_file = QFileInfo(filename).fileName();
		template_valid = true;
		
		setTemplateAreaDirty();
		map->setTemplatesDirty();
	}
	return ret_val;
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
	setAdjustmentDirty(true);
	adjusted = false;
}

void Template::switchTransforms()
{
	setTemplateAreaDirty();
	
	TemplateTransform temp = cur_trans;
	cur_trans = other_trans;
	other_trans = temp;
	template_to_map_other = template_to_map;
	updateTransformationMatrices();
	
	adjusted = !adjusted;
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

Template* Template::templateForFile(const QString& path, Map* map, MapEditorController* controller)
{
	if (path.endsWith(".png", Qt::CaseInsensitive) || path.endsWith(".bmp", Qt::CaseInsensitive) || path.endsWith(".jpg", Qt::CaseInsensitive) ||
		path.endsWith(".gif", Qt::CaseInsensitive) || path.endsWith(".jpeg", Qt::CaseInsensitive) || path.endsWith(".tif", Qt::CaseInsensitive) ||
		path.endsWith(".tiff", Qt::CaseInsensitive))
		return new TemplateImage(path, map);
	else if (path.endsWith(".ocd", Qt::CaseInsensitive) || path.endsWith(".omap", Qt::CaseInsensitive))
		return new TemplateMap(path, map);	// TODO
	else if (path.endsWith(".gpx", Qt::CaseInsensitive) || path.endsWith(".dxf", Qt::CaseInsensitive))
		return new TemplateGPS(path, map, controller);
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

#include "template.moc"
