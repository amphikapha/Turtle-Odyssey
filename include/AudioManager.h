#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <string>
#include <iostream>
#include <vector>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Initialize audio system
    bool Initialize();

    // Load and play a music file (loops infinitely)
    bool PlayMusic(const std::string& filePath);

    // Stop music
    void StopMusic();

    // Set volume (0.0 to 1.0)
    void SetVolume(float volume);

    // Update audio (call this in game loop to check if music needs to restart)
    void Update();

    // Check if music is playing
    bool IsMusicPlaying();

private:
    // OpenAL objects
    ALCdevice* device;
    ALCcontext* context;
    ALuint sourceId;
    ALuint bufferId;

    // Audio data
    std::string currentMusicPath;
    bool musicLoaded;
    bool isPlaying;

    // Helper to load WAV/FLAC/OGG file
    bool LoadAudioFile(const std::string& filePath, ALuint& buffer);
};
