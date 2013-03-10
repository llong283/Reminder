#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QtCore>
#include <QtGui>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include "SolarDate.h"
#include "ChineseDate.h"

#include "dialogset.h"
#include "dialogremind.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //����ʵ�ָú���������������ʾ������ʱ������ϵͳ����ͼ���Ҽ��˵���
    void setVisible ( bool visible );

protected:
    virtual bool eventFilter(QObject *o, QEvent *e);

private:
    Ui::MainWindow *ui;

    //members
    //���ݿ����
    QSqlDatabase m_db;
    QSqlTableModel m_tableModel;    //���������������ʾ�������
    QSqlQueryModel m_queryModel;    //��ѯ���ݿ���
    QSqlQuery m_query;              //�������ݿ���
    QSqlRecord m_record;            //һ����¼

    //���̲˵�
    QAction *restoreAction;         //��ԭ
    QAction *minAction;             //��С��
    QAction *setAction;             //����
    QAction *importAction;          //����
    QAction *exportAction;          //����
    QAction *helpAction;            //˵��
    QAction *quitAction;            //�˳�
    QSystemTrayIcon *trayIcon;      //����
    QMenu *trayIconMenu;            //���̲˵�

    //����Ҽ��˵�
    QAction *deleteAction;
    QMenu *tableViewMenu;

    //���ѶԻ���
    DialogRemind *dr;

    //��������
    QDate m_currentDate;

    //����
    bool m_startMin;        //����ʱ�Ƿ���С����true����С����false������С��
    int m_remindTime;       //����������ú�����
    int m_quitTime;         //���Ѷ�ú��˳���0Ϊ���˳�
    int m_aheadDays;        //��ǰ����������
    QString m_dataSep;      //���ݷָ���
    QString m_timeSep;      //���ڷָ���

    //methods
    void init();                            //��ʼ��
    void initTrayIcon();                    //��ʼ������
    void initTable();                       //��ʼ�����

private slots:
    void slotUpdateFormat();                //����������ʾ�ͱ���������Ϣ
    void slotUpdateEditArea();              //���±༭��
    void slotUpdateTrayTip();               //����������ʾ��Ϣ
    void slotNew();                         //�½���Ա
    void slotSave();                        //������Ա
    void slotDelete();                      //ɾ����Ա
    void slotMenuRequested(QPoint);         //�Ҽ��˵���Ӧ
    void slotUpdateDays();                  //���±༭�����������е�����
    void slotSort();                        //�������
    void slotReadSettings();                //��ȡ������Ϣ���������Ƿ��ڹر����һ������ʱ�˳�
    void slotImport();                      //��������
    void slotExport();                      //��������
    void slotSet();                         //����
    void slotQuit();                        //�˳�
    void slotHelp();                        //�鿴˵��
    void slotLook();                        //��ʾ������
    void slotReminder();                    //��ʼ�����ѶԻ���
    void slotTrayIconActivated(QSystemTrayIcon::ActivationReason reason);  //˫��ϵͳ����ʱ��ԭ����С��
    void on_calendarWidget_clicked(QDate date);                            //���������Ӧ
    void on_pushButtonToday_clicked();      //���ؽ���
};

#endif // MAINWINDOW_H
