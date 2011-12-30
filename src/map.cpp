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


#include "map.h"

#include <assert.h>

#include <QMessageBox>
#include <QFile>
#include <QPainter>

#include "map_editor.h"
#include "map_widget.h"
#include "template.h"
#include "util.h"

// ### Map::Color ###

void Map::Color::updateFromCMYK()
{
	color.setCmykF(c, m, y, k);
	qreal rr, rg, rb;
	color.getRgbF(&rr, &rg, &rb);
	r = rr;
	g = rg;
	b = rb;
	assert(color.spec() == QColor::Cmyk);
}
void Map::Color::updateFromRGB()
{
	color.setRgbF(r, g, b);
	color.convertTo(QColor::Cmyk);
	qreal rc, rm, ry, rk;
	color.getCmykF(&rc, &rm, &ry, &rk, NULL);
	c = rc;
	m = rm;
	y = ry;
	k = rk;
}

// ### Map ###

Map::Map()
{
	first_front_template = 0;
	
	colors_dirty = false;
	templates_dirty = false;
	unsaved_changes = false;
}
Map::~Map()
{
	int size = colors.size();
	for (int i = 0; i < size; ++i)
		delete colors[i];
	
	size = templates.size();
	for (int i = 0; i < size; ++i)
		delete templates[i];
}

bool Map::saveTo(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\nfor writing.").arg(path));
		return false;
	}
	
	// Basic stuff
	const char FILE_TYPE_ID[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"
	file.write(FILE_TYPE_ID, 4);
	
	const int FILE_VERSION_ID = 0;
	file.write((const char*)&FILE_VERSION_ID, sizeof(int));
	
	// Write colors
	int num_colors = (int)colors.size();
	file.write((const char*)&num_colors, sizeof(int));
	
	for (int i = 0; i < num_colors; ++i)
	{
		Color* color = colors[i];
		
		file.write((const char*)&color->priority, sizeof(int));
		file.write((const char*)&color->c, sizeof(float));
		file.write((const char*)&color->m, sizeof(float));
		file.write((const char*)&color->y, sizeof(float));
		file.write((const char*)&color->k, sizeof(float));
		file.write((const char*)&color->opacity, sizeof(float));
		
		saveString(&file, color->name);
	}
	
	file.close();
	
	colors_dirty = false;
	templates_dirty = false;
	unsaved_changes = false;
	return true;
}
void Map::saveString(QFile* file, const QString& str)
{
	int length = str.length();
	
	file->write((const char*)&length, sizeof(int));
	file->write((const char*)str.constData(), length * sizeof(QChar));
}
void Map::loadString(QFile* file, QString& str)
{
	int length;
	
	file->read((char*)&length, sizeof(int));
	if (length > 0)
	{
		str.resize(length);
		file->read((char*)str.data(), length * sizeof(QChar));
	}
	else
		str = "";
}
bool Map::loadFrom(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\nfor reading.").arg(path));
		return false;
	}
	
	char buffer[256];
	
	// Basic stuff
	const char FILE_TYPE_ID[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"
	file.read(buffer, 4);
	for (int i = 0; i < 4; ++i)
	{
		if (buffer[i] != FILE_TYPE_ID[i])
		{
			QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\n\nInvalid file type.").arg(path));
			return false;
		}
	}
	
	int FILE_VERSION_ID;
	file.read((char*)&FILE_VERSION_ID, sizeof(int));
	if (FILE_VERSION_ID != 0)
		QMessageBox::warning(NULL, tr("Warning"), tr("Problem while opening file:\n%1\n\nUnknown file format version.").arg(path));
	
	// Load colors
	int num_colors;
	file.read((char*)&num_colors, sizeof(int));
	colors.resize(num_colors);
	
	for (int i = 0; i < num_colors; ++i)
	{
		Color* color = new Color();
		
		file.read((char*)&color->priority, sizeof(int));
		file.read((char*)&color->c, sizeof(float));
		file.read((char*)&color->m, sizeof(float));
		file.read((char*)&color->y, sizeof(float));
		file.read((char*)&color->k, sizeof(float));
		file.read((char*)&color->opacity, sizeof(float));
		color->updateFromCMYK();
		
		loadString(&file, color->name);
		
		colors[i] = color;
	}
	
	file.close();
	return true;
}

void Map::addMapWidget(MapWidget* widget)
{
	widgets.push_back(widget);
}
void Map::removeMapWidget(MapWidget* widget)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
	{
		if (widgets[i] == widget)
		{
			widgets.erase(widgets.begin() + i);
			return;
		}
	}
	assert(false);
}
void Map::updateAllMapWidgets()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateEverything();
}

void Map::setDrawingBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setDrawingBoundingBox(map_coords_rect, pixel_border, do_update);
}
void Map::clearDrawingBoundingBox()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->clearDrawingBoundingBox();
}

void Map::setActivityBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setActivityBoundingBox(map_coords_rect, pixel_border, do_update);
}
void Map::clearActivityBoundingBox()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->clearActivityBoundingBox();
}

void Map::updateDrawing(QRectF map_coords_rect, int pixel_border)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateDrawing(map_coords_rect, pixel_border);
}

void Map::setColor(Map::Color* color, int pos)
{
	colors[pos] = color;
	color->priority = pos;
}
Map::Color* Map::addColor(int pos)
{
	Color* new_color = new Color();
	new_color->name = tr("New color");
	new_color->priority = pos;
	new_color->c = 0;
	new_color->m = 0;
	new_color->y = 0;
	new_color->k = 1;
	new_color->updateFromCMYK();
	
	colors.insert(colors.begin() + pos, new_color);
	checkIfFirstColorAdded();
	return new_color;
}
void Map::addColor(Map::Color* color, int pos)
{
	colors.insert(colors.begin() + pos, color);
	checkIfFirstColorAdded();
	color->priority = pos;
}
void Map::deleteColor(int pos)
{
	colors.erase(colors.begin() + pos);
	for (int i = pos; i < (int)colors.size(); ++i)
		colors[i]->priority = i;
	
	if (getNumColors() == 0)
	{
		// That was the last color - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}
void Map::setColorsDirty()
{
	if (!colors_dirty && !unsaved_changes)
	{
		emit(gotUnsavedChanges());
		unsaved_changes = true;
	}
	colors_dirty = true;
}

void Map::setTemplate(Template* temp, int pos)
{
	templates[pos] = temp;
}
void Map::addTemplate(Template* temp, int pos)
{
	templates.insert(templates.begin() + pos, temp);
	checkIfFirstTemplateAdded();
}
void Map::deleteTemplate(int pos)
{
	TemplateVector::iterator it = templates.begin() + pos;
	Template* temp = *it;
	
	// Delete visibility information for the template from all views - get the views indirectly by iterating over all widgets
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->getMapView()->deleteTemplateVisibility(temp);
	
	templates.erase(it);
	
	if (getNumTemplates() == 0)
	{
		// That was the last tempate - the help text in the map widget(s) should maybe be updated (if there are no objects)
		updateAllMapWidgets();
	}
}
void Map::setTemplateAreaDirty(Template* temp, QRectF area)
{
	bool front_cache = false;	// TODO: is there a better way to find out if that is a front or back template?
	int size = (int)templates.size();
	for (int i = 0; i < size; ++i)
	{
		if (templates[i] == temp)
		{
			front_cache = i >= getFirstFrontTemplate();
			break;
		}
	}
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		if (widgets[i]->getMapView()->isTemplateVisible(temp))
			widgets[i]->markTemplateCacheDirty(widgets[i]->getMapView()->calculateViewBoundingBox(area), front_cache);
}
void Map::setTemplateAreaDirty(int i)
{
	if (i == -1)
		return;	// no assert here as convenience, so setTemplateAreaDirty(-1) can be called without effect for the map layer
	assert(i >= 0 && i < (int)templates.size());
	
	templates[i]->setTemplateAreaDirty();
}
void Map::setTemplatesDirty()
{
	if (!templates_dirty && !unsaved_changes)
	{
		emit(gotUnsavedChanges());
		unsaved_changes = true;
	}
	templates_dirty = true;
}

void Map::checkIfFirstColorAdded()
{
	if (getNumColors() == 1)
	{
		// This is the first color - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}
void Map::checkIfFirstTemplateAdded()
{
	if (getNumTemplates() == 1)
	{
		// This is the first template - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}

// ### MapView ###

const double screen_pixel_per_mm = 4.999838577;	// TODO: make configurable (by specifying screen diameter in inch + resolution, or dpi)
// Calculation: pixel_height / ( sqrt((c^2)/(aspect^2 + 1)) * 2.54 )

MapView::MapView(Map* map) : map(map)
{
	zoom = 1;
	rotation = 0;
	position_x = 0;
	position_y = 0;
	view_x = 0;
	view_y = 0;
	update();
}
MapView::~MapView()
{
	foreach (TemplateVisibility* vis, template_visibilities)
		delete vis;
}

void MapView::addMapWidget(MapWidget* widget)
{
	widgets.push_back(widget);
	
	map->addMapWidget(widget);
}
void MapView::removeMapWidget(MapWidget* widget)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
	{
		if (widgets[i] == widget)
		{
			widgets.erase(widgets.begin() + i);
			return;
		}
	}
	assert(false);
	
	map->removeMapWidget(widget);
}
void MapView::updateAllMapWidgets()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateEverything();
}

double MapView::lengthToPixel(qint64 length)
{
	return zoom * screen_pixel_per_mm * (length / 1000.0);
}
qint64 MapView::pixelToLength(double pixel)
{
	return qRound64(1000 * pixel / (zoom * screen_pixel_per_mm));
}

QRectF MapView::calculateViewedRect(QRectF view_rect)
{
	QPointF min = view_rect.topLeft();
	QPointF max = view_rect.bottomRight();
	MapCoordF top_left = viewToMapF(min.x(), min.y());
	MapCoordF top_right = viewToMapF(max.x(), min.y());
	MapCoordF bottom_right = viewToMapF(max.x(), max.y());
	MapCoordF bottom_left = viewToMapF(min.x(), max.y());
	
	QRectF result = QRectF(top_left.getX(), top_left.getY(), 0, 0);
	rectInclude(result, QPointF(top_right.getX(), top_right.getY()));
	rectInclude(result, QPointF(bottom_right.getX(), bottom_right.getY()));
	rectInclude(result, QPointF(bottom_left.getX(), bottom_left.getY()));
	
	return QRectF(result.left() - 0.001, result.top() - 0.001, result.width() + 0.002, result.height() + 0.002);
}
QRectF MapView::calculateViewBoundingBox(QRectF map_rect)
{
	QPointF min = map_rect.topLeft();
	MapCoord map_min = MapCoord(min.x(), min.y());
	QPointF max = map_rect.bottomRight();
	MapCoord map_max = MapCoord(max.x(), max.y());
	
	QPointF top_left;
	mapToView(map_min, top_left.rx(), top_left.ry());
	QPointF bottom_right;
	mapToView(map_max, bottom_right.rx(), bottom_right.ry());
	
	MapCoord map_top_right = MapCoord(max.x(), min.y());
	QPointF top_right;
	mapToView(map_top_right, top_right.rx(), top_right.ry());
	
	MapCoord map_bottom_left = MapCoord(min.x(), max.y());
	QPointF bottom_left;
	mapToView(map_bottom_left, bottom_left.rx(), bottom_left.ry());
	
	QRectF result = QRectF(top_left.x(), top_left.y(), 0, 0);
	rectInclude(result, QPointF(top_right.x(), top_right.y()));
	rectInclude(result, QPointF(bottom_right.x(), bottom_right.y()));
	rectInclude(result, QPointF(bottom_left.x(), bottom_left.y()));
	
	return QRectF(result.left() - 1, result.top() - 1, result.width() + 2, result.height() + 2);
}

void MapView::applyTransform(QPainter* painter)
{
	QTransform world_transform;
	// NOTE: transposing the matrix here ...
	world_transform.setMatrix(map_to_view.get(0, 0), map_to_view.get(1, 0), map_to_view.get(2, 0),
							  map_to_view.get(0, 1), map_to_view.get(1, 1), map_to_view.get(2, 1),
							  map_to_view.get(0, 2), map_to_view.get(1, 2), map_to_view.get(2, 2));
	painter->setWorldTransform(world_transform, true);
}

void MapView::setDragOffset(QPoint offset)
{
	drag_offset = offset;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setDragOffset(drag_offset);
}
void MapView::completeDragging(QPoint offset)
{
	drag_offset = QPoint(0, 0);
	qint64 move_x = -pixelToLength(offset.x());
	qint64 move_y = -pixelToLength(offset.y());
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->completeDragging(offset, move_x, move_y);
	
	position_x += move_x;
	position_y += move_y;
	update();
}

void MapView::setZoom(float value)
{
	float zoom_factor = value / zoom;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->zoom(zoom_factor);
	
	zoom = value;
	update();
}
void MapView::setPositionX(qint64 value)
{
	qint64 offset = value - position_x;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->moveView(offset, 0);
	
	position_x = value;
	update();
}
void MapView::setPositionY(qint64 value)
{
	qint64 offset = value - position_y;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->moveView(0, offset);
	
	position_y = value;
	update();
}

void MapView::update()
{
	double cosr = cos(rotation);
	double sinr = sin(rotation);
	
	double final_zoom = lengthToPixel(1000);
	
	// Create map_to_view
	map_to_view.setSize(3, 3);
	map_to_view.set(0, 0, final_zoom * cosr);
	map_to_view.set(0, 1, final_zoom * (-sinr));
	map_to_view.set(1, 0, final_zoom * sinr);
	map_to_view.set(1, 1, final_zoom * cosr);
	map_to_view.set(0, 2, -final_zoom*(position_x/1000.0)*cosr + final_zoom*(position_y/1000.0)*sinr - view_x);
	map_to_view.set(1, 2, -final_zoom*(position_x/1000.0)*sinr - final_zoom*(position_y/1000.0)*cosr - view_y);
	map_to_view.set(2, 0, 0);
	map_to_view.set(2, 1, 0);
	map_to_view.set(2, 2, 1);
	
	// Create view_to_map
	map_to_view.invert(view_to_map);
}

bool MapView::isTemplateVisible(Template* temp)
{
	if (template_visibilities.contains(temp))
	{
		TemplateVisibility* vis = template_visibilities.value(temp);
		return vis->visible && vis->opacity > 0;
	}
	else
		return false;
}
TemplateVisibility* MapView::getTemplateVisibility(Template* temp)
{
	if (!template_visibilities.contains(temp))
	{
		TemplateVisibility* vis = new TemplateVisibility();
		template_visibilities.insert(temp, vis);
		return vis;
	}
	else
		return template_visibilities.value(temp);
}
void MapView::deleteTemplateVisibility(Template* temp)
{
	delete template_visibilities.value(temp);
	template_visibilities.remove(temp);
}

#include "map.moc"
