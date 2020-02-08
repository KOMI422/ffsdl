#include "MediaPlayer.h"

void VideoPlaybackThread::threadEntry(VideoPlaybackThread* pPlayback)
{
    pPlayback->run();
}

VideoPlaybackThread::VideoPlaybackThread()
{
    m_running = false;
    m_videoPlayback.setPlaybackMode(VideoPlayback::FLUSH_MODE);
}

void VideoPlaybackThread::start()
{
    m_running = true;
    m_pThread = std::shared_ptr<std::thread>(new std::thread(VideoPlaybackThread::threadEntry, this));
}

void VideoPlaybackThread::stop()
{
    m_running = false;
}

void VideoPlaybackThread::run()
{
    while(m_running)
    {
        PlaybackThreadQueue<PlaybackPacket>::QueueType pktQueue;
        if(!m_pktQueue.swapQueue(pktQueue, 500))
            continue;

        for(PlaybackPacket& pkt : pktQueue)
        {
            if(pkt.pktType == PlaybackPacket::PLAYBACK_CONFIG)
            {
                //TODO: flush 
                m_videoPlayback.resetPlayback();
                m_videoPlayback.setupPlayback(pkt.playbackCfg);
            }
            else if(pkt.pktType == PlaybackPacket::PLAYBACK_PACKET)
            {
                m_videoPlayback.appendContent(pkt.playbackPkt);
            }
            else
            {
                m_videoPlayback.flushContent();
            }
            
        }
    }
}

void VideoPlaybackThread::appendPacket(const VideoPlaybackPacket& pkt)
{
    PlaybackPacket pbPkt;
    pbPkt.pktType = PlaybackPacket::PLAYBACK_PACKET;
    pbPkt.playbackPkt = pkt;

    m_pktQueue.pushBack(pbPkt);
}

void VideoPlaybackThread::updatePlaybackConfig(const VideoPlaybackConfig& cfg)
{
    PlaybackPacket pbPkt;
    pbPkt.pktType = PlaybackPacket::PLAYBACK_CONFIG;
    pbPkt.playbackCfg = cfg;

    m_pktQueue.pushBack(pbPkt);
}

void VideoPlaybackThread::flush()
{
    PlaybackPacket pbPkt;
    pbPkt.pktType = PlaybackPacket::PLAYBACK_FLUSH;

    m_pktQueue.pushBack(pbPkt);
}

void AudioPlaybackThread::threadEntry(AudioPlaybackThread* pPlayback)
{
    pPlayback->run();
}

AudioPlaybackThread::AudioPlaybackThread()
{
    m_running = false;
    m_audioPlayback.setPlaybackMode(AudioPlayback::FLUSH_MODE);
}

void AudioPlaybackThread::start()
{
    m_running = true;
    m_pThread = std::shared_ptr<std::thread>(new std::thread(AudioPlaybackThread::threadEntry, this));
}

void AudioPlaybackThread::stop()
{
    m_running = false;
}

void AudioPlaybackThread::run()
{
    while(m_running)
    {
        PlaybackThreadQueue<PlaybackPacket>::QueueType pktQueue;
        if(!m_pktQueue.swapQueue(pktQueue, 500))
            continue;

        for(PlaybackPacket& pkt : pktQueue)
        {
            if(pkt.pktType == PlaybackPacket::PLAYBACK_CONFIG)
            {
                //TODO: flush 
                m_audioPlayback.resetPlayback();
                m_audioPlayback.setupPlayback(pkt.playbackCfg);
            }
            else if(pkt.pktType == PlaybackPacket::PLAYBACK_PACKET)
            {
                m_audioPlayback.appendContent(pkt.playbackPkt);
            }
            else
            {
                m_audioPlayback.flushContent();
            }
        }
    }
}

void AudioPlaybackThread::appendPacket(const AudioPlaybackPacket& pkt)
{
    PlaybackPacket pbPkt;
    pbPkt.pktType = PlaybackPacket::PLAYBACK_PACKET;
    pbPkt.playbackPkt = pkt;

    m_pktQueue.pushBack(pbPkt);
}

void AudioPlaybackThread::updatePlaybackConfig(const AudioPlaybackConfig& cfg)
{
    PlaybackPacket pbPkt;
    pbPkt.pktType = PlaybackPacket::PLAYBACK_CONFIG;
    pbPkt.playbackCfg = cfg;

    m_pktQueue.pushBack(pbPkt);
}

void AudioPlaybackThread::flush()
{
    PlaybackPacket pbPkt;
    pbPkt.pktType = PlaybackPacket::PLAYBACK_FLUSH;

    m_pktQueue.pushBack(pbPkt);
}

MediaPlayer::MediaPlayer()
{
    m_contextId = 0;
    m_audioContextId = 0;
    m_tickStartTs = 0;
    m_playBufferLen = 1000;
}

VideoPlayContext_Ptr MediaPlayer::startPlayVideo(VideoPlayback_cb cb)
{
    VideoPlayContext_Ptr pCtx = std::make_shared<VideoPlayContext>();
    VideoPlayback_cb ctxCb = std::bind(&VideoPlayContext::onVideoPlaybackCb, pCtx, std::placeholders::_1);
    m_contextId++;
    pCtx->contextId = m_contextId;
    pCtx->playbackCb = cb;
    pCtx->videoPlaybackThread = std::make_shared<VideoPlaybackThread>();
    pCtx->videoPlaybackThread->setVideoPlaybackCb(ctxCb);
    pCtx->videoPlaybackThread->start();

    m_videoPlaybacks[m_contextId] = pCtx;
    return pCtx;
}

void MediaPlayer::stopPlayVideo(VideoPlayContext_Ptr pCtx)
{
    pCtx->videoPlaybackThread->stop();
    pCtx->videoPlaybackThread->join();
    m_videoPlaybacks.erase(pCtx->contextId);
}

void MediaPlayer::playVideo(VideoPlayContext_Ptr pCtx, const VideoPlaybackPacket& pkt)
{
    // std::cout << "playVideo pts=" << pkt.pts << " dts=" << pkt.dts << " size=" << pkt.content.size() << std::endl;
    pCtx->videoPlaybackThread->appendPacket(pkt);
}

void MediaPlayer::updateVideoPlayConfig(VideoPlayContext_Ptr pCtx, const VideoPlaybackConfig& cfg)
{
    pCtx->videoPlaybackThread->updatePlaybackConfig(cfg);
}

AudioPlayContext_Ptr MediaPlayer::startPlayAudio(AudioPlayback_cb cb)
{
    AudioPlayContext_Ptr pCtx = std::make_shared<AudioPlayContext>();
    AudioPlayback_cb ctxCb = std::bind(AudioPlayContext::onAudioPlaybackCb, pCtx, std::placeholders::_1);
    m_audioContextId++;
    pCtx->contextId = m_audioContextId;
    pCtx->playbackCb = cb;
    pCtx->audioPlaybackThread = std::make_shared<AudioPlaybackThread>();
    pCtx->audioPlaybackThread->setVideoPlaybackCb(ctxCb);
    pCtx->audioPlaybackThread->start();

    m_audioPlaybacks[m_audioContextId] = pCtx;
    return pCtx;
}

void MediaPlayer::stopPlayAudio(AudioPlayContext_Ptr pCtx)
{
    pCtx->audioPlaybackThread->stop();
    pCtx->audioPlaybackThread->join();
    m_audioPlaybacks.erase(pCtx->contextId);
}

void MediaPlayer::playAudio(AudioPlayContext_Ptr pCtx, const AudioPlaybackPacket& pkt)
{
    pCtx->audioPlaybackThread->appendPacket(pkt);
}

void MediaPlayer::updateAudioPlayConfig(AudioPlayContext_Ptr pCtx, const AudioPlaybackConfig& cfg)
{
    pCtx->audioPlaybackThread->updatePlaybackConfig(cfg);
}

void MediaPlayer::flushMedia()
{
    for(std::map<uint32_t, VideoPlayContext_Ptr>::value_type vctxVal : m_videoPlaybacks)
    {
        VideoPlayContext_Ptr pVCtx = vctxVal.second;
        pVCtx->videoPlaybackThread->flush();
    }

    for(std::map<uint32_t, AudioPlayContext_Ptr>::value_type actxVal : m_audioPlaybacks)
    {
        AudioPlayContext_Ptr pACtx = actxVal.second;
        pACtx->audioPlaybackThread->flush();
    }
}

void MediaPlayer::tickTrigger(uint64_t millisecond)
{
    uint32_t tickTime = (uint32_t)millisecond;
    if(m_tickStartTs == 0)
    {
        m_tickStartTs = tickTime;
    }

    if(m_tickStartTs + m_playBufferLen > tickTime)
        return;

    if(!m_baseTs.tsValid)
    {
        m_baseTs.baseTs = 0;
        m_baseTs.tsValid = genPlaybackBaseTs(m_baseTs.baseTs);
        if(!m_baseTs.tsValid)
            return;
        std::cout << "BaseTs=" << m_baseTs.baseTs << std::endl;
    }
    // tickTime += m_baseTs.baseTs;
    uint32_t playbackTs = tickTime - m_tickStartTs - m_playBufferLen + m_baseTs.baseTs;

    for(std::map<uint32_t, VideoPlayContext_Ptr>::value_type& ctxVal : m_videoPlaybacks)
    {
        VideoPlayContext_Ptr pCtx = ctxVal.second;
        std::unique_lock<std::mutex> lockQueue(pCtx->frameQueueMutex);

        if(pCtx->playbackFrames.empty())
            continue;

        std::shared_ptr<VideoPlaybackFrame> pFrame = pCtx->playbackFrames.front();
        if(pFrame->pts <= playbackTs)
        {
            pCtx->playbackFrames.pop_front();
            lockQueue.unlock();

            pCtx->playbackCb({pFrame});
        }
    }

    for(std::map<uint32_t, AudioPlayContext_Ptr>::value_type& audCtxVal : m_audioPlaybacks)
    {
        AudioPlayContext_Ptr pCtx = audCtxVal.second;
        std::unique_lock<std::mutex> lockQueue(pCtx->frameQueueMutex);

        if(pCtx->playbackFrames.empty())
            continue;

        std::shared_ptr<AudioPlaybackFrame> pFrame;
        while(!pCtx->playbackFrames.empty())
        {
            if(pCtx->playbackFrames.front()->pts <= playbackTs)
            {
                pFrame = pCtx->playbackFrames.front();
                pCtx->playbackFrames.pop_front();
            }
            else
            {
                break;
            }
        }
        lockQueue.unlock();

        if(pFrame)
            pCtx->playbackCb({pFrame});
    }
}

bool MediaPlayer::genPlaybackBaseTs(uint32_t& baseTs) const
{
    bool validTs = false;

    for(const std::map<uint32_t, VideoPlayContext_Ptr>::value_type& ctxVal : m_videoPlaybacks)
    {
        VideoPlayContext_Ptr pCtx = ctxVal.second;
        std::unique_lock<std::mutex> lockQueue(pCtx->frameQueueMutex);

        if(pCtx->playbackFrames.empty())
            continue;

        const std::shared_ptr<VideoPlaybackFrame>& pFrame = pCtx->playbackFrames.front();
        if(!pCtx->playbackFrames.empty() && (baseTs == 0 || baseTs > pFrame->pts))
        {
            baseTs = pFrame->pts;
            validTs = true;
        }
    }

    return validTs;
}
