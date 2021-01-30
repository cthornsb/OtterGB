#ifndef WAV_FILE_HPP
#define WAV_FILE_HPP

#include <fstream>

#include "AudioSampler.hpp"

class WavFile : public AudioSampler {
public:
	/** Default file constructor
	  */
	WavFile() :
		AudioSampler(),
		nFileSize(0),
		nSampleRate(0),
		nBytesPerSecond(0),
		nLengthFormatData(0),
		nLengthData(0),
		nSamples(0),
		nFormat(0),
		nChannels(0),
		nBytesPerSample(0),
		nBitsPerSample(0),
		nBitsPerChannel(0),
		fSamplePeriod(0.f),
		audio()
	{ 
	}

	/** Wav file constructor
	  */
	WavFile(const std::string& filename);

	/** Destructor
	  */	
	~WavFile();
	
	/** Does nothing. Returns zero
	  */
	virtual float sample(const float&) { return 0.f; }
	
	/** Read a specified number of samples from the input wav file and write them to an output array
	  */
	virtual void sample(const float& timeStep, float* arr, const unsigned int& N);

	/** Print wav file header information
	  */
	void print();

private:
	/*class WavSample{
	public:
		unsigned short nChannels;
		unsigned short nBytes;
	
		unsigned char* payload;
		
		WavSample() :
			nChannels(0),
			nBytes(0),
			payload(0x0)
		{ 
		}
		
		WavSample(unsigned char* ptr, const unsigned short& bytes, const unsigned short& channels) :
			nChannels(channels),
			nBytes(bytes),
			payload(ptr)
		{ 
		}
		
		
	};*/

	unsigned int nFileSize; ///< Total file size (minus 8 bytes)
	unsigned int nSampleRate; ///< Audio sample rate (Hz)
	unsigned int nBytesPerSecond; ///< Data rate (B/s)
	unsigned int nLengthFormatData; ///< Total length of audio format data
	unsigned int nLengthData; ///< Total length of audio data (in bytes)
	unsigned int nLengthRemaining; ///< Remaining bytes of audio data
	unsigned int nSamples; ///< Total number of audio samples
	
	unsigned short nFormat; ///< Audio format
	unsigned short nChannels; ///< Number of audio channels
	unsigned short nBytesPerSample; ///< Number of byes per sample
	unsigned short nBitsPerSample; ///< Number of bits per sample
	unsigned short nBitsPerChannel; ///< Number of bits per sample per channel
	
	float fSamplePeriod; ///< Period of audio sampling (seconds)
	
	std::ifstream audio; ///< Input audio file
	
	/** Read wav file header
	  */
	bool readHeader();
};

#endif
