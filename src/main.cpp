#include "widget.h"
#include <QApplication>

#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Widget w;
	w.show();

/*
	QJsonArray arr
	{
		"Hello",
		"World",
		QJsonObject
		{
			{"str1", "Laa"}
		}
	};
	QJsonValueRefPtr ptr(&arr, 0);

	qDebug() << arr;
	*ptr = "Whee";

	qDebug() << arr[2] << arr[2].isObject();

	arr[2].toObject();

	// ... Can't get a QJsonValueRef to an nested structure... :-(
	qDebug() << arr;
*/

	return a.exec();
}
