#ifndef JSONTREEMODEL_H
#define JSONTREEMODEL_H

#include <QAbstractItemModel>

#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

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
		Scalar,
		Object,
		Array
	};

	JsonTreeModelNode(JsonTreeModelNode* parent) : m_parent(parent) {}
	virtual ~JsonTreeModelNode() {}

	inline JsonTreeModelNode* parent() const
	{ return m_parent; }

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

	Type type() const override
	{ return Scalar; }

private:
	QJsonValue m_value;
};

class JsonTreeModelListNode : public JsonTreeModelNode
{
public:
	JsonTreeModelListNode(JsonTreeModelNode* parent) : JsonTreeModelNode(parent) {}
	JsonTreeModelListNode(const QJsonArray& arr, JsonTreeModelNode* parent);

	~JsonTreeModelListNode()
	{
		// TODO: Tell parent to remove this child from its list? Only if we do partial deletions
		qDeleteAll(m_childList);
	}

	inline JsonTreeModelNode* childAt(int i) const
	{ return m_childList[i]; }

	inline int childCount() const
	{ return m_childList.count(); }

	Type type() const override
	{ return Array; }

	QJsonValue value() const override;

protected:
	void addChild(JsonTreeModelNode* child)
	{ m_childList << child; }

private:
	QVector<JsonTreeModelNode*> m_childList;
};

class JsonTreeModelNamedListNode : public JsonTreeModelListNode
{
public:
	JsonTreeModelNamedListNode(const QJsonObject& obj, JsonTreeModelNode* parent);

	inline QString childListNodeName(JsonTreeModelNode* child) const
	{ return m_childListNodeNames[child]; }

	inline QJsonValue namedScalarValue(const QString& name) const
	{ return m_namedScalarMap[name]; }

	Type type() const override
	{ return Object; }

	QJsonValue value() const override;

private:
	QMap<JsonTreeModelNode*, QString> m_childListNodeNames;
	QMap<QString, QJsonValue> m_namedScalarMap;
};


//=================================
// JsonTreeModel itself
//=================================
class JsonTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum ScalarColumnSearchMode
	{
		NoSearch,
		QuickSearch,
		ComprehensiveSearch
	};

	explicit JsonTreeModel(QObject* parent = nullptr);
	~JsonTreeModel() { delete m_rootNode; }

	// Header:
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	// Basic functionality:
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	void setJson(const QJsonArray& array, ScalarColumnSearchMode searchMode = QuickSearch);
	void setJson(const QJsonObject& object, ScalarColumnSearchMode searchMode = QuickSearch);
	QJsonValue json(const QModelIndex& index = QModelIndex()) const;

	void setScalarColumns(const QStringList& columns);
	QStringList scalarColumns() const { return m_headers.mid(2); }

private:
	static QSet<QString> findScalarNames(const QJsonValue& data, bool comprehensive);

	JsonTreeModelListNode* m_rootNode;
	QStringList m_headers;
};

#endif // JSONTREEMODEL_H
