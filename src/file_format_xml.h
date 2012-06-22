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

#ifndef XML_IMPORT_EXPORT_H
#define XML_IMPORT_EXPORT_H

#include <QDomDocument>

#include "file_format.h"
#include "symbol_area.h"

/** Provides the Builder pattern for a DOM tree.
 */
class XMLBuilder
{
public:
    XMLBuilder(const QString &rootElement, const QString &rootNamespace = QString::null);
    inline QDomDocument document() const { return doc; }

    XMLBuilder &attr(const QString &name, bool value);
    template <typename T> XMLBuilder &attr(const QString &name, const T &value);

    XMLBuilder &append(const QString &text);

    XMLBuilder &down(const QString &name, const QString &ns = QString::null);
    XMLBuilder &up();

    XMLBuilder &sub(const QString &name, const QString &value, const QString &ns = QString::null);

private:
    QDomDocument doc;
    QDomElement current;
};


class XMLFileFormat : public Format
{
public:
    XMLFileFormat() : Format("XML", QObject::tr("OpenOrienteering Mapper XML"), "xml", false, true, true) {}

    bool understands(const unsigned char *buffer, size_t sz) const;
    //Importer *createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException);
    Exporter *createExporter(QIODevice* stream, const QString& path, Map *map, MapView *view) const throw (FormatException);
};

/*
class XMLFileImporter : public Importer
{
public:
    XMLFileImporter(const QString &path, Map *map, MapView *view);
    ~XMLFileImporter() {}

    void doImport(bool load_symbols_only) throw (FormatException);

};
*/


class XMLFileExporter : public Exporter
{
public:
    XMLFileExporter(QIODevice* stream, Map *map, MapView *view);
    ~XMLFileExporter() {}

    void doExport() throw (FormatException);

protected:
    void exportSymbol(const Symbol *symbol, bool anonymous = false);
    void exportObject(const Object *object, bool symbol_reference = true);
    void exportPattern(const struct AreaSymbol::FillPattern &pattern);
    QString makePath(const Object* object) const;

private:
    XMLBuilder builder;

    QHash<const MapColor *, int> color_index;

};

#endif // XML_IMPORT_EXPORT_H
