#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    //初始化窗口状态
    QRect screenRect;
    QPoint pos;
    screenRect = QApplication::desktop()->availableGeometry();
    pos = QPoint((screenRect.width() - frameSize().width()) / 2,
                 (screenRect.height() - frameSize().height()) / 2);
    move(pos);
    setWindowFlags(Qt::WindowMinimizeButtonHint);
    setFixedSize(size());
    setWindowTitle("生日提醒");
    if (m_startMin)
        hide();
    else
        show();    
}

MainWindow::~MainWindow()
{
    delete dr;
    delete ui;
}

//------------------------------------------------------------------
//初始化
//------------------------------------------------------------------
void MainWindow::init()
{
    //初始化变量
    m_currentDate = QDate::currentDate();
    m_startMin = true;
    m_quitTime = 0;
    m_aheadDays = 10;
    m_remindTime = 0;
    dr = NULL;
    slotReadSettings();
    m_db = QSqlDatabase::database();

    //初始化控件
    deleteAction = new QAction("delete", this);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(slotDelete()));

    tableViewMenu = new QMenu(ui->tableView);
    tableViewMenu->addAction(deleteAction);

    m_tableModel.setEditStrategy(QSqlTableModel::OnManualSubmit);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableView->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);

    ui->dateEditDate->setEnabled(false);

    ui->labelInfo->setStyleSheet("QLabel {border: 1px solid black;}");
    ui->labelInfo->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    ui->dateEditDate->setFocusPolicy(Qt::NoFocus);
    ui->lineEditId->setFocusPolicy(Qt::NoFocus);
    ui->tableView->setFocusPolicy(Qt::ClickFocus);
    ui->calendarWidget->setFocusPolicy(Qt::ClickFocus);

    ui->pushNew->installEventFilter(this);
    ui->pushSave->installEventFilter(this);
    ui->pushDelete->installEventFilter(this);
    ui->lineEditName->installEventFilter(this);

    ui->pushNew->setShortcut(QKeySequence("Ctrl+N"));
    ui->pushSave->setShortcut(QKeySequence("Ctrl+S"));
    ui->pushDelete->setShortcut(QKeySequence("Ctrl+D"));

    ui->lineEditId->hide();
//    ui->menuBar->hide();
    ui->statusBar->hide();

    for (int i = 1950; i <= 2050; i++) {
        ui->comboBoxYear->addItem(QString("%1").arg(i));
    }
    for (int i = 1; i <= 12; i++) {
        ui->comboBoxMonth->addItem(QString("%1").arg(i));
    }

    connect(ui->tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(slotUpdateEditArea()));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotMenuRequested(QPoint)));
    connect(ui->tableView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotSort()));
    connect(ui->calendarWidget, SIGNAL(currentPageChanged(int,int)), this, SLOT(slotUpdateFormat()));
    connect(ui->pushNew, SIGNAL(clicked()), this, SLOT(slotNew()));
    connect(ui->pushSave, SIGNAL(clicked()), this, SLOT(slotSave()));
    connect(ui->pushDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
    connect(ui->comboBoxYear, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotUpdateDays()));
    connect(ui->comboBoxMonth, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotUpdateDays()));

    initTable();
    initTrayIcon();

    ui->tableView->hideColumn(0);
    ui->tableView->selectRow(0);
    slotUpdateEditArea();
    slotUpdateFormat();
    slotUpdateTrayTip();

    QTimer::singleShot(m_remindTime * 1000, this, SLOT(slotReminder()));

    ui->tableView->setFocus();
}

//------------------------------------------------------------------
//初始化托盘
//------------------------------------------------------------------
void MainWindow::initTrayIcon()
{
    //创建并设置动作
    minAction = new QAction("最小化", this);
    connect(minAction,SIGNAL(triggered()),this,SLOT(hide()));

    restoreAction = new QAction("还原", this);
    connect(restoreAction,SIGNAL(triggered()),this,SLOT(slotLook()));

    importAction = new QAction("导入", this);
    connect(importAction, SIGNAL(triggered()), this, SLOT(slotImport()));

    exportAction = new QAction("导出", this);
    connect(exportAction, SIGNAL(triggered()), this, SLOT(slotExport()));

    setAction = new QAction("设置", this);
    connect(setAction, SIGNAL(triggered()), this, SLOT(slotSet()));

    helpAction = new QAction("说明", this);
    connect(helpAction,SIGNAL(triggered()),this,SLOT(slotHelp()));

    quitAction = new QAction("退出", this);
    connect(quitAction,SIGNAL(triggered()),qApp,SLOT(quit()));

    //创建托盘图标菜单并添加动作
    trayIconMenu = new QMenu(); // 不设置parent，这样点击菜单外的任何位置，菜单会消失
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(minAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(importAction);
    trayIconMenu->addAction(exportAction);
    trayIconMenu->addAction(setAction);
    trayIconMenu->addAction(helpAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    //初始化窗口菜单
    ui->menuFile->addAction(importAction);
    ui->menuFile->addAction(exportAction);
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(setAction);
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(quitAction);
    ui->menuHelp->addAction(helpAction);

    //创建并设置托盘图标
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/pic/tray.png"));

    //显示系统托盘图标
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(slotTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

//------------------------------------------------------------------
//双击系统托盘时还原或最小化
//------------------------------------------------------------------
void MainWindow::slotTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::DoubleClick:
        if (isMinimized() || !isVisible())
            slotLook();
        else
            hide();
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------
//初始化提醒对话框
//------------------------------------------------------------------
void MainWindow::slotReminder()
{
    slotUpdateTrayTip();

    QRect screenRect;
    QPoint pos;

    dr = new DialogRemind;
    dr->init(trayIcon->toolTip(), m_quitTime);
    connect(dr, SIGNAL(signLook()), this, SLOT(slotLook()));
    //show要放在move之前，不然对话框边框的尺寸获取不到
    pos = QPoint(screenRect.width() - dr->frameSize().width(),
                 screenRect.height() - dr->frameSize().height());
    dr->move(pos);
    dr->show();
    screenRect = QApplication::desktop()->availableGeometry();
    pos = QPoint(screenRect.width() - dr->frameSize().width(),
                 screenRect.height() - dr->frameSize().height());
    dr->move(pos);
}

//------------------------------------------------------------------
//读取设置信息，并决定是否在关闭最后一个窗口时退出
//------------------------------------------------------------------
void MainWindow::slotReadSettings()
{
    QSettings setting(QApplication::applicationDirPath() + "/setting.ini", QSettings::IniFormat);
    bool isCloseMin;

    isCloseMin = setting.value("closemin", true).toBool();
    //决定关闭最后一个窗口时是否退出
    QApplication::setQuitOnLastWindowClosed(!isCloseMin);
    m_startMin = setting.value("startmin", true).toBool();
    m_quitTime = setting.value("quittime", 0).toInt();
    m_aheadDays = setting.value("aheaddays", 10).toInt();
    m_remindTime = setting.value("remindtime", 0).toInt();
    m_dataSep = setting.value("datasep", "|").toString();
    m_timeSep = setting.value("timesep", "-").toString();
}

//------------------------------------------------------------------
//更新日历显示和本月生日信息
//------------------------------------------------------------------
void MainWindow::slotUpdateFormat()
{
    QDate start;
    QDate end;
    QDate date;
    QString sql;
    QString info;
    QTextCharFormat format;

    date = QDate(ui->calendarWidget->yearShown(), ui->calendarWidget->monthShown(), 1);
    start = date.addDays(-7);
    end = date.addDays(42);
    //清空日历格式
    for (int i = 0; i < start.daysTo(end); i++) {
        QTextCharFormat cf;
        ui->calendarWidget->setDateTextFormat(start.addDays(i), cf);
    }
    //今天日期加下划线
    format = ui->calendarWidget->dateTextFormat(m_currentDate);
    format.setFontUnderline(true);
    ui->calendarWidget->setDateTextFormat(m_currentDate, format);
    //有生日的日期背景显示绿色
    format = QTextCharFormat();
    format.setBackground(Qt::green);
    sql = QString("select * from birthday "
                  "where date >= '%1' and date <= '%2' order by date")
             .arg(start.toString("yyyy-MM-dd"))
             .arg(end.toString("yyyy-MM-dd"));
    m_queryModel.setQuery(sql);
    if (m_queryModel.lastError().isValid()) {
        qDebug() << m_queryModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    info = QString("本月生日：\n");
    for (int i = 0; i < m_queryModel.rowCount(); i++) {
        QString toolTip;
        QString temp;

        m_record = m_queryModel.record(i);
        if (m_record.value("date").toDate().month() == ui->calendarWidget->monthShown()) {
            info += QString("%1:\t%2\n")
                .arg(m_record.value("name").toString())
                .arg(m_record.value("date").toString());
        }
        toolTip = QString("%1 生日 %2(%3)")
                .arg(m_record.value("name").toString())
                .arg(m_record.value("birthday").toString())
                .arg(m_record.value("type").toString());
        //calendar
        if (m_record.value("date").toDate() == m_currentDate) {
            format.setFontUnderline(true);
        } else {
            format.setFontUnderline(false);
        }
        temp = ui->calendarWidget->dateTextFormat(m_record.value("date").toDate()).toolTip();
        if (temp.isEmpty()) {
            format.setToolTip(toolTip);
        } else {
            format.setToolTip(temp + "\n" + toolTip);
        }
        ui->calendarWidget->setDateTextFormat(m_record.value("date").toDate(), format);
    }
    ui->labelInfo->setText(info);
}

//------------------------------------------------------------------
//更新编辑区
//------------------------------------------------------------------
void MainWindow::slotUpdateEditArea()
{
    int row;
    QDate birthday;

    row = ui->tableView->currentIndex().row();
    m_record = m_tableModel.record(row);
    ui->lineEditId->setText(m_record.value("id").toString());
    ui->lineEditName->setText(m_record.value("name").toString());
    ui->comboBoxType->setCurrentIndex(ui->comboBoxType->findText(m_record.value("type").toString()));
    birthday = m_record.value("birthday").toDate();
    ui->comboBoxYear->setCurrentIndex(birthday.year() - ui->comboBoxYear->itemText(0).toInt());
    ui->comboBoxMonth->setCurrentIndex(birthday.month() - 1);
    ui->comboBoxDay->setCurrentIndex(birthday.day() - 1);
    ui->dateEditDate->setDate(m_record.value("date").toDate());
}

//------------------------------------------------------------------
//初始化表格
//------------------------------------------------------------------
void MainWindow::initTable()
{
    //检查是否是新的一年，如果是，则更新数据库里的日期
    QString sql;

    sql = QString("select * from birthday order by date");
    m_queryModel.setQuery(sql);
    if (m_queryModel.lastError().isValid()) {
        qDebug() << m_queryModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    for (int i = 0; i < m_queryModel.rowCount(); i++) {
        m_record = m_queryModel.record(i);
        if (m_record.value("date").toDate().isValid())
            break;
    }
    //只需判断最早生日的年份是否是今年即可
    if (m_record.value("date").toDate().year() != m_currentDate.year()) {
        for (int i = 0; i < m_queryModel.rowCount(); i++) {
            QSqlRecord record;
            QDate birthday;
            QDate date;
            int id;

            record = m_queryModel.record(i);
            id = record.value("id").toInt();
            birthday = record.value("birthday").toDate();
            date = QDate(m_currentDate.year(), birthday.month(), birthday.day());
            if (record.value("type").toString() == "农历") {
                ChineseDate cd;
                SolarDate sd;

                cd = date;
                sd = cd.ToSolarDate();
                date.setDate(sd.GetYear(), sd.GetMonth(), sd.GetDay());
            }
            sql = QString("update birthday set date = '%1' where id = %2")
                    .arg(date.toString("yyyy-MM-dd"))
                    .arg(id);
            m_query.exec(sql);
            if (m_query.lastError().isValid()) {
                qDebug() << m_query.lastError() << __FILE__ << __LINE__;
                return;
            }
        }
    }

    m_tableModel.setTable("birthday");
    m_tableModel.setHeaderData(1, Qt::Horizontal, "姓名");
    m_tableModel.setHeaderData(2, Qt::Horizontal, "出生日期");
    m_tableModel.setHeaderData(3, Qt::Horizontal, "生日类型");
    m_tableModel.setHeaderData(4, Qt::Horizontal, "今年日期");
    m_tableModel.setFilter("");
    m_tableModel.sort(ui->tableView->horizontalHeader()->sortIndicatorSection(),
                    ui->tableView->horizontalHeader()->sortIndicatorOrder());
    m_tableModel.select();
    if (m_tableModel.lastError().isValid()) {
        qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    ui->tableView->setModel(&m_tableModel);
    ui->tableView->setFocus();
}

//------------------------------------------------------------------
//更新托盘提示信息
//------------------------------------------------------------------
void MainWindow::slotUpdateTrayTip()
{
    QDate startDate;
    QDate endDate;
    QString trayTip;
    QString sql;

    startDate = m_currentDate;
    endDate = startDate.addDays(m_aheadDays);
    sql = QString("select * from birthday where date >= '%1' and date <= '%2' order by date")
            .arg(startDate.toString("yyyy-MM-dd"))
            .arg(endDate.toString("yyyy-MM-dd"));
    m_queryModel.setQuery(sql);
    if (m_queryModel.lastError().isValid()) {
        qDebug() << m_queryModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    trayTip = "生日提醒\n";
    if (m_queryModel.rowCount() > 0) {
        for (int i = 0; i < m_queryModel.rowCount(); i++) {
            QDate date;
            QString remainDaysInfo;
            int remainDays;

            m_record = m_queryModel.record(i);
            date = m_record.value("date").toDate();
            remainDays = m_currentDate.daysTo(date);
            switch (remainDays) {
            case 0:
                remainDaysInfo = "今天";
                break;
            case 1:
                remainDaysInfo = "明天";
                break;
            case 2:
                remainDaysInfo = "后天";
                break;
            default:
                remainDaysInfo = QString("%1天后").arg(remainDays);
                break;
            }
            trayTip += QString("%1 %2<%3>生日\n")
                    .arg(m_record.value("name").toString())
                    .arg(m_record.value("date").toString())
                    .arg(remainDaysInfo);
        }
    } else {
        trayTip += QString("<%1天内无人生日！>\n").arg(m_aheadDays);
    }
    trayIcon->setToolTip(trayTip);
}

//------------------------------------------------------------------
//设置
//------------------------------------------------------------------
void MainWindow::slotSet()
{
    DialogSet *ds = new DialogSet(this);
    connect(ds, SIGNAL(signSaveSettings()), this, SLOT(slotReadSettings()));
    connect(ds, SIGNAL(signSaveSettings()), this, SLOT(slotUpdateTrayTip()));
    ds->exec();
}

//------------------------------------------------------------------
//保存人员
//------------------------------------------------------------------
void MainWindow::slotSave()
{
    QString sql;
    QDate birthday;
    QDate date;
    ChineseDate chday;
    SolarDate solarday;
    int row;

    if (ui->lineEditName->text().contains(m_dataSep) || ui->lineEditName->text().contains(m_timeSep)) {
        QMessageBox::information(this, "消息", "姓名不能包含分隔符！");
        return;
    }
    row = ui->tableView->currentIndex().row();
    birthday = QDate(ui->comboBoxYear->currentText().toInt(),
                     ui->comboBoxMonth->currentText().toInt(),
                     ui->comboBoxDay->currentText().toInt());
    date = QDate(m_currentDate.year(), birthday.month(), birthday.day());
    if (ui->comboBoxType->currentText() == "公历") {
        ui->dateEditDate->setDate(date);
    } else {
        chday = date;
        solarday = chday.ToSolarDate();
        ui->dateEditDate->setDate(QDate(solarday.GetYear(), solarday.GetMonth(), solarday.GetDay()));
    }
    sql = QString("update birthday set name = '%1', birthday = '%2', type = '%3',"
                  "date = '%4' where id = %5")
            .arg(ui->lineEditName->text())
            .arg(birthday.toString("yyyy-MM-dd"))
            .arg(ui->comboBoxType->currentText())
            .arg(ui->dateEditDate->date().toString("yyyy-MM-dd"))
            .arg(ui->lineEditId->text().toInt());
    m_query.exec(sql);
    if (m_query.lastError().isValid()) {
        qDebug() << m_query.lastError() << __FILE__ << __LINE__;
        return;
    }
    slotUpdateFormat();
    slotUpdateTrayTip();
    m_tableModel.select();
    if (m_tableModel.lastError().isValid()) {
        qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    for (int i = 0; i < m_tableModel.rowCount(); i++) {
        if (m_tableModel.record(i).value("id").toInt() == ui->lineEditId->text().toInt()) {
            ui->tableView->selectRow(i);
            break;
        }
    }
}

//------------------------------------------------------------------
//删除人员
//------------------------------------------------------------------
void MainWindow::slotDelete()
{
    QMessageBox msg;

    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msg.setText(QString("确定删除 %1?").arg(ui->lineEditName->text()));
    if (msg.exec() == QMessageBox::Yes) {
        int row;
        QString sql;

        row = ui->tableView->currentIndex().row();
        sql = QString("delete from birthday where id = %1").arg(ui->lineEditId->text().toInt());
        m_query.exec(sql);
        if (m_query.lastError().isValid()) {
            qDebug() << m_query.lastError() << __FILE__ << __LINE__;
            return;
        }
        m_tableModel.select();
        if (m_tableModel.lastError().isValid()) {
            qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
            return;
        }
        if (row < 1)
            row = 1;
        ui->tableView->selectRow(row - 1);
        slotUpdateEditArea();
        slotUpdateFormat();
        slotUpdateTrayTip();
    }
}

//------------------------------------------------------------------
//新建人员
//------------------------------------------------------------------
void MainWindow::slotNew()
{
    QString sql;
    int id;

    sql = QString("insert into birthday(name, birthday, type, date) "
                  "values('%1', '%2', '%3', '%4')")
            .arg("").arg(m_currentDate.toString("yyyy-MM-dd"))
            .arg("公历").arg(m_currentDate.toString("yyyy-MM-dd"));
    m_query.exec(sql);
    if (m_query.lastError().isValid()) {
        qDebug() << m_query.lastError() << __FILE__ << __LINE__;
        return;
    }
    m_queryModel.setQuery("select * from birthday");
    if (m_queryModel.lastError().isValid()) {
        qDebug() << m_queryModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    id = m_queryModel.record(m_queryModel.rowCount() - 1).value("id").toInt();
    m_tableModel.setFilter("");
    m_tableModel.select();
    if (m_tableModel.lastError().isValid()) {
        qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    for (int i = 0; i < m_tableModel.rowCount(); i++) {
        if (m_tableModel.record(i).value("id").toInt() == id) {
            ui->tableView->selectRow(i);
            break;
        }
    }
    slotUpdateEditArea();
    slotUpdateFormat();
    slotUpdateTrayTip();
    ui->lineEditName->setFocus();
}

//------------------------------------------------------------------
//点击日历响应
//------------------------------------------------------------------
void MainWindow::on_calendarWidget_clicked(QDate date)
{
    m_tableModel.setFilter(QString("date = '%1'").arg(date.toString("yyyy-MM-dd")));
    m_tableModel.select();
    if (m_tableModel.lastError().isValid()) {
        qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    if (m_tableModel.rowCount() == 0) {
        m_tableModel.setFilter("");
        m_tableModel.select();
        if (m_tableModel.lastError().isValid()) {
            qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
            return;
        }
    }
    ui->tableView->selectRow(0);
    slotUpdateEditArea();
}

//------------------------------------------------------------------
//右键菜单响应
//------------------------------------------------------------------
void MainWindow::slotMenuRequested(QPoint)
{
    tableViewMenu->exec(QCursor::pos());
}

//------------------------------------------------------------------
//更新编辑区出生日期中的天数
//------------------------------------------------------------------
void MainWindow::slotUpdateDays()
{
    int year;
    int month;
    int day;
    int days;

    year = ui->comboBoxYear->currentText().toInt();
    month = ui->comboBoxMonth->currentText().toInt();
    day = ui->comboBoxDay->currentText().toInt();
    days = QDate(year, month, 1).daysInMonth();
    if (ui->comboBoxDay->count() > days) {
        do {
            ui->comboBoxDay->removeItem(ui->comboBoxDay->count() - 1);
        } while (ui->comboBoxDay->count() > days);
    } else if (ui->comboBoxDay->count() < days) {
        do {
            ui->comboBoxDay->addItem(QString("%1").arg(ui->comboBoxDay->count() + 1));
        } while (ui->comboBoxDay->count() < days);
    }
    if (day > days)
        ui->comboBoxDay->setCurrentIndex(ui->comboBoxDay->count() - 1);
}

//------------------------------------------------------------------
//表格排序
//------------------------------------------------------------------
void MainWindow::slotSort()
{
    for (int i = 0; i < m_tableModel.rowCount(); i++) {
        if (m_tableModel.record(i).value("id").toInt() == ui->lineEditId->text().toInt()) {
            ui->tableView->selectRow(i);
            break;
        }
    }
}

//------------------------------------------------------------------
//初始化
//------------------------------------------------------------------
bool MainWindow::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);

        if (o == ui->pushNew) {
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
                slotNew();
        } else if (o == ui->pushSave || o == ui->lineEditName) {
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
                slotSave();
        } else if (o == ui->pushDelete) {
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
                slotDelete();
        }
    }

    return QMainWindow::eventFilter(o, e);
}

//------------------------------------------------------------------
//退出
//------------------------------------------------------------------
void MainWindow::slotQuit()
{
    QApplication::quit();
}

//------------------------------------------------------------------
//查看说明
//------------------------------------------------------------------
void MainWindow::slotHelp()
{
    QFile help(":/help.txt");

    if (!help.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "open file failed" << __FILE__ << __LINE__;
        return;
    }
    QMessageBox::about(NULL, "说明", help.readAll());
}

//------------------------------------------------------------------
//显示主窗口
//------------------------------------------------------------------
void MainWindow::slotLook()
{
    QRect screenRect;
    QPoint pos;

    screenRect = QApplication::desktop()->availableGeometry();
    pos = QPoint((screenRect.width() - frameSize().width()) / 2,
                 (screenRect.height() - frameSize().height()) / 2);
    move(pos);
    //设置2次窗口标志的原因：有时双击托盘图标后，窗口没有显示在最前面，结果看不到窗口
    //暂时的解决办法是：先让它置顶，再取消置顶
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    showNormal();
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    showNormal();
    ui->tableView->setFocus();
}

//------------------------------------------------------------------
//返回今天
//------------------------------------------------------------------
void MainWindow::on_pushButtonToday_clicked()
{
    ui->calendarWidget->setSelectedDate(m_currentDate);
}

//------------------------------------------------------------------
//导入数据
//------------------------------------------------------------------
void MainWindow::slotImport()
{
    QString fileName;
    QFile file;
    QString errorInfo;
    QString strLine;
    QTextStream textStream;
    int addFlag; //-1:初始值，0：重复时总不添加，1：重复时总添加
    int errorNum;
    int rightNum;
    int currentLine;

    addFlag = -1;
    errorNum = 0;
    rightNum = 0;
    currentLine = 0;

    fileName = QFileDialog::getOpenFileName(this, "打开",
                                            QApplication::applicationDirPath(),
                                            "Text files(*.txt);;"
                                            "All files (*.*)");
    if (fileName.isEmpty())
        return;
    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "信息", "打开文件失败！");
        return;
    }
    textStream.setCodec(QTextCodec::codecForName("gb18030"));   //支持中文
    textStream.setDevice(&file);
    if (!m_db.transaction())
        qDebug() << "transaction failed" << __FILE__ << __LINE__;
    while (!textStream.atEnd()) {
        QStringList dataList;
        QStringList birthList;
        QString name;
        QDate birthday;
        QDate date;
        QString type;
        QString sql;

        errorNum++;
        currentLine++;

        strLine = textStream.readLine();
        if (strLine.isEmpty()) {
            qDebug() << "empty line" << __FILE__ << __LINE__;
            errorNum--;
            continue;
        }
        if (!strLine.contains(m_dataSep)) {
            qDebug() << "error line" << __FILE__ << __LINE__;
            errorInfo += QString("第%1行：%2<没有数据分隔符>\n").arg(currentLine).arg(strLine);
            continue;
        } else if (!strLine.contains(m_timeSep)) {
            qDebug() << "error line" << __FILE__ << __LINE__;
            errorInfo += QString("第%1行：%2<没有日期分隔符>\n").arg(currentLine).arg(strLine);
            continue;
        }
        dataList = strLine.split(m_dataSep);
        name = dataList.at(0).trimmed();
        if (name.contains(m_dataSep) || name.contains(m_timeSep)) {
            qDebug() << "name has separating character" << __FILE__ << __LINE__;
            errorInfo += QString("第%1行：%2<姓名包含分隔符>\n").arg(currentLine).arg(strLine);
            continue;
        }
        birthList = dataList.at(1).split(m_timeSep);
        birthday = QDate(birthList.at(0).toInt(),
                         birthList.at(1).toInt(),
                         birthList.at(2).toInt());
        if (!birthday.isValid() || birthday < QDate(1950, 1, 1) || birthday >= QDate(2051, 1, 1)) {
            qDebug() << "birthday is not valid" << __FILE__ << __LINE__;
            errorInfo += QString("第%1行：%2<生日日期有误>\n").arg(currentLine).arg(strLine);
            continue;
        }
        type = dataList.at(2).trimmed();
        if (type == "公历") {
            date = QDate(m_currentDate.year(), birthday.month(), birthday.day());
        } else if (type == "农历") {
            ChineseDate cd;
            SolarDate sd;

            cd = ChineseDate(m_currentDate.year(), birthday.month(), birthday.day());
            sd = cd.ToSolarDate();
            date = QDate(sd.GetYear(), sd.GetMonth(), sd.GetDay());
        } else {
            qDebug() << "type is not valid" << __FILE__ << __LINE__;
            errorInfo += QString("第%1行：%2<生日类型有误>\n").arg(currentLine).arg(strLine);
            continue;
        }
        sql = QString("select * from birthday where name='%1'").arg(name);
        m_queryModel.setQuery(sql);
        if (m_queryModel.lastError().isValid()) {
            qDebug() << m_queryModel.lastError() << __FILE__ << __LINE__;
            goto ErrorHandle;
        }
        if (m_queryModel.rowCount() > 0) {
            if (addFlag == -1) {
                QMessageBox msg;

                msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No
                                       | QMessageBox::YesToAll | QMessageBox::NoToAll);
                msg.setText(QString("%1 已存在，是否添加？").arg(name));
                switch (msg.exec()) {
                case QMessageBox::Yes:
                    break;
                case QMessageBox::No:
                    errorInfo += QString("第%1行：%2<%3 已存在，不添加>\n").arg(currentLine).arg(strLine).arg(name);
                    continue;
                    break;
                case QMessageBox::YesToAll:
                    addFlag = 1;
                    break;
                case QMessageBox::NoToAll:
                    addFlag = 0;
                    errorInfo += QString("第%1行：%2<%3 已存在，不添加>\n").arg(currentLine).arg(strLine).arg(name);
                    continue;
                    break;
                default:
                    break;
                }
            } else if (addFlag == 0) {
                errorInfo += QString("第%1行：%2<%3 已存在，不添加>\n").arg(currentLine).arg(strLine).arg(name);
                continue;
            }
        }
        sql = QString("insert into birthday(name, birthday, type, date) values('%1', '%2', '%3', '%4')")
                .arg(name).arg(birthday.toString("yyyy-MM-dd"))
                .arg(type).arg(date.toString("yyyy-MM-dd"));
        m_query.exec(sql);
        if (m_query.lastError().isValid()) {
            qDebug() << m_query.lastError() << __FILE__ << __LINE__;
            goto ErrorHandle;
        }
        errorNum--;
        rightNum++;
    }
    if (!m_db.commit())
        qDebug() << "commit failed" << __FILE__ << __LINE__;

    if (errorNum > 0) {
        QMessageBox msg;
        msg.setText(QString("数据导入:%1成功，%2失败                                      ")
                    .arg(rightNum).arg(errorNum));
        msg.setDetailedText(errorInfo);
        msg.setWindowTitle("出错");
        msg.setIcon(QMessageBox::Critical);
        msg.exec();
    } else {
        QMessageBox::information(this, "信息", QString("数据导入:%1成功，%2失败")
                                 .arg(rightNum).arg(errorNum));
    }
    m_tableModel.setFilter("");
    m_tableModel.select();
    if (m_tableModel.lastError().isValid()) {
        qDebug() << m_tableModel.lastError() << __FILE__ << __LINE__;
    }
    ui->tableView->selectRow(0);
    slotUpdateFormat();
    slotUpdateEditArea();
    slotUpdateTrayTip();
    file.close();

    return;

ErrorHandle:
    if (!m_db.rollback()) {
        qDebug() << "rollback failed" << __FILE__ << __LINE__;
    }
}

//------------------------------------------------------------------
//导出数据
//------------------------------------------------------------------
void MainWindow::slotExport()
{
    QFile saveFile;
    QTextStream textStream;
    QString fileName;
    QString sql;

    fileName = QFileDialog::getSaveFileName(this, "另存为",
                                            QApplication::applicationDirPath() + "/export.txt",
                                            "Text files(*.txt);;"
                                            "All files(*)");
    if (fileName.isEmpty())
        return;
    saveFile.setFileName(fileName);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "open file failed" << __FILE__ << __LINE__;
        return;
    }
    textStream.setDevice(&saveFile);

    sql = QString("select * from birthday");
    m_queryModel.setQuery(sql);
    if (m_queryModel.lastError().isValid()) {
        qDebug() << m_queryModel.lastError() << __FILE__ << __LINE__;
        return;
    }
    for (int i = 0; i < m_queryModel.rowCount(); i++) {
        QString strLine;
        QString dateFormat;
        QDate birthday;

        m_record = m_queryModel.record(i);
        birthday = m_record.value("birthday").toDate();
        dateFormat = QString("yyyy%1MM%1dd").arg(m_timeSep);
        strLine = m_record.value("name").toString()
                + m_dataSep
                + birthday.toString(dateFormat)
                + m_dataSep
                + m_record.value("type").toString();
        textStream << strLine << "\n";
    }
    saveFile.close();
    QMessageBox::information(this, "信息", "导出成功！");
}

void MainWindow::setVisible(bool visible)
{
    minAction->setEnabled(visible);              //主窗口可见时，最小化有效
    restoreAction->setEnabled(!visible);         //主窗口不可见时，还原有效

    QMainWindow::setVisible(visible);                //调用基类函数
}
