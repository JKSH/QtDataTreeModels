// Copyright (c) 2018 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include "jsonwidget.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QMap>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	JsonWidget w;

	// Populate widget with sample data
	const QMap<QString, QString> resourceMap
	{
		{ "Address Book Table", ":/resources/address_book_table.json" },
		{ "Data Logger Tree",   ":/resources/data_logger_tree.json"   }
	};
	for (auto name : resourceMap.keys())
	{
		QFile inputFile(resourceMap[name]);
		inputFile.open(QFile::ReadOnly|QFile::Text);
		w.addDocument(  name,  QJsonDocument::fromJson( inputFile.readAll() )  );
	}

	// Activate widget
	w.show();
	return a.exec();
}
