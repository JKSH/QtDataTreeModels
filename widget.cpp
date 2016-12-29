#include "widget.h"
#include "ui_widget.h"

#include "jsontreemodel.h"

#include <QDebug>

Widget::Widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Widget)
{
	ui->setupUi(this);

///*
	QJsonArray array{
		QJsonObject{
			{"str1", "Hello"},
			{"str2", "World"}
		},
		"Yabba",
		"Dabba",
		QJsonObject{
			{"str1", "Scooby"},
			{"str2", "Doo"}
		}
	};
//*/

/*
	QJsonArray array{
		QJsonObject{
			{"str1", "Hello"}
		}
	};
*/

	qDebug() << array;

	auto model = new JsonTreeModel(this);
	model->setJson(array);

	qDebug() << "rowCount:" << model->rowCount();
	qDebug() << "columnCount:" << model->columnCount();

	ui->treeView->setModel(model);
}

Widget::~Widget()
{
	delete ui;
}
