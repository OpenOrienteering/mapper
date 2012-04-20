#ifndef DXFPARSER_H
#define DXFPARSER_H

#include <QtCore>
#include <QtGui>
#include "parser.h"

class DXFParser : public Parser
{
public:
	DXFParser(){ device = 0; }
	virtual void setData(QIODevice *data){ device = data; invertex = false; }
	virtual QString parse();
	virtual QList<path_t> getData(){ return paths; }
	virtual QRectF getSize(){ return size; }

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

#endif // DXFPARSER_H
