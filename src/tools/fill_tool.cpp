/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014-2017 Kai Pastor
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


#include "fill_tool.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>

#include <QtGlobal>
#include <QCursor>
#include <QDir>  // IWYU pragma: keep
#include <QFlags>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRgb>
#include <QSize>
#include <QString>

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/path_coord.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"
#include "tools/tool_base.h"
#include "undo/object_undo.h"


// Uncomment this to generate an image file of the rasterized map
//#define FILLTOOL_DEBUG_IMAGE "FillTool.png"


// Make sure that we can use the object ID in place of a qRgb
// directly and in a sufficiently large domain.
Q_STATIC_ASSERT((std::is_same<QRgb, unsigned int>::value));
Q_STATIC_ASSERT(RGB_MASK == 0x00ffffff);


namespace OpenOrienteering {

namespace {

/**
 * Helper structure used to represent a section of a traced path
 * while constructing the fill object.
 */
struct PathSection
{
	PathObject* object;
	PathPartVector::size_type part;
	PathCoord::length_type start_clen;
	PathCoord::length_type end_clen;
};

constexpr auto background = QRgb(0xffffffffu);

}  // namespace



FillTool::FillTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase(QCursor(QPixmap(QString::fromLatin1(":/images/cursor-fill.png")), 11, 11), Other, editor, tool_action)
{
	drawing_symbol = editor->activeSymbol();
	setDrawingSymbol(editor->activeSymbol());
	
	connect(editor, &MapEditorController::activeSymbolChanged, this, &FillTool::setDrawingSymbol);
}

FillTool::~FillTool() = default;



// TODO: create a way for tools to specify which symbols / selections they support and deactivate them automatically if these conditions are not satisfied anymore!
void FillTool::setDrawingSymbol(const Symbol* symbol)
{
	// Avoid using deleted symbol
	if (map()->findSymbolIndex(drawing_symbol) == -1)
		symbol = nullptr;
	
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
	auto widget = editor->getMainWidget();
	QRectF viewport_extent = widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->geometry()));
	int result = fill(viewport_extent);
	if (result == -1 || result == 1)
		return;
	
	// If not successful, try again with rasterizing the whole map part
	QRectF map_part_extent = map()->getCurrentPart()->calculateExtent(true);
	if (viewport_extent.united(map_part_extent) != viewport_extent)
		result = fill(map_part_extent);
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
	constexpr auto extent_area_warning_threshold = qreal(600 * 600); // 60 cm x 60 cm
	
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
	if (image.pixel(clicked_pixel) != background)
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
	for (QPoint free_pixel = clicked_pixel; free_pixel.x() < image.width() - 1; free_pixel += QPoint(1, 0))
	{
		// Check if there is a collision to the right
		QPoint boundary_pixel = free_pixel + QPoint(1, 0);
		if (image.pixel(boundary_pixel) == background)
			continue;
		
		// Found a collision, trace outline of hit object
		// and check whether the outline contains start_pixel
		std::vector<QPoint> boundary;
		int trace_result = traceBoundary(image, free_pixel, boundary_pixel, boundary);
		if (trace_result == -1)
			return 0;
		else if (trace_result == 0)
		{
			// The outline does not contain start_pixel.
			// Jump to the rightmost pixel of the boundary with same y as the start.
			for (const auto& point : boundary)
			{
				if (point.y() == free_pixel.y() && point.x() > free_pixel.x())
					free_pixel = point;
			}
			
			// Skip over the rest of the floating object.
			free_pixel += QPoint(1, 0);
			while (free_pixel.x() < image.width() - 1
				&& image.pixel(free_pixel) != background)
				free_pixel += QPoint(1, 0);
			free_pixel -= QPoint(1, 0);
			continue;
		}
		
		// Don't let the boundary start in the midde of an object
		const auto id = image.pixel(boundary.front());
		auto new_object = std::find_if(begin(boundary), end(boundary), [&image, id](auto pos) {
			return image.pixel(pos) != id;
		});
		std::rotate(begin(boundary), new_object, end(boundary));
		
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
	
	constexpr auto zoom_level = qreal(4);
	
	// Create map view centered on the extent
	MapView view{ map() };
	view.setCenter(MapCoord{ extent.center() });
	view.setZoom(zoom_level);
	
	// Allocate the image
	auto image_size = view.calculateViewBoundingBox(extent).toAlignedRect().size();
	QImage image = QImage(image_size, QImage::Format_RGB32);
	image.fill(background);
	
	// Draw map
	RenderConfig::Options options = RenderConfig::DisableAntialiasing | RenderConfig::ForceMinSize;
	RenderConfig config = { *map(), extent, view.calculateFinalZoomFactor(), options, 1.0 };
	
	QPainter painter;
	painter.begin(&image);
	painter.translate(image_size.width() / 2.0, image_size.height() / 2.0);
	painter.setWorldTransform(view.worldTransform(), true);
	
	auto original_area_hatching = map()->isAreaHatchingEnabled();
	if (original_area_hatching)
	{
		map()->setAreaHatchingEnabled(false);
	}
	
	if (!map()->isBaselineViewEnabled())
	{
		// Temporarily enable baseline view and draw map once.
		map()->setBaselineViewEnabled(true);
		map()->getCurrentPart()->applyOnAllObjects(&Object::forceUpdate);
		drawObjectIDs(map(), &painter, config);
		map()->setBaselineViewEnabled(false);
		map()->getCurrentPart()->applyOnAllObjects(&Object::forceUpdate);
	}
	else if (original_area_hatching)
	{
		map()->getCurrentPart()->applyOnAllObjects(&Object::forceUpdate);
	}
	
	// Draw the map in original mode (but without area hatching)
	drawObjectIDs(map(), &painter, config);
	
	if (original_area_hatching)
	{
		map()->setAreaHatchingEnabled(original_area_hatching);
		map()->getCurrentPart()->applyOnAllObjects(&Object::forceUpdate);
	}
	
	out_transform = painter.combinedTransform();
	painter.end();
	
#ifdef FILLTOOL_DEBUG_IMAGE
	image.save(QDir::temp().absoluteFilePath(QString::fromLatin1(FILLTOOL_DEBUG_IMAGE)));
#endif
	
	return image;
}

void FillTool::drawObjectIDs(Map* map, QPainter* painter, const RenderConfig &config)
{
	Q_STATIC_ASSERT(MapColor::Reserved == -1);
	Q_ASSERT(!map->isAreaHatchingEnabled());
	
	auto part = map->getCurrentPart();
	auto num_objects = qMin(part->getNumObjects(), int(RGB_MASK));
	auto num_colors = map->getNumColors();
	for (auto c = num_colors-1; c >= MapColor::Reserved; --c)
	{
		auto map_color = map->getColor(c);
		for (int o = 0; o < num_objects; ++o)
		{
			auto object = part->getObject(o);
			if (object->getType() != Object::Path)
				continue;
			if (auto symbol = object->getSymbol())
			{
				if (symbol->isHidden())
					continue;
				if (symbol->getType() == Symbol::Area
				    && static_cast<const AreaSymbol*>(symbol)->getColor() != map_color)
					continue;
			}
			
			object->update();
			object->renderables().draw(c, QRgb(o) | ~RGB_MASK, painter, config);
		}
	}
}

int FillTool::traceBoundary(const QImage& image, const QPoint& free_pixel, const QPoint& boundary_pixel, std::vector<QPoint>& out_boundary)
{
	Q_ASSERT(image.pixel(free_pixel) == background);
	Q_ASSERT(image.pixel(boundary_pixel) != background);
	
#ifdef FILLTOOL_DEBUG_IMAGE
	{
		auto debugImage = image.copy();
		debugImage.setPixel(free_pixel, qRgb(255, 0, 0));
		debugImage.save(QDir::temp().absoluteFilePath(QString::fromLatin1(FILLTOOL_DEBUG_IMAGE)));
	}
#endif
	
	out_boundary.clear();
	out_boundary.reserve(4096);
	out_boundary.push_back(boundary_pixel);
	
	// Go along obstructed pixels with a "right(?) hand on the wall" method.
	// Iteration keeps the following variables as state:
	// cur_free_pixel:     current free position next to boundary
	// cur_boundary_pixel: current position on boundary
	auto cur_free_pixel     = free_pixel;
	auto cur_boundary_pixel = boundary_pixel;
	do
	{
		const auto fwd_vector = cur_free_pixel - cur_boundary_pixel;
		const auto right_vector = QPoint(fwd_vector.y(), -fwd_vector.x());
		const auto right_pixel = cur_free_pixel + right_vector;
		if (!image.rect().contains(right_pixel, true))
			return -1;
		
		// Now analysing a 2x2 pixel block:
		// | cur_free_pixel | cur_boundary_pixel |
		// |  right_pixel   |     diag_pixel     |
		const auto diag_pixel = cur_boundary_pixel + right_vector;
		if (image.pixel(right_pixel) == image.pixel(cur_boundary_pixel))
		{
			out_boundary.push_back(right_pixel);
		}
		else if (image.pixel(diag_pixel) != background)
		{
			out_boundary.push_back(diag_pixel);
			if (image.pixel(right_pixel) == background)
				cur_free_pixel = right_pixel;
			else
				out_boundary.push_back(right_pixel);
		}
		else if (image.pixel(right_pixel) != background)
		{
			out_boundary.push_back(right_pixel);
		}
		else
		{
			cur_free_pixel = diag_pixel;
		}
		cur_boundary_pixel = out_boundary.back();
	}
	while (cur_boundary_pixel != boundary_pixel || cur_free_pixel != free_pixel);
	
	bool inside = false;
	auto size = out_boundary.size();
	for (std::size_t i = 0, j = size - 1; i < size; j = i++)
	{
		if ( ((out_boundary[i].y() > free_pixel.y()) != (out_boundary[j].y() > free_pixel.y()))
		     &&	(free_pixel.x() < (out_boundary[j].x() - out_boundary[i].x())
		                           * (free_pixel.y() - out_boundary[i].y())
		                           / float(out_boundary[j].y() - out_boundary[i].y()) + out_boundary[i].x()) )
			inside = !inside;
	}
	return inside ? 1 : 0;
}

bool FillTool::fillBoundary(const QImage& image, const std::vector<QPoint>& boundary, const QTransform& image_to_map)
{
	auto path = new PathObject(drawing_symbol);
	auto append_section = [path](const PathSection& section)
	{
		if (!section.object)
			return;
		
		const auto& part = section.object->parts()[section.part];
		if (section.end_clen == section.start_clen)
		{
			path->addCoordinate(MapCoord(SplitPathCoord::at(section.start_clen, SplitPathCoord::begin(part.path_coords)).pos));
			return;
		}
		
		PathObject part_copy { part };
		if (section.end_clen < section.start_clen)
		{
			part_copy.changePathBounds(0, section.end_clen, section.start_clen);
			part_copy.reverse();
		}
		else
		{
			part_copy.changePathBounds(0, section.start_clen, section.end_clen);
		}
		
		if (path->getCoordinateCount() == 0)
			path->appendPath(&part_copy);
		else
			path->connectPathParts(0, &part_copy, 0, false, false);
	};
	
	auto last_pixel = background; // no object
	const auto pixel_length = PathCoord::length_type((image_to_map.map(QPointF(0, 0)) - image_to_map.map(QPointF(1, 1))).manhattanLength());
	auto threshold = std::numeric_limits<PathCoord::length_type>::max();
	auto section = PathSection{ nullptr, 0, 0, 0 };
	for (const auto& point : boundary)
	{
		auto pixel = image.pixel(point);
		if (pixel == background)
			continue;
		
		MapCoordF map_pos = MapCoordF(image_to_map.map(QPointF(point)));
		PathCoord path_coord;
		float distance_sq;
		
		if (pixel != last_pixel)
		{
			// Change of object
			append_section(section);
			
			section.object = map()->getCurrentPart()->getObject(int(pixel & RGB_MASK))->asPath();
			section.object->calcClosestPointOnPath(map_pos, distance_sq, path_coord);
			section.part = section.object->findPartIndexForIndex(path_coord.index);
			section.start_clen = path_coord.clen;
			section.end_clen = path_coord.clen;
			last_pixel = pixel;
			threshold = section.object->parts()[section.part].length() - 5*pixel_length;
			continue;
		}
		
		section.object->calcClosestPointOnPath(map_pos, distance_sq, path_coord);
		auto part = section.object->findPartIndexForIndex(path_coord.index);
		if (Q_UNLIKELY(part != section.part))
		{
			// Change of path part
			append_section(section);
			
			section.part = part;
			section.start_clen = path_coord.clen;
			section.end_clen = path_coord.clen;
			threshold = section.object->parts()[section.part].length() - 4*pixel_length;
			continue;
		}
		
		if (section.end_clen - path_coord.clen >= threshold)
		{
			// Forward over closing point
			section.end_clen = section.object->parts()[section.part].length();
			append_section(section);
			section.start_clen = 0;
		}
		else if (path_coord.clen - section.end_clen >= threshold)
		{
			// Backward over closing point
			section.end_clen = 0;
			append_section(section);
			section.start_clen = section.object->parts()[section.part].length();
		}
		section.end_clen = path_coord.clen;
	}
	// Final section
	append_section(section);
	
	if (path->getCoordinateCount() < 2)
	{
		delete path;
		return false;
	}
	
	path->closeAllParts();
	
	// Obsolete: The resultung path is as simple as the bounding objects,
	// so better avoid the loss in precision from PathObject::simplify.
	//   const auto simplify_epsilon = 1e-2;
	//   path->simplify(nullptr, simplify_epsilon);
	
	int index = map()->addObject(path);
	map()->clearObjectSelection(false);
	map()->addObjectToSelection(path, true);
	
	auto undo_step = new DeleteObjectsUndoStep(map());
	undo_step->addObject(index);
	map()->push(undo_step);
	
	map()->setObjectsDirty();
	updateDirtyRect();
	
	return true;
}


}  // namespace OpenOrienteering
