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

    //重新实现该函数，当主窗口显示或隐藏时，更新系统托盘图标右键菜单项
    void setVisible ( bool visible );

protected:
    virtual bool eventFilter(QObject *o, QEvent *e);

private:
    Ui::MainWindow *ui;

    //members
    //数据库相关
    QSqlDatabase m_db;
    QSqlTableModel m_tableModel;    //与表格关联，更新显示表格数据
    QSqlQueryModel m_queryModel;    //查询数据库用
    QSqlQuery m_query;              //更新数据库用
    QSqlRecord m_record;            //一条记录

    //托盘菜单
    QAction *restoreAction;         //还原
    QAction *minAction;             //最小化
    QAction *setAction;             //设置
    QAction *importAction;          //导入
    QAction *exportAction;          //导出
    QAction *helpAction;            //说明
    QAction *quitAction;            //退出
    QSystemTrayIcon *trayIcon;      //托盘
    QMenu *trayIconMenu;            //托盘菜单

    //表格右键菜单
    QAction *deleteAction;
    QMenu *tableViewMenu;

    //提醒对话框
    DialogRemind *dr;

    //今天日期
    QDate m_currentDate;

    //设置
    bool m_startMin;        //启动时是否最小化，true：最小化，false：不最小化
    int m_remindTime;       //程序启动多久后提醒
    int m_quitTime;         //提醒多久后退出，0为不退出
    int m_aheadDays;        //提前多少天提醒
    QString m_dataSep;      //数据分隔符
    QString m_timeSep;      //日期分隔符

    //methods
    void init();                            //初始化
    void initTrayIcon();                    //初始化托盘
    void initTable();                       //初始化表格

private slots:
    void slotUpdateFormat();                //更新日历显示和本月生日信息
    void slotUpdateEditArea();              //更新编辑区
    void slotUpdateTrayTip();               //更新托盘提示信息
    void slotNew();                         //新建人员
    void slotSave();                        //保存人员
    void slotDelete();                      //删除人员
    void slotMenuRequested(QPoint);         //右键菜单响应
    void slotUpdateDays();                  //更新编辑区出生日期中的天数
    void slotSort();                        //表格排序
    void slotReadSettings();                //读取设置信息，并决定是否在关闭最后一个窗口时退出
    void slotImport();                      //导入数据
    void slotExport();                      //导出数据
    void slotSet();                         //设置
    void slotQuit();                        //退出
    void slotHelp();                        //查看说明
    void slotLook();                        //显示主窗口
    void slotReminder();                    //初始化提醒对话框
    void slotTrayIconActivated(QSystemTrayIcon::ActivationReason reason);  //双击系统托盘时还原或最小化
    void on_calendarWidget_clicked(QDate date);                            //点击日历响应
    void on_pushButtonToday_clicked();      //返回今天
};

#endif // MAINWINDOW_H
