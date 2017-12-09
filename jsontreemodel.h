#ifndef JSONTREEMODEL_H
#define JSONTREEMODEL_H

#include <QAbstractItemModel>

#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

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

class JsonTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	// TODO: Make these flags; add "PreserveOrder" flag for Comprehensive mode
	enum HeaderDiscoveryMode
	{
		Manual,
		FirstObjectOnly,
		Comprehensive
	};

	explicit JsonTreeModel(QObject* parent = nullptr);
	~JsonTreeModel(){}

	// Header:
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	// Basic functionality:
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	void setJson(const QJsonValue& value)
	{
		// TODO: Discard old data

		qDebug() << "Setting JSON:" << value;
		if (value.type() == QJsonValue::Array)
			m_rootNode = new JsonTreeModelListNode(value.toArray(), nullptr);
		else
			m_rootNode = new JsonTreeModelListNode(QJsonArray{value}, nullptr);

		// TODO: Hide the '0' label in the wrapper array

		// TODO: Handle cases where there's not Struct/Scalar column
		// TODO: Handle recursive header scans
//		m_headers = QStringList{"<structure>", "<scalar>"} << m_rootNode->namedScalars(0);

		// TODO: Emit the relevant signals
	}

private:
	JsonTreeModelListNode* m_rootNode;

	QStringList m_headers;

	// TODO: Replace with QFlags
	bool m_structColumnVisible;
	bool m_scalarColumnVisible;
};

#endif // JSONTREEMODEL_H
