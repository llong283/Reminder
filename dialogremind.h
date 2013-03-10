#ifndef DIALOGREMIND_H
#define DIALOGREMIND_H

#include <QDialog>
#include <QtCore>

namespace Ui {
    class DialogRemind;
}

class DialogRemind : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRemind(QWidget *parent = 0);
    ~DialogRemind();

    //methods
    void init(QString info, int quitTime);

private:
    Ui::DialogRemind *ui;

    //members
    QTimer *m_timer;

signals:
    void signLook();

private slots:
    void slotUpdateRemainTime();
    void on_pushButtonLook_clicked();
    void on_pushButtonCancelQuit_clicked();
};

#endif // DIALOGREMIND_H
