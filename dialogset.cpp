#include "dialogset.h"
#include "ui_dialogset.h"

DialogSet::DialogSet(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSet)
{
    ui->setupUi(this);

    init();
}

DialogSet::~DialogSet()
{
    delete ui;
}

void DialogSet::init()
{
    //init variables
    m_appPath = QApplication::applicationFilePath();
    m_settingPath = QApplication::applicationDirPath() + "/setting.ini";

    //init widgets
    QRect screenRect;
    QPoint pos;
    screenRect = QApplication::desktop()->availableGeometry();
    pos = QPoint((screenRect.width() - frameSize().width()) / 2,
                 (screenRect.height() - frameSize().height()) / 2);
    move(pos);
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::Dialog);
    setWindowTitle(tr("set"));
    readSettings();

    ui->lineEditAheadDays->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->lineEditQuitTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->lineEditRemindTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    connect(ui->pushCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void DialogSet::readSettings()
{
#if defined(Q_WS_WIN)
    QSettings settingAutoRun("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\"
                       "CurrentVersion\\Run\\", QSettings::NativeFormat);
    if (settingAutoRun.value("Reminder", "rem \"" + m_appPath + "\"").toString() == "\"" + m_appPath + "\"")
        ui->checkBoxAutoRun->setChecked(true);
    else
        ui->checkBoxAutoRun->setChecked(false);
#endif
    QSettings setting(m_settingPath, QSettings::IniFormat);

    ui->checkBoxStartMin->setChecked(setting.value("startmin", true).toBool());
    ui->checkBoxCloseMin->setChecked(setting.value("closemin", true).toBool());
    ui->lineEditQuitTime->setText(setting.value("quittime", 0).toString());
    ui->lineEditAheadDays->setText(setting.value("aheaddays", 10).toString());
    ui->lineEditRemindTime->setText(setting.value("remindtime", 0).toString());

    ui->lineEditDataSep->setText(setting.value("datasep", "|").toString());
    ui->lineEditTimeSep->setText(setting.value("timesep", "-").toString());
}

bool DialogSet::writeSettings()
{
    if (ui->lineEditDataSep->text() == ui->lineEditTimeSep->text()) {
        QMessageBox::information(this, tr("msg"), tr("数据分隔符和时间分隔符不能相同！"));
        return false;
    }
#if defined(Q_WS_WIN)
    QSettings settingAutoRun("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\"
                       "CurrentVersion\\Run\\", QSettings::NativeFormat);
    if (ui->checkBoxAutoRun->checkState() == Qt::Checked)
        settingAutoRun.setValue("Reminder", "\"" + m_appPath + "\"");
    else
        settingAutoRun.setValue("Reminder", "rem \"" + m_appPath + "\"");
#endif
    QSettings setting(m_settingPath, QSettings::IniFormat);

    if (ui->checkBoxStartMin->checkState() == Qt::Checked)
        setting.setValue("startmin", true);
    else
        setting.setValue("startmin", false);
    if (ui->checkBoxCloseMin->checkState() == Qt::Checked)
        setting.setValue("closemin", true);
    else
        setting.setValue("closemin", false);
    setting.setValue("quittime", ui->lineEditQuitTime->text());
    setting.setValue("aheaddays", ui->lineEditAheadDays->text());
    setting.setValue("remindtime", ui->lineEditRemindTime->text());

    setting.setValue("datasep", ui->lineEditDataSep->text());
    setting.setValue("timesep", ui->lineEditTimeSep->text());
    return true;
}

void DialogSet::on_pushConfirm_clicked()
{
    if (!writeSettings())
        return;
    emit signSaveSettings();
    close();
}
