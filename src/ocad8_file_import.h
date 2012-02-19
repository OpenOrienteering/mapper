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
    // Symbol import
    Symbol *importPointSymbol(const OCADPointSymbol *ocad_symbol);
    Symbol *importLineSymbol(const OCADLineSymbol *ocad_symbol);
    Symbol *importAreaSymbol(const OCADAreaSymbol *ocad_symbol);
    Symbol *importTextSymbol(const OCADTextSymbol *ocad_symbol);
    //Symbol *importRectSymbol(const OCADRectSymbol *ocad_symbol);

    // Object import
    Object *importObject(const OCADObject *ocad_object);

    // Some helper functions that are used in multiple places
    PointSymbol *importPattern(s16 npts, OCADPoint *pts);
    void fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol);
    void fillPathCoords(Object *object, s16 npts, OCADPoint *pts);
    bool fillTextPathCoords(TextObject *object, s16 npts, OCADPoint *pts);

    // Unit conversion functions
    QString convertPascalString(const char *p);
    QString convertCString(const char *p, size_t n);
    QString convertWideCString(const char *p, size_t n);
    float convertRotation(int angle);
    void convertPoint(MapCoord &c, int ocad_x, int ocad_y);
    qint64 convertSize(int ocad_size);

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

    /// Offset between OCAD map origin and Mapper map origin (in Mapper coordinates)
    qint64 offset_x, offset_y;

};

#endif // OCAD8_FILE_IMPORT_H
