/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2016  Kai Pastor
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

#include "editor_settings_page.h"

#include <functional>
#include <limits>

#include <QApplication>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDropEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPalette>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStyle>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "settings.h"
#include "gui/map/map_editor.h"
#include "gui/modifier_key.h"
#include "gui/util_gui.h"
#include "gui/widgets/settings_page.h"


namespace OpenOrienteering {

namespace {

const auto mobile_toolbar_action_mime_type = QStringLiteral("application/x-openorienteering-mobile-toolbar-action");

QString mobileToolbarSelectionSummary(const QString& id)
{
	if (id.isEmpty())
	{
		return QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
		                                   "Select an icon to show its details.");
	}

	auto summary = QStringList{ MapEditorController::mobileToolbarActionLabel(id) };
	if (!MapEditorController::mobileToolbarActionRemovable(id))
		summary << QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Required");
	auto const span = MapEditorController::mobileToolbarActionSpan(id);
	if (span.width() > 1 || span.height() > 1)
	{
		summary << QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "%1x%2 button")
		              .arg(span.width())
		              .arg(span.height());
	}
	if (MapEditorController::mobileToolbarActionAlwaysVisible(id))
	{
		summary << QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
		                                       "Stays visible when the toolbar becomes crowded");
	}
	return summary.join(QStringLiteral(" | "));
}

bool mobileToolbarZoneIsTrailing(MapEditorController::MobileToolbarZone zone)
{
	return zone == MapEditorController::MobileToolbarTopRight
	       || zone == MapEditorController::MobileToolbarBottomRight;
}

struct MobileToolbarPreviewItem
{
	QString id;
	QRect rect;
	int index = -1;
};

struct MobileToolbarPreviewLayout
{
	QVector<MobileToolbarPreviewItem> items;
	int column_count = 1;
};

MobileToolbarPreviewLayout layoutMobileToolbarPreview(
        const QStringList& action_ids,
        bool trailing_alignment,
        const QSize& cell_size
)
{
	struct PendingItem
	{
		QString id;
		int base_col;
		int row;
		int row_span;
		int col_span;
		int index;
	};

	auto pending = QVector<PendingItem>{};
	int col = 0;
	bool row_used[2] = { false, false };
	auto advance_column = [&]() {
		++col;
		row_used[0] = false;
		row_used[1] = false;
	};

	for (int index = 0; index < action_ids.size(); ++index)
	{
		auto const& id = action_ids[index];
		auto const span = MapEditorController::mobileToolbarActionSpan(id);
		auto const row_span = qBound(1, span.height(), 2);
		auto const col_span = qMax(1, span.width());
		auto const preferred_row = MapEditorController::mobileToolbarActionPreferredRow(id);

		if (row_span > 1 || col_span > 1)
		{
			if (row_used[0] || row_used[1])
				advance_column();
			pending.push_back({ id, col, 0, row_span, col_span, index });
			col += col_span;
			row_used[0] = false;
			row_used[1] = false;
			continue;
		}

		int row = preferred_row;
		if (row >= 0)
		{
			if (row_used[row])
				advance_column();
			if (row_used[row])
				continue;
		}
		else
		{
			if (!row_used[0])
				row = 0;
			else if (!row_used[1])
				row = 1;
			else
			{
				advance_column();
				row = 0;
			}
		}

		pending.push_back({ id, col, row, row_span, col_span, index });
		row_used[row] = true;
		if (row_used[0] && row_used[1])
			advance_column();
	}

	auto layout = MobileToolbarPreviewLayout{};
	for (const auto& item : pending)
		layout.column_count = qMax(layout.column_count, item.base_col + item.col_span);

	for (const auto& item : pending)
	{
		auto const display_col = trailing_alignment
		                         ? (layout.column_count - item.base_col - item.col_span)
		                         : item.base_col;
		layout.items.push_back({
		        item.id,
		        QRect(display_col * cell_size.width(),
		              item.row * cell_size.height(),
		              item.col_span * cell_size.width(),
		              item.row_span * cell_size.height()),
		        item.index
		});
	}

	return layout;
}

class MobileToolbarActionCanvas final : public QWidget
{
public:
	enum LayoutRole
	{
		AvailableActions,
		ToolbarZone,
	};

	explicit MobileToolbarActionCanvas(LayoutRole list_role, QWidget* parent = nullptr)
	: QWidget(parent)
	, role(list_role)
	{
		auto const button_size_px = qMax(32, qRound(Util::mmToPixelPhysical(Settings::getInstance().getSetting(Settings::ActionGridBar_ButtonSizeMM).toReal())));
		auto const margin_size_px = qMax(1, button_size_px / 4);
		auto const cell_padding_px = qMax(4, margin_size_px / 2);
		cell_size = QSize(button_size_px + cell_padding_px, button_size_px + cell_padding_px);
		icon_margin_px = qMax(4, cell_padding_px / 2);
		content_height = cell_size.height() * 2;

		setAcceptDrops(true);
		setAutoFillBackground(true);
		setBackgroundRole(QPalette::Base);
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		setFixedHeight(content_height);

		if (role == AvailableActions)
		{
			setToolTip(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
			                                       "Actions are shown in two rows. Drag icons into the selected toolbar area."));
		}
		else
		{
			setToolTip(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
			                                       "The preview uses the same two-row arrangement as the map editor toolbar."));
		}

		refreshLayout();
	}

	void setToolbarZone(MapEditorController::MobileToolbarZone new_zone)
	{
		if (toolbar_zone == new_zone)
			return;
		toolbar_zone = new_zone;
		refreshLayout();
	}

	void setActionIds(const QStringList& ids)
	{
		action_ids = ids;
		if (!action_ids.contains(selected_id))
			selected_id.clear();
		drag_candidate_id.clear();
		refreshLayout();
	}

	QStringList actionIds() const
	{
		return action_ids;
	}

	QString selectedActionId() const
	{
		return selected_id;
	}

	bool hasSelection() const
	{
		return !selected_id.isEmpty();
	}

	void setSelectedAction(const QString& id)
	{
		auto const new_id = action_ids.contains(id) ? id : QString{};
		if (selected_id == new_id)
			return;
		selected_id = new_id;
		update();
		if (selection_changed)
			selection_changed(this);
	}

	void clearSelectedAction()
	{
		setSelectedAction(QString{});
	}

	int contentHeight() const
	{
		return content_height;
	}

	std::function<void(MobileToolbarActionCanvas*)> selection_changed;
	std::function<void(MobileToolbarActionCanvas*)> actions_changed;

protected:
	QSize sizeHint() const override
	{
		return QSize(qMax(1, layout.column_count) * cell_size.width(), content_height);
	}

	QSize minimumSizeHint() const override
	{
		return sizeHint();
	}

	void paintEvent(QPaintEvent* event) override
	{
		Q_UNUSED(event);

		QPainter painter(this);
		painter.fillRect(rect(), palette().color(QPalette::Base));

		auto divider_pen = QPen(palette().color(QPalette::Mid));
		divider_pen.setWidth(1);
		painter.setPen(divider_pen);
		painter.drawRect(rect().adjusted(0, 0, -1, -1));
		painter.drawLine(0, cell_size.height(), width(), cell_size.height());

		for (const auto& item : layout.items)
		{
			auto frame_rect = item.rect.adjusted(1, 1, -1, -1);
			auto outline_color = palette().color(QPalette::Mid);
			auto fill_color = palette().color(QPalette::Base);
			if (item.id == selected_id)
			{
				outline_color = palette().color(QPalette::Highlight);
				fill_color = palette().color(QPalette::Highlight).lighter(175);
			}

			painter.fillRect(frame_rect, fill_color);
			painter.setPen(outline_color);
			painter.drawRect(frame_rect);

			auto icon_rect = item.rect.adjusted(icon_margin_px, icon_margin_px, -icon_margin_px, -icon_margin_px);
			MapEditorController::mobileToolbarActionIcon(item.id).paint(&painter, icon_rect, Qt::AlignCenter);
		}
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() != Qt::LeftButton)
		{
			QWidget::mousePressEvent(event);
			return;
		}

		drag_start_pos = event->pos();
		drag_candidate_id = actionIdAt(event->pos());
		setSelectedAction(drag_candidate_id);
		event->accept();
	}

	void mouseMoveEvent(QMouseEvent* event) override
	{
		if (!(event->buttons() & Qt::LeftButton) || drag_candidate_id.isEmpty())
		{
			QWidget::mouseMoveEvent(event);
			return;
		}

		if ((event->pos() - drag_start_pos).manhattanLength() < QApplication::startDragDistance())
			return;

		auto const id = drag_candidate_id;
		drag_candidate_id.clear();

		auto* drag = new QDrag(this);
		auto* mime_data = new QMimeData();
		mime_data->setData(mobile_toolbar_action_mime_type, id.toUtf8());
		drag->setMimeData(mime_data);

		auto const span = MapEditorController::mobileToolbarActionSpan(id);
		auto const pixmap_size = QSize(qMax(1, span.width()) * (cell_size.width() - 2 * icon_margin_px),
		                               qMax(1, span.height()) * (cell_size.height() - 2 * icon_margin_px));
		auto const pixmap = MapEditorController::mobileToolbarActionIcon(id).pixmap(pixmap_size);
		if (!pixmap.isNull())
		{
			drag->setPixmap(pixmap);
			drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
		}

		drag->exec(Qt::MoveAction);
	}

	void mouseReleaseEvent(QMouseEvent* event) override
	{
		drag_candidate_id.clear();
		QWidget::mouseReleaseEvent(event);
	}

	void dragEnterEvent(QDragEnterEvent* event) override
	{
		if (event->mimeData()->hasFormat(mobile_toolbar_action_mime_type))
			event->acceptProposedAction();
		else
			event->ignore();
	}

	void dragMoveEvent(QDragMoveEvent* event) override
	{
		auto* source = dynamic_cast<MobileToolbarActionCanvas*>(event->source());
		if (!source || !event->mimeData()->hasFormat(mobile_toolbar_action_mime_type))
		{
			event->ignore();
			return;
		}

		auto const id = QString::fromUtf8(event->mimeData()->data(mobile_toolbar_action_mime_type));
		if (id.isEmpty() || (isAvailableList() && !MapEditorController::mobileToolbarActionRemovable(id)))
		{
			event->ignore();
			return;
		}

		if (insertionIndexForDrop(id, event->pos(), source) < 0)
		{
			event->ignore();
			return;
		}

		event->acceptProposedAction();
	}

	void dropEvent(QDropEvent* event) override
	{
		auto* source = dynamic_cast<MobileToolbarActionCanvas*>(event->source());
		if (!source || !event->mimeData()->hasFormat(mobile_toolbar_action_mime_type))
		{
			event->ignore();
			return;
		}

		auto const id = QString::fromUtf8(event->mimeData()->data(mobile_toolbar_action_mime_type));
		if (id.isEmpty() || (isAvailableList() && !MapEditorController::mobileToolbarActionRemovable(id)))
		{
			event->ignore();
			return;
		}

		auto const target_index = insertionIndexForDrop(id, event->pos(), source);
		if (target_index < 0)
		{
			event->ignore();
			return;
		}

		if (source == this)
		{
			auto reordered_actions = action_ids;
			auto const source_index = reordered_actions.indexOf(id);
			if (source_index < 0)
			{
				event->ignore();
				return;
			}
			reordered_actions.removeAt(source_index);
			reordered_actions.insert(target_index, id);
			action_ids = reordered_actions;
			refreshLayout();
			setSelectedAction(id);
			if (actions_changed)
				actions_changed(this);
			event->setDropAction(Qt::MoveAction);
			event->accept();
			return;
		}

		if (action_ids.contains(id) || !source->removeAction(id))
		{
			event->ignore();
			return;
		}

		action_ids.insert(target_index, id);
		refreshLayout();
		setSelectedAction(id);
		if (actions_changed)
			actions_changed(this);
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

private:
	bool isAvailableList() const
	{
		return role == AvailableActions;
	}

	QString actionIdAt(const QPoint& pos) const
	{
		for (auto it = layout.items.crbegin(); it != layout.items.crend(); ++it)
		{
			if (it->rect.contains(pos))
				return it->id;
		}
		return {};
	}

	bool removeAction(const QString& id)
	{
		auto const index = action_ids.indexOf(id);
		if (index < 0)
			return false;

		action_ids.removeAt(index);
		if (selected_id == id)
			selected_id.clear();
		drag_candidate_id.clear();
		refreshLayout();
		if (selection_changed)
			selection_changed(this);
		if (actions_changed)
			actions_changed(this);
		return true;
	}

	int insertionIndexForDrop(const QString& id, const QPoint& pos, const MobileToolbarActionCanvas* source) const
	{
		if (source != this && action_ids.contains(id))
			return -1;

		auto base_actions = action_ids;
		if (source == this)
		{
			auto const source_index = base_actions.indexOf(id);
			if (source_index < 0)
				return -1;
			base_actions.removeAt(source_index);
		}

		auto best_index = 0;
		auto best_score = std::numeric_limits<qint64>::max();
		for (int index = 0; index <= base_actions.size(); ++index)
		{
			auto candidate_actions = base_actions;
			candidate_actions.insert(index, id);
			auto const candidate_layout = previewLayout(candidate_actions);
			for (const auto& item : candidate_layout.items)
			{
				if (item.id != id)
					continue;

				auto dx = 0;
				if (pos.x() < item.rect.left())
					dx = item.rect.left() - pos.x();
				else if (pos.x() > item.rect.right())
					dx = pos.x() - item.rect.right();

				auto dy = 0;
				if (pos.y() < item.rect.top())
					dy = item.rect.top() - pos.y();
				else if (pos.y() > item.rect.bottom())
					dy = pos.y() - item.rect.bottom();

				auto const score = qint64(dx) * dx + qint64(dy) * dy;
				if (score < best_score)
				{
					best_score = score;
					best_index = index;
				}
				break;
			}
		}

		return best_index;
	}

	MobileToolbarPreviewLayout previewLayout(const QStringList& ids) const
	{
		return layoutMobileToolbarPreview(
		        ids,
		        role == ToolbarZone && mobileToolbarZoneIsTrailing(toolbar_zone),
		        cell_size
		);
	}

	void refreshLayout()
	{
		layout = previewLayout(action_ids);
		setFixedSize(qMax(1, layout.column_count) * cell_size.width(), content_height);
		updateGeometry();
		update();
	}

	LayoutRole role;
	MapEditorController::MobileToolbarZone toolbar_zone = MapEditorController::MobileToolbarTopLeft;
	QStringList action_ids;
	QString selected_id;
	QString drag_candidate_id;
	QPoint drag_start_pos;
	QSize cell_size;
	int icon_margin_px = 0;
	int content_height = 0;
	MobileToolbarPreviewLayout layout;
};

class MobileToolbarConfigDialog final : public QDialog
{
public:
	explicit MobileToolbarConfigDialog(QWidget* parent = nullptr)
	: QDialog(parent)
	, available_canvas(new MobileToolbarActionCanvas(MobileToolbarActionCanvas::AvailableActions, this))
	, current_zone_canvas(new MobileToolbarActionCanvas(MobileToolbarActionCanvas::ToolbarZone, this))
	, current_zone_view(new QScrollArea(this))
	, available_view(new QScrollArea(this))
	, current_zone_scrollbar(new QScrollBar(Qt::Horizontal, this))
	, available_scrollbar(new QScrollBar(Qt::Horizontal, this))
	, zone_selector(new QComboBox(this))
	, selected_action_label(new QLabel(this))
	, remove_button(new QPushButton(this))
	{
		setWindowTitle(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Mobile toolbar icons"));
		if (parentWidget())
		{
			auto const parent_size = parentWidget()->window()->size();
			if (parent_size.isValid())
				resize(qMax(320, parent_size.width() * 95 / 100),
				       qMax(520, parent_size.height() * 90 / 100));
		}

		auto* layout = new QVBoxLayout(this);

		auto* note = new QLabel(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage",
			"Edit one toolbar area at a time. Toolbar previews use the same two-row arrangement as the map editor. If not all icons fit, use the horizontal scroll bars. To move an icon to another area, drag it to Available actions, switch area, and drag it back."));
		note->setWordWrap(true);
		layout->addWidget(note);

		selected_action_label->setWordWrap(true);
		layout->addWidget(selected_action_label);

		for (auto* view : { current_zone_view, available_view })
		{
			view->setWidgetResizable(false);
			view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		}
		current_zone_view->setWidget(current_zone_canvas);
		available_view->setWidget(available_canvas);
		current_zone_view->setFixedHeight(current_zone_canvas->contentHeight() + 2 * current_zone_view->frameWidth());
		available_view->setFixedHeight(available_canvas->contentHeight() + 2 * available_view->frameWidth());

		layout->addWidget(Util::Headline::create(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Editing area:")));
		zone_selector->addItem(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarTopLeft),
		                       QVariant::fromValue(int(MapEditorController::MobileToolbarTopLeft)));
		zone_selector->addItem(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarTopRight),
		                       QVariant::fromValue(int(MapEditorController::MobileToolbarTopRight)));
		zone_selector->addItem(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarBottomLeft),
		                       QVariant::fromValue(int(MapEditorController::MobileToolbarBottomLeft)));
		zone_selector->addItem(MapEditorController::mobileToolbarZoneLabel(MapEditorController::MobileToolbarBottomRight),
		                       QVariant::fromValue(int(MapEditorController::MobileToolbarBottomRight)));
		displayed_zone = currentZone();
		layout->addWidget(zone_selector);
		layout->addWidget(current_zone_view);
		layout->addWidget(current_zone_scrollbar);

		layout->addWidget(Util::Headline::create(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Available actions:")));
		layout->addWidget(available_view);
		layout->addWidget(available_scrollbar);

		auto* controls_layout = new QHBoxLayout();
		remove_button->setText(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Remove selected"));
		auto* reset_button = new QPushButton(QCoreApplication::translate("OpenOrienteering::EditorSettingsPage", "Reset"), this);
		controls_layout->addWidget(remove_button);
		controls_layout->addStretch(1);
		controls_layout->addWidget(reset_button);
		layout->addLayout(controls_layout);

		auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		layout->addWidget(button_box);

		connectScrollBars(current_zone_view, current_zone_scrollbar);
		connectScrollBars(available_view, available_scrollbar);

		for (auto* canvas : { available_canvas, current_zone_canvas })
		{
			canvas->selection_changed = [this](MobileToolbarActionCanvas* active_canvas) { activateCanvas(active_canvas); };
			canvas->actions_changed = [this](MobileToolbarActionCanvas* changed_canvas) {
				if (changed_canvas == current_zone_canvas)
					saveDisplayedZoneActions();
				syncExternalScrollBar(current_zone_view, current_zone_scrollbar);
				syncExternalScrollBar(available_view, available_scrollbar);
				updateButtons();
			};
		}
		connect(zone_selector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
			saveDisplayedZoneActions();
			displayed_zone = currentZone();
			selection_update_in_progress = true;
			available_canvas->clearSelectedAction();
			current_zone_canvas->clearSelectedAction();
			selection_update_in_progress = false;
			refillLists(QString{});
		});
		connect(remove_button, &QPushButton::clicked, this, &MobileToolbarConfigDialog::removeSelectedAction);
		connect(reset_button, &QPushButton::clicked, this, [this]() {
			top_left_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarTopLeft);
			top_right_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarTopRight);
			bottom_left_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarBottomLeft);
			bottom_right_actions = MapEditorController::defaultMobileToolbarActionIds(MapEditorController::MobileToolbarBottomRight);
			refillLists(QString{});
		});
		connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
		updateButtons();
	}

	void setSelectedActions(
	        const QStringList& new_top_left_actions,
	        const QStringList& new_top_right_actions,
	        const QStringList& new_bottom_left_actions,
	        const QStringList& new_bottom_right_actions
	)
	{
		top_left_actions = new_top_left_actions;
		top_right_actions = new_top_right_actions;
		bottom_left_actions = new_bottom_left_actions;
		bottom_right_actions = new_bottom_right_actions;
		MapEditorController::sanitizeMobileToolbarConfiguration(top_left_actions, top_right_actions, bottom_left_actions, bottom_right_actions);
		refillLists(QString{});
	}

	QStringList selectedActions(MapEditorController::MobileToolbarZone zone) const
	{
		if (zone == displayedZone())
			return current_zone_canvas->actionIds();
		return actionsForZone(zone);
	}

private:
	QStringList& actionsForZone(MapEditorController::MobileToolbarZone zone)
	{
		switch (zone)
		{
		case MapEditorController::MobileToolbarTopLeft:
			return top_left_actions;
		case MapEditorController::MobileToolbarTopRight:
			return top_right_actions;
		case MapEditorController::MobileToolbarBottomLeft:
			return bottom_left_actions;
		case MapEditorController::MobileToolbarBottomRight:
			return bottom_right_actions;
		}
		Q_UNREACHABLE();
		return top_left_actions;
	}

	const QStringList& actionsForZone(MapEditorController::MobileToolbarZone zone) const
	{
		switch (zone)
		{
		case MapEditorController::MobileToolbarTopLeft:
			return top_left_actions;
		case MapEditorController::MobileToolbarTopRight:
			return top_right_actions;
		case MapEditorController::MobileToolbarBottomLeft:
			return bottom_left_actions;
		case MapEditorController::MobileToolbarBottomRight:
			return bottom_right_actions;
		}
		Q_UNREACHABLE();
		return top_left_actions;
	}

	MapEditorController::MobileToolbarZone currentZone() const
	{
		return static_cast<MapEditorController::MobileToolbarZone>(zone_selector->currentData().toInt());
	}

	MapEditorController::MobileToolbarZone displayedZone() const
	{
		return displayed_zone;
	}

	static void syncExternalScrollBar(QAbstractScrollArea* area, QScrollBar* external_scrollbar)
	{
		auto* internal_scrollbar = area->horizontalScrollBar();
		QSignalBlocker block_external(external_scrollbar);
		external_scrollbar->setRange(internal_scrollbar->minimum(), internal_scrollbar->maximum());
		external_scrollbar->setPageStep(internal_scrollbar->pageStep());
		external_scrollbar->setSingleStep(internal_scrollbar->singleStep());
		external_scrollbar->setValue(internal_scrollbar->value());
		external_scrollbar->setVisible(internal_scrollbar->maximum() > internal_scrollbar->minimum());
	}

	static void connectScrollBars(QAbstractScrollArea* area, QScrollBar* external_scrollbar)
	{
		auto* internal_scrollbar = area->horizontalScrollBar();
		external_scrollbar->setVisible(false);
		QObject::connect(internal_scrollbar, &QScrollBar::rangeChanged, area, [area, external_scrollbar](int, int) {
			syncExternalScrollBar(area, external_scrollbar);
		});
		QObject::connect(internal_scrollbar, &QScrollBar::valueChanged, area, [area, external_scrollbar](int) {
			syncExternalScrollBar(area, external_scrollbar);
		});
		QObject::connect(external_scrollbar, &QScrollBar::valueChanged, area, [internal_scrollbar](int value) {
			if (internal_scrollbar->value() != value)
				internal_scrollbar->setValue(value);
		});
		syncExternalScrollBar(area, external_scrollbar);
	}

	void saveDisplayedZoneActions()
	{
		actionsForZone(displayedZone()) = current_zone_canvas->actionIds();
	}

	void activateCanvas(MobileToolbarActionCanvas* active_canvas)
	{
		if (selection_update_in_progress)
			return;
		if (!active_canvas || !active_canvas->hasSelection())
		{
			updateButtons();
			return;
		}

		selection_update_in_progress = true;
		auto* other_canvas = (active_canvas == available_canvas) ? current_zone_canvas : available_canvas;
		other_canvas->clearSelectedAction();
		selection_update_in_progress = false;
		updateButtons();
	}

	void refillLists(const QString& selected_id)
	{
		current_zone_canvas->setToolbarZone(displayedZone());

		auto const& current_actions = actionsForZone(displayedZone());
		current_zone_canvas->setActionIds(current_actions);

		auto const configured_actions = top_left_actions + top_right_actions + bottom_left_actions + bottom_right_actions;
		auto available_actions = QStringList{};
		for (const auto& id : MapEditorController::editableMobileToolbarActionIds())
		{
			if (!configured_actions.contains(id))
				available_actions << id;
		}
		available_canvas->setActionIds(available_actions);

		selection_update_in_progress = true;
		current_zone_canvas->setSelectedAction(selected_id);
		available_canvas->setSelectedAction(selected_id);
		if (selected_id.isEmpty())
		{
			available_canvas->clearSelectedAction();
			current_zone_canvas->clearSelectedAction();
		}
		selection_update_in_progress = false;
		current_zone_view->horizontalScrollBar()->setValue(0);
		available_view->horizontalScrollBar()->setValue(0);
		syncExternalScrollBar(current_zone_view, current_zone_scrollbar);
		syncExternalScrollBar(available_view, available_scrollbar);
		updateButtons();
	}

	bool selectedAction(QString& id, MapEditorController::MobileToolbarZone* zone = nullptr) const
	{
		if (current_zone_canvas->hasSelection())
		{
			id = current_zone_canvas->selectedActionId();
			if (zone)
				*zone = displayedZone();
			return true;
		}
		if (available_canvas->hasSelection())
		{
			id = available_canvas->selectedActionId();
			return true;
		}
		return false;
	}

	void removeSelectedAction()
	{
		QString id;
		MapEditorController::MobileToolbarZone source_zone = MapEditorController::MobileToolbarTopLeft;
		if (!selectedAction(id, &source_zone) || available_canvas->hasSelection())
			return;
		if (!MapEditorController::mobileToolbarActionRemovable(id))
			return;

		auto current_actions = current_zone_canvas->actionIds();
		current_actions.removeAll(id);
		current_zone_canvas->setActionIds(current_actions);
		saveDisplayedZoneActions();

		auto available_actions = available_canvas->actionIds();
		available_actions << id;
		available_canvas->setActionIds(available_actions);
		available_canvas->setSelectedAction(id);
		activateCanvas(available_canvas);
		syncExternalScrollBar(current_zone_view, current_zone_scrollbar);
		syncExternalScrollBar(available_view, available_scrollbar);
	}

	void updateButtons()
	{
		QString id;
		MapEditorController::MobileToolbarZone source_zone = MapEditorController::MobileToolbarTopLeft;
		auto const has_selection = selectedAction(id, &source_zone);
		auto const selected_in_available = available_canvas->hasSelection();
		auto const selected_in_zone = has_selection && !selected_in_available;

		remove_button->setEnabled(selected_in_zone && MapEditorController::mobileToolbarActionRemovable(id));
		selected_action_label->setText(mobileToolbarSelectionSummary(has_selection ? id : QString{}));
	}

	MobileToolbarActionCanvas* available_canvas;
	MobileToolbarActionCanvas* current_zone_canvas;
	QScrollArea* current_zone_view;
	QScrollArea* available_view;
	QScrollBar* current_zone_scrollbar;
	QScrollBar* available_scrollbar;
	QComboBox* zone_selector;
	QLabel* selected_action_label;
	QPushButton* remove_button;
	QStringList top_left_actions;
	QStringList top_right_actions;
	QStringList bottom_left_actions;
	QStringList bottom_right_actions;
	MapEditorController::MobileToolbarZone displayed_zone = MapEditorController::MobileToolbarTopLeft;
	bool selection_update_in_progress = false;
};

}  // namespace

EditorSettingsPage::EditorSettingsPage(QWidget* parent)
 : SettingsPage(parent)
{
	auto layout = new QFormLayout(this);
	
	if (Settings::getInstance().touchModeEnabled())
	{
		button_size = Util::SpinBox::create(1, 3.0, 26.0, tr("mm", "millimeters"), 0.1);
		layout->addRow(tr("Action button size:"), button_size);

		mobile_toolbar_button = new QPushButton(tr("Configure mobile toolbars..."), this);
		layout->addRow(tr("Toolbar icons:"), mobile_toolbar_button);
	}

	icon_size = Util::SpinBox::create(1, 25, tr("mm", "millimeters"));
	layout->addRow(tr("Symbol icon size:"), icon_size);
	
	antialiasing = new QCheckBox(tr("High quality map display (antialiasing)"), this);
	antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addRow(antialiasing);
	
	text_antialiasing = new QCheckBox(tr("High quality text display in map (antialiasing), slow"), this);
	text_antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addRow(text_antialiasing);
	
	tolerance = Util::SpinBox::create(0, 50, tr("mm", "millimeters"));
	layout->addRow(tr("Click tolerance:"), tolerance);
	
	snap_distance = Util::SpinBox::create(0, 100, tr("mm", "millimeters"));
	layout->addRow(tr("Snap distance (%1):").arg(ModifierKey::shift()), snap_distance);
	
	fixed_angle_stepping = Util::SpinBox::create<Util::RotationalDegrees>();
	fixed_angle_stepping->setDecimals(1);
	fixed_angle_stepping->setRange(0.1, 180.0);
	layout->addRow(tr("Stepping of fixed angle mode (%1):").arg(ModifierKey::control()), fixed_angle_stepping);

	select_symbol_of_objects = new QCheckBox(tr("When selecting an object, automatically select its symbol, too"));
	layout->addRow(select_symbol_of_objects);
	
	zoom_out_away_from_cursor = new QCheckBox(tr("Zoom away from cursor when zooming out"));
	layout->addRow(zoom_out_away_from_cursor);
	
	draw_last_point_on_right_click = new QCheckBox(tr("Drawing tools: set last point on finishing with right click"));
	layout->addRow(draw_last_point_on_right_click);
	
	keep_settings_of_closed_templates = new QCheckBox(tr("Templates: keep settings of closed templates"));
	layout->addRow(keep_settings_of_closed_templates);
	
	ignore_touch_input = new QCheckBox(tr("User input: Ignore display touch"));
	layout->addRow(ignore_touch_input);

	gesture_extra_rendering = new QComboBox();
	gesture_extra_rendering->addItem(tr("Off (gray)"));
	gesture_extra_rendering->addItem(tr("Templates only"));
	gesture_extra_rendering->addItem(tr("Full (templates + map)"));
	layout->addRow(tr("Gesture uncovered area:"), gesture_extra_rendering);
	
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Edit tool:")));
	
	edit_tool_delete_bezier_point_action = new QComboBox();
	edit_tool_delete_bezier_point_action->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addRow(tr("Action on deleting a curve point with %1:").arg(ModifierKey::control()), edit_tool_delete_bezier_point_action);
	
	edit_tool_delete_bezier_point_action_alternative = new QComboBox();
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addRow(tr("Action on deleting a curve point with %1:").arg(ModifierKey::controlShift()), edit_tool_delete_bezier_point_action_alternative);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Rectangle tool:")));
	
	rectangle_helper_cross_radius = Util::SpinBox::create(0, 999999, tr("mm", "millimeters"));
	layout->addRow(tr("Radius of helper cross:"), rectangle_helper_cross_radius);
	
	rectangle_preview_line_width = new QCheckBox(tr("Preview the width of lines with helper cross"));
	layout->addRow(rectangle_preview_line_width);
	
	
	connect(antialiasing, &QAbstractButton::toggled, text_antialiasing, &QCheckBox::setEnabled);
	if (mobile_toolbar_button)
		connect(mobile_toolbar_button, &QPushButton::clicked, this, &EditorSettingsPage::configureMobileToolbars);
	
	updateWidgets();
}

EditorSettingsPage::~EditorSettingsPage()
{
	// nothing, not inlined
}

QString EditorSettingsPage::title() const
{
	return tr("Editor");
}

void EditorSettingsPage::apply()
{
	if (button_size != nullptr)
		setSetting(Settings::ActionGridBar_ButtonSizeMM, button_size->value());
	setSetting(Settings::SymbolWidget_IconSizeMM, icon_size->value());
	setSetting(Settings::MapDisplay_Antialiasing, antialiasing->isChecked());
	setSetting(Settings::MapDisplay_TextAntialiasing, text_antialiasing->isChecked());
	setSetting(Settings::MapEditor_ClickToleranceMM, tolerance->value());
	setSetting(Settings::MapEditor_SnapDistanceMM, snap_distance->value());
	setSetting(Settings::MapEditor_FixedAngleStepping, fixed_angle_stepping->value());
	setSetting(Settings::MapEditor_ChangeSymbolWhenSelecting, select_symbol_of_objects->isChecked());
	setSetting(Settings::MapEditor_ZoomOutAwayFromCursor, zoom_out_away_from_cursor->isChecked());
	setSetting(Settings::MapEditor_DrawLastPointOnRightClick, draw_last_point_on_right_click->isChecked());
	setSetting(Settings::Templates_KeepSettingsOfClosed, keep_settings_of_closed_templates->isChecked());
	setSetting(Settings::MapEditor_IgnoreTouchInput, ignore_touch_input->isChecked());
	setSetting(Settings::MapDisplay_GestureExtraRendering, gesture_extra_rendering->currentIndex());
	setSetting(Settings::MobileToolbar_TopLeftActions, mobile_top_left_toolbar_actions);
	setSetting(Settings::MobileToolbar_TopRightActions, mobile_top_right_toolbar_actions);
	setSetting(Settings::MobileToolbar_BottomLeftActions, mobile_bottom_left_toolbar_actions);
	setSetting(Settings::MobileToolbar_BottomRightActions, mobile_bottom_right_toolbar_actions);
	setSetting(Settings::EditTool_DeleteBezierPointAction, edit_tool_delete_bezier_point_action->currentData());
	setSetting(Settings::EditTool_DeleteBezierPointActionAlternative, edit_tool_delete_bezier_point_action_alternative->currentData());
	setSetting(Settings::RectangleTool_HelperCrossRadiusMM, rectangle_helper_cross_radius->value());
	setSetting(Settings::RectangleTool_PreviewLineWidth, rectangle_preview_line_width->isChecked());
}

void EditorSettingsPage::reset()
{
	updateWidgets();
}

void EditorSettingsPage::updateWidgets()
{
	if (button_size != nullptr)
		button_size->setValue(getSetting(Settings::ActionGridBar_ButtonSizeMM).toDouble());
	icon_size->setValue(getSetting(Settings::SymbolWidget_IconSizeMM).toInt());
	antialiasing->setChecked(getSetting(Settings::MapDisplay_Antialiasing).toBool());
	text_antialiasing->setEnabled(antialiasing->isChecked());
	text_antialiasing->setChecked(getSetting(Settings::MapDisplay_TextAntialiasing).toBool());
	tolerance->setValue(getSetting(Settings::MapEditor_ClickToleranceMM).toInt());
	snap_distance->setValue(getSetting(Settings::MapEditor_SnapDistanceMM).toInt());
	fixed_angle_stepping->setValue(getSetting(Settings::MapEditor_FixedAngleStepping).toInt());
	select_symbol_of_objects->setChecked(getSetting(Settings::MapEditor_ChangeSymbolWhenSelecting).toBool());
	zoom_out_away_from_cursor->setChecked(getSetting(Settings::MapEditor_ZoomOutAwayFromCursor).toBool());
	draw_last_point_on_right_click->setChecked(getSetting(Settings::MapEditor_DrawLastPointOnRightClick).toBool());
	keep_settings_of_closed_templates->setChecked(getSetting(Settings::Templates_KeepSettingsOfClosed).toBool());
	ignore_touch_input->setChecked(getSetting(Settings::MapEditor_IgnoreTouchInput).toBool());
	gesture_extra_rendering->setCurrentIndex(getSetting(Settings::MapDisplay_GestureExtraRendering).toInt());
	mobile_top_left_toolbar_actions = getSetting(Settings::MobileToolbar_TopLeftActions).toStringList();
	mobile_top_right_toolbar_actions = getSetting(Settings::MobileToolbar_TopRightActions).toStringList();
	mobile_bottom_left_toolbar_actions = getSetting(Settings::MobileToolbar_BottomLeftActions).toStringList();
	mobile_bottom_right_toolbar_actions = getSetting(Settings::MobileToolbar_BottomRightActions).toStringList();
	MapEditorController::sanitizeMobileToolbarConfiguration(
	        mobile_top_left_toolbar_actions,
	        mobile_top_right_toolbar_actions,
	        mobile_bottom_left_toolbar_actions,
	        mobile_bottom_right_toolbar_actions
	);
	
	edit_tool_delete_bezier_point_action->setCurrentIndex(edit_tool_delete_bezier_point_action->findData(getSetting(Settings::EditTool_DeleteBezierPointAction).toInt()));
	edit_tool_delete_bezier_point_action_alternative->setCurrentIndex(edit_tool_delete_bezier_point_action_alternative->findData(getSetting(Settings::EditTool_DeleteBezierPointActionAlternative).toInt()));
	
	rectangle_helper_cross_radius->setValue(getSetting(Settings::RectangleTool_HelperCrossRadiusMM).toInt());
	rectangle_preview_line_width->setChecked(getSetting(Settings::RectangleTool_PreviewLineWidth).toBool());
}

void EditorSettingsPage::configureMobileToolbars()
{
	MobileToolbarConfigDialog dialog(window());
	dialog.setSelectedActions(
	        mobile_top_left_toolbar_actions,
	        mobile_top_right_toolbar_actions,
	        mobile_bottom_left_toolbar_actions,
	        mobile_bottom_right_toolbar_actions
	);
	if (dialog.exec() != QDialog::Accepted)
		return;

	mobile_top_left_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarTopLeft);
	mobile_top_right_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarTopRight);
	mobile_bottom_left_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarBottomLeft);
	mobile_bottom_right_toolbar_actions = dialog.selectedActions(MapEditorController::MobileToolbarBottomRight);
	MapEditorController::sanitizeMobileToolbarConfiguration(
	        mobile_top_left_toolbar_actions,
	        mobile_top_right_toolbar_actions,
	        mobile_bottom_left_toolbar_actions,
	        mobile_bottom_right_toolbar_actions
	);
}


}  // namespace OpenOrienteering
