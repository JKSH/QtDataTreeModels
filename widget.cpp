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
	connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
	{
		if (index == 2)
			model->setScalarColumnDiscoveryMode(JsonTreeModel::ComprehensiveSearch);
		else if (index == 1)
			model->setScalarColumnDiscoveryMode(JsonTreeModel::QuickSearch);
		else
			model->setScalarColumnDiscoveryMode(JsonTreeModel::Manual);
	});

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
		}
		else if (err.error == QJsonParseError::IllegalValue)
		{
			/*
				Qt bug? (as of Qt 5.10.0)
				- QJsonDocument::fromJson() fails with QJsonParseError::IllegalValue if the document is a single scalar value

				We do manual parsing here
			*/
			if (rawString.startsWith('\"') && rawString.endsWith('\"'))
				model->setJson(   QJsonValue(  rawString.mid( 1, rawString.length()-2 )  )   );
			else if (  isdigit( rawString[0].toLatin1() )  ) // ASSUMPTION: QJsonParseError::IllegalValue means the string is not empty
				model->setJson(  QJsonValue( rawString.toDouble() ) );
			else
			{
				auto moddedString = rawString.toLower();
				if (moddedString == "true" || moddedString == "false")
					model->setJson( QJsonValue(moddedString == "true") );
				else if (moddedString == "null")
					model->setJson( QJsonValue() );
				else
					qDebug() << "NOOOO!!!";
			}

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
