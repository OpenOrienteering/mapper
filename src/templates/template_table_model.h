/*
 *    Copyright 2020 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_TABLE_MODEL_H
#define OPENORIENTEERING_TEMPLATE_TABLE_MODEL_H

#include <Qt>
#include <QAbstractTableModel>
#include <QObject>
#include <QString>
#include <QVariant>

class QModelIndex;

namespace OpenOrienteering {

class Map;
class MapView;
class Template;


/**
 * An item model representing template and map visibility properties.
 * 
 * Each template, and also the map, is represented by a row in the model.
 */
class TemplateTableModel :  public QAbstractTableModel
{
	Q_OBJECT
	
public:
	static constexpr int visibilityColumn() { return 0; }
	static constexpr int opacityColumn() { return 1; }
	static constexpr int groupColumn() { return 2; }
	static constexpr int nameColumn() { return 3; }
	
	/**
	 * States of template loading to be signaled when loading is triggered via this model.
	 */
	enum TemplateLoadingState
	{
		StateLoadingFailed,
		StateLoadingStarted,
		StateLoaded
	};
	
	~TemplateTableModel() override;
	TemplateTableModel(Map& map, MapView& view, QObject* parent = nullptr);
	
	TemplateTableModel(const TemplateTableModel&) = delete;
	TemplateTableModel(TemplateTableModel&&) = delete;
	TemplateTableModel& operator=(const TemplateTableModel&) = delete;
	TemplateTableModel& operator=(TemplateTableModel&&) = delete;
	
	
	/**
	 * Sets the header decorator item of the visibility columnn.
	 */
	void setCheckBoxDecorator(QVariant decorator);
	
	QVariant checkBoxDecorator() const noexcept { return checkbox_decorator; }
	
	
	/**
	 * Configure the model for touch mode.
	 * 
	 * By default, template names and "- Map -" label are displayed (only) in
	 * the name column, and the opacity column is editable.
	 * In touch mode, the labels are displayed (also) in the first column, and
	 * the opacity column is not editable so that a more comfortable way of
	 * editing can be implemented.
	 */
	void setTouchMode(bool enabled);
	
	bool touchMode() const noexcept { return touch_mode; }
	
	
	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	
	int posFromRow(int row) const;
	int rowFromPos(int pos) const;
	int insertionRowFromPos(int pos, bool in_background) const;
	int mapRow(int first_front_template) const;
	
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	
signals:
	/**
	 * Signals changes of the state of template loading when it is triggered
	 * via this model.
	 */
	void templateLoadingChanged(const ::OpenOrienteering::Template* temp, int row, int state);
	
protected:
	QVariant mapData(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant templateData(Template* temp, const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setMapData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	bool setTemplateData(Template* temp, const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	
	void onFirstFrontTemplateAboutToBeChanged(int old_pos, int new_pos);
	void onFirstFrontTemplateChanged(int old_pos, int new_pos);
	void onTemplateAboutToBeAdded(int pos, Template* temp, bool in_background);
	void onTemplateAdded(int pos, Template* temp);
	void onTemplateChanged(int pos, Template* temp);
	void onTemplateAboutToBeMoved(int old_pos, int new_pos);
	void onTemplateMoved();
	void onTemplateAboutToBeDeleted(int pos, Template* temp);
	void onTemplateDeleted();
	void onTemplateStateChanged();
	
	void loadTemplate(Template* temp, int row);
	
private:
	Map& map;
	MapView& view;
	Template* to_be_loaded = nullptr;
	QVariant checkbox_decorator;
	bool touch_mode = false;
	
};


}  // namespace OpenOrienteering

#endif
