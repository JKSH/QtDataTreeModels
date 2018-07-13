/*\
 * Copyright (c) 2018 Sze Howe Koh
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
\*/

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
