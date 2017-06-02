#include "jsontreemodel.h"
#include <QJsonArray>

#include <QDebug>

JsonTreeModel::JsonTreeModel(QObject* parent) :
	QAbstractItemModel(parent)
{
	// TODO: Make use of the following flags
	m_structColumnVisible = true;
	m_scalarColumnVisible = true;
	m_compactMode = false;

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

QModelIndex JsonTreeModel::blankIndex(int row, int column, const JsonTreeModelNode* parentNode, const QString& reason) const
{
	if (!reason.isEmpty())
		qDebug() << "\tCreating index for BLANK cell:" << reason;

	return createIndex(row, column, parentNode->blank());
}

QModelIndex JsonTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	qDebug() << "JsonTreeModel::index()" << row << column << parent;

	if (column >= m_headers.count() || column < 0) // NOTE: m_headers also takes the struct column and scalar column into account
		return nullIndex("Invalid header count");

	// TODO: Replace with "auto"
	JsonTreeModelNode* parentNode = parent.isValid() ?
			static_cast<JsonTreeModelNode*>(parent.internalPointer()) :
			m_rootNode;

	// ASSUMPTION: For sub-items, parent's column always == 0 and the parent is an array/object
	// TODO: Check assumption

	// TODO: Use same algorithm for both cMod and rMod
	int cMod = column;
	int rMod = row;

	// NOTE: Only Objects can have Named Scalar elements
	if (parentNode->hasNamedScalars())
	{
		qDebug() << "\tParent node has named scalars. Decrementing rMod.";
		--rMod;
	}
	else
	{
		qDebug() << "\tParent node has NO named scalars.";

		if (parent.row() == -1 && parent.column() == 0)
		{
			qDebug() << "Hmm";
		}
	}


	if (m_structColumnVisible)
	{
		// If the caller is asking for a Structure...
		if (cMod == 0)
		{
			if (rMod == -1)
			{
				qDebug() << "\tCreating blank index for named scalar in core column";
//				return nullIndex(); // This row only contains Named Scalars
				return blankIndex(row, column, parentNode, "Name scalar in core column");
			}

			auto childNode = parentNode->childAt(rMod);
			if ( childNode && !childNode->isScalar() ) // Short-circuit
			{
				qDebug() << "\tCreating valid index for" << row << column << childNode;
				return createIndex(row, column, childNode);
			}
//			return nullIndex();
			return blankIndex(row, column, parentNode, "Not a structure");
		}

		// Not looking for the struct column. Move along.
		--cMod;
	}

	if (m_scalarColumnVisible)
	{
		// If the caller is asking for an Unnamed Scalar...
		if (cMod == 0)
		{
			if (rMod == -1)
//				return nullIndex(); // This row only contains Named Scalars
				return blankIndex(row, column, parentNode, "?!!");

			// NOTE: Only Arrays can have Unnamed Scalar elements
			// TODO: Support cases where the root value is a Scalar
			if (parentNode->isArray())
			{
				auto childNode = parentNode->childAt(row);
				if (childNode == nullptr || !childNode->isScalar()) // Short-circuit
//					return nullIndex(); // TODO: Return special value to represent Undefined?
					return blankIndex(row, column, parentNode, "Not a scalar");

				qDebug() << "\tCreating valid index for" << row << column << childNode;
				return createIndex(row, column, childNode);
			}
//			return nullIndex();
			return createIndex(row, column, parentNode->blank());
		}

		// Not looking for the scalar column. Move along.
		--cMod;
	}

	// TODO: Unify the style of the previous Array check and the following Object check

	// TODO: Support Compact Mode
	// If the caller is asking for a Named Scalar...
	if (rMod == -1)
	{
		// NOTE: Only Objects can have Named Scalar elements
		if (!parentNode->isObject())
			return nullIndex("Parent is not an object");

		auto childNode = parentNode->namedScalarNode(m_headers[column]);
		if (!childNode)
			return nullIndex("Named scalar not found");

		qDebug() << "\tCreating valid index for" << row << column << childNode;
		return createIndex(row, column, childNode);
	}

	qDebug() << "\tBO JIO!!!" << rMod << cMod;
	// None of the above. There's nothing here
	return blankIndex(row, column, parentNode, "There's nothing here.");
//	return QModelIndex(); // Using an invalid index makes highlighting not work for that row...
}

QModelIndex JsonTreeModel::parent(const QModelIndex& index) const
{
	qDebug() << "JsonTreeModel::parent()" << index;

	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	if (node)
	{
		auto parentNode = node->parentNode();
		if (parentNode != nullptr && parentNode != m_rootNode)
		{
			// TODO: Calculate row and column!!!
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

	int n = node->childCount();
	if (node->hasNamedScalars())
		++n;
	return n;
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
//		qDebug() << "\tReturning display data!" << node->displayData();

		// TODO: Do data display logic here (and remove displayData())? That would require doing the same gymnastics as index() though
		return node->displayData();
	}

	return QVariant();
}
