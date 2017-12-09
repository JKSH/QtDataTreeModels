#include "jsontreemodel.h"
#include <QJsonArray>

#include <QDebug>
JsonTreeModelScalarNode::JsonTreeModelScalarNode(const QJsonValue& value, JsonTreeModelNode* parent) :
	JsonTreeModelNode(parent),
	m_value(value)
{
	qDebug() << "Creating Scalar row for" << value;
}

JsonTreeModelListNode::JsonTreeModelListNode(const QJsonArray& arr, JsonTreeModelNode* parent) :
	JsonTreeModelNode(parent)
{
	qDebug() << "Creating Array row for" << arr;

	JsonTreeModelNode* childNode;
	for (int i = 0; i < arr.count(); ++i)
	{
		const auto& child = arr[i];

		switch (child.type())
		{
		case QJsonValue::Null:
		case QJsonValue::Bool:
		case QJsonValue::Double:
		case QJsonValue::String:
			childNode = new JsonTreeModelScalarNode(child, this);
			break;

		case QJsonValue::Array:
			childNode = new JsonTreeModelListNode(child.toArray(), this);
			break;

		case QJsonValue::Object:
			childNode = new JsonTreeModelNamedListNode(child.toObject(), this);
			break;

		default: break;
		}
		addChild(childNode);
	}
}

QJsonValue
JsonTreeModelListNode::value() const
{
	QJsonArray fullArray;
	for (const auto childNode : qAsConst(m_childList))
		fullArray << childNode->value();
	return fullArray;
}

JsonTreeModelNamedListNode::JsonTreeModelNamedListNode(const QJsonObject& obj, JsonTreeModelNode* parent) :
	JsonTreeModelListNode(parent)
{
	qDebug() << "Creating Object row for" << obj;

	JsonTreeModelNode* childNode;
	for (const QString& key : obj.keys())
	{
		const auto& child = obj[key];
		switch (child.type())
		{
		case QJsonValue::Null:
		case QJsonValue::Bool:
		case QJsonValue::Double:
		case QJsonValue::String:
			m_namedScalarMap[key] = child;
			continue;

		case QJsonValue::Array:
			childNode = new JsonTreeModelListNode(child.toArray(), this);
			break;

		case QJsonValue::Object:
			childNode = new JsonTreeModelNamedListNode(child.toObject(), this);
			break;

		default: continue;
		}
		addChild(childNode);
		m_childListNodeNames[childNode] = key;
	}
}

QJsonValue
JsonTreeModelNamedListNode::value() const
{
	QJsonObject fullObject;
	for (auto i = m_namedScalarMap.constBegin(); i != m_namedScalarMap.constEnd(); ++i)
		fullObject.insert(i.key(), i.value());
	for (auto i = m_childListNodeNames.constBegin(); i != m_childListNodeNames.constEnd(); ++i)
		fullObject.insert(i.value(), i.key()->value()); // i's value is the element name, while i's key is the node
	return fullObject;
}

JsonTreeModel::JsonTreeModel(QObject* parent) :
	QAbstractItemModel(parent)
{
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

	if (parentNode->type() == JsonTreeModelNode::Scalar)
		return nullIndex("Scalar parent (Bad index!)");

	// ASSUMPTION: For sub-items, parent's column always == 0 and the parent is an array/object
	// TODO: Check assumption
	auto specificParentNode = static_cast<JsonTreeModelListNode*>(parentNode);
	if (row >= specificParentNode->childCount() || row < 0)
		return nullIndex("Invalid row number");

	qDebug() << "\tCreating valid index for" << row << column << childRow;
	auto childRow = specificParentNode->childAt(row);
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
			Q_ASSERT(parentNode->type() != JsonTreeModelNode::Scalar);
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

	// NOTE: A QTreeView will try to probe the child count of all nodes, so we must check the node type.
	auto node = parent.isValid() ? static_cast<JsonTreeModelNode*>(parent.internalPointer()) : m_rootNode;
	if (node->type() == JsonTreeModelNode::Scalar)
		return 0;

	return static_cast<JsonTreeModelListNode*>(node)->childCount();
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

				// A node's parent cannot be a Scalar node
				Q_ASSERT(node->parent()->type() == JsonTreeModelNode::Object);
				return static_cast<JsonTreeModelNamedListNode*>( node->parent() )->childListNodeName(node);
			}

		/*
			Qt Bug? (As of Qt 5.9.3)
			- If the data value is a QJsonValue that holds a string, it shows up fine in the View.
			- If the data value is a QJsonValue that holds a double, it doesn't show up fine in the View.
			- If the data value is a QVariant that holds a double, it shows up fine in the View.
		*/
		case 1: // Scalar column
			if (node->type() == JsonTreeModelNode::Scalar)
				return static_cast<JsonTreeModelScalarNode*>(node)->value().toVariant();
			break;

		default: // Named scalars
			if (col < m_headers.count() && node->type() == JsonTreeModelNode::Object)
				return static_cast<JsonTreeModelNamedListNode*>(node)->namedScalarValue( m_headers[col] ).toVariant();
		}
	}
	return QVariant();
}
