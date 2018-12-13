/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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

#include <iterator>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QAbstractButton>
#include <QByteArray>
#include <QDebug>
#include <QFileInfo>  // IWYU pragma: keep
#include <QFlags>
#include <QHBoxLayout>
#include <QIcon>
#include <QImageReader>
#include <QIntValidator>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPaintEngine>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPushButton>
#include <QRadioButton>
#include <QRect>
#include <QSize>
#include <QStringRef>
#include <QTransform>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/storage_location.h"  // IWYU pragma: keep
#include "gui/georeferencing_dialog.h"
#include "gui/select_crs_dialog.h"
#include "gui/util_gui.h"
#ifdef QT_PRINTSUPPORT_LIB
#include "printsupport/advanced_pdf_printer.h"
#endif
#include "templates/world_file.h"
#include "util/transformation.h"
#include "util/util.h"

// IWYU pragma: no_forward_declare QHBoxLayout
// IWYU pragma: no_forward_declare QVBoxLayout


namespace OpenOrienteering {

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

TemplateImage::TemplateImage(const QString& path, Map* map) : Template(path, map)
{
	undo_index = 0;
	georef.reset(new Georeferencing());
	
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateImage::updateGeoreferencing);
	connect(&georef, &Georeferencing::transformationChanged, this, &TemplateImage::updateGeoreferencing);
}
TemplateImage::~TemplateImage()
{
	if (template_state == Loaded)
		unloadTemplateFile();
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


#ifndef NO_NATIVE_FILE_FORMAT
bool TemplateImage::loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version)
{
	Q_UNUSED(version);
	
	if (is_georeferenced)
	{
		loadString(stream, temp_crs_spec);
	}
	
	return true;
}
#endif


void TemplateImage::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	if (is_georeferenced)
	{
		// Follow map georeferencing XML structure
		xml.writeStartElement(QString::fromLatin1("crs_spec"));
// TODO: xml.writeAttribute(QString::fromLatin1("language"), "PROJ.4");
		xml.writeCharacters(temp_crs_spec);
		xml.writeEndElement(/*crs_spec*/);
	}
}

bool TemplateImage::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	if (is_georeferenced && xml.name() == QLatin1String("crs_spec"))
	{
		// TODO: check specification language
		temp_crs_spec = xml.readElementText();
	}
	else
		xml.skipCurrentElement(); // unsupported
	
	return true;
}

bool TemplateImage::loadTemplateFileImpl(bool configuring)
{
	QImageReader reader(template_path);
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
	
	// Check if georeferencing information is available
	available_georef = Georeferencing_None;
	
	// TODO: GeoTIFF
	
	WorldFile world_file;
	if (available_georef == Georeferencing_None && world_file.tryToLoadForImage(template_path))
		available_georef = Georeferencing_WorldFile;
	
	if (!configuring && is_georeferenced)
	{
		if (available_georef == Georeferencing_None)
		{
			// Image was georeferenced, but georeferencing info is gone -> deny to load template
			return false;
		}
		else
			calculateGeoreferencing();
	}
	
	return true;
}
bool TemplateImage::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	Q_UNUSED(out_center_in_view);
	
	if (getTemplateFilename().endsWith(QLatin1String(".gif"), Qt::CaseInsensitive))
		QMessageBox::warning(dialog_parent, tr("Warning"), tr("Loading a GIF image template.\nSaving GIF files is not supported. This means that drawings on this template won't be saved!\nIf you do not intend to draw on this template however, that is no problem."));
	
	TemplateImageOpenDialog open_dialog(this, dialog_parent);
	open_dialog.setWindowModality(Qt::WindowModal);
	while (true)
	{
		if (open_dialog.exec() == QDialog::Rejected)
			return false;
		
		if (open_dialog.isGeorefRadioChecked() && map->getGeoreferencing().isLocal())
		{
			// Make sure that the map is georeferenced;
			// use the center coordinates of the image as initial reference point.
			calculateGeoreferencing();
			QPointF template_coords_center = georef->toProjectedCoords(MapCoordF(0.5 * (image.width() - 1), 0.5 * (image.height() - 1)));
			bool template_coords_probably_geographic =
				template_coords_center.x() >= -90 && template_coords_center.x() <= 90 &&
				template_coords_center.y() >= -90 && template_coords_center.y() <= 90;
			
			Georeferencing initial_georef(map->getGeoreferencing());
			if (template_coords_probably_geographic)
				initial_georef.setGeographicRefPoint(LatLon(template_coords_center.y(), template_coords_center.x()));
			else
				initial_georef.setProjectedRefPoint(template_coords_center);
			initial_georef.setState(Georeferencing::Local);
			
			GeoreferencingDialog dialog(dialog_parent, map, &initial_georef, false);
			if (template_coords_probably_geographic)
				dialog.setKeepGeographicRefCoords();
			else
				dialog.setKeepProjectedRefCoords();
			if (dialog.exec() == QDialog::Rejected)
				continue;
		}
		
		if (open_dialog.isGeorefRadioChecked() && available_georef == Georeferencing_WorldFile)
		{
			// Let user select the coordinate reference system, as this is not specified in world files
			SelectCRSDialog dialog(
			            map->getGeoreferencing(),
			            dialog_parent,
			            SelectCRSDialog::TakeFromMap | SelectCRSDialog::Geographic,
			            tr("Select the coordinate reference system of the coordinates in the world file") );
			if (dialog.exec() == QDialog::Rejected)
				continue;
			temp_crs_spec = dialog.currentCRSSpec();
		}
		
		break;
	}
	
	is_georeferenced = open_dialog.isGeorefRadioChecked();	
	if (is_georeferenced)
		calculateGeoreferencing();
	else
	{
		transform.template_scale_x = open_dialog.getMpp() * 1000.0 / map->getScaleDenominator();
		transform.template_scale_y = transform.template_scale_x;
		
		//transform.template_x = main_view->getPositionX();
		//transform.template_y = main_view->getPositionY();
		
		updateTransformationMatrices();
	}
	
	return true;
}

void TemplateImage::unloadTemplateFileImpl()
{
	image = QImage();
}

void TemplateImage::drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const
{
	Q_UNUSED(clip_rect);
	Q_UNUSED(scale);
	Q_UNUSED(on_screen);
	
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

void TemplateImage::updateGeoreferencing()
{
	if (is_georeferenced && template_state == Template::Loaded)
		updatePosFromGeoreferencing();
}

Template* TemplateImage::duplicateImpl() const
{
	auto new_template = new TemplateImage(template_path, map);
	new_template->image = image;
	new_template->available_georef = available_georef;
	return new_template;
}

void TemplateImage::drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width)
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
	for (int i = 0; i < draw_iterations; ++ i)
		painter.drawPolyline(points, num_coords);
	
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
	// Calculate georeferencing of image coordinates where the coordinate (0, 0)
	// is mapped to the world position of the middle of the top-left pixel
	georef.reset(new Georeferencing());
	if (!temp_crs_spec.isEmpty())
		georef->setProjectedCRS(QString{}, temp_crs_spec);
	
	if (available_georef == Georeferencing_WorldFile)
	{
		WorldFile world_file;
		if (!world_file.tryToLoadForImage(template_path))
		{
			// TODO: world file lost, disable georeferencing or unload template
			return;
		}
		auto pixel_to_world = world_file.pixel_to_world;
		if (georef->isGeographic())
		{
			constexpr auto factor = qDegreesToRadians(1.0);
			pixel_to_world = {
			    pixel_to_world.m11() * factor, pixel_to_world.m12() * factor, 0,
			    pixel_to_world.m21() * factor, pixel_to_world.m22() * factor, 0,
			    pixel_to_world.m31() * factor, pixel_to_world.m32() * factor, 1
			};
		}
		georef->setTransformationDirectly(pixel_to_world);
	}
	else if (available_georef == Georeferencing_GeoTiff)
	{
		// TODO: GeoTIFF
	}
	
	if (map->getGeoreferencing().isValid())
		updatePosFromGeoreferencing();
}

void TemplateImage::updatePosFromGeoreferencing()
{
	// Determine map coords of three image corner points
	// by transforming the points from one Georeferencing into the other
	bool ok;
	MapCoordF top_left = map->getGeoreferencing().toMapCoordF(georef.data(), MapCoordF(-0.5, -0.5), &ok);
	if (!ok)
	{
		qDebug() << "updatePosFromGeoreferencing() failed";
		return; // TODO: proper error message?
	}
	MapCoordF top_right = map->getGeoreferencing().toMapCoordF(georef.data(), MapCoordF(image.width() - 0.5, -0.5), &ok);
	if (!ok)
	{
		qDebug() << "updatePosFromGeoreferencing() failed";
		return; // TODO: proper error message?
	}
	MapCoordF bottom_left = map->getGeoreferencing().toMapCoordF(georef.data(), MapCoordF(-0.5, image.height() - 0.5), &ok);
	if (!ok)
	{
		qDebug() << "updatePosFromGeoreferencing() failed";
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
		qDebug() << "updatePosFromGeoreferencing() failed";
		return; // TODO: proper error message?
	}
	transform = TemplateTransform::fromQTransform(q_transform);
	updateTransformationMatrices();
}


// ### TemplateImageOpenDialog ###

TemplateImageOpenDialog::TemplateImageOpenDialog(TemplateImage* templ, QWidget* parent)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), templ(templ)
{
	setWindowTitle(tr("Opening %1").arg(templ->getTemplateFilename()));
	
	QLabel* size_label = new QLabel(QLatin1String("<b>") + tr("Image size:") + QLatin1String("</b> ")
	                                + QString::number(templ->getImage().width()) + QLatin1String(" x ")
	                                + QString::number(templ->getImage().height()));
	QLabel* desc_label = new QLabel(tr("Specify how to position or scale the image:"));
	
	bool use_meters_per_pixel;
	double meters_per_pixel;
	double dpi;
	double scale;
	templ->getMap()->getImageTemplateDefaults(use_meters_per_pixel, meters_per_pixel, dpi, scale);
	
	QString georef_type_string;
	if (templ->getAvailableGeoreferencing() == TemplateImage::Georeferencing_WorldFile)
		georef_type_string = tr("World file");
	else if (templ->getAvailableGeoreferencing() == TemplateImage::Georeferencing_GeoTiff)
		georef_type_string = tr("GeoTIFF");
	else if (templ->getAvailableGeoreferencing() == TemplateImage::Georeferencing_None)
		georef_type_string = tr("no georeferencing information");
	
	georef_radio = new QRadioButton(tr("Georeferenced") +
		(georef_type_string.isEmpty() ? QString{} : (QLatin1String(" (") + georef_type_string + QLatin1String(")"))));
	georef_radio->setEnabled(templ->getAvailableGeoreferencing() != TemplateImage::Georeferencing_None);
	
	mpp_radio = new QRadioButton(tr("Meters per pixel:"));
	mpp_edit = new QLineEdit((meters_per_pixel > 0) ? QString::number(meters_per_pixel) : QString{});
	mpp_edit->setValidator(new DoubleValidator(0, 999999, mpp_edit));
	
	dpi_radio = new QRadioButton(tr("Scanned with"));
	dpi_edit = new QLineEdit((dpi > 0) ? QString::number(dpi) : QString{});
	dpi_edit->setValidator(new DoubleValidator(1, 999999, dpi_edit));
	QLabel* dpi_label = new QLabel(tr("dpi"));
	
	QLabel* scale_label = new QLabel(tr("Template scale:  1 :"));
	scale_edit = new QLineEdit((scale > 0) ? QString::number(scale) : QString{});
	scale_edit->setValidator(new QIntValidator(1, 999999, scale_edit));
	
	if (georef_radio->isEnabled())
		georef_radio->setChecked(true);
	else if (use_meters_per_pixel)
		mpp_radio->setChecked(true);
	else
		dpi_radio->setChecked(true);
	
	auto mpp_layout = new QHBoxLayout();
	mpp_layout->addWidget(mpp_radio);
	mpp_layout->addWidget(mpp_edit);
	mpp_layout->addStretch(1);
	auto dpi_layout = new QHBoxLayout();
	dpi_layout->addWidget(dpi_radio);
	dpi_layout->addWidget(dpi_edit);
	dpi_layout->addWidget(dpi_label);
	dpi_layout->addStretch(1);
	auto scale_layout = new QHBoxLayout();
	scale_layout->addSpacing(16);
	scale_layout->addWidget(scale_label);
	scale_layout->addWidget(scale_edit);
	scale_layout->addStretch(1);
	
	auto cancel_button = new QPushButton(tr("Cancel"));
	open_button = new QPushButton(QIcon(QString::fromLatin1(":/images/arrow-right.png")), tr("Open"));
	open_button->setDefault(true);
	
	auto buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(open_button);
	
	auto layout = new QVBoxLayout();
	layout->addWidget(size_label);
	layout->addSpacing(16);
	layout->addWidget(desc_label);
	layout->addWidget(georef_radio);
	layout->addLayout(mpp_layout);
	layout->addLayout(dpi_layout);
	layout->addLayout(scale_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(mpp_edit, &QLineEdit::textEdited, this, &TemplateImageOpenDialog::setOpenEnabled);
	connect(dpi_edit, &QLineEdit::textEdited, this, &TemplateImageOpenDialog::setOpenEnabled);
	connect(scale_edit, &QLineEdit::textEdited, this, &TemplateImageOpenDialog::setOpenEnabled);
	connect(cancel_button, &QAbstractButton::clicked, this, &QDialog::reject);
	connect(open_button, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::doAccept);
	connect(georef_radio, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::radioClicked);
	connect(mpp_radio, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::radioClicked);
	connect(dpi_radio, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::radioClicked);
	
	radioClicked();
}
double TemplateImageOpenDialog::getMpp() const
{
	if (mpp_radio->isChecked())
		return mpp_edit->text().toDouble();
	else
	{
		double dpi = dpi_edit->text().toDouble();			// dots/pixels per inch(on map)
		double ipd = 1.0 / dpi;								// inch(on map) per pixel
		double mpd = ipd * 0.0254;							// meters(on map) per pixel	
		double mpp = mpd * scale_edit->text().toDouble();	// meters(in reality) per pixel
		return mpp;
	}
}
bool TemplateImageOpenDialog::isGeorefRadioChecked() const
{
	return georef_radio->isChecked();
}
void TemplateImageOpenDialog::radioClicked()
{
	bool mpp_checked = mpp_radio->isChecked();
	bool dpi_checked = dpi_radio->isChecked();
	dpi_edit->setEnabled(dpi_checked);
	scale_edit->setEnabled(dpi_checked);
	mpp_edit->setEnabled(mpp_checked);
	setOpenEnabled();
}
void TemplateImageOpenDialog::setOpenEnabled()
{
	if (mpp_radio->isChecked())
		open_button->setEnabled(!mpp_edit->text().isEmpty());
	else if (dpi_radio->isChecked())
		open_button->setEnabled(!scale_edit->text().isEmpty() && !dpi_edit->text().isEmpty());
	else //if (georef_radio->isChecked())
		open_button->setEnabled(true);
}
void TemplateImageOpenDialog::doAccept()
{
	templ->getMap()->setImageTemplateDefaults(mpp_radio->isChecked(), mpp_edit->text().toDouble(), dpi_edit->text().toDouble(), scale_edit->text().toDouble());
	accept();
}


}  // namespace OpenOrienteering
