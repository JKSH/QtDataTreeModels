/*\
 * Copyright (c) 2018 Sze Howe Koh
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
\*/

#ifndef JSONWIDGET_H
#define JSONWIDGET_H

#include <QWidget>

namespace Ui {
class JsonWidget;
}

class JsonWidget : public QWidget
{
	Q_OBJECT

public:
	explicit JsonWidget(QWidget* parent = 0);
	~JsonWidget();

private:
	Ui::JsonWidget* ui;
};

#endif // JSONWIDGET_H
