#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "WeatherTool.h"

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QTimer>
#include "WeatherData.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    bool eventFilter(QObject *watched, QEvent *event) override;
    ~MainWindow();
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void getWeatherInfo(QNetworkAccessManager *manager);
    void parseJson(QByteArray bytes);
    void setLabelContent();
    void paintSunRiseSet();
    void paintCurve();
private slots:
    void replayFinished(QNetworkReply* reply);
    void on_searchBt_clicked();
    void on_refreshBt_clicked();

private:
    Ui::MainWindow *ui;
    QMenu* rightMenu;// 右键菜单
    QAction* quitAction;// 退出选项
    QPoint mPos;
    QList<QLabel *> forecast_week_list;         //星期
    QList<QLabel *> forecast_date_list;         //日期
    QList<QLabel *> forecast_aqi_list;          //天气指数
    QList<QLabel *> forecast_type_list;         //天气
    QList<QLabel *> forecast_typeIco_list;      //天气图标
    QList<QLabel *> forecast_high_list;         //高温
    QList<QLabel *> forecast_low_list;          //低温

    QString url;        //接口链接
    QString city;       //访问的城市
    QString cityTmp;    //临时存放城市变量，防止输入错误城市的时候，原来的城市名称还在。
    WeatherTool tool;   //天气工具对象

    Today today;
    Forecast forecast[6];

    QNetworkAccessManager* manager;

    QTimer* sunTimer;

    const static QPoint sun[2];
    const static QRect sunRizeSet[2];
    const static QRect rect[2];
};
#endif // MAINWINDOW_H
