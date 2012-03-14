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


#include "object_text.h"

#include "symbol.h"
#include "symbol_text.h"
#include "map_editor.h"

float TextObjectPartInfo::getX(int pos) const
{
	return part_x + metrics.width(text.left(pos - start_index));
}

int TextObjectPartInfo::getCharacterIndex(const QPointF& point) const
{
	int left = 0;
	int right = text.length();
	while (right != left)	
	{
		int middle = (left + right) / 2;
		float x = part_x + metrics.width(text.left(middle));
		if (point.x() >= x)
		{
			if (middle >= right)
				return right;
			float next = part_x + metrics.width(text.left(middle + 1));
			if (point.x() < next)
				if (point.x() < (x + next) / 2)
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
			float prev = part_x + metrics.width(text.left(middle - 1));
			if (point.x() > prev)
				if (point.x() > (x + prev) / 2)
					return middle;
				else
					return middle - 1;
			else
				right = middle - 1;
		}
	}
	return right;
}

float TextObjectLineInfo::getX(int pos) const
{
	if (pos == 0)
		return line_x;
	
	pos += start_index;
	int num_parts = part_infos.size();
	int i = 0;
	for ( ; i < num_parts; i++)
	{
		const TextObjectPartInfo& part(part_infos.at(i));
		if (pos <= part.end_index)
			return part.getX(pos);
	}
	
	return line_x + width;
}

int TextObjectLineInfo::getCharacterIndex(const QPointF& point) const
{
// TODO: evaluate std::vector<TextObjectPartInfo>::iterator it;
	int num_parts = part_infos.size();
	for (int i=0; i < num_parts; i++)
	{
		if (part_infos.at(i).part_x > point.x())
		{
			if (i==0)
				// before first part
				return start_index;
			else if (part_infos.at(i-1).part_x + part_infos.at(i-1).width < point.x())
			{
				// between parts
				return (point.x() - (part_infos.at(i-1).part_x + part_infos.at(i-1).width) < part_infos.at(i).part_x - point.x())
				  ? part_infos.at(i-1).end_index
				  : part_infos.at(i).start_index;
			}
			else
				// inside part
				return part_infos.at(i-1).start_index + part_infos.at(i-1).getCharacterIndex(point);
		}
	}
	return part_infos.back().start_index + part_infos.at(num_parts-1).getCharacterIndex(point);
}

// ### TextObject ###

TextObject::TextObject(Symbol* symbol): Object(Object::Text, symbol)
{
	assert(!symbol || (symbol->getType() == Symbol::Text));
	coords.push_back(MapCoord(0, 0));
	text = "";
	h_align = AlignHCenter;
	v_align = AlignVCenter;
	rotation = 0;
}

Object* TextObject::duplicate()
{
	TextObject* new_text = new TextObject(symbol);
	new_text->coords = coords;
	
	new_text->text = text;
	new_text->h_align = h_align;
	new_text->v_align = v_align;
	new_text->rotation = rotation;
	return new_text;
}

void TextObject::setAnchorPosition(qint64 x, qint64 y)
{
	coords.resize(1);
	coords[0].setRawX(x);
	coords[0].setRawY(y);
	setOutputDirty();
}
void TextObject::setAnchorPosition(MapCoordF coord)
{
	coords.resize(1);
	coords[0].setX(coord.getX());
	coords[0].setY(coord.getY());
	setOutputDirty();
}
void TextObject::getAnchorPosition(qint64& x, qint64& y) const
{
	x = coords[0].rawX();
	y = coords[0].rawY();
}
MapCoordF TextObject::getAnchorCoordF() const
{
	return MapCoordF(coords[0]);
}
void TextObject::setBox(qint64 mid_x, qint64 mid_y, double width, double height)
{
	coords.resize(2);
	coords[0].setRawX(mid_x);
	coords[0].setRawY(mid_y);
	coords[1] = MapCoord(width, height);
	setOutputDirty();
}

QTransform TextObject::calcTextToMapTransform()
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	QTransform transform;
	double scaling = 1.0f / text_symbol->calculateInternalScaling();
	transform.translate(coords[0].xd(), coords[0].yd());
	if (rotation != 0)
		transform.rotate(-rotation * 180 / M_PI);
	transform.scale(scaling, scaling);
	
	return transform;
}
QTransform TextObject::calcMapToTextTransform()
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	QTransform transform;
	double scaling = 1.0f / text_symbol->calculateInternalScaling();
	transform.scale(1.0f / scaling, 1.0f / scaling);
	if (rotation != 0)
		transform.rotate(rotation * 180 / M_PI);
	transform.translate(-coords[0].xd(), -coords[0].yd());
	
	return transform;
}

void TextObject::setText(const QString& text)
{
	this->text = text;
	this->text.remove(QChar('\r'));
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

void TextObject::setRotation(float new_rotation)
{
	rotation = new_rotation;
	setOutputDirty();
}

int TextObject::calcTextPositionAt(MapCoordF coord, bool find_line_only)
{
	return calcTextPositionAt(calcMapToTextTransform().map(coord.toQPointF()), find_line_only);
}
	
// TODO click_tolerance is the only dependency on MapEditorTool; consider click_tolerance parameter
// FIXME actually this is two functions, selected by parameter find_line_only; make two functions or return TextObjectLineInfo reference
int TextObject::calcTextPositionAt(QPointF point, bool find_line_only)
{
	for (int line = 0; line < getNumLineInfos(); ++line)
	{
		TextObjectLineInfo* line_info = getLineInfo(line);
		if (line_info->line_y - line_info->ascent > point.y())
			return -1;	// NOTE: Only true as long as every line has a bigger or equal y value than the line before
		
		if (point.x() < line_info->line_x - MapEditorTool::click_tolerance) continue;
		if (point.y() > line_info->line_y + line_info->descent) continue;
		if (point.x() > line_info->line_x + line_info->width + MapEditorTool::click_tolerance) continue;
		
		// Position in the line rect.
		if (find_line_only)
			return line;
		else
			return line_info->getCharacterIndex(point);
	}
	return -1;
}

int TextObject::findLineForIndex(int index)
{
	int line_num = 0;
	for (int line = 1; line < getNumLineInfos(); ++line)
	{
		TextObjectLineInfo* line_info = getLineInfo(line);
		if (index < line_info->start_index)
			break;
		line_num = line;
	}
	return line_num;
}

TextObjectLineInfo* TextObject::findLineInfoForIndex(int index)
{
	TextObjectLineInfo* line_info = getLineInfo(0);
	for (int line = 1; line < getNumLineInfos(); ++line)
	{
		TextObjectLineInfo* next_line_info = getLineInfo(line);
		if (index < next_line_info->start_index)
			break;
		line_info = next_line_info;
	}
	return line_info;
}

void TextObject::prepareLineInfos(bool word_wrap, double max_width)
{
	TextSymbol* text_symbol = reinterpret_cast<TextSymbol*>(symbol);
	QFontMetricsF metrics = text_symbol->getFontMetrics();
	double line_spacing = text_symbol->getLineSpacing() * metrics.lineSpacing();
	
	int text_end = text.length();
	const QChar   line_break('\n');
	const QRegExp part_break("[\n\t]");
	const QRegExp word_break(word_wrap ? "[\n\t ]" : "[\n\t]");
	
	line_infos.clear();
	int line_num = 0;
	double line_y = 0.0;
	int pos = 0;
	while(pos <= text_end) 
	{
		// Initialize input line
		double line_width = 0.0;
		int line_start = pos;
		int line_end = text.indexOf(line_break, line_start);
		if (line_end == -1)
			line_end = text_end;

		std::vector<TextObjectPartInfo> part_infos;
		
		double part_x = 0.0;
		while (pos <= line_end)
		{
			// Initialize part 
			int part_start = pos;
			int part_end = text.indexOf(part_break, pos);
			if (part_end == -1)
				part_end = text_end;
			
			QString part = text.mid(part_start, part_end - part_start);
			double part_width = metrics.width(part);
			
			bool last_part = false;
			if (word_wrap)
			{
				// shrink overflowing part to maximum possible size
				while (part_x + part_width > max_width && part_end > 0)
				{
					last_part = true;
					int new_part_end =  text.lastIndexOf(word_break, part_end - 1);
					if (new_part_end < part_start)
					{
						// part won't fit
						if (part_start == line_start)
						{
							// Never wrap first part of input line
							break;
						}
						else
						{
							// terminate current input line with empty part
							part_end = part_start;
							part = "";
							part_width = 0.0;
							break;
						}
					}
					part_end = new_part_end;
					part = text.mid(part_start, part_end - part_start);
					part_width = metrics.width(part);
				}
			}
			if (last_part)
				line_end = part_end;
				
			// Add the current part
			part_infos.push_back(TextObjectPartInfo(part, part_start, part_end, part_x, metrics.width(part), metrics));
			line_width = part_x + part_width;
			
			// Advance to next part
			pos = part_end + 1;
			part_x = text_symbol->getNextTab(part_x + part_width);
			if (word_wrap && part_x >= max_width)
				// terminate current input line
				break; 
		}
		
		line_infos.push_back(TextObjectLineInfo(line_start, line_end, 0.0, line_y, line_width, metrics.ascent(), metrics.descent(), part_infos));

		// Advance to next line
		line_y += line_spacing;
		line_num++;
		pos = line_end + 1;
	}
}
