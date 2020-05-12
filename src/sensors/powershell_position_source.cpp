/*
 *    Copyright 2019-2020 Kai Pastor
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

#include "powershell_position_source.h"

#include <algorithm>
#include <iterator>

#include <Qt>
#include <QtGlobal>
#include <QtNumeric>
#include <QChar>
#include <QDateTime>
#include <QFile>
#include <QFileDevice>
#include <QGeoCoordinate>
#include <QIODevice>
#include <QStandardPaths>
#include <QStringList>  // IWYU pragma: keep


inline void initPowershellPositionResources()
{
#ifdef Q_OS_WIN
	// This resource is to be linked under Windows only.
	Q_INIT_RESOURCE(powershell_position_source);
#endif
}


namespace OpenOrienteering
{

namespace {

/**
 * A utility to continuously read fields separated by ';'
 *
 * The data is assumed to use Latin-1 encoding.
 */
class CsvFieldReader
{
private:
	const QByteArray& line;
	int pos = 0;
	
public:
	CsvFieldReader(const QByteArray& line, int pos = 0)
	: line(line)
	, pos(std::max(0, std::min(pos, line.size())))
	{}
	
	bool atEnd() const
	{
		return pos < 0;
	}
	
	QByteArray readByteArray()
	{
		auto next_pos = line.indexOf(';', pos);
		auto result = QByteArray::fromRawData(line.constData() + pos, (next_pos < 0 ? line.size() : next_pos) - pos);
		pos = next_pos > 0 ? next_pos + 1 : -1;
		return result;
	}
	
	QString readString()
	{
		auto next_pos = line.indexOf(';', pos);
		auto result = QString::fromLatin1(line.constData() + pos, (next_pos < 0 ? line.size() : next_pos) - pos);
		pos = next_pos > 0 ? next_pos + 1 : -1;
		return result;
	}
	
	template<typename T>
	QDateTime readDateTime(T format_or_spec)
	{
		return QDateTime::fromString(readString(), format_or_spec);
	}
	
	double readDouble(bool* ok = nullptr)
	{
		return readString().toDouble(ok);
	}
	
};


/**
 * Returns the main Powershell script.
 */
QByteArray startScript()
{
	QByteArray script;
	initPowershellPositionResources();
	QFile script_file(QStringLiteral(":/sensors/powershell_position_source.ps1"));
	if (script_file.open(QIODevice::ReadOnly))
		script = script_file.readAll();
	if (script_file.error() != QFileDevice::NoError)
		script.clear();
	return script;
}

/**
 * Configures the process for accessing the Windows Location API via powershell.
 */
QGeoPositionInfoSource::Error defaultSetup(QProcess& process, QByteArray& start_script, QByteArray& stop_script)
{
	start_script = startScript();
	if (start_script.isEmpty())
		return QGeoPositionInfoSource::UnknownSourceError;
	
	stop_script = QByteArrayLiteral("Exit 0 \r\n");
	
	auto const powershell_path = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
	if (powershell_path.isEmpty())
		return QGeoPositionInfoSource::UnknownSourceError;
	
	process.setProgram(powershell_path);
	process.setArguments(QStringLiteral("-NoLogo -NoProfile -NonInteractive -Command -").split(QChar::Space));
	return QGeoPositionInfoSource::NoError;
}


}  // namespace


PowershellPositionSource::PowershellPositionSource(QObject* parent)
: PowershellPositionSource(defaultSetup, parent)
{}

PowershellPositionSource::PowershellPositionSource(SetupFunction& setup, QObject* parent)
: QGeoPositionInfoSource(parent)
{
	setError(setup(powershell, start_script, stop_script));
	if (position_error != QGeoPositionInfoSource::NoError)
		return;
	
	powershell.setReadChannel(QProcess::StandardOutput);
	
	connect(&powershell, &QProcess::stateChanged, this, &PowershellPositionSource::powershellStateChanged);
	connect(&powershell, &QProcess::readyReadStandardOutput, this, &PowershellPositionSource::readStandardOutput);
	connect(&powershell, &QProcess::readyReadStandardError, this, &PowershellPositionSource::readStandardError);
	
	periodic_update_timer.setSingleShot(true);
	connect(&periodic_update_timer, &QTimer::timeout, this, &PowershellPositionSource::periodicUpdateTimeout);
	
	single_update_timer.setSingleShot(true);
	connect(&single_update_timer, &QTimer::timeout, this, &PowershellPositionSource::singleUpdateTimeout);
}

PowershellPositionSource::~PowershellPositionSource()
{
	powershell.disconnect(this);
	if (powershell.state() != QProcess::NotRunning)
		powershell.kill();
	powershell.waitForFinished(100);
}


QGeoPositionInfoSource::Error PowershellPositionSource::error() const
{
	return position_error;
}

void PowershellPositionSource::setError(QGeoPositionInfoSource::Error value)
{
	if (this->position_error == value)
		return;
	
	this->position_error = value;
	switch (value)
	{
	case NoError:
		break;
	case AccessError:
		emit this->QGeoPositionInfoSource::error(value);
		// Exit gracefully
		powershell.write(stop_script);
		break;
	default:
		emit this->QGeoPositionInfoSource::error(value);
		break;
	}
}


QGeoPositionInfo PowershellPositionSource::lastKnownPosition(bool /* satellite_only */) const
{
	return last_position;
}

QGeoPositionInfoSource::PositioningMethods PowershellPositionSource::supportedPositioningMethods() const
{
	switch (position_error)
	{
	case NoError:
		return AllPositioningMethods;
	default:
		return NoPositioningMethods;
	}
}

int PowershellPositionSource::minimumUpdateInterval() const
{
	return 1000;
}

// slot
void PowershellPositionSource::startUpdates()
{
	if (!startPowershell())
		return;
	
	updates_ongoing = true;
	// Grant some extra delay for the update to arrive.
	periodic_update_timer.setInterval(std::max(1500, updateInterval()));
	periodic_update_timer.start();
}

// slot
void PowershellPositionSource::stopUpdates()
{
	if (!updates_ongoing)
		return;
	
	updates_ongoing = false;
	periodic_update_timer.stop();
	// Exit gracefully
	powershell.write(stop_script);
}

// slot
void PowershellPositionSource::requestUpdate(int timeout)
{
	if (!startPowershell())
		return;
	
	setError(QGeoPositionInfoSource::NoError);
	if (timeout == 0)
	{
		timeout = 120000; // 2 min for cold start
	}
	else if (timeout < minimumUpdateInterval())
	{
		emit updateTimeout();
		return;
	}
	
	single_update_timer.start(timeout);
}


bool PowershellPositionSource::startPowershell()
{
	if (powershell.state() != QProcess::NotRunning)
		return true;
	
	powershell.start();
	if (powershell.state() == QProcess::NotRunning)
	{
		setError(QGeoPositionInfoSource::UnknownSourceError);
		return false;
	}
	
	return true;
}

void PowershellPositionSource::powershellStateChanged(QProcess::ProcessState new_state)
{
	switch (new_state)
	{
	case QProcess::Starting:
		// nothing
		break;
	case QProcess::Running:
		powershell.write(start_script);
		break;
	case QProcess::NotRunning:
		updates_ongoing = false;
		periodic_update_timer.stop();
		single_update_timer.stop();
		if (powershell.exitStatus() != QProcess::NormalExit)
		{
			setError(UnknownSourceError);
		}
		break;
	}
}

void PowershellPositionSource::readStandardError()
{
	auto const output = powershell.readAllStandardError();
	qWarning("PowershellPositionSource: Error output:\r\n%s", output.data());
}

void PowershellPositionSource::readStandardOutput()
{
	while (powershell.canReadLine())
	{
		auto const line = powershell.readLine(100).trimmed();
		if (line.startsWith("Position;"))
			processPosition(line);
		else if (line.startsWith("Status;"))
			processStatus(line);
		else if (line.startsWith("Permission;"))
			processPermission(line);
		else if (!line.isEmpty())
			qDebug("PowershellPositionSource: Bogus data: '%s'", line.data());
	}
}

void PowershellPositionSource::processPosition(const QByteArray& line)
{
	Q_ASSERT(line.startsWith("Position;"));
	auto reader = CsvFieldReader(line, 9);
	auto const date_time  = reader.readDateTime(Qt::ISODate);
	bool numbers_ok[5];
	auto latitude   = reader.readDouble(&numbers_ok[0]);
	auto longitude  = reader.readDouble(&numbers_ok[1]);
	auto altitude   = reader.readDouble(&numbers_ok[2]);
	auto const h_accuracy = reader.readDouble(&numbers_ok[3]);
	auto const v_accuracy = reader.readDouble(&numbers_ok[4]);
	
	using std::begin; using std::end;
	if (std::find(begin(numbers_ok), end(numbers_ok), false) != end(numbers_ok)
	    || !date_time.isValid())
	{
		qDebug("PowershellPositionSource: Could not parse location '%s'", line.data());
		setError(UnknownSourceError);
		return;
	}
	
	if (qIsNaN(h_accuracy))
	{
		qDebug("PowershellPositionSource: Horizontal accuracy unknown");
		return;
	}
	
	auto geo_coord = QGeoCoordinate{latitude, longitude};
	if (!qIsNaN(v_accuracy))
		geo_coord.setAltitude(altitude);
	
	auto position = QGeoPositionInfo{geo_coord, date_time};
	position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, h_accuracy);
	if (!qIsNaN(v_accuracy))
		position.setAttribute(QGeoPositionInfo::VerticalAccuracy, v_accuracy);
	
	nativePositionUpdate(position);
}

void PowershellPositionSource::processStatus(const QByteArray& line)
{
	Q_ASSERT(line.startsWith("Status;"));
	auto reader = CsvFieldReader(line, 7);
	auto value = reader.readByteArray();
	// Cf. https://docs.microsoft.com/en-US/dotnet/api/system.device.location.geopositionstatus
	if (value == "Disabled")
	{
		setError(AccessError);
	}
	else if (value == "Initializing")
	{
		setError(NoError);
	}
	else if (value == "NoData")
	{
		setError(UnknownSourceError);
	}
	else if (value == "Ready")
	{
		setError(NoError);
	}
	else
	{
		qDebug("PowershellPositionSource: Unknown status '%s'", line.data()+7);
		setError(UnknownSourceError);
	}
}

void PowershellPositionSource::processPermission(const QByteArray& line)
{
	Q_ASSERT(line.startsWith("Permission;"));
	auto reader = CsvFieldReader(line, 11);
	auto value = reader.readByteArray();
	// Cf. https://docs.microsoft.com/en-US/dotnet/api/system.device.location.geopositionpermission
	if (value == "Denied")
	{
		setError(AccessError);
	}
	else if (value == "Granted")
	{
		// no action
	}
	else if (value == "Unknown")
	{
		// no action
	}
	else
	{
		qDebug("PowershellPositionSource: Unknown permission '%s'", line.data()+11);
		// nothing else
	}
}

void PowershellPositionSource::nativePositionUpdate(const QGeoPositionInfo& position)
{
	periodic_update_timer.stop();
	single_update_timer.stop();
	last_position = position;
	setError(NoError);
	emit positionUpdated(last_position);
	
	if (updates_ongoing)
		periodic_update_timer.start();
}

void PowershellPositionSource::periodicUpdateTimeout()
{
	if (last_position.isValid())
	{
		// Emit virtual position update
		last_position.setTimestamp(last_position.timestamp().addMSecs(updateInterval()));
		emit positionUpdated(last_position);
	}
	periodic_update_timer.start();
}

void PowershellPositionSource::singleUpdateTimeout()
{
	emit updateTimeout();
}

}  // namespace OpenOrienteering
