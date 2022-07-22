#include "../headers/AudioManager.h"

void AudioManager::Initialize()
{
	Mix_QuerySpec(&frequency, &format, &deviceChannels);
	printf("Initializing AudioManager with: Frequency -> %d |  Format -> %d | Channels -> %d\n", frequency, format, deviceChannels);
	
	channels = Mix_AllocateChannels(CHANNELS_TO_ALLOCATE);
	activeAudioEffects.resize(CHANNELS_TO_ALLOCATE);

	// ----- SFX LOADING ----- //
	sfx["test_gunshot"] = Mix_LoadWAV("../../audio/sfx/gunshot.mp3");
	for (const std::pair<std::string, Mix_Chunk*> sound : sfx)
	{
		if (sound.second == nullptr)
		{
			util::Assert(false, "AudioManager::Initialize failure: " + std::string(Mix_GetError()));
		}
	}
}

int AudioManager::PlaySfx(std::string sfx)
{
	return Mix_PlayChannel(-1, this->sfx[sfx], 0);
}

int AudioManager::ComputeChunkLengthMillisec(int chunkSize)
{
	const Uint32 points = chunkSize / FormatSampleSize(format);
	const Uint32 frames = (points / deviceChannels);
	return ((frames * 1000) / frequency);
}

void AudioManager::RegisterEffect(int channel, const Mix_Chunk* chunk, float speed)
{
	tty::Log("Attempting to register effect on channel ", channel);
	tty::Log(activeAudioEffects[channel].effect);
	if (activeAudioEffects[channel].effect != nullptr) { return; }
	tty::Log(format);
	switch (format)
	{
	case AUDIO_U16: LoadAudioEffect<Uint16>(channel, chunk, speed); break;
	case AUDIO_S16: LoadAudioEffect<Sint16>(channel, chunk, speed); break;
	case AUDIO_S32: LoadAudioEffect<Sint32>(channel, chunk, speed); break;
	case AUDIO_F32: LoadAudioEffect<float>(channel, chunk, speed);	break;
	}
}

void AudioManager::CheckForFinishedChannels()
{
	for (int i = 0; i < CHANNELS_TO_ALLOCATE; ++i)
	{
		if (activeAudioEffects[i].finished)
		{
			Mix_HaltChannel(i);
			activeAudioEffects[i].finished = false;
		}
	}
}

template <typename T>
void AudioManager::LoadAudioEffect(int channel, const Mix_Chunk* chunk, float speed)
{
	activeAudioEffects[channel].effect = new SfxEffectWrapper<T>(this, chunk, speed);
	Mix_RegisterEffect(channel, SfxEffectWrapper<T>::EffectModifierCallback, SfxEffectWrapper<T>::EffectDoneCallback, nullptr);
}
