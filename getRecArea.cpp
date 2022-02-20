#include "getRecArea.h"
#include <QDialog>
#include <QRubberBand>
#include <QMouseEvent>
#include <ui_getRecArea.h>

GetRecArea::GetRecArea(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog) {
    this->setWindowOpacity(0.5);
    qrb = new QRubberBand(QRubberBand::Rectangle, this);
    this->setCursor(Qt::CrossCursor);
    ui->setupUi(this);
}

GetRecArea::~GetRecArea() {
    delete ui;
}

void GetRecArea::mousePressEvent(QMouseEvent *event) {
    origin = event->pos();
    qrb->setGeometry(QRect(origin, QSize()));
    qrb->show();
}

void GetRecArea::mouseMoveEvent(QMouseEvent *event) {
    end = event->pos();
    qrb->setGeometry(QRect(origin, event->pos()).normalized());
}

void GetRecArea::mouseReleaseEvent(QMouseEvent *event) {
    qrb->hide();
    emit sendArea(origin, end);
    close();
}

