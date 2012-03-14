/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_OBJECT_TEXT_H_
#define _OPENORIENTEERING_OBJECT_TEXT_H_

#include <cassert>
#include <vector>

#include <QString>
#include <QFontMetricsF>
#include <QPointF>
#include <QTransform>

#include "object.h"
#include "map_coord.h"

class Symbol;

struct TextObjectPartInfo
{
	QString text;			// substring shown in this part
	int start_index;		// line start character index in original string
	int end_index;
	double part_x;			// left endpoint of the baseline of this part of text in text coordinates
	double width;
	QFontMetricsF metrics;
	
	inline TextObjectPartInfo(const QString& text, int start_index, int end_index, double part_x, double width, const QFontMetricsF& metrics)
	 : text(text), start_index(start_index), end_index(end_index), part_x(part_x), width(width), metrics(metrics) {}
	
	/* Get the horizontal position of a particular character in a part.
	 * @param pos the index of the character, relative to start_index
	 * @ret   the character's horizontal position
	 */
	float getX(int pos) const;
	
	/* Get the index of the character in the original string that is pointed to by a point
	 */
	 int getCharacterIndex(const QPointF& point) const;
};

typedef std::vector<TextObjectPartInfo> PartInfoContainer;

struct TextObjectLineInfo
{
	int start_index;		// line start character index in original string
	int end_index;			// line end character index in original string
	double line_x;			// left endpoint of the baseline of this line of text in text coordinates
	double line_y;			// vertical position of the baseline of this line of text in text coordinates
	double width;			// total width of the text in this line
	double ascent;			// 
	double descent;			// 
	PartInfoContainer part_infos; // the distinct parts of the line
	
	inline TextObjectLineInfo(int start_index, int end_index, double line_x, double line_y, double width, double ascent, double descent, PartInfoContainer& part_infos)
	 : start_index(start_index), end_index(end_index), line_x(line_x), line_y(line_y), width(width), ascent(ascent), descent(descent), part_infos(part_infos) {
	   assert(start_index == part_infos.front().start_index);
	   assert(end_index == part_infos.back().end_index);
	}
	
	/* Get the horizontal position of a particular character in a line.
	 * @param pos the index of the character, relative to start_index
	 * @ret   the character's horizontal position
	 */
	float getX(int pos) const;
	
	/* Get the index of the character in the original string that is pointed to by a point
	 */
	int getCharacterIndex(const QPointF& point) const;
};

typedef std::vector<TextObjectLineInfo> LineInfoContainer;

/// Object type which can only be used for text symbols.
/// Contains either 1 coordinate (single anchor point) or 2 coordinates (word wrap box: midpoint coordinate and width/height in second coordinate)
class TextObject : public Object
{
public:
	enum HorizontalAlignment
	{
		AlignLeft = 0,
		AlignHCenter = 1,
		AlignRight = 2
	};
	
	enum VerticalAlignment
	{
		AlignBaseline = 0,
		AlignTop = 1,
		AlignVCenter = 2,
		AlignBottom = 3
	};
	
	TextObject(Symbol* symbol = NULL);
	virtual Object* duplicate();
	
	inline bool hasSingleAnchor() const {return coords.size() == 1;}
	void setAnchorPosition(qint64 x, qint64 y);
	void setAnchorPosition(MapCoordF coord);
	void getAnchorPosition(qint64& x, qint64& y) const;	// or midpoint if a box is used
	MapCoordF getAnchorCoordF() const;
	void setBox(qint64 mid_x, qint64 mid_y, double width, double height);
	inline double getBoxWidth() const {assert(!hasSingleAnchor()); return coords[1].xd();}
	inline double getBoxHeight() const {assert(!hasSingleAnchor()); return coords[1].yd();}
	
	QTransform calcTextToMapTransform();
	QTransform calcMapToTextTransform();
	
	void setText(const QString& text);
	inline const QString& getText() const {return text;}
	
	void setHorizontalAlignment(HorizontalAlignment h_align);
	inline HorizontalAlignment getHorizontalAlignment() const {return h_align;}
	
	void setVerticalAlignment(VerticalAlignment v_align);
	inline VerticalAlignment getVerticalAlignment() const {return v_align;}
	
	void setRotation(float new_rotation);
	inline float getRotation() const {return rotation;}
	
	inline int getNumLineInfos() const {return (int)line_infos.size();}
	inline TextObjectLineInfo* getLineInfo(int i) {return &line_infos[i];}
	
	int calcTextPositionAt(QPointF coord, bool find_line_only);	    // returns -1 if the coordinate is not at a text position
	int calcTextPositionAt(MapCoordF coord, bool find_line_only);	// returns -1 if the coordinate is not at a text position
	int findLineForIndex(int index);
	TextObjectLineInfo* findLineInfoForIndex(int index);
	
	void prepareLineInfos(bool word_wrap, double max_width);
	
private:
	QString text;
	HorizontalAlignment h_align;
	VerticalAlignment v_align;
	float rotation;	// 0 to 2*M_PI
	
	/// when renderables are generated for this object by a call to update(), this is filled with information about the generated lines
	LineInfoContainer line_infos;
};

#endif
