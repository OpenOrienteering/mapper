/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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


#include "template_image.h"

#include <iosfwd>
#include <iterator>
#include <utility>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QBrush>
#include <QByteArray>
#include <QDialog>
#include <QFileInfo>  // IWYU pragma: keep
#include <QImageReader>
#include <QImageWriter>
#include <QLatin1String>
#include <QList>
#include <QPaintEngine>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QStringRef>
#include <QTransform>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/storage_location.h"  // IWYU pragma: keep
#include "gui/georeferencing_dialog.h"
#include "gui/select_crs_dialog.h"
#ifdef QT_PRINTSUPPORT_LIB
#include "printsupport/advanced_pdf_printer.h"
#endif
#include "templates/template_image_open_dialog.h"
#include "templates/world_file.h"
#include "util/transformation.h"
#include "util/util.h"


namespace OpenOrienteering {

#ifdef MAPPER_USE_GDAL

// Forward declaration, from "gdal/gdal_image_reader.h",
// to avoid a direct dependency on GDAL API includes.
TemplateImage::GeoreferencingOption readGdalGeoTransform(const QString& filepath);

#endif


const std::vector<QByteArray>& TemplateImage::supportedExtensions()
{
	static std::vector<QByteArray> extensions;
	if (extensions.empty())
	{
		using namespace std;
		auto formats = QImageReader::supportedImageFormats();
		extensions.reserve(formats.size());
		extensions.insert(end(extensions), begin(formats), end(formats));
	}
	return extensions;
}

TemplateImage::TemplateImage(const QString& path, Map* map)
: Template(path, map)
, georef(std::make_unique<Georeferencing>())
{
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateImage::updateGeoreferencing);
	connect(&georef, &Georeferencing::transformationChanged, this, &TemplateImage::updateGeoreferencing);
}

TemplateImage::TemplateImage(const TemplateImage& proto)
: Template(proto)
, image(proto.image)
// not copied: undo_steps
// not copied: undo_index
, available_georef(proto.available_georef)
, georef(new Georeferencing(*proto.georef))
{
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateImage::updateGeoreferencing);
	connect(&georef, &Georeferencing::transformationChanged, this, &TemplateImage::updateGeoreferencing);
}

TemplateImage::~TemplateImage()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}


TemplateImage* TemplateImage::duplicate() const
{
	return new TemplateImage(*this);
}


bool TemplateImage::saveTemplateFile() const
{
	const auto result = image.save(template_path);
#ifdef Q_OS_ANDROID
	// Make the MediaScanner aware of the *updated* file.
	Android::mediaScannerScanFile(QFileInfo(template_path).absolutePath());
#endif
	return result;
}



void TemplateImage::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	if (is_georeferenced)
	{
		// Follow map georeferencing XML structure
		xml.writeStartElement(QString::fromLatin1("crs_spec"));
		/// \todo xml.writeAttribute(QString::fromLatin1("language"), "PROJ.4");
		xml.writeCharacters(available_georef.effective.crs_spec);
		xml.writeEndElement(/*crs_spec*/);
	}
}

bool TemplateImage::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	if (is_georeferenced && xml.name() == QLatin1String("crs_spec"))
	{
		/// \todo check specification language
		available_georef.effective.crs_spec = xml.readElementText();
	}
	else
		xml.skipCurrentElement(); // unsupported
	
	return true;
}

bool TemplateImage::loadTemplateFileImpl(bool configuring)
{
	QImageReader reader(template_path);
	
	// QImageReader::format() cannot be called after reading.
	drawable = QImageWriter::supportedImageFormats().contains(reader.format());
	
	const QSize size = reader.size();
	const QImage::Format format = reader.imageFormat();
	if (size.isEmpty() || format == QImage::Format_Invalid)
	{
		// Leave memory allocation to QImageReader
		image = reader.read();
	}
	else
	{
		// Pre-allocate the memory in order to catch errors
		image = QImage(size, format);
		if (image.isNull())
		{
			setErrorString(tr("Not enough free memory (image size: %1x%2 pixels)").arg(size.width()).arg(size.height()));
			return false;
		}
		// Read into pre-allocated image
		reader.read(&image);
	}
	
	if (image.isNull())
	{
		setErrorString(reader.errorString());
		return false;
	}
	
#ifdef MAPPER_USE_GDAL
	available_georef = findAvailableGeoreferencing(readGdalGeoTransform(template_path));
#else
	available_georef = findAvailableGeoreferencing({});
#endif
	
	if (!configuring && is_georeferenced)
	{
		if (!isGeoreferencingUsable())
		{
			// Image was georeferenced, but georeferencing info is gone -> deny to load template
			setErrorString(tr("Georeferencing not found"));
			return false;
		}
		
		calculateGeoreferencing();
	}
	
	return true;
}

bool TemplateImage::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	TemplateImageOpenDialog open_dialog(this, dialog_parent);
	open_dialog.setWindowModality(Qt::WindowModal);
	while (true)
	{
		if (open_dialog.exec() == QDialog::Rejected)
			return false;
		
		if (!open_dialog.isGeorefRadioChecked())
			break;
		
		if (map->getGeoreferencing().isLocal())
		{
			// Make sure that the map is georeferenced.
			Georeferencing initial_georef(map->getGeoreferencing());
			initial_georef.setProjectedCRS({}, available_georef.effective.crs_spec);
			if (initial_georef.getMapRefPoint() == MapCoord{}
			    && initial_georef.getProjectedRefPoint() == QPointF{})
			{
				// Use the center coordinates of the image as initial reference point.
				calculateGeoreferencing();
				auto const center_pixel = MapCoordF(0.5 * (image.width() - 1), 0.5 * (image.height() - 1));
				initial_georef.setProjectedRefPoint(georef->toProjectedCoords(center_pixel));
				initial_georef.setCombinedScaleFactor(1.0);
				initial_georef.setGrivation(0.0);
			}
			
			GeoreferencingDialog dialog(dialog_parent, map, &initial_georef, false);
			if (initial_georef.isLocal())
				dialog.setKeepProjectedRefCoords();
			else
				dialog.setKeepGeographicRefCoords();
			if (dialog.exec() == QDialog::Rejected)
				continue;
			
			if (map->getGeoreferencing().getProjectedCRSSpec() == available_georef.effective.crs_spec)
			{
				// For convenience, skip CRS selection.
				break;
			}
		}
		
		if (!map->getGeoreferencing().isLocal())
		{
			// Let user select the coordinate reference system.
			// \todo Change description text below (no longer just for world files.)
			Q_UNUSED(QT_TR_NOOP("Select the coordinate reference system of the georeferenced image."))
			SelectCRSDialog dialog(
			            available_georef,
			            map->getGeoreferencing(),
			            dialog_parent,
			            tr("Select the coordinate reference system of the coordinates in the world file") );
			if (dialog.exec() == QDialog::Accepted)
			{
				available_georef.effective.crs_spec = dialog.currentCRSSpec();
				break;
			}
		}
	}
	
	if (open_dialog.isGeorefRadioChecked())
	{
		calculateGeoreferencing();
		// If not both the template and the map are fully georeferenced,
		// we use the transform, but don't claim the georeferenced state.
		is_georeferenced = georef->isValid()
		                   && !georef->isLocal()
		                   && map->getGeoreferencing().isValid()
		                   && !map->getGeoreferencing().isLocal();
		out_center_in_view = false;
	}
	else
	{
		is_georeferenced = false;
		transform.template_scale_x = open_dialog.getMpp() * 1000.0 / map->getScaleDenominator();
		transform.template_scale_y = transform.template_scale_x;
		transform.template_shear = 0.0;
		updateTransformationMatrices();
	}
	
	return true;
}

void TemplateImage::unloadTemplateFileImpl()
{
	image = QImage();
}

void TemplateImage::drawTemplate(QPainter* painter, const QRectF& /*clip_rect*/, double /*scale*/, bool /*on_screen*/, qreal opacity) const
{
	applyTemplateTransform(painter);
	
	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->setOpacity(opacity);
#ifdef QT_PRINTSUPPORT_LIB
	// QTBUG-70752: QPdfEngine fails to properly apply constant opacity on
	// images. This can be worked around by setting a real brush.
	// Fixed in Qt 5.12.0.
	/// \todo Fix image opacity in AdvancedPdfEngine
#if QT_VERSION < 0x051200
	if (painter->paintEngine()->type() == QPaintEngine::Pdf
	    || painter->paintEngine()->type() == AdvancedPdfPrinter::paintEngineType())
#else
	if (painter->paintEngine()->type() == AdvancedPdfPrinter::paintEngineType())
#endif
	{
		if (opacity < 1)
			painter->setBrush(Qt::white);
	}
#endif
	painter->drawImage(QPointF(-image.width() * 0.5, -image.height() * 0.5), image);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
}
QRectF TemplateImage::getTemplateExtent() const
{
    // If the image is invalid, the extent is an empty rectangle.
    if (image.isNull())
		return QRectF();
	return QRectF(-image.width() * 0.5, -image.height() * 0.5, image.width(), image.height());
}

QPointF TemplateImage::calcCenterOfGravity(QRgb background_color)
{
	int num_points = 0;
	QPointF center = QPointF(0, 0);
	int width = image.width();
	int height = image.height();
	
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			QRgb pixel = image.pixel(x, y);
			if (qAlpha(pixel) < 127 || pixel == background_color)
				continue;
			
			center += QPointF(x, y);
			++num_points;
		}
	}
	
	if (num_points > 0)
		center = QPointF(center.x() / num_points, center.y() / num_points);
	center -= QPointF(image.width() * 0.5 - 0.5, image.height() * 0.5 - 0.5);
	
	return center;
}


bool TemplateImage::canChangeTemplateGeoreferenced()
{
	// No need to care for CRS here and now: This is handled by the dialog ATM.
	return !available_georef.effective.transform.source.isEmpty();
}

bool TemplateImage::trySetTemplateGeoreferenced(bool value, QWidget* dialog_parent)
{
	if (canChangeTemplateGeoreferenced()
	    && isTemplateGeoreferenced() != value)
	{
		// Loaded state is implied by canChangeTemplateGeoreferenced().
		Q_ASSERT(getTemplateState() == Template::Loaded);
		
		if (value)
		{
			// Cf. postLoadConfiguration
			// Let user select the coordinate reference system.
			// \todo Change description text below (no longer just for world files.)
			Q_UNUSED(QT_TR_NOOP("Select the coordinate reference system of the georeferenced image."))
			SelectCRSDialog dialog(
			            available_georef,
			            map->getGeoreferencing(),
			            dialog_parent,
			            tr("Select the coordinate reference system of the coordinates in the world file") );
			if (dialog.exec() == QDialog::Rejected)
				return true;  // not failed!
			
			setTemplateAreaDirty();
			available_georef.effective.crs_spec = dialog.currentCRSSpec();
			calculateGeoreferencing();
			// If not both the template and the map are fully georeferenced,
			// we use the transform, but don't claim the georeferenced state.
			is_georeferenced = georef->isValid()
			                   && !georef->isLocal()
			                   && map->getGeoreferencing().isValid()
			                   && !map->getGeoreferencing().isLocal();
			setTemplateAreaDirty();
		}
		else
		{
			is_georeferenced = false;
		}
		map->setTemplatesDirty();
	}
	return isTemplateGeoreferenced() == value;
}

void TemplateImage::updateGeoreferencing()
{
	if (is_georeferenced && template_state == Template::Loaded)
		updatePosFromGeoreferencing();
}


namespace {

TemplateImage::GeoreferencingOption worldFileOption(const QString& template_path)
{
	auto option = TemplateImage::GeoreferencingOption {};
	WorldFile world_file;
	if (world_file.tryToLoadForImage(template_path))
	{
		option.transform.source = "World file";
		option.transform.pixel_to_world = QTransform(world_file);
	}
	return option;
}

}  // namespace


TemplateImage::GeoreferencingOptions TemplateImage::findAvailableGeoreferencing(GeoreferencingOption template_file_option) const
{
	auto options = GeoreferencingOptions {
		{},
		worldFileOption(template_path),
		std::move(template_file_option),
	};
	
	// Try to select the effective CRS from:
	// (1) current effective CRS, (2) template file, (3) map
	if (!available_georef.effective.crs_spec.isEmpty())
	{
		options.effective.crs_spec = available_georef.effective.crs_spec;
	}
	else if (!options.template_file.crs_spec.isEmpty())
	{
		options.effective.crs_spec = options.template_file.crs_spec;
	}
	else if (!map->getGeoreferencing().getProjectedCRSSpec().isEmpty())
	{
		options.effective.crs_spec = map->getGeoreferencing().getProjectedCRSSpec();
	}
	
	// Try to select the effective transform from:
	// (1) world file, (2) template file
	if (!options.world_file.transform.source.isEmpty())
	{
		options.effective.transform = options.world_file.transform;
	}
	else if (!options.template_file.transform.source.isEmpty())
	{
		options.effective.transform = options.template_file.transform;
	}
	
	// Modify transform from geographic CRS
	if (!options.effective.crs_spec.isEmpty() && !options.effective.transform.source.isEmpty())
	{
		Georeferencing tmp_georef;
		tmp_georef.setProjectedCRS(QString{}, options.effective.crs_spec);
		if (tmp_georef.isGeographic())
		{
			constexpr auto factor = qDegreesToRadians(1.0);
			auto& pixel_to_world = options.effective.transform.pixel_to_world;
			pixel_to_world = {
			    pixel_to_world.m11() * factor, pixel_to_world.m12() * factor, 0,
			    pixel_to_world.m21() * factor, pixel_to_world.m22() * factor, 0,
			    pixel_to_world.m31() * factor, pixel_to_world.m32() * factor, 1
			};
		}
	}
	
	return options;
}

bool TemplateImage::isGeoreferencingUsable() const
{
	return !available_georef.effective.transform.source.isEmpty()
	       && (!available_georef.effective.crs_spec.isEmpty()
	           || map->getGeoreferencing().getProjectedCRSSpec().isEmpty());
}


void TemplateImage::drawOntoTemplateImpl(MapCoordF* coords, int num_coords, const QColor& color, qreal width, ScribbleOptions mode)
{
	QPointF* points;
	QRect radius_bbox;
	int draw_iterations = 1;

	bool all_coords_equal = true;
	for (int i = 1; i < num_coords; ++i)
	{
		if (coords[i] != coords[i-1])
		{
			all_coords_equal = false;
			break;
		}
	}
	
	// Special case for points because drawPolyline() draws nothing in this case.
	// drawPoint() is also unsuitable because it aligns the point to the closest pixel.
	// drawEllipse() in the tested Qt version (5.1.1) seems to have a bug with antialiasing here.
	if (all_coords_equal)
	{
		const qreal ring_radius = 0.8;
		const qreal width_factor = 2.0;
		
		draw_iterations = 2;
		width *= width_factor;
		num_coords = 5;
		points = new QPointF[5];
		points[0] = mapToTemplate(coords[0]) + QPointF(image.width() * 0.5f, image.height() * 0.5f);
		points[1] = points[0] + QPointF(ring_radius, 0);
		points[2] = points[0] + QPointF(0, ring_radius);
		points[3] = points[0] + QPointF(-ring_radius, 0);
		points[4] = points[0] + QPointF(0, -ring_radius);
		points[0] = points[4];
		radius_bbox = QRect(
			qFloor(points[3].x() - width - 1), qFloor(points[4].y() - width - 1),
			qCeil(2 * ring_radius + 2*width + 2.5f), qCeil(2 * ring_radius + 2*width + 2.5f)
		);
	}
	else
	{
		points = new QPointF[num_coords];
		QRectF bbox;
		for (int i = 0; i < num_coords; ++i)
		{
			points[i] = mapToTemplate(coords[i]) + QPointF(image.width() * 0.5f, image.height() * 0.5f);
			rectIncludeSafe(bbox, points[i]);
		}
		radius_bbox = QRect(
			qFloor(bbox.left() - width - 1), qFloor(bbox.top() - width - 1),
			qCeil(bbox.width() + 2*width + 2.5f), qCeil(bbox.height() + 2*width + 2.5f)
		);
		radius_bbox = radius_bbox.intersected(QRect(0, 0, image.width(), image.height()));
	}
	
	// Create undo step
	DrawOnImageUndoStep undo_step;
	undo_step.x = radius_bbox.left();
	undo_step.y = radius_bbox.top();
	undo_step.image = image.copy(radius_bbox);
	addUndoStep(undo_step);
	
	// This conversion is to prevent a very strange bug where the behavior of the
	// default QPainter composition mode seems to be incorrect for images which are
	// loaded from a file without alpha and then painted over with the eraser
	if (color.alpha() == 0 && image.format() != QImage::Format_ARGB32_Premultiplied)
		image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	
    QPainter painter;
	painter.begin(&image);
	if (color.alpha() == 0)
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	else
		painter.setOpacity(color.alphaF());

	QPen pen(color);
	pen.setWidthF(width);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing);

	if (mode.testFlag(FilledAreas))
	{
		painter.setBrush(QBrush(color));
		for (int i = 0; i < draw_iterations; ++i)
			painter.drawPolygon(points, num_coords);
	} else {
		for (int i = 0; i < draw_iterations; ++i)
			painter.drawPolyline(points, num_coords);
	}
	
	painter.end();
	delete[] points;
}

void TemplateImage::drawOntoTemplateUndo(bool redo)
{
	int step_index;
	if (!redo)
	{
		step_index = undo_index - 1;
		if (step_index < 0)
			return;
	}
	else
	{
		step_index = undo_index;
		if (step_index >= static_cast<int>(undo_steps.size()))
			return;
	}
	
	DrawOnImageUndoStep& step = undo_steps[step_index];
	QImage undo_image = step.image.copy();
	step.image = image.copy(step.x, step.y, step.image.width(), step.image.height());
	QPainter painter(&image);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawImage(step.x, step.y, undo_image);
	
	undo_index += redo ? 1 : -1;
	
	qreal template_left = step.x - 0.5 * image.width();
	qreal template_top = step.y - 0.5 * image.height();
	QRectF map_bbox;
	rectIncludeSafe(map_bbox, templateToMap(QPointF(template_left, template_top)));
	rectIncludeSafe(map_bbox, templateToMap(QPointF(template_left + step.image.width(), template_top)));
	rectIncludeSafe(map_bbox, templateToMap(QPointF(template_left, template_top + step.image.height())));
	rectIncludeSafe(map_bbox, templateToMap(QPointF(template_left + step.image.width(), template_top + step.image.height())));
	map->setTemplateAreaDirty(this, map_bbox, 0);
	
	setHasUnsavedChanges(true);
}

void TemplateImage::addUndoStep(const TemplateImage::DrawOnImageUndoStep& new_step)
{
	// TODO: undo step limitation based on used memory
	const int max_undo_steps = 5;
	
	while (static_cast<int>(undo_steps.size()) > undo_index)
		undo_steps.pop_back();
	while (static_cast<int>(undo_steps.size()) >= max_undo_steps)
		undo_steps.erase(undo_steps.begin());
	
	undo_steps.push_back(new_step);
	undo_index = static_cast<int>(undo_steps.size());
}

void TemplateImage::calculateGeoreferencing()
{
	if (!isGeoreferencingUsable())
	{
		qWarning("%s must not be called with incomplete georeferencing", Q_FUNC_INFO);
		georef->setTransformationDirectly({});
		return;
	}
	
	// Calculate georeferencing of image coordinates where the coordinate (0, 0)
	// is mapped to the world position of the top-left corner of the top-left pixel
	georef = std::make_unique<Georeferencing>();
	georef->setProjectedCRS(QString{}, available_georef.effective.crs_spec);
	georef->setTransformationDirectly(available_georef.effective.transform.pixel_to_world);
	if (map->getGeoreferencing().isValid())
		updatePosFromGeoreferencing();
}

void TemplateImage::updatePosFromGeoreferencing()
{
	// Determine map coords of three image corner points
	// by transforming the points from one Georeferencing into the other
	bool ok;
	MapCoordF top_left = map->getGeoreferencing().toMapCoordF(georef.get(), MapCoordF(0.0, 0.0), &ok);
	if (!ok)
	{
		qDebug("%s failed", Q_FUNC_INFO);
		return; // TODO: proper error message?
	}
	MapCoordF top_right = map->getGeoreferencing().toMapCoordF(georef.get(), MapCoordF(image.width(), 0.0), &ok);
	if (!ok)
	{
		qDebug("%s failed", Q_FUNC_INFO);
		return; // TODO: proper error message?
	}
	MapCoordF bottom_left = map->getGeoreferencing().toMapCoordF(georef.get(), MapCoordF(0.0, image.height()), &ok);
	if (!ok)
	{
		qDebug("%s failed", Q_FUNC_INFO);
		return; // TODO: proper error message?
	}
	
	// Calculate template transform as similarity transform from pixels to map coordinates
	PassPointList pp_list;
	
	PassPoint pp;
	pp.src_coords = MapCoordF(-0.5 * image.width(), -0.5 * image.height());
	pp.dest_coords = top_left;
	pp_list.push_back(pp);
	pp.src_coords = MapCoordF(0.5 * image.width(), -0.5 * image.height());
	pp.dest_coords = top_right;
	pp_list.push_back(pp);
	pp.src_coords = MapCoordF(-0.5 * image.width(), 0.5 * image.height());
	pp.dest_coords = bottom_left;
	pp_list.push_back(pp);
	
	QTransform q_transform;
	if (!pp_list.estimateNonIsometricSimilarityTransform(&q_transform))
	{
		qDebug("%s failed", Q_FUNC_INFO);
		return; // TODO: proper error message?
	}
	transform = TemplateTransform::fromQTransform(q_transform);
	updateTransformationMatrices();
}


}  // namespace OpenOrienteering
