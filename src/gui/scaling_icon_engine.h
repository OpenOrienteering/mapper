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


#ifndef OPENORIENTEERING_SCALING_ICON_ENGINE_H
#define OPENORIENTEERING_SCALING_ICON_ENGINE_H

#include <QIcon>
#include <QIconEngine>
#include <QList>
#include <QPixmap>
#include <QSize>
#include <QString>

class QPainter;
class QRect;

namespace OpenOrienteering {


/**
 * An icon engine that can scale up icons.
 * 
 * This engine relies on BMP icons being handled by the regular Qt engine.
 */
class ScalingIconEngine : public QIconEngine
{
public:
	~ScalingIconEngine();
	
	explicit ScalingIconEngine(const QString& filename);

	explicit ScalingIconEngine(const QIcon& icon);

	ScalingIconEngine(const ScalingIconEngine&) = default;
	
	ScalingIconEngine& operator=(const ScalingIconEngine&) = delete;
	
	void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
	
	QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
	
	QIconEngine* clone() const override;
	
	void addPixmap(const QPixmap& pixmap, QIcon::Mode mode, QIcon::State state) override;
	
	void addFile(const QString& fileName, const QSize& size, QIcon::Mode mode, QIcon::State state) override;
	
	QSize actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
	
	QString key() const override;
	
	QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) const override;
	
	QString iconName() const override;
	
private:
	QIcon icon;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SCALING_ICON_ENGINE_H
