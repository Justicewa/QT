#include "widget.h"
#include "ui_widget.h"

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

void Widget::on_pushButton_clicked()
{
    char string1[20]={0};
    char string2[20]={0};
    char fu[2]={0};

    int sum = 0;
    strcpy(string1, ui->lineEdit1->text().toLatin1().data()); //文本转char
    strcpy(string2, ui->lineEdit3->text().toLatin1().data());
    strcpy(fu,ui->lineEdit2->text().toLatin1().data());

    switch (fu[0]) {
    case '+':
        sum = atoi(string1) + atoi(string2);
        break;
    case '-':
        sum = atoi(string1) - atoi(string2);
        break;
    case '*':
        sum = atoi(string1) * atoi(string2);
        break;
    case '/':
        sum = atoi(string1) / atoi(string2);
        break;

    default:
        break;
    }

    char buf[20] = {0};
    sprintf(buf,"%d",sum);
    ui->lineEdit4->setText(buf);

}

void Widget::on_pushButton_13_clicked()
{

}
