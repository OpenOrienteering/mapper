/*
 *    Copyright 2012 Peter Curtis
 *
 *    This file is part of libocad.
 *
 *    libocad is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    libocad is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with libocad.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBOCAD_H
#define LIBOCAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "types.h"
#include "array.h"
#include "geometry.h"

// Use a custom strdup and round implementation to prevent portability problems
char *my_strdup(const char *s);
int my_round (double x);

#ifndef _WIN32
// Some definitions to fix up compilation under POSIX
#define O_BINARY 0
#endif

#ifdef _MSC_VER
	#include <io.h>
	#define snprintf _snprintf
#endif

typedef char str;

#ifdef _MSC_VER
	#define PACK( __Declaration__ , __Name__ ) __pragma( pack(push, 1) ) __Declaration__ __Name__; __pragma( pack(pop) )
#elif defined(__GNUC__)
	#define PACK( __Declaration__ , __Name__ ) __Declaration__ __attribute__((__packed__)) __Name__;
#endif


// Flags in X coordinate
#define PX_CTL1 0x1
#define PX_CTL2 0x2
#define PX_LEFT 0x4

// Flags in Y coordinate
#define PY_CORNER 0x1
#define PY_HOLE 0x2
#define PY_RIGHT 0x4
#define PY_DASH 0x8

#define OCAD_MAX_OBJECT_PTS 32768

typedef
struct _OCADString {
	u8 size;
    char *data;
}
OCADString;


PACK(typedef
struct _OCADPoint {
	s32 x;
	s32 y;
}
, OCADPoint)


PACK(typedef
struct _OCADRect {
	OCADPoint min;
	OCADPoint max;
}
, OCADRect)


PACK(typedef
struct _OCADFileHeader {
	word magic;
    word ftype; // 2=normal, ?=course setting
	word major;
	word minor;
	dword osymidx;
	dword oobjidx;
	dword osetup;
	dword ssetup;
	dword infopos;
	dword infosize;
	dword ostringidx;
	dword res4;
	dword res5;
	dword res6;
	// Begin of "symbol header"
	word ncolors;
	word nsep;
	u8 res8[20];
}
, OCADFileHeader)


PACK(typedef
struct _OCADColor {
	/** The color number associated with this color. This is independent from the color index (which
	 *  defines the painting order) and is the number referenced by symbols.
	 */
	word number;
	
	/** A reserved word in the data structure. */
	word res1;
	
	/** The proportion of cyan ink in this color, in the range 0 to 200 inclusive.
	 */
	byte cyan;

	/** The proportion of magenta ink in this color, in the range 0 to 200 inclusive.
	 */
	byte magenta;

	/** The proportion of yellow ink in this color, in the range 0 to 200 inclusive.
	 */
	byte yellow;
	
	/** The proportion of black ink in this color, in the range 0 to 200 inclusive.
	 */	
	byte black;
	
	/** The name of this color.
	 */
	str name[32];

	/** The spot color attributes for this color; currently an uncharacteristized data blob.
	 */
	u8 spot[32];
}
, OCADColor)


PACK(typedef
struct _OCADColorSeparation {
	str sep_name[16];
	byte cyan;
	byte magenta;
	byte yellow;
	byte black;
	word raster_freq;
	word raster_angle;
}
, OCADColorSeparation)


PACK(typedef
struct _OCADSymbolEntry {
	dword ptr;
}
, OCADSymbolEntry)


PACK(typedef
struct _OCADSymbolIndex {
	dword next;
	OCADSymbolEntry entry[256];
}
, OCADSymbolIndex)


#define OCAD_POINT_SYMBOL 1
#define OCAD_LINE_SYMBOL 2
#define OCAD_AREA_SYMBOL 3
#define OCAD_TEXT_SYMBOL 4
#define OCAD_RECT_SYMBOL 5

#define OCADSymbol_COMMON \
	s16 size; \
	s16 number; \
	s16 type; \
	byte subtype; \
	byte base_flags; \
	s16 extent; \
	bool selected; \
	byte status; \
	s16 res2; \
	s16 res3; \
	dword respos; \
	u8 colors[32]; /* bitmask */ \
	str name[32]; \
	u8 icon[12 * 22];


#define OCAD_LINE_ELEMENT 1
#define OCAD_AREA_ELEMENT 2
#define OCAD_CIRCLE_ELEMENT 3
#define OCAD_DOT_ELEMENT 4

PACK(typedef
struct _OCADSymbolElement {
	s16 type;
	word flags;
	s16 color;
	s16 width;
	s16 diameter;
	u16 npts;
	s16 res1;
	s16 res2;
    OCADPoint pts[1];
}
, OCADSymbolElement)


PACK(typedef
struct _OCADSymbol {
	OCADSymbol_COMMON
}
, OCADSymbol)


PACK(typedef
struct _OCADPointSymbol {
	OCADSymbol_COMMON
	s16 ngrp;			// number of 8-byte groups that follow this
	s16 res;
    OCADPoint pts[1]; 	// This is really a sequence of variable-length OCADSymbolElements.
}
, OCADPointSymbol)


PACK(typedef
struct _OCADLineSymbol {
	OCADSymbol_COMMON
	u16 color;
	u16 width;
    word ends;  // true if "round line ends" is checked
    s16 bdist;  // distance from begin
    s16 edist;  // distance to end
    s16 len;    // main length A
    s16 elen;   // end length B
    s16 gap;    // main gap C
    s16 gap2;   // 2ndy gap D
    s16 egap;   // end gap E
    s16 smin;   // minimum symbols (one less than displayed number)
    s16 snum;   // number of symbols
    s16 sdist;  // symbol distance
    word dmode; // double line mode
    word dflags;// bit 0="fill"
    s16 dcolor; // main color
    s16 lcolor; // left color
    s16 rcolor; // right color
    s16 dwidth; // main width
    s16 lwidth; // left width
    s16 rwidth; // right width
    s16 dlen;   // dashed/distance a
    s16 dgap;   // dashed/gap
    s16 dres[3];
    word tmode; // taper mode: 0=off 1=towards end 2=towards both
    s16 tlast;  // taper last symbol
    s16 tres;
    s16 fcolor; // color of framing line
    s16 fwidth; // width of framing line
    s16 fstyle; // 0=flat/bevel 1=round/round 4=flat/miter
    s16 smnpts; // main symbol point count
    s16 ssnpts; // secondary symbol
    s16 scnpts; // corner symbol
    s16 sbnpts; // begin symbol
    s16 senpts; // end symbol
    s16 res4;
    OCADPoint pts[1]; // symbols
}
, OCADLineSymbol)


PACK(typedef
struct _OCADAreaSymbol {
	OCADSymbol_COMMON
	word flags;
	wbool fill;
	s16 color;
	s16 hmode;
	s16 hcolor;
	s16 hwidth;
	s16 hdist;
	s16 hangle1;
	s16 hangle2;
	s16 hres;
	s16 pmode;
	s16 pwidth;
	s16 pheight;
	s16 pangle;
	s16 pres;
	s16 npts;
    OCADPoint pts[1];
}
, OCADAreaSymbol)


PACK(typedef
struct _OCADTextSymbol {
    OCADSymbol_COMMON
    str font[32];
    s16 color;
    s16 dpts;       // in decipoints
    s16 bold;       // as used in Windows GDI, 400=normal, 700=bold
    byte italic;
	byte charset;
    s16 cspace;     // character spacing
    s16 wspace;     // word spacing
    s16 halign;     // left, center, right, justified = 0-3
    s16 lspace;     // line spacing
    s16 pspace;     // paragraph spacing
    s16 indent1;    // first indent
    s16 indent2;    // subsequent indents
    s16 ntabs;      // number of tabs
    s32 tab[32];    // tab positions
    wbool under;    // underline on
    s16 ucolor;     // underline color
    s16 uwidth;     // underline width
    s16 udist;      // unerline distance from (baseline?)
    s16 res4;
    s16 fmode;      // framing mode 0=none 1=with font 2=with line
    str ffont[32];
    s16 fcolor;
    s16 fdpts;      // in decipoints
    s16 fbold;
    wbool fitalic;
    s16 fdx;        // horizontal offset
    s16 fdy;        // vertical offset
}
, OCADTextSymbol)


PACK(typedef
struct _OCADRectSymbol {
    OCADSymbol_COMMON
    s16 color;
    s16 width;
    s16 corner;
    word flags;     // 1=grid on, 2=numbered from bottom
    s16 cwidth;     // if grid on, cell width
    s16 cheight;    // and cell height
    s16 gcolor;     // grid line color
    s16 gwidth;     // grid line width
    s16 gcells;     // unnumbered cells ??
    str gtext[4];   // text in unnumbered cells
    s16 resgrid;
    str resfont[32];
    s16 rescolor;
    s16 ressize;
    s16 resbold;
    wbool resitalic;
    s16 resdx;
    s16 resdy;
}
, OCADRectSymbol)


PACK(typedef
struct _OCADObject {
	s16 symbol; // symbol number
	byte type; // object type
	byte unicode;
	u16 npts; // number of OCADPoints
	u16 ntext;
	s16 angle;
	s16 res1;
	s32 res2;
	u8 res3[16];
    OCADPoint pts[1];
}
, OCADObject)


PACK(typedef
struct _OCADObjectEntry {
	OCADRect rect;
	dword ptr;
	u16 npts;
	word symbol; // symbol number
}
, OCADObjectEntry)


typedef
struct _OCADObjectIndex {
	dword next;
	OCADObjectEntry entry[256];
}
OCADObjectIndex;


PACK(typedef
struct _OCADCString {
    char str[1];  // zero terminated
}
, OCADCString)


/*
 * FIXME: according to the OCAD 8 format desc, this is:
    Pos: Cardinal;
    Len: Cardinal;
    StType: integer;
    ObjIndex: integer;
 */
PACK(typedef
struct _OCADStringEntry {
	dword ptr;
	dword size;
	word type;
	word res1;
	dword res2;
}
, OCADStringEntry)


PACK(typedef
struct _OCADStringIndex {
	dword next;
	OCADStringEntry entry[256];
}
, OCADStringIndex)


PACK(typedef
struct _OCADSetup {
	OCADPoint center;		// Center point of view
	double pgrid;			// Grid spacing, in mm on paper
	s16 tool;				// Current tool
	s16 linemode;			// Current line mode
	s16 editmode;			// Current edit mode
	s16 selsym;				// Selected symbol number
	double scale;			// Map scale
	double offsetx;
	double offsety;
	double angle;
	double grid;			// in meters
	double gpsangle;
	// aGpsAdjust: array[0..11] of TGpsAdjPoint;
	u8 notused[12*40+4+8+8+8+256+2+2+8+8+8];		// Contains <=v7 template info
	// Print information
	OCADPoint printmin;		// Lower left corner of print window
	OCADPoint printmax;		// Upper right corner of print window
	wbool printgrid;
	s16 printcolor;
	s16 printlapx;
	s16 printlapy;
	double printscale;
	s16 printintens;
	s16 printlinewidth;
	wbool printres;
	wbool printstdfonts;
	wbool printres2;
	wbool printres3;
	OCADPoint partmin;
	OCADPoint partmax;
	double zoom;
	// ZoomHist: array[0..8] of TZoomRec;
	s32 nzoom;
	wbool realcoord;
/*
 FileName: ShortString;     {used internally in temporary files.
                               The name of the original file
                               is stored here}
    HatchAreas: wordbool;      {true if Hatch areas is active}
    DimTemp: wordbool;         {true if Dim template is active}
    HideTemp: wordbool;        {true if Hide template is active}
    TempMode: SmallInt;        {template mode
                                 0: in the background
                                 1: above a color}
    TempColor: SmallInt;       {the color if template mode is 1}
	*/
}
, OCADSetup)


typedef
struct _OCADFile {
	const char *filename;	// Filename
	int fd; 				// File descriptor
	u8 *buffer;				// Location of the buffer
	u32 size;				// Size of the used part of the buffer
	u32 reserved_size;		// Complete size of the buffer
	bool mapped;			// Flag indicating whether the memory is mapped

	OCADFileHeader *header; // Pointer to file header
	OCADColor *colors;		// Pointer to first element of color array.
	OCADSetup *setup;		// Pointer to setup object
}
OCADFile;


typedef
struct _OCADBackground {
	const char *filename;	// Filename of template
	int s;					// 
	s32 trnx;				// Horizontal translation (x)
	s32 trny;				// Vertical translation (y)
	double angle;			// Rotation angle (a)
	double sclx;			// Horizontal scale (u)
	double scly;			// Vertical scale (v)
	int dimming;			// Dimming (d)
	int p;					//
	wbool transparent;		// Transparent flag (t)
	int o;					// Overprint color?? (o)

}
OCADBackground;


#define OCADExportOptions_COMMON \
    int (*do_export)(OCADFile *, OCADExportOptions *); \
	FILE *output; \
	OCADRect rect; \
	int partial;

typedef struct _OCADExportOptions OCADExportOptions;
struct _OCADExportOptions {
	OCADExportOptions_COMMON
};

// PRIVATE to OCADPaintData
typedef
struct _OCADPaintSymbolIndex {
	u16 count;
	u16 size;
	u16 *num;
	PtrArray *arr;
    u8 *data;
}
OCADPaintSymbolIndex;

typedef
struct _OCADPaintColorIndex {
	u16 nsyms;
	OCADPaintSymbolIndex *idx[256];
}
OCADPaintColorIndex;
// PRIVATE to OCADPaintData

typedef
struct _OCADPaintData {
	OCADFile *file;
	bool rect_set;
	OCADRect rect;
	OCADPaintColorIndex index;
}
OCADPaintData;


typedef
struct _OCADPaintCallback {
	void (*set_color)(void *, OCADFile *, OCADColor *);
	void (*set_symbol)(void *, OCADFile *, s16, OCADSymbol *);
	bool (*paint_object)(void *, OCADFile *, s16, OCADSymbol *, OCADObject *);
}
OCADPaintCallback;

//OCADPaintCallback *ocad_paint_debug;
//OCADPaintCallback *ocad_paint_text;


typedef bool (*OCADSymbolCallback)(void *, OCADFile *, OCADSymbol *);
typedef bool (*OCADSymbolElementCallback)(void *, OCADSymbolElement *);
typedef bool (*OCADObjectCallback)(void *, OCADFile *, OCADObject *);
typedef bool (*OCADObjectEntryCallback)(void *, OCADFile *, OCADObjectEntry *);
typedef bool (*OCADStringEntryCallback)(void *, OCADFile *, OCADStringEntry *);


typedef enum _SegmentType { MoveTo, SegmentLineTo, CurveTo, ClosePath } SegmentType;

typedef bool (*IntPathCallback)(void *, SegmentType, s32 *);

#define OCAD_OK 0
#define OCAD_OUT_OF_MEMORY -1
#define OCAD_FILE_WAS_READONLY -2
#define OCAD_MMAP_FAILED -3
#define OCAD_INVALID_FORMAT -4
#define OCAD_MMAP_NOT_SUPPORTED -10


/** Iterates over an set of OCAD points, providing a list of PostScript-like path segments.
 */
bool ocad_path_iterate(u32 npts, const OCADPoint *pts, IntPathCallback callback, void *param);


/** Initializes the libocad library. Currently, this only does some checks to determine that
 *  the library was compiled correctly and structures are packed appropriately. Returns 0 on
 *  success, or exits the current process on failure.
 */
int ocad_init();


/** Shuts down the libocad library. Currently this is a no-op, but is included as part of the
 *  API to support future enhancements. Processes that use the library should call this function
 *  when they exit. The function always returns 0 for success.
 */
int ocad_shutdown();


/** Converts an OCAD point into a triplet of signed integers. The first value is the x-coordinate,
 *  second is the y-coordinate, and the third is a set of flags. The buffer should have at least
 *  three elements (3 * sizeof(s32)), or 12 bytes. A pointer to the beginning of the buffer is
 *  returned.
 */
const s32 *ocad_point(s32 *buf, const OCADPoint *pt);


/** Converts an OCAD point into a triplet of signed integers like ocad_point(), but advances the
 *  buffer pointer by three elements. This allows the caller to accumulate a list of points in
 *  a single buffer.
 */
const s32 *ocad_point2(s32 **pbuf, const OCADPoint *pt);


/** Converts an OCAD string (0-255 characters) to a zero-terminated string in the given buffer.
 *  The buffer must be at least 256 bytes long. A pointer to the zero-terminated string is
 *  returned.
 */
const char *ocad_str(char *buf, const str *ostr);


/** Converts an OCAD string (0-255 characters) to a zero-terminated string in the given buffer,
 *  returns a pointer to the zero-terminated string, and advances the buffer point just beyond
 *  the zero byte. Subsequent calls to this function can be made to build up a list of zero-
 *  terminated strings. This is useful, for example, in printf statements.
 *
 *  \code{.c}
 *  char tmp[512], *ptmp = &tmp;
 *  printf("%s %s", ocad_str2(&ptmp, str1), ocad_str2(&ptmp, str2));
 *  \endcode
 *
 *  Both OCAD strings will be appended to the buffer and separate pointers will be passed to
 *  printf. Note that the buffer size must be at least 256 times the number of strings to be
 *  converted before ptmp is reset, to prevent the potential for buffer overflow.
 */
const char *ocad_str2(char **buf, const str *ostr);


/** Calculates the bounds of the given path and returns it in the provided rectangle. Returns
 *  FALSE and does not modify the rectangle if and only if npts is equal to zero. Otherwise,
 *  the rectangle is modified and TRUE is returned.
 *
 *  The "prect" array must have at least four elements. The bounds are returned in the order
 *  { minX, minY, maxX, maxY }.
 */
bool ocad_path_bounds(s32 *prect, u32 npts, const OCADPoint *pts);


/** Calculates the bounds of the given path and returns it in the provided rectangle. Returns
 *  FALSE and does not modify the rectangle if and only if npts is equal to zero. Otherwise,
 *  the rectangle is modified and TRUE is returned.
 */
bool ocad_path_bounds_rect(OCADRect *prect, u32 npts, const OCADPoint *pts);


/** Grows the given rectangle by the given amount, which may be negative. This method will not
 *  validate the size of rectangle. Always returns TRUE.
 */
bool ocad_rect_grow(OCADRect *prect, s32 amount);


/** Returns true if the two rectangle intersect, FALSE otherwise.
 */
bool ocad_rect_intersects(const OCADRect *r1, const OCADRect *r2);


/** Converts an OCAD string of the correct type into an OCADBackground structure.
 */
int ocad_to_background(OCADBackground *bg, OCADCString *templ);

/** Creates a new OCADFile struct in memory. Unused parts of the file buffer are set to zero.
 * 
 *  Returns OCAD_OK on success, or OCAD_OUT_OF_MEMORY.
 */
int ocad_file_new(OCADFile **pfile);

/** Makes sure that 'amount' number of bytes are reserved in the file's buffer
 *  in addition to the already used space. Sets newly reserved memory to zero.
 *  Returns OCAD_OK or OCAD_OUT_OF_MEMORY.
 * 
 *  WARNING: be extremely careful with this, as it might invalidate pointers to the buffer!
 */
int ocad_file_reserve(OCADFile *file, int amount);

/** Opens a file with the given filename. The file is loaded into memory and is accessible through
 *  the various ocad_file_* methods. The first argument can either be a pointer to a pre-allocated
 *  OCADFile object (e.g., on the stack), or nullptr to cause the object to be allocated on the heap.
 *  In the first case, the preallocated memory is considered to be uninitialized and will be zeroed
 *  before opening the file.
 *
 *  This function will verify the magic number and set up the OCAD file version.
 *
 *  An open OCADFile can be closed with ocad_file_close().
 *
 *  Returns OCAD_OK on success, or one of the following error codes:<ul>
 *      <li>OCAD_OUT_OF_MEMORY:  Unable to allocate memory. errno was last set by malloc(2).
 *      <li>OCAD_FILE_WAS_READONLY:  Unable to open file in read/write mode. errno was last set by open(2).
 *      <li>OCAD_MMAP_FAILED:  Unable to memory map file. errno was last set by mmap(2) or read(2).
 *      <li>OCAD_INVALID_FORMAT:  Unknown file format.
 *  </ul>
 */
int ocad_file_open(OCADFile **pfile, const char *filename);


/** Behaves exactly like ocad_file_open(), except that the system attempts to open the file via memory
 *  mapping.
 *
 *  Returns 0 on success, one of the error codes returned by ocad_file_open(), or one of the following:<ul>
 *     <li>OCAD_MMAP_NOT_SUPPORTED: Memory mapping is not supported with this combination of system and libraries.
 * </ul>
 */
int ocad_file_open_mapped(OCADFile **pfile, const char *filename);


/** Behaves exactly like ocad_file_open(), except that the given buffer must contain the map data.
 *  The buffer must be allocated with malloc(). Ownership of the buffer is transferred to libocad.
 */
int ocad_file_open_memory(OCADFile **pfile, u8* buffer, u32 size);


/** Closes an open OCADFile. The memory map is cleared, the file is closed, and all memory buffers
 *  used by the object are deallocated. After calling this function, the OCADFile object can be
 *  reused by ocad_file_open() without causing a memory leak.
 *
 *  Always returns OCAD_OK for success.
 */
int ocad_file_close(OCADFile *pfile);


/** Optimizes and repairs an open OCADFile. The file is reordered into Header, Colors, Setup, Symbols,
 *  Objects, and Strings. Entities of the same type are brought together into a contiguous section
 *  of file and any free space is compacted. The entity indexes are likewise compacted and cached data
 *  in the entities and index entries is regenerated, as follows:
 *
 * <p> Symbols: The "extent" field is recalculated from the symbol properties.
 * <p> Objects: The bounding rectangle is recalculated from the path and symbol.
 */
int ocad_file_compact(OCADFile *pfile);


/** Saves an open OCADFile to the given filename.
 *
 *  Returns 0 on success, or one of the following error codes:
 *      -2:  Unable to open file for writing errno was last set by open(2).
 *      -3:  Unable to completely write data to the file. errno was last set by write(2).
 */
int ocad_file_save_as(OCADFile *pfile, const char *filename);


/** Calculates the bounding rectangle of all objects in the file.
 */
bool ocad_file_bounds(OCADFile *file, OCADRect *rect);


/** Exports part or all of an OCAD file to a specific format. The output stream must be preset in the
 *  options structure. You can use ocad_export_file() to export to a file. The options parameter
 *  must be binary compatible with OCADExportOptions.
 */
int ocad_export(OCADFile *pfile, void *options);


/** Exports part or all of an OCAD file to a specific file. The "output" field in the options is
 *  ignored and will be overwritten by a stream for the provided filename. The options parameter
 *  must be binary compatible with OCADExportOptions.
 */
int ocad_export_file(OCADFile *pfile, const char *filename, void *options);


/** Fills in the provided matrix with a transformation from map coordinates to real-world coordinates.
 *  Returns TRUE if the matrix was computed, FALSE if it could not be computed because the file is
 *  invalid or the real-world coordinate flag is unset (in which case the parameters are invalid.)
 */
bool ocad_setup_world_matrix(OCADFile *pfile, Transform *matrix);


/** Returns the number of colors defined in the color table, or -1 if the file isn't valid.
 */
int ocad_color_count(OCADFile *pfile);


/** Returns a pointer to a particular color in the color table. If index is less than zero or greater
 *  than the maximum color index, then nullptr is returned. Otherwise, a valid pointer to a color is
 *  returned. Note that the color index defined the order that colors are drawn (from top to bottom),
 *  and is independent from the color number that is used by symbol definition. The color number is
 *  available from (OCADColor *)->number.
 */
OCADColor *ocad_color_at(OCADFile *pfile, int index);


/** Returns a pointer to a color in the color table with the given number. If a color with that number
 *  is not found, nullptr is returned. 
 */
OCADColor *ocad_color(OCADFile *pfile, u8 number);


/** Returns the number of spot colors defined in the separations table.
 *  Returns -1 if the file isn't valid, or returns the negative value if the
 *  number in the file exceeds 32.
 */
int ocad_separation_count(OCADFile *pfile);


/** Returns a pointer to a particular spot color definition in the separations
 *  table. If index is less than zero, greater than the maximum color index or
 *  greater than 32, then nullptr is returned. The index corresponds to the index
 *  in the spot color table at OCADColor::spot.
 */
OCADColorSeparation *ocad_separation_at(OCADFile *pfile, int index);


/** Converts an OCADColor to an RGB triplet in the provided array of ints. The RGB values range from
 *  0 to 255, and the array must have at least three valid elements.
 */
void ocad_color_to_rgb(const OCADColor *c, int *arr);


/** Converts an OCADColor to an RGB triplet in the provided array of floats. The RGB values range from
 *  0.0f to 1.0f, and the array must have at least three valid elements.
 */
void ocad_color_to_rgbf(const OCADColor *c, float *arr);


/** Returns a pointer to the first symbol index block, or nullptr if the file isn't valid. Also returns
 *  nullptr if the file contains no symbols.
 */
OCADSymbolIndex *ocad_symidx_first(OCADFile *pfile);


/** Returns a pointer to the next symbol index block after the given one, or nullptr if there is none.
 */
OCADSymbolIndex *ocad_symidx_next(OCADFile *pfile, OCADSymbolIndex *current);


/** Returns the number of symbols defined in the file, or -1 if the file is invalid.
 */
int ocad_symbol_count(OCADFile *pfile);


/** Adds a new symbol with the given size in bytes to the file and returns a pointer to the new symbol.
 */
OCADSymbol *ocad_symbol_new(OCADFile *pfile, int size);


/** Returns a pointer to the symbol in the specified location within the index block, or nullptr if there
 *  is no such symbol.
 */
OCADSymbol *ocad_symbol_at(OCADFile *pfile, OCADSymbolIndex *current, int index);


/** Finds the symbol with a particular number, or nullptr if no such symbol exists.
 */
OCADSymbol *ocad_symbol(OCADFile *pfile, word number);


/** Returns TRUE if the symbol uses the given color number, FALSE otherwise.
 */
bool ocad_symbol_uses_color(const OCADSymbol *symbol, s16 number);


/** Iterates over all symbols in the file. The extra parameter is passed to the callback function,
 *  together with a pointer to the file and a pointer to the current symbol. The callback should
 *  return TRUE to continue iteration or FALSE to abort iteration. This function returns TRUE if
 *  the iteration completed successfully, FALSE if it was aborted by the callback.
 */
bool ocad_symbol_iterate(OCADFile *pfile, OCADSymbolCallback callback, void *param);


/** Iterates over all symbol elements in the given location. ngrp is the number of 8-byte groups, 
 *  starting at the pts pointer. Currently this is used in two places; within point objects, and within
 *  area objects that have a pattern defined. The callback should
 *  return TRUE to continue iteration or FALSE to abort iteration. This function returns TRUE if
 *  the iteration completed successfully, FALSE if it was aborted by the callback.
 */
bool ocad_symbol_element_iterate(s16 ngrp, OCADPoint *pts, OCADSymbolElementCallback callback, void *param);


/** Returns TRUE if any of the sequence of symbol elements at the given location use the specified color.
 */ 
bool ocad_symbol_elements_use_color(s16 ngrp, OCADPoint *pts, s16 number);


/** Returns a pointer to the first object index block, or nullptr if the file isn't valid. Also returns
 *  nullptr if the file contains no object.
 */
OCADObjectIndex *ocad_objidx_first(OCADFile *pfile);


/** Returns a pointer to the next object index block after the given one, or nullptr if there is none.
 */
OCADObjectIndex *ocad_objidx_next(OCADFile *pfile, OCADObjectIndex *current);


/** Returns a pointer to the given object index entry, or nullptr if the file isn't valid. Also returns
 *  nullptr if the index is out of range, but always returns a valid pointer if the file, index block,
 *  and index are valid.
 */
OCADObjectEntry *ocad_object_entry_at(OCADFile *pfile, OCADObjectIndex *current, int index);


/** Recalculates the bounding rectangle and updates the size and symbol fields in an object index
 *  entry. The entry's pointer and size field are not affected, and the object passed to this function
 *  doesn't need to be the same as the object referenced by the entry (although it should end up that
 *  way). The extent of the object's symbol is included in the bounding rectangle.
 *
 *  You usually shouldn't need to call this method directly. It is implicitly used by
 *  ocad_object_add(), ocad_object_replace(), and ocad_file_compact(). However, it's made available
 *  in case you need to selectively repair a particular index entry.
 */
void ocad_object_entry_refresh(OCADFile *pfile, OCADObjectEntry *entry, OCADObject *object);


/** Returns a pointer to the first empty object index entry large enough to fit an object with the
 *  given number of points. If no suitable object index entry is found, a new entry will be created.
 *  This method will only return nullptr if the file isn't valid or if there is a memory allocation
 *  problem.
 *
 *  The returned entry will have its ptr and npts fields set; the caller is responsible for writing
 *  the object into the location pointed to by ptr, and setting the symbol, min, and max fields in
 *  the index entry. ocad_object_entry_refresh provides an easy way to sync these extra fields with
 *  an existing object.
 */
OCADObjectEntry *ocad_object_entry_new(OCADFile *pfile, u32 npts);


/** Removes the object at the given object index entry. Returns 0 on success, -1 if the file isn't
 *  valid. The object's symbol number is set to zero, and the entry's symbol number is set to zero.
 *  The entry becomes eligible to be returned by a call to ocad_object_entry_new(), if it is large
 *  enough for the new object.
 */
int ocad_object_remove(OCADFile *pfile, OCADObjectEntry *entry);


/** Iterates over all object entries in the file.
 */
bool ocad_object_entry_iterate(OCADFile *pfile, OCADObjectEntryCallback callback, void *param);


/** Returns a pointer to the object in the specified location within the index block, or nullptr if there
 *  is no such object.
 */
OCADObject *ocad_object_at(OCADFile *pfile, OCADObjectIndex *current, int index);


/** Returns a pointer to an object, given a valid pointer to its index entry. Returns nullptr if the
 *  file isn't valid or the index entry is empty.
 */
OCADObject *ocad_object(OCADFile *pfile, OCADObjectEntry *entry);


/** Iterates over all objects in the file.
 */
bool ocad_object_iterate(OCADFile *pfile, OCADObjectCallback callback, void *param);


/** Returns the storage size of an OCAD object, in bytes.
 */
u32 ocad_object_size(const OCADObject *object);


/** Returns the storage size of an OCAD object, in bytes, given the number of points.
 */
u32 ocad_object_size_npts(u32 npts);


/** Allocates space for a temporary OCAD object, optionally copying it from an existing object. The
 *  allocated space is the size of the largest object supported by the file format (32767 points).
 *  This object can then be manipulated with the ocad_object_* functions, and finally saved back into
 *  the file with ocad_object_add() or ocad_object_replace().
 *
 *  You may pass a nullptr parameter to create a new, empty object.
 */
OCADObject *ocad_object_alloc(const OCADObject *source);


/** Adds a new object to the file. The size field should be set correctly prior to calling this function.
 *  This method can be used either to add a new object created by ocad_object_alloc(), or to duplicate
 *  an object by passing a pointer to an object already in the file.
 *
 *  The provided object is copied into the file and is left unchanged by this call. A pointer to the new
 *  object is returned, or nullptr if the object could not be added.
 */
OCADObject *ocad_object_add(OCADFile *file, const OCADObject *object, OCADObjectEntry** out_entry);

/** Returns a pointer to the first string index block, or nullptr if the file isn't valid. Also returns
 *  nullptr if the file contains no object.
 */
OCADStringIndex *ocad_string_index_first(OCADFile *pfile);


/** Returns a pointer to the next string index block after the given one, or nullptr if there is none.
 */
OCADStringIndex *ocad_string_index_next(OCADFile *pfile, OCADStringIndex *current);


/** Returns a pointer to the given string index entry, or nullptr if the file isn't valid. Also returns
 *  nullptr if the index is out of range, but always returns a valid pointer if the file, index block,
 *  and index are valid.
 */
OCADStringEntry *ocad_string_entry_at(OCADFile *pfile, OCADStringIndex *current, int index);


/** Creates a new string entry large enough to fit the specified number of bytes. If there is an
 *  empty string index entry of sufficient size, it will be used; otherwise, a new entry will be
 *  allocated. The entry returned will have its ptr and size fields set; the size field may be larger
 *  than the size requested in the parameter. The caller is responsible for copying data into the
 *  actual string (retrieved via ocad_string()). If there is not enough memory to create a new
 *  entry, nullptr is returned.
 */
OCADStringEntry *ocad_string_entry_new(OCADFile *pfile, u32 size);


/** Removes the string at the given entry. The string type is set to zero and the entry becomes
 *  eligibile to be returned by ocad_string_entry_new().
 */
int ocad_string_remove(OCADFile *pfile, OCADStringEntry *entry);


/** Iterates over all string entries in the file.
 */
bool ocad_string_entry_iterate(OCADFile *pfile, OCADStringEntryCallback callback, void *param);


/** Returns a pointer to the string in the specified location within the index block, or nullptr if there
 *  is no such string.
 */
OCADCString *ocad_string_at(OCADFile *pfile, OCADStringIndex *current, int index);


/** Returns a pointer to a string, given a valid pointer to its index entry. Returns nullptr if the
 *  file isn't valid or the index entry is empty.
 */
OCADCString *ocad_string(OCADFile *pfile, OCADStringEntry *entry);


/** Adds a background template to the file.
 */
int ocad_string_add_background(OCADFile *pfile, OCADBackground *bg);


/** Initializes an OCADPaintData object suitable for the given file.
 */
int ocad_paint_data_init(OCADFile *pfile, OCADPaintData *data);


/** Frees the memory used by an OCADPaintData object.
 */
int ocad_paint_data_free(OCADPaintData *data);


/** Initializes the given OCADPaintData object and fills it with references to all objects intersecting
 *  the specified rectangle. The rectangle may be nullptr to indicate no spatial filtering.
 *
 *  If this call completes successfully, then the OCADPaintData object can be passed to ocad_paint().
 *  The object can be cached and used for later calls to ocad_paint(), if desired.
 */
void ocad_paint_data_fill(OCADPaintData *pdata, const OCADRect *rect);


/** Paints the contents of a valid OCADPaintData object, by creating successive calls to the methods
 *  defined in the callback object. The callback can abort painting (by returning FALSE from
 *  callback->paint_object()), and if this happens the function will return FALSE. Otherwise, TRUE
 *  is returned.
 *
 *  The sequence of calls to the callback look like this:
 *
 *  set_color(OCADColor *color1);
 *    set_symbol(OCADSymbol *symbol1);
 *      paint_object(OCADObject *object1);
 *      paint_object(OCADObject *object2);
 *      paint_object(OCADObject *object3);
 *    set_symbol(OCADSymbol *symbol2);
 *      paint_object(OCADObject *object4);
 *  set_color(OCADColor *color2);
 *    set_symbol(OCADSymbol *symbol1);
 *      paint_object(OCADObject *object2);
 *
 *  Colors are selected in reverse index order (bottom to top), and objects with the same symbol are
 *  grouped together, to minimize the need for switching pen and brush properties in the target
 *  rendering context.
 */
bool ocad_paint(const OCADPaintData *pdata, OCADPaintCallback *callback, void *param);


/** Combines two OCADRects
 */
void ocad_rect_union(OCADRect *into, const OCADRect *other);


// Debugging function
void dump_bytes(u8 *base, u32 size);

#ifdef __cplusplus
}
#endif


#endif

