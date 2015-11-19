/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#include "util.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QIODevice>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QScreen>

#include "core/map_coord.h"
#include "mapper_resource.h"
#include "settings.h"
#include <mapper_config.h>

DoubleValidator::DoubleValidator(double bottom, double top, QObject* parent, int decimals) : QDoubleValidator(bottom, top, decimals, parent)
{
	// Don't cause confusion, accept only English formatting
	setLocale(QLocale(QLocale::English));
}
QValidator::State DoubleValidator::validate(QString& input, int& pos) const
{
	// Transform thousands group separators into decimal points to avoid confusion
	input.replace(',', '.');
	
	return QDoubleValidator::validate(input, pos);
}

void blockSignalsRecursively(QObject* obj, bool block)
{
	if (!obj)
		return;
	obj->blockSignals(block);
	
	const QObjectList& list = obj->children();
	for (QObjectList::const_iterator it = list.begin(), end = list.end(); it != end; ++it)
		blockSignalsRecursively(*it, block);
}

void rectInclude(QRectF& rect, QPointF point)
{
	if (point.x() < rect.left())
		rect.setLeft(point.x());
	else if (point.x() > rect.right())
		rect.setRight(point.x());
	
	if (point.y() < rect.top())
		rect.setTop(point.y());
	else if (point.y() > rect.bottom())
		rect.setBottom(point.y());
}

void rectIncludeSafe(QRectF& rect, QPointF point)
{
	if (rect.isValid())
		rectInclude(rect, point);
	else
		rect = QRectF(point.x(), point.y(), 0.0001, 0.0001);
}

void rectInclude(QRectF& rect, const QRectF& other_rect)
{
	if (other_rect.left() < rect.left())
		rect.setLeft(other_rect.left());
	if (other_rect.right() > rect.right())
		rect.setRight(other_rect.right());
	
	if (other_rect.top() < rect.top())
		rect.setTop(other_rect.top());
	if (other_rect.bottom() > rect.bottom())
		rect.setBottom(other_rect.bottom());
}
void rectIncludeSafe(QRectF& rect, const QRectF& other_rect)
{
	if (other_rect.isValid())
	{
		if (rect.isValid())
			rectInclude(rect, other_rect);
		else 
			rect = other_rect;
	}
}

void rectIncludeSafe(QRect& rect, const QRect& other_rect)
{
	if (other_rect.isValid())
	{
		if (rect.isValid())
		{
			if (other_rect.left() < rect.left())
				rect.setLeft(other_rect.left());
			if (other_rect.right() > rect.right())
				rect.setRight(other_rect.right());
			
			if (other_rect.top() < rect.top())
				rect.setTop(other_rect.top());
			if (other_rect.bottom() > rect.bottom())
				rect.setBottom(other_rect.bottom());
		}
		else 
			rect = other_rect;
	}
}

bool lineIntersectsRect(const QRectF& rect, const QPointF& p1, const QPointF& p2)
{
	if (rect.contains(p1) || rect.contains(p2))
		return true;
	
	// Left side
	if ((p1.x() > rect.left()) != (p2.x() > rect.left()))
	{
		if ((p2.x() == p1.x()) && !((p1.y() < rect.top() && p2.y() < rect.top()) || (p1.y() > rect.bottom() && p2.y() > rect.bottom())))
			return true;
		qreal intersection_y = p1.y() + (p2.y() - p1.y()) * (rect.left() - p1.x()) / (p2.x() - p1.x());
		if (intersection_y >= rect.top() && intersection_y <= rect.bottom())
			return true;
	}
	// Right side
	if ((p1.x() > rect.right()) != (p2.x() > rect.right()))
	{
		if ((p2.x() == p1.x()) && !((p1.y() < rect.top() && p2.y() < rect.top()) || (p1.y() > rect.bottom() && p2.y() > rect.bottom())))
			return true;
		qreal intersection_y = p1.y() + (p2.y() - p1.y()) * (rect.right() - p1.x()) / (p2.x() - p1.x());
		if (intersection_y >= rect.top() && intersection_y <= rect.bottom())
			return true;
	}
	
	// Top side
	if ((p1.y() > rect.top()) != (p2.y() > rect.top()))
	{
		if ((p2.y() == p1.y()) && !((p1.x() < rect.left() && p2.x() < rect.left()) || (p1.x() > rect.right() && p2.x() > rect.right())))
			return true;
		qreal intersection_x = p1.x() + (p2.x() - p1.x()) * (rect.top() - p1.y()) / (p2.y() - p1.y());
		if (intersection_x >= rect.left() && intersection_x <= rect.right())
			return true;
	}
	// Bottom side
	if ((p1.y() > rect.bottom()) != (p2.y() > rect.bottom()))
	{
		if ((p2.y() == p1.y()) && !((p1.x() < rect.left() && p2.x() < rect.left()) || (p1.x() > rect.right() && p2.x() > rect.right())))
			return true;
		qreal intersection_x = p1.x() + (p2.x() - p1.x()) * (rect.bottom() - p1.y()) / (p2.y() - p1.y());
		if (intersection_x >= rect.left() && intersection_x <= rect.right())
			return true;
	}
	
	return false;
}

double parameterOfPointOnLine(double x0, double y0, double dx, double dy, double x, double y, bool& ok)
{
	const double epsilon = 1e-5;
	ok = true;
	
	if (qAbs(dx) > qAbs(dy))
	{
		double param = (x - x0) / dx;
		if (qAbs(y0 + param * dy - y) < epsilon)
			return param;
	}
	else if (dy != 0)
	{
		double param = (y - y0) / dy;
		if (qAbs(x0 + param * dx - x) < epsilon)
			return param;
	}
	
	ok = false;
	return -1;
}

bool isPointOnSegment(const MapCoordF& seg_start, const MapCoordF& seg_end, const MapCoordF& point)
{
	bool ok;
	double param = parameterOfPointOnLine(seg_start.x(), seg_start.y(),
	                                      seg_end.x() - seg_start.x(), seg_end.y() - seg_start.y(),
	                                      point.x(), point.y(), ok);
	return ok && param >= 0 && param <= 1;
}

void loadString(QIODevice* file, QString& str)
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

namespace Util
{

void showHelp(QWidget* dialog_parent, QString filename, QString fragment)
{
	static QProcess assistant_process;
	if (assistant_process.state() == QProcess::Running)
	{
		QString command("setSource " + makeHelpUrl(filename, fragment) + "\n");
		assistant_process.write(command.toLatin1());
	}
	else
	{
		const QString help_collection_path(MapperResource::locate(MapperResource::MANUAL, QLatin1String("Mapper ")+APP_VERSION+" Manual.qhc"));
		const QString compiled_help_path(MapperResource::locate(MapperResource::MANUAL, QLatin1String("Mapper ")+APP_VERSION+" Manual.qch"));
		if (help_collection_path.isEmpty() || compiled_help_path.isEmpty())
		{
			QMessageBox::warning(dialog_parent, QApplication::translate("Util", "Error"), QApplication::translate("Util", "Failed to locate the help files."));
			return;
		}
		
		QString assistant_path = MapperResource::locate(MapperResource::ASSISTANT);
		if (assistant_path.isEmpty())
		{
			QMessageBox::warning(dialog_parent, QApplication::translate("Util", "Error"), QApplication::translate("Util", "Failed to locate the help browser (\"Qt Assistant\")."));
			return;
		}
		
		// Try to start the Qt Assistant process
		QStringList args;
		args << QLatin1String("-collectionFile")
			 << QDir::toNativeSeparators(help_collection_path)
			 << QLatin1String("-showUrl")
			 << makeHelpUrl(filename, fragment)
			 << QLatin1String("-enableRemoteControl");
		
		if ( QGuiApplication::platformName() == QLatin1String("xcb") ||
			 QGuiApplication::platformName().isEmpty() )
		{
			// Use the modern 'fusion' style instead of the default "windows"
			// style on X11.
			args << QLatin1String("-style") << QLatin1String("fusion");
		}
		
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
		auto env = QProcessEnvironment::systemEnvironment();
		env.insert(QLatin1String("QT_SELECT"), QLatin1String("qt5")); // #541
		assistant_process.setProcessEnvironment(env);
#endif
		
		assistant_process.start(assistant_path, args);
		
		// FIXME: Calling waitForStarted() from the main thread might cause the user interface to freeze.
		if (!assistant_process.waitForStarted())
		{
			QMessageBox msg_box;
			msg_box.setIcon(QMessageBox::Warning);
			msg_box.setWindowTitle(QApplication::translate("Util", "Error"));
			msg_box.setText(QApplication::translate("Util", "Failed to launch the help browser (\"Qt Assistant\")."));
			msg_box.setStandardButtons(QMessageBox::Ok);
			QString details = assistant_process.readAllStandardError();
			if (! details.isEmpty())
				msg_box.setDetailedText(details);
			
			msg_box.exec();
		}
	}
}

QString makeHelpUrl(QString filename, QString fragment)
{
	return QLatin1String("qthelp://") + MAPPER_HELP_NAMESPACE + "/manual/" + filename + (fragment.isEmpty() ? "" : ("#" + fragment));
}

qreal mmToPixelPhysical(qreal millimeters)
{
	auto ppi = Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal();
	return millimeters * ppi / 25.4;
}

qreal pixelToMMPhysical(qreal pixels)
{
	auto ppi = Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal();
	return pixels * 25.4 / ppi;
}

qreal mmToPixelLogical(qreal millimeters)
{
	auto ppi = QApplication::primaryScreen()->logicalDotsPerInch();
	return millimeters * ppi / 25.4f;
}

qreal pixelToMMLogical(qreal pixels)
{
	auto ppi = QApplication::primaryScreen()->logicalDotsPerInch();
	return pixels * 25.4f / ppi;
}

bool isAntialiasingRequired()
{
	return isAntialiasingRequired(Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal());
}

}
