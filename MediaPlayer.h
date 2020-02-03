#ifndef _MEDIA_PLAYER_H_
#define _MEDIA_PLAYER_H_

#include <memory>
#include <map>
#include "VideoPlayback.h"
#include "AudioPlayback.h"

struct VideoPlayContext
{
    uint32_t contextId;
    std::shared_ptr<VideoPlayback> videoPlayback;

    VideoPlayContext() : contextId(0) {}
};
typedef std::shared_ptr<VideoPlayContext> VideoPlayContext_Ptr;

struct AudioPlayContext
{
    uint32_t contextId;
    std::shared_ptr<AudioPlayback> audioPlayback;

    AudioPlayContext() : contextId(0) {}
};
typedef std::shared_ptr<AudioPlayContext> AudioPlayContext_Ptr;

class MediaPlayer
{
public:
    MediaPlayer();

    VideoPlayContext_Ptr startPlayVideo(VideoPlayback_cb cb);
    void stopPlayVideo(VideoPlayContext_Ptr pCtx);
    void playVideo(VideoPlayContext_Ptr pCtx, const VideoPlaybackPacket& pkt);
    void updateVideoPlayConfig(VideoPlayContext_Ptr pCtx, const VideoPlaybackConfig& cfg);

    AudioPlayContext_Ptr startPlayAudio(AudioPlayback_cb cb);
    void stopPlayAudio(AudioPlayContext_Ptr pCtx);
    void playAudio(AudioPlayContext_Ptr pCtx, const AudioPlaybackPacket& pkt);
    void updateAudioPlayConfig(AudioPlayContext_Ptr pCtx, const AudioPlaybackConfig& cfg);

    void tickTrigger(uint64_t millisecond);
private:
    bool genPlaybackBaseTs(uint32_t& baseTs) const;
private:
    uint32_t m_contextId;
    std::map<uint32_t, VideoPlayContext_Ptr> m_videoPlaybacks;

    uint32_t m_audioContextId;
    std::map<uint32_t, AudioPlayContext_Ptr> m_audioPlaybacks;

    struct BaseTickStamp
    {
        uint32_t baseTs;
        bool tsValid;
        BaseTickStamp() : baseTs(0), tsValid(false) {}
    };
    BaseTickStamp m_baseTs;
};

#endif