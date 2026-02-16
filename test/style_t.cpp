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


#include <Qt>
#include <QtGlobal>
#include <QtPlugin>
#include <QtTest>
#include <QCommonStyle>
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QSize>
#include <QStaticPlugin>
#include <QString>
#include <QStyle>

#include "core/map_coord.h"
#include "gui/scaling_icon_engine.h"
#include "gui/widgets/mapper_proxystyle.h"

using namespace OpenOrienteering;



/**
 * @test Tests style customizations.
 */
class StyleTest : public QObject
{
Q_OBJECT
private slots:
	void scalingIconEngineTest();
	void standardIconTest();
};


/**
 * Tests key behaviours of ScalingIconEngine.
 */
void StyleTest::scalingIconEngineTest()
{
	QPixmap pm(4, 4);
	pm.fill(Qt::red);
	{
		// Testing Qt's default behaviour:
		QIcon icon(pm);
		// Scaling down
		QCOMPARE(icon.actualSize(QSize(1,1)), QSize(1,1));
		QCOMPARE(icon.pixmap(QSize(1,1)).size(), QSize(1,1));
		// Not scaling up
		QCOMPARE(icon.actualSize(QSize(10,10)), QSize(4,4));
		QCOMPARE(icon.pixmap(QSize(10,10)).size(), QSize(4,4));
	}
	{
		// ScalingIconEngine:
		QIcon icon(new ScalingIconEngine(QString{}));
		icon.addPixmap(pm);
		// Scaling down
		QCOMPARE(icon.actualSize(QSize(1,1)), QSize(1,1));
		QCOMPARE(icon.pixmap(QSize(1,1)).size(), QSize(1,1));
		// Scaling up
		QCOMPARE(icon.actualSize(QSize(10,10)), QSize(10,10));
		QCOMPARE(icon.pixmap(QSize(10,10)).size(), QSize(10,10));
	}
}

/**
 * Tests standard icon behaviours of MapperProxyStyle
 */
void StyleTest::standardIconTest()
{
	// CommonStyle doesn't use PNG for QStyle::SP_TitleBarMenuButton,
	// so it is not passed through ScalingIconEngine.
	auto const standard_icon = QStyle::SP_TitleBarMenuButton;
	auto const large = QSize(1000, 1000);
	{
		// Testing Qt's default behaviour:
		auto const size = QCommonStyle().standardIcon(standard_icon).actualSize(large);
		QVERIFY(size.width() < large.width());
		QVERIFY(size.height() < large.height());
	}
	{
		// ScalingIconEngine:
		// MapperProxyStyle must ensure use of the scaling icon engine
		QCOMPARE(MapperProxyStyle().standardIcon(standard_icon, nullptr, nullptr).actualSize(large), large);
	}
}


/*
 * We select a non-standard QPA because we don't need a real GUI window.
 */
namespace  {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}


Q_IMPORT_PLUGIN(ScalingIconEnginePlugin)

QTEST_MAIN(StyleTest)

#include "style_t.moc"  // IWYU pragma: keep
