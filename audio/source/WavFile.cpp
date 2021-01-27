#include <iostream>
#include <vector>

#include "WavFile.hpp"

WavFile::WavFile(const std::string& filename) :
	WavFile()
{
	audio.open(filename.c_str());
	if(audio.good()){
		readHeader();
	}
}

WavFile::~WavFile(){
	audio.close();
}

void WavFile::sample(const float& timeStep, float* arr, const unsigned long& N, const unsigned short& channels){
	if(!nLengthRemaining){ // Audio file completed
		for(unsigned long i = 0; i < N; i++) { // Over all frames
			arr[i * channels] = 0.f;
		}
		return;
	}
	unsigned int samplesRequested = (int)(timeStep * N / fSamplePeriod);
	unsigned int bytesRequested = std::min(nLengthRemaining, samplesRequested * nBytesPerSample); // Total number of bytes requested
	std::vector<unsigned char> tempData(bytesRequested, 0);
	audio.read((char*)tempData.data(), bytesRequested * sizeof(unsigned char));
	unsigned int prevSample = 0;
	unsigned int currSample = 0;
	phase = 0.f;
	for(unsigned long i = 0; i < N; i++) { // Over all frames
		currSample = (unsigned int)(phase / fSamplePeriod);
		arr[i * channels] = (2.f * tempData[currSample] / 255.f - 1.f);
		if(currSample != prevSample)
			prevSample = currSample;
		phase += timeStep;
	}
	nLengthRemaining -= bytesRequested;
}

void WavFile::print(){
	float lengthOfAudio = nLengthData / (float)(nBytesPerSecond);
	std::cout << " File Size: " << nFileSize << " B" << std::endl;
	std::cout << " Sample Rate: " << nSampleRate << " Hz" << std::endl;
	std::cout << " Number Channels: " << nChannels << std::endl;
	std::cout << " Length of Audio: " << lengthOfAudio << " s" << std::endl;
	std::cout << " Audio Bitrate: " << nBitsPerSample * nSamples / (1000.f * lengthOfAudio) << " kbps" << std::endl;
}

bool WavFile::readHeader(){
	std::string str = "    ";
	audio.read(&str[0], 4); // RIFF
	if(str != "RIFF")
		return false;
	audio.read((char*)&nFileSize, 4);
	audio.read(&str[0], 4); // WAVE
	audio.read(&str[0], 4); // fmt 
	audio.read((char*)&nLengthFormatData, 4);
	audio.read((char*)&nFormat, 2); // 1 is PCM
	audio.read((char*)&nChannels, 2);
	audio.read((char*)&nSampleRate, 4); // in Hz
	audio.read((char*)&nBytesPerSecond, 4); // Data rate (B/s) [(SampleRate * BitsPerSample * Channels) / 8]
	audio.read((char*)&nBytesPerSample, 2); // Bytes per sample [(BitsPerSample * Channels) / 8]
	audio.read((char*)&nBitsPerSample, 2);
	audio.read(&str[0], 4); // data
	if(str != "data")
		return false; // ??
	audio.read((char*)&nLengthData, 4);
	nSamples = nLengthData / nBytesPerSample;
	nBitsPerChannel = nBitsPerSample / nChannels;
	fSamplePeriod = 1.f / nSampleRate;
	nLengthRemaining = nLengthData;
	return true;
}

