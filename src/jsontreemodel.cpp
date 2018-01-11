/*\
 * Copyright (c) 2018 Sze Howe Koh
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
\*/

#include "jsontreemodel.h"
#include <QJsonArray>
#include <QSet>

//=================================
// JsonTreeModelNode and subclasses
//=================================
JsonTreeModelScalarNode::JsonTreeModelScalarNode(const QJsonValue& value, JsonTreeModelNode* parent) :
	JsonTreeModelNode(parent),
	m_value(value)
{}

JsonTreeModelListNode::JsonTreeModelListNode(const QJsonArray& arr, JsonTreeModelNode* parent) :
	JsonTreeModelNode(parent)
{
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


//=================================
// JsonTreeModel itself
//=================================
JsonTreeModel::JsonTreeModel(QObject* parent) :
	QAbstractItemModel(parent),
	m_rootNode(nullptr),
	m_headers({"<Structure>", "<Scalar>"})
{}

QVariant JsonTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal && section < m_headers.count())
			return m_headers[section];

		// ASSUMPTION: Vertical headers are only requested by Table Views
		return data(index(section, 0));
	}
	return QAbstractItemModel::headerData(section, orientation, role);
}

QModelIndex JsonTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	// NOTE: m_headers also takes the struct column and scalar column into account
	if (column >= m_headers.count() || column < 0)
		return QModelIndex();

	auto parentNode = parent.isValid() ?
			static_cast<JsonTreeModelNode*>(parent.internalPointer()) :
			m_rootNode;

	if (parentNode->type() == JsonTreeModelNode::Scalar)
		return QModelIndex();

	// ASSUMPTION: For sub-items, parent's column always == 0 and the parent is an array/object
	// TODO: Check assumption
	auto specificParentNode = static_cast<JsonTreeModelListNode*>(parentNode);
	if (row >= specificParentNode->childCount() || row < 0)
		return QModelIndex();

	auto childRow = specificParentNode->childAt(row);
	return createIndex(row, column, childRow);
}

QModelIndex JsonTreeModel::parent(const QModelIndex& index) const
{
	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	if (node)
	{
		auto parentNode = node->parent();
		if (parentNode != nullptr && parentNode != m_rootNode)
		{
			// TODO: Calculate row!!!
			Q_ASSERT(parentNode->type() != JsonTreeModelNode::Scalar);
			return createIndex(0, 0 , parentNode);
		}
	}
	return QModelIndex();
}

int JsonTreeModel::rowCount(const QModelIndex& parent) const
{
	// NOTE: A QTreeView will try to probe the child count of all nodes, so we must check the node type.
	auto node = parent.isValid() ? static_cast<JsonTreeModelNode*>(parent.internalPointer()) : m_rootNode;
	if (node->type() == JsonTreeModelNode::Scalar)
		return 0;

	return static_cast<JsonTreeModelListNode*>(node)->childCount();
}

int JsonTreeModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);

	// ASSUMPTION: The headers list includes Struct and Scalar columns
	return m_headers.count();
}

QVariant JsonTreeModel::data(const QModelIndex& index, int role) const
{
	// ASSUMPTION: The process of generating this index has already validated the data
	if (!index.isValid())
		return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
		if (!node)
			return QVariant();

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
			Qt Bug? (As of Qt 5.10.0)
			- If the data value is a QJsonValue that holds a string, it shows up fine in the View.
			- If the data value is a QJsonValue that holds a double, it doesn't show up fine in the View.
			- If the data value is a QVariant that holds a double, it shows up fine in the View.

			Possibly related to the fact that QVariant::toString() works if the data is QJsonValue::String, but not QJsonValue::Number

			See also
			- QTBUG-52097
			- QTBUG-53579
			- QTBUG-60999
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

bool JsonTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if ( role != Qt::EditRole
			|| !isEditable(index) // NOTE: isEditable() checks for index validity
			|| data(index, role) == value )
	{
		return false;
	}

	QJsonValue newData;
	switch (value.type())
	{
	case QMetaType::Void:
		// newData remains null
		break;

	case QMetaType::Bool:
		newData = value.toBool();
		break;

	case QMetaType::Short:
	case QMetaType::UShort:
	case QMetaType::Int:
	case QMetaType::UInt:
	case QMetaType::Long:
	case QMetaType::ULong:
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
	case QMetaType::Float:
	case QMetaType::Double:
		newData = value.toDouble(); // TODO: Preserve numeric representation?
		break;

	// TODO: Convert char/QChar?

	case QMetaType::QString:
		newData = value.toString();
		break;

	default:
		return false;
	}

	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	Q_ASSERT(node != nullptr);
	switch (index.column())
	{
	case 0: // "Structure" column
		return false; // TODO: Allow changing an Object's member names

	case 1: // "Scalar" column
		Q_ASSERT(node->type() == JsonTreeModelNode::Scalar);
		static_cast<JsonTreeModelScalarNode*>(node)->setValue(newData);
		break;

	default: // Named scalar columns
		if (node->type() == JsonTreeModelNode::Object)
			static_cast<JsonTreeModelNamedListNode*>(node)->setNamedScalarValue(m_headers[index.column()], newData);
		else
			return false;
	}

	emit dataChanged(index, index, QVector<int>{role});
	return true;
}

Qt::ItemFlags JsonTreeModel::flags(const QModelIndex& index) const
{
	if (isEditable(index))
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	return QAbstractItemModel::flags(index);
}

QJsonValue JsonTreeModel::json(const QModelIndex& index) const
{
	// Not top-level
	if (index.isValid())
	{
		auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
		switch (index.column())
		{
		case 0: // "Structure" column
			if (node->type() != JsonTreeModelNode::Scalar)
				return node->value();
			break;

		case 1: // "Scalar" column
			if (node->type() == JsonTreeModelNode::Scalar)
				return node->value();
			break;

		default: // Named scalar columns
			if (node->type() == JsonTreeModelNode::Object)
				return static_cast<JsonTreeModelNamedListNode*>(node)->namedScalarValue(m_headers[index.column()]);
		}
		return QJsonValue();
	}

	// Top-level
	return m_rootNode->value();
}

void
JsonTreeModel::setJson(const QJsonArray& array, ScalarColumnSearchMode searchMode)
{
	beginResetModel();
	if (m_rootNode != nullptr)
		delete m_rootNode;
	m_rootNode = new JsonTreeModelListNode(array, nullptr);

	if (searchMode != NoSearch)
	{
		auto scalarCols = findScalarNames( array, (searchMode == ComprehensiveSearch) ).toList();
		std::sort(scalarCols.begin(), scalarCols.end());
		m_headers = QStringList{m_headers[0], m_headers[1]} << scalarCols;
		// TODO: Implement QList::resize() upstream to discard all columns except the first two? See QTBUG-42732
		// TODO: Check if it's safe to call setScalarColumns() here, which causes nested beginResetModel() calls
	}
	endResetModel();

	// TODO: Handle cases where there's no Struct/Scalar column
}

void
JsonTreeModel::setJson(const QJsonObject& object, ScalarColumnSearchMode searchMode)
{
	// TODO: (See todo list of other overload)
	beginResetModel();
	if (m_rootNode != nullptr)
		delete m_rootNode;
	m_rootNode = new JsonTreeModelNamedListNode(object, nullptr);

	if (searchMode != NoSearch)
	{
		auto scalarCols = findScalarNames( object, (searchMode == ComprehensiveSearch) ).toList();
		std::sort(scalarCols.begin(), scalarCols.end());
		m_headers = QStringList{m_headers[0], m_headers[1]} << scalarCols;
	}
	endResetModel();
}

void
JsonTreeModel::setScalarColumns(const QStringList& columns)
{
	// TODO: Check if there's anything in common first, before nuking the whole model?
	beginResetModel();
	m_headers = QStringList{m_headers[0], m_headers[1]} << columns;
	endResetModel();
}

QSet<QString>
JsonTreeModel::findScalarNames(const QJsonValue &data, bool comprehensive)
{
	auto processArray = [](const QJsonArray& array, bool comprehensive) -> QSet<QString>
	{
		QSet<QString> names;
		for (const auto element : array)
		{
			names += findScalarNames(element, comprehensive);
			if (!comprehensive)
				break; // Non-comprehensive searches only look at the first array element
		}
		return names;
	};

	QSet<QString> names;
	if (data.type() == QJsonValue::Array)
		names += processArray(data.toArray(), comprehensive);

	else if (data.type() == QJsonValue::Object)
	{
		auto dataObj = data.toObject();
		for (auto i = dataObj.constBegin(); i != dataObj.constEnd(); ++i)
		{
			const auto value = i.value();
			if (value.type() == QJsonValue::Array)
				names += processArray(value.toArray(), comprehensive);
			else if (value.type() == QJsonValue::Object)
				names += findScalarNames(value, comprehensive);
			else
				names += i.key(); // This is a scalar
		}
	}

	return names;
}

bool
JsonTreeModel::isEditable(const QModelIndex& index) const
{
	if (!index.isValid())
		return false;

	// TODO: Allow changing an Object's member names
	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	return !(  node->type() == JsonTreeModelNode::Array
			|| ( node->type() == JsonTreeModelNode::Scalar && index.column() != 1 )
			|| ( node->type() == JsonTreeModelNode::Object && index.column() <= 1 )  );

}
