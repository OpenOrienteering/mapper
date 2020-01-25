/*
 * Copyright (c) 2020 Kai Pastor
 *
 * This file is part of CoVe.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Qt>
#include <QtTest>
#include <QColor>
#include <QImage>
#include <QObject>
#include <QString>

#include "libvectorizer/ParallelImageProcessing.h"
// IWYU pragma: no_include "libvectorizer/ProgressObserver.h"

using namespace cove;

class ParallelImageProcessingTest : public QObject
{
	Q_OBJECT
	
private slots:
	void inplaceImageTest()
	{
		auto original = QImage(64,64, QImage::Format_RGB32);
		
		// QImage: detaching unless explicitly constructed from the bits.
		{
			original.fill(Qt::white);
			QCOMPARE(original.pixel(32,32), QColor(Qt::white).rgb());
			
			auto qimage_clone = QImage(original.bits(), original.width(), original.height(), original.bytesPerLine(), original.format());
			qimage_clone.fill(Qt::red);
			QCOMPARE(original.pixel(32,32), QColor(Qt::red).rgb());
			
			auto qimage_clone_clone = QImage(qimage_clone.bits(), qimage_clone.width(), qimage_clone.height(), qimage_clone.bytesPerLine(), qimage_clone.format());
			qimage_clone_clone.fill(Qt::black);
			QCOMPARE(original.pixel(32,32), QColor(Qt::black).rgb());
			
			// A QImage which is copy-constructed from a bits-cloned QImage detaches when modified.
			qimage_clone_clone = QImage(qimage_clone);
			QVERIFY(qimage_clone_clone.bits() != original.bits());     // already detached!
			qimage_clone_clone.fill(Qt::white);
			QVERIFY(qimage_clone_clone.bits() != original.bits());     // detached!
			QCOMPARE(original.pixel(32,32), QColor(Qt::black).rgb());  // unchanged!
		}
		
		// InplaceImage: not detaching, even if copy-constructed from const ref.
		{
			original.fill(Qt::white);
			QCOMPARE(original.pixel(32,32), QColor(Qt::white).rgb());
			
			auto inplace_clone = InplaceImage(original, original.bits(), original.width(), original.height(), original.bytesPerLine(), original.format());
			inplace_clone.fill(Qt::red);
			QCOMPARE(original.pixel(32,32), QColor(Qt::red).rgb());
			
			auto inplace_clone_clone = InplaceImage(inplace_clone, inplace_clone.bits(), inplace_clone.width(), inplace_clone.height(), inplace_clone.bytesPerLine(), inplace_clone.format());
			inplace_clone_clone.fill(Qt::black);
			QCOMPARE(original.pixel(32,32), QColor(Qt::black).rgb());
			
			// An InplaceImage which is copy-assigned from a const InplaceImage does not detach when modified.
			inplace_clone_clone = static_cast<InplaceImage const>(inplace_clone);
			QVERIFY(inplace_clone_clone.bits() == original.bits());    // not detached
			inplace_clone_clone.fill(Qt::white);
			QCOMPARE(inplace_clone_clone.bits(), original.bits());     // not detached!
			QCOMPARE(original.pixel(32,32), QColor(Qt::white).rgb());  // changed!
			
			// An InplaceImage which is copy-constructed from a const InplaceImage does not detach when modified.
			inplace_clone_clone = InplaceImage(static_cast<InplaceImage const>(inplace_clone));
			QVERIFY(inplace_clone_clone.bits() == original.bits());    // not detached
			inplace_clone_clone.fill(Qt::white);
			QCOMPARE(inplace_clone_clone.bits(), original.bits());     // not detached!
			QCOMPARE(original.pixel(32,32), QColor(Qt::white).rgb());  // changed!
		}
	}
};
	
QTEST_GUILESS_MAIN(ParallelImageProcessingTest)
#include "ParallelImageProcessingTest.moc"  // IWYU pragma: keep
