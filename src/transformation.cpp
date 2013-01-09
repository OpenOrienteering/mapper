/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#include "transformation.h"

#include <qmath.h>

#include "map_coord.h"
#include "matrix.h"
#include "template.h"

// ### PassPoint ###

void PassPoint::save(QIODevice* file)
{
	file->write((const char*)&src_coords, sizeof(MapCoordF));
	file->write((const char*)&dest_coords, sizeof(MapCoordF));
	file->write((const char*)&calculated_coords, sizeof(MapCoordF));
	file->write((const char*)&error, sizeof(double));
}

void PassPoint::load(QIODevice* file, int version)
{
	if (version < 27)
	{
		MapCoordF src_coords_template;
		file->read((char*)&src_coords_template, sizeof(MapCoordF));
	}
	file->read((char*)&src_coords, sizeof(MapCoordF));
	file->read((char*)&dest_coords, sizeof(MapCoordF));
	file->read((char*)&calculated_coords, sizeof(MapCoordF));
	file->read((char*)&error, sizeof(double));
}

void PassPoint::save(QXmlStreamWriter& xml)
{
	xml.writeStartElement("passpoint");
	xml.writeAttribute("error", QString::number(error));
	
	xml.writeStartElement("source");
	src_coords.toMapCoord().save(xml);
	xml.writeEndElement();
	
	xml.writeStartElement("destination");
	dest_coords.toMapCoord().save(xml);
	xml.writeEndElement();
	
	xml.writeStartElement("calculated");
	calculated_coords.toMapCoord().save(xml);
	xml.writeEndElement();
	
	xml.writeEndElement(/*passpoint*/);
}

PassPoint PassPoint::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == "passpoint");
	
	PassPoint p;
	p.error = xml.attributes().value("error").toString().toDouble();
	while (xml.readNextStartElement())
	{
		QStringRef name = xml.name();
		MapCoord coord;
		while (xml.readNextStartElement())
		{
			if (xml.name() == "coord")
			{
				if (name == "source")
					p.src_coords = MapCoordF(MapCoord::load(xml));
				else if (name == "destination")
					p.dest_coords = MapCoordF(MapCoord::load(xml));
				else if (name == "calculated")
					p.calculated_coords = MapCoordF(MapCoord::load(xml));
				else
					xml.skipCurrentElement(); // unsupported
			}
			else
				xml.skipCurrentElement(); // unsupported
		}
	}
	return p;
}



// ### PassPointList ###

bool PassPointList::estimateSimilarityTransformation(TemplateTransform* transform)
{
	int num_pass_points = (int)size();
	if (num_pass_points == 1)
	{
		PassPoint* point = &at(0);
		MapCoordF offset = point->dest_coords - point->src_coords;
		
		transform->template_x += qRound64(1000 * offset.getX());
		transform->template_y += qRound64(1000 * offset.getY());
		point->calculated_coords = point->dest_coords;
		point->error = 0;
	}
	else if (num_pass_points >= 2)
	{
		// Create linear equation system and solve using the pseuo inverse
		
		// Derivation:
		// (Attention: not by a mathematician. Please correct any errors.)
		//
		// Start by stating that the original coordinates (x, y) multiplied
		// with the transformation matrix should give the desired coordinates (X, Y):
		//
		// | a  b  c|   |x|   |X|
		// | d  e  f| * |y| = |Y|
		//              |1|
		//
		// The parametrization of the transformation matrix should be simplified because
		// we want to have isotropic scaling.
		// With s = scaling, r = rotation and (x, y) = offset, it looks like this:
		//
		// | s*cos(r) s*sin(r) x|    | a  b  c|
		// |-s*sin(r) s*cos(r) y| =: |-b  a  d|
		// 
		// With this, reordering the matrices to have the unknowns
		// in the second matrix results in:
		// 
		// | x  y  1  0|   |a|   |X|
		// | y -x  0  1| * |b| = |Y|
		//                 |c|
		//                 |d|
		//
		// For every pass point, two rows like this result. The complete, stacked
		// equation system is then "solved" as good as possible using the
		// pseudo inverse. Finally, s, r, x and y are recovered from a, b, c and d.
		
		Matrix mat(2*num_pass_points, 4);
		Matrix values(2*num_pass_points, 1);
		for (int i = 0; i < num_pass_points; ++i)
		{
			PassPoint* point = &at(i);
			mat.set(2*i, 0, point->src_coords.getX());
			mat.set(2*i, 1, point->src_coords.getY());
			mat.set(2*i, 2, 1);
			mat.set(2*i, 3, 0);
			mat.set(2*i+1, 0, point->src_coords.getY());
			mat.set(2*i+1, 1, -point->src_coords.getX());
			mat.set(2*i+1, 2, 0);
			mat.set(2*i+1, 3, 1);
			
			values.set(2*i, 0, point->dest_coords.getX());
			values.set(2*i+1, 0, point->dest_coords.getY());
		}
		
		Matrix transposed;
		mat.transpose(transposed);
		
		Matrix mat_temp, mat_temp2, pseudo_inverse;
		transposed.multiply(mat, mat_temp);
		if (!mat_temp.invert(mat_temp2))
			return false;
		mat_temp2.multiply(transposed, pseudo_inverse);
		
		// Calculate transformation parameters
		Matrix output;
		pseudo_inverse.multiply(values, output);
		
		double move_x = output.get(2, 0);
		double move_y = output.get(3, 0);
		double rotation = qAtan2((-1) * output.get(1, 0), output.get(0, 0));
		double scale = output.get(0, 0) / qCos(rotation);
		
		// Calculate transformation matrix
		double cosr = cos(rotation);
		double sinr = sin(rotation);
		
		Matrix trans_change(3, 3);
		trans_change.set(0, 0, scale * cosr);
		trans_change.set(0, 1, scale * (-sinr));
		trans_change.set(1, 0, scale * sinr);
		trans_change.set(1, 1, scale * cosr);
		trans_change.set(0, 2, move_x);
		trans_change.set(1, 2, move_y);
		
		// Transform the original transformation parameters to get the new transformation
		transform->template_scale_x *= scale;
		transform->template_scale_y *= scale;
		transform->template_rotation -= rotation;
		qint64 temp_x = qRound64(1000.0 * (trans_change.get(0, 0) * (transform->template_x/1000.0) + trans_change.get(0, 1) * (transform->template_y/1000.0) + trans_change.get(0, 2)));
		transform->template_y = qRound64(1000.0 * (trans_change.get(1, 0) * (transform->template_x/1000.0) + trans_change.get(1, 1) * (transform->template_y/1000.0) + trans_change.get(1, 2)));
		transform->template_x = temp_x;
		
		// Transform the pass points and calculate error
		for (int i = 0; i < num_pass_points; ++i)
		{
			PassPoint* point = &at(i);
			
			point->calculated_coords = MapCoordF(trans_change.get(0, 0) * point->src_coords.getX() + trans_change.get(0, 1) * point->src_coords.getY() + trans_change.get(0, 2),
												 trans_change.get(1, 0) * point->src_coords.getX() + trans_change.get(1, 1) * point->src_coords.getY() + trans_change.get(1, 2));
			point->error = point->calculated_coords.lengthTo(point->dest_coords);
		}
	}
	
	return true;
}

bool PassPointList::estimateNonIsometricSimilarityTransform(QTransform* out)
{
	int num_pass_points = (int)size();
	assert(num_pass_points >= 3);
	
	// Create linear equation system and solve using the pseuo inverse
	
	// Derivation: see comment in estimateSimilarityTransformation().
	// Here, the resulting matrices look a bit different because the constraint
	// to have isotropic scaling is omitted:
	//
	// | x  y  1  0  0  0|   |a|   |X|
	// | 0  0  0  x  y  1| * |b| = |Y|
	//                       |c|
	//                       |d|
	//                       |e|
	//                       |f|
	
	Matrix mat(2*num_pass_points, 6);
	Matrix values(2*num_pass_points, 1);
	for (int i = 0; i < num_pass_points; ++i)
	{
		PassPoint* point = &at(i);
		mat.set(2*i, 0, point->src_coords.getX());
		mat.set(2*i, 1, point->src_coords.getY());
		mat.set(2*i, 2, 1);
		mat.set(2*i, 3, 0);
		mat.set(2*i, 4, 0);
		mat.set(2*i, 5, 0);
		mat.set(2*i+1, 0, 0);
		mat.set(2*i+1, 1, 0);
		mat.set(2*i+1, 2, 0);
		mat.set(2*i+1, 3, point->src_coords.getX());
		mat.set(2*i+1, 4, point->src_coords.getY());
		mat.set(2*i+1, 5, 1);
		
		values.set(2*i, 0, point->dest_coords.getX());
		values.set(2*i+1, 0, point->dest_coords.getY());
	}
	
	Matrix transposed;
	mat.transpose(transposed);
	
	Matrix mat_temp, mat_temp2, pseudo_inverse;
	transposed.multiply(mat, mat_temp);
	if (!mat_temp.invert(mat_temp2))
		return false;
	mat_temp2.multiply(transposed, pseudo_inverse);
	
	// Calculate transformation parameters
	Matrix output;
	pseudo_inverse.multiply(values, output);
	
	out->setMatrix(
		output.get(0, 0), output.get(1, 0), output.get(2, 0),
		output.get(3, 0), output.get(4, 0), output.get(5, 0),
		0, 0, 1);
	return true;
}


void qTransformToTemplateTransform(const QTransform& in, TemplateTransform* out)
{
	out->template_x = qRound64(1000 * in.m13());
	out->template_y = qRound64(1000 * in.m23());
	
	out->template_rotation = qAtan2(-in.m21(), in.m11());
	
	out->template_scale_x = in.m11() / qCos(out->template_rotation);
	out->template_scale_y = in.m22() / qCos(out->template_rotation);
}
