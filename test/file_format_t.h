/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2021, 2024, 2025 Kai Pastor
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

#ifndef OPENORIENTEERING_FILE_FORMAT_T_H
#define OPENORIENTEERING_FILE_FORMAT_T_H

#include <QObject>
#include <QString>


/**
 * @test Tests concerning the file formats, registry, import and export.
 * 
 */
class FileFormatTest : public QObject
{
Q_OBJECT
	
private slots:
	void initTestCase();
	
	/**
	 * Tests the MapCoord::toString() implementation which is used for export.
	 */
	void mapCoordtoString();
	void mapCoordtoString_data();
	
	/**
	 * Tests the MapCoord::fromString() implementation which is used for import.
	 */
	void mapCoordFromString();
	void mapCoordFromString_data();
	
	/**
	 * Tests filename extension fixup.
	 */
	void fixupExtensionTest();
	void fixupExtensionTest_data();
	
	/**
	 * Tests FileFormat::understands() implementations.
	 */
	void understandsTest();
	void understandsTest_data();
	
	/**
	 * Tests FileFormatRegistry::formatForData().
	 */
	void formatForDataTest();
	void formatForDataTest_data();
	
	/**
	 * Tests that high coordinates are correctly moved to the central region
	 * of the map.
	 * 
	 * \see issue #513
	 */
	void issue_513_high_coordinates();
	void issue_513_high_coordinates_data();
	
	/**
	 * Tests that maps contain the same information before and after saving
     * them and loading them again.
     */
	void saveAndLoad();
	void saveAndLoad_data();
	
	/**
	 * Tests saving and loading a map which is created in memory and does not go
	 * through an implicit export-import-cycle before the test.
	 */
	void pristineMapTest();
	
	/**
	 * Tests export of geospatial vector data via OGR.
	 */
	void ogrExportTest();
	void ogrExportTest_data();
	
	/**
	 * Tests the export of KML courses.
	 */
	void kmlCourseExportTest();
	
	/**
	 * Tests the export of IOF courses.
	 */
	void iofCourseExportTest();
	
	/**
	 * Test the creation of templates.
	 */
	void importTemplateTest_data();
	void importTemplateTest();
	
	/**
	 * Test text export to OCD.
	 */
	void ocdTextExportTest_data();
	void ocdTextExportTest();
	
	/**
	 * Test text import from OCD.
	 */
	void ocdTextImportTest_data();
	void ocdTextImportTest();
	
	/**
	 * Test path import from OCD.
	 */
	void ocdPathImportTest_data();
	void ocdPathImportTest();
	
	/**
	 * Test OCD area symbol export and import.
	 */
	void ocdAreaSymbolTest_data();
	void ocdAreaSymbolTest();

};

#endif // OPENORIENTEERING_FILE_FORMAT_T_H
