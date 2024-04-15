/*
 *    Copyright 2013-2015 Kai Pastor
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

#ifndef OPENORIENTEERING_XML_BENCHMARK_T_H
#define OPENORIENTEERING_XML_BENCHMARK_T_H

#include <QBuffer>
#include <QObject>

#include "core/map_coord.h"

class QXmlStreamWriter;

namespace OpenOrienteering {


/**
 * @test Benchmarks different implementations of saving and loading
 *       sequences of coordinates.
 */
class CoordXmlTest : public QObject
{
Q_OBJECT
	
private slots:
	/** Initialization. */
	void initTestCase();
	
	/** Writes rich XML. */
	void writeXml();
	void writeXml_data();
	
	/** Writes a simple human readable format by means of QTextStream. */
	void writeHumanReadableStream();
	void writeHumanReadableStream_data();
	
	/** Writes a simple human readable format by means of QString. */
	void writeHumanReadableString();
	void writeHumanReadableString_data();
	
	/** Writes a dense base64-like format. */
	void writeCompressed();
	void writeCompressed_data();
	
	/** Calls the actual fast implementation from MapCoord. */
	void writeFastImplementation();
	void writeFastImplementation_data();
	
	/** Reads rich XML. */
	void readXml();
	void readXml_data();
	
	/** Reads a simple human readable format by means of QTextStream. */
	void readHumanReadableStream();
	void readHumanReadableStream_data();
	
	/** Reads a simple human readable format by means of QStringRef. */
	void readHumanReadableStringRef();
	void readHumanReadableStringRef_data();
	
	/** Reads a dense base64-like format. */
	void readCompressed();
	void readCompressed_data();
	
	/** Calls the actual fast implementation from MapCoord. */
	void readFastImplementation();
	void readFastImplementation_data();
	
private:
	/** The common test data setup. */
	void common_data();
	
	/** The actual implementation of writing rich XML. */
	void writeXml_implementation(MapCoordVector& coords, QXmlStreamWriter& xml);
	
	/** The actual implementation of writing dense human-readable text.
	 *  This implementation is based on QTextStream. */
	void writeHumanReadableStream_implementation(MapCoordVector& coords, QXmlStreamWriter& xml);
	
	/** The actual implementation of writing dense human-readable text.
	 *  This implementation is based on QString. */
	void writeHumanReadableString_implementation(MapCoordVector& coords, QXmlStreamWriter& xml);
	
	/** The actual implementation of writing dense base64-like text. */
	void writeCompressed_implementation(MapCoordVector& coords, QXmlStreamWriter& xml);
	
	/** Compares all coords members to the expected MapCoord.
	 *  Returns true if all match, false otherwise. */
	bool compare_all(MapCoordVector& coords, MapCoord& expected) const;
	
	MapCoord proto_coord;
	QBuffer buffer;
};


}  // namespace OpenOrienteering

#endif
