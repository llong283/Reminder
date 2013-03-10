#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    //��ʼ������״̬
    QRect screenRect;
    QPoint pos;
    screenRect = QApplication::desktop()->availableGeometry();
    pos = QPoint((screenRect.width() - frameSize().width()) / 2,
                 (screenRect.height() - frameSize().height()) / 2);
    move(pos);
    setWindowFlags(Qt::WindowMinimizeButtonHint);
    setFixedSize(size());
    setWindowTitle("��������");
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
//��ʼ��
//------------------------------------------------------------------
void MainWindow::init()
{
    //��ʼ������
    m_currentDate = QDate::currentDate();
    m_startMin = true;
    m_quitTime = 0;
    m_aheadDays = 10;
    m_remindTime = 0;
    dr = NULL;
    slotReadSettings();
    m_db = QSqlDatabase::database();

    //��ʼ���ؼ�
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
//��ʼ������
//------------------------------------------------------------------
void MainWindow::initTrayIcon()
{
    //���������ö���
    minAction = new QAction("��С��", this);
    connect(minAction,SIGNAL(triggered()),this,SLOT(hide()));

    restoreAction = new QAction("��ԭ", this);
    connect(restoreAction,SIGNAL(triggered()),this,SLOT(slotLook()));

    importAction = new QAction("����", this);
    connect(importAction, SIGNAL(triggered()), this, SLOT(slotImport()));

    exportAction = new QAction("����", this);
    connect(exportAction, SIGNAL(triggered()), this, SLOT(slotExport()));

    setAction = new QAction("����", this);
    connect(setAction, SIGNAL(triggered()), this, SLOT(slotSet()));

    helpAction = new QAction("˵��", this);
    connect(helpAction,SIGNAL(triggered()),this,SLOT(slotHelp()));

    quitAction = new QAction("�˳�", this);
    connect(quitAction,SIGNAL(triggered()),qApp,SLOT(quit()));

    //��������ͼ��˵�����Ӷ���
    trayIconMenu = new QMenu(); // ������parent����������˵�����κ�λ�ã��˵�����ʧ
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(minAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(importAction);
    trayIconMenu->addAction(exportAction);
    trayIconMenu->addAction(setAction);
    trayIconMenu->addAction(helpAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    //��ʼ�����ڲ˵�
    ui->menuFile->addAction(importAction);
    ui->menuFile->addAction(exportAction);
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(setAction);
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(quitAction);
    ui->menuHelp->addAction(helpAction);

    //��������������ͼ��
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/pic/tray.png"));

    //��ʾϵͳ����ͼ��
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(slotTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

//------------------------------------------------------------------
//˫��ϵͳ����ʱ��ԭ����С��
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
//��ʼ�����ѶԻ���
//------------------------------------------------------------------
void MainWindow::slotReminder()
{
    slotUpdateTrayTip();

    QRect screenRect;
    QPoint pos;

    dr = new DialogRemind;
    dr->init(trayIcon->toolTip(), m_quitTime);
    connect(dr, SIGNAL(signLook()), this, SLOT(slotLook()));
    //showҪ����move֮ǰ����Ȼ�Ի���߿�ĳߴ��ȡ����
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
//��ȡ������Ϣ���������Ƿ��ڹر����һ������ʱ�˳�
//------------------------------------------------------------------
void MainWindow::slotReadSettings()
{
    QSettings setting(QApplication::applicationDirPath() + "/setting.ini", QSettings::IniFormat);
    bool isCloseMin;

    isCloseMin = setting.value("closemin", true).toBool();
    //�����ر����һ������ʱ�Ƿ��˳�
    QApplication::setQuitOnLastWindowClosed(!isCloseMin);
    m_startMin = setting.value("startmin", true).toBool();
    m_quitTime = setting.value("quittime", 0).toInt();
    m_aheadDays = setting.value("aheaddays", 10).toInt();
    m_remindTime = setting.value("remindtime", 0).toInt();
    m_dataSep = setting.value("datasep", "|").toString();
    m_timeSep = setting.value("timesep", "-").toString();
}

//------------------------------------------------------------------
//����������ʾ�ͱ���������Ϣ
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
    //���������ʽ
    for (int i = 0; i < start.daysTo(end); i++) {
        QTextCharFormat cf;
        ui->calendarWidget->setDateTextFormat(start.addDays(i), cf);
    }
    //�������ڼ��»���
    format = ui->calendarWidget->dateTextFormat(m_currentDate);
    format.setFontUnderline(true);
    ui->calendarWidget->setDateTextFormat(m_currentDate, format);
    //�����յ����ڱ�����ʾ��ɫ
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
    info = QString("�������գ�\n");
    for (int i = 0; i < m_queryModel.rowCount(); i++) {
        QString toolTip;
        QString temp;

        m_record = m_queryModel.record(i);
        if (m_record.value("date").toDate().month() == ui->calendarWidget->monthShown()) {
            info += QString("%1:\t%2\n")
                .arg(m_record.value("name").toString())
                .arg(m_record.value("date").toString());
        }
        toolTip = QString("%1 ���� %2(%3)")
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
//���±༭��
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
//��ʼ�����
//------------------------------------------------------------------
void MainWindow::initTable()
{
    //����Ƿ����µ�һ�꣬����ǣ���������ݿ��������
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
    //ֻ���ж��������յ�����Ƿ��ǽ��꼴��
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
            if (record.value("type").toString() == "ũ��") {
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
    m_tableModel.setHeaderData(1, Qt::Horizontal, "����");
    m_tableModel.setHeaderData(2, Qt::Horizontal, "��������");
    m_tableModel.setHeaderData(3, Qt::Horizontal, "��������");
    m_tableModel.setHeaderData(4, Qt::Horizontal, "��������");
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
//����������ʾ��Ϣ
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
    trayTip = "��������\n";
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
                remainDaysInfo = "����";
                break;
            case 1:
                remainDaysInfo = "����";
                break;
            case 2:
                remainDaysInfo = "����";
                break;
            default:
                remainDaysInfo = QString("%1���").arg(remainDays);
                break;
            }
            trayTip += QString("%1 %2<%3>����\n")
                    .arg(m_record.value("name").toString())
                    .arg(m_record.value("date").toString())
                    .arg(remainDaysInfo);
        }
    } else {
        trayTip += QString("<%1�����������գ�>\n").arg(m_aheadDays);
    }
    trayIcon->setToolTip(trayTip);
}

//------------------------------------------------------------------
//����
//------------------------------------------------------------------
void MainWindow::slotSet()
{
    DialogSet *ds = new DialogSet(this);
    connect(ds, SIGNAL(signSaveSettings()), this, SLOT(slotReadSettings()));
    connect(ds, SIGNAL(signSaveSettings()), this, SLOT(slotUpdateTrayTip()));
    ds->exec();
}

//------------------------------------------------------------------
//������Ա
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
        QMessageBox::information(this, "��Ϣ", "�������ܰ����ָ�����");
        return;
    }
    row = ui->tableView->currentIndex().row();
    birthday = QDate(ui->comboBoxYear->currentText().toInt(),
                     ui->comboBoxMonth->currentText().toInt(),
                     ui->comboBoxDay->currentText().toInt());
    date = QDate(m_currentDate.year(), birthday.month(), birthday.day());
    if (ui->comboBoxType->currentText() == "����") {
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
//ɾ����Ա
//------------------------------------------------------------------
void MainWindow::slotDelete()
{
    QMessageBox msg;

    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msg.setText(QString("ȷ��ɾ�� %1?").arg(ui->lineEditName->text()));
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
//�½���Ա
//------------------------------------------------------------------
void MainWindow::slotNew()
{
    QString sql;
    int id;

    sql = QString("insert into birthday(name, birthday, type, date) "
                  "values('%1', '%2', '%3', '%4')")
            .arg("").arg(m_currentDate.toString("yyyy-MM-dd"))
            .arg("����").arg(m_currentDate.toString("yyyy-MM-dd"));
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
//���������Ӧ
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
//�Ҽ��˵���Ӧ
//------------------------------------------------------------------
void MainWindow::slotMenuRequested(QPoint)
{
    tableViewMenu->exec(QCursor::pos());
}

//------------------------------------------------------------------
//���±༭�����������е�����
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
//�������
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
//��ʼ��
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
//�˳�
//------------------------------------------------------------------
void MainWindow::slotQuit()
{
    QApplication::quit();
}

//------------------------------------------------------------------
//�鿴˵��
//------------------------------------------------------------------
void MainWindow::slotHelp()
{
    QFile help(":/help.txt");

    if (!help.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "open file failed" << __FILE__ << __LINE__;
        return;
    }
    QMessageBox::about(NULL, "˵��", help.readAll());
}

//------------------------------------------------------------------
//��ʾ������
//------------------------------------------------------------------
void MainWindow::slotLook()
{
    QRect screenRect;
    QPoint pos;

    screenRect = QApplication::desktop()->availableGeometry();
    pos = QPoint((screenRect.width() - frameSize().width()) / 2,
                 (screenRect.height() - frameSize().height()) / 2);
    move(pos);
    //����2�δ��ڱ�־��ԭ����ʱ˫������ͼ��󣬴���û����ʾ����ǰ�棬�������������
    //��ʱ�Ľ���취�ǣ��������ö�����ȡ���ö�
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    showNormal();
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    showNormal();
    ui->tableView->setFocus();
}

//------------------------------------------------------------------
//���ؽ���
//------------------------------------------------------------------
void MainWindow::on_pushButtonToday_clicked()
{
    ui->calendarWidget->setSelectedDate(m_currentDate);
}

//------------------------------------------------------------------
//��������
//------------------------------------------------------------------
void MainWindow::slotImport()
{
    QString fileName;
    QFile file;
    QString errorInfo;
    QString strLine;
    QTextStream textStream;
    int addFlag; //-1:��ʼֵ��0���ظ�ʱ�ܲ���ӣ�1���ظ�ʱ�����
    int errorNum;
    int rightNum;
    int currentLine;

    addFlag = -1;
    errorNum = 0;
    rightNum = 0;
    currentLine = 0;

    fileName = QFileDialog::getOpenFileName(this, "��",
                                            QApplication::applicationDirPath(),
                                            "Text files(*.txt);;"
                                            "All files (*.*)");
    if (fileName.isEmpty())
        return;
    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "��Ϣ", "���ļ�ʧ�ܣ�");
        return;
    }
    textStream.setCodec(QTextCodec::codecForName("gb18030"));   //֧������
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
            errorInfo += QString("��%1�У�%2<û�����ݷָ���>\n").arg(currentLine).arg(strLine);
            continue;
        } else if (!strLine.contains(m_timeSep)) {
            qDebug() << "error line" << __FILE__ << __LINE__;
            errorInfo += QString("��%1�У�%2<û�����ڷָ���>\n").arg(currentLine).arg(strLine);
            continue;
        }
        dataList = strLine.split(m_dataSep);
        name = dataList.at(0).trimmed();
        if (name.contains(m_dataSep) || name.contains(m_timeSep)) {
            qDebug() << "name has separating character" << __FILE__ << __LINE__;
            errorInfo += QString("��%1�У�%2<���������ָ���>\n").arg(currentLine).arg(strLine);
            continue;
        }
        birthList = dataList.at(1).split(m_timeSep);
        birthday = QDate(birthList.at(0).toInt(),
                         birthList.at(1).toInt(),
                         birthList.at(2).toInt());
        if (!birthday.isValid() || birthday < QDate(1950, 1, 1) || birthday >= QDate(2051, 1, 1)) {
            qDebug() << "birthday is not valid" << __FILE__ << __LINE__;
            errorInfo += QString("��%1�У�%2<������������>\n").arg(currentLine).arg(strLine);
            continue;
        }
        type = dataList.at(2).trimmed();
        if (type == "����") {
            date = QDate(m_currentDate.year(), birthday.month(), birthday.day());
        } else if (type == "ũ��") {
            ChineseDate cd;
            SolarDate sd;

            cd = ChineseDate(m_currentDate.year(), birthday.month(), birthday.day());
            sd = cd.ToSolarDate();
            date = QDate(sd.GetYear(), sd.GetMonth(), sd.GetDay());
        } else {
            qDebug() << "type is not valid" << __FILE__ << __LINE__;
            errorInfo += QString("��%1�У�%2<������������>\n").arg(currentLine).arg(strLine);
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
                msg.setText(QString("%1 �Ѵ��ڣ��Ƿ���ӣ�").arg(name));
                switch (msg.exec()) {
                case QMessageBox::Yes:
                    break;
                case QMessageBox::No:
                    errorInfo += QString("��%1�У�%2<%3 �Ѵ��ڣ������>\n").arg(currentLine).arg(strLine).arg(name);
                    continue;
                    break;
                case QMessageBox::YesToAll:
                    addFlag = 1;
                    break;
                case QMessageBox::NoToAll:
                    addFlag = 0;
                    errorInfo += QString("��%1�У�%2<%3 �Ѵ��ڣ������>\n").arg(currentLine).arg(strLine).arg(name);
                    continue;
                    break;
                default:
                    break;
                }
            } else if (addFlag == 0) {
                errorInfo += QString("��%1�У�%2<%3 �Ѵ��ڣ������>\n").arg(currentLine).arg(strLine).arg(name);
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
        msg.setText(QString("���ݵ���:%1�ɹ���%2ʧ��                                      ")
                    .arg(rightNum).arg(errorNum));
        msg.setDetailedText(errorInfo);
        msg.setWindowTitle("����");
        msg.setIcon(QMessageBox::Critical);
        msg.exec();
    } else {
        QMessageBox::information(this, "��Ϣ", QString("���ݵ���:%1�ɹ���%2ʧ��")
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
//��������
//------------------------------------------------------------------
void MainWindow::slotExport()
{
    QFile saveFile;
    QTextStream textStream;
    QString fileName;
    QString sql;

    fileName = QFileDialog::getSaveFileName(this, "���Ϊ",
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
    QMessageBox::information(this, "��Ϣ", "�����ɹ���");
}

void MainWindow::setVisible(bool visible)
{
    minAction->setEnabled(visible);              //�����ڿɼ�ʱ����С����Ч
    restoreAction->setEnabled(!visible);         //�����ڲ��ɼ�ʱ����ԭ��Ч

    QMainWindow::setVisible(visible);                //���û��ຯ��
}
