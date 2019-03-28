/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2015 Kai Pastor
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


#ifndef OPENORIENTEERING_COMPASS_DISPLAY_H
#define OPENORIENTEERING_COMPASS_DISPLAY_H

#include <QtGlobal>
#include <QObject>
#include <QSize>
#include <QTime>
#include <QWidget>

class QHideEvent;
class QPaintEvent;
class QShowEvent;

namespace OpenOrienteering {


/**
 * A widget which displays a compass.
 * 
 * By default, this widget is transparent (Qt::WA_NoSystemBackground), i.e. it
 * it does not draw a background. For proper background drawing, it shall not be
 * a child but a sibling of the background widget.
 */
class CompassDisplay : public QWidget
{
Q_OBJECT
public:
	/** 
	 * Creates a compass display.
	 */
	CompassDisplay(QWidget* parent = nullptr);
	
	/** 
	 * Destructor.
	 */
	~CompassDisplay() override;
	
	/** 
	 * Sets the compass direction, and updates the widget.
	 * 
	 * This does nothing unless at least 200 ms elapsed since the last change.
	 */
	void setAzimuth(qreal azimuth_deg);
	
	QSize sizeHint() const override;
	
protected:
	void showEvent(QShowEvent* event) override;
	
	void hideEvent(QHideEvent* event) override;
	
	void paintEvent(QPaintEvent* event) override;
	
	qreal azimuth;
	QTime last_update_time;
};


}  // namespace OpenOrienteering

#endif
