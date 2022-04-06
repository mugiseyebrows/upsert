#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlTableModel>

class SqlTableModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit SqlTableModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase());

    bool exec(const QString &stmt, bool prepStatement, const QSqlRecord &rec, const QSqlRecord &whereValues);
signals:

protected:
    bool insertRowIntoTable(const QSqlRecord &values);
};

#endif // SQLTABLEMODEL_H
