/*
 *    Copyright 2011 Thomas Schöps
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
	pixmap = new QPixmap(filename);
	if (pixmap->isNull())
	{
		delete pixmap;
		pixmap = NULL;
		return;
	}
	
	template_valid = true;
}
TemplateImage::TemplateImage(const TemplateImage& other) : Template(other)
{
	pixmap = new QPixmap(*other.pixmap);
}
TemplateImage::~TemplateImage()
{
	delete pixmap;
}
Template* TemplateImage::duplicate()
{
	TemplateImage* copy = new TemplateImage(*this);
	return copy;
}
bool TemplateImage::saveTemplateFile()
{
	return pixmap->save(template_path);
}

bool TemplateImage::open(QWidget* dialog_parent, MapView* main_view)
{
	TemplateImageOpenDialog open_dialog(dialog_parent);
	open_dialog.setWindowModality(Qt::WindowModal);
	if (open_dialog.exec() == QDialog::Rejected)
		return false;
	
	cur_trans.template_scale_x = open_dialog.getMpp();
	cur_trans.template_scale_y = open_dialog.getMpp();
	
	cur_trans.template_x = main_view->getPositionX();
	cur_trans.template_y = main_view->getPositionY();
	
	updateTransformationMatrices();
	
	return true;
}
void TemplateImage::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	painter->setOpacity(opacity);
	painter->drawPixmap(QPointF(-pixmap->width() * 0.5, -pixmap->height() * 0.5), *pixmap);
}
QRectF TemplateImage::getExtent()
{
	return QRectF(-pixmap->width() * 0.5, -pixmap->height() * 0.5, pixmap->width(), pixmap->height());
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
		points[i] = QPointF(points[i].x() + pixmap->width() * 0.5, points[i].y() + pixmap->height() * 0.5);
	
    QPainter painter;
	painter.begin(pixmap);
	
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
	QPixmap* new_pixmap = new QPixmap(filename);
	if (new_pixmap->isNull())
	{
		delete new_pixmap;
		new_pixmap = NULL;
		return false;
	}
	else
	{
		if (pixmap)
			delete pixmap;
		pixmap = new_pixmap;
		return true;
	}
}

TemplateImageOpenDialog::TemplateImageOpenDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Open image template"));
	
	QLabel* mpp_label = new QLabel(tr("Meters per pixel:"));
	mpp_edit = new QLineEdit("1");
	mpp_edit->setValidator(new DoubleValidator(0, 999999, mpp_edit));
	
	QHBoxLayout* mpp_layout = new QHBoxLayout();
	mpp_layout->addWidget(mpp_label);
	mpp_layout->addWidget(mpp_edit);
	mpp_layout->addStretch(1);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	open_button = new QPushButton(QIcon("images/arrow-right.png"), tr("Open"));
	open_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(open_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(mpp_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(open_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(mpp_edit, SIGNAL(textChanged(QString)), this, SLOT(mppChanged(QString)));
	
	mpp = 1;
}
void TemplateImageOpenDialog::mppChanged(const QString& new_text)
{
	bool ok = false;
	mpp = new_text.toDouble(&ok);
	open_button->setEnabled(ok);
}

#include "template_image.moc"
