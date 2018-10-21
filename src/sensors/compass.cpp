/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2014, 2018 Kai Pastor
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

#include "compass.h"

#include <QMetaMethod>
#include <QMetaObject>

#ifdef QT_SENSORS_LIB

#include <cmath>
#include <cstring>

#include <Qt>
#include <QtMath>
#include <QAccelerometer>
#include <QAccelerometerReading>
#include <QGyroscope>
#include <QGyroscopeFilter>
#include <QGyroscopeReading>
#include <QList>
#include <QMagnetometer>
#include <QMagnetometerReading>
#include <QMutex>
#include <QSensor>
#include <QThread>
#include <QWaitCondition>

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniObject>
#endif

#else  // no Qt Sensors lib

#include <QTime>

#endif  // QT_SENSORS_LIB


// clazy:excludeall=missing-qobject-macro


namespace OpenOrienteering {

#ifdef QT_SENSORS_LIB

namespace {

namespace SensorHelpers {
	
	void matrixMultiplication(const qreal* const A, const qreal* B, qreal* result)
	{
		result[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
		result[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
		result[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];

		result[3] = A[3] * B[0] + A[4] * B[3] + A[5] * B[6];
		result[4] = A[3] * B[1] + A[4] * B[4] + A[5] * B[7];
		result[5] = A[3] * B[2] + A[4] * B[5] + A[5] * B[8];

		result[6] = A[6] * B[0] + A[7] * B[3] + A[8] * B[6];
		result[7] = A[6] * B[1] + A[7] * B[4] + A[8] * B[7];
		result[8] = A[6] * B[2] + A[7] * B[5] + A[8] * B[8];
	}

	/**
	 * Ported from android.hardware.SensorManager.getOrientation().
	 * 
	 * Computes the device's orientation based on the rotation matrix.
	 * <p> When it returns, the array values is filled with the result:
	 * <ul>
	 * <li>values[0]: <i>azimuth</i>, rotation around the Z axis.</li>
	 * <li>values[1]: <i>pitch</i>, rotation around the X axis.</li>
	 * <li>values[2]: <i>roll</i>, rotation around the Y axis.</li>
	 * </ul>
	 * <p>
	 */
	void getOrientation(const qreal* const R, qreal* values)
	{
		/*
		* / R[ 0] R[ 1] R[ 2] \
		* | R[ 3] R[ 4] R[ 5] |
		* \ R[ 6] R[ 7] R[ 8] /
		*/
		values[0] = atan2(R[1], R[4]);
		values[1] = asin(-R[7]);
		values[2] = atan2(-R[6], R[8]);
	}
	
	/** From http://www.thousand-thoughts.com/2012/03/android-sensor-fusion-tutorial/2/
	 *  Should be optimized ... */
	void getRotationMatrixFromOrientation(const qreal* const orientation, qreal* result)
	{
		qreal xM[9];
		qreal yM[9];
		qreal zM[9];

		const auto sinX = qSin(orientation[1]);
		const auto cosX = qCos(orientation[1]);
		const auto sinY = qSin(orientation[2]);
		const auto cosY = qCos(orientation[2]);
		const auto sinZ = qSin(orientation[0]);
		const auto cosZ = qCos(orientation[0]);

		// rotation about x-axis (pitch)
		xM[0] = 1; xM[1] =     0; xM[2] =    0;
		xM[3] = 0; xM[4] =  cosX; xM[5] = sinX;
		xM[6] = 0; xM[7] = -sinX; xM[8] = cosX;

		// rotation about y-axis (roll)
		yM[0] =  cosY; yM[1] = 0; yM[2] = sinY;
		yM[3] =     0; yM[4] = 1; yM[5] =    0;
		yM[6] = -sinY; yM[7] = 0; yM[8] = cosY;

		// rotation about z-axis (azimuth)
		zM[0] =  cosZ; zM[1] = sinZ; zM[2] = 0;
		zM[3] = -sinZ; zM[4] = cosZ; zM[5] = 0;
		zM[6] =     0; zM[7] =    0; zM[8] = 1;

		// rotation order is y, x, z (roll, pitch, azimuth)
		qreal temp[9];
		matrixMultiplication(xM, yM, temp);
		matrixMultiplication(zM, temp, result);
	}
	
	/**
	 * Ported from android.hardware.SensorManager.getRotationMatrix().
	 * 
	 * Computes the inclination matrix <b>I</b> as well as the rotation
	 * matrix <b>R</b> transforming a vector from the
	 * device coordinate system to the world's coordinate system [...]
	 */
	bool getRotationMatrix(qreal* R, const qreal* const gravity, const qreal* const geomagnetic)
	{
		auto Ax = gravity[0];
		auto Ay = gravity[1];
		auto Az = gravity[2];
		// Change relative to Android version: scale values from teslas to microteslas
		const auto to_micro = qreal(1000*1000);
		const auto Ex = to_micro * geomagnetic[0];
		const auto Ey = to_micro * geomagnetic[1];
		const auto Ez = to_micro * geomagnetic[2];
		
		auto Hx = Ey*Az - Ez*Ay;
		auto Hy = Ez*Ax - Ex*Az;
		auto Hz = Ex*Ay - Ey*Ax;
		const auto normH = sqrt(Hx*Hx + Hy*Hy + Hz*Hz);
		if (normH < 0.1) {
			// device is close to free fall (or in space?), or close to
			// magnetic north pole. Typical values are > 100.
			return false;
		}
		const auto invH = 1 / normH;
		Hx *= invH;
		Hy *= invH;
		Hz *= invH;
		const auto invA = 1 / sqrt(Ax*Ax + Ay*Ay + Az*Az);
		Ax *= invA;
		Ay *= invA;
		Az *= invA;
		const auto Mx = Ay*Hz - Az*Hy;
		const auto My = Az*Hx - Ax*Hz;
		const auto Mz = Ax*Hy - Ay*Hx;
		
		R[0] = Hx; R[1] = Hy; R[2] = Hz;
		R[3] = Mx; R[4] = My; R[5] = Mz;
		R[6] = Ax; R[7] = Ay; R[8] = Az;
		
		return true;
	}
	
	/**
	 * Ported from android.hardware.SensorManager.getRotationMatrixFromVector().
	 * 
	 * Helper function to convert a rotation vector to a rotation matrix.
	 */
	void getRotationMatrixFromVector(qreal* R, const qreal* const rotationVector)
	{
		const auto q0 = rotationVector[3];  // !
		const auto q1 = rotationVector[0];
		const auto q2 = rotationVector[1];
		const auto q3 = rotationVector[2];

		const auto sq_q1 = 2 * q1 * q1;
		const auto sq_q2 = 2 * q2 * q2;
		const auto sq_q3 = 2 * q3 * q3;
		const auto q1_q2 = 2 * q1 * q2;
		const auto q3_q0 = 2 * q3 * q0;
		const auto q1_q3 = 2 * q1 * q3;
		const auto q2_q0 = 2 * q2 * q0;
		const auto q2_q3 = 2 * q2 * q3;
		const auto q1_q0 = 2 * q1 * q0;

		R[0] = 1 - sq_q2 - sq_q3;
		R[1] = q1_q2 - q3_q0;
		R[2] = q1_q3 + q2_q0;

		R[3] = q1_q2 + q3_q0;
		R[4] = 1 - sq_q1 - sq_q3;
		R[5] = q2_q3 - q1_q0;

		R[6] = q1_q3 - q2_q0;
		R[7] = q2_q3 + q1_q0;
		R[8] = 1 - sq_q1 - sq_q2;
	}
	
	
}  // namespace SensorHelpers

}  // namespace



class CompassPrivate : public QGyroscopeFilter
{
public:
	CompassPrivate(Compass* compass)
	: thread(this)
	, compass(compass)
	, gyro_available(QSensor::sensorsForType(QGyroscope::type).empty())
	{
		// Try to filter out non-gravity sources of acceleration
		accelerometer.setAccelerationMode(QAccelerometer::Gravity);

		// Try to filter out local magnetic interference
		magnetometer.setReturnGeoValues(true);
		
		// Check if a gyroscope is available
		if (gyro_available)
			gyroscope.addFilter(this);
		
		thread.start();
	}
	
	~CompassPrivate() override
	{
		thread.keep_running = false;
		thread.condition.wakeAll();
		thread.wait();
	}
	
	void enable(bool enabled)
	{
		if (enabled)
		{
			last_gyro_timestamp = 0;
			gyro_orientation_initialized = false;
			
			accelerometer.start();
			magnetometer.start();
			gyroscope.start();
			
			thread.wait_mutex.lock();
			this->enabled = true;
			thread.condition.wakeAll();
			thread.wait_mutex.unlock();
		}
		else
		{
			this->enabled = false;
			gyroscope.stop();
			magnetometer.stop();
			accelerometer.stop();
		}
	}
	
	qreal getLatestAzimuth()
	{
		return latest_azimuth;
	}
	
	/** Called on new gyro readings */
	bool filter(QGyroscopeReading* reading) override
	{
		if (! gyro_orientation_initialized)
			return false;
		
		if (last_gyro_timestamp > 0)
		{
			// Calculate rotation matrix from gyro reading according to Android developer documentation
			const auto EPSILON = qreal(1e-9);
			
			const auto microseconds_to_seconds = 1 / qreal(1000*1000);
			const auto dt = (reading->timestamp() - last_gyro_timestamp) * microseconds_to_seconds;
			// Axis of the rotation sample, not normalized yet.
			auto axisX = qDegreesToRadians(reading->x());
			auto axisY = qDegreesToRadians(reading->y());
			auto axisZ = qDegreesToRadians(reading->z());

			// Calculate the angular speed of the sample
			const auto omegaMagnitude = sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);

			// Normalize the rotation vector if it's big enough to get the axis
			if (omegaMagnitude > EPSILON) {
				axisX /= omegaMagnitude;
				axisY /= omegaMagnitude;
				axisZ /= omegaMagnitude;
			}

			// Integrate around this axis with the angular speed by the timestep
			// in order to get a delta rotation from this sample over the timestep
			// We will convert this axis-angle representation of the delta rotation
			// into a quaternion before turning it into the rotation matrix.
			const auto thetaOverTwo = omegaMagnitude * dt / 2;
			const auto sinThetaOverTwo = qSin(thetaOverTwo);
			const auto cosThetaOverTwo = qCos(thetaOverTwo);
			qreal deltaRotationVector[4];
			deltaRotationVector[0] = sinThetaOverTwo * axisX;
			deltaRotationVector[1] = sinThetaOverTwo * axisY;
			deltaRotationVector[2] = sinThetaOverTwo * axisZ;
			deltaRotationVector[3] = cosThetaOverTwo;
			
			qreal R[9];
			SensorHelpers::getRotationMatrixFromVector(R, deltaRotationVector);
			
			qreal temp[9];
			gyro_mutex.lock();
			SensorHelpers::matrixMultiplication(gyro_rotation_matrix, R, temp);
			memcpy(gyro_rotation_matrix, temp, 9 * sizeof(float));
			SensorHelpers::getOrientation(gyro_rotation_matrix, gyro_orientation);
			gyro_mutex.unlock();
		}
		
		last_gyro_timestamp = reading->timestamp();
		return true;
	}
	
private:
	/// \todo Review QThread inheritance / Q_OBJECT macro usage
	class SensorThread : public QThread
	{
	// no Q_OBJECT as it is not required here and was problematic in .cpp
	//Q_OBJECT
	public:
		SensorThread(CompassPrivate* p)
		: p(p)
		{
			// nothing else
		}
		
		qreal fuseOrientationCoefficient(qreal gyro, qreal acc_mag)
		{
			const auto FILTER_COEFFICIENT = qreal(0.98);
			const auto ONE_MINUS_COEFF = 1 - FILTER_COEFFICIENT;
			
			qreal result;
			// Correctly handle wrap-around
			if (gyro < -0.5 * M_PI && acc_mag > 0.0) {
				result = FILTER_COEFFICIENT * (gyro + 2.0 * M_PI) + ONE_MINUS_COEFF * acc_mag;
				result -= (result > M_PI) ? 2.0 * M_PI : 0;
			}
			else if (acc_mag < -0.5 * M_PI && gyro > 0.0) {
				result = FILTER_COEFFICIENT * gyro + ONE_MINUS_COEFF * (acc_mag + 2.0 * M_PI);
				result -= (result > M_PI)? 2.0 * M_PI : 0;
			}
			else {
				result = FILTER_COEFFICIENT * gyro + ONE_MINUS_COEFF * acc_mag;
			}
			return result;
		}
		
		void filter()
		{
			if (p->accelerometer.reading() == nullptr ||
				p->magnetometer.reading() == nullptr)
				return;
			
			// Make copies of the sensor readings (and hope that the reading thread
			// does not overwrite parts of them while they are being copied)
			const qreal acceleration[3] = {
				p->accelerometer.reading()->x(),
				p->accelerometer.reading()->y(),
				p->accelerometer.reading()->z()
			};
			const qreal geomagnetic[3] = {
				p->magnetometer.reading()->x(),
				p->magnetometer.reading()->y(),
				p->magnetometer.reading()->z()
			};
			
			// Calculate orientation from accelerometer and magnetometer (acc_mag_orientation)
			qreal R[9];
			bool ok = SensorHelpers::getRotationMatrix(R, acceleration, geomagnetic);
			if (!ok)
				return;
			
			qreal acc_mag_orientation[3];
			SensorHelpers::getOrientation(R, acc_mag_orientation);
			
			// If gyro not initialized yet (or we do not have a gyro):
			// use acc_mag_orientation (and initialize gyro if present)
			qreal azimuth;
			if (! p->gyro_available || ! p->gyro_orientation_initialized)
			{
				if (p->gyro_available)
				{
					memcpy(p->gyro_orientation, acc_mag_orientation, 3 * sizeof(float));
					SensorHelpers::getRotationMatrixFromOrientation(p->gyro_orientation, p->gyro_rotation_matrix);
					p->gyro_orientation_initialized = true;
				}
				
				azimuth = acc_mag_orientation[0];
			}
			else
			{
				// Filter acc_mag_orientation and gyro_orientation to fused_orientation
				p->gyro_mutex.lock();
				qreal fused_orientation[3];
				fused_orientation[0] = fuseOrientationCoefficient(p->gyro_orientation[0], acc_mag_orientation[0]);
				fused_orientation[1] = fuseOrientationCoefficient(p->gyro_orientation[1], acc_mag_orientation[1]);
				fused_orientation[2] = fuseOrientationCoefficient(p->gyro_orientation[2], acc_mag_orientation[2]);
				
				// Write back fused_orientation to gyro_orientation
				SensorHelpers::getRotationMatrixFromOrientation(fused_orientation, p->gyro_rotation_matrix);
				memcpy(p->gyro_orientation, fused_orientation, 3 * sizeof(float));
				p->gyro_mutex.unlock();
				
				azimuth = fused_orientation[0];
			}
			
			p->latest_azimuth = qRadiansToDegrees(azimuth);
#ifdef Q_OS_ANDROID
			// Adjust for display rotation
			jint orientation = QAndroidJniObject::callStaticMethod<jint>(
			                       "org/openorienteering/mapper/MapperActivity",
			                       "getDisplayRotation");
			switch (orientation)
			{
			case 1:
				p->latest_azimuth = (p->latest_azimuth < 90) ? (p->latest_azimuth + 90) : (p->latest_azimuth - 90);
				break;
			case 2:
				p->latest_azimuth = (p->latest_azimuth < 180) ? (p->latest_azimuth + 180) : (p->latest_azimuth - 180);
				break;
			case 3:
				p->latest_azimuth = (p->latest_azimuth < 270) ? (p->latest_azimuth + 270) : (p->latest_azimuth - 270);
				break;
			default:
				;// nothing
			}
#endif

			// Send update to receivers
			p->compass->emitAzimuthChanged(p->latest_azimuth);
		}
		
		void run() override
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
		CompassPrivate* p;
		bool keep_running = true;
	};
	
	SensorThread thread;
	QAccelerometer accelerometer;
	QMagnetometer magnetometer;
	Compass* compass;
	QGyroscope gyroscope;
	QMutex gyro_mutex;
	quint64 last_gyro_timestamp;
	qreal gyro_rotation_matrix[9];
	qreal gyro_orientation[3];
	qreal latest_azimuth = -1;
	bool gyro_available;
	bool gyro_orientation_initialized;
	bool enabled = false;
};
#else
class CompassPrivate {};
#endif


Compass::Compass(): QObject()
{
#ifdef QT_SENSORS_LIB
	p.reset(new CompassPrivate(this));
#endif
}

Compass::~Compass() = default;

Compass& Compass::getInstance()
{
	static Compass theOneAndOnly;
	return theOneAndOnly;
}

void Compass::startUsage()
{
	++ reference_counter;
#ifdef QT_SENSORS_LIB
	if (reference_counter == 1)
		p->enable(true);
#endif
}

void Compass::stopUsage()
{
	-- reference_counter;
#ifdef QT_SENSORS_LIB
	if (reference_counter == 0)
		p->enable(false);
#endif
}

qreal Compass::getCurrentAzimuth()
{
#ifdef QT_SENSORS_LIB
	return p->getLatestAzimuth();
#elif MAPPER_DEVELOPMENT_BUILD
	// DEBUG: rotate around ...
	QTime now = QTime::currentTime();
	return 360 * (now.msecsSinceStartOfDay() % (10 * 1000)) / qreal(10 * 1000);
#else
	return 0;
#endif
}

void Compass::connectToAzimuthChanges(const QObject* receiver, const char* slot)
{
#ifdef QT_SENSORS_LIB
	connect(this, SIGNAL(azimuthChanged(qreal)), receiver, slot, Qt::QueuedConnection);
#else
	Q_UNUSED(receiver);
	Q_UNUSED(slot);
#endif
}

void Compass::disconnectFromAzimuthChanges(const QObject* receiver)
{
#ifdef QT_SENSORS_LIB
	this->disconnect(receiver);
#else
	Q_UNUSED(receiver);
#endif
}

void Compass::connectNotify(const QMetaMethod& signal)
{
	if (signal == QMetaMethod::fromSignal(&Compass::azimuthChanged))
	    startUsage();
}

void Compass::disconnectNotify(const QMetaMethod& signal)
{
	if (signal == QMetaMethod::fromSignal(&Compass::azimuthChanged))
	    stopUsage();
}

void Compass::emitAzimuthChanged(qreal value)
{
	emit azimuthChanged(value);
}


}  // namespace OpenOrienteering
