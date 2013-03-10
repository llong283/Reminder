#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QTextCodec>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec *gbk = QTextCodec::codecForName("gb18030");
    QTextCodec::setCodecForCStrings(gbk);
    QTextCodec::setCodecForLocale(gbk);
    QTextCodec::setCodecForTr(gbk);

    // 确保只运行一次
    QSystemSemaphore sema("JAMKey",1,QSystemSemaphore::Open);
    sema.acquire();// 在临界区操作共享内存   SharedMemory
    QSharedMemory mem("Reminder");// 全局对象名
    if (!mem.create(1))// 如果全局对象以存在则退出
    {
        QMessageBox::information(0, "msg", "An instance has already been running.");
        sema.release();// 如果是 Unix 系统，会自动释放。
        return 0;
    }
    sema.release();// 临界区

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(QApplication::applicationDirPath() + "/reminder.db");
    if (!db.open()) {
        QMessageBox::information(NULL, "msg", "open db failed");
        return -1;
    }
    else {
        QSqlQuery query;
        query.exec("create table if not exists birthday("
                   "id INTEGER NOT NULL ON CONFLICT FAIL PRIMARY KEY ON CONFLICT FAIL UNIQUE ON CONFLICT FAIL,"
                   "name VARCHAR(0),"
                   "birthday DATE,"
                   "type VARCHAR(0),"
                   "date DATE)");
        if (query.lastError().isValid()) {
            QMessageBox::information(NULL, "msg", query.lastError().text());
            return -1;
        }
    }

    MainWindow w;
//    w.show();

    return a.exec();
}
