/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


#include "tool_fill.h"

#include <limits>

#include <QMessageBox>
#include <QLabel>
#include <QPainter>

#include "map_editor.h"
#include "map_widget.h"
#include "object.h"
#include "tool_helpers.h"
#include "object_undo.h"


FillTool::FillTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase(QCursor(QPixmap(":/images/cursor-fill.png"), 11, 11), Other, editor, tool_button)
{
	drawing_symbol = editor->activeSymbol();
	setDrawingSymbol(editor->activeSymbol());
	
	connect(editor, SIGNAL(activeSymbolChanged(const Symbol*)), this, SLOT(setDrawingSymbol(const Symbol*)));
}

FillTool::~FillTool()
{
	// Nothing, not inlined
}

// TODO: create a way for tools to specify which symbols / selections they support and deactivate them automatically if these conditions are not satisfied anymore!
void FillTool::setDrawingSymbol(const Symbol* symbol)
{
	// Avoid using deleted symbol
	if (map()->findSymbolIndex(drawing_symbol) == -1)
		symbol = NULL;
	
	if (!symbol)
		deactivate();
	else if (symbol->isHidden())
		deactivate();
	else if ((symbol->getType() & (Symbol::Line | Symbol::Area | Symbol::Combined)) == 0)
		switchToDefaultDrawTool(symbol);
	else
		drawing_symbol = symbol;
}

void FillTool::clickPress()
{
	// First try to apply with current viewport only as extent (for speed)
	MapWidget* widget = editor->getMainWidget();
	QRectF viewport_extent = widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->geometry()));
	int result = fill(viewport_extent);
	if (result == -1 || result == 1)
		return;
	
	// If not successful, try again with rasterizing the whole map
	QRectF map_extent = map()->calculateExtent(true, false);
	result = fill(map_extent);
	if (result == -1 || result == 1)
		return;
	
	QMessageBox::warning(
		window(),
		tr("Error"),
		tr("The clicked area is not bounded by lines or areas, cannot fill this area.")
	);
}

int FillTool::fill(const QRectF& extent)
{
	const float extent_area_warning_threshold = 600 * 600; // 60 cm x 60 cm
	
	// Warn if desired extent is large
	if (extent.width() * extent.height() > extent_area_warning_threshold)
	{
		if (QMessageBox::question(
			window(),
			tr("Warning"),
			tr("The map area is large. Use of the fill tool may be very slow. Do you want to use it anyway?"),
			QMessageBox::No | QMessageBox::Yes) == QMessageBox::No)
		{
			return -1;
		}
	}
	
	// Rasterize map into image
	QTransform transform;
	QImage image = rasterizeMap(extent, transform);
	
	// Calculate click position in image and check if it is inside the map area and free
	QPoint clicked_pixel = transform.map(cur_map_widget->viewportToMapF(click_pos)).toPoint();
	if (!image.rect().contains(clicked_pixel, true))
		return 0;
	if (qAlpha(image.pixel(clicked_pixel)) > 0)
	{
		QMessageBox::warning(
			window(),
			tr("Error"),
			tr("The clicked position is not free, cannot use the fill tool there.")
		);
		return -1;
	}
	
	// Go to the right and find collisions with objects.
	// For every collision, trace the boundary of the collision object
	// and check whether the click position is inside the boundary.
	// If it is, the correct outline was found which is then filled.
	for (QPoint start_pixel = clicked_pixel; start_pixel.x() < image.width() - 1; start_pixel += QPoint(1, 0))
	{
		// Check if there is a collision to the right
		QPoint test_pixel = start_pixel + QPoint(1, 0);
		if (qAlpha(image.pixel(test_pixel)) == 0)
			continue;
		
		// Found a collision, trace outline of hit object
		// and check whether the outline contains start_pixel
		std::vector<QPoint> boundary;
		int trace_result = traceBoundary(image, start_pixel, test_pixel, boundary);
		if (trace_result == -1)
			return 0;
		else if (trace_result == 0)
		{
			// The outline does not contain start_pixel.
			// Jump to the rightmost pixel of the boundary with same y as the start.
			for (size_t b = 0, size = boundary.size(); b < size; ++b)
			{
				if (boundary[b].y() == start_pixel.y()
					&& boundary[b].x() > start_pixel.x())
					start_pixel = boundary[b];
			}
			
			// Skip over the rest of the floating object.
			start_pixel += QPoint(1, 0);
			while (start_pixel.x() < image.width() - 1
				&& qAlpha(image.pixel(start_pixel)) > 0)
				start_pixel += QPoint(1, 0);
			start_pixel -= QPoint(1, 0);
			continue;
		}
		
		// Create fill object
		if (!fillBoundary(image, boundary, transform.inverted()))
		{
			QMessageBox::warning(
				window(),
				tr("Error"),
				tr("Failed to create the fill object.")
			);
			return -1;
		}
		return 1;
	}
	return 0;
}

void FillTool::updateStatusText()
{
	setStatusBarText(tr("<b>Click</b>: Fill area with active symbol. The area to be filled must be bounded by lines or areas, other symbols are not taken into account. "));
}

void FillTool::objectSelectionChangedImpl()
{
}

QImage FillTool::rasterizeMap(const QRectF& extent, QTransform& out_transform)
{
	// Draw map into a QImage with the following settings:
	// - specific zoom factor (resolution)
	// - no antialiasing
	// - encode object ids in object colors
	// - draw baselines in advance to normal rendering
	//   This makes it possible to fill areas bounded by e.g. dashed paths.
	
	const float zoom_level = 4;
	
	// Create map view centered on the extent
	MapView* view = new MapView(map());
	view->setCenter(MapCoord{ extent.center() });
	view->setZoom(zoom_level);
	
	// Allocate the image
	QRect image_size = view->calculateViewBoundingBox(extent).toAlignedRect();
	QImage image = QImage(image_size.size(), QImage::Format_ARGB32_Premultiplied);
	
	// Start drawing
	QPainter painter;
	painter.begin(&image);
	
	// Make image transparent
	QPainter::CompositionMode mode = painter.compositionMode();
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(0, 0, image_size.width(), image_size.height(), Qt::transparent);
	painter.setCompositionMode(mode);
	
	// Draw map
	RenderConfig::Options options = RenderConfig::DisableAntialiasing | RenderConfig::ForceMinSize;
	RenderConfig config = { *map(), extent, view->calculateFinalZoomFactor(), options, 1.0 };
	
	painter.translate(image_size.width() / 2.0, image_size.height() / 2.0);
	painter.setWorldTransform(view->worldTransform(), true);
	
	auto original_area_hatching = map()->isAreaHatchingEnabled();
	if (original_area_hatching)
	{
		map()->setAreaHatchingEnabled(false);
	}
	
	if (!map()->isBaselineViewEnabled())
	{
		// Temporarily enable baseline view and draw map once.
		map()->setBaselineViewEnabled(true);
		map()->updateAllObjects();
		drawObjectIDs(map(), &painter, config);
		map()->setBaselineViewEnabled(false);
		map()->updateAllObjects();
	}
	else if (original_area_hatching)
	{
		map()->updateAllObjects();
	}
	
	// Draw the map in original mode (but without area hatching)
	drawObjectIDs(map(), &painter, config);
	
	if (original_area_hatching)
	{
		map()->setAreaHatchingEnabled(original_area_hatching);
		map()->updateAllObjects();
	}
	
	out_transform = painter.combinedTransform();
	painter.end();
	delete view;
	return image;
}

void FillTool::drawObjectIDs(Map* map, QPainter* painter, const RenderConfig &config)
{
	MapPart* part = map->getCurrentPart();
	for (int o = 0, num_objects = part->getNumObjects(); o < num_objects; ++o)
	{
		Object* object = part->getObject(o);
		if (object->getSymbol() && object->getSymbol()->isHidden())
			continue;
		if (object->getType() != Object::Path)
			continue;
		
		object->update();
		object->renderables().draw(
			qRgb(o % 256, (o / 256) % 256, (o / (256 * 256)) % 256),
			painter,
			config
		);
	}
}

int FillTool::traceBoundary(QImage image, QPoint start_pixel, QPoint test_pixel, std::vector< QPoint >& out_boundary)
{
	out_boundary.clear();
	out_boundary.reserve(4096);
	out_boundary.push_back(test_pixel);
	Q_ASSERT(qAlpha(image.pixel(start_pixel)) == 0);
	Q_ASSERT(qAlpha(image.pixel(test_pixel)) > 0);
	
	// Uncomment this and below references to debugImage to generate path visualizations
// 	QImage debugImage = image.copy();
// 	debugImage.setPixel(test_pixel, qRgb(255, 0, 0));
	
	// Go along obstructed pixels with a "right hand on the wall" method.
	// Iteration keeps the following variables as state:
	// cur_pixel: current (obstructed) position
	// fwd_vector: vector from test_pixel to free spot
	QPoint cur_pixel = test_pixel;
	QPoint fwd_vector = start_pixel - test_pixel;
	int max_length = image.width() * image.height();
	for (int i = 0; i < max_length; ++i)
	{
		QPoint right_vector = QPoint(fwd_vector.y(), -fwd_vector.x());
		if (!image.rect().contains(cur_pixel + fwd_vector + right_vector, true))
			return -1;
		if (!image.rect().contains(cur_pixel + right_vector, true))
			return -1;
		
		if (qAlpha(image.pixel(cur_pixel + fwd_vector + right_vector)) > 0)
		{
			cur_pixel = cur_pixel + fwd_vector + right_vector;
			fwd_vector = -1 * right_vector;
		}
		else if (qAlpha(image.pixel(cur_pixel + right_vector)) > 0)
		{
			cur_pixel = cur_pixel + right_vector;
			// fwd_vector stays the same
		}
		else
		{
			// cur_pixel stays the same
			fwd_vector = right_vector;
		}
		
		QPoint cur_free_pixel = cur_pixel + fwd_vector;
		if (cur_pixel == test_pixel && cur_free_pixel == start_pixel)
			break;
		
// 		debugImage.setPixel(cur_pixel, qRgb(0, 0, 255));
		
		if (out_boundary.back() != cur_pixel)
			out_boundary.push_back(cur_pixel);
	}
	
// 	QLabel* debugImageLabel = new QLabel();
// 	debugImageLabel->setPixmap(QPixmap::fromImage(debugImage));
// 	debugImageLabel->show();
// 	debugImage.save("debugImage.png");
	
	bool inside = false;
	int size = (int)out_boundary.size();
	int i, j;
	for (i = 0, j = size - 1; i < size; j = i++)
	{
		if ( ((out_boundary[i].y() > start_pixel.y()) != (out_boundary[j].y() > start_pixel.y())) &&
			(start_pixel.x() < (out_boundary[j].x() - out_boundary[i].x()) *
			(start_pixel.y() - out_boundary[i].y()) / (float)(out_boundary[j].y() - out_boundary[i].y()) + out_boundary[i].x()) )
			inside = !inside;
	}
	return inside ? 1 : 0;
}

bool FillTool::fillBoundary(const QImage& image, const std::vector< QPoint >& boundary, QTransform image_to_map)
{
	// Test of simpler implementation,
	// does not work properly like this (would need fixing of path->simplify() and dilatation of path)
// 	PathObject* path = new PathObject(last_used_symbol);
// 	for (size_t b = 0, end = boundary.size(); b < end; ++b)
// 		path->addCoordinate(MapCoord(image_to_map.map(QPointF(boundary[b]))));
// 	path->closeAllParts();
// 	
// 	path->convertToCurves();
// 	path->simplify();
	
	// Create PathSection vector
	std::vector< PathSection > sections;
	for (size_t b = 0, end = boundary.size(); b < end; ++b)
	{
		QRgb color = image.pixel(boundary[b]);
		if (qAlpha(color) == 0)
			continue;
		int object_index = qRed(color) + 256 * qGreen(color) + (256 * 256) * qBlue(color);
		PathObject* path = map()->getCurrentPart()->getObject(object_index)->asPath();
		
		MapCoordF map_pos = MapCoordF(image_to_map.map(QPointF(boundary[b])));
		float distance_sq;
		PathCoord path_coord;
		path->calcClosestPointOnPath(map_pos, distance_sq, path_coord);
		int part = path->findPartIndexForIndex(path_coord.index);
		
		// Insert snap info into sections vector.
		// Start new section if this is the first section,
		// if the object changed,
		// if the part changed,
		// if the clen advancing direction changes,
		// or if the clen advancement is more than a magic factor times the pixel advancement
		bool start_new_section =
			sections.empty()
			|| sections.back().object != path
			|| sections.back().part != part
			|| (sections.back().end_clen - sections.back().start_clen) * (path_coord.clen - sections.back().end_clen) < 0
			|| qAbs(path_coord.clen - sections.back().end_clen) > 5 * (map_pos.distanceTo(MapCoordF(image_to_map.map(QPointF(boundary[b - 1])))));
		
		if (start_new_section)
		{
			PathSection new_section;
			new_section.object = path;
			new_section.part = part;
			new_section.start_clen = path_coord.clen;
			new_section.end_clen = path_coord.clen;
			sections.push_back(new_section);
		}
		else
		{
			sections.back().end_clen = path_coord.clen;
		}
	}
	
	// Clean up PathSection vector
	const float pixel_length = (image_to_map.map(QPointF(0, 0)) - image_to_map.map(QPointF(1, 0))).manhattanLength();
	for (int s = 0, end = (int)sections.size(); s < end; ++s)
	{
		PathSection& section = sections[s];
		
		// Remove back-and-forth sections
		if (s > 0)
		{
			PathSection& prev_section = sections[s - 1];
			if (section.object == prev_section.object
				&& qAbs(section.start_clen - prev_section.end_clen) < 2 * pixel_length
				&& (section.end_clen - section.start_clen) * (prev_section.end_clen - prev_section.start_clen) < 0)
			{
				if ((section.end_clen > section.start_clen) == (section.end_clen > prev_section.end_clen))
				{
					// section.end_clen is between prev_section.start_clen and prev_section.end_clen.
					// Delete the new section and shrink prev_section.
					prev_section.end_clen = section.end_clen;
					sections.erase(sections.begin() + s);
				}
				else
				{
					// section.end_clen extends over prev_section.start_clen.
					// Delete prev_section and shrink the new section.
					section.start_clen = prev_section.start_clen;
					sections.erase(sections.begin() + (s - 1));
				}
				--end;
				--s;
			}
		}
		
		// Slightly extend sections where the start is equal to the end,
		// otherwise changePathBounds() will give us the whole path later
		const float epsilon = 1e-4f;
		if (section.end_clen == section.start_clen)
			section.end_clen += epsilon;
	}
	
	// Create fill object
	PathObject* path = new PathObject(drawing_symbol);
	for (auto& section : sections)
	{
		const auto& part = section.object->parts().front();
		if (section.start_clen > part.length() || section.end_clen > part.length())
			continue;
		
		PathObject* part_copy = new PathObject { section.object->parts()[section.part] };
		if (section.end_clen < section.start_clen)
		{
			part_copy->changePathBounds(0, section.end_clen, section.start_clen);
			part_copy->reverse();
		}
		else
		{
			part_copy->changePathBounds(0, section.start_clen, section.end_clen);
		}
		
		if (path->getCoordinateCount() == 0)
			path->appendPath(part_copy);
		else
			path->connectPathParts(0, part_copy, 0, false, false);
		
		delete part_copy;
	}
	
	if (path->getCoordinateCount() < 2)
	{
		delete path;
		return false;
	}
	
	path->closeAllParts();
	
	const auto simplify_epsilon = 1e-2;
	path->simplify(NULL, simplify_epsilon);
	
	int index = map()->addObject(path);
	map()->clearObjectSelection(false);
	map()->addObjectToSelection(path, true);
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map());
	undo_step->addObject(index);
	map()->push(undo_step);
	
	map()->setObjectsDirty();
	updateDirtyRect();
	
	return true;
}
