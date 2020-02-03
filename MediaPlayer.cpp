#include "MediaPlayer.h"

MediaPlayer::MediaPlayer()
{
    m_contextId = 0;
    m_audioContextId = 0;
}

VideoPlayContext_Ptr MediaPlayer::startPlayVideo(VideoPlayback_cb cb)
{
    VideoPlayContext_Ptr pCtx = std::make_shared<VideoPlayContext>();
    m_contextId++;
    pCtx->contextId = m_contextId;
    pCtx->videoPlayback = std::make_shared<VideoPlayback>();
    pCtx->videoPlayback->setPlaybackCallback(cb);

    m_videoPlaybacks[m_contextId] = pCtx;
    return pCtx;
}

void MediaPlayer::stopPlayVideo(VideoPlayContext_Ptr pCtx)
{
    pCtx->videoPlayback->resetPlayback();
    m_videoPlaybacks.erase(pCtx->contextId);
}

void MediaPlayer::playVideo(VideoPlayContext_Ptr pCtx, const VideoPlaybackPacket& pkt)
{
    // std::cout << "playVideo pts=" << pkt.pts << " dts=" << pkt.dts << " size=" << pkt.content.size() << std::endl;
    pCtx->videoPlayback->appendContent(pkt);
}

void MediaPlayer::updateVideoPlayConfig(VideoPlayContext_Ptr pCtx, const VideoPlaybackConfig& cfg)
{
    pCtx->videoPlayback->setupPlayback(cfg);
}

AudioPlayContext_Ptr MediaPlayer::startPlayAudio(AudioPlayback_cb cb)
{
    AudioPlayContext_Ptr pCtx = std::make_shared<AudioPlayContext>();
    m_audioContextId++;
    pCtx->contextId = m_audioContextId;
    pCtx->audioPlayback = std::make_shared<AudioPlayback>();
    pCtx->audioPlayback->setPlaybackCallback(cb);

    m_audioPlaybacks[m_audioContextId] = pCtx;
    return pCtx;
}

void MediaPlayer::stopPlayAudio(AudioPlayContext_Ptr pCtx)
{
    pCtx->audioPlayback->resetPlayback();
    m_audioPlaybacks.erase(pCtx->contextId);
}

void MediaPlayer::playAudio(AudioPlayContext_Ptr pCtx, const AudioPlaybackPacket& pkt)
{
    pCtx->audioPlayback->appendContent(pkt);
}

void MediaPlayer::updateAudioPlayConfig(AudioPlayContext_Ptr pCtx, const AudioPlaybackConfig& cfg)
{
    pCtx->audioPlayback->setupPlayback(cfg);
}

void MediaPlayer::tickTrigger(uint64_t millisecond)
{
    uint32_t tickTime = (uint32_t)millisecond;

    if(!m_baseTs.tsValid)
    {
        m_baseTs.tsValid = genPlaybackBaseTs(m_baseTs.baseTs);
        if(!m_baseTs.tsValid)
            return;

        std::cout << "BaseTs=" << m_baseTs.baseTs << std::endl;
    }
    tickTime += m_baseTs.baseTs;

    for(std::map<uint32_t, VideoPlayContext_Ptr>::value_type& ctxVal : m_videoPlaybacks)
    {
        VideoPlayContext_Ptr pCtx = ctxVal.second;
        pCtx->videoPlayback->tickTrigger(tickTime);
    }

    for(std::map<uint32_t, AudioPlayContext_Ptr>::value_type& audCtxVal : m_audioPlaybacks)
    {
        AudioPlayContext_Ptr pCtx = audCtxVal.second;
        pCtx->audioPlayback->tickTrigger(tickTime);
    }
}

bool MediaPlayer::genPlaybackBaseTs(uint32_t& baseTs) const
{
    bool validTs = false;

    for(const std::map<uint32_t, VideoPlayContext_Ptr>::value_type& ctxVal : m_videoPlaybacks)
    {
        VideoPlayContext_Ptr pCtx = ctxVal.second;
        const std::shared_ptr<VideoPlaybackFrame> pFrame = 
            pCtx->videoPlayback->getHeadPlaybackFrame();
        if(pFrame && (!validTs || baseTs > pFrame->pts))
        {
            baseTs = pFrame->pts;
            validTs = true;
        }
    }

    return validTs;
}
