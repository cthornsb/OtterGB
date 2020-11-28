#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "SystemGBC.hpp"

class QLineEdit;
class QRadioButton;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QApplication *parent = 0);
    
    ~MainWindow();

	void update();

	void connectToSystem(SystemGBC *ptr);

	void processEvents();
	
	void closeAllWindows();

private slots:
    void on_checkBox_Background_stateChanged(int arg1);

    void on_checkBox_Window_stateChanged(int arg1);

    void on_checkBox_Sprites_stateChanged(int arg1);

	void on_checkBox_Show_Framerate_stateChanged(int arg1);

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

	// Menu action slots

	void on_actionLoad_ROM_triggered();

	void on_actionQuit_triggered();

	void on_actionPause_Emulation_triggered();

	void on_actionResume_Emulation_triggered();

	void on_actionDump_Memory_triggered();

	void on_actionDump_VRM_triggered();

	void on_actionDump_SRAM_triggered();

	void on_actionPower_Off_triggered();

	void on_actionSave_State_triggered();

	void on_actionLoad_State_triggered();

	void on_actionDump_Registers_triggered();

	void on_actionDump_HRAM_triggered();

	void on_actionDump_WRAM_triggered();

	void on_actionHlep_triggered();

	void on_tabWidget_currentChanged(int index);

    void on_spinBox_MemoryPage_valueChanged(int arg1);

	void on_horizontalSlider_MemoryPage_valueChanged(int arg1);

    void on_lineEdit_MemoryByte_editingFinished();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    
    QApplication *app;
    
    SystemGBC *sys;

	ComponentList *components;

    void setLineEditText(QLineEdit *line, const std::string &str);
    
    void setLineEditText(QLineEdit *line, const unsigned char &value);
    
    void setLineEditText(QLineEdit *line, const unsigned short &value);

    void setLineEditText(QLineEdit *line, const float &value);
    
    void setLineEditText(QLineEdit *line, const double &value);

    void setLineEditHex(QLineEdit *line, const unsigned char &value);
    
    void setLineEditHex(QLineEdit *line, const unsigned short &value);
    
    void setRadioButtonState(QRadioButton *button, const bool &state);
    
	void updateMainTab();
	
	void updateGraphicsTab();
	
	void updateSpritesTab();
	
	void updateSoundTab();
	
	void updateCartridgeTab();
	
	void updateRegistersTab();
	
	void updateMemoryTab();
};

#endif // MAINWINDOW_H
