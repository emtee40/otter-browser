/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_WIDGETFACTORY_H
#define OTTER_WIDGETFACTORY_H

#include "../core/ToolBarsManager.h"

#include <QtWidgets/QWidget>

namespace Otter
{

class ContentsWidget;
class MainWindow;
class ToolBarWidget;
class Window;

namespace WidgetFactory
{

ToolBarWidget* createToolBar(int identifier, Window *window, QWidget *parent);
QWidget* createToolBarItem(const ToolBarsManager::ToolBarDefinition::Entry &definition, Window *window, QWidget *parent);
ContentsWidget* createContentsWidget(const QString &identifier, const QVariantMap &parameters, Window *window, QWidget *parent);
ContentsWidget* createContentsWidget(const QUrl &url, const QVariantMap &parameters, Window *window, QWidget *parent);
ContentsWidget* createSidebarPanel(const QString &identifier, int sidebar, MainWindow *mainWindow, QWidget *parent);

}

}

#endif
