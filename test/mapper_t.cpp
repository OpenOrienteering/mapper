/*
 *    Copyright 2016 Kai Pastor
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

/*
 * The following definitions are required for compilation:
 * CMAKE_COMMAND:   Path to the cmake executable
 * MAPPER_COMMAND:  Path to the Mapper executable
 */

#include <QProcess>
#include <QString>

int main(int, char**)
{
	QProcess tests;
	tests.setProgram(QStringLiteral(CMAKE_COMMAND));
	tests.setArguments({QStringLiteral("-P"), QStringLiteral("AUTORUN_TESTS.cmake")});
	tests.setProcessChannelMode(QProcess::ForwardedChannels);
	tests.start();
	tests.waitForFinished(-1);
	int result = tests.exitCode();
	
	if (!result)
	{
		QProcess mapper;
		mapper.setProgram(QStringLiteral(MAPPER_LOCATION));
		mapper.setArguments({});
		mapper.setProcessChannelMode(QProcess::ForwardedChannels);
		mapper.start();
		mapper.waitForFinished(-1);
		result = mapper.exitCode();
	}
	
	return result;
}
