#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->screen = QGuiApplication::screens()[0];
    QRect rect = screen->geometry();
    this->height = rect.height();
    this->width = rect.width();
    this->resolution = std::to_string(width) + "x" + std::to_string(height);
    this->custom = false;
    this->micRec = false;
    this->filename = "../output.mp4";
    recorder = new ScreenRecorder();
    ui->setupUi(this);
    ui->checkBox_2->setChecked(true);
    ui->label_4->setVisible(false);
    ui->label_4->setStyleSheet("QLabel {color: red;}");
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_pushButton_clicked() {
    GetRecArea* d = new GetRecArea(this);
    QObject::connect(d, &GetRecArea::sendArea, this, &MainWindow::getArea);
    d->showFullScreen();
}

void MainWindow::getArea(QPoint origin, QPoint end) {
    this->origin = origin;
    this->end = end;
    std::cout << origin.x() << ", " << origin.y() << std::endl;
    std::cout << end.x() << ", " << end.y() << std::endl;
    this->cresolution = std::to_string(end.x() - origin.x()) + "x" + std::to_string(end.y() - origin.y());
}

void MainWindow::on_checkBox_2_stateChanged(int arg1) {
    if (!ui->checkBox_2->isChecked()) {
        ui->checkBox_3->setChecked(true);
        ui->pushButton->setEnabled(true);
        this->custom = true;
    }
    else if (ui->checkBox_2->isChecked()) {
        ui->checkBox_3->setChecked(false);
        ui->pushButton->setEnabled(false);
        this->custom = false;
    }
}

void MainWindow::on_checkBox_3_stateChanged(int arg1) {
    if (!ui->checkBox_3->isChecked()) {
        ui->checkBox_2->setChecked(true);
        ui->pushButton->setEnabled(false);
        this->custom = false;
    }
    else if (ui->checkBox_3->isChecked()) {
        ui->checkBox_2->setChecked(false);
        ui->pushButton->setEnabled(true);
        this->custom = true;
    }
}

void MainWindow::on_checkBox_stateChanged(int arg1) {
    if (ui->checkBox->isChecked())
        this->micRec = true;
    else this->micRec = false;
}

void MainWindow::on_pushButton_2_clicked() {
    std::cout << "Recording with: " << std::endl;
    std::cout << "Mic: " << this->micRec << std::endl;
    std::cout << "Area rec: ";
    if (this->custom) {
        std::cout << "custom with [" << origin.x() << ", " << origin.y() << "]";
        std::cout << "[" << end.x() << ", " << end.y() << "]" << std::endl;
    } else std::cout << "full screen " << "(" << width << "x" << height << ")" << std::endl;
    std::cout << "output file: " << filename << std::endl;
    if (custom) {
        if (recorder->start(filename, micRec, origin.x(), origin.y(), cresolution) < 0) {
            ui->label_4->setText(QString::fromStdString(recorder->getError()));
            ui->label_4->setVisible(true);
            return;
        }
    } else {
        if (recorder->start(filename, micRec, 0, 0, resolution) < 0) {
            ui->label_4->setText(QString::fromStdString(recorder->getError()));
            ui->label_4->setVisible(true);
            return;
        }
    }
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_5->setEnabled(true);
    ui->pushButton_3->setEnabled(true);
    ui->label_4->setVisible(false);
}

void MainWindow::on_pushButton_6_clicked() {
    QString fn = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath(), tr("Videos (*.mp4)"));
    this->filename = fn.toStdString();
    if (!hasEnding(this->filename, ".mp4"))
        this->filename += ".mp4";
}

bool MainWindow::hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void MainWindow::on_pushButton_5_clicked() {
    if (recorder->stop() < 0) {
        ui->label_4->setText(QString::fromStdString(recorder->getError()));
        ui->label_4->setVisible(true);
        return;
    }
    ui->pushButton_5->setEnabled(false);
    ui->pushButton_3->setEnabled(false);
    ui->pushButton_4->setEnabled(false);
    ui->pushButton_2->setEnabled(true);

    if (recorder->checkEncodeError()) {
        ui->label_4->setText("An error occured during encoding. Output file may be corrupted.");
        ui->label_4->setVisible(true);
        return;
    }
}

void MainWindow::on_pushButton_3_clicked() {
    if (recorder->pause() < 0) {
        ui->label_4->setText(QString::fromStdString(recorder->getError()));
        ui->label_4->setVisible(true);
        return;
    }
    ui->pushButton_3->setEnabled(false);
    ui->pushButton_4->setEnabled(true);
}

void MainWindow::on_pushButton_4_clicked() {
    if (recorder->resume() < 0) {
        ui->label_4->setText(QString::fromStdString(recorder->getError()));
        ui->label_4->setVisible(true);
        return;
    }
    ui->pushButton_3->setEnabled(true);
    ui->pushButton_4->setEnabled(false);
}
