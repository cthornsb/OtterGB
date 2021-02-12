#include <iostream>
#include <cstring>
#include <cmath>

#include "Support.hpp"
#include "MidiFile.hpp"
#include "SimpleSynthesizers.hpp"

using namespace MidiFile;

using namespace PianoKeys;

unsigned short reverseByteOrder(const unsigned short& input){
	unsigned short retval = 0;
	for(int i = 0; i < 2; i++){
		retval += ((input & (0xff << (8 * i))) >> (8 * i)) << (16 - 8 * (i + 1));
	}
	return retval;
}

unsigned int reverseByteOrder(const unsigned int& input){
	unsigned int retval = 0;
	for(int i = 0; i < 4; i++){
		retval += ((input & (0xff << (8 * i))) >> (8 * i)) << (32 - 8 * (i + 1));
	}
	return retval;
}

unsigned int reverseByteOrder(const unsigned int& input, const char& bits){
	unsigned int retval = 0;
	for(int i = 0; i < 4; i++){
		retval += ((input & (0xff << (8 * i))) >> (8 * i)) << (24 - 8 * (i + 1));
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
	
void MidiMessage::set(const MidiKey& other){
	bPressed = other.isPressed();
	nChannel = other.getChannel();
	nKeyNumber = other.getKeyNumber();
	nVelocity = other.getKeyVelocity();
	nTime = other.getTime();
	// Set status
	if(bPressed)
		nStatus = MidiStatusType::PRESSED;
	else
		nStatus = MidiStatusType::RELEASED;
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
		// 327680 is default
		chunk.copyMemory(static_cast<void*>(&nTempo), 3); // 24-bit (microseconds / quarter-note)
		nTempo = reverseByteOrder(nTempo, 24); // High byte first
		std::cout << " Tempo=" << nTempo << " (" << 1.f / (nTempo * 1E-6) << " bps)" << std::endl;
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

void TrackEvent::addNote(const unsigned char& chan, const unsigned int& t, const unsigned char& note){
	if(chan > 15) // Channel out of range
		return;
	if(bNotePressed[chan]) // Release current note
		release(chan, t);
	MidiKey newkey;
	newkey.setChannel(chan);
	newkey.setKeyNumber(note);
	newkey.setKeyVelocity(0x40); // Not sensitive to velocity, so use the default
	newkey.setTime(t); // Absolute midi clock time
	keylist.push_back(newkey);
	nPrevTime = t; // Update running midi time
	if(t < nStartTime)
		nStartTime = t;
	//std::cout << " addNote() ch=" << (int)chan << ", note=" << (int)note << ", t=" << t << std::endl;
	bNotePressed[chan] = true;
}

void TrackEvent::addNote(const unsigned char& chan, const unsigned int& t, const std::string& note){
	if(chan > 15) // Channel out of range
		return;
	if(bNotePressed[chan]) // Release current note
		release(chan, t);
	MidiKey newkey;
	newkey.setChannel(chan);
	newkey.setKeyNumber(60);
	newkey.setKeyVelocity(0x40); // Not sensitive to velocity, so use the default
	newkey.setTime(t); // Absolute midi clock time
	keylist.push_back(newkey);
	nPrevTime = t; // Update running midi time
	if(t < nStartTime)
		nStartTime = t;
	bNotePressed[chan] = true;
}

void TrackEvent::addNote(const MidiKey& note){
	unsigned char chan = note.getChannel();
	if(bNotePressed[chan]) // Release current note
		release(chan, note.getTime());
	keylist.push_back(note);
	nPrevTime = note.getTime(); // Update running midi time
	if(note.getTime() < nStartTime)
		nStartTime = note.getTime();
	bNotePressed[chan] = true;
}

void TrackEvent::release(const unsigned char& chan, const unsigned int& t){
	if(chan > 15) // Channel out of range
		return;
	if(keylist.empty() || !bNotePressed[chan]) // No note pressed on this channel
		return;
	// Find most recent occurance of a played note on this channel
	MidiKey* recent = 0x0;
	for(std::deque<MidiKey>::reverse_iterator iter = keylist.rbegin(); iter != keylist.rend(); iter++){
		if(iter->getChannel() == chan){
			recent = &(*iter);
			break;
		}
	}
	if(!recent) // Note not found
		return;
	MidiKey newkey(*recent); // Copy the note
	newkey.release(); // Mark it as released
	newkey.setTime(t); // Absolute midi clock time
	keylist.push_back(newkey);
	nPrevTime = t; // Update running midi time
	//std::cout << " release() ch=" << (int)chan << ", note=" << (int)newkey.getKeyNumber() << ", t=" << t << std::endl;
	bNotePressed[chan] = false;
}

bool TrackEvent::getNote(MidiMessage* msg){
	if(keylist.empty())
		return false;
	msg->set(keylist.front());
	keylist.pop_front();
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

void MidiFileReader::addNote(const int& ch, const unsigned int& t, const float& freq){
	if(ch < 1 || ch > 4)
		return;
	if(bFirstNote){ // First recorded midi note
		bFirstNote = false;
		timer.start(); // Start the audio timer
	}
	unsigned char note = notemap(freq);
	track.addNote(ch, t, note);
	nTime = t; // Update midi clocks
}

void MidiFileReader::release(const int& ch, const unsigned int& t){
	if(ch < 1 || ch > 4)
		return;
	track.release(ch, t);
	nTime = t; // Update midi clocks
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
	double audioLength = timer.stop(); // Stop the audio timer (s)
	std::ofstream file((!filename.empty() ? filename : sFilename).c_str(), std::ios::binary);
	if(!file.good())
		return false;
	writeHeader(file);
	writeTrack(file);
	std::cout << "  Length: " << audioLength << " s" << std::endl;
	std::cout << "  Midi: " << nTime << " clocks (" << nTime / audioLength << " clk/s)" << std::endl;
	file.close();
	bFirstNote = true; // Reset in case we record more audio
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
	if(bitTest(nDivision, 15)){ // SMPTE format (ignore!)
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

bool MidiFileReader::writeHeader(std::ofstream& f){
	if(!f.good())
		return false;
	//nFormat = 1; // Midi format [0: Single track, 1: Simultaneous tracks, 2: Independent tracks]
	//nTracks = 4; // Number of tracks (equal to 1 for format 0)
	nFormat = 0; // Midi format [0: Single track, 1: Simultaneous tracks, 2: Independent tracks]
	nTracks = 1; // Number of tracks (equal to 1 for format 0)
	nDivision = 0x18; // 24 ticks per quarter note
	MidiChunk chunk;
	chunk.setType("MThd");
	chunk.pushUShort(nFormat);
	chunk.pushUShort(nTracks);
	chunk.pushUShort(nDivision);
	return chunk.writeMidiChunk(f);
}

bool MidiFileReader::writeTrack(std::ofstream& f){
	MidiChunk chunk;
	chunk.setType("MTrk"); // Track chunk
	
	// Write track title
	if(!sTrackname.empty()){
		chunk.pushUChar(0x00); // Delta-time
		chunk.pushUChar(0xff);
		chunk.pushUChar(0x03);
		chunk.pushUChar((unsigned char)sTrackname.length());
		chunk.pushString(sTrackname);
	}
	
	// Write tempo ff 51 03 tt tt tt (microseconds / quarter notes)
	// Default tempo = 120 bpm (500000=0x7a120)
	/*chunk.pushUChar(0xff);
	chunk.pushUChar(0x51);
	chunk.pushUChar(0x03);
	chunk.pushMemory(0x07a120, 3);*/
	
	// Write time signature ff 58 04 nn dd cc bb
	// nn = numerator
	// dd = denominator (expressed as power of 2 e.g. 4 -> dd=2)
	// cc = midi clocks per metronome tick
	// bb = number of 1/32 notes per 24 midi clocks (8 standard)
	chunk.pushUChar(0x00); // Delta-time
	chunk.pushUChar(0xff);
	chunk.pushUChar(0x58);
	chunk.pushUChar(0x04);
	chunk.pushUChar(0x04); // 4
	chunk.pushUChar(0x02); // 4
	chunk.pushUChar(0x18); // 24 midi clocks per metronome tick
	chunk.pushUChar(0x08); // 8 32nd notes per 24 midi clock ticks (1 quarter note)
	
	// Write key signature ff 59 02 sf mi
	// sf = number of sharps or flats (0 is C key)
	// mi = [0: major key, 1: minor key] 
	chunk.pushUChar(0x00); // Delta-time
	chunk.pushUChar(0xff);
	chunk.pushUChar(0x59);
	chunk.pushUChar(0x02);
	chunk.pushUChar(0x00); // C
	chunk.pushUChar(0x00); // Major
	
	// Set program number (instrument)
	for(unsigned char ch = 0; ch < 4; ch++){
		chunk.pushUChar(0x00); // Delta-time
		chunk.pushUChar(0xc0 | ch); // Program change (for channel=ch)
		chunk.pushUChar(track.getProgramNumber() & 0x7f); // Instrument program number [0, 127]
	}
	
	// Write notes
	MidiMessage msg;
	unsigned int prevTime = track.getStartTime();
	while(track.getNote(&msg)){
		msg.setDeltaTime(prevTime);
		msg.write(chunk);
		prevTime = msg.getTime();
	}
	
	// Write end of track flag ff 2f 00 (required)
	chunk.pushUChar(0x00); // Delta-time
	chunk.pushUChar(0xff);
	chunk.pushUChar(0x2f);
	chunk.pushUChar(0x00);
	
	// Write the chunk to file
	return chunk.writeMidiChunk(f);
}

