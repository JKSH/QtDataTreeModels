/*\
 * Copyright (c) 2018 Sze Howe Koh
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
\*/

#ifndef JSONTREEMODEL_H
#define JSONTREEMODEL_H

#include <QAbstractItemModel>
#include <QJsonObject>
#include <QJsonArray>

//=================================
// JsonTreeModelNode and subclasses
//=================================
/*
	If only we could get a QJsonValueRef to a nested item,
	we wouldn't need such "heavy" node classes...
*/
/*
	TODO: Support user-defined Structure and Scalar column names
	TODO: Support user-defined icons for different datatypes in Structure column???
	TODO: Support user-defined font for Array indices in Structure column???
*/
class JsonTreeModelNode
{
public:
	enum Type {
		Scalar, ///< Represents scalar JSON values (nulls, Booleans, numbers, and strings).
		Object, ///< Represents JSON objects.
		Array   ///< Represents JSON arrays.
	};

	JsonTreeModelNode(JsonTreeModelNode* parent) : m_parent(parent) {}
	virtual ~JsonTreeModelNode() {}

	inline JsonTreeModelNode* parent() const
	{ return m_parent; }

	inline void setParent(JsonTreeModelNode* parent)
	{ Q_ASSERT(parent->type() != Scalar); m_parent = parent; }

	virtual Type type() const = 0;
	virtual QJsonValue value() const = 0;

private:
	// NOTE: Only JsonTreeModelListNode can be a parent, but I don't want to introduce a dependency to a subclass
	JsonTreeModelNode* m_parent;
};

class JsonTreeModelScalarNode : public JsonTreeModelNode
{
public:
	JsonTreeModelScalarNode(const QJsonValue& value, JsonTreeModelNode* parent);

	QJsonValue value() const override
	{ return m_value; }

	void setValue(const QJsonValue& value)
	{ m_value = value; }

	Type type() const override
	{ return Scalar; }

private:
	QJsonValue m_value;
};

class JsonTreeModelListNode : public JsonTreeModelNode
{
public:
	JsonTreeModelListNode(JsonTreeModelNode* parent) : JsonTreeModelNode(parent) {}
	JsonTreeModelListNode(const QJsonArray& array, JsonTreeModelNode* parent);

	~JsonTreeModelListNode() override
	{
		// TODO: Tell parent to remove this child from its list? Only if we do partial deletions
		qDeleteAll(m_childList);
	}

	inline JsonTreeModelNode* childAt(int i) const
	{ return m_childList[i]; }

	inline int childCount() const
	{ return m_childList.count(); }

	inline int childPosition(JsonTreeModelNode* child) const
	{ return m_childPositions.value(child, -1); }

	Type type() const override
	{ return Array; }

	QJsonValue value() const override;

protected:
	void registerChild(JsonTreeModelNode* child);
	void deregisterChild(JsonTreeModelNode* child);

private:
	QVector<JsonTreeModelNode*> m_childList;
	QMap<JsonTreeModelNode*, int> m_childPositions;
};

class JsonTreeModelNamedListNode : public JsonTreeModelListNode
{
public:
	JsonTreeModelNamedListNode(const QJsonObject& object, JsonTreeModelNode* parent);

	inline QString childListNodeName(JsonTreeModelNode* child) const
	{ return m_childListNodeNames[child]; }

	inline int namedScalarCount() const
	{ return m_namedScalarMap.size(); }

	inline QJsonValue namedScalarValue(const QString& name) const
	{ return m_namedScalarMap[name]; }

	inline void setNamedScalarValue(const QString& name, const QJsonValue& value)
	{
		Q_ASSERT(value.type() != QJsonValue::Undefined && value.type() != QJsonValue::Array && value.type() != QJsonValue::Object);
		m_namedScalarMap[name] = value;
	}

	Type type() const override
	{ return Object; }

	QJsonValue value() const override;

private:
	// TODO: Use JsonTreeModelListNode::childPosition() for indexing; not need for map with m_childListNodeNames
	QMap<JsonTreeModelNode*, QString> m_childListNodeNames;
	QMap<QString, QJsonValue> m_namedScalarMap;
};

class JsonTreeModelWrapperNode : public JsonTreeModelListNode
{
public:
	JsonTreeModelWrapperNode(JsonTreeModelNamedListNode* realNode);

	QJsonValue value() const override
	{ return childAt(0)->value(); } // ASSUMPTION: A wrapper node will always have exactly 1 child JsonTreeModelNamedListNode
};


//=================================
// JsonTreeModel itself
//=================================
class JsonTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	// TODO: Add flag to sort the keys, or leave them in the order of discovery
	enum ScalarColumnSearchMode
	{
		NoSearch,
		QuickSearch,
		ComprehensiveSearch
	};

	explicit JsonTreeModel(QObject* parent = nullptr);
	~JsonTreeModel() override { delete m_rootNode; }

	// Header:
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	// Basic functionality:
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	// Editable:
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	// API specific to JsonTreeModel:
	void setJson(const QJsonArray& array, ScalarColumnSearchMode searchMode = QuickSearch);
	void setJson(const QJsonObject& object, ScalarColumnSearchMode searchMode = QuickSearch);
	QJsonValue json(const QModelIndex& index = QModelIndex()) const;

	// TODO: Decide if the json()/setJson() API should be symmetrical or not

	void setScalarColumns(const QStringList& columns);
	QStringList scalarColumns() const { return m_headers.mid(2); }

private:
	bool isEditable(const QModelIndex& index) const;

	JsonTreeModelListNode* m_rootNode;
	QStringList m_headers;
};

#endif // JSONTREEMODEL_H
