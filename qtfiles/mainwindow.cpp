#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "GraphicsOpenGL.hpp"
#include "Support.hpp"
#include "SystemTimer.hpp"
#include "LR35902.hpp"
#include "GPU.hpp"

unsigned char ZERO = 0;

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
}

void MainWindow::update()
{
	static bool firstUpdate = true;
	if(firstUpdate){
	    // Get pointers to the page in memory.
	    updateMemoryArray();
	    if(!bGBCMODE)
	    	setDmgMode();
	    Cartridge *cart = components->cart;
		if(cart->getRamSize() == 0){
			ui->radioButton_RomSramEnabled->setEnabled(false);
			ui->lineEdit_RomSramSize->setEnabled(false);
			ui->lineEdit_SRAM_Bank->setEnabled(false);
		}
		if(cart->getRomSize() <= 32){
			ui->lineEdit_ROM_Bank->setEnabled(false);
		}
		for(unsigned short i = 0; i < 128; i++){
			Register *reg = sys->getPtrToRegister(0xFF00+i); 
			if(!reg->set())
				continue;
			registers["ALL"].push_back(reg);
			registers[reg->getSystemComponent()->getName()].push_back(reg);
		}
		// Initialize palette viewer
		paletteViewer = std::unique_ptr<Window>(new Window(160, 160));
		paletteViewer->setParent(ui->widget_PaletteViewer);
		paletteViewer->initialize();
	    firstUpdate = false;
	}
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
		case 7: // Clock tab
			updateClockTab();
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
	setLineEditHex(ui->lineEdit_WRAM_Bank, components->wram->getBankSelect());
	
	// Frames per second
	setLineEditText(ui->lineEdit_FPS, components->sclk->getFramerate());
	
	// Instruction history
	if(ui->checkBox_Show_Instructions->isChecked()){
		const int maxInstructions = 14;
		static int instructions = 1;
		QString history = ui->plainText_Instr_History->toPlainText();
		if(instructions >= maxInstructions){
			int index = history.indexOf('\n');
			history.remove(0, index+1);
		}
		else
			instructions++;
		std::string newinstr = cpu->getInstruction();
		history.append(getQString(newinstr+"\n"));
		ui->plainText_Instr_History->setPlainText(history);
	}
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
	//gpu->getDmgPaletteColorHex(bgp*4)
	if(bGBCMODE){
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
	}
	
	// VRAM bank select
	setLineEditHex(ui->lineEdit_VRAM_Bank, gpu->getBankSelect());
	
	// Update palette viewer
	paletteViewer->setCurrent();
	gpu->drawTileMaps(paletteViewer.get());
	paletteViewer->render();
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
	std::string component = ui->comboBox_Registers->currentText().toStdString();
	std::map<std::string, std::vector<Register*> >::iterator iter = registers.find(component);
	if(iter != registers.end()){
		for(auto reg = iter->second.begin(); reg != iter->second.end(); reg++){
			str.append(getQString((*reg)->dump()+"\n"));
		}
	}
	ui->textBrowser_Registers->setPlainText(str);
}

void MainWindow::updateMemoryTab(){
	unsigned short currentByte = 0x80*ui->spinBox_MemoryPage->value();
	QString str;
	for(unsigned short i = 0; i < 8; i++){
		str.append(getQString(getHex(currentByte))+" ");
		for(unsigned short j = 0; j < 16; j++){
			str.append(getQString(getHex(*memory[8*i+j]))+" ");
		}
		str.append("\n");
		currentByte += 16;
	}
	ui->textBrowser_Memory->setPlainText(str);
}

void MainWindow::updateClockTab(){
	SystemClock *sclk = components->sclk;

	setLineEditText(ui->lineEdit_Clock_Frequency, sclk->getFrequency()/1E6);
	setLineEditText(ui->lineEdit_Clock_SinceVBlank, sclk->getCyclesSinceVBlank());
	setLineEditText(ui->lineEdit_Clock_SinceHBlank, sclk->getCyclesSinceHBlank());

	unsigned char driverMode = sclk->getDriverMode();	
	setRadioButtonState(ui->radioButton_Clock_VBlank, driverMode == 1);
	setRadioButtonState(ui->radioButton_Clock_HBlank, driverMode == 0);
	setRadioButtonState(ui->radioButton_Clock_Mode0, driverMode == 0);
	setRadioButtonState(ui->radioButton_Clock_Mode1, driverMode == 1);
	setRadioButtonState(ui->radioButton_Clock_Mode2, driverMode == 2);
	setRadioButtonState(ui->radioButton_Clock_Mode3, driverMode == 3);
	setLineEditText(ui->lineEdit_rLY, *sys->getPtrToRegisterValue(0xFF44));
}

void MainWindow::updateMemoryArray()
{
	unsigned short memLow = 0x80*ui->spinBox_MemoryPage->value();
	unsigned short memHigh = memLow + 0x80;
	setLineEditHex(ui->lineEdit_MemoryPageLow, memLow);
	setLineEditHex(ui->lineEdit_MemoryPageHigh, memHigh);
	for(unsigned short i = 0; i < 128; i++){
		memory[i] = sys->getConstPtr(memLow+i);
		if(!memory[i]) // Inaccessible memory location
			memory[i] = &ZERO;
	}	
}

void MainWindow::setDmgMode(){
	ui->radioButton_CurrentSpeed->setEnabled(false);
	ui->lineEdit_BGP_0->setEnabled(false);
	ui->lineEdit_BGP_1->setEnabled(false);
	ui->lineEdit_BGP_2->setEnabled(false);
	ui->lineEdit_BGP_3->setEnabled(false);
	ui->lineEdit_OBP_0->setEnabled(false);
	ui->lineEdit_OBP_1->setEnabled(false);
	ui->lineEdit_OBP_2->setEnabled(false);
	ui->lineEdit_OBP_3->setEnabled(false);
	ui->spinBox_BGP->setEnabled(false);
	ui->spinBox_OBP->setEnabled(false);
	ui->lineEdit_VRAM_Bank->setEnabled(false);
}

void MainWindow::connectToSystem(SystemGBC *ptr){ 
	sys = ptr; 
	components = std::unique_ptr<ComponentList>(new ComponentList(ptr));
	ui->comboBox_Registers->addItem("ALL");
	for(auto component = components->list.begin(); component != components->list.end(); component++)
		ui->comboBox_Registers->addItem(getQString(component->second->getName()));
    LR35902::Opcode *opcodes = components->cpu->getOpcodes();
    for(unsigned short i = 0; i < 256; i++)
    	ui->comboBox_Breakpoint_Opcode->addItem(getQString(opcodes[i].sName));
    opcodes = components->cpu->getOpcodesCB();
    for(unsigned short i = 0; i < 256; i++)
    	ui->comboBox_Breakpoint_Opcode->addItem(getQString(opcodes[i].sName));
	// Toggle opcode breakpoint off since it was activated by adding names to the list.
	ui->checkBox_Breakpoint_Opcode->setChecked(false);
	sys->clearOpcodeBreakpoint();
}

void MainWindow::processEvents()
{
	app->processEvents();
}

void MainWindow::closeAllWindows()
{
	app->closeAllWindows();
}

void MainWindow::updatePausedState(bool state/*=true*/){
	if(state)
		ui->pushButton_PauseResume->setText("Resume");
	else
		ui->pushButton_PauseResume->setText("Pause");
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

void MainWindow::setLineEditText(QLineEdit *line, const unsigned int &value){
	line->setText(getQString(uintToStr(value)));
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

void MainWindow::on_checkBox_Breakpoint_PC_stateChanged(int arg1)
{
	if(arg1){
		std::string str = ui->lineEdit_Breakpoint_PC->text().toStdString();
		sys->setBreakpoint((unsigned short)strtoul(str.c_str(), 0, 16));
	}
	else
		sys->clearBreakpoint();
}

void MainWindow::on_checkBox_Breakpoint_Write_stateChanged(int arg1)
{
	if(arg1){
		std::string str = ui->lineEdit_Breakpoint_Write->text().toStdString();
		sys->setMemWriteBreakpoint((unsigned short)strtoul(str.c_str(), 0, 16));
	}
	else
		sys->clearMemWriteBreakpoint();
}

void MainWindow::on_checkBox_Breakpoint_Read_stateChanged(int arg1)
{
	if(arg1){
		std::string str = ui->lineEdit_Breakpoint_Read->text().toStdString();
		sys->setMemReadBreakpoint((unsigned short)strtoul(str.c_str(), 0, 16));
	}
	else
		sys->clearMemReadBreakpoint();
}

void MainWindow::on_checkBox_Breakpoint_Opcode_stateChanged(int arg1)
{
	if(arg1){
		int index = ui->comboBox_Breakpoint_Opcode->currentIndex();
		if(index < 256) // Normal opcodes
			sys->setOpcodeBreakpoint((unsigned char)index);		
		else // CB prefix opcodes
			sys->setOpcodeBreakpoint((unsigned char)(index-256), true);
	}
	else
		sys->clearOpcodeBreakpoint();
}

void MainWindow::on_lineEdit_Breakpoint_PC_editingFinished()
{
	if(ui->checkBox_Breakpoint_PC->isChecked())
		on_checkBox_Breakpoint_PC_stateChanged(1);
	else
		ui->checkBox_Breakpoint_PC->setChecked(true);
}

void MainWindow::on_lineEdit_Breakpoint_Write_editingFinished()
{
	if(ui->checkBox_Breakpoint_Write->isChecked())
		on_checkBox_Breakpoint_Write_stateChanged(1);
	else
		ui->checkBox_Breakpoint_Write->setChecked(true);
}

void MainWindow::on_lineEdit_Breakpoint_Read_editingFinished()
{
	if(ui->checkBox_Breakpoint_Read->isChecked())
		on_checkBox_Breakpoint_Read_stateChanged(1);
	else
		ui->checkBox_Breakpoint_Read->setChecked(true);
}

void MainWindow::on_comboBox_Breakpoint_Opcode_currentIndexChanged(int arg1)
{
	if(ui->checkBox_Breakpoint_Opcode->isChecked())
		on_checkBox_Breakpoint_Opcode_stateChanged(1);
	else
		ui->checkBox_Breakpoint_Opcode->setChecked(true);
}

void MainWindow::on_comboBox_Registers_currentIndexChanged(int arg1)
{
	//updateRegistersTab();
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

void MainWindow::on_spinBox_ScreenScale_valueChanged(int arg1)
{
	components->gpu->getWindow()->setScalingFactor(arg1);
}

void MainWindow::on_pushButton_PauseResume_pressed()
{
	if(!sys->getEmulationPaused()){
		sys->pause();
		updatePausedState(true);
	}
	else{
		sys->unpause();
		updatePausedState(false);
	}
}

void MainWindow::on_pushButton_Step_pressed()
{
	sys->stepThrough();
}

void MainWindow::on_pushButton_Reset_pressed()
{
	sys->reset();
}

void MainWindow::on_pushButton_NextScanline_pressed()
{
	sys->resumeUntilNextHBlank();
}

void MainWindow::on_pushButton_NextFrame_pressed()
{
	sys->resumeUntilNextVBlank();
}

void MainWindow::on_spinBox_SpriteIndex_valueChanged(int arg1)
{

}

void MainWindow::on_checkBox_SoundEnabled_stateChanged(int arg1)
{

}

void MainWindow::on_tabWidget_currentChanged(int index)
{
	this->update();
}

void MainWindow::on_spinBox_MemoryPage_valueChanged(int arg1)
{
	ui->horizontalSlider_MemoryPage->setValue(arg1);
	updateMemoryArray();
	updateMemoryTab();
}

void MainWindow::on_horizontalSlider_MemoryPage_valueChanged(int arg1)
{
	ui->spinBox_MemoryPage->setValue(arg1);
	updateMemoryArray();
}

void MainWindow::on_lineEdit_MemoryByte_editingFinished()
{
	std::string str = ui->lineEdit_MemoryByte->text().toStdString();
	unsigned short byte = strtoul(str.c_str(), 0, 16);
	ui->spinBox_MemoryPage->setValue(byte/128); // This will automatically update the memory array
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
