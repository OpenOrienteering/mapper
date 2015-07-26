/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#ifndef _OPENORIENTEERING_SYMBOL_TEXT_H_
#define _OPENORIENTEERING_SYMBOL_TEXT_H_

#include "symbol.h"

#include <QGroupBox>
#include <QDialog>

#include "symbol_properties_widget.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFontComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
QT_END_NAMESPACE

class ColorDropDown;
class SymbolSettingDialog;
class TextObject;

/** Symbol for text, can be applied to TextObjects.
 * 
 * NOTE: Uses a hack to disable hinting for fonts:
 * Uses a big internal font size, internal_point_size, where hinting is
 * neglegible and scales it down for output.
 * TODO: A maybe better solution would be to use FreeType directly.
 */
class TextSymbol : public Symbol
{
friend class TextSymbolSettings;
friend class DetermineFontSizeDialog;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	/** Modes for text framing */
	enum FramingMode
	{
		/** Only the text itself is displayed. */
		NoFraming = 0,
		
		/**
		 * The text path is drawn with a QPainter with active QPen in addition
		 * to the normal fill, resulting in a line around the letters.
		 */
		LineFraming = 1,
		
		/**
		 * The text is drawn a second time with a slight offset to the main
		 * text, creating a shadow effect.
		 */
		ShadowFraming = 2
	};
	
	/** Creates an empty text symbol. */
	TextSymbol();
	virtual ~TextSymbol();
	virtual Symbol* duplicate(const MapColorMap* color_map = NULL) const;
	
	void createRenderables(
	        const Object *object,
	        const VirtualCoordVector &coords,
	        ObjectRenderables &output,
	        Symbol::RenderableOptions options) const override;
	
	void createBaselineRenderables(
	        const TextObject* text_object,
	        const VirtualCoordVector &coords,
	        ObjectRenderables &output) const;
	
	void createLineBelowRenderables(const Object* object, ObjectRenderables& output) const;
	virtual void colorDeleted(const MapColor* color);
	virtual bool containsColor(const MapColor* color) const;
	virtual const MapColor* guessDominantColor() const;
	virtual void scale(double factor);
	
	/** Updates the internal QFont from the font settings. */
	void updateQFont();
	
	/** Calculates the factor to convert from the real font size to the internal font size */
	inline double calculateInternalScaling() const {return internal_point_size / (0.001 * font_size);}
	
	// Getters
	inline const MapColor* getColor() const {return color;}
	inline void setColor(const MapColor* color) {this->color = color;}
	inline const QString& getFontFamily() const {return font_family;}
	inline double getFontSize() const {return 0.001 * font_size;}
	inline bool isBold() const {return bold;}
	inline bool isItalic() const {return italic;}
	inline bool isUnderlined() const {return underline;}
	inline float getLineSpacing() const {return line_spacing;}
	inline double getParagraphSpacing() const {return 0.001 * paragraph_spacing;}
	inline double getCharacterSpacing() const {return 0.001 * character_spacing;}
	inline bool usesKerning() const {return kerning;}
	QString getIconText() const; // returns a default text if no custom text is set.
	inline bool usesFraming() const {return framing;}
	inline const MapColor* getFramingColor() const {return framing_color;}
	inline int getFramingMode() const {return framing_mode;}
	inline int getFramingLineHalfWidth() const {return framing_line_half_width;}
	inline int getFramingShadowXOffset() const {return framing_shadow_x_offset;}
	inline int getFramingShadowYOffset() const {return framing_shadow_y_offset;}
	inline bool hasLineBelow() const {return line_below;}
	inline const MapColor* getLineBelowColor() const {return line_below_color;}
	inline double getLineBelowWidth() const {return 0.001 * line_below_width;}
	inline double getLineBelowDistance() const {return 0.001 * line_below_distance;}
	inline int getNumCustomTabs() const {return (int)custom_tabs.size();}
	inline int getCustomTab(int index) const {return custom_tabs[index];}
	inline const QFont& getQFont() const {return qfont;}
	inline const QFontMetricsF& getFontMetrics() const { return metrics; }
	
	double getNextTab(double pos) const;
	
	static const float internal_point_size;
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
protected:
#ifndef NO_NATIVE_FILE_FORMAT
	virtual bool loadImpl(QIODevice* file, int version, Map* map);
#endif
	virtual void saveImpl(QXmlStreamWriter& xml, const Map& map) const;
	virtual bool loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict);
	virtual bool equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const;
	
	QFont qfont;
	QFontMetricsF metrics;
	
	const MapColor* color;
	QString font_family;
	int font_size;				// this defines the font size in 1000 mm. How big the letters really are depends on the design of the font though
	bool bold;
	bool italic;
	bool underline;
	float line_spacing;			// as factor of original line spacing
	// (BEGIN) DON'T CHANGE THE ORDER OF THESE FIELDS BEFORE SOLVING [tickets:#428]
	int paragraph_spacing;		// in mm
	float character_spacing;	// as a factor of the space character width
	bool kerning;
	QString icon_text;			// text to be drawn in the symbol's icon
	//  (END)  DON'T CHANGE THE ORDER OF THESE FIELDS BEFORE SOLVING [tickets:#428]
	
	bool framing;
	const MapColor* framing_color;
	int framing_mode;
	int framing_line_half_width;
	int framing_shadow_x_offset;
	int framing_shadow_y_offset;
	
	// OCAD compatibility features
	bool line_below;
	const MapColor* line_below_color;
	int line_below_width;
	int line_below_distance;
	std::vector<int> custom_tabs;
	
	double tab_interval;		/// default tab interval length in text coordinates
};

class TextSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	TextSymbolSettings(TextSymbol* symbol, SymbolSettingDialog* dialog);
    virtual ~TextSymbolSettings();
	
	virtual void reset(Symbol* symbol);
	
	void updateGeneralContents();
	
	void updateFramingContents();
	
	void updateCompatibilityCheckEnabled();
	
	void updateCompatibilityContents();
	
protected slots:
	void fontChanged(QFont font);
	void sizeChanged(double value);
	void sizeUnitChanged(int index);
	void updateSizeEdit();
	void determineSizeClicked();
	void colorChanged();
	void checkToggled(bool checked);
	void spacingChanged(double value);
	void iconTextEdited(const QString& text);
	
	void framingCheckClicked(bool checked);
	void framingColorChanged();
	void framingModeChanged();
	void framingSettingChanged();
	
	void ocadCompatibilityButtonClicked(bool checked);
	void lineBelowCheckClicked(bool checked);
	void lineBelowSettingChanged();
	void customTabRowChanged(int row);
	void addCustomTabClicked();
	void removeCustomTabClicked();
	
private:
	enum SizeUnit
	{
		SizeInMM = 0,
		SizeInPT
	};
	
	TextSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	ColorDropDown*  color_edit;
	QFontComboBox*  font_edit;
	QDoubleSpinBox* size_edit;
	QComboBox*      size_unit_combo;
	QPushButton*    size_determine_button;
	QCheckBox*      bold_check;
	QCheckBox*      italic_check;
	QCheckBox*      underline_check;
	QDoubleSpinBox* line_spacing_edit;
	QDoubleSpinBox* paragraph_spacing_edit;
	QDoubleSpinBox* character_spacing_edit;
	QCheckBox*      kerning_check;
	QLineEdit*      icon_text_edit;
	QCheckBox*      framing_check;
	QCheckBox*      ocad_compat_check;
	
	QWidget*        framing_widget;
	ColorDropDown*  framing_color_edit;
	QRadioButton*   framing_line_radio;
	QDoubleSpinBox* framing_line_half_width_edit;
	QRadioButton*   framing_shadow_radio;
	QDoubleSpinBox* framing_shadow_x_offset_edit;
	QDoubleSpinBox* framing_shadow_y_offset_edit;
	
	QWidget*        ocad_compat_widget;
	QCheckBox*      line_below_check;
	QDoubleSpinBox* line_below_width_edit;
	ColorDropDown*  line_below_color_edit;
	QDoubleSpinBox* line_below_distance_edit;
	QListWidget*    custom_tab_list;
	QPushButton*    custom_tab_add;
	QPushButton*    custom_tab_remove;
	
	bool react_to_changes;
};

/**
 * Dialog to calculate the font size at which a certain letter
 * has a certain real size.
 */
class DetermineFontSizeDialog : public QDialog
{
Q_OBJECT
public:
	DetermineFontSizeDialog(QWidget* parent, TextSymbol* symbol);
	
public slots:
	virtual void accept();
	void updateOkButton();
	
private:
	QLineEdit* character_edit;
	QDoubleSpinBox* size_edit;
	QPushButton* ok_button;
	
	TextSymbol* symbol;
};

#endif
