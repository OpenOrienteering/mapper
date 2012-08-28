/*
 *    Copyright 2012 Pete Curtis
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
#include "symbol_combined.h"
#include "object.h"
#include "object_text.h"


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


/*
Importer *XMLFileFormat::createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    return new XMLFileImporter(path, map, view);
}
*/

Exporter *XMLFileFormat::createExporter(QIODevice* stream, const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    return new XMLFileExporter(stream, map, view);
}

#define NS_V1 "http://oo-mapper.com/oo-mapper/v1"

/*
XMLFileImporter::XMLFileImporter(const QString &path, Map *map, MapView *view)
    : Importer(path, map, view), reader("document", NS_V1)
{

}


void XMLFileImporter::doExport() throw (FormatException)
{
    reader.down("map");

    reader.down("colors");
    Q_FOREACH(reader.all("color"))
    {
        MapColor *color = parseColor(reader);

    }

    for (int i = 0; i < map->getNumColors(); i++)
    {

        const MapColor *color = map->getColor(i);
        reader.down("color")
                .attr("cmyk", QString("%1,%2,%3,%4").arg(color->c).arg(color->m).arg(color->y).arg(color->k))
                .attr("priority", color->priority)
                .attr("opacity", color->opacity)
                .sub("name", color->name)
                .up();
        color_index[color] = i;
    }
    reader.up();
}
*/


XMLFileExporter::XMLFileExporter(QIODevice* stream, Map *map, MapView *view)
    : Exporter(stream, map, view), builder("document", NS_V1)
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
        const MapLayer *layer = map->getLayer(i);
        builder.down("layer").sub("name", layer->getName());
        for (int j = 0; j < layer->getNumObjects(); j++)
        {
            exportObject(layer->getObject(j));
        }
        builder.up();
    }
    builder.up();

    builder.up();

    builder.down("view");
    // TODO: save view parameters

    builder.up();

	stream->write(builder.document().toString().toUtf8());
}

void XMLFileExporter::exportSymbol(const Symbol *symbol, bool anonymous)
{
    if (!anonymous)
    {
        builder.attr("number", symbol->getNumberAsString())
                .attr("helper", symbol->isHelperSymbol())
                .attr("hidden", symbol->isHidden())
                .attr("protected", symbol->isProtected())
                .sub("name", symbol->getName())
                .sub("description", symbol->getDescription());

        // TODO: save icon
    }

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
        /*
        if (s->getCapStyle() == LineSymbol::PointedCap)
        {
            builder.attr("taper-length", s->pointed_cap_length);
        }
        */

        if (s->getBorder().width > 0)
        {
            builder.attr("border-color", color_index[s->getBorder().color])
                    .attr("border-width", s->getBorder().width)
                    .attr("border-shift", s->getBorder().shift);
        }

        if (s->getStartSymbol())
        {
            builder.down("start-symbol");
            exportSymbol(s->getStartSymbol(), true);
            builder.up();
        }
        if (s->getMidSymbol())
        {
            builder.down("mid-symbol");
            exportSymbol(s->getMidSymbol(), true);
            builder.up();
        }
        if (s->getEndSymbol())
        {
            builder.down("end-symbol");
            exportSymbol(s->getEndSymbol(), true);
            builder.up();
        }
        if (s->getDashSymbol())
        {
            builder.down("dash-symbol");
            exportSymbol(s->getDashSymbol(), true);
            builder.up();
        }


    }
    else if (symbol->getType() == Symbol::Area)
    {
        builder.down("area-symbol");
        const AreaSymbol *s = reinterpret_cast<const AreaSymbol *>(symbol);

        builder.attr("color", color_index[s->getColor()])
                .attr("min-area", s->getMinimumArea());
        for (int i = 0; i < s->getNumFillPatterns(); i++)
        {
            builder.down("pattern");
            exportPattern(s->getFillPattern(i));
            builder.up();
        }
        builder.up();
    }
    else if (symbol->getType() == Symbol::Text)
    {
        builder.down("text-symbol");
        const TextSymbol *s = reinterpret_cast<const TextSymbol *>(symbol);
        builder.attr("color", color_index[s->getColor()])
                .attr("font-family", s->getFontFamily())
                .attr("font-size", s->getFontSize())
                .attr("font-weight", s->isBold() ? "bold" : "normal")
                .attr("font-style", s->isItalic() ? "italic" : "normal")
                .attr("text-decoration", s->isUnderlined() ? "underline" : "normal")
                .attr("char-spacing", s->getCharacterSpacing())
                .attr("line-spacing", s->getLineSpacing())
                .attr("paragraph-spacing", s->getParagraphSpacing())
                .attr("font-kerning", s->usesKerning());
        if (s->hasLineBelow())
        {
            builder.down("underline")
                    .attr("color", color_index[s->getLineBelowColor()])
                    .attr("width", s->getLineBelowWidth())
                    .attr("distance", s->getLineBelowDistance())
                    .up();
        }
        for (int i = 0; i < s->getNumCustomTabs(); i++)
        {
            int tab = s->getCustomTab(i);
            builder.down("tab-stop").attr("position", tab).up();
        }
        //builder.attr("default-tab", s->tab_interval); // no getter or setter?
        builder.up();
    }
    else if (symbol->getType() == Symbol::Combined)
    {
        builder.down("combined-symbol");
        const CombinedSymbol *s = reinterpret_cast<const CombinedSymbol *>(symbol);
        for (int i = 0; i < s->getNumParts(); i++)
        {
            exportSymbol(s->getPart(i), true);
        }
    }
    else
    {
        builder.down("symbol");
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
    builder.attr("path", makePath(object));
    builder.up();
}

void XMLFileExporter::exportPattern(const struct AreaSymbol::FillPattern &pattern)
{
    // FIXME
}

QString XMLFileExporter::makePath(const Object *object) const
{
    QString d ="";
	const MapCoordVector& coords = object->getRawCoordinateVector();
	for (int i = 0; i < (int)coords.size(); i++)
    {
        const MapCoord &coord = coords[i];
        if (!d.isEmpty()) d = d % " ";
        d = d % QString("%1,%2").arg(coord.internalX()).arg(coord.internalY());
    }
    return d;
}
