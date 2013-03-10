#ifndef DIALOGSET_H
#define DIALOGSET_H

#include <QDialog>
#include <QtCore>
#include <QtGui>

namespace Ui {
    class DialogSet;
}

class DialogSet : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSet(QWidget *parent = 0);
    ~DialogSet();

private:
    void init();
    void readSettings();
    bool  writeSettings();

    QString m_appPath;
    QString m_settingPath;

signals:
    void signSaveSettings();

private slots:
    void on_pushConfirm_clicked();

private:
    Ui::DialogSet *ui;
};

#endif // DIALOGSET_H
