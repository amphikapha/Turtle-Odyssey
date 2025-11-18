#include "AudioManager.h"
#include <vector>

AudioManager::AudioManager() 
    : device(nullptr), context(nullptr), sourceId(0), bufferId(0), 
      musicLoaded(false), isPlaying(false) 
{
}

AudioManager::~AudioManager() 
{
    StopMusic();
    
    // Clean up OpenAL
    if (sourceId != 0) {
        alDeleteSources(1, &sourceId);
    }
    if (bufferId != 0) {
        alDeleteBuffers(1, &bufferId);
    }
    
    if (context != nullptr) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
    }
    
    if (device != nullptr) {
        alcCloseDevice(device);
    }
}

bool AudioManager::Initialize() 
{
    // Open default audio device
    device = alcOpenDevice(nullptr);
    if (device == nullptr) {
        std::cerr << "Failed to open audio device" << std::endl;
        return false;
    }

    // Create audio context
    context = alcCreateContext(device, nullptr);
    if (context == nullptr) {
        std::cerr << "Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }

    // Make context current
    if (!alcMakeContextCurrent(context)) {
        std::cerr << "Failed to make audio context current" << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);
        device = nullptr;
        context = nullptr;
        return false;
    }

    // Set listener properties
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    ALfloat orientation[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
    alListenerfv(AL_ORIENTATION, orientation);

    // Generate source
    alGenSources(1, &sourceId);
    if (sourceId == 0) {
        std::cerr << "Failed to generate audio source" << std::endl;
        return false;
    }

    // Set source properties
    alSourcef(sourceId, AL_PITCH, 1.0f);
    alSourcef(sourceId, AL_GAIN, 1.0f);
    alSource3f(sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(sourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcei(sourceId, AL_LOOPING, AL_TRUE);

    std::cout << "Audio system initialized successfully" << std::endl;
    return true;
}

bool AudioManager::LoadAudioFile(const std::string& filePath, ALuint& buffer) 
{
    // Use libsndfile for audio files (WAV, FLAC, OGG)
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));
    
    SNDFILE* file = sf_open(filePath.c_str(), SFM_READ, &sfInfo);
    if (file == nullptr) {
        std::cerr << "Failed to open audio file: " << filePath << std::endl;
        std::cerr << "libsndfile error: " << sf_strerror(nullptr) << std::endl;
        return false;
    }

    std::cout << "Loading audio file: " << filePath << std::endl;
    std::cout << "  Channels: " << sfInfo.channels << ", Sample rate: " << sfInfo.samplerate 
              << ", Frames: " << sfInfo.frames << std::endl;

    // Read all audio data
    std::vector<short> audioData(sfInfo.frames * sfInfo.channels);
    sf_count_t numFrames = sf_readf_short(file, audioData.data(), sfInfo.frames);
    
    if (numFrames != sfInfo.frames) {
        std::cerr << "Warning: Read " << numFrames << " frames instead of " << sfInfo.frames << std::endl;
    }

    sf_close(file);

    // Determine OpenAL format
    ALenum format;
    if (sfInfo.channels == 1) {
        format = AL_FORMAT_MONO16;
    } else if (sfInfo.channels == 2) {
        format = AL_FORMAT_STEREO16;
    } else {
        std::cerr << "Unsupported number of channels: " << sfInfo.channels << std::endl;
        return false;
    }

    // Create OpenAL buffer
    if (buffer == 0) {
        alGenBuffers(1, &buffer);
    }

    // Upload audio data to buffer
    alBufferData(buffer, format, audioData.data(), 
                 audioData.size() * sizeof(short), sfInfo.samplerate);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error while loading buffer: " << error << std::endl;
        return false;
    }

    std::cout << "Audio file loaded successfully" << std::endl;
    return true;
}

bool AudioManager::PlayMusic(const std::string& filePath) 
{
    if (sourceId == 0) {
        std::cerr << "Audio system not initialized" << std::endl;
        return false;
    }

    // Stop any currently playing music
    StopMusic();

    // Create or reuse buffer
    if (bufferId == 0) {
        alGenBuffers(1, &bufferId);
    }

    // Load audio file
    if (!LoadAudioFile(filePath, bufferId)) {
        return false;
    }

    // Attach buffer to source and play
    alSourcei(sourceId, AL_BUFFER, bufferId);
    alSourcePlay(sourceId);

    currentMusicPath = filePath;
    musicLoaded = true;
    isPlaying = true;

    std::cout << "Now playing: " << filePath << std::endl;
    return true;
}

void AudioManager::StopMusic() 
{
    if (sourceId != 0) {
        alSourceStop(sourceId);
        isPlaying = false;
    }
}

void AudioManager::SetVolume(float volume) 
{
    // Clamp volume to [0, 1]
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    if (sourceId != 0) {
        alSourcef(sourceId, AL_GAIN, volume);
    }
}

void AudioManager::Update() 
{
    if (!isPlaying || sourceId == 0) return;

    // Check if source is still playing
    ALint state;
    alGetSourcei(sourceId, AL_SOURCE_STATE, &state);

    if (state == AL_STOPPED) {
        // Music finished, restart it (looping)
        std::cout << "Music finished, restarting..." << std::endl;
        alSourcePlay(sourceId);
    }
}

bool AudioManager::IsMusicPlaying() 
{
    if (sourceId == 0) return false;

    ALint state;
    alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
    return (state == AL_PLAYING);
}
