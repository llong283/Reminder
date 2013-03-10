#include "dialogremind.h"
#include "ui_dialogremind.h"

DialogRemind::DialogRemind(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRemind)
{
    ui->setupUi(this);
}

DialogRemind::~DialogRemind()
{
    delete ui;
}

void DialogRemind::init(QString info, int quitTime)
{
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint | Qt::Dialog);
    setWindowTitle("Ьсаб");

    ui->labelRemindInfo->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->labelRemindInfo->setText(info);
    if (quitTime == 0) {
        ui->labelRemainTime->hide();
        ui->labelRemainInfo->hide();
        ui->pushButtonCancelQuit->hide();
    } else {
        ui->labelRemainTime->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->labelRemainTime->setText(QString("%1").arg(quitTime));

        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateRemainTime()));
        m_timer->start();
    }

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(close()));
}

void DialogRemind::slotUpdateRemainTime()
{
    int remainTime;

    remainTime = ui->labelRemainTime->text().toInt();
    if (remainTime > 0) {
        ui->labelRemainTime->setText(QString("%1").arg(remainTime - 1));
    } else {
        QApplication::quit();
    }
}

void DialogRemind::on_pushButtonLook_clicked()
{
    emit signLook();
    close();
}

void DialogRemind::on_pushButtonCancelQuit_clicked()
{
    m_timer->stop();
    ui->labelRemainInfo->hide();
    ui->labelRemainTime->hide();
    ui->pushButtonCancelQuit->hide();
}
