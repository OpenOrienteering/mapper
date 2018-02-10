/*
 *    Copyright 2012, 2013 Jan Dalheimer
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

#include "dxfparser.h"

#include <QApplication>
#include <QBuffer>
#include <QDebug>


namespace OpenOrienteering {

QString DXFParser::parse()
{
	Q_ASSERT(device); // Programmer's responsibility
	
	bool must_close_device = false;
	if (!device->isOpen())
	{
		if (!device->open(QIODevice::ReadOnly))
		{
			return QApplication::translate("OpenOrienteering::DXFParser", "Could not open the file.");
		}
		must_close_device = true;
	}
	
	int code;
	QString value;
	
	QByteArray head(device->peek(16));
	QBuffer buffer(&head);
	buffer.open(QIODevice::ReadOnly);
	readNextCodeValue(&buffer, code, value);
	buffer.close();
	if (code != 0 || value != QLatin1String("SECTION"))
	{
		// File does not start with DXF section
		return QApplication::translate("OpenOrienteering::DXFParser", "The file is not an DXF file.");
	}

	paths = QList<DXFPath>();
	current_section = NOTHING;
	QPointF bottom_right, top_left;
	
	/*
	SECTION
		ENTITIES
			POLYLINE
		SEQEND
	ENDSEC
	EOF
	  */
	while (readNextCodeValue(device, code, value))
	{
		if (code == 0 && value == QLatin1String("ENDSEC"))
		{
			current_section = NOTHING;
		}
		else if (code == 0 && value == QLatin1String("EOF"))
		{
			current_section = NOTHING;
		}
		else if (current_section == NOTHING)
		{
			if (code == 0 && value == QLatin1String("SECTION"))
			{
				current_section = SECTION;
			}
			else if (value == QLatin1String("EOF"))
			{
				break;
			}
		}
		else if (current_section == SECTION)
		{
			if (code == 2 && value == QLatin1String("ENTITIES"))
			{
				current_section = ENTITIES;
			}
			if (code == 2 && value == QLatin1String("HEADER"))
			{
				current_section = HEADER;
			}
		}
		else if (current_section == ENTITIES)
		{
			if (code == 0 && value == QLatin1String("LINE"))
				parseLine(device, &paths);
			else if (code == 0 && value == QLatin1String("POLYLINE"))
			{
				parsePolyline(device, &paths);
				current_section = POLYLINE;
			}
			else if (code == 0 && value == QLatin1String("LWPOLYLINE"))
				parseLwPolyline(device, &paths);
			else if (code == 0 && value == QLatin1String("SPLINE"))
				parseSpline(device, &paths);
			else if (code == 0 && value == QLatin1String("CIRCLE"))
				parseCircle(device, &paths);
			else if (code == 0 && value == QLatin1String("POINT"))
				parsePoint(device, &paths);
			else if (code == 0 && value == QLatin1String("TEXT"))
				parseText(device, &paths);
			else if (code == 0 && value == QLatin1String("ARC"))
				parseArc(device, &paths);
#if defined(MAPPER_DEVELOPMENT_BUILD)
			else if (code == 0)
				qDebug() << "Unknown entity:" << value;
#endif
		}
		else if (current_section == HEADER)
		{
			if (code == 9 && value == QLatin1String("$EXTMIN"))
				parseExtminmax(device, bottom_right);
			else if (code == 9 && value == QLatin1String("$EXTMAX"))
				parseExtminmax(device, top_left);
		}
		else if (current_section == POLYLINE)
		{
			if (code == 0 && value == QLatin1String("SEQEND"))
			{
				parseSeqend(device, &paths);
				current_section = ENTITIES;
			}
			else if (code == 0 && value == QLatin1String("VERTEX"))
				parseVertex(device, &paths);
		}
	}
	
	if (must_close_device)
	{
		device->close();
	}
	
	size = QRectF(top_left, bottom_right);
	return QString();
}

inline
bool DXFParser::atEntityEnd(QIODevice* d)
{
	QByteArray data = d->peek(5);
	const int linebreak = data.indexOf('\n');
	if (linebreak > 0)
		data.truncate(linebreak);
	return data.trimmed() == "0";
}

inline
bool DXFParser::readNextCodeValue(QIODevice *device, int &code, QString &value)
{
	code  = device->readLine(128).trimmed().toInt();
	value = QString::fromUtf8(device->readLine(256).trimmed());
	return !device->atEnd();
}

inline
void DXFParser::parseCommon(int code, const QString& value, DXFPath& path)
{
	if (code == 8)
	{
		path.layer = value;
	}
	else if (code == 420)
	{
		QColor color;
		color.setRed(value.leftRef(2).toInt());
		color.setGreen(value.midRef(2, 2).toInt());
		color.setBlue(value.rightRef(2).toInt());
		path.color = color;
	}
	else if (code == 430)
	{
		path.color.setNamedColor(value);
	}
	else if (code == 440)
	{
		path.color.setAlpha(value.toInt());
	}
}

void DXFParser::parseLine(QIODevice *d, QList<DXFPath> *p)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		paths.append(vertex_main);
		in_vertex = false;
	}
	
	DXFPath path(LINE);
	DXFCoordinate co1;
	DXFCoordinate co2;
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			path.thickness = value.toInt();
		else if (code == 10)
			co1.x = value.toDouble();
		else if (code == 20)
			co1.y = value.toDouble();
		else if (code == 30 || code == 50)
			co1.z = value.toDouble();
		else if (code == 11)
			co2.x = value.toDouble();
		else if (code == 21)
			co2.y = value.toDouble();
		else if (code == 31)
			co2.z = value.toDouble();
		else
			parseCommon(code, value, path);
	}
	
	path.coords.append(co1);
	path.coords.append(co2);
	p->append(path);
}

void DXFParser::parsePolyline(QIODevice *d, QList<DXFPath> *p)
{
	Q_UNUSED(p)
	
	vertex_main = DXFPath(LINE);
	vertices = QList<DXFCoordinate>();
	in_vertex = true;
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			vertex_main.thickness = value.toInt();
		else
			parseCommon(code, value, vertex_main);
	}
}

void DXFParser::parseLwPolyline(QIODevice *d, QList<DXFPath> *p)
{
	DXFPath path(LINE);
	QList<DXFCoordinate> coordinates;
	DXFCoordinate coord;
	bool have_x = false;
	bool have_y = false;

	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			path.thickness = value.toInt();
		else if (code == 10)
		{
			coord.x = value.toDouble();
			have_x = true;
		}
		else if (code == 20)
		{
			coord.y = value.toDouble();
			have_y = true;
		}
		else if (code == 70)
		{
			path.closed = (value.toInt() & 1) == 1;
		}
		else
			parseCommon(code, value, path);
		
		if (have_x && have_y)
		{
			coordinates.append(coord);
			have_x = false;
			have_y = false;
		}
	}
	path.coords = coordinates;
	p->append(path);
}

void DXFParser::parseSpline(QIODevice* d, QList<DXFPath>* p)
{
	DXFPath path(SPLINE);
	QList<DXFCoordinate> coordinates;
	DXFCoordinate coord;
	bool have_x = false;
	bool have_y = false;
	
	// TODO: very basic implementation assuming cubic bezier splines.
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 71)
		{
			if (value != QLatin1String("3"))
			{
				qWarning() << "DXFParser: Splines of degree" << value << "are not supported!";
				return;
			}
		}
		else if (code == 10)
		{
			coord.x = value.toDouble();
			have_x = true;
		}
		else if (code == 20)
		{
			coord.y = value.toDouble();
			have_y = true;
		}
		else if (code == 70)
		{
			path.closed = (value.toInt() & 1) == 1;
		}
		else
			parseCommon(code, value, path);
		
		if (have_x && have_y)
		{
			coordinates.append(coord);
			have_x = false;
			have_y = false;
		}
	}
	path.coords = coordinates;
	p->append(path);
}

void DXFParser::parseCircle(QIODevice *d, QList<DXFPath> *p)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		paths.append(vertex_main);
		in_vertex = false;
	}
	
	DXFPath path(CIRCLE);
	DXFCoordinate co;

	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			path.thickness = value.toInt();
		else if (code == 10)
			co.x = value.toDouble();
		else if (code == 20)
			co.y = value.toDouble();
		else if (code == 30 || code == 50)
			co.z = value.toDouble();
		else if (code == 40)
			path.radius = value.toInt();
		else
			parseCommon(code, value, path);
	}
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parsePoint(QIODevice *d, QList<DXFPath> *p)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		paths.append(vertex_main);
		in_vertex = false;
	}
	
	DXFPath path(POINT);
	DXFCoordinate co;
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			path.thickness = value.toInt();
		else if (code == 10)
			co.x = value.toDouble();
		else if (code == 20)
			co.y = value.toDouble();
		else if (code == 30)
			co.z = value.toDouble();
		else if (code == 50)
			path.rotation = value.toDouble();
		else
			parseCommon(code, value, path);
	}
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parseVertex(QIODevice *d, QList<DXFPath> *p)
{
	Q_UNUSED(p)
	
	DXFCoordinate co;
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 10)
			co.x = value.toDouble();
		if (code == 20)
			co.y = value.toDouble();
		if (code == 30 || code == 50)
			co.z = value.toDouble();
	}
	vertices.append(co);
}

void DXFParser::parseSeqend(QIODevice *d, QList<DXFPath> *p)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		p->append(vertex_main);
		in_vertex = false;
	}
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		; // nothing
	}
}

void DXFParser::parseText(QIODevice *d, QList<DXFPath> *p)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		paths.append(vertex_main);
		in_vertex = false;
	}
	
	DXFPath path(TEXT);
	path.color = Qt::red;
	path.text = QString::fromLatin1("<span style=\"%1\"></span>");
	DXFCoordinate co;
	int alignment = 0;
	int valignment = 0;
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			path.thickness = value.toInt();
		else if (code == 10)
			co.x = value.toDouble();
		else if (code == 20)
			co.y = value.toDouble();
		else if (code == 30)
			co.z = value.toDouble();
		else if (code == 50)
			path.rotation = value.toDouble();
		else if (code == 1)
			path.text = path.text.insert(path.text.indexOf(QLatin1Char('>'))+1, value);
		else if (code == 7)
			path.text = path.text.arg(QLatin1String("font-family:") + value + QLatin1String(";%1"));
		else if (code == 40)
			path.font.setPointSizeF(value.toDouble());
		else if (code == 72)
			alignment = value.toInt();
		else if (code == 73)
			valignment = value.toInt();
		else
			parseCommon(code, value, path);
	}
	
	if (path.color != QColor(127,127,127))
	{
		path.text = path.text.arg(QLatin1String("color:") + path.color.name() + QLatin1String(";%1"));
	}
	if (!path.text.contains(QLatin1String("color")))
	{
		path.text = path.text.arg(QString::fromLatin1("color:red"));
	}
	switch(alignment)
	{
	case 0:
		path.text = path.text.arg(QString::fromLatin1("text-align:left;%1"));
		break;
	case 1:
		path.text = path.text.arg(QString::fromLatin1("text-align:center;%1"));
		break;
	case 2:
		path.text = path.text.arg(QString::fromLatin1("text-align:right;%1"));
		break;
	}
	switch(valignment)
	{
	case 0:
		path.text = path.text.arg(QString::fromLatin1("text-align:baseline;%1"));
		break;
	case 1:
		path.text = path.text.arg(QString::fromLatin1("text-align:bottom;%1"));
		break;
	case 2:
		path.text = path.text.arg(QString::fromLatin1("text-align:middle;%1"));
		break;
	case 3:
		path.text = path.text.arg(QString::fromLatin1("text-align:top;%1"));
	}

	    path.text = path.text.arg(QString{});
	//qDebug() << path.text;
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parseArc(QIODevice *d, QList<DXFPath> *p)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		paths.append(vertex_main);
		in_vertex = false;
	}
	
	DXFPath path(ARC);
	DXFCoordinate co;
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 39)
			path.thickness = value.toInt();
		else if (code == 10)
			co.x = value.toDouble();
		else if (code == 20)
			co.y = value.toDouble();
		else if (code == 30)
			co.z = value.toDouble();
		else if (code == 40)
			path.radius = value.toDouble();
		else if (code == 50)
			path.start_angle = value.toDouble();
		else if (code == 51)
			path.end_angle = value.toDouble();
		else
			parseCommon(code, value, path);
	}
	//qDebug() << "start: " << path.startAngle <<" stop "<< path.endAngle << " radius " << path.radius;
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parseExtminmax(QIODevice *d, QPointF &point)
{
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		if (code == 10)
			point.setX(value.toDouble());
		if (code == 20)
			point.setY(value.toDouble());
	}
}

void DXFParser::parseUnknown(QIODevice *d)
{
	if (in_vertex)
	{
		vertex_main.coords = vertices;
		paths.append(vertex_main);
		in_vertex = false;
	}
	
	int code;
	QString value;
	while (!atEntityEnd(d) && readNextCodeValue(d, code, value))
	{
		; // nothing
	}
}


}  // namespace OpenOrienteering
