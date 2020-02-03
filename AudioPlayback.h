#ifndef _AUDIO_PLAY_BACK_H_
#define _AUDIO_PLAY_BACK_H_

#include "ffcodec/FFDecoder.h"
#include <functional>
#include <deque>
#include <memory>
#include <vector>

struct AudioPlaybackPacket
{
    uint32_t dts;
    std::string content;

    AudioPlaybackPacket() : dts(0) {}
};

struct AudioPlaybackFrame
{
    uint32_t pts;
    std::string pcmContent;

    uint32_t sampleRate;
    uint32_t channels;

    AudioPlaybackFrame() : pts(0), sampleRate(0), channels(0) {}
};

struct AudioPlaybackConfig
{
    std::string audioHeader;
};

typedef std::function<void(const std::vector<std::shared_ptr<AudioPlaybackFrame>>&)> AudioPlayback_cb;

class AudioPlayback
{
public:
    AudioPlayback();
    
    bool setupPlayback(const AudioPlaybackConfig& cfg);
    void resetPlayback();

    void appendContent(const AudioPlaybackPacket& content);
    void tickTrigger(uint32_t tickTime);

    void setPlaybackCallback(AudioPlayback_cb cb) { m_playbackCb = cb; }
private:
    void notifyPlaybackFrames(const std::vector<std::shared_ptr<AudioPlaybackFrame>>& playFrames);
private:
    FFDecoder m_decoder;
    std::deque<std::shared_ptr<AudioPlaybackFrame>> m_playbackFrames;

    AudioPlayback_cb m_playbackCb;
};

#endif
