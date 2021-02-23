#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <vector>
#include <map>

#include <QMainWindow>

#include "SystemGBC.hpp"
#include "PianoKeys.hpp"

class QApplication;

class QLineEdit;
class QRadioButton;
class Window;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    
    ~MainWindow();
    
    /** Return true if application is exiting and return false otherwise
      */
    bool isQuitting() const {
    	return bQuitting;
    }

	void setApplication(QApplication* application){
		app = application;
	}

	void update();

	void connectToSystem(SystemGBC *ptr);

	void processEvents();
	
	void closeAllWindows();
	
	void updatePausedState(bool state=true);
	
	void openTileViewer();
	
	void openLayerViewer();
	
	/** The application is exiting
	  */
	void quit(){
		bQuitting = true;
	}

private slots:
    void on_checkBox_Background_stateChanged(int arg1);

    void on_checkBox_Window_stateChanged(int arg1);

    void on_checkBox_Sprites_stateChanged(int arg1);

	void on_checkBox_Breakpoint_PC_stateChanged(int arg1);

	void on_checkBox_Breakpoint_Write_stateChanged(int arg1);

	void on_checkBox_Breakpoint_Read_stateChanged(int arg1);

	void on_checkBox_Breakpoint_Opcode_stateChanged(int arg1);

	void on_lineEdit_Breakpoint_PC_editingFinished();

	void on_lineEdit_Breakpoint_Write_editingFinished();

	void on_lineEdit_Breakpoint_Read_editingFinished();
	
	void on_lineEdit_PaletteSelect_editingFinished();

	void on_comboBox_Breakpoint_Opcode_currentIndexChanged(int arg1);

	void on_comboBox_Registers_currentIndexChanged(int arg1);

    void on_spinBox_Frameskip_valueChanged(int arg1);
    
    void on_spinBox_ScreenScale_valueChanged(int arg1);
    
    void on_doubleSpinBox_Clock_Multiplier_editingFinished();

    void on_pushButton_PauseResume_pressed();

    void on_pushButton_Step_pressed();

	void on_pushButton_Advance_pressed();

    void on_pushButton_Reset_pressed();

    void on_pushButton_NextScanline_pressed();

    void on_pushButton_NextFrame_pressed();

	void on_pushButton_Refresh_pressed();

	void on_pushButton_PPU_TileViewer_pressed();
	
	void on_pushButton_PPU_LayerViewer_pressed();

    void on_spinBox_SpriteIndex_valueChanged(int arg1);

	void on_radioButton_PPU_Map0_clicked();
	
	void on_radioButton_PPU_Map1_clicked();

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

	void on_actionWrite_Save_Data_triggered();

	void on_actionRead_Save_Data_triggered();

	void on_actionDump_Registers_triggered();

	void on_actionDump_HRAM_triggered();

	void on_actionDump_WRAM_triggered();

	void on_actionHlep_triggered();

	void on_tabWidget_currentChanged(int index);

    void on_spinBox_MemoryPage_valueChanged(int arg1);

	void on_horizontalSlider_MemoryPage_valueChanged(int arg1);

    void on_lineEdit_MemoryByte_editingFinished();
    
	/////////////////////////////////////////////////////////////////////
	// APU
	/////////////////////////////////////////////////////////////////////
	
	void on_dial_APU_MasterVolume_valueChanged(int arg1);
	
	void on_dial_APU_AudioBalance_valueChanged(int arg1);
	
	void on_checkBox_APU_MasterEnable_clicked(bool arg1);
	
	void on_checkBox_APU_Ch1Enable_clicked(bool arg1);
	
	void on_checkBox_APU_Ch2Enable_clicked(bool arg1);
	
	void on_checkBox_APU_Ch3Enable_clicked(bool arg1);
	
	void on_checkBox_APU_Ch4Enable_clicked(bool arg1);
	
	void on_checkBox_APU_Mute_clicked();

	void on_radioButton_APU_Stereo_clicked();
	
	void on_radioButton_APU_Mono_clicked();
	
	void on_pushButton_APU_ClockSequencer_pressed();
	
	void on_lineEdit_APU_Ch1Frequency_textChanged(QString arg1);
	
	void on_lineEdit_APU_Ch2Frequency_textChanged(QString arg1);
	
	void on_lineEdit_APU_Ch3Frequency_textChanged(QString arg1);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    
    bool bQuitting;
    
    SystemGBC* sys;

	QApplication* app;

	const unsigned char* memoryPtr;
	
	PianoKeys::Keyboard keyboard;
	
	std::map<std::string, std::vector<Register*> > registers;

	std::unique_ptr<ComponentList> components;

	std::unique_ptr<Window> tileViewer;
	
	std::unique_ptr<Window> layerViewer;

    void setLineEditText(QLineEdit *line, const std::string &str);
    
    void setLineEditText(QLineEdit *line, const unsigned char &value);
    
    void setLineEditText(QLineEdit *line, const unsigned short &value);

    void setLineEditText(QLineEdit *line, const unsigned int &value);

    void setLineEditText(QLineEdit *line, const float &value);
    
    void setLineEditText(QLineEdit *line, const double &value);

    void setLineEditHex(QLineEdit *line, const unsigned char &value);
    
    void setLineEditHex(QLineEdit *line, const unsigned short &value);
    
    void setRadioButtonState(QRadioButton *button, const bool &state);
    
	void updateMainTab();

	void updateInstructionTab();
	
	void updateGraphicsTab();
	
	void updateSpritesTab();
	
	void updateSoundTab();
	
	void updateCartridgeTab();
	
	void updateRegistersTab();
	
	void updateMemoryTab();
	
	void updateClockTab();
	
	void updateDmaTab();
	
	void updateMemoryArray();
	
	void setDmgMode();
};

#endif // MAINWINDOW_H
