#ifndef JSONTREEMODEL_H
#define JSONTREEMODEL_H

#include <QAbstractItemModel>

#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

class JsonTreeModelNode
{
public:
	JsonTreeModelNode(const QJsonValue& value, JsonTreeModelNode* parent, const QString& name = QString()) :
		m_value(value),
		m_parent(parent),
		m_undefinedChild(nullptr),
		m_name(name) // TODO: Remove this? Perhaps the parent should keep track of the names, since it already keeps a map
	{
		qDebug() << "Creating node for" << value << "...";

		switch (value.type())
		{
		case QJsonValue::Null:
		case QJsonValue::Bool:
		case QJsonValue::Double:
		case QJsonValue::String:
			break;

		case QJsonValue::Array:
			{
/*
				for (const auto& child : value.toArray())
					m_childList << new JsonTreeModelNode(child, this);
				break;
*/
				auto arr = value.toArray();
				for (int i = 0; i < arr.count(); ++i)
					m_childList << new JsonTreeModelNode(arr[i], this, QString::number(i)); // TODO: Let the model manage the number, not the node itself

				m_undefinedChild = new JsonTreeModelNode(QJsonValue(), this); // TODO: Only create if the node has any sub-items
				break;
			}

		case QJsonValue::Object:
			{
				const QJsonObject obj = value.toObject();
				for (const QString& key : obj.keys())
				{
					const auto& child = obj[key];
					auto childNode = new JsonTreeModelNode(child, this, key);
					switch (child.type())
					{
					case QJsonValue::Null:
					case QJsonValue::Bool:
					case QJsonValue::Double:
					case QJsonValue::String:
						m_namedScalarMap[key] = childNode;
						break;

					case QJsonValue::Array:
					case QJsonValue::Object:
						m_childList << childNode;
						break;

					default: break;
					}
				}

				m_undefinedChild = new JsonTreeModelNode(QJsonValue(), this);
				break;
			}
		default: break;
		}

		qDebug() << "Done:" << m_value;
		qDebug() << "Named Scalars:" << m_namedScalarMap;
		qDebug() << "Children:" << m_childList << '\n';
	}

	~JsonTreeModelNode()
	{
		// TODO: Tell parent to remove this child from its list? Only if we do partial deletions

		qDeleteAll(m_namedScalarMap);
		qDeleteAll(m_childList);
	}

	inline bool hasNamedScalars() const { return !m_namedScalarMap.isEmpty(); }
	// TODO: Provide options for recursive searching and order-preservation
	QStringList namedScalars(int /*recursionDepth*/) const { return m_namedScalarMap.keys(); } // TODO: Rename (to "scalarNames"?

	JsonTreeModelNode* namedScalarNode(const QString& name) const
	{
		// TODO: Check if default value == nullptr
		return m_namedScalarMap.value(name);
	}

	JsonTreeModelNode* parentNode() const { return m_parent; }

	inline bool isObject() const { return m_value.isObject(); }
	inline bool isArray() const { return m_value.isArray(); }
	inline bool isScalar() const { return !(isObject() || isArray()); }

	int childCount() const { return m_childList.count(); } // Excludes named scalars

	JsonTreeModelNode* childAt(int index) const
	{
		if (index >= m_childList.count())
			return nullptr;
		return m_childList[index];
	}

	JsonTreeModelNode* blank() const { return m_undefinedChild; } // TODO: Rename

	QVariant displayData() const
	{
//		qDebug() << "\tDisplaying!" << m_value;

		if (isScalar())
			return m_value.toVariant();

		// NOTE: Name will only be displayed in the <Structure> column
		if (isObject() || isArray())
			return m_name; // TODO: Do this differently?

		// TODO: Check if the node can even be anything other than Object, Array, or Scalar
		return QVariant();
	}

	QJsonValue jsonValue() const
	{
		return m_value;
	}

private:
	QJsonValue m_value;
	JsonTreeModelNode* m_parent;
	JsonTreeModelNode* m_undefinedChild;
	QString m_name;

	QMap<QString, JsonTreeModelNode*> m_namedScalarMap;
	QVector<JsonTreeModelNode*> m_childList;
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

		m_rootNode = new JsonTreeModelNode(value, nullptr);

		// TODO: Handle cases where there's not Struct/Scalar column
		// TODO: Handle recursive header scans
//		m_headers = QStringList{"<structure>", "<scalar>"} << m_rootNode->namedScalars(0);

		// TODO: Emit the relevant signals
	}

private:
	JsonTreeModelNode* m_rootNode;
	JsonTreeModelNode* m_tmpUndefinedNode;

	QStringList m_headers;

	// TODO: Replace with QFlags
	bool m_structColumnVisible;
	bool m_scalarColumnVisible;
	bool m_compactMode;
};

#endif // JSONTREEMODEL_H
