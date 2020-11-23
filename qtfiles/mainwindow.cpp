#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_checkBox_Background_stateChanged(int arg1)
{

}

void MainWindow::on_checkBox_Window_stateChanged(int arg1)
{

}

void MainWindow::on_checkBox_Sprites_stateChanged(int arg1)
{

}

void MainWindow::on_spinBox_BGP_valueChanged(int arg1)
{

}

void MainWindow::on_spinBox_OBP_valueChanged(int arg1)
{

}

void MainWindow::on_spinBox_Frameskip_valueChanged(int arg1)
{

}

void MainWindow::on_pushButton_PauseResume_pressed()
{

}

void MainWindow::on_pushButton_Step_pressed()
{

}

void MainWindow::on_pushButton_Reset_pressed()
{

}

void MainWindow::on_pushButton_NextScanline_pressed()
{

}

void MainWindow::on_pushButton_NextFrame_pressed()
{

}

void MainWindow::on_spinBox_SpriteIndex_valueChanged(int arg1)
{

}

void MainWindow::on_checkBox_SoundEnabled_stateChanged(int arg1)
{

}
