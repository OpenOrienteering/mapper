/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2014, 2019 Kai Pastor
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

#include <cmath>

#include <QtMath>  // IWYU pragma: keep
#include <QtGlobal>  // IWYU pragma: keep
#include <QMetaMethod>
#include <QMetaObject>
#include <QMutex>  // IWYU pragma: keep
#include <QTime>

#ifdef QT_SENSORS_LIB

#include <QThread>
#include <QDebug>
#include <QWaitCondition>
#include <QtSensors/QAccelerometer>
#include <QtSensors/QMagnetometer>
#include <QtSensors/QGyroscope>

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniObject>
#endif

#endif  // # QT_SENSORS_LIB


// clazy:excludeall=missing-qobject-macro


namespace OpenOrienteering {

#ifdef QT_SENSORS_LIB
	
namespace SensorHelpers {
	
	void matrixMultiplication(float* A, float* B, float* result)
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
	void getOrientation(float* R, float* values)
	{
		/*
		* / R[ 0] R[ 1] R[ 2] \
		* | R[ 3] R[ 4] R[ 5] |
		* \ R[ 6] R[ 7] R[ 8] /
		*/
		values[0] = (float)atan2(R[1], R[4]);
		values[1] = (float)asin(-R[7]);
		values[2] = (float)atan2(-R[6], R[8]);
	}
	
	/** From http://www.thousand-thoughts.com/2012/03/android-sensor-fusion-tutorial/2/
	 *  Should be optimized ... */
	void getRotationMatrixFromOrientation(float* orientation, float* result)
	{
		float xM[9];
		float yM[9];
		float zM[9];

		float sinX = (float)qSin(orientation[1]);
		float cosX = (float)qCos(orientation[1]);
		float sinY = (float)qSin(orientation[2]);
		float cosY = (float)qCos(orientation[2]);
		float sinZ = (float)qSin(orientation[0]);
		float cosZ = (float)qCos(orientation[0]);

		// rotation about x-axis (pitch)
		xM[0] = 1.0f; xM[1] = 0.0f; xM[2] = 0.0f;
		xM[3] = 0.0f; xM[4] = cosX; xM[5] = sinX;
		xM[6] = 0.0f; xM[7] = -sinX; xM[8] = cosX;

		// rotation about y-axis (roll)
		yM[0] = cosY; yM[1] = 0.0f; yM[2] = sinY;
		yM[3] = 0.0f; yM[4] = 1.0f; yM[5] = 0.0f;
		yM[6] = -sinY; yM[7] = 0.0f; yM[8] = cosY;

		// rotation about z-axis (azimuth)
		zM[0] = cosZ; zM[1] = sinZ; zM[2] = 0.0f;
		zM[3] = -sinZ; zM[4] = cosZ; zM[5] = 0.0f;
		zM[6] = 0.0f; zM[7] = 0.0f; zM[8] = 1.0f;

		// rotation order is y, x, z (roll, pitch, azimuth)
		float temp[9];
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
		const float normH = (float)sqrt(Hx*Hx + Hy*Hy + Hz*Hz);
		if (normH < 0.1f) {
			// device is close to free fall (or in space?), or close to
			// magnetic north pole. Typical values are > 100.
			return false;
		}
		const float invH = 1.0f / normH;
		Hx *= invH;
		Hy *= invH;
		Hz *= invH;
		const float invA = 1.0f / (float)sqrt(Ax*Ax + Ay*Ay + Az*Az);
		Ax *= invA;
		Ay *= invA;
		Az *= invA;
		const float Mx = Ay*Hz - Az*Hy;
		const float My = Az*Hx - Ax*Hz;
		const float Mz = Ax*Hy - Ay*Hx;
		
		R[0] = Hx; R[1] = Hy; R[2] = Hz;
		R[3] = Mx; R[4] = My; R[5] = Mz;
		R[6] = Ax; R[7] = Ay; R[8] = Az;
		
// 		if (I != null) {
// 			// compute the inclination matrix by projecting the geomagnetic
// 			// vector onto the Z (gravity) and X (horizontal component
// 			// of geomagnetic vector) axes.
// 			const float invE = 1.0f / (float)std::sqrt(Ex*Ex + Ey*Ey + Ez*Ez);
// 			const float c = (Ex*Mx + Ey*My + Ez*Mz) * invE;
// 			const float s = (Ex*Ax + Ey*Ay + Ez*Az) * invE;
// 			I[0] = 1; I[1] = 0; I[2] = 0;
// 			I[3] = 0; I[4] = c; I[5] = s;
// 			I[6] = 0; I[7] =-s; I[8] = c;
// 		}
		return true;
	}
	
	/**
	 * Ported from android.hardware.SensorManager.getRotationMatrixFromVector().
	 * 
	 * Helper function to convert a rotation vector to a rotation matrix.
	 */
    void getRotationMatrixFromVector(float* R, float* rotationVector)
	{
        float q0;
        float q1 = rotationVector[0];
        float q2 = rotationVector[1];
        float q3 = rotationVector[2];

//        if (rotationVector.length == 4) {
            q0 = rotationVector[3];
//        } else {
//            q0 = 1 - q1*q1 - q2*q2 - q3*q3;
//            q0 = (q0 > 0) ? (float)std::sqrt(q0) : 0;
//        }

        float sq_q1 = 2 * q1 * q1;
        float sq_q2 = 2 * q2 * q2;
        float sq_q3 = 2 * q3 * q3;
        float q1_q2 = 2 * q1 * q2;
        float q3_q0 = 2 * q3 * q0;
        float q1_q3 = 2 * q1 * q3;
        float q2_q0 = 2 * q2 * q0;
        float q2_q3 = 2 * q2 * q3;
        float q1_q0 = 2 * q1 * q0;

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
	
	
}  // namespace OpenOrienteering



class CompassPrivate : public QGyroscopeFilter
{
public:
	CompassPrivate(Compass* compass)
	 : thread(this)
	 , compass(compass)
	 , enabled(false)
	 , latest_azimuth(-1)
	{
		// Try to filter out non-gravity sources of acceleration
		accelerometer.setAccelerationMode(QAccelerometer::Gravity);

		// Try to filter out local magnetic interference
		magnetometer.setReturnGeoValues(true);
		
		// Check if a gyroscope is available
		gyro_available = ! QSensor::sensorsForType(QGyroscope::type).empty();
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
	
	float getLatestAzimuth()
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
			const float EPSILON = 1e-9f;
			
			const float microseconds_to_seconds = 1.0f / (1000*1000);
			const float dt = (reading->timestamp() - last_gyro_timestamp) * microseconds_to_seconds;
			// Axis of the rotation sample, not normalized yet.
			const float deg_to_rad = M_PI / 180.0f;
			float axisX = deg_to_rad * reading->x();
			float axisY = deg_to_rad * reading->y();
			float axisZ = deg_to_rad * reading->z();

			// Calculate the angular speed of the sample
			float omegaMagnitude = sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);

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
			float thetaOverTwo = omegaMagnitude * dt / 2.0f;
			float sinThetaOverTwo = qSin(thetaOverTwo);
			float cosThetaOverTwo = qCos(thetaOverTwo);
			float deltaRotationVector[4];
			deltaRotationVector[0] = sinThetaOverTwo * axisX;
			deltaRotationVector[1] = sinThetaOverTwo * axisY;
			deltaRotationVector[2] = sinThetaOverTwo * axisZ;
			deltaRotationVector[3] = cosThetaOverTwo;
			
			float R[9];
			SensorHelpers::getRotationMatrixFromVector(R, deltaRotationVector);
			
			float temp[9];
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
		 : keep_running(true)
		 , p(p)
		{
		}
		
		float fuseOrientationCoefficient(float gyro, float acc_mag)
		{
			const float FILTER_COEFFICIENT = 0.98f;
			const float ONE_MINUS_COEFF = 1.0f - FILTER_COEFFICIENT;
			
			float result;
			// Correctly handle wrap-around
            if (gyro < -0.5 * M_PI && acc_mag > 0.0) {
            	result = (float) (FILTER_COEFFICIENT * (gyro + 2.0 * M_PI) + ONE_MINUS_COEFF * acc_mag);
        		result -= (result > M_PI) ? 2.0 * M_PI : 0;
            }
            else if (acc_mag < -0.5 * M_PI && gyro > 0.0) {
            	result = (float) (FILTER_COEFFICIENT * gyro + ONE_MINUS_COEFF * (acc_mag + 2.0 * M_PI));
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
			
			// Calculate orientation from accelerometer and magnetometer (acc_mag_orientation)
			float R[9];
			bool ok = SensorHelpers::getRotationMatrix(R, acceleration, geomagnetic);
			if (!ok)
				return;
			
			float acc_mag_orientation[3];
			SensorHelpers::getOrientation(R, acc_mag_orientation);
			
			// If gyro not initialized yet (or we do not have a gyro):
			// use acc_mag_orientation (and initialize gyro if present)
			float azimuth;
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
				float fused_orientation[3];
				fused_orientation[0] = fuseOrientationCoefficient(p->gyro_orientation[0], acc_mag_orientation[0]);
				fused_orientation[1] = fuseOrientationCoefficient(p->gyro_orientation[1], acc_mag_orientation[1]);
				fused_orientation[2] = fuseOrientationCoefficient(p->gyro_orientation[2], acc_mag_orientation[2]);
				
				// Write back fused_orientation to gyro_orientation
				SensorHelpers::getRotationMatrixFromOrientation(fused_orientation, p->gyro_rotation_matrix);
				memcpy(p->gyro_orientation, fused_orientation, 3 * sizeof(float));
				p->gyro_mutex.unlock();
				
				azimuth = fused_orientation[0];
			}
			
			p->latest_azimuth = 180 * azimuth / M_PI;
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
		bool keep_running;
		CompassPrivate* p;
	};
	
	QAccelerometer accelerometer;
	QMagnetometer magnetometer;
	QGyroscope gyroscope;
	bool gyro_available;
	float gyro_orientation[3];
	float gyro_rotation_matrix[9];
	quint64 last_gyro_timestamp;
	bool gyro_orientation_initialized;
	QMutex gyro_mutex;
	
	SensorThread thread;
	Compass* compass;
	
	bool enabled;
	float latest_azimuth;
};
#endif


// Emit vtable once, in this translation unit
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
	{
		p.reset(new CompassPrivate(this));
		p->enable(true);
	}
#endif
}

void Compass::stopUsage()
{
	-- reference_counter;
#ifdef QT_SENSORS_LIB
	if (reference_counter == 0)
	{
		p->enable(false);
		p.reset();
	}
#endif
}

float Compass::getCurrentAzimuth()
{
#ifdef QT_SENSORS_LIB
	return p ? p->getLatestAzimuth() : 0.0f;
#elif MAPPER_DEVELOPMENT_BUILD
	// DEBUG: rotate around ...
	QTime now = QTime::currentTime();
	return 360 * (now.msecsSinceStartOfDay() % (10 * 1000)) / (float)(10 * 1000);
#else
	return 0.0f;
#endif
}

void Compass::connectToAzimuthChanges(const QObject* receiver, const char* slot)
{
#ifdef QT_SENSORS_LIB
	connect(this, SIGNAL(azimuthChanged(float)), receiver, slot, Qt::QueuedConnection);
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

void Compass::emitAzimuthChanged(float value)
{
	emit azimuthChanged(value);
}


}  // namespace OpenOrienteering
