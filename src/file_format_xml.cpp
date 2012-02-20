#include <QDomImplementation>
#include <QDebug>
#include <QFile>
#include <QStringBuilder>

#include "file_format_xml.h"
#include "map_color.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "object.h"


XMLBuilder::XMLBuilder(const QString &rootElement, const QString &rootNamespace)
{
    doc = QDomImplementation().createDocument(rootNamespace, rootElement, QDomDocumentType());
    current = doc.documentElement();
}

XMLBuilder &XMLBuilder::attr(const QString &name, bool value)
{
    current.setAttribute(name, value ? "true" : "false");
    return *this;
}

template <typename T> XMLBuilder &XMLBuilder::attr(const QString &name, const T &value)
{
    current.setAttribute(name, value);
    return *this;
}

XMLBuilder &XMLBuilder::append(const QString &text)
{
    QDomNode child = doc.createTextNode(text);
    current.appendChild(child);
    return *this;
}

XMLBuilder &XMLBuilder::down(const QString &name, const QString &ns)
{
    QDomElement child = doc.createElementNS(ns, name);
    current.appendChild(child);
    current = child;
    return *this;
}

XMLBuilder &XMLBuilder::up()
{
    current = current.parentNode().toElement();
    return *this;
}

XMLBuilder &XMLBuilder::sub(const QString &name, const QString &value, const QString &ns)
{
    return down(name, ns).append(value).up();
}





bool XMLFileFormat::understands(const unsigned char *buffer, size_t sz) const
{
    // The first six bytes of the file must be '<?xml '
    static const char *magic = "<?xml ";
    if (sz >= 6 && memcmp(buffer, magic, 6) == 0) return true;
    return false;
}

Exporter *XMLFileFormat::createExporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    return new XMLFileExporter(path, map, view);
}





#define NS_V1 "http://oo-mapper.com/oo-mapper/v1"

XMLFileExporter::XMLFileExporter(const QString &path, Map *map, MapView *view)
    : Exporter(path, map, view), builder("document", NS_V1)
{

}

void XMLFileExporter::doExport() throw (FormatException)
{
    builder.down("map");

    builder.down("colors");
    for (int i = 0; i < map->getNumColors(); i++)
    {
        const MapColor *color = map->getColor(i);
        builder.down("color")
                .attr("cmyk", QString("%1,%2,%3,%4").arg(color->c).arg(color->m).arg(color->y).arg(color->k))
                .attr("priority", color->priority)
                .attr("opacity", color->opacity)
                .sub("name", color->name)
                .up();
        color_index[color] = i;
    }
    builder.up();

    builder.down("symbols");
    for (int i = 0; i < map->getNumSymbols(); i++)
    {
        exportSymbol(map->getSymbol(i));
    }
    builder.up();

    builder.down("layers");
    for (int i = 0; i < map->getNumLayers(); i++)
    {
        MapLayer *layer = map->getLayer(i);
        builder.down("layer").sub("name", layer->getName());
        for (int j = 0; j < layer->getNumObjects(); j++)
        {
            const Object *object = layer->getObject(j);
            exportObject(object);
        }
        builder.up();
    }
    builder.up();

    builder.up();
    builder.down("view").up();

    QFile f(path);
    if (f.open(QIODevice::ReadWrite))
    {
        f.write(builder.document().toString().toUtf8());
        f.close();
    }

}

void XMLFileExporter::exportSymbol(const Symbol *symbol, bool anonymous)
{
    if (symbol->getType() == Symbol::Point)
    {
        const PointSymbol *s = reinterpret_cast<const PointSymbol *>(symbol);
        builder.down("point-symbol").attr("rotatable", s->isRotatable());
        if (s->getInnerColor())
        {
            builder.attr("radius", s->getInnerRadius()).attr("fill", color_index[s->getInnerColor()]);
        }
        if (s->getOuterColor())
        {
            builder.attr("border", s->getOuterWidth()).attr("stroke", color_index[s->getOuterColor()]);
        }
        for (int i = 0; i < s->getNumElements(); i++)
        {
            builder.down("element");
            exportSymbol(s->getElementSymbol(i), true);
            exportObject(s->getElementObject(i), false);
            builder.up();
        }
    }
    else if (symbol->getType() == Symbol::Line)
    {
        const LineSymbol *s = reinterpret_cast<const LineSymbol *>(symbol);
        builder.down("line-symbol")
                .attr("width", s->getLineWidth())
                .attr("cap", s->getCapStyle())
                .attr("join", s->getJoinStyle());
                //.attr("min-length", s->minimum_length);
        /*
        if (s->getCapStyle() == LineSymbol::PointedCap)
        {
            builder.attr("taper-length", s->pointed_cap_length);
        }
        */

        if (s->getStartSymbol())
        {
            builder.down("start-symbol");
            exportSymbol(s->getStartSymbol());
            builder.up();
        }
        if (s->getMidSymbol())
        {
            builder.down("mid-symbol");
            exportSymbol(s->getMidSymbol());
            builder.up();
        }
        if (s->getEndSymbol())
        {
            builder.down("end-symbol");
            exportSymbol(s->getEndSymbol());
            builder.up();
        }
        if (s->getDashSymbol())
        {
            builder.down("dash-symbol");
            exportSymbol(s->getDashSymbol());
            builder.up();
        }


    }
    else
    {
        builder.down("symbol");
    }

    if (!anonymous)
    {
        builder.attr("number", symbol->getNumberAsString())
                .attr("helper", symbol->isHelperSymbol())
                .sub("name", symbol->getName())
                .sub("description", symbol->getDescription());
        // FIXME: do icon
    }

    builder.up();
}

void XMLFileExporter::exportObject(const Object *object, bool reference_symbol)
{
    if (object->getType() == Object::Point)
    {
        const PointObject *o = reinterpret_cast<const PointObject *>(object);
        builder.down("point-object").attr("rotation", o->getRotation());
    }
    else if (object->getType() == Object::Text)
    {
        const TextObject *t = reinterpret_cast<const TextObject *>(object);
        builder.down("text-object")
                .attr("rotation", t->getRotation())
                .attr("halign", t->getHorizontalAlignment())
                .attr("valign", t->getVerticalAlignment())
                .append(t->getText());
    }
    else
    {
        builder.down("object");
    }

    if (reference_symbol)
    {
        builder.attr("symbol", object->getSymbol()->getNumberAsString());
    }
    builder.attr("path", makePath(object)).attr("closed", object->isPathClosed()).up();
}

QString XMLFileExporter::makePath(const Object *object) const
{
    QString d ="";
    for (int i = 0; i < object->getCoordinateCount(); i++)
    {
        const MapCoord &coord = object->getCoordinate(i);
        if (!d.isEmpty()) d = d % " ";
        d = d % QString("%1,%2").arg(coord.internalX()).arg(coord.internalY());
    }
    return d;
}
