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


#include "gps_coordinates.h"

#include <QtGui>

GPSProjectionParameters::GPSProjectionParameters()
{
	a = 6378137.0;
	b = 6356752.3142;
	center_latitude = 0;
	center_longitude = 0;
}
void GPSProjectionParameters::update()
{
	e_sq = (a*a - b*b) / (a*a);
	cos_center_latitude = cos(center_latitude);
	sin_center_latitude = sin(center_latitude);
	v0 = a / sqrt(1 - e_sq * sin_center_latitude*sin_center_latitude);
}

GPSCoordinate::GPSCoordinate()
{
	latitude = 0;
	longitude = 0;
}
GPSCoordinate::GPSCoordinate(double latitude, double longitude, bool given_in_degrees) : latitude(latitude), longitude(longitude)
{
	if (given_in_degrees)
	{
		this->latitude = latitude * M_PI / 180;
		this->longitude = longitude * M_PI / 180;
	}
}
GPSCoordinate::GPSCoordinate(MapCoordF map_coord, const GPSProjectionParameters& params)
{
	const int MAX_ITERATIONS = 20;						// TODO: is that ok?
	const double INSIGNIFICANT_CHANGE = 0.000000001;	// TODO: is that ok?
	
	double start_E = map_coord.getX();
	double start_N = (-1) * map_coord.getY();
	
	latitude = params.center_latitude;	// TODO: would a better initial guess bring benefits?
	longitude = params.center_longitude;
	for (int i = 0; i < MAX_ITERATIONS; ++i)
	{
		double sin_l = sin(latitude);
		double cos_l = cos(latitude);
		double sin_dlong = sin(longitude - params.center_longitude);
		double cos_dlong = cos(longitude - params.center_longitude);
		
		double denominator_inner = 1 - params.e_sq * sin_l*sin_l;
		double v = params.a / sqrt(denominator_inner);
		double p = params.a*(1 - params.e_sq) / pow(denominator_inner, 1.5);	// meridian
		
		double E_test = v*cos_l*sin(longitude - params.center_longitude);
		double N_test = v*(sin_l*params.cos_center_latitude - cos_l*params.sin_center_latitude*cos(longitude - params.center_longitude)) +
		                   params.e_sq*(params.v0*params.sin_center_latitude - v*sin_l)*params.cos_center_latitude;
		
		double J11 = -p*sin_l*sin_dlong;
		double J12 = v*cos_l*cos_dlong;
		double J21 = p*(cos_l*params.cos_center_latitude + sin_l*params.sin_center_latitude*cos_dlong);
		double J22 = v*params.sin_center_latitude*cos_l*sin_dlong;
		double D = J11*J22 - J12*J21;
		
		double dE = start_E - E_test;
		double dN = start_N - N_test;
		
		double d_latitude = (J22*dE - J12*dN) / D;
		double d_longitude = (-J21*dE + J11*dN) / D;
		latitude = latitude + d_latitude;
		longitude = longitude + d_longitude;
		
		if (qMax(qAbs(d_latitude), qAbs(d_longitude)) < INSIGNIFICANT_CHANGE)
			break;
	}
}
MapCoordF GPSCoordinate::toMapCoordF(const GPSProjectionParameters& params)
{
	double sin_l = sin(latitude);
	double cos_l = cos(latitude);
	
	double v = params.a / sqrt(1 - params.e_sq * sin_l*sin_l);
	
	return MapCoordF(v*cos_l*sin(longitude - params.center_longitude),
					 (-1) * (v*(sin_l*params.cos_center_latitude - cos_l*params.sin_center_latitude*cos(longitude - params.center_longitude)) +
					    params.e_sq*(params.v0*params.sin_center_latitude - v*sin_l)*params.cos_center_latitude));
}

void GPSCoordinate::toCartesianCoordinates(const GPSProjectionParameters& params, double height, double& x, double& y, double& z)
{
	double alpha = acos(params.b / params.a);
	double N = params.a / sqrt(1 - (sin(latitude)*sin(alpha))*(sin(latitude)*sin(alpha)));
	
	x = (N + height)*cos(latitude)*cos(longitude);
	y = (N + height)*cos(latitude)*sin(longitude);
	z = (cos(alpha)*cos(alpha)*N + height)*sin(latitude);
}

bool GPSCoordinate::fromString(QString str)
{
	// TODO: This cannot handle spaces in some in-between positions, e.g. "S 48° 31' 43.932\" E 12° 8' 25.332\"" or "S 48° 31.732 E 012° 08.422"
	
	QString temp = "";
	double temp_latitude;
	double temp_longitude;
	bool temp_latitude_set = false;
	
	QChar letter = 0;
	double degrees = 0;
	bool degrees_set = false;
	bool minutes_set = false;
	
	int str_len = str.length();
	for (int i = 0; i <= str_len; ++i)
	{
		const QChar c = (i < str_len) ? str[i].toUpper() : ' ';
		
		if ((c >= '0' && c <= '9') || (c == '.') || (c == '-'))
			temp.append(c);
		else if (c.unicode() == 176)	// '°'
		{
			degrees = temp.toDouble();
			degrees_set = true;
			temp = "";
		}
		else if (c.unicode() == 194)	// comes before '°'
			continue;
		else if (c == '"' || ((c == '\'') && (i < str_len - 1) && (str[i+1] == '\'')))
		{
			degrees += temp.toDouble() / (60.0*60.0);
			temp = "";
		}
		else if (c == '\'')
		{
			degrees += temp.toDouble() / 60.0;
			minutes_set = true;
			temp = "";
		}
		else if (c == 'N' || c == 'E' || c == 'S' || c == 'W')
		{
			letter = c;
		}
		else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
		{
			if (temp.isEmpty() && !degrees_set)
				continue;
			
			if (!temp.isEmpty())
			{
				if (!degrees_set)
					degrees = temp.toDouble();
				else if (!minutes_set)
					degrees += temp.toDouble() / 60.0;
				else
					degrees += temp.toDouble() / (60.0*60.0);
				
				temp = "";
			}
			
			if (letter == 'S' || letter == 'W')
				degrees = -degrees;
			
			if (letter == 'N' || letter == 'S')
			{
				temp_latitude = degrees;
				temp_latitude_set = true;
			}
			else if (letter == 'W' || letter == 'E')
			{
				temp_longitude = degrees;
			}
			else if (temp_latitude_set)
				temp_longitude = degrees;
			else
			{
				temp_latitude = degrees;
				temp_latitude_set = true;
			}
			
			letter = 0;
			degrees = 0;
			degrees_set = false;
			minutes_set = false;
		}
		else
			return false;
	}
	
	latitude = temp_latitude * M_PI / 180;
	longitude = temp_longitude * M_PI / 180;
	return true;
}

// ### GPSProjectionParametersDialog ###

GPSProjectionParametersDialog::GPSProjectionParametersDialog(QWidget* parent, const GPSProjectionParameters* initial_values) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("GPS coordinates projection parameters"));
	
	if (initial_values)
		params = *initial_values;
	
	QLabel* projection_label = new QLabel(tr("Orthographic projection:"));
	projection_label->setAlignment(Qt::AlignCenter);
	QLabel* lat_label = new QLabel(tr("Origin latitude <b>phi 0</b>:"));
	lat_edit = new QLineEdit(QString::number(params.center_latitude * 180 / M_PI, 'f', 12));
	QLabel* lon_label = new QLabel(tr("Origin longitude <b>lambda 0</b>:"));
	lon_edit = new QLineEdit(QString::number(params.center_longitude * 180 / M_PI, 'f', 12));
	
	QGridLayout* edit_layout = new QGridLayout();
	edit_layout->addWidget(projection_label, 0, 0, 1, 2);
	edit_layout->addWidget(lat_label, 1, 0);
	edit_layout->addWidget(lat_edit, 1, 1);
	edit_layout->addWidget(lon_label, 2, 0);
	edit_layout->addWidget(lon_edit, 2, 1);
	
	edit_layout->setRowStretch(7, 1);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("Ok"));
	ok_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(edit_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(lat_edit, SIGNAL(textChanged(QString)), this, SLOT(editChanged()));
	connect(lon_edit, SIGNAL(textChanged(QString)), this, SLOT(editChanged()));
}
void GPSProjectionParametersDialog::editChanged()
{
	bool ok = false;
	
	params.center_latitude = lat_edit->text().toDouble(&ok) * M_PI / 180;
	if (!ok)
	{
		ok_button->setEnabled(false);
		return;
	}
	
	params.center_longitude = lon_edit->text().toDouble(&ok) * M_PI / 180;
	if (!ok)
	{
		ok_button->setEnabled(false);
		return;
	}
	
	ok_button->setEnabled(true);
}

#include "moc_gps_coordinates.cpp"
