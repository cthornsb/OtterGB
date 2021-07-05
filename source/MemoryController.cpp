#include "MemoryController.hpp"
#include "SystemComponent.hpp"
#include "Cartridge.hpp"
#include "Support.hpp"

namespace mbcs{

bool MBC1::writeRegister(const unsigned short &reg, const unsigned char &val) {
	// Write to MBC register
	if (reg < 0x2000) // RAM enable
		rRamEnable.write(val);
	else if (reg < 0x4000) // ROM bank number low 5 bits
		rRomBankLow.write(val);
	else if (reg < 0x6000) // ROM bank number high 2 bits (or RAM bank number)
		rRomBankHigh.write(val);
	else if (reg < 0x8000) // ROM / RAM bank mode select
		rBankModeSelect.write(val);
	else
		return false;

	// RAM enabled
	bRamEnabled = (rRamEnable.getBits(0, 3) == 0x0a);

	// Low 5 bits of ROM bank (MBC1 reads bank 0 as bank 1)
	unsigned char bankLow = rRomBankLow.getBits(0, 4);
	if (bankLow == 0)
		bankLow = 1;

	// ROM / RAM select
	unsigned short bankSelected = 0;
	if (!rBankModeSelect.getBit(0)) { // Mode 0: Simple ROM banking. Specify bits 5 & 6 of the ROM bank number.
		bankSelected = (rRomBankHigh.getBits(0, 1) << 5) + bankLow;
		setBank(0);
	}
	else{ // Mode1: RAM bank or advanced ROM banking
		bankSelected = bankLow;
		setBank(rRomBankHigh.getBits(0, 1));
	}

	// Set ROM bank, masked with the maximum number of bits in the event that the selected bank was out of range.
	cart->setBank(bankSelected < cart->getNumberOfBanks() ? bankSelected : bankSelected & (cart->getNumberOfBanks() - 1));
	
	return true;
}

bool MBC2::writeRegister(const unsigned short &reg, const unsigned char &val) {
	// Write to MBC register
	if (reg < 0x4000) // RAM enable and ROM bank number
		rRomBank.write(val);
	else
		return false;

	if (!bitTest(reg, 8)) { // RAM enable / disable
		bRamEnabled = (rRomBank == 0x0a);
	}
	else{ // ROM bank number
		unsigned char bankSelected = rRomBank.getBits(0, 3);
		cart->setBank(bankSelected != 0 ? bankSelected : 1);
	}
	
	return true;
}

bool MBC3::writeRegister(const unsigned short &reg, const unsigned char &val) {
	// Write to MBC register
	if (reg < 0x2000) // RAM and timer enable
		rRamEnable.write(val);
	else if (reg < 0x4000) // ROM bank number
		rRomBank.write(val);
	else if (reg < 0x6000) // RAM bank select or RTC register select
		rRamBank.write(val);
	else if (reg < 0x8000) // Latch clock data
		rLatch.write(val);
	else
		return false;

	// RAM and timer enabled
	bRamEnabled = (rRamEnable.getBits(0, 3) == 0x0a);

	// 7 bit ROM bank (MBC3 reads bank 0 as bank 1, like MBC1)
	unsigned char bankSelected = rRomBank.getBits(0, 6);
	cart->setBank(bankSelected != 0 ? bankSelected : 1);

	// RAM bank select or RTC register select
	registerSelect = 0x0;
	unsigned char ramBankSelect = rRamBank.getBits(0, 3); // (0-3: RAM bank, 8-C: RTC register)
	if (ramBankSelect <= 0x3) // RAM bank
		setBank(ramBankSelect);
	else {
		switch (ramBankSelect) {
		case 0x8:
			registerSelect = &rSeconds;
			break;
		case 0x9:
			registerSelect = &rMinutes;
			break;
		case 0xa:
			registerSelect = &rHours;
			break;
		case 0xb:
			registerSelect = &rDayLow;
			break;
		case 0xc:
			registerSelect = &rDayHigh;
			break;
		default:
			break;
		}
	}

	// Latch clock data, write current clock time to RTC registers
	bool nextLatchState = rLatch.getBit(0);
	if (!bLatchState && nextLatchState) { // 0->1, latch data
		unsigned int nDays = nRtcTimerSeconds / 86400; // Number of days elapsed (< 512)
		unsigned int nSeconds = nRtcTimerSeconds % 86400; // Number of seconds into the current day
		unsigned char nTimeHours = nSeconds / 3600;
		unsigned char nTimeMinutes = (nSeconds % 3600) / 60;
		unsigned char nTimeSeconds = (nSeconds % 3600) % 60;
		rSeconds.setValue(nTimeSeconds); // Seconds
		rMinutes.setValue(nTimeMinutes); // Minutes
		rHours.setValue(nTimeHours); // Hours
		rDayLow.setValue((unsigned char)(nDays & 0x000000ff)); // Low 8-bits of day counter
		if (nDays >= 256)
			rDayHigh.setBit(0); // Upper 1-bit of day
		else
			rDayHigh.resetBit(0); // Upper 1-bit of day
	}
	bLatchState = nextLatchState;
	
	return true;
}

bool MBC3::onClockUpdate() {
	if (rtcTimer.clock()) { // Increment the RTC second counter
		if (rtcTimer.getTimerCounter() >= 32768) { // One tick per second
			if (++nRtcTimerSeconds >= 44236800) //44236800 seconds max (512 days)
				nRtcTimerSeconds %= 44236800;
			rtcTimer.resetCounter();
		}
		return true;
	}
	return false;
}

bool MBC3::writeToRam(const unsigned short& addr, const unsigned char& value) {
	if (registerSelect) { // RTC register
		registerSelect->write(value);
		if (registerSelect->getName() == "MBC3_DAYHIGH") {
			if (registerSelect->bit6()) // Halt RTC timer
				rtcTimer.disableTimer();
			else // Resume RTC timer
				rtcTimer.enableTimer();
		}
		return true;
	}
	return false;
}

bool MBC3::readFromRam(const unsigned short& addr, unsigned char& value) {
	if (registerSelect) { // RTC register	
		registerSelect->read(value);
		return true;
	}
	return false;
}

bool MBC5::writeRegister(const unsigned short &reg, const unsigned char &val) {
	// Write to MBC register
	if (reg < 0x2000) // RAM enable
		rRamEnable.write(val);
	else if (reg < 0x3000) // ROM bank number low 8 bits
		rRomBankLow.write(val);
	else if (reg < 0x4000) // ROM bank number high bit
		rRomBankHigh.write(val);
	else if (reg < 0x6000) // RAM bank number
		rRamBank.write(val);
	else
		return false;

	// RAM enabled
	bRamEnabled = (rRamEnable.getBits(0, 3) == 0x0a);
	
	// ROM bank (MBC5 reads bank 0 as bank 0, unlike MBC1)
	unsigned short bank = (unsigned short)rRomBankLow.getValue() + (rRomBankHigh.getBit(0) ? 0x0100 : 0x0);
	if (bank >= cart->getNumberOfBanks()) {
		bank &= (cart->getNumberOfBanks() - 1);
	}
	cart->setBank(bank);
	
	// RAM bank
	unsigned short ramBank = 0;
	if (!bRumbleSupport) { // No rumble pak
		ramBank = (unsigned short)rRamBank.getBits(0, 3);
	}
	else{ // Rumble pak is present
		ramBank = (unsigned short)rRamBank.getBits(0, 2);
		/*if (rRamBank.getBit(3)) { // Activate rumble
		}
		else{ // Deactivate rumble
		}*/
	}
	setBank(ramBank);
	
	return true;
}

} // namespace mbcs

