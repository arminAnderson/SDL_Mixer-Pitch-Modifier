#pragma once
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <unordered_map>
#include "Util.h"

struct AudioBundle
{
	void* effect = nullptr;
	bool finished = false;
};

class AudioManager
{
public:
	AudioManager() = default;

	void Initialize();
	void PlayMusic(std::string music);
	int PlaySfx(std::string sfx);

	int frequency;
	Uint16 format;
	int deviceChannels; //mono/stereo
	int channels;

	const int CHANNELS_TO_ALLOCATE = 16;

	std::unordered_map<std::string, Mix_Music*> music;
	std::unordered_map<std::string, Mix_Chunk*> sfx;

	inline Uint16 FormatSampleSize(Uint16 format) { return (format & 0xFF) / 8; }
	int ComputeChunkLengthMillisec(int chunkSize);

	void RegisterEffect(int channel, const Mix_Chunk* chunk, float speed);
	void CheckForFinishedChannels();
	template <typename T> void LoadAudioEffect(int channel, const Mix_Chunk* chunk, float speed);

	static std::vector<AudioBundle> activeAudioEffects;
};

template <typename AudioFormat>
struct SfxEffectWrapper
{
	AudioManager* audioManager;
	AudioFormat* chunkData;
	float speed, position;
	int duration, chunkSize;
	bool handled = false;

	SfxEffectWrapper(AudioManager* manager, const Mix_Chunk* chunk, const float speed) :
		audioManager(manager),
		chunkData((AudioFormat*)chunk->abuf),
		speed(speed),
		duration(manager->ComputeChunkLengthMillisec(chunk->alen)),
		chunkSize(chunk->alen / manager->FormatSampleSize(manager->format)),
		position(0)
	{
	}

	~SfxEffectWrapper()
	{
		//tty::Log("DELETED");
	}

	void ModifyPlaybackSpeed(int mixChannel, void* stream, int length)
	{
		AudioFormat* buffer = (AudioFormat*)stream;
		const int bufferSize = length / sizeof(AudioFormat); 

		if (position < duration)
		{
			const float delta = 1000.0f / audioManager->frequency,  
						vdelta = delta * speed; 
			if (!handled)
			{
				//handled = true;
				for (int i = 0; i < bufferSize; i += audioManager->deviceChannels)
				{
					const int j = i / audioManager->deviceChannels; 
					const float x = position + j * vdelta;
					const int k = floor(x / delta);  
					const float prop = (x / delta) - k;

					for (int c = 0; c < audioManager->deviceChannels; c++)
					{
						if (k * audioManager->deviceChannels + audioManager->deviceChannels - 1 < chunkSize)
						{
							AudioFormat v0 = chunkData[(k * audioManager->deviceChannels + c) % chunkSize],
										v1 = chunkData[((k + 1) * audioManager->deviceChannels + c) % chunkSize];
							buffer[i + c] = v0 + prop * (v1 - v0); 
						}
						else
						{
							buffer[i + c] = 0;
						}
					}
				}
			}
			position += (bufferSize / audioManager->deviceChannels) * vdelta;
		}
		else 
		{
			for (int i = 0; i < bufferSize; i++) { buffer[i] = 0; }
			audioManager->activeAudioEffects[mixChannel].finished = true;
		}
	}

	static void EffectModifierCallback(int channel, void* stream, int length, void* userData)
	{
		((SfxEffectWrapper*)AudioManager::activeAudioEffects[channel].effect)->ModifyPlaybackSpeed(channel, stream, length);
	}

	static void EffectDoneCallback(int channel , void* userData)
	{
		delete (SfxEffectWrapper*)AudioManager::activeAudioEffects[channel].effect;
		AudioManager::activeAudioEffects[channel].effect = nullptr;
	}
};