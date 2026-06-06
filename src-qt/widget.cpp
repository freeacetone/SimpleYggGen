#include "widget.h"
#include "ui_widget.h"
#include "configure.h"
#include "miner.h"

#include <iomanip>
#include <thread>
#include <sstream>
#include <iostream>
#include <QString>
#include <QAction>
#include <QDir>
#include <QTimer>
#include <QClipboard>

Widget::Widget(QWidget *parent): QWidget(parent), ui(new Ui::Widget), m_tpool(new QThreadPool)
{
    ui->setupUi(this);

    const unsigned int processor_count = std::thread::hardware_concurrency();
    ui->threads->setMaximum(processor_count);
    ui->threads->setValue(processor_count);

    auto actionShotcuts = new QAction(this);
    actionShotcuts->setShortcuts({ Qt::Key_Return, Qt::Key_Enter });
    this->addAction(actionShotcuts);
    connect(actionShotcuts, &QAction::triggered, this, [&](){ ui->action->animateClick(); });

    QObject::connect(ui->height, SIGNAL(valueChanged(int)), this, SLOT(secondByteEdit(int)));
    QObject::connect(ui->action, SIGNAL(clicked()), this, SLOT(action()));

    QObject::connect(ui->ipv6_pat_mode,      SIGNAL(clicked()), this, SLOT(ipv6_pat_mode()));
    QObject::connect(ui->high_mode,          SIGNAL(clicked()), this, SLOT(high_mode()));
    QObject::connect(ui->ipv6_pat_high_mode, SIGNAL(clicked()), this, SLOT(ipv6_pat_high_mode()));
    QObject::connect(ui->ipv6_reg_mode,      SIGNAL(clicked()), this, SLOT(ipv6_reg_mode()));
    QObject::connect(ui->ipv6_reg_high_mode, SIGNAL(clicked()), this, SLOT(ipv6_reg_high_mode()));
    QObject::connect(ui->mesh_pat_mode,      SIGNAL(clicked()), this, SLOT(mesh_pat_mode()));
    QObject::connect(ui->mesh_reg_mode,      SIGNAL(clicked()), this, SLOT(mesh_reg_mode()));

    connect (ui->xmrCopyAddressButton, &QPushButton::clicked, this, [&](){
        QApplication::clipboard()->setText(
            "84RsgSFSrJ2iKhd3XW6cYHdH6hVfLKYwBfUj6BNWqbVzhtCJz7GzaFe8jnToG1NGmPh7pwABdvBiCivTZRE2KfphPFXiyNN"
            );
        ui->xmrCopyAddressButton->setText("Copied!");

        QTimer* timer = new QTimer;
        timer->setSingleShot(true);
        timer->setInterval(1500);
        connect (timer, &QTimer::timeout, this, [&, timer](){
            ui->xmrCopyAddressButton->setText("Copy");
            timer->deleteLater();
        });
        timer->start();
    });

    ui->path->setText(QDir::currentPath());
    this->setFixedSize(this->size());
}

void Widget::setLog(const QString tm, const quint64 tt, const quint64 f, const quint64 k)
{
    if (k > m_speedRecord)
    {
        m_speedRecord = k;
        QString hs = "Maximum speed: " + numToReadableString(m_speedRecord) + " kH/s";
        ui->hs->setText(hs);
    }
    ui->time->setText(tm);                       // время
    ui->found->setText(numToReadableString(f));  // колесо фартуны
    ui->khs->setText(numToReadableString(k));    // скорость
    ui->total->setText(numToReadableString(tt)); // общий счетчик
}

void Widget::setAddr(const QString address)
{
    ui->last->setText(address);
}

Widget::~Widget()
{
    if (not conf.stop)
    {
        conf.stop = true;
        m_tpool->waitForDone(100);
    }

    delete ui;
    if (m_tpool)
    {
        m_tpool->clear();
        m_tpool->deleteLater();
    }
}

QString Widget::numToReadableString(const quint64 num)
{
    QString string = QString::number(num);
    for (int i = string.size()-1, counter = 0; i >= 0; --i)
    {
        if (++counter % 3 == 0 && i != 0) string.insert(i, ' ');
    }
    return string;
}

void Widget::secondByteEdit(int i)
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::hex << i ;
    ui->secondByte->setText("2<b>" + QString::fromStdString(ss.str()) + "</b>:");
}

void Widget::ipv6_pat_mode()
{
    m_mode = 0;
    string_status(true);
    altitude_status(false);
}

void Widget::high_mode()
{
    m_mode = 1;
    altitude_status(true);
    string_status(false);
}

void Widget::ipv6_pat_high_mode()
{
    m_mode = 2;
    altitude_status(true);
    string_status(true);
}

void Widget::ipv6_reg_mode()
{
    m_mode = 3;
    string_status(true);
    altitude_status(false);
}

void Widget::ipv6_reg_high_mode()
{
    m_mode = 4;
    altitude_status(true);
    string_status(true);
}

void Widget::mesh_pat_mode()
{
    m_mode = 5;
    string_status(true);
    altitude_status(false);
}

void Widget::mesh_reg_mode()
{
    m_mode = 6;
    string_status(true);
    altitude_status(false);
}

void Widget::altitude_status(bool b)
{
    ui->x_startaltitude->setEnabled(b);
    ui->secondByte->setEnabled(b);
    ui->height->setEnabled(b);
    ui->disableIncrease->setEnabled(b);
}

void Widget::string_status(bool b)
{
    ui->stringSet->setEnabled(b);
}

void Widget::action()
{
    if (m_isStarted)
    {
        conf.stop = true;
        m_isStarted = false;
        ui->khs->setNum(0);
        ui->stackedWidget->setCurrentIndex(0);
        ui->action->setText("START");
        return;
    }

    if (ui->stringSet->text() == "" and m_mode != 1)
    {
        ui->stringSet->setPlaceholderText("PATTERN/REGEXP");
        ui->stringSet->setStyleSheet("background-color: gray");

        QTimer* timer = new QTimer;
        timer->setSingleShot(true);
        timer->setInterval(1000);
        connect (timer, &QTimer::timeout, this, [&, timer](){
            ui->stringSet->setStyleSheet("");
            timer->deleteLater();
        });
        timer->start();
        return;
    }

    ui->stackedWidget->setCurrentIndex(1);
    ui->last->setText("<last address will be here>");
    ui->hs->setText("Maximum speed: 0 kH/s");
    setLog("00:00:00:00", 0, 0, 0);
    m_speedRecord = 0;

    conf.mode   = m_mode;
    conf.proc   = ui->threads->value();
    conf.high   = ui->height->value();
    conf.str    = ui->stringSet->text();
    conf.letsup = !ui->disableIncrease->isChecked();
    conf.stop   = false;

    m_isStarted = true;
    ui->action->setText("STOP");

    Miner::dropCounters();

    for (unsigned i = 0; i < conf.proc; i++)
    {
        auto miner = new Miner(this);
        miner->setAutoDelete(true);
        m_tpool->start(miner);
    }
}
