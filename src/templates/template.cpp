/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2018 Kai Pastor
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

#include <cmath>
#include <new>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QScopedValueRollback>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map_view.h"
#include "core/map.h"
#include "fileformats/file_format.h"
#include "gdal/ogr_template.h"
#include "gui/file_dialog.h"
#include "templates/template_image.h"
#include "templates/template_map.h"
#include "templates/template_track.h"
#include "util/backports.h"  // IWYU pragma: keep
#include "util/util.h"
#include "util/xml_stream_util.h"


namespace OpenOrienteering {

class Template::ScopedOffsetReversal
{
public:
	ScopedOffsetReversal(Template& temp)
	: temp(temp)
	, copy(temp.transform)
	, needed(temp.accounted_offset != MapCoord{} )
	{
		if (needed)
		{
			// Temporarily revert the offset the saved data is expected to
			// match the original data before bounds checking / correction
			temp.applyTemplatePositionOffset();
		}
	}
	
	~ScopedOffsetReversal()
	{
		if (needed)
		{
			temp.transform = copy;
			temp.updateTransformationMatrices();
		}
	}
	
private:
	Template& temp;
	const TemplateTransform copy;
	const bool needed;
};



// ### TemplateTransform ###

Q_STATIC_ASSERT(std::is_standard_layout<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_default_constructible<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_copy_constructible<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_constructible<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_copy_assignable<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_assignable<TemplateTransform>::value);

// static
TemplateTransform TemplateTransform::fromQTransform(const QTransform& qt) noexcept
{
	const auto scale_x  = std::hypot(qt.m11(), qt.m12());
	const auto scale_y  = std::hypot(qt.m21(), qt.m22());
	const auto rotation = std::atan2(qt.m21() / scale_y, qt.m11() / scale_x);
	return { qRound(1000 * qt.m31()),
	         qRound(1000 * qt.m32()),
	         rotation,
	         scale_x,
	         scale_y };
}



void TemplateTransform::save(QXmlStreamWriter& xml, const QString& role) const
{
	xml.writeStartElement(QString::fromLatin1("transformation"));
	xml.writeAttribute(QString::fromLatin1("role"), role);
	xml.writeAttribute(QString::fromLatin1("x"), QString::number(template_x));
	xml.writeAttribute(QString::fromLatin1("y"), QString::number(template_y));
	xml.writeAttribute(QString::fromLatin1("scale_x"), QString::number(template_scale_x));
	xml.writeAttribute(QString::fromLatin1("scale_y"), QString::number(template_scale_y));
	xml.writeAttribute(QString::fromLatin1("rotation"), QString::number(template_rotation));
	xml.writeEndElement(/*transformation*/);
}

void TemplateTransform::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == QLatin1String("transformation"));
	
	XmlElementReader element { xml };
	auto x64 = element.attribute<qint64>(QLatin1String("x"));
	auto y64 = element.attribute<qint64>(QLatin1String("y"));
	auto coord = MapCoord::fromNative64withOffset(x64, y64);
	template_x = coord.nativeX();
	template_y = coord.nativeY();
	template_scale_x = element.attribute<double>(QLatin1String("scale_x"));
	template_scale_y = element.attribute<double>(QLatin1String("scale_y"));
	template_rotation = element.attribute<double>(QLatin1String("rotation"));
}

bool operator==(const TemplateTransform& lhs, const TemplateTransform& rhs) noexcept
{
	return lhs.template_x == rhs.template_x
	        && (qFuzzyCompare(lhs.template_rotation, rhs.template_rotation)
	            || (qIsNull(lhs.template_rotation) && qIsNull(rhs.template_rotation)))
	        && qFuzzyCompare(lhs.template_scale_x, rhs.template_scale_x)
	        && qFuzzyCompare(lhs.template_scale_y, rhs.template_scale_y);
}

bool operator!=(const TemplateTransform& lhs, const TemplateTransform& rhs) noexcept
{
	return !operator==(lhs, rhs);
}



// ### Template ###

bool Template::suppressAbsolutePaths = false;


Template::Template(const QString& path, not_null<Map*> map)
 : map(map)
 , template_group(0)
{
	template_path = path;
	if (! QFileInfo(path).canonicalFilePath().isEmpty())
		template_path = QFileInfo(path).canonicalFilePath();
	template_file = QFileInfo(path).fileName();
	template_relative_path = QString{};
	template_state = Unloaded;
	
	has_unsaved_changes = false;
	
	is_georeferenced = false;
	
	adjusted = false;
	adjustment_dirty = true;
	
	updateTransformationMatrices();
}

Template::~Template()
{
	Q_ASSERT(template_state != Loaded);
}

Template* Template::duplicate() const
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

QString Template::errorString() const
{
	return error_string;
}

void Template::setErrorString(const QString &text)
{
	error_string = text;
}



void Template::saveTemplateConfiguration(QXmlStreamWriter& xml, bool open, const QDir* map_dir) const
{
	xml.writeStartElement(QString::fromLatin1("template"));
	xml.writeAttribute(QString::fromLatin1("open"), QString::fromLatin1(open ? "true" : "false"));
	xml.writeAttribute(QString::fromLatin1("name"), getTemplateFilename());
	auto primary_path = getTemplatePath();
	auto relative_path = getTemplateRelativePath();
	if (getTemplateState() != Invalid)
	{
		if (map_dir)
			relative_path = map_dir->relativeFilePath(primary_path);
		else if (relative_path.isEmpty())
			relative_path = getTemplateFilename();
	}
	if (suppressAbsolutePaths && QFileInfo(primary_path).isAbsolute())
		primary_path = relative_path;
	xml.writeAttribute(QString::fromLatin1("path"), primary_path);
	xml.writeAttribute(QString::fromLatin1("relpath"), relative_path);
	
	if (template_group)
	{
		xml.writeAttribute(QString::fromLatin1("group"), QString::number(template_group));
	}
	
	if (is_georeferenced)
	{
		xml.writeAttribute(QString::fromLatin1("georef"), QString::fromLatin1("true"));
	}
	else
	{
		ScopedOffsetReversal no_offset{*const_cast<Template*>(this)};
		
		xml.writeStartElement(QString::fromLatin1("transformations"));
		if (adjusted)
			xml.writeAttribute(QString::fromLatin1("adjusted"), QString::fromLatin1("true"));
		if (adjustment_dirty)
			xml.writeAttribute(QString::fromLatin1("adjustment_dirty"), QString::fromLatin1("true"));
		int num_passpoints = (int)passpoints.size();
		xml.writeAttribute(QString::fromLatin1("passpoints"), QString::number(num_passpoints));
		
		transform.save(xml, QString::fromLatin1("active"));
		other_transform.save(xml, QString::fromLatin1("other"));
		
		for (int i = 0; i < num_passpoints; ++i)
			passpoints[i].save(xml);
		
		map_to_template.save(xml, QString::fromLatin1("map_to_template"));
		template_to_map.save(xml, QString::fromLatin1("template_to_map"));
		template_to_map_other.save(xml, QString::fromLatin1("template_to_map_other"));
		
		xml.writeEndElement(/*transformations*/);
	}
	
	saveTypeSpecificTemplateConfiguration(xml);
	
	if (open && hasUnsavedChanges())
	{
		// Save the template itself (e.g. image, gpx file, etc.)
		saveTemplateFile();
		const_cast<Template*>(this)->setHasUnsavedChanges(false); // TODO: Error handling?
	}
	xml.writeEndElement(/*template*/);
}

std::unique_ptr<Template> Template::loadTemplateConfiguration(QXmlStreamReader& xml, Map& map, bool& open)
{
	Q_ASSERT(xml.name() == QLatin1String("template"));
	
	QXmlStreamAttributes attributes = xml.attributes();
	if (attributes.hasAttribute(QLatin1String("open")))
		open = (attributes.value(QLatin1String("open")) == QLatin1String("true"));
	
	QString path = attributes.value(QLatin1String("path")).toString();
	auto temp = templateForFile(path, &map);
	if (!temp)
		temp.reset(new TemplateImage(path, &map)); // fallback
	
	temp->setTemplateRelativePath(attributes.value(QLatin1String("relpath")).toString());
	if (attributes.hasAttribute(QLatin1String("name")))
		temp->template_file = attributes.value(QLatin1String("name")).toString();
	temp->is_georeferenced = (attributes.value(QLatin1String("georef")) == QLatin1String("true"));
	if (attributes.hasAttribute(QLatin1String("group")))
		temp->template_group = attributes.value(QLatin1String("group")).toInt();
		
	while (xml.readNextStartElement())
	{
		if (!temp->is_georeferenced && xml.name() == QLatin1String("transformations"))
		{
			temp->adjusted = (xml.attributes().value(QLatin1String("adjusted")) == QLatin1String("true"));
			temp->adjustment_dirty = (xml.attributes().value(QLatin1String("adjustment_dirty")) == QLatin1String("true"));
			int num_passpoints = xml.attributes().value(QLatin1String("passpoints")).toInt();
Q_ASSERT(temp->passpoints.size() == 0);
			temp->passpoints.reserve(qMin(num_passpoints, 10)); // 10 is not a limit
			
			while (xml.readNextStartElement())
			{
				QStringRef role = xml.attributes().value(QLatin1String("role"));
				if (xml.name() == QLatin1String("transformation"))
				{
					if (role == QLatin1String("active"))
						temp->transform.load(xml);
					else if (xml.attributes().value(QLatin1String("role")) == QLatin1String("other"))
						temp->other_transform.load(xml);
					else
					{
						qDebug() << xml.qualifiedName();
						xml.skipCurrentElement(); // unsupported
					}
				}
				else if (xml.name() == QLatin1String("passpoint"))
				{
					temp->passpoints.push_back(PassPoint::load(xml));
				}
				else if (xml.name() == QLatin1String("matrix"))
				{
					if (role == QLatin1String("map_to_template"))
						temp->map_to_template.load(xml);
					else if (role == QLatin1String("template_to_map"))
						temp->template_to_map.load(xml);
					else if (role == QLatin1String("template_to_map_other"))
						temp->template_to_map_other.load(xml);
					else
					{
						qDebug() << xml.qualifiedName();
						xml.skipCurrentElement(); // unsupported
					}
				}
				else
				{
					qDebug() << xml.qualifiedName();
					xml.skipCurrentElement(); // unsupported
				}
			}
		}
		else if (!temp->loadTypeSpecificTemplateConfiguration(xml))
		{
			temp.reset();
			break;
		}
	}
	
	if (temp && !temp->is_georeferenced)
	{
		// Fix template adjustment after moving objects during import (cf. #513)
		const auto offset = MapCoord::boundsOffset();
		if (!offset.isZero())
		{
			if (temp->template_to_map.getCols() == 3 && temp->template_to_map.getRows() == 3)
			{
				temp->template_to_map.set(0, 2, temp->template_to_map.get(0, 2) - offset.x / 1000.0);
				temp->template_to_map.set(1, 2, temp->template_to_map.get(1, 2) - offset.y / 1000.0);
				temp->template_to_map.invert(temp->map_to_template);
			}
				
			if (temp->template_to_map_other.getCols() == 3 && temp->template_to_map_other.getRows() == 3)
			{
				temp->template_to_map_other.set(0, 2, temp->template_to_map_other.get(0, 2) - offset.x / 1000.0);
				temp->template_to_map_other.set(1, 2, temp->template_to_map_other.get(1, 2) - offset.y / 1000.0);
			}
		}
		
		// Fix template alignment problems caused by grivation rounding since version 0.6
		const double correction = map.getGeoreferencing().getGrivationError();
		if (qAbs(correction) != 0.0
		    && (qstrcmp(temp->getTemplateType(), "TemplateTrack") == 0
		        || qstrcmp(temp->getTemplateType(), "OgrTemplate") == 0) )
		{
			temp->setTemplateRotation(temp->getTemplateRotation() + Georeferencing::degToRad(correction));
		}
	}
	
	return temp;
}

bool Template::saveTemplateFile() const
{
	return false;
}

void Template::switchTemplateFile(const QString& new_path, bool load_file)
{
	if (template_state == Loaded)
	{
		setTemplateAreaDirty();
		unloadTemplateFile();
	}
	
	template_path          = new_path;
	template_file          = QFileInfo(new_path).fileName();
	template_relative_path = QString();
	template_state         = Template::Unloaded;
	
	if (load_file)
		loadTemplateFile(false);
}

bool Template::execSwitchTemplateFileDialog(QWidget* dialog_parent)
{
	QString new_path = FileDialog::getOpenFileName(dialog_parent,
	                                               tr("Find the moved template file"),
	                                               QString(),
	                                               tr("All files (*.*)") );
	new_path = QFileInfo(new_path).canonicalFilePath();
	if (new_path.isEmpty())
		return false;
	
	const State   old_state = getTemplateState();
	const QString old_path  = getTemplatePath();
	
	switchTemplateFile(new_path, true);
	if (getTemplateState() != Loaded)
	{
		QString error_template = QCoreApplication::translate("OpenOrienteering::TemplateListWidget", "Cannot open template\n%1:\n%2").arg(new_path);
		QString error = errorString();
		Q_ASSERT(!error.isEmpty());
		QMessageBox::warning(dialog_parent,
		                     tr("Error"),
		                     error_template.arg(error));
		
		// Revert change
		switchTemplateFile(old_path, old_state == Loaded);
		if (old_state == Invalid)
		{
			template_state = Invalid;
		}
		return false;
	}
	
	return true;
}

bool Template::configureAndLoad(QWidget* dialog_parent, MapView* view)
{
	bool center_in_view = true;
	
	if (!preLoadConfiguration(dialog_parent))
		return false;
	if (!loadTemplateFile(true))
		return false;
	if (!postLoadConfiguration(dialog_parent, center_in_view))
	{
		unloadTemplateFile();
		return false;
	}
	
	// If the template is not georeferenced, position it at the viewport midpoint
	if (!isTemplateGeoreferenced() && center_in_view)
	{
		auto offset = MapCoord { calculateTemplateBoundingBox().center() };
		setTemplatePosition(view->center() - offset);
	}
	
	return true;
}



bool Template::tryToFindTemplateFile(QString map_directory, bool* out_found_in_map_dir)
{
	if (!map_directory.isEmpty() && !map_directory.endsWith(QLatin1Char('/')))
		map_directory.append(QLatin1Char('/'));
	
	if (out_found_in_map_dir)
		*out_found_in_map_dir = false;
	
	const QString old_absolute_path = getTemplatePath();
	
	// First try relative path (if this information is available)
	if (!getTemplateRelativePath().isEmpty() && !map_directory.isEmpty())
	{
		auto path = QString{ map_directory + getTemplateRelativePath() };
		if (QFileInfo::exists(path))
		{
			setTemplatePath(path);
			return true;
		}
	}
	
	// Second try absolute path
	if (QFileInfo::exists(template_path))
	{
		return true;
	}
	
	// Third try the template filename in the map's directory
	if (!map_directory.isEmpty())
	{
		auto path = QString{ map_directory + getTemplateFilename() };
		if (QFileInfo::exists(path))
		{
			setTemplatePath(path);
			if (out_found_in_map_dir)
				*out_found_in_map_dir = true;
			return true;
		}
	}
	
	setTemplatePath(old_absolute_path);
	template_state = Invalid;
	setErrorString(tr("No such file."));
	return false;
}


bool Template::tryToFindAndReloadTemplateFile(QString map_directory, bool* out_loaded_from_map_dir)
{
	return tryToFindTemplateFile(map_directory, out_loaded_from_map_dir)
	       && loadTemplateFile(false);
}

bool Template::preLoadConfiguration(QWidget* dialog_parent)
{
	Q_UNUSED(dialog_parent);
	return true;
}

bool Template::loadTemplateFile(bool configuring)
{
	Q_ASSERT(template_state != Loaded);
	
	const State old_state = template_state;
	
	setErrorString(QString());
	try
	{
		if (!QFileInfo::exists(template_path))
		{
			template_state = Invalid;
			setErrorString(tr("No such file."));
		}
		else if (loadTemplateFileImpl(configuring))
		{
			template_state = Loaded;
		}
		else
		{
			template_state = Invalid;
			if (errorString().isEmpty())
			{
				qDebug("%s: Missing error message from failure in %s::loadTemplateFileImpl(bool)", \
				         Q_FUNC_INFO, \
				         getTemplateType() );
				setErrorString(tr("Is the format of the file correct for this template type?"));
			}
		}
	}
	catch (std::bad_alloc&)
	{
		template_state = Invalid;
		setErrorString(tr("Not enough free memory."));
	}
	catch (FileFormatException& e)
	{
		template_state = Invalid;
		setErrorString(e.message());
	}
	
	if (old_state != template_state)
		emit templateStateChanged();
		
	return template_state == Loaded;
}

bool Template::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	Q_UNUSED(dialog_parent);
	Q_UNUSED(out_center_in_view);
	return true;
}

void Template::unloadTemplateFile()
{
	Q_ASSERT(template_state == Loaded);
	if (hasUnsavedChanges())
	{
		// The changes are lost
		setHasUnsavedChanges(false);
	}
	unloadTemplateFileImpl();
	template_state = Unloaded;
	emit templateStateChanged();
}

void Template::applyTemplateTransform(QPainter* painter) const
{
	painter->translate(transform.template_x / 1000.0, transform.template_y / 1000.0);
	painter->rotate(-transform.template_rotation * (180 / M_PI));
	painter->scale(transform.template_scale_x, transform.template_scale_y);
}

QRectF Template::getTemplateExtent() const
{
	Q_ASSERT(!is_georeferenced);
	return infiniteRectF();
}

void Template::scale(double factor, const MapCoord& center)
{
	Q_ASSERT(!is_georeferenced);
	setTemplatePosition(center + factor * (templatePosition() - center));
	setTemplateScaleX(factor * getTemplateScaleX());
	setTemplateScaleY(factor * getTemplateScaleY());
	
	accounted_offset *= factor;
}

void Template::rotate(double rotation, const MapCoord& center)
{
	Q_ASSERT(!is_georeferenced);
	
	auto offset = templatePositionOffset();
	setTemplatePositionOffset({});
	
	setTemplateRotation(getTemplateRotation() + rotation);
	
	auto position = MapCoordF{templatePosition() - center};
	position.rotate(-rotation);
	setTemplatePosition(MapCoord{position} + center);
	
	setTemplatePositionOffset(offset);
}

void Template::setTemplateAreaDirty()
{
	QRectF template_area = calculateTemplateBoundingBox();
	map->setTemplateAreaDirty(this, template_area, getTemplateBoundingBoxPixelBorder());	// TODO: Would be better to do this with the corner points, instead of the bounding box
}

bool Template::canBeDrawnOnto() const
{
	return false;
}

QRectF Template::calculateTemplateBoundingBox() const
{
	// Create bounding box by calculating the positions of all corners of the transformed extent rect
	QRectF extent = getTemplateExtent();
	QRectF bbox;
	rectIncludeSafe(bbox, templateToMap(extent.topLeft()));
	rectInclude(bbox, templateToMap(extent.topRight()));
	rectInclude(bbox, templateToMap(extent.bottomRight()));
	rectInclude(bbox, templateToMap(extent.bottomLeft()));
	return bbox;
}

void Template::drawOntoTemplate(not_null<MapCoordF*> coords, int num_coords, QColor color, float width, QRectF map_bbox)
{
	Q_ASSERT(canBeDrawnOnto());
	Q_ASSERT(num_coords > 1);
	
	if (!map_bbox.isValid())
	{
		map_bbox = QRectF(coords[0].x(), coords[0].y(), 0, 0);
		for (int i = 1; i < num_coords; ++i)
			rectInclude(map_bbox, coords[i]);
	}
	float radius = qMin(getTemplateScaleX(), getTemplateScaleY()) * qMax((width+1) / 2, 1.0f);
	QRectF radius_bbox = QRectF(map_bbox.left() - radius, map_bbox.top() - radius,
								map_bbox.width() + 2*radius, map_bbox.height() + 2*radius);
	
	drawOntoTemplateImpl(coords, num_coords, color, width);
	map->setTemplateAreaDirty(this, radius_bbox, 0);
	
	setHasUnsavedChanges(true);
}

void Template::drawOntoTemplateUndo(bool redo)
{
	Q_UNUSED(redo);
	// nothing
}

void Template::addPassPoint(const PassPoint& point, int pos)
{
	Q_ASSERT(!is_georeferenced);
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
	Q_ASSERT(!is_georeferenced);
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
void Template::getTransform(TemplateTransform& out) const
{
	Q_ASSERT(!is_georeferenced);
	out = transform;
}
void Template::setTransform(const TemplateTransform& transform)
{
	Q_ASSERT(!is_georeferenced);
	setTemplateAreaDirty();
	
	this->transform = transform;
	updateTransformationMatrices();
	
	setTemplateAreaDirty();
	map->setTemplatesDirty();
}
void Template::getOtherTransform(TemplateTransform& out) const
{
	Q_ASSERT(!is_georeferenced);
	out = other_transform;
}
void Template::setOtherTransform(const TemplateTransform& transform)
{
	Q_ASSERT(!is_georeferenced);
	other_transform = transform;
}

void Template::setTemplatePath(const QString& value)
{
	template_path = value;
	if (! QFileInfo(value).canonicalFilePath().isEmpty())
		template_path = QFileInfo(value).canonicalFilePath();
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

const std::vector<QByteArray>& Template::supportedExtensions()
{
	static std::vector<QByteArray> extensions;
	if (extensions.empty())
	{
		auto& image_extensions = TemplateImage::supportedExtensions();
		auto& map_extensions   = TemplateMap::supportedExtensions();
#ifdef MAPPER_USE_GDAL
		auto& ogr_extensions   = OgrTemplate::supportedExtensions();
#else
		auto ogr_extensions    = std::vector<QByteArray>{ };
#endif
		auto& track_extensions = TemplateTrack::supportedExtensions();
		extensions.reserve(image_extensions.size()
		                   + map_extensions.size()
		                   + ogr_extensions.size()
		                   + track_extensions.size());
		extensions.insert(end(extensions), begin(image_extensions), end(image_extensions));
		extensions.insert(end(extensions), begin(map_extensions), end(map_extensions));
		extensions.insert(end(extensions), begin(ogr_extensions), end(ogr_extensions));
		extensions.insert(end(extensions), begin(track_extensions), end(track_extensions));
	}
	return extensions;
}

std::unique_ptr<Template> Template::templateForFile(const QString& path, Map* map)
{
	auto path_ends_with_any_of = [path](const std::vector<QByteArray>& list) -> bool {
		using namespace std;
		return any_of(begin(list), end(list), [path](const QByteArray& extension) {
			return path.endsWith(QLatin1String(extension), Qt::CaseInsensitive);
		} );
	};
	
	std::unique_ptr<Template> t;
	if (path_ends_with_any_of(TemplateImage::supportedExtensions()))
		t.reset(new TemplateImage(path, map));
	else if (path_ends_with_any_of(TemplateMap::supportedExtensions()))
		t.reset(new TemplateMap(path, map));
#ifdef MAPPER_USE_GDAL
	else if (path_ends_with_any_of(OgrTemplate::supportedExtensions()))
		t.reset(new OgrTemplate(path, map));
#endif
	else if (path_ends_with_any_of(TemplateTrack::supportedExtensions()))
		t.reset(new TemplateTrack(path, map));
#ifdef MAPPER_USE_GDAL
	else
		t.reset(new OgrTemplate(path, map));
#endif
	
	return t;
}



void Template::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	Q_UNUSED(xml);
	// nothing
}

bool Template::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	xml.skipCurrentElement();
	return true;
}

void Template::drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width)
{
	Q_UNUSED(coords);
	Q_UNUSED(num_coords);
	Q_UNUSED(color);
	Q_UNUSED(width);
	// nothing
}

MapCoord Template::templatePosition() const
{
	return MapCoord::fromNative(transform.template_x, transform.template_y);
}


void Template::setTemplatePosition(MapCoord coord)
{
	transform.template_x = coord.nativeX();
	transform.template_y = coord.nativeY();
	updateTransformationMatrices();
}

MapCoord Template::templatePositionOffset() const
{
	return accounted_offset;
}

void Template::setTemplatePositionOffset(MapCoord offset)
{
	const auto move = accounted_offset - offset;
	if (move != MapCoord{})
	{
		accounted_offset = move;
		applyTemplatePositionOffset();
	}
	accounted_offset = offset;
}

void Template::applyTemplatePositionOffset()
{
	QTransform t;
	t.rotate(-qRadiansToDegrees(getTemplateRotation()));
	t.scale(getTemplateScaleX(), getTemplateScaleY());
	setTemplatePosition(templatePosition() - MapCoord{t.map(QPointF{accounted_offset})});
}

void Template::resetTemplatePositionOffset()
{
	accounted_offset = {};
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


}  // namespace OpenOrienteering
