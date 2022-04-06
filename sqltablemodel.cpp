#include "sqltablemodel.h"

#include <QSqlRecord>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlField>
#include <QSqlDriver>
#include <QSqlIndex>

SqlTableModel::SqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel{parent, db}
{

}

// qtbase-everywhere-src-5.15.2\src\sql\kernel\qsqldriver.cpp
static QString prepareIdentifier(const QString &identifier,
        QSqlDriver::IdentifierType type, const QSqlDriver *driver)
{
    Q_ASSERT( driver != nullptr );
    QString ret = identifier;
    if (!driver->isIdentifierEscaped(identifier, type)) {
        ret = driver->escapeIdentifier(identifier, type);
    }
    return ret;
}

static QString upsertStatement(QSqlDriver* driver, const QString &tableName,
                        const QSqlRecord &rec, bool preparedStatement, QSqlRecord& whereValues) {

    Q_ASSERT_X(driver->dbmsType() == QSqlDriver::MySqlServer, "upsertStatement()", "not implemented");

    QString s;
    const auto tableNameString = tableName.isEmpty() ? QString()
                                    : prepareIdentifier(tableName, QSqlDriver::TableName, driver);

    s = s + QLatin1String("INSERT INTO ") + tableNameString + QLatin1String(" (");
    QString vals;
    for (int i = 0; i < rec.count(); ++i) {
        s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, driver)).append(QLatin1String(", "));
        if (preparedStatement)
            vals.append(QLatin1Char('?'));
        else
            vals.append(driver->formatValue(rec.field(i)));
        vals.append(QLatin1String(", "));
    }
    if (vals.isEmpty()) {
        s.clear();
    } else {
        vals.chop(2); // remove trailing comma
        s[s.length() - 2] = QLatin1Char(')');
        s.append(QLatin1String("VALUES (")).append(vals).append(QLatin1Char(')'));
    }
    s.append(" ON DUPLICATE KEY UPDATE ");
    QSqlRecord primaryKey = driver->primaryIndex(tableName);
    for (int i = 0; i < rec.count(); ++i) {
        if (!rec.isGenerated(i)) {
            continue;
        }
        if (primaryKey.contains(rec.fieldName(i))) {
            continue;
        }
        s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, driver)).append(QLatin1Char('='));
        if (preparedStatement) {
            s.append(QLatin1Char('?'));
            whereValues.append(rec.field(i));
        } else {
            s.append(driver->formatValue(rec.field(i)));
        }
        s.append(QLatin1String(", "));
    }
    if (s.endsWith(QLatin1String(", ")))
        s.chop(2);

    return s;
}

// qtbase-everywhere-src-5.15.2\src\sql\models\qsqltablemodel.cpp
bool SqlTableModel::insertRowIntoTable(const QSqlRecord &values)
{
    QSqlDatabase db = database();
    QString tableName = this->tableName();

    QSqlRecord rec = values;
    emit beforeInsert(rec);

    const bool prepStatement = db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QSqlRecord whereValues;
    const QString stmt = upsertStatement(db.driver(), tableName, rec, prepStatement, whereValues);

    if (stmt.isEmpty()) {
        qDebug() << "No Fields to update";
        return false;
    }

    return exec(stmt, prepStatement, rec, whereValues);
}

static QVariantList values(const QSqlRecord &rec) {
    QVariantList res = {};
    for(int i=0;i<rec.count();i++) {
        res.append(rec.field(i).value());
    }
    return res;
}

// qtbase-everywhere-src-5.15.2\src\sql\models\qsqltablemodel.cpp
bool SqlTableModel::exec(const QString &stmt, bool prepStatement,
                                 const QSqlRecord &rec, const QSqlRecord &whereValues)
{
    qDebug() << stmt;
    qDebug() << values(rec) << values(whereValues);
    if (stmt.isEmpty())
        return false;
    QSqlDatabase db = database();
    auto editQuery = QSqlQuery(db);

    if (prepStatement) {
        if (!editQuery.prepare(stmt)) {
            return false;
        }
        int i;
        for (i = 0; i < rec.count(); ++i)
            editQuery.addBindValue(rec.value(i));
        for (i = 0; i < whereValues.count(); ++i)
            if (!whereValues.isNull(i)) {
                editQuery.addBindValue(whereValues.value(i));
            } else {
                qDebug() << "unexpected null value";
                return false;
            }
        if (!editQuery.exec()) {
            qDebug() << editQuery.lastError().text();
            return false;
        }
    } else {
        if (!editQuery.exec(stmt)) {
            qDebug() << editQuery.lastError().text();
            return false;
        }
    }
    return true;
}
