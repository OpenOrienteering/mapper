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

#ifndef COVE_IMAGEVIEW_H
#define COVE_IMAGEVIEW_H

#include <memory>

#include <QtGlobal>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QScrollArea>
#include <QString>
#include <QWidget>

class QImage;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

namespace cove {

class ImageWidget : public QWidget
{
	Q_OBJECT

public:
	ImageWidget(QWidget* parent = nullptr);
	ImageWidget(const ImageWidget&) = delete;
	ImageWidget(ImageWidget&&) = delete;
	~ImageWidget() override;

	ImageWidget& operator=(const ImageWidget&) = delete;
	ImageWidget&& operator=(ImageWidget&&) = delete;

	const QImage* image() const { return dispImage; }
	void setImage(const QImage* im);

	qreal magnification() const { return dispMagnification; }
	void setMagnification(qreal mag);

	bool smoothScaling() const { return scalingSmooth; }
	void setSmoothScaling(bool ss);

	bool realPaintEnabled() const { return dispRealPaintEnabled; }
	void setRealPaintEnabled(bool ss);

	void displayRect(const QRect& r);

protected:
	void paintEvent(QPaintEvent* pe) override;

private:
	const QImage* dispImage = nullptr;
	QRect drect = {0, 0, 0, 0};
	qreal dispMagnification = 1;
	bool scalingSmooth = false;
	bool dispRealPaintEnabled = true;
};


class ImageView : public QScrollArea
{
	Q_OBJECT

public:
	ImageView(QWidget* parent = nullptr);
	ImageView(const ImageView&) = delete;
	ImageView(ImageView&&) = delete;
	~ImageView() override;

	ImageView& operator=(const ImageView&) = delete;
	ImageView& operator=(ImageView&&) = delete;

	void reset();

	const QImage* image() const;
	void setImage(const QImage* im);

	qreal magnification() const;
	void setMagnification(qreal mag);

	bool smoothScaling() const;

public slots:
	void setMoveMode();
	void setZoomInMode();
	void setZoomOutMode();
	void setOrigSize();
	void setSmoothScaling(bool ss);

signals:
	void magnificationChanged(qreal oldmag, qreal mag);

protected:
	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

	std::unique_ptr<ImageWidget> iw;

private:
	QPoint dragStartPos;
	QPoint dragCurPos;
	QRect zoomRect;
	int lastSliderHPos = 0;
	int lastSliderVPos = 0;
	enum OperatingMode
	{
		MOVE,
		ZOOM_IN,
		ZOOM_OUT
	} opMode = MOVE;
};


} // cove

#endif
