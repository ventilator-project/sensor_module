#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPortInfo>
#include <QDebug>

#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    vfbChart.setTitle("VFB");
    vfbChart.addSeries(&vfbData);
    vfbChart.createDefaultAxes();
    vfbChart.axes().at(0)->setTitleText("Sample");
    vfbChart.axes().at(1)->setTitleText("Counts");
    vfbChart.legend()->hide();

    vfbChart.axes().at(0)->setRange(0,maxcounts);

    vfbData.setUseOpenGL(true);

    ui->vfbChartView->setChart(&vfbChart);
    ui->vfbChartView->setRenderHint(QPainter::Antialiasing);


    temperatureChart.setTitle("Temperature");
    temperatureChart.addSeries(&temperatureData);
    temperatureChart.createDefaultAxes();
    temperatureChart.axes().at(0)->setTitleText("Sample");
    temperatureChart.axes().at(1)->setTitleText("Counts");
    temperatureChart.legend()->hide();

    temperatureChart.axes().at(0)->setRange(0,maxcounts);

    temperatureData.setUseOpenGL(true);

    ui->temperatureChartView->setChart(&temperatureChart);
    ui->temperatureChartView->setRenderHint(QPainter::Antialiasing);


    pressureChart.setTitle("Pressure");
    pressureChart.addSeries(&pressureData);
    pressureChart.createDefaultAxes();
    pressureChart.axes().at(0)->setTitleText("Sample");
    pressureChart.axes().at(1)->setTitleText("Counts");
    pressureChart.legend()->hide();

    pressureChart.axes().at(0)->setRange(0,maxcounts);

    pressureData.setUseOpenGL(true);

    ui->pressureChartView->setChart(&pressureChart);
    ui->pressureChartView->setRenderHint(QPainter::Antialiasing);



    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);

    connect(&serial, &QSerialPort::readyRead,
            this, &MainWindow::readyRead);

    connect(&serial, &QSerialPort::errorOccurred,
            [=] (QSerialPort::SerialPortError error) {
        qDebug() << error;
        ui->connectButton->setText("Connect");
    }
    );

    connect(&portScanner, &QTimer::timeout,
            [=] () {
        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

        // add any new ports
        for(const auto &port : ports)
            if(ui->portsComboBox->findText(port.portName()) == -1)
                ui->portsComboBox->addItem(port.portName());

        // remove any dead ports
        for(int i = ui->portsComboBox->count() -1 ; i >= 0; i--) {
            bool matched = false;
            for(const auto &port : ports)
                if(port.portName() == ui->portsComboBox->itemText(i))
                    matched = true;
            if(!matched)
                ui->portsComboBox->removeItem(i);
        }

    });

    portScanner.start(1000);
}

void MainWindow::recordPoint(double point, QLineSeries &series, QChart &chart)
{
//    const int average_count = 5;

//    static int count = 0;
//    static float val = 0;

//    val += point;
//    count++;

//    if(count < average_count)
//        return;

//    point = val/average_count;
//    val = 0;
//    count = 0;

    // Record the point
    if(series.count() < maxcounts) {
        series.append(series.count(), point);
    }
    else {
        QList<QPointF> points = series.points();
        points.removeFirst();
        for(QPointF &point : points)
            point.setX(point.x()-1);
        points.append(QPointF(series.count(), point));
        series.replace(points);
    }

    qreal yMax = std::numeric_limits<qreal>::min();
    qreal yMin = std::numeric_limits<qreal>::max();

    for(const auto &point : series.points()) {
        yMin = fmin(yMin, point.y());
        yMax = fmax(yMax, point.y());
    }

    chart.axes().at(1)->setRange(floor(yMin/10)*10,ceil(yMax/10)*10);
//    dataChart.axes().at(1)->setRange(0,((1<<11)-1));
}

void MainWindow::handleData(char data)
{
    static QByteArray packet_buffer;

    switch(packet_buffer.length()) {
        case 0: // SOF 1
            if(data == 0x55)
                packet_buffer.append(data);
            else {
                qDebug() << "SOF1 error";
                packet_buffer.clear();
            }
        break;
    case 1: // SOF 2
        if(data == 0x55)
            packet_buffer.append(data);
        else {
            qDebug() << "SOF2 error";
            packet_buffer.clear();
        }
        break;
    case 2: // version
        if(data == 0)
            packet_buffer.append(data);
        else {
            qDebug() << "Version error";
            packet_buffer.clear();
        }
        break;
    case 3: // packet type
        packet_buffer.append(data);
        break;
    default:
        packet_buffer.append(data);

        // Note: Packet type set in packet[3], but we only handle one type.
        if((packet_buffer.length()) == sizeof(message_packet_t)) {
            message_packet_t *packet = (message_packet_t*)packet_buffer.data();

            // TODO: CRC

            const double temperature = packet->temperature / 100.0; // temperature is in 1/100s of a C
            const double pressure = packet->pressure / 100.0; // pressure is pascals

            recordPoint(packet->vfb, vfbData ,vfbChart);
            recordPoint(temperature, temperatureData ,temperatureChart);
            recordPoint(pressure, pressureData ,pressureChart);

            packet_buffer.clear();
        }

        if((packet_buffer.length()) > sizeof(message_packet_t)) {
            qDebug() << "oversize error";
            packet_buffer.clear();
        }

        break;
    }
}

void MainWindow::readyRead()
{
    QByteArray newData;
    while (!serial.atEnd()) {
        newData.append(serial.read(100));
    }

    for(const auto data : newData)
        handleData(data);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    if(serial.isOpen()) {
        serial.close();
    }
    else {
        serial.setPortName(ui->portsComboBox->currentText());
        serial.open(QIODevice::ReadWrite);
    }

    if(serial.isOpen())
        ui->connectButton->setText("Disconnect");
    else
        ui->connectButton->setText("Connect");
}
