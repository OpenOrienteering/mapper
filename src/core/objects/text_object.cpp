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


#include "text_object.h"

#include <QtMath>
#include <QChar>
#include <QLatin1Char>
#include <QPointF>

#include "settings.h"
#include "core/objects/object.h"
#include "core/symbols/text_symbol.h"
#include "core/symbols/symbol.h"

// IWYU pragma: no_forward_declare QPointF


namespace OpenOrienteering {

// ### TextObjectPartInfo ###

int TextObjectPartInfo::getIndex(double pos_x) const
{
	int left = 0;
	int right = part_text.length();
	while (right != left)	
	{
		int middle = (left + right) / 2;
		double x = part_x + metrics.width(part_text.left(middle));
		if (pos_x >= x)
		{
			if (middle >= right)
				return right;
			double next = part_x + metrics.width(part_text.left(middle + 1));
			if (pos_x < next)
				if (pos_x < (x + next) / 2)
					return middle;
				else
					return middle + 1;
			else
				left = middle + 1;
		}
		else // if (point.x() < x)
		{
			if (middle <= 0)
				return 0;
			double prev = part_x + metrics.width(part_text.left(middle - 1));
			if (pos_x > prev)
				if (pos_x > (x + prev) / 2)
					return middle;
				else
					return middle - 1;
			else
				right = middle - 1;
		}
	}
	return right;
}



// ### TextObjectLineInfo ###

double TextObjectLineInfo::getX(int index) const
{
	int num_parts = part_infos.size();
	int i = 0;
	for ( ; i < num_parts; i++)
	{
		const TextObjectPartInfo& part(part_infos.at(i));
		if (index <= part.end_index)
			return part.getX(index);
	}
	
	return line_x + width;
}

int TextObjectLineInfo::getIndex(double pos_x) const
{
// TODO: evaluate std::vector<TextObjectPartInfo>::iterator it;
	int num_parts = part_infos.size();
	for (int i=0; i < num_parts; i++)
	{
		if (part_infos.at(i).part_x > pos_x)
		{
			if (i==0)
				// before first part
				return start_index;
			else if (part_infos.at(i-1).part_x + part_infos.at(i-1).width < pos_x)
			{
				// between parts
				return (pos_x - (part_infos.at(i-1).part_x + part_infos.at(i-1).width) < part_infos.at(i).part_x - pos_x)
				  ? part_infos.at(i-1).end_index
				  : part_infos.at(i).start_index;
			}
			else
				// inside part
				return part_infos.at(i-1).start_index + part_infos.at(i-1).getIndex(pos_x);
		}
	}
	return part_infos.back().start_index + part_infos.at(num_parts-1).getIndex(pos_x);
}

// ### TextObject ###

TextObject::TextObject(const Symbol* symbol)
 : Object(Object::Text, symbol)
 , rotation(0)
 , h_align(AlignHCenter)
 , v_align(AlignVCenter)
{
	Q_ASSERT(!symbol || (symbol->getType() == Symbol::Text));
	coords.reserve(2); // Extra element used during saving
	coords.push_back(MapCoord(0, 0));
}

TextObject::TextObject(const TextObject& proto)
 : Object(proto)
 , text(proto.text)
 , rotation(proto.rotation)
 , h_align(proto.h_align)
 , v_align(proto.v_align)
 , has_single_anchor(proto.has_single_anchor)
 , size(proto.size)
 , line_infos(proto.line_infos)
{
	// nothing
}

TextObject* TextObject::duplicate() const
{
	return new TextObject(*this);
}

void TextObject::copyFrom(const Object& other)
{
	if (&other == this)
		return;
	
	Object::copyFrom(other);
	const TextObject& other_text = *other.asText();
	text = other_text.text;
	h_align = other_text.h_align;
	v_align = other_text.v_align;
	rotation = other_text.rotation;
	has_single_anchor = other_text.has_single_anchor;
	size = other_text.size;
	line_infos = other_text.line_infos;
}

void TextObject::setAnchorPosition(qint32 x, qint32 y)
{
	has_single_anchor = true;
	coords[0].setNativeX(x);
	coords[0].setNativeY(y);
	setOutputDirty();
}

void TextObject::setAnchorPosition(const MapCoord& coord)
{
	has_single_anchor = true;
	coords[0] = coord;
	setOutputDirty();
}

void TextObject::setAnchorPosition(const MapCoordF& coord)
{
	has_single_anchor = true;
	coords[0].setX(coord.x());
	coords[0].setY(coord.y());
	setOutputDirty();
}

MapCoordF TextObject::getAnchorCoordF() const
{
	return MapCoordF(coords[0]);
}


void TextObject::transform(const QTransform& t)
{
	if (t.isIdentity())
		return;
	
	auto& coord = coords.front();
	const auto p = t.map(MapCoordF{coord});
	coord.setX(p.x());
	coord.setY(p.y());
	setOutputDirty();
}



void TextObject::setBox(qint32 mid_x, qint32 mid_y, qreal width, qreal height)
{
	has_single_anchor = false;
	coords[0].setNativeX(mid_x);
	coords[0].setNativeY(mid_y);
	size = {width, height};
	setOutputDirty();
}

void TextObject::setBoxSize(const MapCoord& size)
{
	has_single_anchor = false;
	this->size = size;
	setOutputDirty();
}

std::vector<QPointF> TextObject::controlPoints() const
{
	auto anchor = getAnchorCoordF();
	std::vector<QPointF> handles(4, anchor);
	
	if (hasSingleAnchor())
	{
		handles.resize(1);
	}
	else
	{
		QTransform transform;
		transform.rotate(-qRadiansToDegrees(getRotation()));
		
		handles[0] += transform.map(QPointF(+getBoxWidth() / 2, -getBoxHeight() / 2));
		handles[1] += transform.map(QPointF(+getBoxWidth() / 2, +getBoxHeight() / 2));
		handles[2] += transform.map(QPointF(-getBoxWidth() / 2, +getBoxHeight() / 2));
		handles[3] += transform.map(QPointF(-getBoxWidth() / 2, -getBoxHeight() / 2));
	}
	
	return handles;
}



void TextObject::scale(const MapCoordF& center, double factor)
{
	coords.front() = MapCoord{center + (MapCoordF{coords.front()} - center) * factor};
	if (!has_single_anchor)
		size *= factor;
	setOutputDirty();
}


void TextObject::scale(double factor_x, double factor_y)
{
	auto& coord = coords.front();
	coord.setX(coord.x() * factor_x);
	coord.setY(coord.y() * factor_y);
	if (!has_single_anchor)
	{
		size.setX(size.x() * factor_x);
		size.setY(size.y() * factor_y);
	}
	setOutputDirty();
}



QTransform TextObject::calcTextToMapTransform() const
{
	const TextSymbol* text_symbol = reinterpret_cast<const TextSymbol*>(symbol);
	
	QTransform transform;
	double scaling = 1.0f / text_symbol->calculateInternalScaling();
	transform.translate(coords[0].x(), coords[0].y());
	if (rotation != 0)
		transform.rotate(-rotation * 180 / M_PI);
	transform.scale(scaling, scaling);
	
	return transform;
}

QTransform TextObject::calcMapToTextTransform() const
{
	const TextSymbol* text_symbol = reinterpret_cast<const TextSymbol*>(symbol);
	
	QTransform transform;
	double scaling = 1.0f / text_symbol->calculateInternalScaling();
	transform.scale(1.0f / scaling, 1.0f / scaling);
	if (rotation != 0)
		transform.rotate(rotation * 180 / M_PI);
	transform.translate(-coords[0].x(), -coords[0].y());
	
	return transform;
}

void TextObject::setText(const QString& text)
{
	this->text = text;
	this->text.remove(QLatin1Char('\r'));
	setOutputDirty();
}

void TextObject::setHorizontalAlignment(TextObject::HorizontalAlignment h_align)
{
	this->h_align = h_align;
	setOutputDirty();
}

void TextObject::setVerticalAlignment(TextObject::VerticalAlignment v_align)
{
	this->v_align = v_align;
	setOutputDirty();
}

void TextObject::setRotation(qreal new_rotation)
{
	rotation = new_rotation;
	setOutputDirty();
}

bool TextObject::intersectsBox(const QRectF& box) const
{
	return getExtent().intersects(box);
}

int TextObject::calcTextPositionAt(const MapCoordF& coord, bool find_line_only) const
{
	return calcTextPositionAt(calcMapToTextTransform().map(coord), find_line_only);
}

// FIXME actually this is two functions, selected by parameter find_line_only; make two functions or return TextObjectLineInfo reference
int TextObject::calcTextPositionAt(const QPointF& point, bool find_line_only) const
{
	auto click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
	
	for (int line = 0; line < getNumLines(); ++line)
	{
		const TextObjectLineInfo* line_info = getLineInfo(line);
		if (line_info->line_y - line_info->ascent > point.y())
			return -1;	// NOTE: Only true as long as every line has a bigger or equal y value than the line before
		
		if (point.x() < line_info->line_x - click_tolerance) continue;
		if (point.y() > line_info->line_y + line_info->descent) continue;
		if (point.x() > line_info->line_x + line_info->width + click_tolerance) continue;
		
		// Position in the line rect.
		if (find_line_only)
			return line;
		else
			return line_info->getIndex(point.x());
	}
	return -1;
}

int TextObject::findLineForIndex(int index) const
{
	int line_num = 0;
	for (int line = 1; line < getNumLines(); ++line)
	{
		const TextObjectLineInfo* line_info = getLineInfo(line);
		if (index < line_info->start_index)
			break;
		line_num = line;
	}
	return line_num;
}

const TextObjectLineInfo& TextObject::findLineInfoForIndex(int index) const
{
	const TextObjectLineInfo* line_info = getLineInfo(0);
	for (int line = 1; line < getNumLines(); ++line)
	{
		const TextObjectLineInfo* next_line_info = getLineInfo(line);
		if (index < next_line_info->start_index)
			break;
		line_info = next_line_info;
	}
	return *line_info;
}

void TextObject::prepareLineInfos() const
{
	const TextSymbol* text_symbol = reinterpret_cast<const TextSymbol*>(symbol);
	
	double scaling = text_symbol->calculateInternalScaling();
	QFontMetricsF metrics = text_symbol->getFontMetrics();
	double line_spacing = text_symbol->getLineSpacing() * metrics.lineSpacing();
	double paragraph_spacing = scaling * text_symbol->getParagraphSpacing() + (text_symbol->hasLineBelow() ? (scaling * (text_symbol->getLineBelowDistance() + text_symbol->getLineBelowWidth())) : 0);
	double ascent = metrics.ascent();
	
	bool word_wrap = ! hasSingleAnchor();
	double box_width  = word_wrap ? (scaling * getBoxWidth())  : 0.0;
	double box_height = word_wrap ? (scaling * getBoxHeight()) : 0.0;
	
	int text_end = text.length();
	const QLatin1Char line_break('\n');
	const QLatin1Char part_break('\t');
	const QLatin1Char word_break(' ');
	
	line_infos.clear();
	
	// Initialize offsets
	
	double line_x = 0.0;
	if (h_align == TextObject::AlignLeft)
		line_x -= 0.5 * box_width;
	else if (h_align == TextObject::AlignRight)
		line_x += 0.5 * box_width;

	double line_y = 0.0;
	if (v_align == TextObject::AlignTop || v_align == TextObject::AlignBaseline)
		line_y += -0.5 * box_height;
	if (v_align != TextObject::AlignBaseline)
		line_y += ascent;
	
	// Determine lines and parts
	
	//double next_line_x_offset = 0; // to keep indentation after word wrap in a line with tabs
	int num_paragraphs = 0;
	int line_num = 0;
	int line_start = 0;
	while (line_start <= text_end) 
	{
		// Initialize input line
		double line_width = 0.0;
		int line_end = text.indexOf(line_break, line_start);
		if (line_end == -1)
			line_end = text_end;
		bool paragraph_end = true;
		
		std::vector<TextObjectPartInfo> part_infos;
		
		int part_start = line_start;
		double part_x = line_x;
		
		while (part_start <= line_end)
		{
			// Initialize part (sequence of letters terminated by tab or line break)
			int part_end = text.indexOf(part_break, part_start);
			if (part_end == -1)
				part_end = line_end;
			else if (part_end > line_end)
				part_end = line_end;
			
			if (part_start > 0 && text[part_start - 1] == part_break)
				part_x = line_x + text_symbol->getNextTab(part_x - line_x);
			
			QString part = text.mid(part_start, part_end - part_start);
			double part_width = metrics.boundingRect(part).width();
			
			if (word_wrap)
			{
				// shrink overflowing part to maximum possible size
				while (part_x + part_width - line_x > box_width)
				{
					// find latest possible break
					int new_part_end =  text.lastIndexOf(word_break, part_end - 1);
					if (new_part_end <= part_start)
					{
						// part won't fit
						if (part_start > line_start)
						{
							// don't put another part on this line
							part_end = part_start - 1;
							paragraph_end = false;
						}
						break;
					}
					
					paragraph_end = false;
					
					// Shrink the part and the line
					part_end = new_part_end;
					part = text.mid(part_start, part_end - part_start);
					part_width = metrics.width(part);
					line_end = part_end;
				}
			}
			if (part_end < part_start)
				break;
			
			// Add the current part
			part_infos.push_back( { part, part_start, part_end, part_x, metrics.width(part), metrics } );
			
			// Advance to next part position
			part_start = part_end + 1;
			part_x += part_width;
		}
		
		TextObjectPartInfo& last_part_info = part_infos.back();
		line_end   = last_part_info.end_index;
		line_width = last_part_info.part_x + last_part_info.width - line_x;
		
		// Jump over whitespace after the end of the line and check if it contains a newline character to determine if it is a paragraph end
		int next_line_start = line_end + 1;
		/*while (next_line_start < text.size() && (text[next_line_start] == line_break || text[next_line_start] == part_break || text[next_line_start] == word_break))
		{
			if (text[next_line_start - 1] == line_break)
			{
				paragraph_end = true;
				break;
			}
			++next_line_start;
		}*/
		
		line_infos.push_back( { line_start, line_end, paragraph_end, line_x, line_y, line_width, metrics.ascent(), metrics.descent(), part_infos } );
		
		// Advance to next line
		line_y += line_spacing;
		if (paragraph_end)
		{
			line_y += paragraph_spacing;
			num_paragraphs++;
		}
		line_num++;
		line_start = next_line_start;
	}
	
	// Update the line and part offset for every other alignment than top-left or baseline-left
	
	double delta_y = 0.0;
	if (v_align == TextObject::AlignBottom || v_align == TextObject::AlignVCenter)
	{
		int num_lines = getNumLines();
		double height = ascent + (num_lines - 1) * line_spacing + (num_paragraphs - 1) * paragraph_spacing;
		
		if (v_align == TextObject::AlignVCenter)
			delta_y = -0.5 * height;
		else if (v_align == TextObject::AlignBottom)
			delta_y = -height + 0.5 * box_height;
	}
	
	if (delta_y != 0.0 || h_align != TextObject::AlignLeft)
	{
		int num_lines = getNumLines();
		for (int i = 0; i < num_lines; i++)
		{
			TextObjectLineInfo* line_info = &line_infos[i];
			
			double delta_x = 0.0;
			if (h_align == TextObject::AlignHCenter)
				delta_x = -0.5 * line_info->width;
			else if (h_align == TextObject::AlignRight)
				delta_x -= line_info->width;
			
			line_info->line_x += delta_x;
			line_info->line_y += delta_y;
			
			int num_parts = line_info->part_infos.size();
			for (int j = 0; j < num_parts; j++)
			{
				line_info->part_infos.at(j).part_x += delta_x;
			}
		}
	}
}


}  // namespace OpenOrienteering
