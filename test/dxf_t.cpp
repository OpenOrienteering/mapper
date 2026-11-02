/*
 *    Copyright 2026 Matthias Kühlewein
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
#include <QtMath>
#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QLatin1String>
#include <QObject>
#include <QString>
#include <QTimer>

#include "test_config.h"

#include "global.h"
#include "core/map.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/symbol.h"

#ifdef MAPPER_USE_GDAL
#include "gdal/ogr_template.h"
#endif

class QWidget;

using namespace OpenOrienteering;

/**
 * @test Tests DXF import.
 */
class DxfTest : public QObject
{
Q_OBJECT
private:
	void handleCascadedDialogs()
	{
		QTimer::singleShot(500, this, [this] {
			QWidget* positioning_dialog = QApplication::activeModalWidget();
			if (positioning_dialog)
			{
				QTimer::singleShot(500, this, [] {
					QWidget* symbol_assignment_dialog = QApplication::activeModalWidget();
					if (symbol_assignment_dialog)
					{
						QTest::keyPress(symbol_assignment_dialog, Qt::Key_Enter);	// triggers the Ok button
					}
				});
				QTest::keyPress(positioning_dialog, Qt::Key_Enter);		// triggers the Ok button
			}
		});
	}
	
private slots:
	void initTestCase()
	{
		Q_INIT_RESOURCE(resources);
		doStaticInitializations();
		QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
	}
	
#ifdef MAPPER_USE_GDAL
	void dxfTextImport()
	{
		Map map;
		MapView view{ &map };
		
		const QString filename = QStringLiteral("testdata:import/DXFTextImport.dxf");
		OgrTemplate ogr_template {filename, &map};
		
		handleCascadedDialogs();
		
		QVERIFY(ogr_template.setupAndLoad(nullptr, &view));
		auto template_map = ogr_template.takeTemplateMap();
		QVERIFY(template_map);
		
		map.importMap(*template_map, Map::MinimalObjectImport);
		
		const auto number_symbols = map.getNumSymbols();
		QCOMPARE(number_symbols, 3);
		
		const auto number_objects = map.getNumObjects();
		QCOMPARE(number_objects, 8);
		
		for (auto i = 0; i < number_objects; ++i)
		{
			const auto* object = map.getCurrentPart()->getObject(i);
			QVERIFY(object->getType() == Object::Type::Text);
			
			const auto* text_object = object->asText();
			const auto text = text_object->getText();
			QVERIFY(text.startsWith(QLatin1String("Test")));
			
			const auto rotation = text_object->getRotation();
			if (text == QLatin1String("Test1"))
			{
				QCOMPARE(rotation, qDegreesToRadians(-17.2));
			}
			else if (text == QLatin1String("Test2"))
			{
				QCOMPARE(rotation, qDegreesToRadians(53.2));
			}
			else if (text == QLatin1String("Test3"))
			{
				QCOMPARE(rotation, qDegreesToRadians(0.0));
			}
			else if (text == QLatin1String("Test4"))
			{
				QCOMPARE(rotation, qDegreesToRadians(-348.0));	// instead of -348.4 due to GDAL issue
			}
		}
		
		for (auto i = 0; i < number_symbols; ++i)
		{
			const auto* symbol = map.getSymbol(i);
			QVERIFY(symbol->getDescription().isEmpty());
		}
	}
	
	void dxfTextImportTwice()
	{
		Map map;
		MapView view{ &map };
		
		const QString filename = QStringLiteral("testdata:import/DXFTextImport.dxf");
		OgrTemplate ogr_template {filename, &map};
		
		handleCascadedDialogs();
		
		QVERIFY(ogr_template.setupAndLoad(nullptr, &view));
		auto template_map = ogr_template.takeTemplateMap();
		QVERIFY(template_map);
		
		map.importMap(*template_map, Map::MinimalObjectImport);
		map.importMap(*template_map, Map::MinimalObjectImport);
		
		QCOMPARE(map.getNumSymbols(), 3);
		QCOMPARE(map.getNumObjects(), 16);
	}
#endif
};  // class DxfTest

/*
 * We don't need a real GUI window.
 */
namespace {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}

QTEST_MAIN(DxfTest)
#include "dxf_t.moc"  // IWYU pragma: keep
