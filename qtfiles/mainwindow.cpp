#include <iostream>

#include <QApplication>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "OTTWindow.hpp"
#include "Support.hpp"
#include "SystemClock.hpp"
#include "LR35902.hpp"
#include "GPU.hpp"
#include "DmaController.hpp"
#include "Cartridge.hpp"
#include "WorkRam.hpp"
#include "Sound.hpp"
#include "SoundMixer.hpp"

QString getQString(const std::string &str)
{
	return QString(str.c_str());
}

std::string getStdString(const QString& str)
{
	return str.toStdString();
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	bQuitting(false),
	sys(0x0),
	app(0x0)
{
	ui->setupUi(this);
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
		firstUpdate = false;
	}
	switch(ui->tabWidget->currentIndex()){
		case 0: // Main tab
			updateMainTab();
			break;
		case 1: // Graphics tab
			updateInstructionTab();
			break;
		case 2:
			updateGraphicsTab();
			break;
		case 3: // Sprites tab
			updateSpritesTab();
			break;
		case 4: // Sound tab
			updateSoundTab();
			break;
		case 5: // Cartridge tab
			updateCartridgeTab();
			break;
		case 6: // Registers tab
			updateRegistersTab();
			break;
		case 7: // Memory tab
			updateMemoryTab();
			break;
		case 8: // Clock tab
			updateClockTab();
			break;
		case 9: // DMA tab
			updateDmaTab();
		default:
			break;
	}
}

void MainWindow::updateMainTab(){
	// Memory bank indicators
	setLineEditHex(ui->lineEdit_WRAM_Bank, components->wram->getBankSelect());
	
	// Frames per second
	setLineEditText(ui->lineEdit_FPS, components->sclk->getFramerate());
	
	// Emulator CGB mode
	setRadioButtonState(ui->radioButton_CgbMode, bGBCMODE);
	
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
		std::string newinstr = components->cpu->getInstruction();
		history.append(getQString(newinstr+"\n"));
		ui->plainText_Instr_History->setPlainText(history);
	}
}

void MainWindow::updateInstructionTab()
{
	LR35902 *cpu = components->cpu;

	// Interrupt registers
	setRadioButtonState(ui->radioButton_Instr_IME, (*rIME == 1));
	setLineEditText(ui->lineEdit_Instr_IE, getBinary(rIE->getValue()));
	setLineEditText(ui->lineEdit_Instr_IF, getBinary(rIF->getValue()));

	// CPU state
	setRadioButtonState(ui->radioButton_CurrentSpeed, bCPUSPEED);
	setRadioButtonState(ui->radioButton_CpuStopped, sys->cpuIsStopped());
	setRadioButtonState(ui->radioButton_CpuHalted, sys->cpuIsHalted());
	
	// Current instruction being executed
	OpcodeData *op = cpu->getLastOpcode();
	setLineEditText(ui->lineEdit_Instr_Instruction, op->getInstruction());
	setRadioButtonState(ui->radioButton_Instr_Execute, op->onExecute());
	setRadioButtonState(ui->radioButton_Instr_Overtime, op->onOvertime());
	ui->lcdNumber_Instr_Cycles->display(op->cyclesRemaining());
	if(op->memoryAccess()){
		setLineEditHex(ui->lineEdit_Instr_MemAddress, cpu->getMemoryAddress());
		setLineEditHex(ui->lineEdit_Instr_MemValue, cpu->getMemoryValue());
	}
	else{
		setLineEditText(ui->lineEdit_Instr_MemAddress, "");
		setLineEditText(ui->lineEdit_Instr_MemValue, "");
	}
	setRadioButtonState(ui->radioButton_Instr_MemRead, op->onRead());
	setRadioButtonState(ui->radioButton_Instr_MemWrite, op->onWrite());

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
}

void MainWindow::updateGraphicsTab(){
	GPU *gpu = components->gpu;

	// LCD control register (LCDC)
	setRadioButtonState(ui->radioButton_BackgroundEnabled, rLCDC->getBit(0));
	setRadioButtonState(ui->radioButton_SpritesEnabled,    rLCDC->getBit(1));
	setRadioButtonState(ui->radioButton_SpriteSizeSelect,  rLCDC->getBit(2));
	setRadioButtonState(ui->radioButton_BackgroundTilemap, rLCDC->getBit(3));
	setRadioButtonState(ui->radioButton_BgWinTileData,     rLCDC->getBit(4));
	setRadioButtonState(ui->radioButton_WindowEnabled,     rLCDC->getBit(5));
	setRadioButtonState(ui->radioButton_WindowTilemap,     rLCDC->getBit(6));
	setRadioButtonState(ui->radioButton_LcdEnabled,        rLCDC->getBit(7));

	// Color palettes
	//gpu->getDmgPaletteColorHex(bgp*4)
	/*if(bGBCMODE){
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
	}*/
	
	// VRAM bank select
	setLineEditHex(ui->lineEdit_VRAM_Bank, gpu->getBankSelect());
	
	// Set screen region registers
	setLineEditHex(ui->lineEdit_PPU_rSCX, rSCX->getValue());
	setLineEditHex(ui->lineEdit_PPU_rSCY, rSCY->getValue());
	setLineEditHex(ui->lineEdit_PPU_rWX, (unsigned char)(rWX->getValue()-7));
	setLineEditHex(ui->lineEdit_PPU_rWY, rWY->getValue());
	
	// Set scanline registers
	setLineEditText(ui->lineEdit_PPU_rLY, rLY->getValue());
	setLineEditText(ui->lineEdit_PPU_rWLY, rWLY->getValue());
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
	SoundProcessor* sound = components->apu;
	SoundMixer* mixer = sound->getMixer();
	
	// Volume / balance
	ui->checkBox_APU_Mute->setChecked(mixer->isMuted());
	ui->dial_APU_MasterVolume->setValue((int)(mixer->getVolume() * 100));
	//ui->dial_APU_Balance->setValue(mixer->getBalance());
	
	// Audio output channels
	/*ui->checkBox_APU_MasterEnable->setChecked(sound->isEnabled());
	ui->checkBox_APU_Ch1Enable->setChecked(sound->isChannelEnabled(1));
	ui->checkBox_APU_Ch2Enable->setChecked(sound->isChannelEnabled(2));
	ui->checkBox_APU_Ch3Enable->setChecked(sound->isChannelEnabled(3));
	ui->checkBox_APU_Ch4Enable->setChecked(sound->isChannelEnabled(4));*/
	
	// Audio DACs
	setRadioButtonState(ui->radioButton_APU_Ch1, sound->isDacEnabled(1));
	setRadioButtonState(ui->radioButton_APU_Ch2, sound->isDacEnabled(2));
	setRadioButtonState(ui->radioButton_APU_Ch3, sound->isDacEnabled(3));
	setRadioButtonState(ui->radioButton_APU_Ch4, sound->isDacEnabled(4));
	
	// Length values
	setLineEditText(ui->lineEdit_APU_Ch1Length, sound->getChannelTime(1));
	setLineEditText(ui->lineEdit_APU_Ch2Length, sound->getChannelTime(2));
	setLineEditText(ui->lineEdit_APU_Ch3Length, sound->getChannelTime(3));
	setLineEditText(ui->lineEdit_APU_Ch4Length, sound->getChannelTime(4));

	// Frequencies	
	setLineEditText(ui->lineEdit_APU_Ch1Frequency, sound->getChannelFrequency(1));
	setLineEditText(ui->lineEdit_APU_Ch2Frequency, sound->getChannelFrequency(2));
	setLineEditText(ui->lineEdit_APU_Ch3Frequency, sound->getChannelFrequency(3));
	setLineEditText(ui->lineEdit_APU_Ch4Frequency, sound->getChannelFrequency(4));
}

void MainWindow::updateCartridgeTab(){
	Cartridge *cart = components->cart;

	// Cartridge header information
	setLineEditText(ui->lineEdit_RomPath, sys->getRomFilename());
	setLineEditText(ui->lineEdit_RomTitle, cart->getTitleString());
	setLineEditHex(ui->lineEdit_RomEntryPoint, cart->getProgramEntryPoint());
	setLineEditText(ui->lineEdit_RomSize, cart->getRomSize());
	setLineEditText(ui->lineEdit_RomSramSize, cart->getRamSize());
	setLineEditText(ui->lineEdit_RomType, cart->getCartridgeType());
	setLineEditText(ui->lineEdit_RomLanguage, cart->getLanguage());
	
	// Cartridge status
	setLineEditHex(ui->lineEdit_ROM_Bank, cart->getBankSelect());
	setLineEditHex(ui->lineEdit_SRAM_Bank, cart->getRam()->getBankSelect());
	setRadioButtonState(ui->radioButton_RomSramEnabled, cart->getExternalRamEnabled());
	setRadioButtonState(ui->radioButton_RomCgbEnabled, cart->getSupportCGB());
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
	memoryPtr = sys->getConstPtr(currentByte);
	QString str;
	for(unsigned short i = 0; i < 8; i++){
		str.append(getQString(getHex(currentByte))+" ");
		for(unsigned short j = 0; j < 16; j++){
			str.append(getQString(getHex(memoryPtr[8*i+j]))+" ");
		}
		str.append("\n");
		currentByte += 16;
	}
	ui->textBrowser_Memory->setPlainText(str);
}

void MainWindow::updateClockTab()
{
	SystemClock *sclk = components->sclk;

	setLineEditText(ui->lineEdit_Clock_Frequency, sclk->getCyclesPerSecond()/1E6);
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

void MainWindow::updateDmaTab()
{
	DmaController *dma = components->dma;

	setLineEditText(ui->lineEdit_DMA_BytesRemaining, dma->getNumBytesRemaining());
	setLineEditText(ui->lineEdit_DMA_CyclesRemaining, dma->getNumCyclesRemaining());
	setLineEditText(ui->lineEdit_DMA_BytesPerCycle, dma->getNumBytesPerCycle());
	
	setLineEditHex(ui->lineEdit_DMA_SourceStart, dma->getSourceStartAddress());
	setLineEditHex(ui->lineEdit_DMA_SourceStop, dma->getSourceEndAddress());
	setLineEditHex(ui->lineEdit_DMA_DestStart, dma->getDestinationStartAddress());
	setLineEditHex(ui->lineEdit_DMA_DestStop, dma->getDestinationEndAddress());
	setLineEditHex(ui->lineEdit_DMA_CurrentIndex, dma->getCurrentMemoryIndex());

	setRadioButtonState(ui->radioButton_DMA_TransferActive, dma->active());
	
	unsigned char type = dma->getTransferMode();
	setRadioButtonState(ui->radioButton_DMA_OAM, (type == 0));
	setRadioButtonState(ui->radioButton_DMA_General, (type == 1));
	setRadioButtonState(ui->radioButton_DMA_HBlank, (type == 2));
	
	ui->progressBar_DMA->setValue(100*(1.0 - 1.0*dma->getNumBytesRemaining()/dma->getTotalLength()));
}

void MainWindow::updateMemoryArray()
{
	unsigned short memLow = 0x80*ui->spinBox_MemoryPage->value();
	unsigned short memHigh = memLow + 0x80;
	setLineEditHex(ui->lineEdit_MemoryPageLow, memLow);
	setLineEditHex(ui->lineEdit_MemoryPageHigh, memHigh);
}

void MainWindow::setDmgMode(){
	ui->radioButton_CurrentSpeed->setEnabled(false);
	/*ui->lineEdit_BGP_0->setEnabled(false);
	ui->lineEdit_BGP_1->setEnabled(false);
	ui->lineEdit_BGP_2->setEnabled(false);
	ui->lineEdit_BGP_3->setEnabled(false);
	ui->lineEdit_OBP_0->setEnabled(false);
	ui->lineEdit_OBP_1->setEnabled(false);
	ui->lineEdit_OBP_2->setEnabled(false);
	ui->lineEdit_OBP_3->setEnabled(false);
	ui->spinBox_BGP->setEnabled(false);
	ui->spinBox_OBP->setEnabled(false);*/
	ui->lineEdit_VRAM_Bank->setEnabled(false);
}

void MainWindow::connectToSystem(SystemGBC *ptr){ 
	sys = ptr; 
	components = std::unique_ptr<ComponentList>(new ComponentList(ptr));
	ui->comboBox_Registers->addItem("ALL");
	ui->comboBox_Registers->addItem("System");
	for(auto component = components->list.begin(); component != components->list.end(); component++){
		ui->comboBox_Registers->addItem(getQString(component->second->getName()));
	}
	Opcode *opcodes = components->cpu->getOpcodes();
	for(unsigned short i = 0; i < 256; i++){
		ui->comboBox_Breakpoint_Opcode->addItem(getQString(opcodes[i].sName));
	}
	opcodes = components->cpu->getOpcodesCB();
	for(unsigned short i = 0; i < 256; i++){
		ui->comboBox_Breakpoint_Opcode->addItem(getQString(opcodes[i].sName));
	}
	// Toggle opcode breakpoint off since it was activated by adding names to the list.
	ui->checkBox_Breakpoint_Opcode->setChecked(false);
	sys->clearOpcodeBreakpoint();
}

void MainWindow::processEvents(){
	app->processEvents();
}

void MainWindow::closeAllWindows(){
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
	if(arg1)
		components->gpu->enableRenderLayer(0);
	else
		components->gpu->disableRenderLayer(0);
}

void MainWindow::on_checkBox_Window_stateChanged(int arg1)
{
	if(arg1)
		components->gpu->enableRenderLayer(1);
	else
		components->gpu->disableRenderLayer(1);
}

void MainWindow::on_checkBox_Sprites_stateChanged(int arg1)
{
	if(arg1)
		components->gpu->enableRenderLayer(2);
	else
		components->gpu->disableRenderLayer(2);
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
		std::string str = toUppercase(ui->lineEdit_Breakpoint_Write->text().toStdString());
		auto registers = sys->getRegisters();
		for(auto reg = registers->begin(); reg != registers->end(); reg++){
			if(str == reg->getName()){
				sys->setMemWriteBreakpoint(reg->getAddress());
				return;
			}
		}
		sys->setMemWriteBreakpoint((unsigned short)strtoul(str.c_str(), 0, 16));
	}
	else
		sys->clearMemWriteBreakpoint();
}

void MainWindow::on_checkBox_Breakpoint_Read_stateChanged(int arg1)
{
	if(arg1){
		std::string str = toUppercase(ui->lineEdit_Breakpoint_Read->text().toStdString());
		auto registers = sys->getRegisters();
		for(auto reg = registers->begin(); reg != registers->end(); reg++){
			if(str == reg->getName()){
				sys->setMemReadBreakpoint(reg->getAddress());
				return;
			}
		}
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

void MainWindow::on_lineEdit_PaletteSelect_editingFinished()
{
	components->gpu->setColorPaletteDMG((unsigned short)std::stoul(ui->lineEdit_PaletteSelect->text().toStdString(), 0, 16));
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

void MainWindow::on_spinBox_Frameskip_valueChanged(int arg1)
{
	sys->setFrameSkip(arg1);
}

void MainWindow::on_spinBox_ScreenScale_valueChanged(int arg1)
{
}

void MainWindow::on_doubleSpinBox_Clock_Multiplier_editingFinished()
{
	double freq = ui->doubleSpinBox_Clock_Multiplier->value();
	if(freq > 0)
		sys->setFramerateMultiplier(freq);
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

void MainWindow::on_pushButton_Advance_pressed()
{
	sys->advanceClock();
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

void MainWindow::on_pushButton_Refresh_pressed()
{
	update();
}

void MainWindow::on_pushButton_PPU_TileViewer_pressed()
{
	sys->openTileViewer();
	ui->pushButton_PPU_TileViewer->setEnabled(false);
}

void MainWindow::on_pushButton_PPU_LayerViewer_pressed()
{
	sys->openLayerViewer();
	ui->pushButton_PPU_LayerViewer->setEnabled(false);
}

void MainWindow::on_spinBox_SpriteIndex_valueChanged(int arg1)
{

}

void MainWindow::on_radioButton_PPU_Map0_clicked()
{
	ui->radioButton_PPU_Map0->setChecked(true);
	ui->radioButton_PPU_Map1->setChecked(false);
}

void MainWindow::on_radioButton_PPU_Map1_clicked()
{
	ui->radioButton_PPU_Map1->setChecked(true);
	ui->radioButton_PPU_Map0->setChecked(false);
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
// APU
/////////////////////////////////////////////////////////////////////

void MainWindow::on_dial_APU_MasterVolume_valueChanged(int arg1)
{
	components->apu->getMixer()->setVolume(arg1 / 100.f);
}

void MainWindow::on_dial_APU_AudioBalance_valueChanged(int arg1)
{
	components->apu->getMixer()->setBalance(-1.f + (100 + arg1) / 100.f);
}

void MainWindow::on_checkBox_APU_MasterEnable_clicked(bool arg1)
{
	SoundMixer* mixer = components->apu->getMixer();
	if(arg1){
		ui->checkBox_APU_Ch1Enable->setEnabled(true);
		ui->checkBox_APU_Ch2Enable->setEnabled(true);
		ui->checkBox_APU_Ch3Enable->setEnabled(true);
		ui->checkBox_APU_Ch4Enable->setEnabled(true);
		mixer->mute();
		mixer->setChannelVolume(0, 1.f);
		mixer->setChannelVolume(1, 1.f);
		mixer->setChannelVolume(2, 1.f);
		mixer->setChannelVolume(3, 1.f);
	}
	else{
		ui->checkBox_APU_Ch1Enable->setEnabled(false);
		ui->checkBox_APU_Ch2Enable->setEnabled(false);
		ui->checkBox_APU_Ch3Enable->setEnabled(false);
		ui->checkBox_APU_Ch4Enable->setEnabled(false);
		mixer->mute();
		mixer->setChannelVolume(0, 0.f);
		mixer->setChannelVolume(1, 0.f);
		mixer->setChannelVolume(2, 0.f);
		mixer->setChannelVolume(3, 0.f);
	}
}

void MainWindow::on_checkBox_APU_Ch1Enable_clicked(bool arg1)
{
	SoundMixer* mixer = components->apu->getMixer();
	if(arg1) // On
		mixer->setChannelVolume(0, 1.f);
	else // Off
		mixer->setChannelVolume(0, 0.f);
}

void MainWindow::on_checkBox_APU_Ch2Enable_clicked(bool arg1)
{
	SoundMixer* mixer = components->apu->getMixer();
	if(arg1) // On
		mixer->setChannelVolume(1, 1.f);
	else // Off
		mixer->setChannelVolume(1, 0.f);
}

void MainWindow::on_checkBox_APU_Ch3Enable_clicked(bool arg1)
{
	SoundMixer* mixer = components->apu->getMixer();
	if(arg1) // On
		mixer->setChannelVolume(2, 1.f);
	else // Off
		mixer->setChannelVolume(2, 0.f);
}

void MainWindow::on_checkBox_APU_Ch4Enable_clicked(bool arg1)
{
	SoundMixer* mixer = components->apu->getMixer();
	if(arg1) // On
		mixer->setChannelVolume(3, 1.f);
	else // Off
		mixer->setChannelVolume(3, 0.f);
}

void MainWindow::on_checkBox_APU_Mute_clicked()
{
	components->apu->getMixer()->mute();
}

void MainWindow::on_radioButton_APU_Stereo_clicked()
{
	components->apu->getMixer()->setStereoOutput();
}

void MainWindow::on_radioButton_APU_Mono_clicked()
{
	components->apu->getMixer()->setMonoOutput();
}

void MainWindow::on_pushButton_APU_ClockSequencer_pressed()
{
	// Not implemented yet
}

void MainWindow::on_lineEdit_APU_Ch1Frequency_textChanged(QString arg1)
{
	float freq = std::stof(arg1.toStdString()); // Frequency (Hz)
	std::string text;
	if(keyboard.getName(freq, text) <= 0.01f) // <= 1% absolute difference in frequency
		setLineEditText(ui->lineEdit_APU_Ch1Key, text);
	else // > 1% absolute difference
		setLineEditText(ui->lineEdit_APU_Ch1Key, text + "?");
}

void MainWindow::on_lineEdit_APU_Ch2Frequency_textChanged(QString arg1)
{
	float freq = std::stof(arg1.toStdString()); // Frequency (Hz)
	std::string text;
	if(keyboard.getName(freq, text) <= 0.01f) // <= 1% absolute difference in frequency
		setLineEditText(ui->lineEdit_APU_Ch2Key, text);
	else // > 1% absolute difference
		setLineEditText(ui->lineEdit_APU_Ch2Key, text + "?");
}

void MainWindow::on_lineEdit_APU_Ch3Frequency_textChanged(QString arg1)
{
	float freq = std::stof(arg1.toStdString()); // Frequency (Hz)
	std::string text;
	if(keyboard.getName(freq, text) <= 0.01f) // <= 1% absolute difference in frequency
		setLineEditText(ui->lineEdit_APU_Ch3Key, text);
	else // > 1% absolute difference
		setLineEditText(ui->lineEdit_APU_Ch3Key, text + "?");
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
	bQuitting = true;
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
	sys->saveSRAM("sram.dat");
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

void MainWindow::on_actionWrite_Save_Data_triggered()
{
	sys->writeExternalRam();
}

void MainWindow::on_actionRead_Save_Data_triggered()
{
	sys->readExternalRam();
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
