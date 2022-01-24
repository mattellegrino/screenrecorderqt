#ifndef GETRECAREA_H
#define GETRECAREA_H

#include <QDialog>
#include <QRubberBand>
#include <QMouseEvent>

namespace Ui {
class Dialog;
}

class GetRecArea : public QDialog {
    Q_OBJECT

public:
    explicit GetRecArea(QWidget *parent = nullptr);
    ~GetRecArea();

private:
    Ui::Dialog *ui;
    QRubberBand* qrb{};
    QPoint origin;
    QPoint end;

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void sendArea(QPoint origin, QPoint end);

};

#endif // GETRECAREA_H
