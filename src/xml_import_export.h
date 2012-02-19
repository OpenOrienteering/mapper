#ifndef XML_IMPORT_EXPORT_H
#define XML_IMPORT_EXPORT_H

#include <QDomDocument>

#include "import_export.h"

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


class XMLImportExport : public Exporter
{
public:
    XMLImportExport(const QString &path, Map *map, MapView *view);
    ~XMLImportExport() {}

    void doExport() throw (ImportExportException);

protected:
    void exportSymbol(const Symbol *symbol, bool anonymous = false);
    void exportObject(const Object *object, bool symbol_reference = true);
    QString makePath(const Object *object) const;

private:
    XMLBuilder builder;

};

#endif // XML_IMPORT_EXPORT_H
