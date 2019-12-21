/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
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

#include "coverunner.h"

#include <gui/main_window.h>

#include "app/mainform.h"

namespace OpenOrienteering {
class Map;
} // namespace OpenOrienteering

namespace cove {
void CoveRunner::run(OpenOrienteering::MainWindow* parent,
					 OpenOrienteering::Map* map,
					 OpenOrienteering::TemplateImage* templ)
{
	mainForm f(parent, map, templ);
	f.exec();
}
} // cove
