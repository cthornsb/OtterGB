#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_checkBox_Background_stateChanged(int arg1);

    void on_checkBox_Window_stateChanged(int arg1);

    void on_checkBox_Sprites_stateChanged(int arg1);

    void on_spinBox_BGP_valueChanged(int arg1);

    void on_spinBox_OBP_valueChanged(int arg1);

    void on_spinBox_Frameskip_valueChanged(int arg1);

    void on_pushButton_PauseResume_pressed();

    void on_pushButton_Step_pressed();

    void on_pushButton_Reset_pressed();

    void on_pushButton_NextScanline_pressed();

    void on_pushButton_NextFrame_pressed();

    void on_spinBox_SpriteIndex_valueChanged(int arg1);

    void on_checkBox_SoundEnabled_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
