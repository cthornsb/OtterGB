#include <iostream>
#include <cstring>
#include <cmath>

#include "Support.hpp"
#include "MidiFile.hpp"
#include "PianoKeys.hpp"

using namespace MidiFile;

using namespace PianoKeys;

/** Reverse the order of bytes of a 16-bit integer and return the result
  */
unsigned short reverseByteOrder(const unsigned short& input){
	unsigned short retval = 0;
	for(int i = 0; i < 2; i++){
		retval += ((input & (0xff << (8 * i))) >> (8 * i)) << (16 - 8 * (i + 1));
	}
	return retval;
}

/** Reverse the order of bytes of a 32-bit integer and return the result
  */
unsigned int reverseByteOrder(const unsigned int& input){
	unsigned int retval = 0;
	for(int i = 0; i < 4; i++){
		retval += ((input & (0xff << (8 * i))) >> (8 * i)) << (32 - 8 * (i + 1));
	}
	return retval;
}

/** Reverse the order of bytes of an input N-bit integer and return the result
  */
unsigned int reverseByteOrder(const unsigned int& input, const char& N){
	unsigned int retval = 0;
	for(int i = 0; i < 4; i++){
		retval += ((input & (0xff << (8 * i))) >> (8 * i)) << (N - 8 * (i + 1));
	}
	return retval;
}

MidiKeyboard::MidiKeyboard() {
	// 128 notes, C-1 to G9
	// One octave: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
	for(int i = -1; i <= 9; i++){ // Over 9 octaves
		frequencies.push_back(getFrequency(Key::C, Modifier::NONE,  i));
		frequencies.push_back(getFrequency(Key::C, Modifier::SHARP, i));
		frequencies.push_back(getFrequency(Key::D, Modifier::NONE,  i));
		frequencies.push_back(getFrequency(Key::D, Modifier::SHARP, i));
		frequencies.push_back(getFrequency(Key::E, Modifier::NONE,  i));
		frequencies.push_back(getFrequency(Key::F, Modifier::NONE,  i));
		frequencies.push_back(getFrequency(Key::F, Modifier::SHARP, i));
		frequencies.push_back(getFrequency(Key::G, Modifier::NONE,  i));
		if(i < 9){
			frequencies.push_back(getFrequency(Key::G, Modifier::SHARP, i));
			frequencies.push_back(getFrequency(Key::A, Modifier::NONE,  i + 1));
			frequencies.push_back(getFrequency(Key::A, Modifier::SHARP, i + 1));
			frequencies.push_back(getFrequency(Key::B, Modifier::NONE,  i + 1));
		}
	}
}

float MidiKeyboard::getKeyNumber(const float& freq, unsigned char& key) const {
	size_t closestMatch = 0;
	float delta = 999.f; // Percentage difference between input frequency
	for(size_t i = 0; i != frequencies.size(); i++){
		if(frequencies[i] == freq){ // Exact match is unlikely but possible
			key = (unsigned char)i;
			return 0.f;
		}
		float df = std::abs(2.f * std::abs(frequencies[i] - freq) / (frequencies[i] + freq));
		if(df < delta){
			closestMatch = i;
			delta = df;
		}
	}
	key = (unsigned char)closestMatch;
	return delta;
}

bool MidiChunk::getUChar(unsigned char& val) {
	if(nIndex + 1 >= nLength)
		return false;
	val = data[nIndex++];
	return true;
}

bool MidiChunk::getUShort(unsigned short& val) {
	if(nIndex + 2 >= nLength)
		return false;
	memcpy(static_cast<void*>(&val), static_cast<void*>(&data[nIndex]), 2);
	val = reverseByteOrder(val); // High byte first
	nIndex += 2;
	return true;
}

bool MidiChunk::getUInt(unsigned int& val) {
	if(nIndex + 4 >= nLength)
		return false;
	memcpy(static_cast<void*>(&val), static_cast<void*>(&data[nIndex]), 4);
	val = reverseByteOrder(val); // High byte first
	nIndex += 4;
	return true;
}
		
bool MidiChunk::getString(std::string& str, const unsigned int& len){
	if(nIndex + len >= nLength)
		return false;
	str = std::string(len, ' '); // Reserve space in string
	memcpy(static_cast<void*>(&str[0]), static_cast<void*>(&data[nIndex]), len);
	nIndex += len;
	return true;
}

bool MidiChunk::copyMemory(void* dest, const unsigned int& len){
	if(nIndex + len >= nLength)
		return false;
	memcpy(dest, static_cast<void*>(&data[nIndex]), len);
	nIndex += len;
	return true;
}
		
void MidiChunk::pushUChar(const unsigned char& val){
	data.push_back(val);
	nLength++;
}
		
void MidiChunk::pushUShort(const unsigned short& val){
	// MSB first!
	data.push_back((unsigned char)((val & 0xff00) >> 8));
	data.push_back((unsigned char)(val & 0x00ff));
	nLength += 2;
}

void MidiChunk::pushUInt(const unsigned int& val){
	// MSB first!
	data.push_back((unsigned char)((val & 0xff000000) >> 24));
	data.push_back((unsigned char)((val & 0x00ff0000) >> 16));
	data.push_back((unsigned char)((val & 0x0000ff00) >> 8));
	data.push_back((unsigned char)(val & 0x000000ff));
	nLength += 4;
}

void MidiChunk::pushString(const std::string& str){
	for(size_t i = 0; i < str.length(); i++){
		data.push_back((unsigned char)str[i]);
		nLength++;
	}
}

void MidiChunk::pushMemory(const unsigned int& src, const unsigned int& len){
	//data.push_back((unsigned char)((src & 0xff000000) >> 24));
	data.push_back((unsigned char)((src & 0x00ff0000) >> 16));
	data.push_back((unsigned char)((src & 0x0000ff00) >> 8));
	data.push_back((unsigned char)(src & 0x000000ff));
	nLength += len;
}

void MidiChunk::pushVariableSize(const unsigned int& val){
	// Maximum size is 28-bits (0x0fffffff)
	unsigned char nibs[4] = { // MSB first
		(unsigned char)((val & 0x0fe00000) >> 21),
		(unsigned char)((val & 0x001fc000) >> 14),
		(unsigned char)((val & 0x00003f80) >> 7),
		(unsigned char)((val & 0x0000007f))
	};
	if(nibs[3] != 0){ // Write 4 bytes
		pushUChar(nibs[0] | 0x80);
		pushUChar(nibs[1] | 0x80);
		pushUChar(nibs[2] | 0x80);
		pushUChar(nibs[3] & 0x7f);
	}
	else if(nibs[2] != 0){ // 3 bytes
		pushUChar(nibs[0] | 0x80);
		pushUChar(nibs[1] | 0x80);
		pushUChar(nibs[2] & 0x7f);
	}
	else if(nibs[1] != 0){ // 2 bytes
		pushUChar(nibs[0] | 0x80);
		pushUChar(nibs[1] & 0x7f);
	}
	else{ // 1 byte
		pushUChar(nibs[0] & 0x7f);
	}
	// This is less clear and only slightly shorter
	/*int lastByte = 0;
	for(int i = 3; i > 0; i--){ // Find last non-zero byte
		if(nibs[i]){
			lastByte = i;
			break;
		}
	}
	for(int i = 0; i <= lastByte; i++){ // Write the bytes
		if(i != lastByte)
			pushUChar(nibs[i] & 0x7f);
		else
			pushUChar(nibs[i] | 0x80);
	}*/
}

bool MidiChunk::readMidiChunk(std::ifstream& f){
	f.read((char*)&sType[0], 4); // Read chunk type
	f.read((char*)&nLength, 4); // Read header length, MSB first
	nLength = reverseByteOrder(nLength);
	if(f.eof() || !nLength)
		return false;
	data.reserve(nLength);
	f.read((char*)data.data(), nLength);
	nIndex = 0; // Reset index
	return (!f.eof());
}

bool MidiChunk::writeMidiChunk(std::ofstream& f){
	if(!f.good())
		return false;
	f.write(sType.data(), 4); // Write chunk type
	unsigned char lengthBytes[4] = {
		(unsigned char)((nLength & 0xff000000) >> 24),
		(unsigned char)((nLength & 0x00ff0000) >> 16),
		(unsigned char)((nLength & 0x0000ff00) >> 8),
		(unsigned char)(nLength & 0x000000ff)
	};
	for(int i = 0; i < 4; i++){ // Write header length, MSB first
		f.write((char*)&lengthBytes[i], 1);
	}
	f.write((char*)data.data(), nLength); // Write chunk body
	return true;
}

void MidiChunk::clear(){
	sType = "    ";
	nLength = 0;
	data.clear();
}

unsigned int MidiChunk::readVariableLength(MidiChunk& chunk){
	unsigned int retval = 0;
	unsigned char byte = 0;
	int chunkCount = 0;
	while(true){ // Read 7-bit chunks
		if(!chunk.getUChar(byte)) // Not enough data
			break;
		retval += ((byte & 0x7f) << (7 * chunkCount++));
		if(!bitTest(byte, 7)){ // Read bytes until one is encountered with its 7th bit cleared
			break;
		}
	}
	return retval;
}
	
bool MidiMessage::read(MidiChunk& chunk){
	unsigned char byte;
	chunk.getUChar(byte);
	nChannel = (byte & 0xf);
	switch((byte & 0x70) >> 4){
	case 0: // Note released
		chunk.getUChar(nKeyNumber);
		chunk.getUChar(nVelocity);
		nStatus = MidiStatusType::RELEASED;
		break;
	case 1: // Note pressed
		chunk.getUChar(nKeyNumber);
		chunk.getUChar(nVelocity);
		nStatus = MidiStatusType::PRESSED;
		break;
	case 2: // Polyphonic key pressure
		chunk.getUChar(nKeyNumber);
		chunk.getUChar(nVelocity);
		nStatus = MidiStatusType::POLYPRESSURE;
		break;
	case 3: // Control change
		chunk.skipBytes(2);
		nStatus = MidiStatusType::CONTROLCHANGE;
		break;
	case 4: // Program change
		chunk.skipBytes(1);
		nStatus = MidiStatusType::PROGRAMCHANGE;
		break;
	case 5: // Channel pressure
		chunk.skipBytes(1);
		nStatus = MidiStatusType::CHANPRESSURE;
		break;
	case 6: // Pitch wheel change
		chunk.skipBytes(2);
		nStatus = MidiStatusType::PITCHCHANGE;
		break;
	default:
		return false;
	};		
	return true;	
}

bool MidiMessage::write(MidiChunk& chunk){
	// Write delta-time
	chunk.pushVariableSize(nDeltaTime);
	switch(nStatus){
	case MidiStatusType::RELEASED: // Note released
		chunk.pushUChar(0x80 | (0x0 << 4) | nChannel); // Status byte
		chunk.pushUChar(nKeyNumber);
		chunk.pushUChar(nVelocity);
		break;
	case MidiStatusType::PRESSED: // Note pressed
		chunk.pushUChar(0x80 | (0x1 << 4) | nChannel); // Status byte
		chunk.pushUChar(nKeyNumber);
		chunk.pushUChar(nVelocity);
		break;
	case MidiStatusType::POLYPRESSURE: // Polyphonic key pressure
		chunk.pushUChar(0x80 | (0x2 << 4) | nChannel); // Status byte
		chunk.pushUChar(nKeyNumber);
		chunk.pushUChar(nVelocity);
		break;
	case MidiStatusType::CONTROLCHANGE: // Control change
		chunk.pushUChar(0x80 | (0x3 << 4) | nChannel); // Status byte
		chunk.pushUShort(0);
		break;
	case MidiStatusType::PROGRAMCHANGE: // Program change
		chunk.pushUChar(0x80 | (0x4 << 4) | nChannel); // Status byte
		chunk.pushUChar(0);
		break;
	case MidiStatusType::CHANPRESSURE: // Channel pressure
		chunk.pushUChar(0x80 | (0x5 << 4) | nChannel); // Status byte
		chunk.pushUChar(0);
		break;
	case MidiStatusType::PITCHCHANGE: // Pitch wheel change
		chunk.pushUChar(0x80 | (0x6 << 4) | nChannel); // Status byte
		chunk.pushUShort(0);
		break;
	default:
		return false;
	};	
	return true;
}

bool MidiSysExclusive::read(MidiChunk& chunk){
	unsigned char byte;
	chunk.getUChar(byte); // 0xf0
	nLength = MidiChunk::readVariableLength(chunk);
	chunk.skipBytes(nLength);
	return true;
}
	
bool MidiMetaEvent::read(MidiChunk& chunk){
	unsigned int nTempo = 0;
	std::string message;
	unsigned char byte;
	chunk.getUChar(byte); // 0xff
	chunk.getUChar(nType);
	nLength = MidiChunk::readVariableLength(chunk);
	switch(nType){
	case 0x00: // Sequence number
		chunk.skipBytes(2);
		break;
	case 0x01: // Text event
		chunk.getString(message, nLength);
		std::cout << " Text=" << message << std::endl;
		break;
	case 0x02: // Copyright
		chunk.getString(message, nLength);
		std::cout << " Copyright=" << message << std::endl;
		break;
	case 0x03: // Sequencer/Track name
		chunk.getString(message, nLength);
		std::cout << " Track=" << message << std::endl;
		break;
	case 0x04: // Instrument name
		chunk.getString(message, nLength);
		std::cout << " Instrument=" << message << std::endl;
		break;
	case 0x05: // Lyrics
		chunk.getString(message, nLength);
		std::cout << " Lyrics=" << message << std::endl;
		break;
	case 0x06: // Marker
		chunk.getString(message, nLength);
		std::cout << " Marker=" << message << std::endl;
		break;
	case 0x07: // Cue point
		chunk.getString(message, nLength);
		std::cout << " Cue=" << message << std::endl;
		break;
	case 0x20: // Midi channel prefix
		chunk.skipBytes(1);
		break;
	case 0x2f: // End of track
		std::cout << " END OF TRACK\n";
		break;
	case 0x51: // Tempo
		// 500000 is default
		chunk.copyMemory(static_cast<void*>(&nTempo), 3); // 24-bit (microseconds / quarter-note)
		nTempo = reverseByteOrder(nTempo, 24); // High byte first
		std::cout << " Tempo=" << nTempo << " (" << 60.f / (nTempo * 1E-6) << " bps)" << std::endl;
		break;
	case 0x54: // SMTPE offset
		chunk.skipBytes(5);
		break;
	case 0x58: // Time signature
		std::cout << " Time Signature:\n";
		chunk.getUChar(byte);
		std::cout << "  " << (int)byte << "/";
		chunk.getUChar(byte);
		std::cout << std::pow(2, byte) << std::endl;
		chunk.getUChar(byte);
		std::cout << "  " << (int)byte << " midi clocks per metronome tick\n";
		chunk.getUChar(byte);
		std::cout << "  " << (int)byte << " 32nd notes per 24 midi clocks\n";
		break;
	case 0x59: // Key signature
		chunk.skipBytes(2);
		break;
	case 0x7f: // Sequencer meta event
		chunk.skipBytes(nLength);
		break;
	default:
		chunk.skipBytes(nLength);
		break;
	};
	return true;
}

bool TrackEvent::read(MidiChunk& chunk){
	// Read a byte
	if(chunk.getBytesRemaining() < 2)
		return false;
	nDeltaTime = MidiChunk::readVariableLength(chunk);
	unsigned char byte = chunk.peekByte();
	if(byte == 0xff){ // Meta-event
		MidiMetaEvent msg(chunk);
	}
	else if(byte == 0xf0){ // Sys-exclusive message
		MidiSysExclusive msg(chunk);
	}
	else if(bitTest(byte, 7)){ // Midi message
		MidiMessage msg(chunk);
	}
	else{ // Running status?
	}
	return true;
}

MidiFileReader::MidiFileReader() :
	bFirstNote(true),
	bFinalized(false),
	nTime(0),
	nFormat(0),
	nTracks(0),
	nDivision(0),
	nDeltaTicksPerQuarter(0),
	fClockMultiplier(1.f),
	sFilename("out.mid"),
	sTrackname(),
	notemap(),
	timer(),
	header(),
	track(),
	bNotePressed()
{
	// Setup midi chunks
	header.setType("MThd"); // Header chunk
	track.setType("MTrk"); // Track chunk
	midiHeader();
	if (!sTrackname.empty()) // Midi track title
		midiTrackName(sTrackname);
	midiTimeSignature(4, 4, 24, 8); // 4/4 
	midiKeySignature(0, false); // C Major
	midiTempo(120);
}

MidiFileReader::MidiFileReader(const std::string& filename, const std::string& title/*=""*/) :
	MidiFileReader()
{
	sFilename = filename;
	sTrackname = title;
}

void MidiFileReader::press(const unsigned char& ch, const unsigned int& t, const float& freq){
	if(ch >= 16)
		return;
	unsigned int clkT = (unsigned int)(t * fClockMultiplier);
	if(bFirstNote){ // First recorded midi note
		bFirstNote = false;
		nTime = clkT;
		timer.start(); // Start the audio timer
	}
	else if (bNotePressed[ch] == true) { // Note already pressed on this channel, release it first
		release(ch, t);
	}
	MidiMessage msg(MidiKey(clkT, ch, notemap(freq)), nTime);
	msg.press();
	msg.write(track); // Write note to track chunk
	bNotePressed[ch].press();
	bNotePressed[ch].setKeyNumber(msg.getKeyNumber()); // Key number needed for release()
	nTime = clkT; // Update midi clock
}

void MidiFileReader::release(const unsigned char& ch, const unsigned int& t){
	if(ch >= 16 || (bNotePressed[ch] == false))
		return;
	unsigned int clkT = (unsigned int)(t * fClockMultiplier);
	MidiMessage msg(MidiKey(clkT, ch, bNotePressed[ch].getKeyNumber()), nTime);
	msg.release();
	msg.write(track); // Write note to track chunk
	bNotePressed[ch].release();
	nTime = clkT; // Update midi clock
}

void MidiFileReader::setMidiInstrument(const unsigned char& ch, const unsigned char& nPC) {
	// Write midi program change event 
	if (ch >= 16)
		return;
	track.pushUChar(0x00); // Delta-time
	track.pushUChar(0xc0 | ch); // Program change (for channel=ch)
	track.pushUChar(nPC & 0x7f); // Instrument program number [0, 127]
}

void MidiFileReader::finalize(const unsigned int& t) {
	double audioLength = timer.stop(); // Stop the audio timer (s)
	std::cout << "  Length: " << audioLength << " s" << std::endl;
	std::cout << "  Midi: " << nTime << " clocks (" << nTime / audioLength << " clk/s)" << std::endl;
	for (unsigned char i = 0; i < 16; i++) { // Release any notes which are currently down
		if (bNotePressed[i] == true)
			release(i, t);
	}
	midiEndOfTrack(); // Finalize track chunk (REQUIRED by midi format)
	bFinalized = true;
}

bool MidiFileReader::read(const std::string& filename/*=""*/){
	std::ifstream file((!filename.empty() ? filename : sFilename).c_str(), std::ios::binary);
	if(!file.good())
		return false;
	MidiChunk header(file);
	readHeaderChunk(header);
	print();
	MidiChunk trackChunk;
	while(trackChunk.readMidiChunk(file)){
		readTrackChunk(trackChunk);
		trackChunk.clear();
	}
	file.close();
	return true;
}

bool MidiFileReader::write(const std::string& filename/*=""*/){
	if (!bFinalized)
		finalize(nTime);
	std::ofstream file((!filename.empty() ? filename : sFilename).c_str(), std::ios::binary);
	if(!file.good())
		return false;
	header.writeMidiChunk(file); // Write header chunk to file
	track.writeMidiChunk(file); // Write track chunk to file
	file.close();
	bFirstNote = true; // Reset in case we record more audio
	bFinalized = false;
	header.clear();
	track.clear();
	return true;
}

void MidiFileReader::print(){
	std::cout << " Format: " << nFormat << std::endl;
	std::cout << " Tracks: " << nTracks << std::endl;
	std::cout << " Division: " << nDivision << std::endl;
	std::cout << " Ticks: " << nDeltaTicksPerQuarter << std::endl;
}

bool MidiFileReader::readHeaderChunk(MidiChunk& hdr){
	if((hdr != "MThd") || hdr.getLength() < 6)
		return false;
	
	// Read 6 header bytes
	hdr.getUShort(nFormat);
	hdr.getUShort(nTracks);
	hdr.getUShort(nDivision);
	
	// Division code
	if(bitTest((unsigned short)nDivision, 15)){ // SMPTE format (ignore!)
	}
	else{ // Metric time
		nDeltaTicksPerQuarter = nDivision & 0x7fff; // Bits 0,14
	}
	
	return true;
}
		
bool MidiFileReader::readTrackChunk(MidiChunk& chunk){
	if((chunk != "MTrk") || chunk.empty())
		return false;
	TrackEvent event;
	while(event.read(chunk)){
	}
	return true;
}

void MidiFileReader::midiHeader(const unsigned short& div/*=24*/) {
	header.pushUShort(0); // Midi format [0: Single track, 1: Simultaneous tracks, 2: Independent tracks]
	header.pushUShort(1); // Number of tracks (equal to 1 for format 0)
	header.pushUShort(div); // Number of midi clock ticks per quarter note
}

void MidiFileReader::midiTrackName(const std::string& str) {
	// Write track title
	track.pushUChar(0x00); // Delta-time
	track.pushUChar(0xff);
	track.pushUChar(0x03);
	track.pushUChar((unsigned char)str.length());
	track.pushString(str);
}

void MidiFileReader::midiTempo(const unsigned short& bpm/*=120*/) {
	// Write tempo ff 51 03 tt tt tt (microseconds / quarter notes)
	// Default tempo = 120 bpm (500000=0x7a120)
	unsigned int usPerQuarterNote = (60.f / bpm) * 1E6; // Microseconds per beat (quarter note)
	track.pushUChar(0x00); // Delta-time
	track.pushUChar(0xff);
	track.pushUChar(0x51);
	track.pushUChar(0x03);
	track.pushMemory(usPerQuarterNote, 3);
}

void MidiFileReader::midiTimeSignature(const unsigned char& nn/*=4*/, const unsigned char& dd/*=4*/, const unsigned char& cc/*=24*/, const unsigned char& bb/*=8*/) {
	// Write time signature ff 58 04 nn dd cc bb
	track.pushUChar(0x00); // Delta-time
	track.pushUChar(0xff);
	track.pushUChar(0x58);
	track.pushUChar(0x04);
	track.pushUChar(nn); // Numerator
	track.pushUChar((unsigned char)std::log2(dd)); // Denominator
	track.pushUChar(cc); // 24 midi clocks per metronome tick
	track.pushUChar(bb); // 8 32nd notes per 24 midi clock ticks (1 quarter note)
}

void MidiFileReader::midiKeySignature(const unsigned char& sf/*=0*/, bool minor/*=false*/) {
	// Write key signature ff 59 02 sf mi
	track.pushUChar(0x00); // Delta-time
	track.pushUChar(0xff);
	track.pushUChar(0x59);
	track.pushUChar(0x02);
	track.pushUChar(sf);
	track.pushUChar(minor ? 1 : 0);
}

void MidiFileReader::midiEndOfTrack() {
	// Write end of track flag ff 2f 00 (required)
	track.pushUChar(0x00); // Delta-time
	track.pushUChar(0xff);
	track.pushUChar(0x2f);
	track.pushUChar(0x00);
}
