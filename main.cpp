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

    // ȷ��ֻ����һ��
    QSystemSemaphore sema("JAMKey",1,QSystemSemaphore::Open);
    sema.acquire();// ���ٽ������������ڴ�   SharedMemory
    QSharedMemory mem("Reminder");// ȫ�ֶ�����
    if (!mem.create(1))// ���ȫ�ֶ����Դ������˳�
    {
        QMessageBox::information(0, "msg", "An instance has already been running.");
        sema.release();// ����� Unix ϵͳ�����Զ��ͷš�
        return 0;
    }
    sema.release();// �ٽ���

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
