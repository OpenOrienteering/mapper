#include <QDomImplementation>
#include <QDebug>
#include <QFile>
#include <QStringBuilder>

#include "xml_import_export.h"
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






#define NS_V1 "http://oo-mapper.com/oo-mapper/v1"

XMLImportExport::XMLImportExport(const QString &path, Map *map, MapView *view)
    : Exporter(path, map, view), builder("document", NS_V1)
{

}

void XMLImportExport::doExport() throw (ImportExportException)
{
    builder.down("map");

    builder.down("colors");
    for (uint i = 0; i < map->color_set->colors.size(); i++)
    {
        const MapColor *color = map->color_set->colors[i];
        builder.down("color")
                .attr("cmyk", QString("%1,%2,%3,%4").arg(color->c).arg(color->m).arg(color->y).arg(color->k))
                .attr("priority", color->priority)
                .attr("opacity", color->opacity)
                .sub("name", color->name)
                .up();
    }
    builder.up();

    builder.down("symbols");
    for (uint i = 0; i < map->symbols.size(); i++)
    {
        exportSymbol(map->symbols[i]);
    }
    builder.up();

    builder.down("layers");
    for (uint i = 0; i < map->layers.size(); i++)
    {
        MapLayer *layer = map->layers[i];
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

    QFile f("/Users/pcurtis/Documents/workspace/oo-mapper/omap.xml");
    if (f.open(QIODevice::ReadWrite))
    {
        f.write(builder.document().toString().toUtf8());
        f.close();
    }

}

void XMLImportExport::exportSymbol(const Symbol *symbol, bool anonymous)
{
    if (symbol->type == Symbol::Point)
    {
        const PointSymbol *s = reinterpret_cast<const PointSymbol *>(symbol);
        builder.down("point-symbol").attr("rotatable", s->rotatable);
        if (s->inner_color)
        {
            builder.attr("radius", s->inner_radius).attr("fill", s->inner_color->priority);
        }
        if (s->outer_color)
        {
            builder.attr("border", s->outer_width).attr("stroke", s->outer_color->priority);
        }
        for (uint i = 0; i < s->symbols.size(); i++)
        {
            builder.down("element");
            exportSymbol(s->symbols[i], true);
            exportObject(s->objects[i], false);
            builder.up();
        }
    }
    else if (symbol->type == Symbol::Line)
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

void XMLImportExport::exportObject(const Object *object, bool reference_symbol)
{
    if (object->type == Object::Point)
    {
        const PointObject *o = reinterpret_cast<const PointObject *>(object);
        builder.down("point-object").attr("rotation", o->getRotation());
    }
    else if (object->type == Object::Text)
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
    builder.attr("path", makePath(object)).attr("closed", object->path_closed).up();
}

QString XMLImportExport::makePath(const Object *object) const
{
    QString d ="";
    for (uint i = 0; i < object->coords.size(); i++)
    {
        const MapCoord &coord = object->coords[i];
        if (!d.isEmpty()) d = d % " ";
        d = d % QString("%1,%2").arg(coord.internalX()).arg(coord.internalY());
    }
    return d;
}
