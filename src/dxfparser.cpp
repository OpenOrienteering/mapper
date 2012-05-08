/*
 *    Copyright 2012 Jan Dalheimer
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

#define PARSE_COMMON(p) do{ \
if(line1 == "8") \
	p.layer = line2; \
if(line1 == "420"){ \
	QColor color; \
	color.setRed(line2.left(2).toInt()); \
	color.setGreen(line2.mid(2, 2).toInt()); \
	color.setBlue(line2.right(2).toInt()); \
	p.color = color; \
} \
if(line1 == "430") \
	p.color.setNamedColor(line2); \
if(line1 == "440") \
	p.color.setAlpha(line2.toInt()); \
}while(false)

QString DXFParser::parse(){
	bool weOpened = false;
	if(!device){
		return QString("No data set");
	}
	if(!device->isOpen()){
		if(!device->open(QIODevice::ReadOnly)){
			return QString("Could not open device");
		}
		weOpened = true;
	}

	if(!device->readAll().contains("EOF"))
		return "The file is not a DXF file";
	device->seek(0);

	paths = QList<path_t>();
	QString line1 = "  0";
	QString line2 = "  0";
	getLines(line1, line2, device);
	currentSection = NOTHING;
	QPointF bottomRight, topLeft;

	/*
	SECTION
		ENTITIES
			POLYLINE
		SEQEND
	ENDSEC
	EOF
	  */
	do{
		if(currentSection == NOTHING){
			if(line1 == "0" && line2 == "SECTION"){
				parseUnknown(device);
				currentSection = SECTION;
			}
			else if(line2 == "EOF")
				break;
		}
		else if(currentSection == SECTION){
			if(line1 == "2" && line2 == "ENTITIES"){
				parseUnknown(device);
				currentSection = ENTITIES;
			}
			if(line1 == "2" && line2 == "HEADER"){
				parseUnknown(device);
				currentSection = HEADER;
			}
		}
		else if(currentSection == ENTITIES){
			if(line1 == "0" && line2 == "LINE")
				parseLine(device, &paths);
			else if(line1 == "0" && line2 == "POLYLINE"){
				parsePolyline(device, &paths);
				currentSection = POLYLINE;
			}
			else if(line1 == "0" && line2 == "CIRCLE")
				parseCircle(device, &paths);
			else if(line1 == "0" && line2 == "POINT")
				parsePoint(device, &paths);
			else if(line1 == "0" && line2 == "TEXT")
				parseText(device, &paths);
			else if(line1 == "0" && line2 == "ARC")
				parseArc(device, &paths);
			else if(line1 == "0" && line2 == "ENDSEC"){
				parseUnknown(device);
				currentSection = NOTHING;
			}
			else;
				//qDebug(qPrintable(QString("Unknown entity: ")+line2));
		}
		else if(currentSection == HEADER){
			if(line1 == "9" && line2 == "$EXTMIN")
				parseExtminmax(device, bottomRight);
			else if(line1 == "9" && line2 == "$EXTMAX")
				parseExtminmax(device, topLeft);
		}
		else if(currentSection == POLYLINE){
			if(line1 == "0" && line2 == "SEQEND"){
				parseSeqend(device, &paths);
				currentSection = ENTITIES;
			}
			else if(line1 == "0" && line2 == "VERTEX")
				parseVertex(device, &paths);
		}
		getLines(line1, line2, device);
		if(device->atEnd())
			break;
	}while(true);

	if(weOpened)
		device->close();
	size = QRectF(topLeft, bottomRight);
	return QString();
}

void DXFParser::getLines(QString &l1, QString &l2, QIODevice *d){

	l1 = QString(d->readLine(128).trimmed());
	l2 = QString(d->readLine(128).trimmed());
}

void DXFParser::parseLine(QIODevice *d, QList<path_t> *p){
	if(invertex){
		vertexMain.coords = vertexs;
		paths.append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	path_t path;
	INIT_PATH(path);
	path.type = LINE;
	coordinate_t co1;
	co1.x = 0.0;
	co1.y = 0.0;
	co1.z = 0.0;
	coordinate_t co2;
	co2.x = 0.0;
	co2.y = 0.0;
	co2.z = 0.0;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		PARSE_COMMON(path);
		if(line1 == "39")
			path.thickness = line2.toInt();
		if(line1 == "10")
			co1.x = line2.toDouble();
		if(line1 == "20")
			co1.y = line2.toDouble();
		if(line1 == "30" || line1 == "50")
			co1.z = line2.toDouble();
		if(line1 == "11")
			co2.x = line2.toDouble();
		if(line1 == "21")
			co2.y = line2.toDouble();
		if(line1 == "31")
			co2.z = line2.toDouble();
	}while(true);
	path.coords.append(co1);
	path.coords.append(co2);
	p->append(path);
}

void DXFParser::parsePolyline(QIODevice *d, QList<path_t> *p){
	QString line1;
	QString line2;

	INIT_PATH(vertexMain);
	vertexMain.type = LINE;
	vertexs = QList<coordinate_t>();
	invertex = true;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		PARSE_COMMON(vertexMain);
		if(line1 == "39")
			vertexMain.thickness = line2.toInt();
	}while(true);
	Q_UNUSED(p)
}

void DXFParser::parseCircle(QIODevice *d, QList<path_t> *p){
	if(invertex){
		vertexMain.coords = vertexs;
		paths.append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	path_t path;
	INIT_PATH(path);
	path.type = CIRCLE;
	coordinate_t co;
	co.x = 0.0;
	co.y = 0.0;
	co.z = 0.0;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		PARSE_COMMON(path);
		if(line1 == "39")
			path.thickness = line2.toInt();
		if(line1 == "10")
			co.x = line2.toDouble();
		if(line1 == "20")
			co.y = line2.toDouble();
		if(line1 == "30" || line1 == "50")
			co.z = line2.toDouble();
		if(line1 == "40")
			path.radius = line2.toInt();
	}while(true);
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parsePoint(QIODevice *d, QList<path_t> *p){
	if(invertex){
		vertexMain.coords = vertexs;
		paths.append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	path_t path;
	INIT_PATH(path);
	path.type = POINT;
	coordinate_t co;
	co.x = 0.0;
	co.y = 0.0;
	co.z = 0.0;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		PARSE_COMMON(path);
		if(line1 == "39")
			path.thickness = line2.toInt();
		if(line1 == "10")
			co.x = line2.toDouble();
		if(line1 == "20")
			co.y = line2.toDouble();
		if(line1 == "30")
			co.z = line2.toDouble();
		if(line1 == "50")
			path.rotation = line2.toDouble();
	}while(true);
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parseVertex(QIODevice *d, QList<path_t> *p){
	QString line1;
	QString line2;

	coordinate_t co;
	co.x = 0.0;
	co.y = 0.0;
	co.z = 0.0;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		if(line1 == "10")
			co.x = line2.toDouble();
		if(line1 == "20")
			co.y = line2.toDouble();
		if(line1 == "30" || line1 == "50")
			co.z = line2.toDouble();
	}while(true);
	vertexs.append(co);
	Q_UNUSED(p)
}

void DXFParser::parseSeqend(QIODevice *d, QList<path_t> *p){
	if(invertex){
		vertexMain.coords = vertexs;
		p->append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	if(d->peek(3).trimmed() == "0")
		return;
	getLines(line1, line2, d);

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
	}while(true);
}

void DXFParser::parseText(QIODevice *d, QList<path_t> *p){
	if(invertex){
		vertexMain.coords = vertexs;
		paths.append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	path_t path;
	INIT_PATH(path);
	path.type = TEXT;
	path.color = Qt::red;
	path.text = "<span style=\"%1\"></span>";
	coordinate_t co;
	co.x = 0.0;
	co.y = 0.0;
	co.z = 0.0;
	int alignment = 0;
	int valignment = 0;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		PARSE_COMMON(path);
		if(line1 == "39")
			path.thickness = line2.toInt();
		if(line1 == "10")
			co.x = line2.toDouble();
		if(line1 == "20")
			co.y = line2.toDouble();
		if(line1 == "30")
			co.z = line2.toDouble();
		if(line1 == "50")
			path.rotation = line2.toDouble();
		if(line1 == "1")
			path.text = path.text.insert(path.text.indexOf(">")+1, line2);
		if(line1 == "7")
			path.text = path.text.arg(QString("font-family:")+line2+QString(";%1"));
		if(line1 == "40")
			path.font.setPointSizeF(line2.toDouble());
		if(line1 == "72")
			alignment = line2.toInt();
		if(line1 == "73")
			valignment = line2.toInt();
	}while(true);
	if(path.color != QColor(127,127,127)){
		path.text = path.text.arg(QString("color:")+path.color.name()+QString(";%1"));
	}
	if(!path.text.contains("color")){
		path.text = path.text.arg("color:red");
	}
	switch(alignment){
	case 0:
		path.text = path.text.arg(QString("text-align:left;%1"));
		break;
	case 1:
		path.text = path.text.arg(QString("text-align:center;%1"));
		break;
	case 2:
		path.text = path.text.arg(QString("text-align:right;%1"));
		break;
	}
	switch(valignment){
	case 0:
		path.text = path.text.arg(QString("text-align:baseline;%1"));
		break;
	case 1:
		path.text = path.text.arg(QString("text-align:bottom;%1"));
		break;
	case 2:
		path.text = path.text.arg(QString("text-align:middle;%1"));
		break;
	case 3:
		path.text = path.text.arg(QString("text-align:top;%1"));
	}

	path.text = path.text.arg("");
	//qDebug() << path.text;
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parseArc(QIODevice *d, QList<path_t> *p){
	if(invertex){
		vertexMain.coords = vertexs;
		paths.append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	path_t path;
	INIT_PATH(path);
	path.type = ARC;
	coordinate_t co;
	co.x = 0.0;
	co.y = 0.0;
	co.z = 0.0;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		PARSE_COMMON(path);
		if(line1 == "39")
			path.thickness = line2.toInt();
		if(line1 == "10")
			co.x = line2.toDouble();
		if(line1 == "20")
			co.y = line2.toDouble();
		if(line1 == "30")
			co.z = line2.toDouble();
		if(line1 == "40")
			path.radius = line2.toDouble();
		if(line1 == "50")
			path.startAngle = line2.toDouble();
		if(line1 == "51")
			path.endAngle = line2.toDouble();
		//qDebug() << "start: " << path.startAngle <<" stop "<< path.endAngle << " radius " << path.radius;
	}while(true);
	path.coords.append(co);
	p->append(path);
}

void DXFParser::parseExtminmax(QIODevice *d, QPointF &ResultingPoint){
	QString line1;
	QString line2;

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
		if(line1 == "10")
			ResultingPoint.setX(line2.toDouble());
		if(line1 == "20")
			ResultingPoint.setY(line2.toDouble());
	}while(true);
}

void DXFParser::parseUnknown(QIODevice *d){
	if(invertex){
		vertexMain.coords = vertexs;
		paths.append(vertexMain);
		invertex = false;
	}
	QString line1;
	QString line2;

	if(d->peek(3).trimmed() == "0")
		return;
	getLines(line1, line2, d);

	do{
		if(d->peek(3).trimmed() == "0")
			break;
		getLines(line1, line2, d);
	}while(true);
}
