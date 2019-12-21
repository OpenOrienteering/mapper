/*
 * Copyright (c) 2005-2019 Libor Pecháček.
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

#ifndef COVE_QIMAGEVIEW_H
#define COVE_QIMAGEVIEW_H

#include <memory>

#include <QScrollArea>
#include <QWidget>

class QImage;
class QPaintEvent;

namespace cove {
class ImageWidget : public QWidget
{
protected:
	const QImage* dispImage;
	float dispMagnification;
	bool scalingSmooth;
	bool dispRealPaintEnabled;
	QRect drect;
	virtual void paintEvent(QPaintEvent* pe);

public:
	ImageWidget(QWidget* parent = 0);
	virtual ~ImageWidget();
	const QImage* image() const;
	void setImage(const QImage* im);
	float magnification() const;
	void setMagnification(float mag);
	bool smoothScaling() const;
	void setSmoothScaling(bool ss);
	bool realPaintEnabled() const;
	void setRealPaintEnabled(bool ss);
	void displayRect(const QRect& r);
};

class QImageView : public QScrollArea
{
	Q_OBJECT

protected:
	std::unique_ptr<ImageWidget> iw;
	QPoint dragStartPos, dragCurPos;
	QRect zoomRect;
	enum OperatingMode
	{
		MOVE,
		ZOOM_IN,
		ZOOM_OUT
	} opMode;
	int lastSliderHPos, lastSliderVPos;
	void wheelEvent(QWheelEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

public:
	QImageView(QWidget* parent = 0);
	void reset();
	const QImage* image() const;
	void setImage(const QImage* im);
	float magnification() const;
	void setMagnification(float mag);
	bool smoothScaling() const;
public slots:
	void setMoveMode();
	void setZoomInMode();
	void setZoomOutMode();
	void setOrigSize();
	void setSmoothScaling(bool ss);
signals:
	void magnificationChanged(float oldmag, float mag);
};
} // cove

#endif
