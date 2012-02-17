#ifndef OCAD8_FILE_IMPORT_H
#define OCAD8_FILE_IMPORT_H

#include <QTextCodec>

#include "map.h"
#include "symbol.h"
#include "object.h"
#include "../libocad/libocad.h"

class OCAD8FileImport
{
public:
    OCAD8FileImport(Map *map, MapView *view);
    ~OCAD8FileImport();

    void loadFrom(const char *filename) throw (std::exception);
    void saveTo(const char *filename) throw (std::exception);

    inline const std::vector<QString> &warnings() { return warn; }

    void setStringEncodings(const char *narrow, const char *wide = "UTF-16LE");

protected:
    QString str1(const char *p);
    QString str2(const char *p);
    float convertRotation(int angle);
    qint64 convertPosition(int position);

    Symbol *importPointSymbol(const OCADPointSymbol *ocad_symbol);
    Symbol *importLineSymbol(const OCADLineSymbol *ocad_symbol);
    Symbol *importAreaSymbol(const OCADAreaSymbol *ocad_symbol);
    //Symbol *importTextSymbol(const OCADTextSymbol *ocad_symbol);
    //Symbol *importRectSymbol(const OCADRectSymbol *ocad_symbol);

    void fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol);
    PointSymbol *importPointSymbolFromElements(s16 npts, OCADPoint* pts); // used for area patterns

    Object *importObject(const OCADObject *ocad_object);

private:
    /// A list of import warnings
    std::vector<QString> warn;

    /// The Map to populate
    Map *map;

    /// The MapView to populate
    MapView *view;

    /// Handle to the open OCAD file
    OCADFile *file;

    /// Character encoding to use for 1-byte (narrow) strings
    QTextCodec *encoding_1byte;

    /// Character encoding to use for 2-byte (wide) strings
    QTextCodec *encoding_2byte;

    /// maps OCAD color number to oo-mapper color object
    QHash<int, MapColor *> color_index;

    /// maps OCAD symbol number to oo-mapper symbol object
    QHash<int, Symbol *> symbol_index;


};

#endif // OCAD8_FILE_IMPORT_H
