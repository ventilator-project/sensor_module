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

    dataChart.setTitle("VFB");
    dataChart.addSeries(&sensorData);
    dataChart.createDefaultAxes();
    dataChart.axes().at(0)->setTitleText("Sample");
    dataChart.axes().at(1)->setTitleText("Counts");
    dataChart.legend()->hide();

    ui->vfbChartView->setChart(&dataChart);
//    ui->vfbChartView->setRenderHint(QPainter::Antialiasing);

    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
//    serial.setPortName("/dev/ttyACM1");
//    serial.open(QIODevice::ReadWrite);

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

void MainWindow::recordVfbPoint(uint16_t point)
{
    const int maxcounts = 1000;

    // Record the point
    if(sensorData.count() < maxcounts) {
        sensorData.append(sensorData.count(), point);
    }
    else {
        QList<QPointF> points = sensorData.points();
        points.removeFirst();
        for(QPointF &point : points)
            point.setX(point.x()-1);
        points.append(QPointF(sensorData.count(), point));
        sensorData.replace(points);
    }

    qreal yMax = std::numeric_limits<qreal>::min();
    qreal yMin = std::numeric_limits<qreal>::max();

    for(const auto &point : sensorData.points()) {
        yMin = fmin(yMin, point.y());
        yMax = fmax(yMax, point.y());
    }

    dataChart.axes().at(0)->setRange(0,maxcounts);
    dataChart.axes().at(1)->setRange(floor(yMin),ceil(yMax));
//    dataChart.axes().at(1)->setRange(0,((1<<11)-1));
}

void MainWindow::handleData(char data)
{
    static QByteArray packet_buffer;

    switch(packet_buffer.length()) {
        case 0: // SOF 1
            if(data == 0x55)
                packet_buffer.append(data);
            else
                packet_buffer.clear();
        break;
    case 1: // SOF 2
        if(data == 0x55)
            packet_buffer.append(data);
        else
            packet_buffer.clear();
        break;
    case 2: // version
        if(data == 0)
            packet_buffer.append(data);
        else
            packet_buffer.clear();
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

            recordVfbPoint(packet->vfb);

            packet_buffer.clear();
        }

        if((packet_buffer.length()) > sizeof(message_packet_t))
            packet_buffer.clear();

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
