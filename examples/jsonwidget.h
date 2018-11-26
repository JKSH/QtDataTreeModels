// Copyright (c) 2018 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#ifndef JSONWIDGET_H
#define JSONWIDGET_H

#include <QWidget>
class JsonTreeModel;

namespace Ui {
class JsonWidget;
}

class JsonWidget : public QWidget
{
	Q_OBJECT

public:
	explicit JsonWidget(QWidget* parent = 0);
	~JsonWidget();

	void addDocument(const QString& name, const QJsonDocument& doc);

private slots:
	void applyJsonText();

private:
	Ui::JsonWidget* ui;
	JsonTreeModel* m_model;
};

#endif // JSONWIDGET_H
