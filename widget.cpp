#include "widget.h"
#include "ui_widget.h"

#include "jsontreemodel.h"
#include <QJsonDocument>
#include <QDebug>

Widget::Widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Widget)
{
	ui->setupUi(this);

///*
	QJsonArray modelData{
		QJsonObject{
			{"str1", "Hello"},
			{"str2", "World"}
		},
		"Yabba",
		"Dabba",
		QJsonObject{
			{"str1", "Scooby"},
			{"str2", "Doo"}
		},
		QJsonArray{
			QJsonObject{
				{"obj1", QJsonObject{
						{"str1", "So"},
						{"str2", "Long"}
					}
				},
				{"obj2", QJsonObject{
						{"str1", "Fare"},
						{"str2", "Well"}
					}
				}
			},
			"Yee",
			"Haw"
		}
	};
//*/

/*
	QJsonArray modelData{
		QJsonObject{
			{"str1", "Hello"}
		}
	};
*/

	qDebug() << modelData;

	auto model = new JsonTreeModel(this);
	model->setJson(modelData);

	qDebug() << "rowCount:" << model->rowCount();
	qDebug() << "columnCount:" << model->columnCount();

	ui->treeView->setModel(model);
	ui->tableView->setModel(model);


	// Connect inputs
	connect(ui->pb_setScalarColumns, &QPushButton::clicked, [=]
	{
		auto columns = ui->te_scalarColumns->toPlainText().split('\n', QString::SkipEmptyParts);
		model->setScalarColumns(columns);
	});

	connect(ui->pb_setJson, &QPushButton::clicked, [=]
	{
		/*
			Qt bug? (as of Qt 5.10.0)
			- QJsonDocument::fromJson() fails with QJsonParseError::IllegalValue if the document is a single scalar value
			- doc.toVariant() correctly produces a QVariantList/QVariantMap if doc contains an array/object
			- doc.toVariant().toJsonValue() always produces a null value
		*/
		QJsonParseError err;
		auto rawString = ui->te_json->toPlainText();
		auto doc = QJsonDocument::fromJson(rawString.toUtf8(), &err);

		if (err.error == QJsonParseError::NoError)
		{
			auto headerSearchMode = JsonTreeModel::Manual;
			if (ui->comboBox->currentIndex() == 1)
				headerSearchMode = JsonTreeModel::QuickSearch;
			else if (ui->comboBox->currentIndex() == 2)
				headerSearchMode = JsonTreeModel::ComprehensiveSearch;

			if (doc.isArray())
				model->setJson(doc.array(), headerSearchMode);
			else if (doc.isObject())
				model->setJson(doc.object(), headerSearchMode);

			qDebug() << "LETS SEE";
			qDebug() << model->json();
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

	connect(ui->tableView, &QTableView::clicked, [=](const QModelIndex& index)
	{
		ui->ter_tableView->setText(getModelJson(index));
	});

	// Connect splitters
	connect(ui->sp_treeView, &QSplitter::splitterMoved, [=]
	{
		ui->sp_tableView->setSizes(ui->sp_treeView->sizes());
	});

	connect(ui->sp_tableView, &QSplitter::splitterMoved, [=]
	{
		ui->sp_treeView->setSizes(ui->sp_tableView->sizes());
	});
}

Widget::~Widget()
{
	delete ui;
}
