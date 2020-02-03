#ifndef _VIDEO_PLAY_BACK_H_
#define _VIDEO_PLAY_BACK_H_

#include "ffcodec/FFDecoder.h"
#include <functional>
#include <memory>
#include <deque>
#include <vector>

struct VideoPlaybackPacket
{
    uint32_t pts;
    uint32_t dts;
    std::string content;

    VideoPlaybackPacket() : pts(0), dts(0) {}
};

struct VideoPlaybackFrame
{
    //IYUV 4:2:0
    uint32_t pts;
    uint32_t width;
    uint32_t height;
    std::string yuvContent;

    VideoPlaybackFrame() : pts(0), width(0), height(0) {}
};

struct VideoPlaybackConfig
{
    std::string decodeHeader;
};

typedef std::function<void(const std::vector<std::shared_ptr<VideoPlaybackFrame>>&)> VideoPlayback_cb;

class VideoPlayback
{
public:
    VideoPlayback();

    bool setupPlayback(const VideoPlaybackConfig& cfg);
    void resetPlayback();

    void appendContent(const VideoPlaybackPacket& content);
    void tickTrigger(uint32_t tickTime);

    void setPlaybackCallback(VideoPlayback_cb cb) { m_playbackCb = cb; }
    const std::shared_ptr<VideoPlaybackFrame> getHeadPlaybackFrame() const;
private:
    void notifyPlaybackFrames(const std::vector<std::shared_ptr<VideoPlaybackFrame>>& playFrames);
private:
    FFDecoder m_decoder;
    std::deque<std::shared_ptr<VideoPlaybackFrame>> m_playbackFrames;
    
    VideoPlayback_cb m_playbackCb;
};

#endif