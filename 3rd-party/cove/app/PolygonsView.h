/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 * Copyright 2020 Kai Pastor
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COVE_POLYGONSVIEW_H
#define COVE_POLYGONSVIEW_H

#include <QObject>
#include <QString>

#include "libvectorizer/Polygons.h"

#include "QImageView.h"

class QPaintEvent;
class QWidget;

namespace cove {

class PolyImageWidget : public ImageWidget
{
	Q_OBJECT

public:
	PolyImageWidget(QWidget* parent = nullptr);

	PolyImageWidget(const PolyImageWidget&) = delete;
	PolyImageWidget(PolyImageWidget&&) = delete;
	~PolyImageWidget() override;

	PolyImageWidget& operator=(const PolyImageWidget&) = delete;
	PolyImageWidget& operator=(PolyImageWidget&&) = delete;

	const PolygonList& polygons() const;
	void setPolygons(PolygonList p);

protected:
	void paintEvent(QPaintEvent* pe) override;

private:
	PolygonList polygonsList;
};


class PolygonsView : public QImageView
{
	Q_OBJECT

public:
	PolygonsView(QWidget* parent = nullptr);

	PolygonsView(const PolygonsView&) = delete;
	PolygonsView(PolygonsView&&) = delete;
	~PolygonsView() override;

	PolygonsView& operator=(const PolygonsView&) = delete;
	PolygonsView& operator=(PolygonsView&&) = delete;

	const PolygonList& polygons() const;
	void setPolygons(PolygonList p);
};


}  // namespace cove

#endif
