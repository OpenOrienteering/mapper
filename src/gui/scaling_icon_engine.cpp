/*
 *    Copyright 2020 Kai Pastor
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


#include "scaling_icon_engine.h"

#include <Qt>
#include <QtGlobal>
#include <QCoreApplication>
#include <QIcon>
#include <QIconEngine>
#include <QLatin1String>
#include <QList>
#include <QPaintDevice>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QSize>


namespace {

/**
 * Functor which returns true if an object with a particular size shall be scaled up
 * to the requested size.
 */
struct needsScalingTo
{
	const QSize& requested_size;
	
	explicit needsScalingTo (const QSize& requested_size) noexcept : requested_size(requested_size)
	{}
	
	bool operator()(const QSize& s) const noexcept
	{
		return 4 * s.width() < 3 * requested_size.width();
	}
	template<class T>
	bool operator()(const T& t) const noexcept
	{
		return operator()(t.size());
	}
};


}  // namespace



namespace OpenOrienteering {

// override
ScalingIconEngine::~ScalingIconEngine() = default;

ScalingIconEngine::ScalingIconEngine(const QString& filename)
: QIconEngine()
, icon(filename + QLatin1String(".bmp"))
{
	icon.addFile(filename);
}

ScalingIconEngine::ScalingIconEngine(const QIcon& icon)
: QIconEngine()
, icon(icon)
{}

// override
void ScalingIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
	auto const device_pixel_ratio = qApp->testAttribute(Qt::AA_UseHighDpiPixmaps) ? painter->device()->devicePixelRatioF() : qreal(1.0);
	QSize pixmapSize = rect.size() * device_pixel_ratio;
	painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

// override
QPixmap ScalingIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
	auto pm = icon.pixmap(size, mode, state);
	if (needsScalingTo(size)(pm))
	{
		pm = pm.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		addPixmap(pm, mode, state);
	}
	return pm;
}

// override
QIconEngine* ScalingIconEngine::clone() const
{
	return new ScalingIconEngine(*this);
}

// override
void ScalingIconEngine::addPixmap(const QPixmap& pixmap, QIcon::Mode mode, QIcon::State state)
{
	icon.addPixmap(pixmap, mode, state);
}

// override
void ScalingIconEngine::addFile(const QString& fileName, const QSize& size, QIcon::Mode mode, QIcon::State state)
{
	icon.addFile(fileName, size, mode, state);
}

// override
QSize ScalingIconEngine::actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
	auto actual_size = icon.actualSize(size, mode, state);
	if (needsScalingTo(size)(actual_size))
	{
		actual_size = size;
	}
	return actual_size;
}

// override
QString ScalingIconEngine::key() const
{
	return QStringLiteral("ScalingIconEngine");
}

// override
QList<QSize> ScalingIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) const
{
	return icon.availableSizes(mode, state);
}

// override
QString  ScalingIconEngine::iconName() const
{
	return icon.name();
}


}  // namespace OpenOrienteering
