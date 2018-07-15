/*\
 * Copyright (c) 2018 Sze Howe Koh
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
\*/

#include "jsonwidget.h"
#include "ui_jsonwidget.h"
#include <QMessageBox>

#include "../src/jsontreemodel.h"
#include <QJsonDocument>

JsonWidget::JsonWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::JsonWidget),
	m_model(new JsonTreeModel(this))
{
	ui->setupUi(this);

	auto model = m_model;
	ui->treeView->setModel(model);


	// Connect inputs
	connect(ui->pb_setScalarColumns, &QPushButton::clicked, [=]
	{
		auto columns = ui->te_scalarColumns->toPlainText().split('\n', QString::SkipEmptyParts);
		model->setScalarColumns(columns);
	});

	connect(ui->pb_setJson, &QPushButton::clicked,
			this, &JsonWidget::applyJsonText);

	connect(ui->cb_jsonSource, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
	{
		if (index == 0)
		{
			// Let the user type in the JSON text; don't update the model
			ui->te_json->setEnabled(true);
			ui->pb_setJson->setEnabled(true);
		}
		else
		{
			// Apply the preset, read-only JSON text; auto-update the model
			ui->te_json->setEnabled(false);
			ui->pb_setJson->setEnabled(false);

			auto jsonText = ui->cb_jsonSource->itemData(index).toString();
			ui->te_json->setText(jsonText);
			applyJsonText();
		}
	});


	// Connect View actions
	auto getModelJson = [=](const QModelIndex& index) -> QByteArray
	{
		auto json = model->json(index);
		switch (json.type())
		{
		case QJsonValue::Bool: return json.toBool() ? "true" : "false";
		case QJsonValue::Double: return QByteArray::number(json.toDouble());
		case QJsonValue::String: return json.toString().toUtf8();
		case QJsonValue::Array: return QJsonDocument(json.toArray()).toJson();
		case QJsonValue::Object: return QJsonDocument(json.toObject()).toJson();
		default: return QByteArray();
		}
	};

	connect(ui->treeView, &QTreeView::clicked, [=](const QModelIndex& index)
	{
		ui->ter_treeView->setText(getModelJson(index));
	});
}

JsonWidget::~JsonWidget()
{
	delete ui;
}

void
JsonWidget::addDocument(const QString& name, const QJsonDocument& doc)
{
	// Stringify, store, and select the JSON document
	ui->cb_jsonSource->addItem(name, doc.toJson());
	ui->cb_jsonSource->setCurrentIndex( ui->cb_jsonSource->count() - 1 );
}

void
JsonWidget::applyJsonText()
{
	QJsonParseError err;
	auto rawString = ui->te_json->toPlainText();
	auto doc = QJsonDocument::fromJson(rawString.toUtf8(), &err);

	if (err.error == QJsonParseError::NoError)
	{
		auto headerSearchMode = JsonTreeModel::NoSearch;
		if (ui->comboBox->currentIndex() == 1)
			headerSearchMode = JsonTreeModel::QuickSearch;
		else if (ui->comboBox->currentIndex() == 2)
			headerSearchMode = JsonTreeModel::ComprehensiveSearch;

		if (doc.isArray())
			m_model->setJson(doc.array(), headerSearchMode);
		else if (doc.isObject())
			m_model->setJson(doc.object(), headerSearchMode);

		ui->te_scalarColumns->setText( m_model->scalarColumns().join('\n') );
	}
	else
		QMessageBox::warning(this, "Error", "Invalid JSON array/object");
}
