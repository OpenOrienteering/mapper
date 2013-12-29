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

#include "gps_source_android.h"

#include <QDebug>
#include <jni.h>
#include <QtAndroidExtras/QAndroidJniObject>


Q_DECLARE_METATYPE(QGeoPositionInfo);

AndroidGPSPositionSource* android_gps_instance = NULL;

AndroidGPSPositionSource::AndroidGPSPositionSource(QObject* parent)
 : QGeoPositionInfoSource(parent)
{
	if (android_gps_instance == NULL)
		qRegisterMetaType<QGeoPositionInfo>();
	
	android_gps_instance = this;
	has_error = false;
	updates_started = false;
}

QGeoPositionInfo AndroidGPSPositionSource::lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const
{
	Q_UNUSED(fromSatellitePositioningMethodsOnly);
	return lastPosition;
}

QGeoPositionInfoSource::PositioningMethods AndroidGPSPositionSource::supportedPositioningMethods() const
{
	return QGeoPositionInfoSource::SatellitePositioningMethods;
}

int AndroidGPSPositionSource::minimumUpdateInterval() const
{
	// TODO?
	return 0;
}

QGeoPositionInfoSource::Error AndroidGPSPositionSource::error() const
{
	// TODO?
	return has_error ? QGeoPositionInfoSource::UnknownSourceError : QGeoPositionInfoSource::NoError;
}

void AndroidGPSPositionSource::positionUpdatedCallback(float latitude, float longitude, float altitude, float horizontal_stddev)
{
	has_error = false;
	
	QDateTime timestamp = QDateTime::currentDateTime();
	QGeoCoordinate coordinate(latitude, longitude, altitude);
	QGeoPositionInfo position_info(coordinate, timestamp);
	position_info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, horizontal_stddev);
	lastPosition = position_info;
	
	emit positionUpdated(position_info);
}

void AndroidGPSPositionSource::setError()
{
	has_error = true;
	emit QGeoPositionInfoSource::error(QGeoPositionInfoSource::UnknownSourceError);
}

void AndroidGPSPositionSource::setUpdateTimeout()
{
	emit updateTimeout();
}

void AndroidGPSPositionSource::startUpdates()
{
	if (updates_started)
		return;
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
                                       "startGPSUpdates",
                                       "(I)V",
                                       updateInterval());
	updates_started = true;
}

void AndroidGPSPositionSource::stopUpdates()
{
	if (!updates_started)
		return;
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
                                       "stopGPSUpdates",
                                       "()V");
	updates_started = false;
}

void AndroidGPSPositionSource::requestUpdate(int timeout)
{
	Q_UNUSED(timeout);

	// TODO: not implemented!
	qDebug() << "AndroidGPSPositionSource::requestUpdate() called which is not implemented!";
}


// Android JNI callbacks

extern "C" {

JNIEXPORT void JNICALL Java_org_openorienteering_mapper_MapperActivity_positionUpdated(JNIEnv * env, jobject obj, jfloat latitude, jfloat longitude, jfloat altitude, jfloat horizontal_stddev)
{
	Q_UNUSED(env);
	Q_UNUSED(obj);
	android_gps_instance->positionUpdatedCallback(latitude, longitude, altitude, horizontal_stddev);
}

JNIEXPORT void JNICALL Java_org_openorienteering_mapper_MapperActivity_error(JNIEnv * env, jobject obj)
{
	Q_UNUSED(env);
	Q_UNUSED(obj);
	android_gps_instance->setError();
}

JNIEXPORT void JNICALL Java_org_openorienteering_mapper_MapperActivity_updateTimeout(JNIEnv * env, jobject obj)
{
	Q_UNUSED(env);
	Q_UNUSED(obj);
	android_gps_instance->setUpdateTimeout();
}

}
