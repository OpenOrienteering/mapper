/*
 *    Copyright 2020 Kai Pastor
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

#include "nmea_position_plugin.h"

#include <QtGlobal>
#include <QByteArray>
#include <QFileInfo>
#include <QGeoPositionInfoSource>
#include <QLatin1Char>
#include <QList>
#include <QNmeaPositionInfoSource>
#include <QProcess>

#ifdef QT_SERIALPORT_LIB
#  include <QSerialPortInfo>
#endif


namespace OpenOrienteering
{

/**
 * A position info source which reads NMEA from an arbitrary file or device.
 * 
 * In order to not block when opening devices such as FIFOs, and to provide the
 * signals required by QNmeaPositionInfoSource, this class uses QProcess to run
 * `/bin/cat` on the selected device. `/bin/cat` must exist on Linux systems,
 * according to the FHS, and it is available on macOS, too.
 * 
 * The serial port device is taken from the QT_NMEA_SERIAL_PORT environment
 * variable which is also used by Qt's serialnmea plugin. In OpenOrienteering
 * Mapper, this variable is set by the GPSDisplay class if the user explicitly
 * chooses a setting different from "Default".
 * 
 * Unlike Qt's serialnmea, this plugin accepts the absolute path of any readable
 * file via QT_NMEA_SERIAL_PORT. When compiled with Qt SerialPort, the name of a
 * serial port can be used, too.
 */
class NmeaPositionSource : public QNmeaPositionInfoSource
{
public:
	~NmeaPositionSource() override
	{
		if (process.state() == QProcess::Running)
			process.kill();
		process.waitForFinished(100);
	}
	
	explicit NmeaPositionSource(QObject* parent)
	: QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, parent)
	{
		auto program = QString::fromLatin1("/bin/cat");
		if (!QFileInfo::exists(program))
		{
			setError(QGeoPositionInfoSource::UnknownSourceError);
			return;
		}
		process.setProgram(program);
		process.setArguments({devicePath()});
		setDevice(&process);
		if (process.state() != QProcess::NotRunning)
			process.terminate();
	}
	
	void startUpdates() override
	{
		auto device = devicePath();
		if (!QFileInfo::exists(device))
		{
			setError(QGeoPositionInfoSource::UnknownSourceError);
			return;
		}
		if (!QFileInfo(device).isReadable())
		{
			setError(QGeoPositionInfoSource::AccessError);
			return;
		}
		process.setArguments({device});
		if (process.state() == QProcess::NotRunning)
			process.start();
		QNmeaPositionInfoSource::startUpdates();
	}
	
	void stopUpdates() override
	{
		QNmeaPositionInfoSource::stopUpdates();
		if (process.state() == QProcess::Running)
			process.terminate();
	}
	
	QString devicePathFromEnvironment() const
	{
		auto device =  QString::fromUtf8(qgetenv("QT_NMEA_SERIAL_PORT"));
#ifdef QT_SERIALPORT_LIB
		if (!device.startsWith(QLatin1Char('/')))
			device = QSerialPortInfo(device).systemLocation();
#endif
		return device;
	}
	
	QString defaultDevicePath() const
	{
		auto device = QString {};
#ifdef QT_SERIALPORT_LIB
		auto const ports = QSerialPortInfo::availablePorts();
		if (!ports.empty())
			device = ports.front().systemLocation();
#endif
		if (device.isEmpty())
			device = QString::fromLatin1("/dev/null");
		return device;
	}
	
	QString devicePath() const
	{
		auto device = devicePathFromEnvironment();
		if (device.isEmpty())
			device = defaultDevicePath();
		qDebug("NmeaPositionSource device: %s", qPrintable(device));
		return device;
	}

	QGeoPositionInfoSource::Error error() const override
	{
		return position_error;
	}
	
	using QGeoPositionInfoSource::error;  // the signal

protected:
	/**
	 * Sets the error and emits the error signal (unless NoError).
	 */
	void setError(QGeoPositionInfoSource::Error value)
	{
		if (this->position_error == value)
			return;
		
		this->position_error = value;
		switch (value)
		{
		case NoError:
			break;
		default:
			emit this->QGeoPositionInfoSource::error(value);
			break;
		}
	}
	
private:
	QProcess process;
	Error position_error = NoError;
};



// virtual
NmeaPositionPlugin::~NmeaPositionPlugin() = default;

NmeaPositionPlugin::NmeaPositionPlugin(QObject* parent)
: QObject(parent)
{}


// virtual
QGeoAreaMonitorSource* NmeaPositionPlugin::areaMonitor(QObject* /* parent */)
{
	return nullptr;
}

// virtual
QGeoPositionInfoSource* NmeaPositionPlugin::positionInfoSource(QObject* parent)
{
	return new NmeaPositionSource(parent);
}

// virtual
QGeoSatelliteInfoSource* NmeaPositionPlugin::satelliteInfoSource(QObject* /* parent */)
{
	return nullptr;
}

}  // namespace OpenOrienteering
