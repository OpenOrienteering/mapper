/*
 *    Copyright 2012, 2013 Kai Pastor
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


#include "color_dialog.h"

#include "../core/map_color.h"
#include "../map.h"
#include "../util.h"
#include "../util_gui.h"
#include "widgets/color_dropdown.h"

ColorDialog::ColorDialog(const Map& map, const MapColor& source_color, QWidget* parent, Qt::WindowFlags f)
: QDialog(parent, f),
  map(map),
  source_color(source_color),
  color(source_color),
  color_modified(false),
  react_to_changes(true)
{
	setWindowTitle(tr("Edit map color"));
	setSizeGripEnabled(true);
	
	color_preview_label = new QLabel();
	mc_name_edit = new QLineEdit();
	
	prof_color_layout = new QGridLayout();
	int col = 0;
	prof_color_layout->setColumnStretch(col, 1);
	prof_color_layout->setColumnStretch(col+1, 3);
	
	int row = 0;
	prof_color_layout->addWidget(Util::Headline::create("Spot color printing"), row, col, 1, 2);
	
	QButtonGroup* spot_color_options = new QButtonGroup(this);
	
	++row;
	full_tone_option = new QRadioButton(tr("Defines a spot color:"));
	spot_color_options->addButton(full_tone_option, MapColor::SpotColor);
	prof_color_layout->addWidget(full_tone_option, row, col, 1, 2);
	
	++row;
	sc_name_edit = new QLineEdit();
	prof_color_layout->addWidget(sc_name_edit, row, col, 1, 2);
	
	++row;
	composition_option = new QRadioButton(tr("Mixture of spot colors (screens and overprint):"));
	spot_color_options->addButton(composition_option, MapColor::CustomColor);
	prof_color_layout->addWidget(composition_option, row, col, 1, 2);
	
	int num_components = 0 /*color.getComponents().size()*/; // FIXME: cleanup
	components_row0 = row+1;
	components_col0 = col;
	component_colors.resize(num_components+1);
	component_halftone.resize(num_components+1);
	for (int i = 0; i <= num_components; i++)
	{
		++row;
		component_colors[i] = new ColorDropDown(&map, &color, true);
		prof_color_layout->addWidget(component_colors[i], row, col);
		component_halftone[i] = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
		prof_color_layout->addWidget(component_halftone[i], row, col+1);
	}
	
	++row;
	knockout_option = new QCheckBox(tr("Knockout: erases lower colors"));
	prof_color_layout->addWidget(knockout_option, row, col, 1, 2);
	knockout_option->setEnabled(false);
	
	row = 0, col += 2;
	prof_color_layout->setColumnStretch(col, 1);
	
	const int spacing = style()->pixelMetric(QStyle::PM_LayoutTopMargin);
	prof_color_layout->addItem(new QSpacerItem(3*spacing, spacing), row, col, 7, 1);
	
	row = 0, col +=1;
	prof_color_layout->setColumnStretch(col, 1);
	prof_color_layout->setColumnStretch(col+1, 3);
	prof_color_layout->addWidget(Util::Headline::create("CMYK"), row, col, 1, 2);
	
	QButtonGroup* cmyk_color_options = new QButtonGroup(this);
	
	++row;
	cmyk_spot_color_option = new QRadioButton(tr("Calculate from spot colors"));
	cmyk_color_options->addButton(cmyk_spot_color_option, MapColor::SpotColor);
	prof_color_layout->addWidget(cmyk_spot_color_option, row, col, 1, 2);
	
	++row;
	evaluate_rgb_option = new QRadioButton(tr("Calculate from RGB color"));
	cmyk_color_options->addButton(evaluate_rgb_option, MapColor::RgbColor);
	prof_color_layout->addWidget(evaluate_rgb_option, row, col, 1, 2);
	
	++row;
	custom_cmyk_option = new QRadioButton(tr("Custom process color:"));
	cmyk_color_options->addButton(custom_cmyk_option, MapColor::CustomColor);
	prof_color_layout->addWidget(custom_cmyk_option, row, col, 1, 2);
	
	++row;
	c_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Cyan")), row, col);
	prof_color_layout->addWidget(c_edit, row, col+1);
	
	++row;
	m_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Magenta")), row, col);
	prof_color_layout->addWidget(m_edit, row, col+1);
	
	++row;
	y_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Yellow")), row, col);
	prof_color_layout->addWidget(y_edit, row, col+1);
	
	++row;
	k_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Black")), row, col);
	prof_color_layout->addWidget(k_edit, row, col+1);
	
	++row;
	stretch_row0 = row;
	stretch_col0 = col;
	stretch = new QWidget();
	prof_color_layout->addWidget(stretch, row, col);
	prof_color_layout->setRowStretch(row, 1);
	
	QWidget* prof_color_widget = new QWidget();
	prof_color_widget->setLayout(prof_color_layout);
	prof_color_widget->setObjectName("professional");
	
	
	QGridLayout* desktop_layout = new QGridLayout();
	col = 0;
	desktop_layout->setColumnStretch(col, 1);
	desktop_layout->setColumnStretch(col+1, 3);
	
	row = 0;
	desktop_layout->addWidget(Util::Headline::create("RGB"), row, col, 1, 2);
	
	QButtonGroup* rgb_color_options = new QButtonGroup(this);
	
	++row;
	rgb_spot_color_option = new QRadioButton(tr("Calculate from spot colors"));
	rgb_color_options->addButton(rgb_spot_color_option, MapColor::SpotColor);
	desktop_layout->addWidget(rgb_spot_color_option, row, col, 1, 2);
	
	++row;
	evaluate_cmyk_option = new QRadioButton(tr("Calculate from CMYK color"));
	rgb_color_options->addButton(evaluate_cmyk_option, MapColor::CmykColor);
	desktop_layout->addWidget(evaluate_cmyk_option, row, col, 1, 2);
	
	++row;
	custom_rgb_option = new QRadioButton(tr("Custom RGB color:"));
	rgb_color_options->addButton(custom_rgb_option, MapColor::CustomColor);
	desktop_layout->addWidget(custom_rgb_option, row, col, 1, 2);
	
	++row;
	r_edit = Util::SpinBox::create(1, 0.0, 255.0, "", 5);
	desktop_layout->addWidget(new QLabel(tr("Red")), row, col);
	desktop_layout->addWidget(r_edit, row, col+1);
	
	++row;
	g_edit = Util::SpinBox::create(1, 0.0, 255.0, "", 5);
	desktop_layout->addWidget(new QLabel(tr("Green")), row, col);
	desktop_layout->addWidget(g_edit, row, col+1);
	
	++row;
	b_edit = Util::SpinBox::create(1, 0.0, 255.0, "", 5);
	desktop_layout->addWidget(new QLabel(tr("Blue")), row, col);
	desktop_layout->addWidget(b_edit, row, col+1);
	
	++row;
	html_edit = new QLineEdit();
	desktop_layout->addWidget(new QLabel(tr("#RRGGBB")), row, col);
	desktop_layout->addWidget(html_edit, row, col+1);
	
	++row;
	desktop_layout->addWidget(new QWidget(), row, col);
	desktop_layout->setRowStretch(row, 1);
	
	row = 0, col += 2;
	desktop_layout->setColumnStretch(col, 7);
	
	desktop_layout->addItem(new QSpacerItem(3*spacing, spacing), row, col, 7, 1);
	
	QWidget* desktop_color_widget = new QWidget();
	desktop_color_widget->setLayout(desktop_layout);
	desktop_color_widget->setObjectName("desktop");
	
	
	properties_widget = new QTabWidget();
	properties_widget->addTab(desktop_color_widget, tr("Desktop"));
	properties_widget->addTab(prof_color_widget, tr("Professional printing"));
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok | QDialogButtonBox::Reset | QDialogButtonBox::Help);
	ok_button = button_box->button(QDialogButtonBox::Ok);
	reset_button = button_box->button(QDialogButtonBox::Reset);
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(reset_button, SIGNAL(clicked(bool)), this, SLOT(reset()));
	connect(button_box->button(QDialogButtonBox::Help), SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(color_preview_label, 0, 0);
	layout->addWidget(mc_name_edit, 0, 1);
	layout->addWidget(properties_widget, 1, 0, 1, 2);
	layout->addWidget(button_box, 2, 0, 1, 2);
	layout->setColumnStretch(1, 1);
	setLayout(layout);
	
	updateWidgets();
	updateButtons();
	
	connect(mc_name_edit, SIGNAL(textChanged(QString)), this, SLOT(mapColorNameChanged()));
	
	connect(spot_color_options, SIGNAL(buttonClicked(int)), this, SLOT(spotColorTypeChanged(int)));
	connect(sc_name_edit, SIGNAL(textChanged(QString)), this, SLOT(spotColorNameChanged()));
	for (int i = 0; i < (int)component_colors.size(); i++)
	{
		connect(component_colors[i], SIGNAL(currentIndexChanged(int)), this, SLOT(spotColorCompositionChanged()));
		connect(component_halftone[i], SIGNAL(valueChanged(double)), this, SLOT(spotColorCompositionChanged()));
	}
	connect(knockout_option, SIGNAL(clicked(bool)), this, SLOT(knockoutChanged()));
	
	connect(cmyk_color_options, SIGNAL(buttonClicked(int)), this, SLOT(cmykColorTypeChanged(int)));
	connect(c_edit, SIGNAL(valueChanged(double)), this, SLOT(cmykValueChanged()));
	connect(m_edit, SIGNAL(valueChanged(double)), this, SLOT(cmykValueChanged()));
	connect(y_edit, SIGNAL(valueChanged(double)), this, SLOT(cmykValueChanged()));
	connect(k_edit, SIGNAL(valueChanged(double)), this, SLOT(cmykValueChanged()));
	
	connect(rgb_color_options, SIGNAL(buttonClicked(int)), this, SLOT(rgbColorTypeChanged(int)));
	connect(r_edit, SIGNAL(valueChanged(double)), this, SLOT(rgbValueChanged()));
	connect(g_edit, SIGNAL(valueChanged(double)), this, SLOT(rgbValueChanged()));
	connect(b_edit, SIGNAL(valueChanged(double)), this, SLOT(rgbValueChanged()));
	
	QSettings settings;
	settings.beginGroup("ColorDialog");
	QString default_view = settings.value("view").toString();
	settings.endGroup();
	properties_widget->setCurrentWidget(properties_widget->findChild<QWidget*>(default_view));
}

void ColorDialog::updateWidgets()
{
	react_to_changes = false;
	
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(color);
	color_preview_label->setPixmap(pixmap);
	
	mc_name_edit->setText(color.getName());
	
	sc_name_edit->setText(color.getSpotColorName());
	
	const MapColorCmyk& cmyk = color.getCmyk();
	c_edit->setValue(100.0 * cmyk.c);
	m_edit->setValue(100.0 * cmyk.m);
	y_edit->setValue(100.0 * cmyk.y);
	k_edit->setValue(100.0 * cmyk.k);
	
	knockout_option->setChecked(color.getKnockout());
	
	if (color.getSpotColorMethod() == MapColor::SpotColor)
	{
		full_tone_option->setChecked(true);
		sc_name_edit->setEnabled(true);
		knockout_option->setEnabled(true);
		cmyk_spot_color_option->setEnabled(false);
		if (cmyk_spot_color_option->isChecked())
			custom_cmyk_option->setChecked(true);
		rgb_spot_color_option->setEnabled(false);
		if (rgb_spot_color_option->isChecked())
			custom_rgb_option->setChecked(true);
	}
	else if (color.getSpotColorMethod() == MapColor::CustomColor)
	{
		composition_option->setChecked(true);
		sc_name_edit->setEnabled(false);
		knockout_option->setEnabled(true);
		cmyk_spot_color_option->setEnabled(true);
		rgb_spot_color_option->setEnabled(true);
	}
	else
	{
		sc_name_edit->setEnabled(false);
		cmyk_spot_color_option->setEnabled(false);
		if (cmyk_spot_color_option->isChecked())
		{
			custom_cmyk_option->setChecked(true);
		}
		cmyk_spot_color_option->setEnabled(false);
		if (rgb_spot_color_option->isChecked())
		{
			custom_rgb_option->setChecked(true);
		}
		rgb_spot_color_option->setEnabled(false);
	}
	
	const SpotColorComponents& color_components = color.getComponents();
	int num_components = color_components.size();
	int num_editors = component_colors.size();
	
	for (int i = num_components+1; i < num_editors; ++i)
	{
		prof_color_layout->removeWidget(component_colors[i]);
		delete component_colors[i];
		prof_color_layout->removeWidget(component_halftone[i]);
		delete component_halftone[i];
	}
	
	if (num_editors != num_components+1)
	{
		prof_color_layout->removeWidget(knockout_option);
		prof_color_layout->addWidget(knockout_option, components_row0+num_components+1, components_col0);
	}
	
	component_colors.resize(num_components+1);
	component_halftone.resize(num_components+1);
	for (int i = num_editors; i <= num_components; ++i)
	{
		component_colors[i] = new ColorDropDown(&map, &color, true);
		prof_color_layout->addWidget(component_colors[i], components_row0+i, components_col0);
		connect(component_colors[i], SIGNAL(currentIndexChanged(int)), this, SLOT(spotColorCompositionChanged()));
		component_halftone[i] = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
		prof_color_layout->addWidget(component_halftone[i], components_row0+i, components_col0+1);
		connect(component_halftone[i], SIGNAL(valueChanged(double)), this, SLOT(spotColorCompositionChanged()));
	}
	
	num_editors = component_colors.size();
	bool enable_component = composition_option->isChecked();
	for (int i = 0; i < num_editors; i++)
	{
		bool have_component = (i < (int)num_components);
		const MapColor* component_color = have_component ? color_components[i].spot_color : NULL;
		component_colors[i]->setEnabled(enable_component);
		component_colors[i]->setColor(component_color);
		
		bool enable_halftone = enable_component && have_component;
		float component_factor = enable_halftone ? color_components[i].factor : 0.0f;
		component_halftone[i]->setEnabled(enable_halftone);
		component_halftone[i]->setValue(component_factor * 100.0);
		
		prof_color_layout->setRowStretch(components_row0 + i, 0);
		
		enable_component = enable_component && enable_halftone;
	}
	// At least one component must be editable to create a composition
	if (color.getSpotColorMethod() == MapColor::UndefinedMethod)
		component_colors[0]->setEnabled(true);
	
	int stretch_row = qMax(stretch_row0, components_row0+num_editors);
	if (stretch_row != stretch_row0)
	{
		prof_color_layout->removeWidget(stretch);
		prof_color_layout->addWidget(stretch, stretch_row, stretch_col0);
		prof_color_layout->setRowStretch(stretch_row, 1);
	}
	
	bool custom_cmyk = false;
	if (color.getCmykColorMethod() == MapColor::SpotColor)
	{
		cmyk_spot_color_option->setChecked(true);
	}
	else if (color.getCmykColorMethod() == MapColor::RgbColor)
	{
		evaluate_rgb_option->setChecked(true);
	}
	else if (color.getCmykColorMethod() == MapColor::CustomColor)
	{
		custom_cmyk_option->setChecked(true);
		custom_cmyk = true;
	}
	c_edit->setEnabled(custom_cmyk);
	m_edit->setEnabled(custom_cmyk);
	y_edit->setEnabled(custom_cmyk);
	k_edit->setEnabled(custom_cmyk);
	
	
	const MapColorRgb& rgb = color.getRgb();
	r_edit->setValue(255.0 * rgb.r);
	g_edit->setValue(255.0 * rgb.g);
	b_edit->setValue(255.0 * rgb.b);
	html_edit->setText(QColor(rgb).name());
	
	bool custom_rgb = false;
	if (color.getRgbColorMethod() == MapColor::SpotColor)
	{
		rgb_spot_color_option->setChecked(true);
	}
	else if (color.getRgbColorMethod() == MapColor::CmykColor)
	{
		evaluate_cmyk_option->setChecked(true);
	}
	else if (color.getRgbColorMethod() == MapColor::CustomColor)
	{
		custom_rgb_option->setChecked(true);
		custom_rgb = true;
	}
	r_edit->setEnabled(custom_rgb);
	g_edit->setEnabled(custom_rgb);
	b_edit->setEnabled(custom_rgb);
	html_edit->setEnabled(false); // TODO: Editor
	
	
	react_to_changes = true;
}

void ColorDialog::accept()
{
	QSettings settings;
	settings.beginGroup("ColorDialog");
	settings.setValue("view", properties_widget->currentWidget()->objectName());
	settings.endGroup();
	
	QDialog::accept();
}

void ColorDialog::reset()
{
	color = source_color;
	updateWidgets();
	setColorModified(false);
}

void ColorDialog::setColorModified(bool modified)
{
	if (color_modified != modified)
	{
		color_modified = modified;
		updateButtons();
	}
}

void ColorDialog::updateButtons()
{
	ok_button->setEnabled(color_modified);
	reset_button->setEnabled(color_modified);
}

void ColorDialog::showHelp()
{
	Util::showHelp(this, "color_dock_widget.html", "editor");
}

// slot
void ColorDialog::mapColorNameChanged()
{
	if (!react_to_changes)
		return;
	
	color.setName(mc_name_edit->text());
	
	setColorModified();
}

void ColorDialog::spotColorTypeChanged(int id)
{
	if (!react_to_changes)
		return;
	
	QString name;
	switch (id)
	{
		case MapColor::SpotColor:
			name = color.getName();
			if (name.isEmpty())
				name = "?";
			color.setSpotColorName(name);
			break;
		case MapColor::CustomColor:
			color.setSpotColorComposition(SpotColorComponents());
			break;
		default:
			; // nothing
	}
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::spotColorNameChanged()
{
	if (!react_to_changes)
		return;
	
	Q_ASSERT(full_tone_option->isChecked());
	color.setSpotColorName(sc_name_edit->text());
	
	setColorModified();
}

void ColorDialog::spotColorCompositionChanged()
{
	if (!react_to_changes)
		return;
	
	SpotColorComponents components;
	SpotColorComponent component;
	int num_editors = component_colors.size();
	components.reserve(num_editors);
	for (int i = 0; i < num_editors; i++)
	{
		if (!component_colors[i]->isEnabled())
			break;
		
		const MapColor* spot_color = component_colors[i]->color();
		if (spot_color == NULL)
			continue;
		
		component.spot_color = spot_color;
		component.factor = component_halftone[i]->value() / 100.0f;
		components.push_back(component);
	}
	color.setSpotColorComposition(components);
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::knockoutChanged()
{
	if (!react_to_changes)
		return;
	
	color.setKnockout(knockout_option->isChecked());
	setColorModified();
}

void ColorDialog::cmykColorTypeChanged(int id)
{
	if (!react_to_changes)
		return;
	
	switch (id)
	{
		case MapColor::SpotColor:
			color.setCmykFromSpotColors();
			break;
		case MapColor::RgbColor:
// 			color.setRgb(color.getRgb());
			color.setCmykFromRgb();
			break;
		case MapColor::CustomColor:
			color.setCmyk(color.getCmyk());
			break;
		default:
			; // nothing
	}
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::cmykValueChanged()
{
	if (!react_to_changes)
		return;
	
	if (custom_cmyk_option->isChecked())
	{
		color.setCmyk( MapColorCmyk(
		  c_edit->value()/100.0, m_edit->value()/100.0, y_edit->value()/100.0, k_edit->value()/100.0
		) );
		updateWidgets();
		setColorModified(true);
	}
}

void ColorDialog::rgbColorTypeChanged(int id)
{
	if (!react_to_changes)
		return;
	
	switch (id)
	{
		case MapColor::SpotColor:
			color.setRgbFromSpotColors();
			break;
		case MapColor::CmykColor:
// 			color.setCmyk(color.getCmyk());
			color.setRgbFromCmyk();
			break;
		case MapColor::CustomColor:
			color.setRgb(color.getRgb());
			break;
		default:
			; // nothing
	}
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::rgbValueChanged()
{
	if (!react_to_changes)
		return;
	
	if (custom_rgb_option->isChecked())
	{
		color.setRgb( MapColorRgb(
		  r_edit->value()/255.0, g_edit->value()/255.0, b_edit->value()/255.0
		) );
		updateWidgets();
		setColorModified(true);
	}
}

