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

#include <array>
#include <cmath>

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

	using Vector = std::array<qreal, 3>;
	using Quaternion = std::array<qreal, 4>;
	/**
	 * A 3x3 matrix, with values in row-major order.
	 * 
	 * / R[ 0] R[ 1] R[ 2] \
	 * | R[ 3] R[ 4] R[ 5] |
	 * \ R[ 6] R[ 7] R[ 8] /
	 */
	using Matrix = std::array<qreal, 9>;

	template<class Reading>
	Vector toVector(const Reading& reading)
	{
		return Vector {
			reading.x(),
			reading.y(),
			reading.z()
		};
	}
	
	constexpr Matrix operator*(const Matrix& A, const Matrix& B)
	{
		return {
			A[0] * B[0] + A[1] * B[3] + A[2] * B[6],
			A[0] * B[1] + A[1] * B[4] + A[2] * B[7],
			A[0] * B[2] + A[1] * B[5] + A[2] * B[8],

			A[3] * B[0] + A[4] * B[3] + A[5] * B[6],
			A[3] * B[1] + A[4] * B[4] + A[5] * B[7],
			A[3] * B[2] + A[4] * B[5] + A[5] * B[8],

			A[6] * B[0] + A[7] * B[3] + A[8] * B[6],
			A[6] * B[1] + A[7] * B[4] + A[8] * B[7],
			A[6] * B[2] + A[7] * B[5] + A[8] * B[8]
		};
	}

	/**
	 * Computes the device's orientation based on the rotation matrix.
	 * 
	 * Ported from android.hardware.SensorManager.getOrientation().
	 * The members of the returned vector have the following meaning:
	 * 
	 * - First:  azimuth, rotation around the Z axis.
	 * - Second: pitch, rotation around the X axis.
	 * - Third:  roll, rotation around the Y axis.
	 */
	Vector getOrientation(const Matrix& R)
	{
		return {
			atan2(R[1], R[4]),  // azimuth
			asin(-R[7]),        // pitch
			atan2(-R[6], R[8])  // rotation
		};
	}
	
	/**
	 * Cf. http://www.thousand-thoughts.com/2012/03/android-sensor-fusion-tutorial/2/
	 */
	Matrix getRotationMatrixFromOrientation(const Vector& orientation)
	{
		const auto sinX = qSin(orientation[1]);
		const auto cosX = qCos(orientation[1]);
		const auto sinY = qSin(orientation[2]);
		const auto cosY = qCos(orientation[2]);
		const auto sinZ = qSin(orientation[0]);
		const auto cosZ = qCos(orientation[0]);

		// rotation about x-axis (pitch)
		const auto xM = Matrix {
		    1,   0,    0,
		    0,  cosX, sinX,
		    0, -sinX, cosX
		};

		// rotation about y-axis (roll)
		const auto yM = Matrix {
		     cosY,  0,  sinY,
		       0,   1,   0,
		    -sinY,  0,  cosY
		};

		// rotation about z-axis (azimuth)
		const auto zM = Matrix {
		     cosZ,  sinZ,  0,
		    -sinZ,  cosZ,  0,
		       0,     0,   1
		};

		// rotation order is y, x, z (roll, pitch, azimuth)
		return zM * (xM * yM);
	}
	
	/**
	 * Computes the rotation matrix transforming a vector from the
	 * device coordinate system to the world's coordinate system.
	 * 
	 * In atypical situations (free fall; close to magnetic north pole), the
	 * returned result isn't valid. This is indicated by return NaN in the
	 * first element of the matrix.
	 * 
	 * Ported from android.hardware.SensorManager.getRotationMatrix().
	 */
	Matrix getRotationMatrix(const Vector& gravity, const Vector& geomagnetic)
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
		if (normH < 0.1)
		{
			// device is close to free fall (or in space?), or close to
			// magnetic north pole. Typical values are > 100.
			Hx = qQNaN();
		}
		else
		{
			// okay
			const auto invH = 1 / normH;
			Hx *= invH;
			Hy *= invH;
			Hz *= invH;
			const auto invA = 1 / sqrt(Ax*Ax + Ay*Ay + Az*Az);
			Ax *= invA;
			Ay *= invA;
			Az *= invA;
		}
		
		const auto Mx = Ay*Hz - Az*Hy;
		const auto My = Az*Hx - Ax*Hz;
		const auto Mz = Ax*Hy - Ay*Hx;
		return {
			Hx, Hy, Hz,
			Mx, My, Mz,
			Ax, Ay, Az,
		};
	}
	
	/**
	 * Helper function to convert a rotation vector to a rotation matrix.
	 * 
	 * Ported from android.hardware.SensorManager.getRotationMatrixFromVector().
	 */
	constexpr Matrix getRotationMatrixFromVector(const Quaternion& rotationVector)
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

		return {
			1 - sq_q2 - sq_q3,     q1_q2 - q3_q0,     q1_q3 + q2_q0,
			    q1_q2 + q3_q0, 1 - sq_q1 - sq_q3,     q2_q3 - q1_q0,
			    q1_q3 - q2_q0,     q2_q3 + q1_q0, 1 - sq_q1 - sq_q2
		};
    }
	
	
	qreal fuseOrientationCoefficient(qreal gyro, qreal mag)
	{
		constexpr auto gyro_coefficient = qreal(0.98);
		
		// Correctly handle wrap-around
		if (gyro < -0.5 * M_PI && mag > 0.0)
			gyro += 2 * M_PI;
		else if (mag < -0.5 * M_PI && gyro > 0.0)
			mag += 2 * M_PI;
		auto result = gyro_coefficient * gyro + (1 - gyro_coefficient) * mag;
		return (result > M_PI)? result - 2.0 * M_PI : result;
	}
	
	Vector fuseOrientationCoefficient(const Vector& gyro, const Vector& mag)
	{
		return {
			fuseOrientationCoefficient(gyro[0], mag[0]),
			fuseOrientationCoefficient(gyro[1], mag[1]),
			fuseOrientationCoefficient(gyro[2], mag[2])
		};
	}
	
}  // namespace SensorHelpers


using SensorHelpers::operator*;

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
			const auto deltaRotationVector = SensorHelpers::Quaternion {
			    sinThetaOverTwo * axisX,
			    sinThetaOverTwo * axisY,
			    sinThetaOverTwo * axisZ,
			    cosThetaOverTwo
			};
			
			const auto R = SensorHelpers::getRotationMatrixFromVector(deltaRotationVector);
			
			gyro_mutex.lock();
			gyro_rotation_matrix = gyro_rotation_matrix * R;
			gyro_orientation = SensorHelpers::getOrientation(gyro_rotation_matrix);
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
		
		void filter()
		{
			if (p->accelerometer.reading() == nullptr ||
				p->magnetometer.reading() == nullptr)
				return;
			
			// Make copies of the sensor readings (and hope that the reading thread
			// does not overwrite parts of them while they are being copied)
			const auto acceleration = SensorHelpers::toVector(*p->accelerometer.reading());
			const auto geomagnetic = SensorHelpers::toVector(*p->magnetometer.reading());
			
			// Calculate orientation from accelerometer and magnetometer (acc_mag_orientation)
			const auto R = SensorHelpers::getRotationMatrix(acceleration, geomagnetic);
			if (qIsNaN(R[0]))
				return;
			
			const auto acc_mag_orientation = SensorHelpers::getOrientation(R);
			
			// If gyro not initialized yet (or we do not have a gyro):
			// use acc_mag_orientation (and initialize gyro if present)
			if (! p->gyro_available || ! p->gyro_orientation_initialized)
			{
				if (p->gyro_available)
				{
					p->gyro_orientation = acc_mag_orientation;
					p->gyro_rotation_matrix = SensorHelpers::getRotationMatrixFromOrientation(p->gyro_orientation);
					p->gyro_orientation_initialized = true;
				}
				p->latest_azimuth = qRadiansToDegrees(acc_mag_orientation[0]);
			}
			else
			{
				// Filter acc_mag_orientation and gyro_orientation to fused orientation
				p->gyro_mutex.lock();
				p->gyro_orientation = SensorHelpers::fuseOrientationCoefficient(p->gyro_orientation, acc_mag_orientation);
				p->gyro_rotation_matrix = SensorHelpers::getRotationMatrixFromOrientation(p->gyro_orientation);
				p->gyro_mutex.unlock();
				p->latest_azimuth = qRadiansToDegrees(p->gyro_orientation[0]);
			}
			
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
	SensorHelpers::Matrix gyro_rotation_matrix;
	SensorHelpers::Vector gyro_orientation;
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
