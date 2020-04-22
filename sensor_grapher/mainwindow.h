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
    uint32_t airflow;
    uint32_t pressure;
    uint32_t temperature;
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
    const int maxcounts = 500;

    Ui::MainWindow *ui;

    QSerialPort serial;

    QChart vfbChart;                   //!< Chart for showing live sensor data
    QLineSeries vfbData;             //!< Most current data series from the sensor

    QChart pressureChart;
    QLineSeries pressureData;

    QChart temperatureChart;
    QLineSeries temperatureData;

    void handleData(char data);
    void recordPoint(double point, QLineSeries &series, QChart &chart);

    QTimer portScanner;
};
#endif // MAINWINDOW_H
