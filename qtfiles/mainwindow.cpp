#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "Support.hpp"
#include "SystemTimer.hpp"
#include "LR35902.hpp"
#include "GPU.hpp"

QString getQString(const std::string &str)
{
	return QString(str.c_str());
}

MainWindow::MainWindow(QApplication *parent) :
    QMainWindow(),
    ui(new Ui::MainWindow),
    app(parent),
    sys(0x0)
{
    ui->setupUi(this);
    show();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete app;
}

void MainWindow::update()
{
	switch(ui->tabWidget->currentIndex()){
		case 0: // Main tab
			updateMainTab();
			break;
		case 1: // Graphics tab
			updateGraphicsTab();
			break;
		case 2: // Sprites tab
			updateSpritesTab();
			break;
		case 3: // Sound tab
			updateSoundTab();
			break;
		case 4: // Cartridge tab
			updateCartridgeTab();
			break;
		case 5: // Registers tab
			updateRegistersTab();
			break;
		case 6: // Memory tab
			updateMemoryTab();
			break;
		default:
			break;
	}
}

void MainWindow::updateMainTab(){
	LR35902 *cpu = components->cpu;

	// CPU registers
	setLineEditHex(ui->lineEdit_rA, cpu->getA());
	setLineEditHex(ui->lineEdit_rB, cpu->getB());
	setLineEditHex(ui->lineEdit_rC, cpu->getC());
	setLineEditHex(ui->lineEdit_rD, cpu->getD());
	setLineEditHex(ui->lineEdit_rE, cpu->getE());
	setLineEditHex(ui->lineEdit_rH, cpu->getH());
	setLineEditHex(ui->lineEdit_rL, cpu->getL());
	
	// CPU flags
	setLineEditText(ui->lineEdit_rF, getBinary(cpu->getF(), 4));
	
	// Program counter and stack pointer
	setLineEditHex(ui->lineEdit_PC, cpu->getProgramCounter());
	setLineEditHex(ui->lineEdit_SP, cpu->getStackPointer());
	
	// Memory bank indicators
	setLineEditText(ui->lineEdit_WRAM_Bank, components->wram->getBankSelect());
	
	// Frames per second
	setLineEditText(ui->lineEdit_FPS, components->sclk->getFramerate());
	
	// Instruction history
	//ui->plainText_Instr_History->setPlainText(getQString(cpu->getInstruction()));
}

void MainWindow::updateGraphicsTab(){
	GPU *gpu = components->gpu;

	// LCD status
	setRadioButtonState(ui->radioButton_LcdEnabled, gpu->bgDisplayEnable);
	setRadioButtonState(ui->radioButton_BackgroundEnabled, gpu->bgDisplayEnable);
	setRadioButtonState(ui->radioButton_WindowEnabled, gpu->winDisplayEnable);
	setRadioButtonState(ui->radioButton_SpritesEnabled, gpu->objDisplayEnable);
	setRadioButtonState(ui->radioButton_SpriteSizeSelect, gpu->objSizeSelect);
	setRadioButtonState(ui->radioButton_BackgroundTilemap, gpu->bgTileMapSelect);
	setRadioButtonState(ui->radioButton_WindowTilemap, gpu->winTileMapSelect);
	setRadioButtonState(ui->radioButton_BgWinTileData, gpu->bgWinTileDataSelect);

	// Color palettes
	int bgp = ui->spinBox_BGP->value();
	int obp = ui->spinBox_OBP->value();
	setLineEditHex(ui->lineEdit_BGP_0, gpu->getBgPaletteColorHex(bgp*4));
	setLineEditHex(ui->lineEdit_BGP_1, gpu->getBgPaletteColorHex(bgp*4+1));
	setLineEditHex(ui->lineEdit_BGP_2, gpu->getBgPaletteColorHex(bgp*4+2));
	setLineEditHex(ui->lineEdit_BGP_3, gpu->getBgPaletteColorHex(bgp*4+3));
	setLineEditHex(ui->lineEdit_OBP_0, gpu->getObjPaletteColorHex(obp*4));
	setLineEditHex(ui->lineEdit_OBP_1, gpu->getObjPaletteColorHex(obp*4+1));
	setLineEditHex(ui->lineEdit_OBP_2, gpu->getObjPaletteColorHex(obp*4+2));
	setLineEditHex(ui->lineEdit_OBP_3, gpu->getObjPaletteColorHex(obp*4+3));
	//QPalette pal(QColor());
	//setPalette(pal);
	
	// VRAM bank select
	setLineEditHex(ui->lineEdit_VRAM_Bank, gpu->getBankSelect());
}

void MainWindow::updateSpritesTab(){
	SpriteAttributes attr = components->oam->getSpriteAttributes(ui->spinBox_SpriteIndex->value());
	
	// Display numerical sprite attributes
	setLineEditHex(ui->lineEdit_SpriteX, attr.xPos);
	setLineEditHex(ui->lineEdit_SpriteY, attr.yPos);
	setLineEditHex(ui->lineEdit_SpriteTile, attr.tileNum);
	if(bGBCMODE)
		setLineEditHex(ui->lineEdit_SpritePalette, attr.gbcPalette);
	else
		setLineEditHex(ui->lineEdit_SpritePalette, (unsigned char)(attr.ngbcPalette ? 1 : 0));
	
	// Display sprite boolean values
	setRadioButtonState(ui->radioButton_SpriteBank, attr.gbcVramBank);
	setRadioButtonState(ui->radioButton_SpriteHorizontal, attr.xFlip);
	setRadioButtonState(ui->radioButton_SpriteVertical, attr.yFlip);
	setRadioButtonState(ui->radioButton_SpritePriority, attr.objPriority);
}

void MainWindow::updateSoundTab(){
}

void MainWindow::updateCartridgeTab(){
	Cartridge *cart = components->cart;

	// Cartridge header information
	setLineEditText(ui->lineEdit_RomPath, sys->getRomFilename());
	setLineEditText(ui->lineEdit_RomTitle, cart->getTitleString());
	setLineEditHex(ui->lineEdit_RomEntryPoint, cart->getProgramEntryPoint());
	setLineEditText(ui->lineEdit_RomSize, cart->getRomSize());
	setLineEditText(ui->lineEdit_RomSramSize, cart->getRamSize());
	setLineEditHex(ui->lineEdit_RomType, cart->getCartridgeType());
	setLineEditText(ui->lineEdit_RomLanguage, cart->getLanguage());
	
	// Cartridge status
	setLineEditHex(ui->lineEdit_ROM_Bank, cart->getBankSelect());
	setLineEditHex(ui->lineEdit_SRAM_Bank, cart->getRam()->getBankSelect());
	setRadioButtonState(ui->radioButton_RomSramEnabled, cart->getExternalRamEnabled());
	setRadioButtonState(ui->radioButton_RomCgbMode, bGBCMODE);
}

void MainWindow::updateRegistersTab(){
	QString str;
	unsigned char byte;
	for(unsigned short i = 0xFF00; i <= 0xFF80; i++){
		Register *reg = sys->getPtrToRegister(i);
		if(!reg) 
			continue;
		str.append(getQString(reg->dump()+"\n"));
	}
	ui->textBrowser_Registers->setPlainText(str);
}

void MainWindow::updateMemoryTab(){
	unsigned short memLow = 0x80*ui->spinBox_MemoryPage->value();
	unsigned short memHigh = memLow + 0x80;
	unsigned short currentByte = memLow;
	setLineEditHex(ui->lineEdit_MemoryPageLow, memLow);
	setLineEditHex(ui->lineEdit_MemoryPageHigh, memHigh);
	QString str;
	unsigned char byte;
	for(unsigned short i = 0; i < 0x8; i++){
		str.append(getQString(getHex(currentByte))+" ");
		for(unsigned short j = 0; j <= 0xF; j++){
			sys->read(currentByte+j, byte);
			str.append(getQString(getHex(byte))+" ");
		}
		str.append("\n");
		currentByte += 16;
	}
	ui->textBrowser_Memory->setPlainText(str);
}

void MainWindow::connectToSystem(SystemGBC *ptr){ 
	sys = ptr; 
	components = ptr->getListOfComponents();
	for(auto comp = components->list.begin(); comp != components->list.end(); comp++)
		ui->comboBox_Registers->addItem(getQString(comp->first));
}

void MainWindow::processEvents()
{
	app->processEvents();
}

void MainWindow::closeAllWindows()
{
	app->closeAllWindows();
}

void MainWindow::setLineEditText(QLineEdit *line, const std::string &str){
	line->setText(getQString(str));
}

void MainWindow::setLineEditText(QLineEdit *line, const unsigned char &value){
	line->setText(getQString(ucharToStr(value)));
}

void MainWindow::setLineEditText(QLineEdit *line, const unsigned short &value){
	line->setText(getQString(ushortToStr(value)));
}

void MainWindow::setLineEditText(QLineEdit *line, const float &value){
	line->setText(getQString(floatToStr(value, 1)));
}

void MainWindow::setLineEditText(QLineEdit *line, const double &value){
	line->setText(getQString(doubleToStr(value, 1)));
}

void MainWindow::setLineEditHex(QLineEdit *line, const unsigned char &value){
	line->setText(getQString(getHex(value)));
}

void MainWindow::setLineEditHex(QLineEdit *line, const unsigned short &value){
	line->setText(getQString(getHex(value)));
}

void MainWindow::setRadioButtonState(QRadioButton *button, const bool &state)
{
	button->setChecked(state);
}

/////////////////////////////////////////////////////////////////////
// slots
/////////////////////////////////////////////////////////////////////

void MainWindow::on_checkBox_Background_stateChanged(int arg1)
{
	components->gpu->userLayerEnable[0] = (arg1 != 0);
}

void MainWindow::on_checkBox_Window_stateChanged(int arg1)
{
	components->gpu->userLayerEnable[1] = (arg1 != 0);
}

void MainWindow::on_checkBox_Sprites_stateChanged(int arg1)
{
	components->gpu->userLayerEnable[2] = (arg1 != 0);
}

void MainWindow::on_checkBox_Show_Framerate_stateChanged(int arg1)
{
	sys->setDisplayFramerate(arg1 != 0);
}

void MainWindow::on_spinBox_BGP_valueChanged(int arg1)
{

}

void MainWindow::on_spinBox_OBP_valueChanged(int arg1)
{

}

void MainWindow::on_spinBox_Frameskip_valueChanged(int arg1)
{
	sys->setFrameSkip(arg1);
}

void MainWindow::on_pushButton_PauseResume_pressed()
{
	if(!sys->getEmulationPaused()){
		sys->pause();
		ui->pushButton_PauseResume->setText("Resume");
	}
	else{
		sys->unpause();
		ui->pushButton_PauseResume->setText("Pause");
	}
}

void MainWindow::on_pushButton_Step_pressed()
{

}

void MainWindow::on_pushButton_Reset_pressed()
{
	sys->reset();
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

/////////////////////////////////////////////////////////////////////
// Menu action slots
/////////////////////////////////////////////////////////////////////

void MainWindow::on_actionLoad_ROM_triggered()
{

}

void MainWindow::on_actionQuit_triggered()
{
	sys->quit();
}

void MainWindow::on_actionPause_Emulation_triggered()
{
	if(!sys->getEmulationPaused()){
		sys->pause();
		ui->pushButton_PauseResume->setText("Resume");
	}
}

void MainWindow::on_actionResume_Emulation_triggered()
{
	if(sys->getEmulationPaused()){
		sys->unpause();
		ui->pushButton_PauseResume->setText("Pause");
	}
}

void MainWindow::on_actionDump_Memory_triggered()
{
	sys->dumpMemory("memory.dat");
}

void MainWindow::on_actionDump_VRM_triggered()
{
	sys->dumpVRAM("vram.dat");
}

void MainWindow::on_actionDump_SRAM_triggered()
{
	sys->dumpSRAM("sram.dat");
}

void MainWindow::on_actionPower_Off_triggered()
{

}

void MainWindow::on_actionSave_State_triggered()
{
	sys->quicksave();
}

void MainWindow::on_actionLoad_State_triggered()
{
	sys->quickload();
}

void MainWindow::on_actionDump_Registers_triggered()
{

}

void MainWindow::on_actionDump_HRAM_triggered()
{
	//sys->dumpHRAM();
}

void MainWindow::on_actionDump_WRAM_triggered()
{
	//sys->dumpWRAM();
}

void MainWindow::on_actionHlep_triggered()
{
	sys->help();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{

}

void MainWindow::on_spinBox_MemoryPage_valueChanged(int arg1)
{
	ui->horizontalSlider_MemoryPage->setValue(arg1);
}

void MainWindow::on_horizontalSlider_MemoryPage_valueChanged(int arg1)
{
	ui->spinBox_MemoryPage->setValue(arg1);
}

void MainWindow::on_lineEdit_MemoryByte_editingFinished()
{
	std::string str = ui->lineEdit_MemoryByte->text().toStdString();
	unsigned short byte = strtoul(str.c_str(), 0, 16);
	ui->spinBox_MemoryPage->setValue(byte/128);
}
