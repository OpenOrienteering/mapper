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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map.h"
#include "util.h"

TemplateImage::TemplateImage(const QString& path, Map* map) : Template(path, map)
{
	image = NULL;
}

TemplateImage::~TemplateImage()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}

bool TemplateImage::saveTemplateFile()
{
	return image->save(template_path);
}

void TemplateImage::saveTypeSpecificTemplateConfiguration(QIODevice* stream)
{
    // TODO: save filtering parameter here
}

bool TemplateImage::loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version)
{
	// TODO: load filtering parameter here
	return true;
}

bool TemplateImage::loadTemplateFileImpl(bool configuring)
{
	image = new QImage(template_path);
	if (image->isNull())
	{
		delete image;
		image = NULL;
		return false;
	}
	else
		return true;
}
bool TemplateImage::postLoadConfiguration(QWidget* dialog_parent)
{
	if (getTemplateFilename().endsWith(".gif", Qt::CaseInsensitive))
		QMessageBox::warning(dialog_parent, tr("Warning"), tr("Loading a GIF image template.\nSaving GIF files is not supported. This means that drawings on this template won't be saved!\nIf you do not intend to draw on this template however, that is no problem."));
	
	TemplateImageOpenDialog open_dialog(this, dialog_parent);
	open_dialog.setWindowModality(Qt::WindowModal);
	if (open_dialog.exec() == QDialog::Rejected)
		return false;
	
	is_georeferenced = false;	// TODO
	
	if (!is_georeferenced)
	{
		transform.template_scale_x = open_dialog.getMpp() * 1000.0 / map->getScaleDenominator();
		transform.template_scale_y = transform.template_scale_x;
		
		//transform.template_x = main_view->getPositionX();
		//transform.template_y = main_view->getPositionY();
		
		updateTransformationMatrices();
	}
	
	return true;
}

void TemplateImage::unloadTemplateFileImpl()
{
	delete image;
	image = NULL;
}

void TemplateImage::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	if (!is_georeferenced)
		applyTemplateTransform(painter);
	else
	{
		// TODO
	}
	
	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->setOpacity(opacity);
	painter->drawImage(QPointF(-image->width() * 0.5, -image->height() * 0.5), *image);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
}
QRectF TemplateImage::getTemplateExtent()
{
    // If the image is invalid, the extent is an empty rectangle.
    if (!image) return QRectF();
	return QRectF(-image->width() * 0.5, -image->height() * 0.5, image->width(), image->height());
}

QRectF TemplateImage::calculateTemplateBoundingBox()
{
	if (!is_georeferenced)
		return Template::calculateTemplateBoundingBox();
	else
	{
		// TODO!
		return QRectF();
	}
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

Template* TemplateImage::duplicateImpl()
{
	TemplateImage* new_template = new TemplateImage(template_path, map);
	new_template->image = new QImage(*image);
	return new_template;
}

void TemplateImage::drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width)
{
	QPointF* points = new QPointF[num_coords];
	
	if (is_georeferenced)
	{
		// TODO!
		return;
	}
	else
	{
		for (int i = 0; i < num_coords; ++i)
			points[i] = mapToTemplateQPoint(coords[i]) + QPointF(image->width() * 0.5, image->height() * 0.5);
	}
	
	// This conversion is to prevent a very strange bug where the behavior of the
	// default QPainter composition mode seems to be incorrect for images which are
	// loaded from a file without alpha and then painted over with the eraser
	if (color.alpha() == 0 && image->format() != QImage::Format_ARGB32_Premultiplied)
		*image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
	
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
	
	painter.drawPolyline(points, num_coords);
	
	painter.end();
}


// ### TemplateImageOpenDialog ###

TemplateImageOpenDialog::TemplateImageOpenDialog(TemplateImage* image, QWidget* parent)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), image(image)
{
	setWindowTitle(tr("Opening %1").arg(image->getTemplateFilename()));
	
	QLabel* size_label = new QLabel("<b>" + tr("Image size:") + QString("</b> %1 x %2")
		.arg(image->getQImage()->width()).arg(image->getQImage()->height()));
	
	bool use_meters_per_pixel;
	double meters_per_pixel;
	double dpi;
	double scale;
	image->getMap()->getImageTemplateDefaults(use_meters_per_pixel, meters_per_pixel, dpi, scale);
	
	mpp_radio = new QRadioButton(tr("Meters per pixel:"));
	mpp_edit = new QLineEdit((meters_per_pixel > 0) ? QString::number(meters_per_pixel) : "");
	mpp_edit->setValidator(new DoubleValidator(0, 999999, mpp_edit));
	
	dpi_radio = new QRadioButton(tr("Scanned with"));
	dpi_edit = new QLineEdit((dpi > 0) ? QString::number(dpi) : "");
	dpi_edit->setValidator(new DoubleValidator(1, 999999, dpi_edit));
	QLabel* dpi_label = new QLabel(tr("dpi"));
	
	QLabel* scale_label = new QLabel(tr("Template scale:  1 :"));
	scale_edit = new QLineEdit((scale > 0) ? QString::number(scale) : "");
	scale_edit->setValidator(new QIntValidator(1, 999999, scale_edit));
	
	if (use_meters_per_pixel)
		mpp_radio->setChecked(true);
	else
		dpi_radio->setChecked(true);
	
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
	scale_layout->addWidget(scale_label);
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
	layout->addWidget(size_label);
	layout->addSpacing(16);
	layout->addLayout(mpp_layout);
	layout->addLayout(dpi_layout);
	layout->addLayout(scale_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(mpp_edit, SIGNAL(textEdited(QString)), this, SLOT(setOpenEnabled()));
	connect(dpi_edit, SIGNAL(textEdited(QString)), this, SLOT(setOpenEnabled()));
	connect(scale_edit, SIGNAL(textEdited(QString)), this, SLOT(setOpenEnabled()));
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(open_button, SIGNAL(clicked(bool)), this, SLOT(doAccept()));
	connect(mpp_radio, SIGNAL(clicked(bool)), this, SLOT(radioClicked()));
	connect(dpi_radio, SIGNAL(clicked(bool)), this, SLOT(radioClicked()));
	
	radioClicked();
}
double TemplateImageOpenDialog::getMpp() const
{
	if (mpp_radio->isChecked())
		return mpp_edit->text().toDouble();
	else
	{
		double dpi = dpi_edit->text().toDouble();			// dots/pixels per inch(on map)
		double ipd = 1 / dpi;								// inch(on map) per pixel
		double mpd = ipd * 0.0254;							// meters(on map) per pixel	
		double mpp = mpd * scale_edit->text().toDouble();	// meters(in reality) per pixel
		return mpp;
	}
}
void TemplateImageOpenDialog::radioClicked()
{
	bool mpp_checked = mpp_radio->isChecked();
	dpi_edit->setEnabled(!mpp_checked);
	scale_edit->setEnabled(!mpp_checked);
	mpp_edit->setEnabled(mpp_checked);
	setOpenEnabled();
}
void TemplateImageOpenDialog::setOpenEnabled()
{
	if (mpp_radio->isChecked())
		open_button->setEnabled(!mpp_edit->text().isEmpty());
	else
		open_button->setEnabled(!scale_edit->text().isEmpty() && !dpi_edit->text().isEmpty());
}
void TemplateImageOpenDialog::doAccept()
{
	image->getMap()->setImageTemplateDefaults(mpp_radio->isChecked(), mpp_edit->text().toDouble(), dpi_edit->text().toDouble(), scale_edit->text().toDouble());
	accept();
}
