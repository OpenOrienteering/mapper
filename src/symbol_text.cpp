/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016 Kai Pastor
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


#include "symbol_text.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIODevice>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map_color.h"
#include "map.h"
#include "object_text.h"
#include "renderable_implementation.h"
#include "symbol_area.h"
#include "symbol_line.h"
#include "symbol_setting_dialog.h"
#include "util.h"
#include "util_gui.h"
#include "gui/widgets/color_dropdown.h"
#include "util/backports.h"
#include "util/scoped_signals_blocker.h"


/**
 * \todo Move translation items to TextSymbolSettings class.
 */
class DetermineFontSizeDialog
{
public:
	Q_DECLARE_TR_FUNCTIONS(DetermineFontSizeDialog)
};


const float TextSymbol::internal_point_size = 256;

TextSymbol::TextSymbol() : Symbol(Symbol::Text), metrics(QFont())
{
	color = NULL;
	font_family = QString::fromLatin1("Arial");
	font_size = 4 * 1000;
	bold = false;
	italic = false;
	underline = false;
	line_spacing = 1;
	paragraph_spacing = 0;
	character_spacing = 0;
	kerning = true;
	icon_text = QString{};
	framing = false;
	framing_color = NULL;
	framing_mode = LineFraming;
	framing_line_half_width = 200;
	framing_shadow_x_offset = 200;
	framing_shadow_y_offset = 200;
	line_below = false;
	line_below_color = NULL;
	line_below_width = 0;
	line_below_distance = 0;
	updateQFont();
}

TextSymbol::~TextSymbol()
{
}

Symbol* TextSymbol::duplicate(const MapColorMap* color_map) const
{
	TextSymbol* new_text = new TextSymbol();
	new_text->duplicateImplCommon(this);
	new_text->color = color_map ? color_map->value(color) : color;
	new_text->font_family = font_family;
	new_text->font_size = font_size;
	new_text->bold = bold;
	new_text->italic = italic;
	new_text->underline = underline;
	new_text->line_spacing = line_spacing;
	new_text->paragraph_spacing = paragraph_spacing;
	new_text->character_spacing = character_spacing;
	new_text->kerning = kerning;
	new_text->icon_text = icon_text;
	new_text->framing = framing;
	new_text->framing_color = color_map ? color_map->value(framing_color) : framing_color;
	new_text->framing_mode = framing_mode;
	new_text->framing_line_half_width = framing_line_half_width;
	new_text->framing_shadow_x_offset = framing_shadow_x_offset;
	new_text->framing_shadow_y_offset = framing_shadow_y_offset;
	new_text->line_below = line_below;
	new_text->line_below_color = color_map ? color_map->value(line_below_color) : line_below_color;
	new_text->line_below_width = line_below_width;
	new_text->line_below_distance = line_below_distance;
	new_text->custom_tabs = custom_tabs;
	new_text->updateQFont();
	return new_text;
}

void TextSymbol::createRenderables(
        const Object *object,
        const VirtualCoordVector &coords,
        ObjectRenderables &output,
        Symbol::RenderableOptions options) const
{
	Q_ASSERT(object);
	
	const TextObject* text_object = static_cast<const TextObject*>(object);
	text_object->prepareLineInfos();
	
	if (options.testFlag(Symbol::RenderBaselines))
	{
		createBaselineRenderables(text_object, coords, output);
	}
	else
	{
		auto anchor = coords[0];
		double anchor_x = anchor.x();
		double anchor_y = anchor.y();
		
		if (color)
			output.insertRenderable(new TextRenderable(this, text_object, color, anchor_x, anchor_y));
		
		if (line_below && line_below_color && line_below_width > 0)
			createLineBelowRenderables(object, output);
		
		if (framing && framing_color)
		{
			if (framing_mode == LineFraming && framing_line_half_width > 0)
			{
				output.insertRenderable(new TextFramingRenderable(this, text_object, framing_color, anchor_x, anchor_y));
			}
			else if (framing_mode == ShadowFraming)
			{
				output.insertRenderable(new TextRenderable(this, text_object, framing_color, anchor_x + 0.001 * framing_shadow_x_offset, anchor_y + 0.001 * framing_shadow_y_offset));
			}
		}
	}
}

void TextSymbol::createBaselineRenderables(
        const TextObject* text_object,
        const VirtualCoordVector& coords,
        ObjectRenderables& output) const
{
	const MapColor* dominant_color = guessDominantColor();
	if (dominant_color && text_object->getNumLines() > 0)
	{
		// Insert text boundary
		LineSymbol line_symbol;
		line_symbol.setColor(dominant_color);
		line_symbol.setLineWidth(0);
		
		const TextObjectLineInfo* line = text_object->getLineInfo(0);
		QRectF text_bbox(line->line_x, line->line_y - line->ascent, line->width, line->ascent + line->descent);
		for (int i = 1; i < text_object->getNumLines(); ++i)
		{
			const TextObjectLineInfo* line = text_object->getLineInfo(i);
			rectInclude(text_bbox, QRectF(line->line_x, line->line_y - line->ascent, line->width, line->ascent + line->descent));
		}
		
		Q_UNUSED(coords); // coords should be used for calcTextToMapTransform()
		QTransform text_to_map = text_object->calcTextToMapTransform();
		PathObject path;
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.topLeft())));
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.topRight())));
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.bottomRight())));
		path.addCoordinate(MapCoord(text_to_map.map(text_bbox.bottomLeft())));
		path.parts().front().setClosed(true, true);
		path.updatePathCoords();
		
		LineRenderable* line_renderable = new LineRenderable(&line_symbol, path.parts().front(), false);
		output.insertRenderable(line_renderable);
	}
}

void TextSymbol::createLineBelowRenderables(const Object* object, ObjectRenderables& output) const
{
	const TextObject* text_object = reinterpret_cast<const TextObject*>(object);
	if (text_object->getNumLines())
	{
		double scale_factor = calculateInternalScaling();
		AreaSymbol area_symbol;
		area_symbol.setColor(line_below_color);
		
		MapCoordVector  line_flags(4);
		MapCoordVectorF line_coords(4);
		VirtualPath line_path = { line_flags, line_coords };
		line_flags.back().setHolePoint(true);
		
		QTransform transform = text_object->calcTextToMapTransform();
		
		for (int i = 0; i < text_object->getNumLines(); ++i)
		{
			const TextObjectLineInfo* line_info = text_object->getLineInfo(i);
			if (!line_info->paragraph_end)
				continue;
			
			double line_below_x0;
			double line_below_x1;
			if (text_object->hasSingleAnchor())
			{
				line_below_x0 = line_info->line_x;
				line_below_x1 = line_below_x0 + line_info->width;
			}
			else
			{
				double box_width = text_object->getBoxWidth() * scale_factor;
				line_below_x0 = -0.5 * box_width;
				line_below_x1 = line_below_x0 + box_width;
			}
			double line_below_y0 = line_info->line_y + getLineBelowDistance() * scale_factor;
			double line_below_y1 = line_below_y0 + getLineBelowWidth() * scale_factor;
			line_coords[0] = MapCoordF(transform.map(QPointF(line_below_x0, line_below_y0)));
			line_coords[1] = MapCoordF(transform.map(QPointF(line_below_x1, line_below_y0)));
			line_coords[2] = MapCoordF(transform.map(QPointF(line_below_x1, line_below_y1)));
			line_coords[3] = MapCoordF(transform.map(QPointF(line_below_x0, line_below_y1)));
			
			line_path.path_coords.update(0);
			output.insertRenderable(new AreaRenderable(&area_symbol, line_path));
		}
	}
}

void TextSymbol::colorDeleted(const MapColor* c)
{
	auto changes = 0;
	if (c == color)
	{
		color = nullptr;
		++changes;
	}
	if (c == framing_color)
	{
		framing_color = nullptr;
		++changes;
	}
	if (c == line_below_color)
	{
		line_below_color = nullptr;
		++changes;
	}
	if (changes)
	{
		resetIcon();
	}
}

bool TextSymbol::containsColor(const MapColor* c) const
{
	return c == color
	       || c == framing_color
	       || c == line_below_color;
}

const MapColor* TextSymbol::guessDominantColor() const
{
	auto c = color;
	if (!c)
		c = framing_color;
	if (!c)
		c = line_below_color;
	return c;
}

void TextSymbol::scale(double factor)
{
	font_size = qRound(factor * font_size);
	framing_line_half_width = qRound(factor * framing_line_half_width);
	framing_shadow_x_offset = qRound(factor * framing_shadow_x_offset);
	framing_shadow_y_offset = qRound(factor * framing_shadow_y_offset);
	
	updateQFont();
	
	resetIcon();
}

void TextSymbol::updateQFont()
{
	qfont = QFont();
	qfont.setBold(bold);
	qfont.setItalic(italic);
	qfont.setUnderline(underline);
	qfont.setPixelSize(internal_point_size);
	qfont.setFamily(font_family);
	qfont.setHintingPreference(QFont::PreferNoHinting);
	qfont.setKerning(kerning);
	metrics = QFontMetricsF(qfont);
	qfont.setLetterSpacing(QFont::AbsoluteSpacing, metrics.width(QString(QLatin1Char{' '})) * character_spacing);
	
	qfont.setStyleStrategy(QFont::ForceOutline);

	metrics = QFontMetricsF(qfont);
	tab_interval = 8.0 * metrics.averageCharWidth();
}

#ifndef NO_NATIVE_FILE_FORMAT

bool TextSymbol::loadImpl(QIODevice* file, int version, Map* map)
{
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	loadString(file, font_family);
	file->read((char*)&font_size, sizeof(int));
	file->read((char*)&bold, sizeof(bool));
	file->read((char*)&italic, sizeof(bool));
	file->read((char*)&underline, sizeof(bool));
	file->read((char*)&line_spacing, sizeof(float));
	if (version >= 13)
		file->read((char*)&paragraph_spacing, sizeof(double));
	if (version >= 14)
		file->read((char*)&character_spacing, sizeof(double));
	if (version >= 12)
		file->read((char*)&kerning, sizeof(bool));
	if (version >= 19)
		loadString(file, icon_text);
	if (version >= 20)
	{
		file->read((char*)&framing, sizeof(bool));
		file->read((char*)&temp, sizeof(int));
		framing_color = map->getColor(temp);
		file->read((char*)&framing_mode, sizeof(int));
		file->read((char*)&framing_line_half_width, sizeof(int));
		file->read((char*)&framing_shadow_x_offset, sizeof(int));
		file->read((char*)&framing_shadow_y_offset, sizeof(int));	
	}
	if (version >= 13)
	{
		file->read((char*)&line_below, sizeof(bool));
		file->read((char*)&temp, sizeof(int));
		line_below_color = (temp >= 0) ? map->getColor(temp) : NULL;
		file->read((char*)&line_below_width, sizeof(int));
		file->read((char*)&line_below_distance, sizeof(int));
		int num_custom_tabs;
		file->read((char*)&num_custom_tabs, sizeof(int));
		custom_tabs.resize(num_custom_tabs);
		for (int i = 0; i < num_custom_tabs; ++i)
			file->read((char*)&custom_tabs[i], sizeof(int));
	}
	
	updateQFont();
	return true;
}

#endif

void TextSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement(QStringLiteral("text_symbol"));
	xml.writeAttribute(QStringLiteral("icon_text"), icon_text);
	
	xml.writeStartElement(QStringLiteral("font"));
	xml.writeAttribute(QStringLiteral("family"), font_family);
	xml.writeAttribute(QStringLiteral("size"), QString::number(font_size));
	if (bold)
		xml.writeAttribute(QStringLiteral("bold"), QStringLiteral("true"));
	if (italic)
		xml.writeAttribute(QStringLiteral("italic"), QStringLiteral("true"));
	if (underline)
		xml.writeAttribute(QStringLiteral("underline"), QStringLiteral("true"));
	xml.writeEndElement(/*font*/);
	
	xml.writeStartElement(QStringLiteral("text"));
	xml.writeAttribute(QStringLiteral("color"), QString::number(map.findColorIndex(color)));
	xml.writeAttribute(QStringLiteral("line_spacing"), QString::number(line_spacing));
	xml.writeAttribute(QStringLiteral("paragraph_spacing"), QString::number(paragraph_spacing));
	xml.writeAttribute(QStringLiteral("character_spacing"), QString::number(character_spacing));
	if (kerning)
		xml.writeAttribute(QStringLiteral("kerning"), QStringLiteral("true"));
	xml.writeEndElement(/*text*/);
	
	if (framing)
	{
		xml.writeStartElement(QStringLiteral("framing"));
		xml.writeAttribute(QStringLiteral("color"), QString::number(map.findColorIndex(framing_color)));
		xml.writeAttribute(QStringLiteral("mode"), QString::number(framing_mode));
		xml.writeAttribute(QStringLiteral("line_half_width"), QString::number(framing_line_half_width));
		xml.writeAttribute(QStringLiteral("shadow_x_offset"), QString::number(framing_shadow_x_offset));
		xml.writeAttribute(QStringLiteral("shadow_y_offset"), QString::number(framing_shadow_y_offset));
		xml.writeEndElement(/*framing*/);
	}
	
	if (line_below)
	{
		xml.writeStartElement(QStringLiteral("line_below"));
		xml.writeAttribute(QStringLiteral("color"), QString::number(map.findColorIndex(line_below_color)));
		xml.writeAttribute(QStringLiteral("width"), QString::number(line_below_width));
		xml.writeAttribute(QStringLiteral("distance"), QString::number(line_below_distance));
		xml.writeEndElement(/*line_below*/);
	}
	
	int num_custom_tabs = getNumCustomTabs();
	if (num_custom_tabs > 0)
	{
		xml.writeStartElement(QStringLiteral("tabs"));
		xml.writeAttribute(QStringLiteral("count"), QString::number(num_custom_tabs));
		for (int i = 0; i < num_custom_tabs; ++i)
			xml.writeTextElement(QStringLiteral("tab"), QString::number(custom_tabs[i]));
		xml.writeEndElement(/*tabs*/);
	}
	
	xml.writeEndElement(/*text_symbol*/);
}

bool TextSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	Q_UNUSED(symbol_dict);
	
	if (xml.name() != QLatin1String("text_symbol"))
		return false;
	
	icon_text = xml.attributes().value(QLatin1String("icon_text")).toString();
	framing = false;
	line_below = false;
	custom_tabs.clear();
	
	while (xml.readNextStartElement())
	{
		QXmlStreamAttributes attributes(xml.attributes());
		if (xml.name() == QLatin1String("font"))
		{
			font_family = xml.attributes().value(QLatin1String("family")).toString();
			font_size = attributes.value(QLatin1String("size")).toInt();
			bold = (attributes.value(QLatin1String("bold")) == QLatin1String("true"));
			italic = (attributes.value(QLatin1String("italic")) == QLatin1String("true"));
			underline = (attributes.value(QLatin1String("underline")) == QLatin1String("true"));
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("text"))
		{
			int temp = attributes.value(QLatin1String("color")).toInt();
			color = map.getColor(temp);
			line_spacing = attributes.value(QLatin1String("line_spacing")).toFloat();
			paragraph_spacing = attributes.value(QLatin1String("paragraph_spacing")).toInt();
			character_spacing = attributes.value(QLatin1String("character_spacing")).toFloat();
			kerning = (attributes.value(QLatin1String("kerning")) == QLatin1String("true"));
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("framing"))
		{
			framing = true;
			int temp = attributes.value(QLatin1String("color")).toInt();
			framing_color = map.getColor(temp);
			framing_mode = attributes.value(QLatin1String("mode")).toInt();
			framing_line_half_width = attributes.value(QLatin1String("line_half_width")).toInt();
			framing_shadow_x_offset = attributes.value(QLatin1String("shadow_x_offset")).toInt();
			framing_shadow_y_offset = attributes.value(QLatin1String("shadow_y_offset")).toInt();
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("line_below"))
		{
			line_below = true;
			int temp = attributes.value(QLatin1String("color")).toInt();
			line_below_color = map.getColor(temp);
			line_below_width = attributes.value(QLatin1String("width")).toInt();
			line_below_distance = attributes.value(QLatin1String("distance")).toInt();
			xml.skipCurrentElement();
		}
		else if (xml.name() == QLatin1String("tabs"))
		{
			int num_custom_tabs = attributes.value(QLatin1String("count")).toInt();
			custom_tabs.reserve(num_custom_tabs % 20); // 20 is not the limit
			while (xml.readNextStartElement())
			{
				if (xml.name() == QLatin1String("tab"))
					custom_tabs.push_back(xml.readElementText().toInt());
				else
					xml.skipCurrentElement();
			}
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	updateQFont();
	return true;
}

bool TextSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	Q_UNUSED(case_sensitivity);
	
	const TextSymbol* text = static_cast<const TextSymbol*>(other);
	
	if (!MapColor::equal(color, text->color))
		return false;
	if (font_family.compare(text->font_family, Qt::CaseInsensitive) != 0)
		return false;
	if (font_size != text->font_size ||
		bold != text->bold ||
		italic != text->italic ||
		underline != text->underline ||
		line_spacing != text->line_spacing ||
		paragraph_spacing != text->paragraph_spacing ||
		character_spacing != text->character_spacing ||
		kerning != text->kerning ||
		line_below != text->line_below ||
		framing != text->framing)
		return false;
	if (framing)
	{
		if (!MapColor::equal(framing_color, text->framing_color))
			return false;
		if (framing_mode != text->framing_mode ||
			(framing_mode == LineFraming && framing_line_half_width != text->framing_line_half_width) ||
			(framing_mode == ShadowFraming && (framing_shadow_x_offset != text->framing_shadow_x_offset || framing_shadow_y_offset != text->framing_shadow_y_offset)))
			return false;
	}
	if (line_below)
	{
		if (!MapColor::equal(line_below_color, text->line_below_color))
			return false;
		if (line_below_width != text->line_below_width ||
			line_below_distance != text->line_below_distance)
			return false;
	}
	if (tab_interval != text->tab_interval)
		return false;
	if (custom_tabs.size() != text->custom_tabs.size())
		return false;
	for (size_t i = 0, end = custom_tabs.size(); i < end; ++i)
	{
		if (custom_tabs[i] != text->custom_tabs[i])
			return false;
	}
	
	return true;
}

QString TextSymbol::getIconText() const
{
	if (icon_text.isEmpty())
		return TextSymbolSettings::tr("A", "First capital letter of the local alphabet");
	return icon_text;
}

double TextSymbol::getNextTab(double pos) const
{
	if (!custom_tabs.empty())
	{
		double scaling = calculateInternalScaling();
		double map_pos = pos / scaling;
		for (int i = 0; i < (int)custom_tabs.size(); ++i)
		{
			if (0.001 * custom_tabs[i] > map_pos)
				return scaling * 0.001 * custom_tabs[i];
		}
		
		// After the given positions, OCAD repeats the distance between the last two tab positions
		double custom_tab_interval = (custom_tabs.size() > 1) ?
									   (custom_tabs[custom_tabs.size() - 1] - custom_tabs[custom_tabs.size() - 2]) :
									   custom_tabs[0];
		custom_tab_interval *= 0.001;
		return scaling * (0.001 * custom_tabs[custom_tabs.size() - 1] + (floor((map_pos - 0.001 * custom_tabs[custom_tabs.size() - 1]) / custom_tab_interval) + 1.0) * custom_tab_interval);
	}
	
	double next_tab = (floor(pos / tab_interval) + 1.0) * tab_interval;
	Q_ASSERT(next_tab > pos);
 	return next_tab;
}

SymbolPropertiesWidget* TextSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new TextSymbolSettings(this, dialog);
}


// ### TextSymbolSettings ###

TextSymbolSettings::TextSymbolSettings(TextSymbol* symbol, SymbolSettingDialog* dialog)
: SymbolPropertiesWidget(symbol, dialog), 
  symbol(symbol), 
  dialog(dialog)
{
	Map* map = dialog->getPreviewMap();
	react_to_changes = true;
	
	QWidget* text_tab = new QWidget();
	addPropertiesGroup(tr("Text settings"), text_tab);
	
	QFormLayout* layout = new QFormLayout();
	text_tab->setLayout(layout);
	
	font_edit = new QFontComboBox();
	layout->addRow(tr("Font family:"), font_edit);
	
	// 0.04 pt font size is app. 0.01 mm letter height, the non-zero minimum
	font_size_edit = Util::SpinBox::create(2, 0.04, 40000.0, tr("pt"));
	layout->addRow(tr("Font size:"), font_size_edit);
	
	QHBoxLayout* letter_size_layout = new QHBoxLayout();
	letter_size_layout->setMargin(0);
	
	letter_size_layout->addWidget(new QLabel(DetermineFontSizeDialog::tr("Letter:")));
	//: "A" is the default letter which is used for determining letter height.
	letter_edit = new QLineEdit(DetermineFontSizeDialog::tr("A"));
	letter_edit->setMaxLength(3);
	letter_size_layout->addWidget(letter_edit);
	
	letter_size_layout->addSpacing(8);
	
	letter_size_layout->addWidget(new QLabel(DetermineFontSizeDialog::tr("Height:")));
	letter_size_edit = Util::SpinBox::create(2, 0.01, 10000.0, tr("mm"));
	letter_size_layout->addWidget(letter_size_edit);
	
	layout->addRow(new QWidget(), letter_size_layout);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	color_edit = new ColorDropDown(map, symbol->getColor());
	layout->addRow(tr("Text color:"), color_edit);
	
	QVBoxLayout* text_style_layout = new QVBoxLayout();
	bold_check = new QCheckBox(tr("bold"));
	text_style_layout->addWidget(bold_check);
	italic_check = new QCheckBox(tr("italic"));
	text_style_layout->addWidget(italic_check);
	underline_check = new QCheckBox(tr("underlined"));
	text_style_layout->addWidget(underline_check);
	
	layout->addRow(tr("Text style:"), text_style_layout);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	line_spacing_edit = Util::SpinBox::create(1, 0.0, 999999.9, tr("%"));
	layout->addRow(tr("Line spacing:"), line_spacing_edit);
	
	paragraph_spacing_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	layout->addRow(tr("Paragraph spacing:"), paragraph_spacing_edit);
	
	character_spacing_edit = Util::SpinBox::create(1, -999999.9, 999999.9, tr("%"));
	layout->addRow(tr("Character spacing:"), character_spacing_edit);
	
	kerning_check = new QCheckBox(tr("Kerning"));
	layout->addRow(QString{}, kerning_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	icon_text_edit = new QLineEdit();
	icon_text_edit->setMaxLength(3);
	layout->addRow(tr("Symbol icon text:"), icon_text_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	framing_check = new QCheckBox(tr("Framing"));
	layout->addRow(framing_check);
	
	ocad_compat_check = new QCheckBox(tr("OCAD compatibility settings"));
	layout->addRow(ocad_compat_check);
	
	
	framing_widget = new QWidget();
	addPropertiesGroup(tr("Framing"), framing_widget);
	
	QFormLayout* framing_layout = new QFormLayout();
	framing_widget->setLayout(framing_layout);
	
	framing_color_edit = new ColorDropDown(map, symbol->getFramingColor());
	framing_layout->addRow(tr("Framing color:"), framing_color_edit);
	
	framing_line_radio = new QRadioButton(tr("Line framing"));
	framing_layout->addRow(framing_line_radio);
	
	framing_line_half_width_edit = Util::SpinBox::create(1, 0.0, 999999.9);
	framing_layout->addRow(tr("Width:"), framing_line_half_width_edit);
	
	framing_shadow_radio = new QRadioButton(tr("Shadow framing"));
	framing_layout->addRow(framing_shadow_radio);
	
	framing_shadow_x_offset_edit = Util::SpinBox::create(1, -999999.9, 999999.9);
	framing_layout->addRow(tr("Left/Right Offset:"), framing_shadow_x_offset_edit);
	
	framing_shadow_y_offset_edit = Util::SpinBox::create(1, -999999.9, 999999.9);
	framing_layout->addRow(tr("Top/Down Offset:"), framing_shadow_y_offset_edit);
	
	
	ocad_compat_widget = new QWidget();
	addPropertiesGroup(tr("OCAD compatibility"), ocad_compat_widget);
	
	QFormLayout* ocad_compat_layout = new QFormLayout();
	ocad_compat_widget->setLayout(ocad_compat_layout);
	
	ocad_compat_layout->addRow(Util::Headline::create(tr("Line below paragraphs")));
	
	line_below_check = new QCheckBox(tr("enabled"));
	ocad_compat_layout->addRow(line_below_check);
	
	line_below_width_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	ocad_compat_layout->addRow(tr("Line width:"), line_below_width_edit);
	
	line_below_color_edit = new ColorDropDown(map);
	ocad_compat_layout->addRow(tr("Line color:"), line_below_color_edit);
	
	line_below_distance_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	ocad_compat_layout->addRow(tr("Distance from baseline:"), line_below_distance_edit);
	
	ocad_compat_layout->addItem(Util::SpacerItem::create(this));
	
	ocad_compat_layout->addRow(Util::Headline::create(tr("Custom tabulator positions")));
	
	custom_tab_list = new QListWidget();
	ocad_compat_layout->addRow(custom_tab_list);
	
	QHBoxLayout* custom_tabs_button_layout = new QHBoxLayout();
	custom_tab_add = new QPushButton(QIcon(QStringLiteral(":/images/plus.png")), QString{});
	custom_tabs_button_layout->addWidget(custom_tab_add);
	custom_tab_remove = new QPushButton(QIcon(QStringLiteral(":/images/minus.png")), QString{});
	custom_tab_remove->setEnabled(false);
	custom_tabs_button_layout->addWidget(custom_tab_remove);
	custom_tabs_button_layout->addStretch(1);
	
	ocad_compat_layout->addRow(custom_tabs_button_layout);
	
	
	updateGeneralContents();
	updateFramingContents();
	updateCompatibilityContents();
	
	connect(font_edit, SIGNAL(currentFontChanged(QFont)), this, SLOT(fontChanged(QFont)));
	connect(font_size_edit, SIGNAL(valueChanged(double)), this, SLOT(fontSizeChanged(double)));
	connect(letter_edit, &QLineEdit::textEdited, this, &TextSymbolSettings::letterSizeChanged);
	connect(letter_size_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::letterSizeChanged);
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(bold_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(italic_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(underline_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(line_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(spacingChanged(double)));
	connect(paragraph_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(spacingChanged(double)));
	connect(character_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(spacingChanged(double)));
	connect(kerning_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(icon_text_edit, SIGNAL(textEdited(QString)), this, SLOT(iconTextEdited(QString)));
	connect(framing_check, SIGNAL(clicked(bool)), this, SLOT(framingCheckClicked(bool)));
	connect(ocad_compat_check, SIGNAL(clicked(bool)), this, SLOT(ocadCompatibilityButtonClicked(bool)));
	
	connect(framing_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(framingColorChanged()));
	connect(framing_line_radio, SIGNAL(clicked(bool)), this, SLOT(framingModeChanged()));
	connect(framing_line_half_width_edit, SIGNAL(valueChanged(double)), this, SLOT(framingSettingChanged()));
	connect(framing_shadow_radio, SIGNAL(clicked(bool)), this, SLOT(framingModeChanged()));
	connect(framing_shadow_x_offset_edit, SIGNAL(valueChanged(double)), this, SLOT(framingSettingChanged()));
	connect(framing_shadow_y_offset_edit, SIGNAL(valueChanged(double)), this, SLOT(framingSettingChanged()));
	
	connect(line_below_check, SIGNAL(clicked(bool)), this, SLOT(lineBelowCheckClicked(bool)));
	connect(line_below_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(lineBelowSettingChanged()));
	connect(line_below_width_edit, SIGNAL(valueChanged(double)), this, SLOT(lineBelowSettingChanged()));
	connect(line_below_distance_edit, SIGNAL(valueChanged(double)), this, SLOT(lineBelowSettingChanged()));
	connect(custom_tab_list, SIGNAL(currentRowChanged(int)), this, SLOT(customTabRowChanged(int)));
	connect(custom_tab_add, SIGNAL(clicked(bool)), this, SLOT(addCustomTabClicked()));
	connect(custom_tab_remove, SIGNAL(clicked(bool)), this, SLOT(removeCustomTabClicked()));
}

TextSymbolSettings::~TextSymbolSettings()
{
}

void TextSymbolSettings::fontChanged(QFont font)
{
	if (!react_to_changes)
		return;
	
	symbol->font_family = font.family();
	symbol->updateQFont();
	updateLetterSizeEdit();
	emit propertiesModified();
}

void TextSymbolSettings::fontSizeChanged(double value)
{
	if (!react_to_changes)
		return;
	
	symbol->font_size = qRound(value * 25400.0 / 72.0);
	symbol->updateQFont();
	updateLetterSizeEdit();
	emit propertiesModified();
}

void TextSymbolSettings::updateFontSizeEdit()
{
	QSignalBlocker block { font_size_edit };
	font_size_edit->setValue(0.01 * qRound(symbol->font_size * 7.2 / 25.4));
}

void TextSymbolSettings::letterSizeChanged()
{
	auto letter_height = calculateLetterHeight();
	if (letter_height > 0.0)
	{
		symbol->font_size = qRound(1000.0 * letter_size_edit->value() / letter_height);
		symbol->updateQFont();
		updateFontSizeEdit();
		emit propertiesModified();
	}
}

qreal TextSymbolSettings::calculateLetterHeight() const
{
	QPainterPath path;
	path.addText(0.0, 0.0, symbol->getQFont(), letter_edit->text());
	return path.boundingRect().height() / qreal(TextSymbol::internal_point_size);
}

void TextSymbolSettings::updateLetterSizeEdit()
{
	auto letter_height = calculateLetterHeight();
	if (letter_height > 0.0)
	{
		QSignalBlocker block { letter_size_edit };
		letter_size_edit->setValue(0.01 * qRound(symbol->font_size * letter_height / 10.0));
	}
}

void TextSymbolSettings::colorChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->color = color_edit->color();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::checkToggled(bool checked)
{
	Q_UNUSED(checked);
	
	if (!react_to_changes)
		return;
	
	symbol->bold = bold_check->isChecked();
	symbol->italic = italic_check->isChecked();
	symbol->underline = underline_check->isChecked();
	symbol->kerning = kerning_check->isChecked();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::spacingChanged(double value)
{
	Q_UNUSED(value);
	
	if (!react_to_changes)
		return;
	
	symbol->line_spacing = 0.01 * line_spacing_edit->value();
	symbol->paragraph_spacing = qRound(1000.0 * paragraph_spacing_edit->value());
	symbol->character_spacing = 0.01f * character_spacing_edit->value();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::iconTextEdited(const QString& text)
{
	if (!react_to_changes)
		return;
	
	symbol->icon_text = text;
	emit propertiesModified();
}

void TextSymbolSettings::framingColorChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->framing_color = framing_color_edit->color();
	updateFramingContents();
	emit propertiesModified();
}

void TextSymbolSettings::framingModeChanged()
{
	if (!react_to_changes)
		return;
	
	if (framing_line_radio->isChecked())
		symbol->framing_mode = TextSymbol::LineFraming;
	else if (framing_shadow_radio->isChecked())
		symbol->framing_mode = TextSymbol::ShadowFraming;
	updateFramingContents();
	emit propertiesModified();
}

void TextSymbolSettings::framingSettingChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->framing_line_half_width = qRound(1000.0 * framing_line_half_width_edit->value());
	symbol->framing_shadow_x_offset = qRound(1000.0 * framing_shadow_x_offset_edit->value());
	symbol->framing_shadow_y_offset = qRound(-1000.0 * framing_shadow_y_offset_edit->value());
	emit propertiesModified();
}

void TextSymbolSettings::framingCheckClicked(bool checked)
{
	if (!react_to_changes)
		return;
	
	setTabEnabled(indexOf(framing_widget), checked);
	symbol->framing = checked;
	emit propertiesModified();
}

void TextSymbolSettings::ocadCompatibilityButtonClicked(bool checked)
{
	if (!react_to_changes)
		return;
	
	setTabEnabled(indexOf(ocad_compat_widget), checked);
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::lineBelowCheckClicked(bool checked)
{
	if (!react_to_changes)
		return;
	
	symbol->line_below = checked;
	if (checked)
	{
		// Set defaults such that the line becomes visible
		if (symbol->line_below_width == 0)
			line_below_width_edit->setValue(0.1);
		if (symbol->line_below_color == NULL)
			line_below_color_edit->setCurrentIndex(1);
	}
	
	symbol->updateQFont();
	emit propertiesModified();
	
	line_below_color_edit->setEnabled(checked);
	line_below_width_edit->setEnabled(checked);
	line_below_distance_edit->setEnabled(checked);
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::lineBelowSettingChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->line_below_color = line_below_color_edit->color();
	symbol->line_below_width = qRound(1000.0 * line_below_width_edit->value());
	symbol->line_below_distance = qRound(1000.0 * line_below_distance_edit->value());
	symbol->updateQFont();
	emit propertiesModified();
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::customTabRowChanged(int row)
{
	custom_tab_remove->setEnabled(row >= 0);
}

void TextSymbolSettings::addCustomTabClicked()
{
	bool ok = false;
	// FIXME: Unit of measurement display in unusual way
	double position = QInputDialog::getDouble(dialog, tr("Add custom tabulator"), QString::fromLatin1("%1 (%2)").arg(tr("Position:")).arg(tr("mm")), 0, 0, 999999, 3, &ok);
	if (ok)
	{
		int int_position = qRound(1000 * position);
		
		int row = symbol->getNumCustomTabs();
		for (int i = 0; i < symbol->getNumCustomTabs(); ++i)
		{
			if (int_position == symbol->custom_tabs[i])
				return;	// do not add a double position
			else if (int_position < symbol->custom_tabs[i])
			{
				row = i;
				break;
			}
		}
		
		custom_tab_list->insertItem(row, locale().toString(position, 'g', 3) + QLatin1Char(' ') + tr("mm"));
		custom_tab_list->setCurrentRow(row);
		symbol->custom_tabs.insert(symbol->custom_tabs.begin() + row, int_position);
		emit propertiesModified();
		updateCompatibilityCheckEnabled();
	}
}

void TextSymbolSettings::removeCustomTabClicked()
{
	if (auto item = custom_tab_list->currentItem())
	{
		int row = custom_tab_list->row(item);
		delete custom_tab_list->item(row);
		custom_tab_list->setCurrentRow(row - 1);
		symbol->custom_tabs.erase(symbol->custom_tabs.begin() + row);
		emit propertiesModified();
		updateCompatibilityCheckEnabled();
	}
}

void TextSymbolSettings::updateGeneralContents()
{
	react_to_changes = false;
	font_edit->setCurrentFont(QFont(symbol->font_family));
	updateFontSizeEdit();
	updateLetterSizeEdit();
	color_edit->setColor(symbol->color);
	bold_check->setChecked(symbol->bold);
	italic_check->setChecked(symbol->italic);
	underline_check->setChecked(symbol->underline);
	line_spacing_edit->setValue(100.0 * symbol->line_spacing);
	paragraph_spacing_edit->setValue(0.001 * symbol->paragraph_spacing);
	character_spacing_edit->setValue(100 * symbol->character_spacing);
	kerning_check->setChecked(symbol->kerning);
	icon_text_edit->setText(symbol->getIconText());
	framing_check->setChecked(symbol->framing);
	ocad_compat_check->setChecked(symbol->line_below || symbol->getNumCustomTabs() > 0);
	react_to_changes = true;
	
	framingCheckClicked(framing_check->isChecked());
	ocadCompatibilityButtonClicked(ocad_compat_check->isChecked());
}

void TextSymbolSettings::updateCompatibilityCheckEnabled()
{
	ocad_compat_check->setEnabled(!symbol->line_below && symbol->getNumCustomTabs() == 0);
}

void TextSymbolSettings::updateFramingContents()
{
	react_to_changes = false;
	
	framing_color_edit->setColor(symbol->framing_color);
	framing_line_radio->setChecked(symbol->framing_mode == TextSymbol::LineFraming);
	framing_line_half_width_edit->setValue(0.001 * symbol->framing_line_half_width);
	framing_shadow_radio->setChecked(symbol->framing_mode == TextSymbol::ShadowFraming);
	framing_shadow_x_offset_edit->setValue(0.001 * symbol->framing_shadow_x_offset);
	framing_shadow_y_offset_edit->setValue(-0.001 * symbol->framing_shadow_y_offset);
	
	
	framing_line_radio->setEnabled(symbol->framing_color != NULL);
	framing_shadow_radio->setEnabled(symbol->framing_color != NULL);
	if (symbol->framing_color == NULL)
	{
		framing_line_half_width_edit->setEnabled(false);
		framing_shadow_x_offset_edit->setEnabled(false);
		framing_shadow_y_offset_edit->setEnabled(false);
	}
	else
	{
		framing_line_half_width_edit->setEnabled(symbol->framing_mode == TextSymbol::LineFraming);
		framing_shadow_x_offset_edit->setEnabled(symbol->framing_mode == TextSymbol::ShadowFraming);
		framing_shadow_y_offset_edit->setEnabled(symbol->framing_mode == TextSymbol::ShadowFraming);
	}
	
	react_to_changes = true;
}

void TextSymbolSettings::updateCompatibilityContents()
{
	react_to_changes = false;
	line_below_check->setChecked(symbol->line_below);
	line_below_width_edit->setValue(0.001 * symbol->line_below_width);
	line_below_color_edit->setColor(symbol->line_below_color);
	line_below_distance_edit->setValue(0.001 * symbol->line_below_distance);
	
	line_below_color_edit->setEnabled(symbol->line_below);
	line_below_width_edit->setEnabled(symbol->line_below);
	line_below_distance_edit->setEnabled(symbol->line_below);

	custom_tab_list->clear();
	for (int i = 0; i < symbol->getNumCustomTabs(); ++i)
		custom_tab_list->addItem(locale().toString(0.001 * symbol->getCustomTab(i), 'g', 3) + QLatin1Char(' ') + tr("mm"));
	
	react_to_changes = true;
}

void TextSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Text);
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	updateGeneralContents();
	updateFramingContents();
	updateCompatibilityContents();
}
