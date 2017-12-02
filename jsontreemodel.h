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

	JsonTreeModelNode(const QJsonValue& value, JsonTreeModelNode* parent) :
		m_parent(parent),
		m_scalarValue(value),
		m_type(Scalar) // TODO: Use subclassing and overridden function to report type
	{
		qDebug() << "Creating Scalar row for" << value;
	}

	JsonTreeModelNode(const QJsonArray& arr, JsonTreeModelNode* parent) :
		m_parent(parent),
		m_type(Array)
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
				childNode = new JsonTreeModelNode(child, this);
				break;

			case QJsonValue::Array:
				childNode = new JsonTreeModelNode(child.toArray(), this);
				break;

			case QJsonValue::Object:
				childNode = new JsonTreeModelNode(child.toObject(), this);
				break;

			default: break;
			}
			m_childList << childNode;
		}
	}

	JsonTreeModelNode(const QJsonObject& obj, JsonTreeModelNode* parent) :
		m_parent(parent),
		m_type(Object)
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
				break;

			case QJsonValue::Array:
				childNode = new JsonTreeModelNode(child.toArray(), this);
				m_childList << childNode;
				break;

			case QJsonValue::Object:
				childNode = new JsonTreeModelNode(child.toObject(), this);
				m_childList << childNode;
				break;

			default: break;
			}
			m_childNames[childNode] = key;
		}
	}

	~JsonTreeModelNode()
	{
		// TODO: Tell parent to remove this child from its list? Only if we do partial deletions

		qDeleteAll(m_childList);
	}


	inline JsonTreeModelNode* childAt(int i) const
	{ return m_childList[i]; }

	inline int childCount() const
	{ return m_childList.count(); }

	inline QString childName(JsonTreeModelNode* child) const
	{ return m_childNames[child]; }

	inline QJsonValue scalarValue() const
	{ return m_scalarValue; }

	inline QJsonValue namedScalarValue(const QString& name) const
	{ return m_namedScalarMap[name]; }

	inline JsonTreeModelNode* parent() const
	{ return m_parent; }

	inline Type type() const
	{ return m_type; }

private:
	JsonTreeModelNode* m_parent;
	QJsonValue m_scalarValue;
	QVector<JsonTreeModelNode*> m_childList;
	QMap<JsonTreeModelNode*, QString> m_childNames;
	QMap<QString, QJsonValue> m_namedScalarMap;
	Type m_type;
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
			m_rootNode = new JsonTreeModelNode(value.toArray(), nullptr);
		else
			m_rootNode = new JsonTreeModelNode(QJsonArray{value}, nullptr);

		// TODO: Hide the '0' label in the wrapper array

		// TODO: Handle cases where there's not Struct/Scalar column
		// TODO: Handle recursive header scans
//		m_headers = QStringList{"<structure>", "<scalar>"} << m_rootNode->namedScalars(0);

		// TODO: Emit the relevant signals
	}

private:
	JsonTreeModelNode* m_rootNode;

	QStringList m_headers;

	// TODO: Replace with QFlags
	bool m_structColumnVisible;
	bool m_scalarColumnVisible;
};

#endif // JSONTREEMODEL_H
