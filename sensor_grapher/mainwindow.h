#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>

QT_CHARTS_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#ifdef _MSC_VER
    #pragma pack(push)
    #pragma pack(1)
    #define PACKED
#elif __GNUC__
    #define PACKED __attribute__((__packed__))
#else
    #error packed structs not implemented
#endif
typedef struct {
    uint16_t SOF; // 0x5555
    uint8_t version; // 0
    uint8_t type; // Packet type: 0
    uint16_t vfb; // ADC value
    uint16_t pressure;
    uint16_t temperature;
    uint16_t crc; // TODO
} PACKED message_packet_t;
#ifdef _MSC_VER
    #pragma pack(pop)
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void readyRead();

private slots:
    void on_connectButton_clicked();

private:
    Ui::MainWindow *ui;

    QSerialPort serial;

    QChart dataChart;                   //!< Chart for showing live sensor data
    QLineSeries sensorData;             //!< Most current data series from the sensor
    void handleData(char data);
    void recordVfbPoint(uint16_t point);

    QTimer portScanner;
};
#endif // MAINWINDOW_H
