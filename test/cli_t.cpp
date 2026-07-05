/*
 *    Copyright 2026 Michael Behrens
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


#include "cli_t.h"

#include <cstring>
#include <vector>

#include <QtGlobal>
#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>

#include "cli.h"
#include "global.h"
#include "test_config.h"

using namespace OpenOrienteering;


namespace {

/**
 * Builds a mutable argv array from string literals and calls execCli.
 * 
 * The caller must ensure the arguments remain valid for the duration of
 * the call (they do when using string literals).
 */
int execCliFrom(std::initializer_list<const char*> args)
{
	std::vector<char*> argv;
	argv.reserve(args.size());
	for (auto a : args)
		argv.push_back(const_cast<char*>(a));
	return execCli(int(argv.size()), argv.data());
}

}  // namespace


void CliTest::initTestCase()
{
	doStaticInitializations();

	// Register a Qt search path for test data
	QDir::addSearchPath(QStringLiteral("testdata"),
	    QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR))
	        .absoluteFilePath(QStringLiteral("data")));
}


void CliTest::testNoSubcommand()
{
	QCOMPARE(execCliFrom({"mapper", "--cli"}), 1);
}


void CliTest::testNonexistingSubcommand()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "nonexistingsubcommand"}), 1);
}


void CliTest::testExportMissingInput()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "export", "-o", "out.pdf"}), 1);
}


void CliTest::testExportMissingOutput()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "export", "-i", "in.omap"}), 1);
}


void CliTest::testExportInvalidInput()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "export", "-i", "nonexistent.omap", "-o", "out.pdf"}), 1);
}


void CliTest::testExportPdf()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.pdf");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "export",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--full-map"
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testExportPng()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.png");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "export",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--full-map"
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testExportFullMapFlag()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QString output = temp_dir.path() + QStringLiteral("/output.pdf");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "export",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--full-map"
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testConvertMissingInput()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "convert", "-o", "out.omap"}), 1);
}


void CliTest::testConvertMissingOutput()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "convert", "-i", "in.omap"}), 1);
}


void CliTest::testConvertInvalidInput()
{
	QCOMPARE(execCliFrom({"mapper", "--cli", "convert", "-i", "nonexistent.omap", "-o", "out.omap"}), 1);
}


void CliTest::testConvertOmapToXmap()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.xmap");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData()
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testConvertXmapToOmap()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString xmap = temp_dir.path() + QStringLiteral("/intermediate.xmap");
	QString omap = temp_dir.path() + QStringLiteral("/output.omap");
	auto input_bytes = input.toLocal8Bit();
	auto xmap_bytes = xmap.toLocal8Bit();
	auto omap_bytes = omap.toLocal8Bit();

	int r1 = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", input_bytes.constData(),
	    "-o", xmap_bytes.constData()
	});
	QCOMPARE(r1, 0);
	QVERIFY(QFile::exists(xmap));

	int r2 = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", xmap_bytes.constData(),
	    "-o", omap_bytes.constData()
	});
	QCOMPARE(r2, 0);
	QVERIFY(QFile::exists(omap));
	QVERIFY(QFileInfo(omap).size() > 0);
}


void CliTest::testConvertOmapToOcd()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.ocd");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData()
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testConvertWithFormatId()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.xmap");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--output-format", "XML"
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testExportFormatIdOverridesExtension()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	// Write a PNG to a file named .pdf
	QString output = temp_dir.path() + QStringLiteral("/output.pdf");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "export",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--output-format", "png",
	    "--full-map"
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testConvertFormatIdOverridesExtension()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	// Write XML format to a non-native file extension
	QString output = temp_dir.path() + QStringLiteral("/output.foo");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--output-format", "XML"
	});
	QCOMPARE(result, 0);
	QVERIFY(QFile::exists(output));
	QVERIFY(QFileInfo(output).size() > 0);
}


void CliTest::testExportRejectsNativeFormatId()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.foo");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "export",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--output-format", "XML"
	});
	QCOMPARE(result, 1);
}


void CliTest::testConvertRejectsUnknownFormatId()
{
	QTemporaryDir temp_dir;
	QVERIFY(temp_dir.isValid());

	auto input = QString::fromUtf8("testdata:/barrier.omap");
	QVERIFY(QFileInfo::exists(input));

	QString output = temp_dir.path() + QStringLiteral("/output.omap");
	auto input_bytes = input.toLocal8Bit();
	auto output_bytes = output.toLocal8Bit();

	int result = execCliFrom({
	    "mapper", "--cli", "convert",
	    "-i", input_bytes.constData(),
	    "-o", output_bytes.constData(),
	    "--output-format", "nonexistingformat"
	});
	QCOMPARE(result, 1);
}


QTEST_MAIN(CliTest)
