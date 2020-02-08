#ifndef _MEDIA_PLAYER_H_
#define _MEDIA_PLAYER_H_

#include <memory>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "VideoPlayback.h"
#include "AudioPlayback.h"

template<typename T>
class PlaybackThreadQueue
{
public:
    typedef std::vector<T> QueueType;
public:
    void pushBack(const T& frame)
    {
        std::unique_lock<std::mutex> lock(m_vecMutex);
        m_packets.push_back(frame);
        m_vecCond.notify_one();
    }

    bool swapQueue(QueueType& outQueue, int32_t millisecond)
    {
        std::unique_lock<std::mutex> lock(m_vecMutex);
        if(m_packets.empty())
        {
            if(millisecond == -1)
                return false;
            else if(millisecond == 0)
                m_vecCond.wait(lock);
            else
            {
                std::cv_status ret = m_vecCond.wait_for(lock, std::chrono::milliseconds(millisecond));
                if(ret == std::cv_status::timeout)
                    return false;
            }
        }
        outQueue.swap(m_packets);
        return true;
    }
private:
    std::mutex m_vecMutex;
    std::condition_variable m_vecCond;
    QueueType m_packets;
};

class VideoPlaybackThread
{
public:
    static void threadEntry(VideoPlaybackThread* pPlayback);
public:
    VideoPlaybackThread();

    void start();
    void stop();
    void join() { m_pThread->join(); }
    void setVideoPlaybackCb(VideoPlayback_cb cb) { m_videoPlayback.setPlaybackCallback(cb); }
    void appendPacket(const VideoPlaybackPacket& pkt);
    void updatePlaybackConfig(const VideoPlaybackConfig& cfg);
    void flush();
private:
    void run();
private:
    std::atomic<bool> m_running;
    std::shared_ptr<std::thread> m_pThread;

    struct PlaybackPacket
    {
        enum AppendPktType
        {
            PLAYBACK_CONFIG = 0,
            PLAYBACK_PACKET = 1,
            PLAYBACK_FLUSH = 2
        };

        AppendPktType pktType;

        VideoPlaybackConfig playbackCfg;
        VideoPlaybackPacket playbackPkt;

        PlaybackPacket() : pktType(PLAYBACK_CONFIG) {}
    };
    PlaybackThreadQueue<PlaybackPacket> m_pktQueue;
    VideoPlayback m_videoPlayback;
};

class AudioPlaybackThread
{
public:
    static void threadEntry(AudioPlaybackThread* pPlayback);
public:
    AudioPlaybackThread();

    void start();
    void stop();
    void join() { m_pThread->join(); }
    void setVideoPlaybackCb(AudioPlayback_cb cb) { m_audioPlayback.setPlaybackCallback(cb); }
    void appendPacket(const AudioPlaybackPacket& pkt);
    void updatePlaybackConfig(const AudioPlaybackConfig& cfg);
    void flush();
private:
    void run();
private:
    std::atomic<bool> m_running;
    std::shared_ptr<std::thread> m_pThread;

    struct PlaybackPacket
    {
        enum AppendPktType
        {
            PLAYBACK_CONFIG = 0,
            PLAYBACK_PACKET = 1,
            PLAYBACK_FLUSH = 2
        };

        AppendPktType pktType;

        AudioPlaybackConfig playbackCfg;
        AudioPlaybackPacket playbackPkt;

        PlaybackPacket() : pktType(PLAYBACK_CONFIG) {}
    };
    PlaybackThreadQueue<PlaybackPacket> m_pktQueue;
    AudioPlayback m_audioPlayback;
};

struct VideoPlayContext
{
    uint32_t contextId;
    VideoPlayback_cb playbackCb;
    std::shared_ptr<VideoPlaybackThread> videoPlaybackThread;

    std::mutex frameQueueMutex;
    std::deque<std::shared_ptr<VideoPlaybackFrame>> playbackFrames;

    VideoPlayContext() : contextId(0) {}

    void onVideoPlaybackCb(const std::vector<std::shared_ptr<VideoPlaybackFrame>>& frames)
    {
        std::unique_lock<std::mutex> lock(frameQueueMutex);
        playbackFrames.insert(playbackFrames.end(), frames.begin(), frames.end());
    }
};
typedef std::shared_ptr<VideoPlayContext> VideoPlayContext_Ptr;

struct AudioPlayContext
{
    uint32_t contextId;
    AudioPlayback_cb playbackCb;
    std::shared_ptr<AudioPlaybackThread> audioPlaybackThread;

    std::mutex frameQueueMutex;
    std::deque<std::shared_ptr<AudioPlaybackFrame>> playbackFrames;

    AudioPlayContext() : contextId(0) {}

    void onAudioPlaybackCb(const std::vector<std::shared_ptr<AudioPlaybackFrame>>& frames)
    {
        std::unique_lock<std::mutex> locak(frameQueueMutex);
        playbackFrames.insert(playbackFrames.end(), frames.begin(), frames.end());
    }
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

    void flushMedia();

    void tickTrigger(uint64_t millisecond);
private:
    bool genPlaybackBaseTs(uint32_t& baseTs) const;
private:
    uint32_t m_contextId;
    std::map<uint32_t, VideoPlayContext_Ptr> m_videoPlaybacks;

    uint32_t m_audioContextId;
    std::map<uint32_t, AudioPlayContext_Ptr> m_audioPlaybacks;

    uint32_t m_tickStartTs;
    uint32_t m_playBufferLen;
    struct BaseTickStamp
    {
        uint32_t baseTs;
        bool tsValid;
        BaseTickStamp() : baseTs(0), tsValid(false) {}
    };
    BaseTickStamp m_baseTs;
};

#endif