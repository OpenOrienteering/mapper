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

#ifndef OPENORIENTEERING_POWERSHELL_POSITION_SOURCE_H
#define OPENORIENTEERING_POWERSHELL_POSITION_SOURCE_H

#include <QByteArray>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

namespace OpenOrienteering
{

/**
 * A Windows position source based on Powershell's access to Windows.Device.Location.
 * 
 * @see QGeoPositionInfoSourceWinRT
 */
class PowershellPositionSource : public QGeoPositionInfoSource
{
	Q_OBJECT
	
public:
	/**
	 * A setup function configures the process which this source reads from,
	 * the start script and the stop script.
	 */
	using SetupFunction = QGeoPositionInfoSource::Error (QProcess&, QByteArray&, QByteArray&);
	
	
	/**
	 * Constructs a source with the default Powershell script.
	 */
	PowershellPositionSource(QObject* parent = nullptr);
	
	/**
	 * Constructs a source with the given Powershell script.
	 */
	PowershellPositionSource(SetupFunction& setup, QObject* parent = nullptr);
	
	PowershellPositionSource(const PowershellPositionSource&) = delete;
	PowershellPositionSource(PowershellPositionSource&&) = delete;
	PowershellPositionSource& operator=(const PowershellPositionSource&) = delete;
	PowershellPositionSource& operator=(PowershellPositionSource&&) = delete;
	
	~PowershellPositionSource() override;
	
	
	QGeoPositionInfoSource::Error error() const override;
	
	using QGeoPositionInfoSource::error;  // the signal

private:
	/**
	 * Sets the error and emits the error signal (unless NoError).
	 */
	void setError(QGeoPositionInfoSource::Error value);


public:
	QGeoPositionInfo lastKnownPosition(bool satellite_only) const override;
	
	PositioningMethods supportedPositioningMethods() const override;
	
	int minimumUpdateInterval() const override;
	

public slots:
	void startUpdates() override;
	
	void stopUpdates() override;
	
	void requestUpdate(int timeout) override;
	
private:
	bool startPowershell();
	
	void powershellStateChanged(QProcess::ProcessState new_state);
	
	void readStandardError();
	
	void readStandardOutput();
	
	void processPosition(const QByteArray& line);
	
	void processStatus(const QByteArray& line);
	
	void processPermission(const QByteArray& line);
	
	void nativePositionUpdate(const QGeoPositionInfo& position);
	

	void periodicUpdateTimeout();
	
	void singleUpdateTimeout();
	
	QProcess powershell;
	QByteArray start_script;
	QByteArray stop_script;
	QGeoPositionInfo last_position;
	QTimer periodic_update_timer;
	QTimer single_update_timer;
	Error position_error = NoError;
	bool updates_ongoing = false;
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_POWERSHELL_POSITION_SOURCE_H
