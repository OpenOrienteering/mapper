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


#ifndef _OPENORIENTEERING_DXFPARSER_H_
#define _OPENORIENTEERING_DXFPARSER_H_

#include <QString>
#include <QList>
#include <QColor>
#include <QFont>
#include <QRectF>

struct coordinate_t
{
	qreal x;
	qreal y;
	qreal z;
};

enum type_e
{
	CIRCLE, LINE, SPLINE, POINT, TEXT, ARC, UNKNOWN
};

struct path_t
{
	QList<coordinate_t> coords;
	QString layer;
	QColor color;
	qreal thickness;
	qreal radius;
	type_e type;
	qreal rotation;
	QFont font;
	QString text;
	qreal startAngle, endAngle;
};

#define INIT_PATH(p) do{p.layer=1;p.color=QColor(127,127,127);p.thickness=0;p.radius=0;p.type=UNKNOWN;p.rotation=0.0;p.font=QFont();}while(false)

class PathParser
{
public:
	PathParser() {}
	virtual ~PathParser() {}
	virtual void setData(QIODevice *data) = 0;
	virtual QString parse() = 0;
	virtual QList<path_t> getData() = 0;
	virtual QRectF getSize() = 0;
};

class DXFParser : public PathParser
{
public:
	inline DXFParser() { device = 0; }
	virtual void setData(QIODevice *data) { device = data; invertex = false; }
	virtual QString parse();
	virtual QList<path_t> getData() { return paths; }
	virtual QRectF getSize() { return size; }

private:
	QIODevice *device;
	QList<path_t> paths;

	QList<coordinate_t> vertexs;
	path_t vertexMain;
	bool invertex;

	QRectF size;

	int currentSection;

	void getLines(QString &l1, QString &l2, QIODevice *d);

	void parseLine(QIODevice *d, QList<path_t> *p);
	void parsePolyline(QIODevice *d, QList<path_t> *p);
	void parseLwPolyline(QIODevice* d, QList<path_t> *p);
	void parseSpline(QIODevice* d, QList<path_t> *p);
	void parseCircle(QIODevice *d, QList<path_t> *p);
	void parsePoint(QIODevice *d, QList<path_t> *p);
	void parseVertex(QIODevice *d, QList<path_t> *p);
	void parseSeqend(QIODevice *d, QList<path_t> *p);
	void parseText(QIODevice *d, QList<path_t> *p);
	void parseArc(QIODevice *d, QList<path_t> *p);
	void parseExtminmax(QIODevice *d, QPointF &p);
	void parseUnknown(QIODevice *d);

	enum{
		HEADER, ENTITIES, SECTION, NOTHING, POLYLINE
	};
};

#endif
