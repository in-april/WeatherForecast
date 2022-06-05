#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QNetworkReply>
#include <QPainter>

#define SPAN_INDEX 2 // 温度曲线间隔指数
#define ORIGIN_SIZE 3 // 温度曲线原点大小
#define TEMPERATURE_STARTING_COORDINATE 30 // 高温平均值起始坐标

#pragma execution_character_set("utf-8")
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //设置窗口属性
    setWindowFlag(Qt::FramelessWindowHint); //隐藏边框
    setFixedSize(width(),height()); //固定尺寸

    //添加右键菜单
    rightMenu = new QMenu(this);
    quitAction = new QAction(rightMenu);
    quitAction->setText("退出");
    quitAction->setIcon(QIcon(":/weatherIco/close.ico"));
    rightMenu->addAction(quitAction);

    //退出动作触发
    connect(quitAction, &QAction::triggered, this, [](){qApp->exit(0);});

    //初始化UI
    forecast_week_list << ui->week0 << ui->week1 << ui->week2 << ui->week3 << ui->week4 << ui->week5;
    forecast_date_list << ui->day0 << ui->day1 << ui->day2 << ui->day3 << ui->day4 << ui->day5;
    forecast_aqi_list << ui->quality0 << ui->quality1 << ui->quality2 << ui->quality3 << ui->quality4 << ui->quality5;
    forecast_type_list << ui->weather0 << ui->weather1 << ui->weather2 << ui->weather3 << ui->weather4 << ui->weather5;
    forecast_typeIco_list << ui->weaIcon0 << ui->weaIcon1 << ui->weaIcon2 << ui->weaIcon3 << ui->weaIcon4 << ui->weaIcon5;
    forecast_high_list << ui->high0 << ui->high1 << ui->high2 << ui->high3 << ui->high4 << ui->high5;
    forecast_low_list << ui->low0 << ui->low1 << ui->low2 << ui->low3 << ui->low4 << ui->low5;

    //样式修改
    for (int i = 0; i < 6; i++)
    {
        forecast_date_list[i]->setStyleSheet("background-color: rgba(0, 255, 255, 100);");
        forecast_week_list[i]->setStyleSheet("background-color: rgba(0, 255, 255, 100);");
    }
    ui->cityEdit->setStyleSheet("QLineEdit{border: 1px solid gray; border-radius: 4px; background:argb(47, 47, 47, 130); color:rgb(255, 255, 255);} QLineEdit:hover{border-color:rgb(101, 255, 106); }");
    \
    //网络请求初始化
    // 请求天气API信息
    url = "http://t.weather.itboy.net/api/weather/city/";
    city = "北京";
    cityTmp = city;
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replayFinished(QNetworkReply*)));
    getWeatherInfo(manager);

    /* 事件过滤 */
   ui->sunRaiseSet->installEventFilter(this); // 启用事件过滤器
   ui->curveLb->installEventFilter(this);

   sunTimer = new QTimer(ui->sunRaiseSet);
   connect(sunTimer, SIGNAL(timeout()), ui->sunRaiseSet, SLOT(update()));
   sunTimer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    rightMenu->exec(QCursor::pos());
    event->accept();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    move(event->globalPos() - mPos);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    mPos = event->globalPos() - this->pos();
}

/* 请求数据 */
void MainWindow::getWeatherInfo(QNetworkAccessManager *manager)
{
    QString citycode = tool[city];
    if(citycode=="000000000"){
        QMessageBox::warning(this, "错误", "天气：指定城市不存在！", QMessageBox::Ok);
        return;
    }
    QUrl jsonUrl(url + citycode);
    manager->get( QNetworkRequest(jsonUrl) );
}

void MainWindow::replayFinished(QNetworkReply *reply)
{
    /* 获取响应的信息，状态码为200表示正常 */
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if(reply->error() != QNetworkReply::NoError || status_code != 200)
    {
        QMessageBox::warning(this, "错误", "天气：请求数据错误，检查网络连接！", QMessageBox::Ok);
        return;
    }

    QByteArray bytes = reply->readAll();
    //    QString result = QString::fromLocal8Bit(bytes);
    parseJson(bytes);
}

/* 解析Json数据 */
void MainWindow::parseJson(QByteArray bytes)
{
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &err); // 检测json格式
    if (err.error != QJsonParseError::NoError) // Json格式错误
    {
        return;
    }

    QJsonObject jsObj = jsonDoc.object();
    QString message = jsObj.value("message").toString();
    if (message.contains("success")==false)
    {
        QMessageBox::information(this, tr("The information of Json_desc"),
                                 "天气：城市错误！", QMessageBox::Ok );
        city = cityTmp;
        return;
    }

    QString dateStr = jsObj.value("date").toString();
    today.date = QDate::fromString(dateStr, "yyyyMMdd").toString("yyyy-MM-dd");
    today.city = jsObj.value("cityInfo").toObject().value("city").toString();

    // 解析data
    QJsonObject dataObj = jsObj.value("data").toObject();
    today.shidu = dataObj.value("shidu").toString();
    today.pm25 = QString::number( dataObj.value("pm25").toDouble() );
    today.quality = dataObj.value("quality").toString();
    today.wendu = dataObj.value("wendu").toString() + "°";
    today.ganmao = dataObj.value("ganmao").toString();

    // 解析data中的yesterday
    QJsonObject yestObj = dataObj.value("yesterday").toObject();
    forecast[0].date = yestObj.value("date").toString();
    forecast[0].high = yestObj.value("high").toString();
    forecast[0].low = yestObj.value("low").toString();
    forecast[0].aqi = QString::number( yestObj.value("aqi").toDouble() );
    forecast[0].type = yestObj.value("type").toString();
    forecast[0].week = yestObj.value("week").toString();

    // 解析data中的forecast
    QJsonArray forecastArr = dataObj.value("forecast").toArray();
    int j = 0;
    for (int i = 1; i < 6; i++)
    {
        QJsonObject dateObj = forecastArr.at(j).toObject();
        forecast[i].date = dateObj.value("date").toString();
        forecast[i].aqi = QString::number( dateObj.value("aqi").toDouble() );
        forecast[i].high = dateObj.value("high").toString();
        forecast[i].low = dateObj.value("low").toString();
        forecast[i].type = dateObj.value("type").toString();
        forecast[i].week = dateObj.value("week").toString();
        j++;
    }

    // 取得今日数据
    QJsonObject todayObj = forecastArr.at(0).toObject();
    today.fx = todayObj.value("fx").toString();
    today.fl = todayObj.value("fl").toString();
    today.type = todayObj.value("type").toString();
    today.sunrise = todayObj.value("sunrise").toString();
    today.sunset = todayObj.value("sunset").toString();
    today.notice = todayObj.value("notice").toString();

    setLabelContent();
}

/* 设置控件文本 */
void MainWindow::setLabelContent()
{
    // 今日数据
    ui->dateLabel->setText(today.date);
    ui->temperature->setText(today.wendu);
    ui->cityName->setText(today.city);
    ui->weather->setText(today.type);
    ui->notice->setText(today.notice);
    ui->shidu->setText(today.shidu);
    ui->pm25->setText(today.pm25);
    ui->fengxiang->setText(today.fx);
    ui->fengli->setText(today.fl);
    ui->ganmaoBrowser->setText(today.ganmao);

    // 判断白天还是夜晚图标
    QString sunsetTime = today.date + " " + today.sunset;
    if (QDateTime::currentDateTime() <= QDateTime::fromString(sunsetTime, "yyyy-MM-dd hh:mm"))
    {
        ui->weaIcon->setStyleSheet( tr("border-image: url(:/day/day/%1.png); background-color: argb(60, 60, 60, 0);").arg(today.type) );
    }
    else
    {
        ui->weaIcon->setStyleSheet( tr("border-image: url(:/night/night/%1.png); background-color: argb(60, 60, 60, 0);").arg(today.type) );
    }

    ui->qualityLb->setText(today.quality == "" ? "无" : today.quality);
    if (today.quality == "优")
    {
        ui->qualityLb->setStyleSheet("color: rgb(0, 255, 0); background-color: argb(255, 255, 255, 0);");
    }
    else if (today.quality == "良")
    {
        ui->qualityLb->setStyleSheet("color: rgb(255, 255, 0); background-color: argb(255, 255, 255, 0);");
    }
    else if (today.quality == "轻度污染")
    {
        ui->qualityLb->setStyleSheet("color: rgb(255, 170, 0); background-color: argb(255, 255, 255, 0);");
    }
    else if (today.quality == "重度污染")
    {
        ui->qualityLb->setStyleSheet("color: rgb(255, 0, 0); background-color: argb(255, 255, 255, 0);");
    }
    else if (today.quality == "严重污染")
    {
        ui->qualityLb->setStyleSheet("color: rgb(170, 0, 0); background-color: argb(255, 255, 255, 0);");
    }
    else
    {
        ui->qualityLb->setStyleSheet("color: rgb(255, 255, 255); background-color: argb(255, 255, 255, 0);");
    }

    // 六天数据
    for (int i = 0; i < 6; i++)
    {
        forecast_week_list[i]->setText(forecast[i].week);
        forecast_date_list[i]->setText(forecast[i].date);
        forecast_type_list[i]->setText(forecast[i].type);
        forecast_high_list[i]->setText(forecast[i].high.split(" ").at(1));
        forecast_low_list[i]->setText(forecast[i].low.split(" ").at(1));
        forecast_typeIco_list[i]->setStyleSheet( tr("image: url(:/day/day/%1.png);").arg(forecast[i].type) );

        if (forecast[i].aqi.toInt() >= 0 && forecast[i].aqi.toInt() <= 50)
        {
            forecast_aqi_list[i]->setText("优质");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(0, 255, 0);");
        }
        else if (forecast[i].aqi.toInt() > 50 && forecast[i].aqi.toInt() <= 100)
        {
            forecast_aqi_list[i]->setText("良好");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 255, 0);");
        }
        else if (forecast[i].aqi.toInt() > 100 && forecast[i].aqi.toInt() <= 150)
        {
            forecast_aqi_list[i]->setText("轻度污染");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 170, 0);");
        }
        else if (forecast[i].aqi.toInt() > 150 && forecast[i].aqi.toInt() <= 200)
        {
            forecast_aqi_list[i]->setText("重度污染");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 0, 0);");
        }
        else
        {
            forecast_aqi_list[i]->setText("严重污染");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(170, 0, 0);");
        }
    }//for

    ui->week0->setText( "昨天" );
    ui->week1->setText( "今天" );
    ui->week2->setText( "明天" );
    ui->curveLb->update();
}

/* 日出日落图形绘制 */
void MainWindow::paintSunRiseSet()
{
    QPainter painter(ui->sunRaiseSet);
    painter.setRenderHint(QPainter::Antialiasing, true); // 反锯齿

    // 绘制日出日落线和文本
    painter.save();
    QPen pen = painter.pen();
    pen.setWidthF(0.5);
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    painter.drawLine(sun[0], sun[1]);
    painter.restore();

    painter.save();
    painter.setFont( QFont("Microsoft Yahei", 8, QFont::Normal) ); // 字体、大小、正常粗细
    painter.setPen(Qt::white);

    if (today.sunrise != "" && today.sunset != "")
    {
        painter.drawText(sunRizeSet[0], Qt::AlignHCenter, today.sunrise);
        painter.drawText(sunRizeSet[1], Qt::AlignHCenter, today.sunset);
    }
    painter.drawText(rect[1], Qt::AlignHCenter, u8"日出日落");
    painter.restore();

    // 绘制圆弧
    painter.save();
    //    pen.setWidth(1);
    pen.setWidthF(0.5);
    pen.setStyle(Qt::DotLine); // 虚线
    pen.setColor(Qt::green);
    painter.setPen(pen);
    painter.drawArc(rect[0], 0 * 16, 180 * 16);
    painter.restore();

    // 绘制日出日落占比
    if (today.sunrise != "" && today.sunset != "")
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 85, 0, 100));

        int startAngle, spanAngle;
        QString sunsetTime = today.date + " " + today.sunset;

        if (QDateTime::currentDateTime() > QDateTime::fromString(sunsetTime, "yyyy-MM-dd hh:mm"))
        {
            startAngle = 0 * 16;
            spanAngle = 180 * 16;
        }
        else
        {
            // 计算起始角度和跨越角度
            static QStringList sunSetTime = today.sunset.split(":");
            static QStringList sunRiseTime = today.sunrise.split(":");

            static QString sunsetHour = sunSetTime.at(0);
            static QString sunsetMint = sunSetTime.at(1);
            static QString sunriseHour = sunRiseTime.at(0);
            static QString sunriseMint = sunRiseTime.at(1);

            static int sunrise = sunriseHour.toInt() * 60 + sunriseMint.toInt();
            static int sunset = sunsetHour.toInt() * 60 + sunsetMint.toInt();
            int now = QTime::currentTime().hour() * 60 + QTime::currentTime().minute();

            startAngle = ( (double)(sunset - now) / (sunset - sunrise) ) * 180 * 16;
            spanAngle = ( (double)(now - sunrise) / (sunset - sunrise) ) * 180 * 16;
        }

        if (startAngle >= 0 && spanAngle >= 0)
        {
            painter.drawPie(rect[0], startAngle, spanAngle); // 扇形绘制
        }
    }
}

/* 温度曲线绘制 */
void MainWindow::paintCurve()
{
    QPainter painter(ui->curveLb);
    painter.setRenderHint(QPainter::Antialiasing, true); // 反锯齿

    // 将温度转换为int类型，并计算平均值，平均值作为curveLb曲线的参考值，参考Y坐标为45
    int tempTotal = 0;
    int high[6] = {};
    int low[6] = {};

    QString h, l;
    for (int i = 0; i < 6; i++)
    {
        h = forecast[i].high.split(" ").at(1);
        h = h.left(h.length() - 1);
        high[i] = (int)(h.toDouble());
        tempTotal += high[i];

        l = forecast[i].low.split(" ").at(1);
        l = l.left(h.length() - 1);
        low[i] = (int)(l.toDouble());
    }
    int tempAverage = (int)(tempTotal / 6); // 最高温平均值

    // 算出温度对应坐标
    int pointX[6] = {35, 103, 172, 241, 310, 379}; // 点的X坐标
    int pointHY[6] = {0};
    int pointLY[6] = {0};
    for (int i = 0; i < 6; i++)
    {
        pointHY[i] = TEMPERATURE_STARTING_COORDINATE - ((high[i] - tempAverage) * SPAN_INDEX);
        pointLY[i] = TEMPERATURE_STARTING_COORDINATE + ((tempAverage - low[i]) * SPAN_INDEX);
    }

    QPen pen = painter.pen();
    pen.setWidth(1);

    // 高温曲线绘制
    painter.save();
    pen.setColor(QColor(255, 170, 0));
    pen.setStyle(Qt::DotLine);
    painter.setPen(pen);
    painter.setBrush(QColor(255, 170, 0));
    painter.drawEllipse(QPoint(pointX[0], pointHY[0]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawEllipse(QPoint(pointX[1], pointHY[1]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawLine(pointX[0], pointHY[0], pointX[1], pointHY[1]);

    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);

    for (int i = 1; i < 5; i++)
    {
        painter.drawEllipse(QPoint(pointX[i+1], pointHY[i+1]), ORIGIN_SIZE, ORIGIN_SIZE);
        painter.drawLine(pointX[i], pointHY[i], pointX[i+1], pointHY[i+1]);
    }
    painter.restore();

    // 低温曲线绘制
    pen.setColor(QColor(0, 255, 255));
    pen.setStyle(Qt::DotLine);
    painter.setPen(pen);
    painter.setBrush(QColor(0, 255, 255));
    painter.drawEllipse(QPoint(pointX[0], pointLY[0]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawEllipse(QPoint(pointX[1], pointLY[1]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawLine(pointX[0], pointLY[0], pointX[1], pointLY[1]);

    pen.setColor(QColor(0, 255, 255));
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    for (int i = 1; i < 5; i++)
    {
        painter.drawEllipse(QPoint(pointX[i+1], pointLY[i+1]), ORIGIN_SIZE, ORIGIN_SIZE);
        painter.drawLine(pointX[i], pointLY[i], pointX[i+1], pointLY[i+1]);
    }
}

/* 事件过滤 */
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->sunRaiseSet && event->type() == QEvent::Paint)
    {
        paintSunRiseSet();
    }
    else if (watched == ui->curveLb && event->type() == QEvent::Paint)
    {
        paintCurve();
    }

    return QWidget::eventFilter(watched,event);
}

// 日出日落底线
const QPoint MainWindow::sun[2] = {
    QPoint(15, 70),
    QPoint(125, 70)
};

// 日出日落时间
const QRect MainWindow::sunRizeSet[2] = {
    QRect(0, 75, 50, 20),
    QRect(95, 75, 50, 20)
};

// 日出日落圆弧
const QRect MainWindow::rect[2] = {
    QRect(20, 20, 100, 100), // 虚线圆弧
    QRect(45, 75, 50, 20) // “日出日落”文本
};

void MainWindow::on_searchBt_clicked()
{
    cityTmp = city;
    city = ui->cityEdit->text();
    getWeatherInfo(manager);
}


void MainWindow::on_refreshBt_clicked()
{
    getWeatherInfo(manager);
    ui->curveLb->update();
}

