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

#include "ImageView.h"

#include <cmath>

#include <QAction>
#include <QBitmap>
#include <QBrush>
#include <QColor>
#include <QCursor>
#include <QFlags>
#include <QGuiApplication>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRgb>
#include <QScrollBar>
#include <QSize>
#include <QVector>
#include <QWheelEvent>
#include <QWidget>
#include <Qt>

class QMouseEvent;
class QWheelEvent;

namespace cove {
//@{
//! \ingroup gui

/*! \class ImageWidget
  \brief QWidget that draws a QImage on its face.
 */

/*! \var const QImage* ImageWidget::dispImage
  QImage to be displayed.
  \sa setImage
*/

/*! \var float ImageWidget::dispMagnification
  Magnification factor of image display.
  \sa setMagnification
*/

/*! \var bool ImageWidget::scalingSmooth
  Whether the Qt::SmoothTransformation options should be used when scaling
  image.
  \sa setSmoothScaling
*/

/*! \var bool ImageWidget::dispRealPaintEnabled
  Do or don't do real paint when moving the image.  Real paint is disabled as
  long as the user moves the image.
  \sa setRealPaintEnabled
*/

/*! \var QRect ImageWidget::drect
  Rectangle that is displayed when doing zoom-in operation.
*/

//! Default constructor.
ImageWidget::ImageWidget(QWidget* parent)
	: QWidget(parent)
{}

//! Destructor.
ImageWidget::~ImageWidget() = default;

//! Painting event handler.
void ImageWidget::paintEvent(QPaintEvent* pe)
{
	if (!dispRealPaintEnabled) return;
	QPainter p(this);
	QRect r = pe->rect();

	// if there is an dispImage and is not a null dispImage draw it
	if (dispImage && !dispImage->isNull())
	{
		// 1-bit deep dispImages are drawn in different way by painter.  Their
		// native
		// colors are ignored.
		if (dispImage->depth() == 1)
		{
			p.setBackground(QBrush(dispImage->color(0)));
			p.setPen(dispImage->color(1));
		}

		// adjust coordinates
		int w = int(std::ceil(r.width() / dispMagnification)) + 1,
			h = int(std::ceil(r.height() / dispMagnification)) + 1,
			x = int(std::floor(r.x() / dispMagnification)),
			y = int(std::floor(r.y() / dispMagnification));

		// create cut of the original dispImage
		// Workaround Qt4 bug in copy(QRect(...))
		if (y + h > dispImage->height()) h = dispImage->height() - y;
		QImage copy = dispImage->copy(QRect(x, y, w, h));

		if (!copy.isNull())
		{
			// Gray out the image when widget is disabled.
			if (!isEnabled())
			{
				// copy = copy.convertToFormat(copy.format(),
				// Qt::MonoOnly|Qt::AvoidDither);
				// Qt 4 workaround - MonoOnly doesn't work
				if (copy.depth() == 32)
				{
					int p = copy.width() * copy.height();
					QRgb* b = reinterpret_cast<QRgb*>(copy.bits());
					while (p--)
					{
						int I = qGray(*b);
						*b++ = qRgb(I, I, I);
					}
				}
				else
				{
					QVector<QRgb> ct = copy.colorTable();
					for (auto& color : ct)
					{
						auto const gray = qGray(color);
						color = qRgb(gray, gray, gray);
					}
					copy.setColorTable(ct);
				}
			}

			// scale it
			if (dispMagnification != 1)
			{
				copy = copy.scaled(int(w * dispMagnification),
								   int(h * dispMagnification),
								   Qt::IgnoreAspectRatio,
								   scalingSmooth ? Qt::SmoothTransformation
												 : Qt::FastTransformation);
			}

			// and draw the copy onto widget
			p.drawImage(
				QPoint(int(x * dispMagnification), int(y * dispMagnification)),
				copy);
		}
	}

	if (!drect.isNull())
	{
		p.setPen(QColor(Qt::magenta));
		p.setBrush(QColor(255, 128, 255, 64));
		p.drawRect(drect);
	}
}

//! What dispImage should be viewed.
void ImageWidget::setImage(const QImage* im)
{
	dispImage = im;
	setMagnification(magnification());
	update();
}

/*! \fn const QImage* ImageWidget::image() const
 * Returns pointer to currently displayed bitmap.
 */


//! What dispMagnification should be used. 1 == no dispMagnification, < 1 size
//! reduction.
void ImageWidget::setMagnification(qreal mag)
{
	if (dispImage)
	{
		int w = static_cast<int>(mag * dispImage->width()),
			h = static_cast<int>(mag * dispImage->height());
		setMinimumSize(w, h);
		resize(w, h);
	}
	dispMagnification = mag;
}

/*! \fn qreal ImageWidget::magnification() const
 * Returns current magnification used.
 * \sa setMagnification
 */


/*! Use smooth scaling (QImage::smoothScale).
 \sa scalingSmooth */
void ImageWidget::setSmoothScaling(bool ss)
{
	bool doupdate = scalingSmooth ^ ss;
	scalingSmooth = ss;
	if (doupdate) update();
}

/*! \fn bool ImageWidget::smoothScaling() const
 * Returns value of the scalingSmooth flag.
 * \sa setSmoothScaling
 */


/*! Do or do not real do painting.  When set to false, paints only background.
 \sa dispRealPaintEnabled */
void ImageWidget::setRealPaintEnabled(bool ss)
{
	dispRealPaintEnabled = ss;
	if (dispRealPaintEnabled) update();
}

/*! \fn bool ImageWidget::realPaintEnabled() const
 * Returns value of the scalingSmooth flag.
 * \sa setRealPaintEnabled
 */


/*! Sets displayed zoom-in rectangle.
  \sa drect */
void ImageWidget::displayRect(const QRect& r)
{
	QRect updreg = r.adjusted(-1, -1, 2, 2);

	if (drect.isValid()) updreg |= drect.adjusted(-1, -1, 2, 2);

	drect = r;
	update(updreg);
}

/*! \class ImageView
		\brief Provides scrollable view of an QImage.
 */

/*! \var ImageWidget* ImageView::iw
 Pointer to currently viewed image. \sa setImage. */
/*! \var QPoint ImageView::dragStartPos
 Point where dragging started when zooming in. */
/*! \var QPoint ImageView::dragCurPos
 Current point where dragging is when zooming in. */
/*! \var QRect ImageView::zoomRect
 THe light magenta semitransparent rectangle used when zoming in. */
/*! \var OperatingMode ImageView::opMode
 Current operation mode as selected in context menu. */
/*! \var int ImageView::lastSliderHPos
 Value of horizontal slider used when moving image. */
/*! \var int ImageView::lastSliderVPos
 Value of vertical slider used when moving image. */
/*! \enum ImageView::OperatingMode
  Operation mode as selected in context menu.
  \sa opMode */
/*! \var ImageView::OperatingMode ImageView::MOVE
 Move mode. */
/*! \var ImageView::OperatingMode ImageView::ZOOM_IN
 Zoom in mode. */
/*! \var ImageView::OperatingMode ImageView::ZOOM_OUT
 Zoom out mode. */
/*! \fn void ImageView::magnificationChanged(float oldmag, float mag)
  Signal emitted when zoom ratio changes. */

//! Default constructor.
ImageView::ImageView(QWidget* parent)
	: QScrollArea(parent)
	, iw(std::make_unique<ImageWidget>(this))
{
	setWidget(iw.get());
	reset();
	QAction* a;
	addAction(a = new QAction(tr("Move"), this));
	connect(a, SIGNAL(triggered(bool)), this, SLOT(setMoveMode()));
	addAction(a = new QAction(tr("Zoom in"), this));
	connect(a, SIGNAL(triggered(bool)), this, SLOT(setZoomInMode()));
	addAction(a = new QAction(tr("Zoom out"), this));
	connect(a, SIGNAL(triggered(bool)), this, SLOT(setZoomOutMode()));
	addAction(a = new QAction(this));
	a->setSeparator(true);
	addAction(a = new QAction(tr("Original size"), this));
	connect(a, SIGNAL(triggered(bool)), this, SLOT(setOrigSize()));
	addAction(a = new QAction(tr("Smooth scaling"), this));
	a->setCheckable(true);
	connect(a, SIGNAL(toggled(bool)), this, SLOT(setSmoothScaling(bool)));
	setContextMenuPolicy(Qt::ActionsContextMenu);
}

//! Resets QImaeView into its initial state.  I.e. move mode, magnification 1,
//! scrollbars 0.
void ImageView::reset()
{
	setMoveMode();
	setMagnification(1);
	horizontalScrollBar()->setValue(0);
	verticalScrollBar()->setValue(0);
}

//! Mouse wheel based zooming.
void ImageView::wheelEvent(QWheelEvent* event)
{
	static QPoint accumulator;
	static int stepsize = 8 * 15 * 2;

	accumulator += event->angleDelta();
	if (abs(accumulator.y()) >= stepsize)
	{
		int sgn = accumulator.y() / abs(accumulator.y());
		accumulator.ry() -= stepsize * sgn;
		double nm = magnification() * pow(2, sgn);
		if (nm < 32 && nm > 1 / 32) setMagnification(nm);
	}
}

//! Handles mouse click.
void ImageView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) return;

	if (opMode == ZOOM_IN ||
		(opMode == MOVE &&
		 QGuiApplication::keyboardModifiers() == Qt::ControlModifier))
	{
		dragStartPos = event->pos();
	}
	else if (opMode == MOVE)
	{
		dragStartPos = event->pos();
		iw->setRealPaintEnabled(false);
		lastSliderHPos = horizontalScrollBar()->value();
		lastSliderVPos = verticalScrollBar()->value();
	}
	else if (opMode == ZOOM_OUT)
	{
		double mwm = viewport()->width() / iw->width();
		double mhm = viewport()->height() / iw->height();
		double nm = magnification() / 2;
		if (nm > 1 / 32 && (nm <= 1 || nm >= mwm || nm >= mhm))
			setMagnification(nm);
	}
}

//! Handles mouse drag when moving image or zooming in.
void ImageView::mouseMoveEvent(QMouseEvent* event)
{
	if (!(event->buttons() & Qt::LeftButton)) return;

	if (opMode == ZOOM_IN ||
		(opMode == MOVE &&
		 QGuiApplication::keyboardModifiers() == Qt::ControlModifier))
	{
		if ((event->pos() - dragStartPos).manhattanLength() > 5)
		{
			dragCurPos = event->pos();

			QRect vportrect = viewport()->rect();
			if (!vportrect.contains(dragCurPos))
			{
				int dx = 0, dy = 0, t;
				if ((t = dragCurPos.x() - vportrect.right()) > 0)
					dx -= t;
				else if ((t = dragCurPos.x() - vportrect.left()) < 0)
					dx -= t;

				if ((t = dragCurPos.y() - vportrect.bottom()) > 0)
					dy -= t;
				else if ((t = dragCurPos.y() - vportrect.top()) < 0)
					dy -= t;

				QPoint d = iw->pos() + QPoint(dx, dy);
				if (d.x() < (t = viewport()->width() - iw->width()))
					d.setX(t);
				else if (d.x() > 0)
					d.setX(0);
				if (d.y() < (t = viewport()->height() - iw->height()))
					d.setY(t);
				else if (d.y() > 0)
					d.setY(0);
				dragStartPos += d - iw->pos();
				iw->move(d);
			}

			QPoint dist = dragStartPos - dragCurPos;
			int x, y, w, h;

			if (dist.x() > 0)
			{
				x = dragCurPos.x();
				w = dist.x();
			}
			else
			{
				x = dragStartPos.x();
				w = -dist.x();
			}

			if (dist.y() > 0)
			{
				y = dragCurPos.y();
				h = dist.y();
			}
			else
			{
				y = dragStartPos.y();
				h = -dist.y();
			}

			zoomRect = QRect(QPoint(x, y) - iw->pos(), QSize(w, h))
						   .intersected(iw->rect().adjusted(0, 0, -1, -1));
			iw->displayRect(zoomRect);
		}
	}
	else if (opMode == MOVE)
	{
		QPoint pixmapOffset = event->pos() - dragStartPos;
		verticalScrollBar()->setValue(lastSliderVPos - pixmapOffset.y());
		horizontalScrollBar()->setValue(lastSliderHPos - pixmapOffset.x());
	}
}

//! Handles end-of-drag.
void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) return;

	if (opMode == ZOOM_IN ||
		(opMode == MOVE &&
		 QGuiApplication::keyboardModifiers() == Qt::ControlModifier))
	{
		int dx, dy;
		if ((event->pos() - dragStartPos).manhattanLength() > 5 &&
			zoomRect.width() && zoomRect.height())
		{
			double m = viewport()->width() / zoomRect.width(),
				   n = viewport()->height() / zoomRect.height(),
				   rmag = m > n ? n : m; // minimum of those two
			// max possible 2^n magnif. with n integer
			double magmul = pow(2, trunc(log(rmag) / log(2)));
			if (floor(magmul) == 0.0) magmul = 1;
			if (magnification() * magmul > 16) magmul = 16.0 / magnification();
			rmag = magnification() * magmul;
			QPoint zrc = zoomRect.center() + iw->pos();
			dx = int(magmul * (zrc.x() - viewport()->size().width() / 2));
			dy = int(magmul * (zrc.y() - viewport()->size().height() / 2));
			iw->displayRect(QRect());
			setMagnification(rmag);
		}
		else
		{
			int magmul = magnification() < 16 ? 2 : 1;
			setMagnification(magnification() * magmul);
			dx = magmul * (event->pos().x() - viewport()->size().width() / 2);
			dy = magmul * (event->pos().y() - viewport()->size().height() / 2);
		}
		verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
	}
	else if (opMode == MOVE)
	{
		QPoint pixmapOffset = event->pos() - dragStartPos;
		iw->setRealPaintEnabled(true);
		verticalScrollBar()->setValue(lastSliderVPos - pixmapOffset.y());
		horizontalScrollBar()->setValue(lastSliderHPos - pixmapOffset.x());
	}
}

//! What dispImage should be viewed.
void ImageView::setImage(const QImage* im)
{
	iw->setImage(im);
}

//! return pointer to currently displayed bitmap.
const QImage* ImageView::image() const
{
	return iw->image();
}

//! What dispMagnification should be used. 1 == no dispMagnification, < 1 size
//! reduction.
void ImageView::setMagnification(qreal mag)
{
	double oldmag = iw->magnification(), magratio = mag / oldmag,
		   oneMmagratio = 1 - magratio;
	int w = viewport()->size().width(), h = viewport()->size().height();
	int dx = (int)((iw->pos().x() - w / 2.0) * oneMmagratio),
		dy = (int)((iw->pos().y() - h / 2.0) * oneMmagratio);
	iw->setMagnification(mag);
	verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
	horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
	if (oldmag != mag) emit magnificationChanged(oldmag, mag);
}

//! Current magnification used.
qreal ImageView::magnification() const
{
	return iw->magnification();
}

//! Use smooth scaling (QImage::smoothScale).
void ImageView::setSmoothScaling(bool ss)
{
	iw->setSmoothScaling(ss);
}

//! Returns true when smooth scaling is enabled.
bool ImageView::smoothScaling() const
{
	return iw->smoothScaling();
}

//! Slot for context menu.
void ImageView::setMoveMode()
{
	iw->setCursor(QCursor(Qt::SizeAllCursor));
	opMode = MOVE;
}

//! Slot for context menu.
void ImageView::setZoomInMode()
{
#define zoomin_width 32
#define zoomin_height 32
	static unsigned char zoomin_bits[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x38, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
		0x80, 0x00, 0x02, 0x00, 0x40, 0x10, 0x04, 0x00, 0x40, 0x10, 0x04, 0x00,
		0x20, 0x10, 0x08, 0x00, 0x20, 0xfe, 0x08, 0x00, 0x20, 0x10, 0x08, 0x00,
		0x40, 0x10, 0x04, 0x00, 0x40, 0x10, 0x04, 0x00, 0x80, 0x00, 0x02, 0x00,
		0x00, 0x01, 0x07, 0x00, 0x00, 0xc6, 0x0e, 0x00, 0x00, 0x38, 0x1c, 0x00,
		0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0xe0, 0x00,
		0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define zoomin_mask_width 32
#define zoomin_mask_height 32
	static unsigned char zoomin_mask_bits[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
		0x00, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x80, 0xc7, 0x03, 0x00,
		0xc0, 0x11, 0x07, 0x00, 0xe0, 0x38, 0x0e, 0x00, 0xe0, 0x38, 0x0e, 0x00,
		0x70, 0xfe, 0x1c, 0x00, 0x70, 0xff, 0x1d, 0x00, 0x70, 0xfe, 0x1c, 0x00,
		0xe0, 0x38, 0x0e, 0x00, 0xe0, 0x38, 0x0e, 0x00, 0xc0, 0x11, 0x07, 0x00,
		0x80, 0xc7, 0x0f, 0x00, 0x00, 0xff, 0x1f, 0x00, 0x00, 0xfe, 0x3e, 0x00,
		0x00, 0x38, 0x7c, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xf0, 0x01,
		0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	iw->setCursor(
		QCursor(QBitmap::fromData(QSize(zoomin_width, zoomin_height),
								  zoomin_bits, QImage::Format_MonoLSB),
				QBitmap::fromData(QSize(zoomin_mask_width, zoomin_mask_height),
								  zoomin_mask_bits, QImage::Format_MonoLSB),
				13, 13));
	opMode = ZOOM_IN;
}

//! Slot for context menu.
void ImageView::setZoomOutMode()
{
#define zoomout_width 32
#define zoomout_height 32
	static unsigned char zoomout_bits[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x38, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
		0x80, 0x00, 0x02, 0x00, 0x40, 0x00, 0x04, 0x00, 0x40, 0x00, 0x04, 0x00,
		0x20, 0x00, 0x08, 0x00, 0x20, 0xfe, 0x08, 0x00, 0x20, 0x00, 0x08, 0x00,
		0x40, 0x00, 0x04, 0x00, 0x40, 0x00, 0x04, 0x00, 0x80, 0x00, 0x02, 0x00,
		0x00, 0x01, 0x07, 0x00, 0x00, 0xc6, 0x0e, 0x00, 0x00, 0x38, 0x1c, 0x00,
		0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0xe0, 0x00,
		0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define zoomout_mask_width 32
#define zoomout_mask_height 32
	static unsigned char zoomout_mask_bits[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
		0x00, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x80, 0xc7, 0x03, 0x00,
		0xc0, 0x01, 0x07, 0x00, 0xe0, 0x00, 0x0e, 0x00, 0xe0, 0x00, 0x0e, 0x00,
		0x70, 0xfe, 0x1c, 0x00, 0x70, 0xff, 0x1d, 0x00, 0x70, 0xfe, 0x1c, 0x00,
		0xe0, 0x00, 0x0e, 0x00, 0xe0, 0x00, 0x0e, 0x00, 0xc0, 0x01, 0x07, 0x00,
		0x80, 0xc7, 0x0f, 0x00, 0x00, 0xff, 0x1f, 0x00, 0x00, 0xfe, 0x3e, 0x00,
		0x00, 0x38, 0x7c, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xf0, 0x01,
		0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	iw->setCursor(QCursor(
		QBitmap::fromData(QSize(zoomout_width, zoomout_height), zoomout_bits,
						  QImage::Format_MonoLSB),
		QBitmap::fromData(QSize(zoomout_mask_width, zoomout_mask_height),
						  zoomout_mask_bits, QImage::Format_MonoLSB),
		13, 13));
	opMode = ZOOM_OUT;
}

//! Slot for context menu. Sets magnification to 1.
void ImageView::setOrigSize()
{
	setMagnification(1);
}
} // cove

//@}
