#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QDebug>
#include "sqltablemodel.h"
#include <QTableView>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("");
    db.setDatabaseName("");
    db.setUserName("");
    db.setPassword("");
    if (!db.open()) {
        qDebug() << db.lastError().text();
        return 1;
    }

    auto execute = [](const QString& query){
        QSqlQuery q;
        qDebug() << query;
        if (!q.exec(query)) {
            qDebug() << q.lastError().text();
        }
    };

    execute("DROP TABLE foo");
    execute("CREATE TABLE foo(i int primary key auto_increment, t text)");

    auto* model = new SqlTableModel(nullptr, db);
    model->setTable("foo");
    model->select();

    auto insert_row = [=](int i, const QString& t){
        int row = model->rowCount();
        bool res = model->insertRow(row);
        if (!res) {
            qDebug() << "failed to insert row" << row;
        }
        model->setData(model->index(row, 0), i);
        model->setData(model->index(row, 1), t);
        model->submit();
    };

    execute("INSERT INTO foo(i, t) VALUES(1, 'foo')");

    // upsert
    insert_row(1, "bar");

    // insert
    insert_row(2, "baz");

    qDebug() << "result:";
    model->select();
    for(int row=0;row<model->rowCount();row++) {
        qDebug() << model->index(row,0).data().toInt()
                 << model->index(row,1).data().toString();
    }

    return a.exec();
}
