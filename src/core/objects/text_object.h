/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2014, 2015 Kai Pastor
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


#ifndef OPENORIENTEERING_OBJECT_TEXT_H
#define OPENORIENTEERING_OBJECT_TEXT_H

#include <memory>
#include <vector>

#include <QtGlobal>
#include <QString>
#include <QFontMetricsF>
#include <QPointF>
#include <QRectF>
#include <QTransform>

#include "core/map_coord.h"
#include "core/objects/object.h"

// IWYU pragma: no_forward_declare QPointF
// IWYU pragma: no_forward_declare QRectF
// IWYU pragma: no_forward_declare QTransform

namespace OpenOrienteering {

class Symbol;


/** 
 * TextObjectPartInfo contains layout information for a continuous sequence of printable characters
 * in a longer text.
 * 
 * Use the implicit initializer list constructor to create a new object of this class.
 */
class TextObjectPartInfo
{
public:
	QString part_text;		/// The sequence of printable characters which makes up this part
	int start_index;		/// The index of the part's first character in the original string
	int end_index;			/// The index of the part's last character in the original string
	double part_x;			/// The left endpoint of the baseline of this part in text coordinates
	double width;			/// The width of the rendered part in text coordinates
	QFontMetricsF metrics;	/// The metrics of the font that is used to render the part
	
	/** Get the horizontal position of a particular character in a part.
	 *  @param index the index of the character in the original string
	 *  @return      the character's horizontal position in text coordinates
	 */
	double getX(int index) const;
	
	/** Find the index of the character corresponding to a particular position.
	 *  @param pos_x the position for which the index is requested
	 *  @return      the character's index in the original string
	 */
	int getIndex(double pos_x) const;
};



/** TextObjectLineInfo contains layout information for a single line
 * in a longer text. A line is a sequence of different parts.
 */
struct TextObjectLineInfo
{
	/** A sequence container of TextObjectPartInfo objects
	*/
	typedef std::vector<TextObjectPartInfo> PartInfoContainer;

	int start_index;		/// The index of the part's first character in the original string
	int end_index;			/// The index of the part's last character in the original string
	bool paragraph_end;		/// Is this line the end of a paragraph?
	double line_x;			/// The left endpoint of the baseline of this line in text coordinates
	double line_y;			/// The vertical position of the baseline of this line in text coordinates
	double width;			/// The total width of the text in this line
	double ascent;			/// The height of the rendered text above the baseline 
	double descent;			/// The height of the rendered text below the baseline 
	PartInfoContainer part_infos; /// The sequence of parts which make up this line
	
	/** Get the horizontal position of a particular character in a line.
	 *  @param pos the index of the character in the original string
	 *  @return    the character's horizontal position in text coordinates
	 */
	double getX(int pos) const;
	
	/** Find the index of the character corresponding to a particular position.
	 *  @param pos_x the position for which the index is requested
	 *  @return      the character's index in the original string
	 */
	int getIndex(double pos_x) const;
};

/** A text object.
 * 
 *  A text object is an instance of a text symbol. 
 *  Its position may be specified by a single coordinate (the anchor point) 
 *  or by two coordinates (word wrap box: 
 *  first coordinate specifies the coordinate of the midpoint,
 *  second coordinates specifies the width and height).
 * 
 * TODO: the way of defining word wrap boxes is inconvenient, as the second
 * coordinate does not specify a real coordinate in this case, but is misused
 * as extent. Change this?
 */
class TextObject : public Object  // clazy:exclude=copyable-polymorphic
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
	
	/** A sequence container of TextObjectLineInfo objects
	*/
	typedef std::vector<TextObjectLineInfo> LineInfoContainer;

	/** Construct a new text object.
	 *  If a symbol is specified, it must be a text symbol.
	 *  @param symbol the text symbol (optional)
	 */
	explicit TextObject(const Symbol* symbol = nullptr);
	
protected:
	/** Constructs a TextObject, initalized from the given prototype. */
	explicit TextObject(const TextObject& proto);
	
public:
	/** Creates a duplicate of the text object.
	 *  @return a new object with same text, symbol and formatting.
	 */
	TextObject* duplicate() const override;

	TextObject& operator=(const TextObject&) = delete;
	
	void copyFrom(const Object& other) override;
	
	
	/** Returns true if the text object has a single anchor, false if it has as word wrap box
	 */
	bool hasSingleAnchor() const;
	
	/** Sets the position of the anchor point to (x,y). 
	 *  This will drop an existing word wrap box.
	 */
	void setAnchorPosition(qint32 x, qint32 y);
	
	/** Sets the position of the anchor point to coord. 
	 *  This will drop an existing word wrap box.
	 */
	void setAnchorPosition(const MapCoord& coord);
	
	/** Sets the position of the anchor point to coord. 
	 *  This will drop an existing word wrap box.
	 */
	void setAnchorPosition(const MapCoordF& coord);
	
	/** Returns the coordinates of the anchor point or midpoint */
	MapCoordF getAnchorCoordF() const;
	
	
	void transform(const QTransform& t) override;
	
	
	/** Set position and size. 
	 *  The midpoint is set to (mid_x, mid_y), the size is specifed by the parameters
	 *  width and heigt.
	 */
	void setBox(qint32 mid_x, qint32 mid_y, qreal width, qreal height);
	
	/** Set size. 
	 */
	void setBoxSize(const MapCoord& size);
	
	/** Returns the size as a MapCoord.
	 */
	MapCoord getBoxSize() const { return size; }
	
	/** Returns the width of the word wrap box.
	 *  The text object must have a specified size.
	 */
	qreal getBoxWidth() const;
	
	/** Returns the height of the word wrap box.
	 *  The text object must have a specified size.
	 */
	qreal getBoxHeight() const;
	
	
	/**
	 * @brief Returns the positions of the control points.
	 * 
	 * The returned vector may have one or four members, depending on the type
	 * of object.
	 */
	std::vector<QPointF> controlPoints() const;
	
	
	/**
	 * Scales position and box, with the given scaling center.
	 */
	void scale(const MapCoordF& center, double factor) override;
	
	/**
	 * Scales position and box, with the center (0, 0).
	 */
	void scale(double factor_x, double factor_y) override;
	
	
	/** Sets the text of the object.
	 */
	void setText(const QString& text);
	
	/** Returns the text of the object.
	 */
	const QString& getText() const;
	
	/** Sets the horizontal alignment of the text.
	 */ 
	void setHorizontalAlignment(HorizontalAlignment h_align);
	
	/** Returns the horizontal alignment of the text.
	 */ 
	HorizontalAlignment getHorizontalAlignment() const;
	
	/** Sets the vertical alignment of the text.
	 */ 
	void setVerticalAlignment(VerticalAlignment v_align);
	
	/** Returns the vertical alignment of the text.
	 */ 
	VerticalAlignment getVerticalAlignment() const;
	
	
	/** Sets the rotation of the text.
	 *  The rotation is measured in radians. The center of rotation is the anchor point.
	 */ 
	void setRotation(qreal new_rotation);
	
	/** Returns the rotation of the text.
	 *  The rotation is measured in radians. The center of rotation is the anchor point.
	 */ 
	qreal getRotation() const;
	
	
	bool intersectsBox(const QRectF& box) const override;
	
	
	/** Returns a QTransform from text coordinates to map coordinates.
	 */
	QTransform calcTextToMapTransform() const;
	
	/** Returns a QTransform from map coordinates to text coordinates.
	 */
	QTransform calcMapToTextTransform() const;
	
	
	/** Return the number of rendered lines.
	 * For a text object with a word wrap box, the number of rendered lines
	 * may be higher than the number of explicit line breaks in the original text.
	 */
	int getNumLines() const;
	
	/** Returns the layout information about a particular line.
	 */
	TextObjectLineInfo* getLineInfo(int i);
	
	/** Returns the layout information about a particular line.
	 */
	const TextObjectLineInfo* getLineInfo(int i) const;
	
	/** Return the index of the character or the line number corresponding to a particular map coordinate.
	 *  Returns -1 if the coordinate is not at a text position. 
	 *  If find_line_only is true, the line number is returned, otherwise the index of the character.
	 */
	int calcTextPositionAt(const MapCoordF& coord, bool find_line_only) const;
	
	/** Return the index of the character or the line number corresponding to a particular text coordinate.
	 *  Returns -1 if the coordinate is not at a text position.
	 *  If find_line_only is true, the line number is returned, otherwise the index of the character.
	 */
	int calcTextPositionAt(const QPointF& coord, bool find_line_only) const;

	/** Returns the line number for a particular index in the text.
	 */
	int findLineForIndex(int index) const;
	
	/** Returns the line layout information for particular index.
	 */
	const TextObjectLineInfo& findLineInfoForIndex(int index) const;
	
	/** Prepare the text layout information.
	 */
	void prepareLineInfos() const;
	
private:
	QString text;
	qreal rotation;	// 0 to 2*M_PI
	HorizontalAlignment h_align;
	VerticalAlignment v_align;
	
	bool has_single_anchor = true;
	MapCoord size;
	
	/** Information about the text layout.
	 */
	mutable LineInfoContainer line_infos;
};



//### TextObjectPartInfo inline code ###

inline
double TextObjectPartInfo::getX(int index) const
{
	return part_x + metrics.width(part_text.left(index - start_index));
}



//### TextObject inline code ###

inline
bool TextObject::hasSingleAnchor() const
{
	return has_single_anchor;
}

inline
qreal TextObject::getBoxWidth() const
{
	Q_ASSERT(!hasSingleAnchor());
	return size.x();
}

inline
qreal TextObject::getBoxHeight() const
{
	Q_ASSERT(!hasSingleAnchor());
	return size.y();
}

inline
const QString&TextObject::getText() const
{
	return text;
}

inline
TextObject::HorizontalAlignment TextObject::getHorizontalAlignment() const
{
	return h_align;
}

inline
TextObject::VerticalAlignment TextObject::getVerticalAlignment() const
{
	return v_align;
}

inline
qreal TextObject::getRotation() const
{
	return rotation;
}

inline
int TextObject::getNumLines() const
{
	return (int)line_infos.size();
}

inline
TextObjectLineInfo*TextObject::getLineInfo(int i)
{
	return &line_infos[i];
}

inline
const TextObjectLineInfo*TextObject::getLineInfo(int i) const
{
	return &line_infos[i];
}


}  // namespace OpenOrienteering

#endif
