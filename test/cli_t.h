/*
 *    Copyright 2026 Michael Behrens
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

#ifndef OPENORIENTEERING_CLI_T_H
#define OPENORIENTEERING_CLI_T_H

#include <QObject>


class CliTest : public QObject
{
Q_OBJECT

private slots:
	void initTestCase();

	void testNoSubcommand();
	void testNonexistingSubcommand();
	void testExportMissingInput();
	void testExportMissingOutput();
	void testExportInvalidInput();
	void testExportPdf();
	void testExportPng();
	void testExportFullMapFlag();

	void testConvertMissingInput();
	void testConvertMissingOutput();
	void testConvertInvalidInput();
	void testConvertOmapToXmap();
	void testConvertXmapToOmap();
	void testConvertOmapToOcd();
	void testConvertWithFormatId();
	void testExportFormatIdOverridesExtension();
	void testConvertFormatIdOverridesExtension();
	void testExportRejectsNativeFormatId();
	void testConvertRejectsUnknownFormatId();
};

#endif
