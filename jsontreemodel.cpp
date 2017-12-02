#include "jsontreemodel.h"
#include <QJsonArray>

#include <QDebug>

JsonTreeModel::JsonTreeModel(QObject* parent) :
	QAbstractItemModel(parent)
{
	// TODO: Make use of the following flags
	m_structColumnVisible = true;
	m_scalarColumnVisible = true;

	// TODO: Adapt headers to the model
	m_headers = QStringList{"<Structure>", "<Scalar>", "str1", "str2"};
}

QVariant JsonTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
//	qDebug() << "JsonTreeModel::headerData()" << section << orientation << role;

	if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section < m_headers.count())
		return m_headers[section];

	return QAbstractItemModel::headerData(section, orientation, role);
}

QModelIndex nullIndex(const QString& reason)
{
	if (!reason.isEmpty())
		qDebug() << "\tCreating NULL index:" << reason;

	return QModelIndex();
}

QModelIndex JsonTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	qDebug() << "JsonTreeModel::index()" << row << column << parent;

	// NOTE: m_headers also takes the struct column and scalar column into account
	if (column >= m_headers.count() || column < 0)
		return nullIndex("Invalid header count");

	auto parentNode = parent.isValid() ?
			static_cast<JsonTreeModelNode*>(parent.internalPointer()) :
			m_rootNode;

	if (row >= parentNode->childCount() || row < 0)
		return nullIndex("Invalid row number");

	// ASSUMPTION: For sub-items, parent's column always == 0 and the parent is an array/object
	// TODO: Check assumption

	auto childRow = parentNode->childAt(row);
	qDebug() << "\tCreating valid index for" << row << column << childRow;
	return createIndex(row, column, childRow);
}

QModelIndex JsonTreeModel::parent(const QModelIndex& index) const
{
	qDebug() << "JsonTreeModel::parent()" << index;

	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	if (node)
	{
		auto parentNode = node->parent();
		if (parentNode != nullptr && parentNode != m_rootNode)
		{
			// TODO: Calculate row!!!
			qDebug() << "\tCreating NON-ROOT index, pointing to" << parentNode;
			return createIndex(0, 0 , parentNode);
		}
		return nullIndex("Parent is root or doesn't exist");
	}
	return nullIndex("Node not specified");
}

int JsonTreeModel::rowCount(const QModelIndex& parent) const
{
//	qDebug() << "JsonTreeModel::rowCount()" << parent;

	auto node = parent.isValid() ? static_cast<JsonTreeModelNode*>(parent.internalPointer()) : m_rootNode;
	return node->childCount();
}

int JsonTreeModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);

//	qDebug() << "JsonTreeModel::columnCount()" << parent;

	// ASSUMPTION: The headers list includes Struct and Scalar columns
	return m_headers.count();
}

QVariant JsonTreeModel::data(const QModelIndex& index, int role) const
{
//	qDebug() << "JsonTreeModel::data()" << index << role;
	qDebug() << '.';

	// ASSUMPTION: The process of generating this index has already validated the data
	if (!index.isValid())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
//		qDebug() << "JsonTreeModel::data()" << index << (DISPLAYROLE);
		auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
		if (!node)
		{
			qDebug() << "\tNull node!";
			return QVariant();
		}

		auto col = index.column();
		switch (col)
		{
		case 0: // Struct column
			{
				if (node->parent()->type() == JsonTreeModelNode::Array)
					return index.row();
				else
					return node->parent()->childName(node);
			}

		/*
			Qt Bug? (As of Qt 5.9.3)
			- If the data value is a QJsonValue that holds a string, it shows up fine in the View.
			- If the data value is a QJsonValue that holds a double, it doesn't show up fine in the View.
			- If the data value is a QVariant that holds a double, it shows up fine in the View.
		*/
		case 1: // Scalar column
			return node->scalarValue().toVariant();

		default: // Named scalars
			if (col < m_headers.count())
				return node->namedScalarValue( m_headers[col] ).toVariant();
		}
	}
	return QVariant();
}
