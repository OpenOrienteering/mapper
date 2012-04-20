#ifndef PARSER_H
#define PARSER_H

#include <QtCore>
#include <QtGui>

typedef struct{
	qreal x;
	qreal y;
	qreal z;
}coordinate_t;

typedef enum{
	CIRCLE, LINE, POINT, TEXT, ARC, UNKNOWN
}type_e;

typedef struct{
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
}path_t;

#define INIT_PATH(p) do{p.layer=1;p.color=QColor(127,127,127);p.thickness=0;p.radius=0;p.type=UNKNOWN;p.rotation=0.0;p.font=QFont();}while(false)

class Parser
{
public:
	Parser(){}
	virtual void setData(QIODevice *data) = 0;
	virtual QString parse() = 0;
	virtual QList<path_t> getData() = 0;
	virtual QRectF getSize() = 0;
};

#endif // PARSER_H
