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
		Structure
	};

	JsonTreeModelNode(const QJsonValue& value, JsonTreeModelNode* parent, const QString& name = QString()) :
		m_parent(parent),
		m_scalarValue(value),
		m_name(name), // TODO: Remove this? Perhaps the parent should keep track of the names, since it already keeps a map
		m_type(Scalar) // TODO: Use subclassing and overridden function to report type
	{
		qDebug() << "Creating Scalar row for" << value;
	}

	JsonTreeModelNode(const QJsonArray& arr, JsonTreeModelNode* parent, const QString& name = QString()) :
		m_parent(parent),
		m_name(name),
		m_type(Structure)
	{
		qDebug() << "Creating Array row for" << arr;

		JsonTreeModelNode* childNode;
		for (int i = 0; i < arr.count(); ++i)
		{
			const auto& child = arr[i];
//			m_childList << new JsonTreeModelNode(arr[i], this, QString::number(i)); // TODO: Let the model manage the number, not the node itself

			switch (child.type())
			{
			case QJsonValue::Null:
			case QJsonValue::Bool:
			case QJsonValue::Double:
			case QJsonValue::String:
				childNode = new JsonTreeModelNode(child, this, QString::number(i));
				break;

			case QJsonValue::Array:
				childNode = new JsonTreeModelNode(child.toArray(), this, QString::number(i));
				break;

			case QJsonValue::Object:
				childNode = new JsonTreeModelNode(child.toObject(), this, QString::number(i));
				break;

			default: break;
			}
			m_childList << childNode;
		}
	}

	JsonTreeModelNode(const QJsonObject& obj, JsonTreeModelNode* parent, const QString& name = QString()) :
		m_parent(parent),
		m_name(name),
		m_type(Structure)
	{
		qDebug() << "Creating Object row for" << obj;

		for (const QString& key : obj.keys())
		{
			const auto& child = obj[key];
//			auto childNode = new JsonTreeModelNode(child, this, key);

			switch (child.type())
			{
			case QJsonValue::Null:
			case QJsonValue::Bool:
			case QJsonValue::Double:
			case QJsonValue::String:
				m_namedScalarMap[key] = child;
				break;

			case QJsonValue::Array:
				m_childList << new JsonTreeModelNode(child.toArray(), this, key);
				break;

			case QJsonValue::Object:
				m_childList << new JsonTreeModelNode(child.toObject(), this, key);
				break;

			default: break;
			}
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

	inline QJsonValue scalarValue() const
	{ return m_scalarValue; }

	inline QJsonValue namedScalarValue(const QString& name) const
	{ return m_namedScalarMap[name]; }

	inline QString label() const
	{ return m_name; }

	inline JsonTreeModelNode* parent() const
	{ return m_parent; }

private:
	JsonTreeModelNode* m_parent;
	QJsonValue m_scalarValue;
	QVector<JsonTreeModelNode*> m_childList;
	QMap<QString, QJsonValue> m_namedScalarMap;
	QString m_name;
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
