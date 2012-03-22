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


#include "template_image.h"

#include <QtGui>

#include "util.h"

TemplateImage::TemplateImage(const QString& filename, Map* map) : Template(filename, map)
{
	image = new QImage(filename);
	if (image->isNull())
	{
		delete image;
		image = NULL;
		return;
	}
	
	template_valid = true;
}
TemplateImage::TemplateImage(const TemplateImage& other) : Template(other)
{
	image = new QImage(*other.image);
}
TemplateImage::~TemplateImage()
{
	delete image;
}
Template* TemplateImage::duplicate()
{
	TemplateImage* copy = new TemplateImage(*this);
	return copy;
}
bool TemplateImage::saveTemplateFile()
{
	return image->save(template_path);
}

bool TemplateImage::open(QWidget* dialog_parent, MapView* main_view)
{
	if (getTemplateFilename().endsWith(".gif", Qt::CaseInsensitive))
		QMessageBox::warning(dialog_parent, tr("Warning"), tr("Loading a GIF image template.\nSaving GIF files is not supported. This means that drawings on this template won't be saved!\nIf you do not intend to draw on this template however, that is no problem."));
	
	TemplateImageOpenDialog open_dialog(dialog_parent);
	open_dialog.setWindowModality(Qt::WindowModal);
	if (open_dialog.exec() == QDialog::Rejected)
		return false;
	
	cur_trans.template_scale_x = open_dialog.getMpp(map);
	cur_trans.template_scale_y = cur_trans.template_scale_x;
	
	cur_trans.template_x = main_view->getPositionX();
	cur_trans.template_y = main_view->getPositionY();
	
	updateTransformationMatrices();
	
	return true;
}
void TemplateImage::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	painter->setOpacity(opacity);
	painter->drawImage(QPointF(-image->width() * 0.5, -image->height() * 0.5), *image);
}
QRectF TemplateImage::getExtent()
{
    // If the image is invalid, the extent is an empty rectangle.
    if (!image) return QRectF();
	return QRectF(-image->width() * 0.5, -image->height() * 0.5, image->width(), image->height());
}

QPointF TemplateImage::calcCenterOfGravity(QRgb background_color)
{
	int num_points = 0;
	QPointF center = QPointF(0, 0);
	int width = image->width();
	int height = image->height();
	
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			QRgb pixel = image->pixel(x, y);
			if (qAlpha(pixel) < 127 || pixel == background_color)
				continue;
			
			center += QPointF(x, y);
			++num_points;
		}
	}
	
	if (num_points > 0)
		center = QPointF(center.x() / num_points, center.y() / num_points);
	center -= QPointF(image->width() * 0.5 - 0.5, image->height() * 0.5 - 0.5);
	
	return center;
}

double TemplateImage::getTemplateFinalScaleX() const
{
	return cur_trans.template_scale_x * 1000 / map->getScaleDenominator();
}
double TemplateImage::getTemplateFinalScaleY() const
{
	return cur_trans.template_scale_y * 1000 / map->getScaleDenominator();
}

void TemplateImage::drawOntoTemplateImpl(QPointF* points, int num_points, QColor color, float width)
{
	for (int i = 0; i < num_points; ++i)
		points[i] = QPointF(points[i].x() + image->width() * 0.5, points[i].y() + image->height() * 0.5);
	
    QPainter painter;
	painter.begin(image);
	if (color.alpha() == 0)
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	else
		painter.setOpacity(color.alphaF());
	
	QPen pen(color);
	pen.setWidthF(width);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing);
	
	painter.drawPolyline(points, num_points);
	
	painter.end();
}
bool TemplateImage::changeTemplateFileImpl(const QString& filename)
{
	QImage* new_image = new QImage(filename);
	if (new_image->isNull())
	{
		delete new_image;
		new_image = NULL;
		return false;
	}
	else
	{
		if (image)
			delete image;
		image = new_image;
		return true;
	}
}

TemplateImageOpenDialog::TemplateImageOpenDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Open image template"));
	
	mpp_radio = new QRadioButton(tr("Meters per pixel:"));
	mpp_edit = new QLineEdit("1");
	mpp_edit->setValidator(new DoubleValidator(0, 999999, mpp_edit));
	
	dpi_radio = new QRadioButton(tr("Scanned with"));
	dpi_edit = new QLineEdit("300");
	dpi_edit->setValidator(new DoubleValidator(1, 999999, dpi_edit));
	dpi_edit->setEnabled(false);
	QLabel* dpi_label = new QLabel(tr("dpi"));
	
	scale_check = new QCheckBox(tr("Different template scale 1 :"));
	scale_check->setEnabled(false);
	scale_edit = new QLineEdit("15000");
	scale_edit->setValidator(new QIntValidator(1, 999999, scale_edit));
	scale_edit->setEnabled(false);
	
	QHBoxLayout* mpp_layout = new QHBoxLayout();
	mpp_layout->addWidget(mpp_radio);
	mpp_layout->addWidget(mpp_edit);
	mpp_layout->addStretch(1);
	QHBoxLayout* dpi_layout = new QHBoxLayout();
	dpi_layout->addWidget(dpi_radio);
	dpi_layout->addWidget(dpi_edit);
	dpi_layout->addWidget(dpi_label);
	dpi_layout->addStretch(1);
	QHBoxLayout* scale_layout = new QHBoxLayout();
	scale_layout->addSpacing(16);
	scale_layout->addWidget(scale_check);
	scale_layout->addWidget(scale_edit);
	scale_layout->addStretch(1);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	open_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("Open"));
	open_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(open_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(mpp_layout);
	layout->addLayout(dpi_layout);
	layout->addLayout(scale_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(open_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(mpp_radio, SIGNAL(clicked(bool)), this, SLOT(mppRadioClicked(bool)));
	connect(dpi_radio, SIGNAL(clicked(bool)), this, SLOT(dpiRadioClicked(bool)));
	connect(scale_check, SIGNAL(clicked(bool)), this, SLOT(scaleCheckClicked(bool)));
	
	mpp_radio->setChecked(true);
}
double TemplateImageOpenDialog::getMpp(Map* map) const
{
	if (mpp_radio->isChecked())
		return mpp_edit->text().toDouble();
	else
	{
		double dpi = dpi_edit->text().toDouble();		// dots/pixels per inch(on map)
		double ipd = 1 / dpi;							// inch(on map) per pixel
		double mpd = ipd * 0.0254;						// meters(on map) per pixel
		double mpp;										// meters(in reality) per pixel
		if (scale_check->isChecked())
			mpp = mpd * scale_edit->text().toDouble();
		else
			mpp = mpd * map->getScaleDenominator();
		return mpp;
	}
}
void TemplateImageOpenDialog::mppRadioClicked(bool checked)
{
	dpi_edit->setEnabled(!checked);
	scale_check->setEnabled(!checked);
	scale_edit->setEnabled(!checked && scale_check->isChecked());
	mpp_edit->setEnabled(checked);
}
void TemplateImageOpenDialog::dpiRadioClicked(bool checked)
{
	dpi_edit->setEnabled(checked);
	scale_check->setEnabled(checked);
	scale_edit->setEnabled(checked && scale_check->isChecked());
	mpp_edit->setEnabled(!checked);
}
void TemplateImageOpenDialog::scaleCheckClicked(bool checked)
{
	scale_edit->setEnabled(checked);
}

#include "template_image.moc"
