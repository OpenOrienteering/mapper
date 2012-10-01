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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>

#include "util.h"
#include "map.h"
#include "template_image.h"
#include "template_gps.h"
#include "template_map.h"

// ### TemplateTransform ###

TemplateTransform::TemplateTransform()
{
	template_x = 0;
	template_y = 0;
	template_scale_x = 1;
	template_scale_y = 1;
	template_rotation = 0;
}
void TemplateTransform::save(QIODevice* file)
{
	file->write((const char*)&template_x, sizeof(qint64));
	file->write((const char*)&template_y, sizeof(qint64));
	
	file->write((const char*)&template_scale_x, sizeof(qint64));
	file->write((const char*)&template_scale_y, sizeof(qint64));
	file->write((const char*)&template_rotation, sizeof(qint64));
}
void TemplateTransform::load(QIODevice* file)
{
	file->read((char*)&template_x, sizeof(qint64));
	file->read((char*)&template_y, sizeof(qint64));
	
	file->read((char*)&template_scale_x, sizeof(qint64));
	file->read((char*)&template_scale_y, sizeof(qint64));
	file->read((char*)&template_rotation, sizeof(qint64));
}

// ### Template ###

Template::Template(const QString& path, Map* map) : map(map)
{
	template_path = path;
	template_file = QFileInfo(path).fileName();
	template_relative_path = "";
	template_state = Unloaded;
	
	has_unsaved_changes = false;
	
	is_georeferenced = false;
	
	adjusted = false;
	adjustment_dirty = true;
	
	template_group = -1;
	
	updateTransformationMatrices();
}
Template::~Template()
{
	assert(template_state != Loaded);
}

Template* Template::duplicate()
{
	Template* copy = duplicateImpl();
	
	copy->map = map;
	
	copy->template_path = template_path;
	copy->template_relative_path = template_relative_path;
	copy->template_file = template_file;
	copy->template_state = template_state;
	copy->is_georeferenced = is_georeferenced;
	
	// Prevent saving the changes twice (if has_unsaved_changes == true)
	copy->has_unsaved_changes = false;
	
	copy->transform = transform;
	copy->other_transform = other_transform;
	copy->adjusted = adjusted;
	copy->adjustment_dirty = adjustment_dirty;
	
	copy->map_to_template = map_to_template;
	copy->template_to_map = template_to_map;
	copy->template_to_map_other = template_to_map_other;
	
	copy->template_group = template_group;
	
	return copy;
}

void Template::saveTemplateConfiguration(QIODevice* stream)
{
	saveString(stream, template_file);
	
	stream->write((const char*)&is_georeferenced, sizeof(bool));
	
	if (!is_georeferenced)
	{
		transform.save(stream);
		other_transform.save(stream);
		
		stream->write((const char*)&adjusted, sizeof(bool));
		stream->write((const char*)&adjustment_dirty, sizeof(bool));
		
		int num_passpoints = (int)passpoints.size();
		stream->write((const char*)&num_passpoints, sizeof(int));
		for (int i = 0; i < num_passpoints; ++i)
			passpoints[i].save(stream);
		
		map_to_template.save(stream);
		template_to_map.save(stream);
		template_to_map_other.save(stream);
		
		stream->write((const char*)&template_group, sizeof(int));
	}
	
	saveTypeSpecificTemplateConfiguration(stream);
}
void Template::loadTemplateConfiguration(QIODevice* stream, int version)
{
	loadString(stream, template_file);
	
	if (version >= 27)
		stream->read((char*)&is_georeferenced, sizeof(bool));
	else
		is_georeferenced = false;
	
	if (!is_georeferenced)
	{
		transform.load(stream);
		other_transform.load(stream);
		
		stream->read((char*)&adjusted, sizeof(bool));
		stream->read((char*)&adjustment_dirty, sizeof(bool));
		
		int num_passpoints;
		stream->read((char*)&num_passpoints, sizeof(int));
		passpoints.resize(num_passpoints);
		for (int i = 0; i < num_passpoints; ++i)
			passpoints[i].load(stream, version);
		
		map_to_template.load(stream);
		template_to_map.load(stream);
		template_to_map_other.load(stream);
		
		stream->read((char*)&template_group, sizeof(int));
	}
	
	if (version >= 27)
		loadTypeSpecificTemplateConfiguration(stream, version);
	else
	{
		// Adjust old file format version's templates
		is_georeferenced = getTemplateType() == "TemplateTrack";
		if (getTemplateType() == "TemplateImage")
		{
			transform.template_scale_x *= 1000.0 / map->getScaleDenominator();
			transform.template_scale_y *= 1000.0 / map->getScaleDenominator();
			updateTransformationMatrices();
		}
	}
}

void Template::switchTemplateFile(const QString& new_path)
{
	if (template_state == Loaded)
	{
		setTemplateAreaDirty();
		unloadTemplateFile();
	}
	
	template_path = new_path;
	template_file = QFileInfo(new_path).fileName();
	template_relative_path = "";
	
	if (template_state == Loaded)
		loadTemplateFile(false);
}

bool Template::execSwitchTemplateFileDialog(QWidget* dialog_parent)
{
	QString path = QFileDialog::getOpenFileName(dialog_parent, tr("Find the moved template file"),
												 QString(), tr("All files (*.*)"));
	path = QFileInfo(path).canonicalFilePath();
	if (path.isEmpty())
		return false;
	
	bool need_to_load = getTemplateState() != Template::Loaded;
	QString old_path = getTemplatePath();
	switchTemplateFile(path);
	if (need_to_load)
		loadTemplateFile(false);
	
	if (getTemplateState() == Template::Loaded)
		return true;
	else
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot change the template to this file! Is the format of the file correct for this template type?"));
		// Revert change
		switchTemplateFile(old_path);
		return false;
	}
}

bool Template::configureAndLoad(QWidget* dialog_parent)
{
	if (!preLoadConfiguration(dialog_parent))
		return false;
	if (!loadTemplateFile(true))
		return false;
	if (!postLoadConfiguration(dialog_parent))
		return false;
	return true;
}

bool Template::tryToFindAndReloadTemplateFile(QString map_directory, bool* out_loaded_from_map_dir)
{
	if (!map_directory.isEmpty() && !map_directory.endsWith('/'))
		map_directory.append('/');
	if (out_loaded_from_map_dir)
		*out_loaded_from_map_dir = false;
	
	// First try relative path (if this information is available)
	if (!getTemplateRelativePath().isEmpty() && !map_directory.isEmpty())
	{
		QString old_absolute_path = getTemplatePath();
		setTemplatePath(map_directory + getTemplateRelativePath());
		loadTemplateFile(false);
		if (getTemplateState() == Template::Loaded)
			return true;
		else
		{
			// Failure: restore old absolute path
			setTemplatePath(old_absolute_path);
		}
	}
	
	// Then try absolute path
	loadTemplateFile(false);
	if (getTemplateState() == Template::Loaded)
		return true;
	
	// Then try the template filename in the map's directory
	if (!map_directory.isEmpty())
	{
		QString old_absolute_path = getTemplatePath();
		setTemplatePath(map_directory + getTemplateFilename());
		loadTemplateFile(false);
		if (getTemplateState() == Template::Loaded)
		{
			if (out_loaded_from_map_dir)
				*out_loaded_from_map_dir = true;
			return true;
		}
		else
		{
			// Failure: restore old absolute path
			setTemplatePath(old_absolute_path);
		}
	}
	return false;
}

bool Template::loadTemplateFile(bool configuring)
{
	assert(template_state != Loaded);
	bool result = loadTemplateFileImpl(configuring);
	if (result)
	{
		template_state = Loaded;
		emit templateStateChanged();
	}
	else if (template_state != Invalid)
	{
		template_state = Invalid;
		emit templateStateChanged();
	}
	return result;
}

void Template::unloadTemplateFile()
{
	assert(template_state == Loaded);
	if (hasUnsavedChanges())
	{
		saveTemplateFile();
		setHasUnsavedChanges(false);
	}
	unloadTemplateFileImpl();
	template_state = Unloaded;
	emit templateStateChanged();
}

void Template::applyTemplateTransform(QPainter* painter)
{
	assert(!is_georeferenced);
	painter->translate(transform.template_x / 1000.0, transform.template_y / 1000.0);
	painter->rotate(-transform.template_rotation * (180 / M_PI));
	painter->scale(transform.template_scale_x, transform.template_scale_y);
}

QRectF Template::getTemplateExtent()
{
	assert(!is_georeferenced);
	return infinteRectF();
}

void Template::setTemplateAreaDirty()
{
	QRectF template_area = calculateTemplateBoundingBox();
	map->setTemplateAreaDirty(this, template_area, getTemplateBoundingBoxPixelBorder());	// TODO: Would be better to do this with the corner points, instead of the bounding box
}

QRectF Template::calculateTemplateBoundingBox()
{
	if (is_georeferenced)
		return infinteRectF();
	
	// Create bounding box by calculating the positions of all corners of the transformed extent rect
	QRectF extent = getTemplateExtent();
	QRectF bbox;
	rectIncludeSafe(bbox, templateToMap(extent.topLeft()).toQPointF());
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
	float radius = qMin(getTemplateScaleX(), getTemplateScaleY()) * qMax((width+1) / 2, 1.0f);
	QRectF radius_bbox = QRectF(map_bbox.left() - radius, map_bbox.top() - radius,
								map_bbox.width() + 2*radius, map_bbox.height() + 2*radius);
	
	drawOntoTemplateImpl(coords, num_coords, color, width);
	map->setTemplateAreaDirty(this, radius_bbox, 0);
	
	setHasUnsavedChanges(true);
}

void Template::addPassPoint(const PassPoint& point, int pos)
{
	assert(!is_georeferenced);
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
	assert(!is_georeferenced);
	setTemplateAreaDirty();
	
	TemplateTransform temp = transform;
	transform = other_transform;
	other_transform = temp;
	template_to_map_other = template_to_map;
	updateTransformationMatrices();
	
	adjusted = !adjusted;
	setTemplateAreaDirty();
	map->setTemplatesDirty();
}
void Template::getTransform(TemplateTransform& out)
{
	assert(!is_georeferenced);
	out = transform;
}
void Template::setTransform(const TemplateTransform& transform)
{
	assert(!is_georeferenced);
	setTemplateAreaDirty();
	
	this->transform = transform;
	updateTransformationMatrices();
	
	setTemplateAreaDirty();
	map->setTemplatesDirty();
}
void Template::getOtherTransform(TemplateTransform& out)
{
	assert(!is_georeferenced);
	out = other_transform;
}
void Template::setOtherTransform(const TemplateTransform& transform)
{
	assert(!is_georeferenced);
	other_transform = transform;
}

void Template::setTemplatePath(const QString& value)
{
	template_path = value;
	template_file = QFileInfo(value).fileName();
}

void Template::setHasUnsavedChanges(bool value)
{
	has_unsaved_changes = value;
	if (value)
		map->setTemplatesDirty();
}

void Template::setAdjustmentDirty(bool value)
{
	adjustment_dirty = value;
	if (value)
		map->setTemplatesDirty();
}

Template* Template::templateForFile(const QString& path, Map* map)
{
	if (path.endsWith(".png", Qt::CaseInsensitive) || path.endsWith(".bmp", Qt::CaseInsensitive) ||
		path.endsWith(".jpg", Qt::CaseInsensitive) || path.endsWith(".gif", Qt::CaseInsensitive) ||
		path.endsWith(".jpeg", Qt::CaseInsensitive) || path.endsWith(".tif", Qt::CaseInsensitive) ||
		path.endsWith(".tiff", Qt::CaseInsensitive))
		return new TemplateImage(path, map);
	else if (path.endsWith(".ocd", Qt::CaseInsensitive) || path.endsWith(".omap", Qt::CaseInsensitive))
		return new TemplateMap(path, map);
	else if (path.endsWith(".gpx", Qt::CaseInsensitive) || path.endsWith(".dxf", Qt::CaseInsensitive) ||
			 path.endsWith(".osm", Qt::CaseInsensitive))
		return new TemplateTrack(path, map);
	else
		return NULL;
}

void Template::updateTransformationMatrices()
{
	double cosr = cos(-transform.template_rotation);
	double sinr = sin(-transform.template_rotation);
	double scale_x = getTemplateScaleX();
	double scale_y = getTemplateScaleY();
	
	template_to_map.setSize(3, 3);
	template_to_map.set(0, 0, scale_x * cosr);
	template_to_map.set(0, 1, scale_y * (-sinr));
	template_to_map.set(1, 0, scale_x * sinr);
	template_to_map.set(1, 1, scale_y * cosr);
	template_to_map.set(0, 2, transform.template_x / 1000.0);
	template_to_map.set(1, 2, transform.template_y / 1000.0);
	template_to_map.set(2, 0, 0);
	template_to_map.set(2, 1, 0);
	template_to_map.set(2, 2, 1);
	
	template_to_map.invert(map_to_template);
}
