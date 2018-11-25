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
/*!
	\class JsonTreeModelNode
	\brief JsonTreeModelNode is the base class for the internal data structure behind JsonTreeModel.
*/
/*!
	\enum JsonTreeModelNode::Type
	\brief This enum describes the type of data represented by a JsonTreeModelNode.

	\sa type()
*/
/*!
	\fn JsonTreeModelNode::JsonTreeModelNode
	\brief Constructs a new node with the given \a parent.

	\note Only a JsonTreeModelListNode (or one of its subclasses) can be a parent.
*/
/*!
	\fn JsonTreeModelNode::~JsonTreeModelNode
	\brief Frees the memory held by this node and its children.

	\warning This node is not automatically removed from its parent's list of children.
*/
/*!
	\fn JsonTreeModelNode* JsonTreeModelNode::parent
	\brief Returns this node's parent.

	\sa setParent()
*/
/*!
	\fn void JsonTreeModelNode::setParent
	\brief Makes this node a child of \a parent.

	\note Only a JsonTreeModelListNode (or one of its subclasses) can be a parent.

	\warning This function only updates this node's internal pointer to its parent. The caller
			 must manually update the child's
			 \link JsonTreeModelListNode::registerChild() registration\endlink.

	\sa parent()
*/
/*!
	\fn virtual QJsonValue JsonTreeModelNode::value
	\brief Returns the JSON value represented by this node and its children (if any).
*/

/*!
	\class JsonTreeModelScalarNode
	\brief JsonTreeModelScalarNode is the most basic element of a JsonTreeModel's internal data.

	It represents a single scalar JSON value (nulls, Booleans, numbers, and strings).

	\note JsonTreeModelScalarNode cannot be the \link JsonTreeModelNode::parent() parent \endlink of another node.
*/
/*!
	\brief Constructs a node under the specified \a parent to represent the specified scalar \a value.
*/
JsonTreeModelScalarNode::JsonTreeModelScalarNode(const QJsonValue& value, JsonTreeModelNode* parent) :
	JsonTreeModelNode(parent),
	m_value(value)
{}

/*!
	\fn Type JsonTreeModelScalarNode::type
	\brief Returns JsonTreeModelNode::Scalar
*/
/*!
	\fn void JsonTreeModelScalarNode::setValue
	\brief Replaces the \a value represented by this node.

	\sa value()
*/
/*!
	\fn QJsonValue JsonTreeModelScalarNode::value
	\brief Returns the scalar JSON value represented by this node.

	\sa setValue()
*/

/*!
	\class JsonTreeModelListNode
	\brief JsonTreeModelListNode represents a JSON structure (namely an array or an object)
		   and provides the backbone of the tree model.

	This class can have \e children. Child nodes manifest as child rows in the
	JsonTreeModel.

	JSON objects are best represented as a JsonTreeModelNamedListNode.
*/
/*!
	\fn JsonTreeModelListNode::JsonTreeModelListNode(JsonTreeModelNode* parent)
	\brief Constructs an empty JsonTreeModelListNode under the specified \a parent.

	The new node can be populated later.
*/
/*!
	\brief Constructs a node under the specified \a parent to represent the specified JSON \a array.

	All \a array elements will be placed within child nodes and \link registerChild() registered\endlink.
*/
JsonTreeModelListNode::JsonTreeModelListNode(const QJsonArray& array, JsonTreeModelNode* parent) :
	JsonTreeModelNode(parent)
{
	JsonTreeModelNode* childNode;
	for (int i = 0; i < array.count(); ++i)
	{
		const auto& child = array[i];

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
		registerChild(childNode);
	}
}

/*!
	\fn JsonTreeModelNode* JsonTreeModelListNode::childAt
	\brief Returns the child node at index \a i.

	\warning The caller must ensure that 0 <= \a i < childCount()

	\sa childPosition()
*/
/*!
	\fn int JsonTreeModelListNode::childCount
	\brief Returns the number of child nodes under this row.

	This is equivalent to the number of \link JsonTreeModel::rowCount() rows \endlink
	in the JsonTreeModel where this node is the parent QModelIndex.
*/
/*!
	\fn int JsonTreeModelListNode::childPosition
	\brief Returns index number of the specified \a child, or
	-1 if the \a child does not belong to this node.

	\sa childAt()
*/
/*!
	\fn Type JsonTreeModelListNode::type
	\brief Returns JsonTreeModelNode::Array.
*/

/*!
	\brief Returns the JSON structure (array or object) represented by this node.
*/
QJsonValue
JsonTreeModelListNode::value() const
{
	QJsonArray fullArray;
	for (const auto childNode : qAsConst(m_childList))
		fullArray << childNode->value();
	return fullArray;
}

/*!
	\brief Puts the \a child node under this node's hierarchy

	\note Only the \a child's parent can call this function
*/
void
JsonTreeModelListNode::registerChild(JsonTreeModelNode* child)
{
	Q_ASSERT_X(child->parent() == this, "registerChild()", "Only a parent can register its own child");
	m_childPositions[child] = m_childList.count();
	m_childList << child;
}

/*!
	\brief Removes the \a child node from this node's hierarchy

	\note Only the \a child's parent can call this function
*/
void
JsonTreeModelListNode::deregisterChild(JsonTreeModelNode* child)
{
	Q_ASSERT_X(child->parent() == this, "deregisterChild()", "Only a parent can deregister its own child");
	auto i = m_childPositions.take(child);
	m_childList.remove(i);

	// ASSUMPTION: Registration/deregistration is infrequent, but lookups are very frequent. Thus, this O(n) loop is acceptable.
	while (i < m_childList.count())
	{
		m_childPositions[ m_childList[i] ] = i;
		++i;
	}
	// TODO: Add function to deregister multiple children simultaneously
}

/*!
	\class JsonTreeModelNamedListNode
	\brief JsonTreeModelNamedListNode represents a JSON object.

	Scalar members of this node appear in named
	\link JsonTreeModel::scalarColumns() scalar columns\endlink in the JsonTreeModel.

	Non-scalar members of this node (arrays and objects) appear as child rows in the JsonTreeModel.

	If this node represents a top-level object of a JSON document and it contains scalar
	members, then this node must be wrapped in a JsonTreeModelWrapperNode.
*/
/*!
	\brief Constructs a node under the specified \a parent to represent the specified JSON \a object.
*/
JsonTreeModelNamedListNode::JsonTreeModelNamedListNode(const QJsonObject& object, JsonTreeModelNode* parent) :
	JsonTreeModelListNode(parent)
{
	JsonTreeModelNode* childNode;
	for (const QString& key : object.keys())
	{
		const auto& child = object[key];
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
		registerChild(childNode);
		m_childListNodeNames[childNode] = key;
	}
}

/*!
	\fn QString JsonTreeModelNamedListNode::childListNodeName
	\brief
*/
/*!
	\fn int JsonTreeModelNamedListNode::namedScalarCount
	\brief Returns the number of scalar elements within this node.

	In the JsonTreeModel, these are the elements that appear under the
	model's \link JsonTreeModel::scalarColumns() scalar columns\endlink.
*/
/*!
	\fn QJsonValue JsonTreeModelNamedListNode::namedScalarValue
	\brief Returns the scalar element in this node which has the given \a name.

	If the underlying JSON object has no member with the given \a name
	(or if the member is non-scalar), this function returns an undefined
	QJsonValue.

	\sa setNamedScalarValue()
*/
/*!
	\fn void JsonTreeModelNamedListNode::setNamedScalarValue
	\brief Adds or updates a scalar element of the JSON object represented
		   by this node.

	\sa namedScalarValue()
*/
/*!
	\fn Type JsonTreeModelNamedListNode::type
	\brief Returns JsonTreeModelNode::Object
*/
/*!
	\fn QJsonValue JsonTreeModelNamedListNode::value
	\brief Returns the JSON object represented by this node.
*/

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

/*!
	\class JsonTreeModelWrapperNode
	\brief The JsonTreeModelWrapperNode class wraps a top-level JsonTreeModelNamedListNode,
		   to allow its scalar members to be shown.
*/
/*!
	\brief Constructs a wrapper for \a realNode and takes ownership of it.

	Unlike the constructors for other classes derived from JsonTreeModelNode, this one does not take a parent
	because JsonTreeModelWrapperNode is only meant to be used as the JsonTreeModel's root node.
*/
JsonTreeModelWrapperNode::JsonTreeModelWrapperNode(JsonTreeModelNamedListNode* realNode) :
	JsonTreeModelListNode(nullptr)
{
	Q_ASSERT(realNode->parent() == nullptr);

	// NOTE: Only a parent can register a child, so we must call setParent() before registerChild()
	realNode->setParent(this);
	registerChild(realNode);
}

/*!
	\fn QJsonValue JsonTreeModelWrapperNode::value

	\brief Returns the JSON object represented by the wrapped node.
*/


//=================================
// JsonTreeModel itself
//=================================
/*!
	\class JsonTreeModel
	\brief The JsonTreeModel class provides a data model for a JSON document.

	JsonTreeModel represents an arbitrary JSON document as a tree. It supports unlimited nesting of
	JSON arrays and JSON objects. For compactness, scalar members of JSON objects are placed under
	named columns.

	- Column 0 shows the structure of the JSON document. It contains array index numbers and object
	  member names.
	- Column 1 shows the scalar elements of JSON arrays.
	- Columns 2 and above show the scalar members of JSON objects. The header text for these
	  columns are the names of the object members, either discovered during the call to setJson(),
	  or set manually via setScalarColumns(). These columns are called the \e named \e scalar
	  \e columns.

	For example, the following JSON document contains an array of similar objects which can be
	compacted into a table:

	\code{.js}
		[
			{
				"First Name": "Hua",
				"Last Name": "Li",
				"Phone Number": "+86 21 51748525",
				"Country": "China"
			}, {
				"First Name": "Gildong",
				"Last Name": "Hong",
				"Phone Number": "+82 31 712 0045",
				"Country": "South Korea"
			}, {
				"First Name": "Tarou",
				"Last Name": "Yamada",
				"Phone Number": "+81 3 6264 4500",
				"Country": "Japan"
			}, {
				"First Name": "Jane",
				"Last Name": "Doe",
				"Phone Number": "+1 408 906 8400",
				"Country": "USA"
			}, {
				"First Name": "Erika",
				"Last Name": "Mustermann",
				"Phone Number": "+49 30 63923257",
				"Country": "Germany"
			}, {
				"First Name": "Pyotr",
				"Last Name": "Ivanov",
				"Phone Number": "+7 921 097 7252",
				"Country": "Russia"
			}, {
				"First Name": "Kari",
				"Last Name": "Nordmann",
				"Phone Number": "+47 21 08 04 20",
				"Country": "Norway"
			}
		]
	\endcode

	\image html example_table.png

	\note Use \c QTableView::hideColumn() or \c QTreeView::hideColumn() to hide the empty \e Scalar column

	Here is a more hierarchical example:

	\code{.js}
		{
			"Server Properties": {
				"Server ID": "314159",
				"Client IP Addresses": [
					"192.168.0.10",
					"192.168.0.11",
					"192.168.0.12"
				]
			},
			"Analog Inputs": [
				{
					"Channel Name": "Transducer X",
					"Analog Input Type": "Voltage",
					"Scale": 1,
					"Offset": 0,
					"High Resolution": false
				},
				{
					"Channel Name": "Sensor Y",
					"Analog Input Type": "Current",
					"Scale": 6.25,
					"Offset": -25,
					"High Resolution": true
				}
			]
		}
	\endcode

	\image html example_tree.png
*/

/*!
	\enum JsonTreeModel::ScalarColumnSearchMode
	\brief This enum controls how setJson() updates the model's column headers.

	\sa setJson(), setScalarColumns()
*/
/*!
	\var JsonTreeModel::NoSearch

	setJson() leaves the model headers unchanged. The caller should manually update the
	column headers via setScalarColumns().
*/
/*!
	\var JsonTreeModel::QuickSearch

	setJson() does a quick scan on the new JSON document. The model scans every member
	of JSON objects, but only scans the first element of JSON arrays.
*/
/*!
	\var JsonTreeModel::ComprehensiveSearch

	setJson() does a full scan on the new JSON document. The model scans every member
	of JSON objects and JSON arrays. All scalar members will be found and displayed,
	but this could be expensive for large JSON documents.
*/


/*!
	\brief Constructs an empty JsonTreeModel with the given \a parent.
*/
JsonTreeModel::JsonTreeModel(QObject* parent) :
	QAbstractItemModel(parent),
	m_rootNode(nullptr),
	m_headers({"<Structure>", "<Scalar>"})
{}

/*!
	\fn JsonTreeModel::~JsonTreeModel()

	\brief Destroys the JsonTreeModel and frees its memory.
*/

/*!
	Horizontal headers show the text of scalarColumns() for the third column onwards.
	Vertical headers show the text of column 0.
*/
QVariant
JsonTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex
JsonTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	// NOTE: m_headers also takes the struct column and scalar column into account
	if (column >= m_headers.count() || column < 0)
		return QModelIndex();

	auto parentNode = parent.isValid() ?
			static_cast<JsonTreeModelNode*>(parent.internalPointer()) :
			m_rootNode;

	if (parentNode == nullptr || parentNode->type() == JsonTreeModelNode::Scalar) // Short-circuit
		return QModelIndex();

	// ASSUMPTION: For sub-items, parent's column always == 0 and the parent is an array/object
	// TODO: Check assumption
	auto specificParentNode = static_cast<JsonTreeModelListNode*>(parentNode);
	if (row >= specificParentNode->childCount() || row < 0)
		return QModelIndex();

	auto childRow = specificParentNode->childAt(row);
	return createIndex(row, column, childRow);
}

QModelIndex
JsonTreeModel::parent(const QModelIndex& index) const
{
	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	if (node)
	{
		auto parentNode = node->parent();
		if (parentNode != nullptr && parentNode != m_rootNode)
		{
			Q_ASSERT(parentNode->type() != JsonTreeModelNode::Scalar);
			auto grandparentNode = static_cast<JsonTreeModelListNode*>(parentNode->parent());
			Q_ASSERT(grandparentNode != nullptr);
			int parentRow = grandparentNode->childPosition(parentNode);

			return createIndex(parentRow, 0 , parentNode);
		}
	}
	return QModelIndex();
}

/*!
	\brief Returns the number of rows under the given \a parent.

	If the \a parent represents a JSON array, then the row count equals the number of array elements.
	If the \a parent represents a JSON object, then the row count equals the number of child arrays and
	child objects combined.

	\sa columnCount()
*/
int
JsonTreeModel::rowCount(const QModelIndex& parent) const
{
	// NOTE: A QTreeView will try to probe the child count of all nodes, so we must check the node type.
	auto node = parent.isValid() ? static_cast<JsonTreeModelNode*>(parent.internalPointer()) : m_rootNode;
	if (node == nullptr || node->type() == JsonTreeModelNode::Scalar) // Short-circuit
		return 0;

	return static_cast<JsonTreeModelListNode*>(node)->childCount();
}

/*!
	\brief Returns the number of columns in the model.

	This number is the same for the entire model; the \a parent is irrelevant.
	- Column 0 shows the structure of the JSON document. It contains array index numbers and object
	  member names.
	- Column 1 shows the scalar elements of JSON arrays.
	- Columns 2 and above are the \e named \e scalar \e columns, corresponding to scalarColumns().

	\sa rowCount(), scalarColumns()
*/
int
JsonTreeModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);

	// ASSUMPTION: The headers list includes Struct and Scalar columns
	return m_headers.count();
}

/*!
	\brief Returns data under the given \a index for the specified \a role.

	Only valid when \a role is \c Qt::DisplayRole or \c Qt::EditRole.

	If \c index.column() is 0, then this function returns the array index or object member name that
	corresponds to \c index.row(). This differs from json(), which returns the full JSON value if
	\c index.column() is 0.

	\note This function is designed for interfacing with Qt item views. To access data, use json().

	\sa setData(), json()
*/
QVariant
JsonTreeModel::data(const QModelIndex& index, int role) const
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

/*
	While setJson() updates the data for the entire model, setData() only updates the data for a single
*/
bool
JsonTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if ( role != Qt::EditRole
			|| !isEditable(index) // NOTE: isEditable() checks for index validity
			|| data(index, role) == value )
	{
		return false;
	}

	QJsonValue newData;
	switch (  static_cast<QMetaType::Type>( value.type() )  )
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

Qt::ItemFlags
JsonTreeModel::flags(const QModelIndex& index) const
{
	if (isEditable(index))
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	return QAbstractItemModel::flags(index);
}

/*!
	\brief Returns the JSON value under the given \a index.

	If the \a index is invalid, then this function returns the entire JSON document stored in the
	model.

	If \c index.column() is 0, then this function returns the full JSON value at \c index.row(); the
	value could be a JSON object or JSON array. If \c index.column() is non-zero, this function
	returns a single scalar	value.

	Example:

	\image html example_table.png

	- Index (6, 2) returns the string "Kari".
	- Index (6, 1) returns an undefined QJsonValue.
	- Index (6, 0) returns the following object:
		\code{.js}
			{
				"First Name": "Kari",
				"Last Name": "Nordmann",
				"Phone Number": "+47 21 08 04 20",
				"Country": "Norway"
			}
		\endcode

	- Invalid indices return the whole document:
		\code{.js}
			[
				{
					"First Name": "Hua",
					"Last Name": "Li",
					"Phone Number": "+86 21 51748525",
					"Country": "China"
				}, {
					"First Name": "Gildong",
					"Last Name": "Hong",
					"Phone Number": "+82 31 712 0045",
					"Country": "South Korea"
				}, {
					"First Name": "Tarou",
					"Last Name": "Yamada",
					"Phone Number": "+81 3 6264 4500",
					"Country": "Japan"
				}, {
					"First Name": "Jane",
					"Last Name": "Doe",
					"Phone Number": "+1 408 906 8400",
					"Country": "USA"
				}, {
					"First Name": "Erika",
					"Last Name": "Mustermann",
					"Phone Number": "+49 30 63923257",
					"Country": "Germany"
				}, {
					"First Name": "Pyotr",
					"Last Name": "Ivanov",
					"Phone Number": "+7 921 097 7252",
					"Country": "Russia"
				}, {
					"First Name": "Kari",
					"Last Name": "Nordmann",
					"Phone Number": "+47 21 08 04 20",
					"Country": "Norway"
				}
			]
		\endcode

	\sa setJson(), data()
*/
QJsonValue
JsonTreeModel::json(const QModelIndex& index) const
{
	// Top-level
	if (!index.isValid())
	{
		if (m_rootNode == nullptr)
			return QJsonValue();
		return m_rootNode->value();
	}

	// Not top-level
	auto node = static_cast<JsonTreeModelNode*>(index.internalPointer());
	switch (index.column())
	{
	case 0: // "Structure" column
		return node->value();

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

static QSet<QString>
findScalarNames(const QJsonValue &data, bool comprehensive);

/*!
	\brief Sets the whole model's internal data structure to the given JSON \a array.

	If \a searchMode is \c QuickSearch (default) or \c ComprehensiveSearch, this function also
	updates the column headers.

	\sa json(), setData()
*/
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

/*!
	\brief Sets the whole model's internal data structure to the given JSON \a object.

	If \a searchMode is \c QuickSearch (default) or \c ComprehensiveSearch, this function also updates the column headers.

	\sa json(), setData()
*/
void
JsonTreeModel::setJson(const QJsonObject& object, ScalarColumnSearchMode searchMode)
{
	// TODO: (See todo list of other overload)
	beginResetModel();
	if (m_rootNode != nullptr)
		delete m_rootNode;

	auto namedListNode = new JsonTreeModelNamedListNode(object, nullptr);
	if (namedListNode->namedScalarCount() > 0)
	{
		auto wrapper = new JsonTreeModelWrapperNode(namedListNode);
		m_rootNode = wrapper;
	}
	else
		m_rootNode = namedListNode;

	if (searchMode != NoSearch)
	{
		auto scalarCols = findScalarNames( object, (searchMode == ComprehensiveSearch) ).toList();
		std::sort(scalarCols.begin(), scalarCols.end());
		m_headers = QStringList{m_headers[0], m_headers[1]} << scalarCols;
	}
	endResetModel();
}

/*!
	\fn JsonTreeModel::scalarColumns
	\brief Returns the names of the JSON objects' scalar members that are shown by the model.

	\sa setScalarColumns()
*/

/*!
	\brief Sets the JSON objects' scalar members that are shown by the model.

	The model's \e named \e scalar \e columns are set to the list of specified \a columns, in the
	listed order.

	If a scalar member of a JSON object is named after one of these columns, it is shown under that
	column. Otherwise, the member is hidden.

	\sa scalarColumns(), setJson()
*/
void
JsonTreeModel::setScalarColumns(const QStringList& columns)
{
	// TODO: Check if there's anything in common first, before nuking the whole model?
	beginResetModel();
	m_headers = QStringList{m_headers[0], m_headers[1]} << columns;
	endResetModel();
}

static QSet<QString>
findScalarNames(const QJsonValue &data, bool comprehensive)
{
	auto processArray = [](const QJsonArray& array, bool comprehensive) -> QSet<QString>
	{
		QSet<QString> names;
		for (const auto element : array)
		{
			if (element.type() == QJsonValue::Object || element.type() == QJsonValue::Array)
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

/*!
	Returns \e true if the data under the given \a index is editable.

	Only scalar elements of JSON arrays or scalar members of JSON objects are editable.
*/
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
