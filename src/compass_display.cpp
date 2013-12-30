/*
 *    Copyright 2013 Thomas Schöps
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

#include "compass_display.h"

#include <QPainter>

#include "map_widget.h"
#include "util.h"

// #ifdef HAVE_QTSENSORS
#include <qmath.h>
#include <QThread>
#include <QTime>
#include <QMutex>
#include <QDebug>
#include <QWaitCondition>
#include <QtSensors/QAccelerometer>
#include <QtSensors/QMagnetometer>
class CompassDisplayPrivate
{
friend class CompassDisplay;
public:
	CompassDisplayPrivate(CompassDisplay* display)
	 : thread(this)
	 , display(display)
	 , enabled(false)
	{
		// Try to filter out local magnetic interference
		magnetometer.setReturnGeoValues(true);
		
// 		accelerometer.addFilter(this);
// 		magnetometer.addFilter(this);
		
		thread.start();
	}
	
	~CompassDisplayPrivate()
	{
		thread.keep_running = false;
		thread.condition.wakeAll();
		thread.wait();
	}
	
	void enable(bool enabled)
	{
		if (enabled)
		{
			accelerometer.start();
			magnetometer.start();
			last_update_time = QTime::currentTime();
			
			thread.wait_mutex.lock();
			this->enabled = true;
			thread.condition.wakeAll();
			thread.wait_mutex.unlock();
		}
		else
		{
			this->enabled = false;
			accelerometer.stop();
			magnetometer.stop();
		}
	}
	
private:
	class SensorThread : public QThread
	{
	//Q_OBJECT
	public:
		SensorThread(CompassDisplayPrivate* p)
		 : p(p)
		{
			 keep_running = true;
		}
		
		/**
		 * Ported from android.hardware.SensorManager.getOrientation().
		 * 
		 * Computes the device's orientation based on the rotation matrix.
		 * <p> When it returns, the array values is filled with the result:
		 * <li>values[0]: <i>azimuth</i>, rotation around the Z axis.</li>
		 * <li>values[1]: <i>pitch</i>, rotation around the X axis.</li>
		 * <li>values[2]: <i>roll</i>, rotation around the Y axis.</li>
		 * <p>
		 */
		void getOrientation(float* R, float* values)
		{
			/*
			* / R[ 0] R[ 1] R[ 2] \
			* | R[ 3] R[ 4] R[ 5] |
			* \ R[ 6] R[ 7] R[ 8] /
			*/
			values[0] = (float)std::atan2(R[1], R[4]);
			values[1] = (float)std::asin(-R[7]);
			values[2] = (float)std::atan2(-R[6], R[8]);
		}
		
		/**
		 * Ported from android.hardware.SensorManager.getRotationMatrix().
		 * 
		 * Computes the inclination matrix <b>I</b> as well as the rotation
		 * matrix <b>R</b> transforming a vector from the
		 * device coordinate system to the world's coordinate system [...]
		*/
		bool getRotationMatrix(float* R, float* gravity, float* geomagnetic)
		{
			float Ax = gravity[0];
			float Ay = gravity[1];
			float Az = gravity[2];
			// Change relative to Android version: scale values from teslas to microteslas
			const float to_micro = 1000*1000;
			const float Ex = to_micro * geomagnetic[0];
			const float Ey = to_micro * geomagnetic[1];
			const float Ez = to_micro * geomagnetic[2];
			
			float Hx = Ey*Az - Ez*Ay;
			float Hy = Ez*Ax - Ex*Az;
			float Hz = Ex*Ay - Ey*Ax;
			const float normH = (float)std::sqrt(Hx*Hx + Hy*Hy + Hz*Hz);
			if (normH < 0.1f) {
				// device is close to free fall (or in space?), or close to
				// magnetic north pole. Typical values are > 100.
				return false;
			}
			const float invH = 1.0f / normH;
			Hx *= invH;
			Hy *= invH;
			Hz *= invH;
			const float invA = 1.0f / (float)std::sqrt(Ax*Ax + Ay*Ay + Az*Az);
			Ax *= invA;
			Ay *= invA;
			Az *= invA;
			const float Mx = Ay*Hz - Az*Hy;
			const float My = Az*Hx - Ax*Hz;
			const float Mz = Ax*Hy - Ay*Hx;
			
			R[0] = Hx; R[1] = Hy; R[2] = Hz;
			R[3] = Mx; R[4] = My; R[5] = Mz;
			R[6] = Ax; R[7] = Ay; R[8] = Az;
			
// 			if (I != null) {
// 				// compute the inclination matrix by projecting the geomagnetic
// 				// vector onto the Z (gravity) and X (horizontal component
// 				// of geomagnetic vector) axes.
// 				const float invE = 1.0f / (float)std::sqrt(Ex*Ex + Ey*Ey + Ez*Ez);
// 				const float c = (Ex*Mx + Ey*My + Ez*Mz) * invE;
// 				const float s = (Ex*Ax + Ey*Ay + Ez*Az) * invE;
// 				I[0] = 1; I[1] = 0; I[2] = 0;
// 				I[3] = 0; I[4] = c; I[5] = s;
// 				I[6] = 0; I[7] =-s; I[8] = c;
// 			}
			return true;
		}
		
		void filter()
		{
			const int draw_update_interval = 30; // milliseconds
			
			if (p->accelerometer.reading() == NULL ||
				p->magnetometer.reading() == NULL)
				return;
			
			// Make copies of the sensor readings (and hope that the reading thread
			// does not overwrite parts of them while they are being copied)
			float acceleration[3] = {
				(float) p->accelerometer.reading()->x(),
				(float) p->accelerometer.reading()->y(),
				(float) p->accelerometer.reading()->z()
			};
			float geomagnetic[3] = {
				(float) p->magnetometer.reading()->x(),
				(float) p->magnetometer.reading()->y(),
				(float) p->magnetometer.reading()->z()
			};
			
			// Calculate azimuth
			float R[9];
			bool ok = getRotationMatrix(R, acceleration, geomagnetic);
			if (!ok)
				return;
			
			float orientation[3];
			getOrientation(R, orientation);
			float azimuth = orientation[0];
			
			// Send update to compass display
			QTime current_time = QTime::currentTime();
			if (qAbs(p->last_update_time.msecsTo(current_time)) >= draw_update_interval)
			{
				p->display->valueChanged(180 * azimuth / M_PI);
				p->last_update_time = current_time;
			}
		}
		
		void run()
		{
			// Wait until sensors are initialized
			QThread::msleep(1000);
			
			while (keep_running)
			{
				if (! p->enabled)
				{
					wait_mutex.lock();
					if (! p->enabled)
						condition.wait(&wait_mutex);
					wait_mutex.unlock();
					
					// May do initializations after (re-)enabling here
				}
				
				filter();
				
				QThread::msleep(30);
			}
		}
		
		QMutex wait_mutex;
		QWaitCondition condition;
		bool keep_running;
		CompassDisplayPrivate* p;
	};
	
	QAccelerometer accelerometer;
	QMagnetometer magnetometer;
	SensorThread thread;
	CompassDisplay* display;
	QTime last_update_time;
	bool enabled;
};
// #endif


CompassDisplay::CompassDisplay(MapWidget* map_widget) : QObject(NULL)
{
	this->widget = map_widget;
	
#ifdef HAVE_QTSENSORS
	p = new CompassDisplayPrivate(this);
#else
	p = NULL;
#endif
	
	enabled = false;
	have_value = false;
	
	widget->setCompassDisplay(this);
}

CompassDisplay::~CompassDisplay()
{
	widget->setCompassDisplay(NULL);
#ifdef HAVE_QTSENSORS
	delete p;
#endif
}

void CompassDisplay::enable(bool enabled)
{
	if (enabled == this->enabled)
		return;
	if (enabled && ! this->enabled)
		have_value = false;
	
#ifdef HAVE_QTSENSORS
	p->enable(enabled);
#endif
	this->enabled = enabled;
	
	updateMapWidget();
}

void CompassDisplay::paint(QPainter* painter)
{
	if (!enabled)
		return;
	
	painter->setBrush(Qt::NoBrush);
	QRectF bounding_box = calcBoundingBox();
	QPointF midpoint = bounding_box.center();
	QPointF endpoint = QPointF(midpoint.x(), bounding_box.top());
	QLineF line(midpoint, endpoint);
	
	// Draw up marker
	float line_width = Util::mmToPixelLogical(0.3f);
	
	painter->setPen(QPen(Qt::white, line_width));
	painter->drawLine(line);
	
	painter->setPen(QPen(Qt::black, line_width));
	painter->drawLine(midpoint + QPointF(line_width, 0), endpoint + QPointF(line_width, 0));
	painter->drawLine(midpoint + QPointF(-1 * line_width, 0), endpoint + QPointF(-1 * line_width, 0));
	
	// Draw needle
	if (have_value)
	{
		painter->setPen(QPen(Qt::red, line_width));
		line.setAngle(90 + value_azimuth);
		line.setLength(line.length() * 0.5f * (1 + value_calibration));
		painter->drawLine(line);
	}
}

void CompassDisplay::valueChanged(float azimuth_deg)
{
	value_azimuth = azimuth_deg;
	value_calibration = 1;
	have_value = true;
	updateMapWidget();
}

void CompassDisplay::updateMapWidget()
{
	widget->update(calcBoundingBox().toAlignedRect());
}

QRectF CompassDisplay::calcBoundingBox()
{
	float start = Util::mmToPixelLogical(2.0f);
	float width = Util::mmToPixelLogical(20.0f);
	return QRectF(start, start, width, width);
}
