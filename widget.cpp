#include "widget.h"
#include "ui_widget.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPixmap>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *event){
    //绘图 类
    QPainter painter(this); //this代表当前UI界面//绘画的人
    //调用其中的一个方法

#if 0
    painter.drawLine(0,0,200,200);//画一条线

    //画矩形
    QPen pen;//定义一支画笔
    pen.setColor(QColor(255,0,0));//red
    QBrush brush(QColor(0,255,0,125));

    painter.setPen(pen);//给绘画者一支画笔
    painter.setBrush(brush);//给绘画者一个画刷
    painter.drawRect(100,100,200,100);//画矩形
#endif

#if 0
    //显示图片
    QPixmap pix;//定义一个图片对象
    pix.load("images/1.jpg");//加载图片到Pix缓冲区
    painter.drawPixmap(0,0,200,200,pix);//将图片画到UI的位置
#endif

#if 0
    QImage img;
    img.load("images/2.jpg");
    QPixmap map = QPixmap::fromImage(img);

    ui->label->setPixmap(map);//显示图片
    ui->label->setScaledContents(true);//依据图片大小填充整个label

#endif
    //动态显示图片



}
