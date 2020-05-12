#include "VideoPlayback.h"

const uint32_t VideoPlayback::TICK_FLUSH = UINT32_MAX;

VideoPlayback::VideoPlayback() : m_playbackMode(TICK_MODE)
{
    FFDecoder::LogFunc logFunc = std::bind(&VideoPlayback::getDecoderLogFunc, this);
    m_decoder.setLogFunction(logFunc);
}

bool VideoPlayback::setupPlayback(const VideoPlaybackConfig& cfg)
{
    FFDecoderConfigs decCfg;
    decCfg.decodeHeader = cfg.decodeHeader;

    return m_decoder.setupDecoder(decCfg);
}

void VideoPlayback::resetPlayback()
{
    m_decoder.resetDecoder();
}

void VideoPlayback::appendContent(const VideoPlaybackPacket& content)
{
    FFDecodePacket pkt;
    pkt.pts = content.pts;
    pkt.dts = content.dts;
    pkt.packetData = content.content;

    std::shared_ptr<FFDecodeFrame> pFrame = std::make_shared<FFDecodeFrame>();
    if(m_decoder.decode(pkt, *pFrame))
    {
        std::shared_ptr<VideoPlaybackFrame> pPlayFrame = 
            std::make_shared<VideoPlaybackFrame>();
        
        pPlayFrame->pts = pFrame->pts;
        pPlayFrame->width = pFrame->width;
        pPlayFrame->height = pFrame->height;
        pPlayFrame->yuvContent = pFrame->frameData;

        // std::cout << "AppendContent appendPpts=" << pkt.pts
        //     << " decodedPts=" << pFrame->pts << std::endl;
        m_playbackFrames.push_back(pPlayFrame);
        
        if(m_playbackMode == FLUSH_MODE)
            tickTrigger(TICK_FLUSH);
    }
}

void VideoPlayback::flushContent()
{
    std::vector<FFDecodeFrame> frames;
    m_decoder.flushDecoder(frames);

    for(FFDecodeFrame& frame : frames)
    {
        std::shared_ptr<VideoPlaybackFrame> pPlayFrame = 
            std::make_shared<VideoPlaybackFrame>();
        
        pPlayFrame->pts = frame.pts;
        pPlayFrame->width = frame.width;
        pPlayFrame->height = frame.height;
        pPlayFrame->yuvContent = frame.frameData;

        m_playbackFrames.push_back(pPlayFrame);
    }
    
    if(m_playbackMode == FLUSH_MODE)
        tickTrigger(TICK_FLUSH);
}

void VideoPlayback::tickTrigger(uint32_t tickTime)
{
    // std::vector<std::shared_ptr<VideoPlaybackFrame>> playFrames;
    // while(!m_playbackFrames.empty())
    // {
    //     std::shared_ptr<VideoPlaybackFrame> pFrame = m_playbackFrames.front();
    //     if(pFrame->pts <= tickTime)
    //     {
    //         playFrames.push_back(pFrame);
    //         m_playbackFrames.pop_front();
    //     }
    //     else
    //     {
    //         break;
    //     }
    // }

    std::vector<std::shared_ptr<VideoPlaybackFrame>> playFrames;
    if(tickTime == VideoPlayback::TICK_FLUSH)
    {
        playFrames.insert(playFrames.end(), m_playbackFrames.begin(), m_playbackFrames.end());
        m_playbackFrames.clear();
    }
    else
    {
        if (!m_playbackFrames.empty() && m_playbackFrames.front()->pts <= tickTime)
        {
            playFrames.push_back(m_playbackFrames.front());
            m_playbackFrames.pop_front();
        }
    }

    if(!playFrames.empty())
    {
        notifyPlaybackFrames(playFrames);
    }
}

const std::shared_ptr<VideoPlaybackFrame> VideoPlayback::getHeadPlaybackFrame() const
{
    std::shared_ptr<VideoPlaybackFrame> pRetFrame;
    if(!m_playbackFrames.empty())
    {
        pRetFrame = m_playbackFrames.front();
    }
    return pRetFrame;
}

void VideoPlayback::notifyPlaybackFrames(
    const std::vector<std::shared_ptr<VideoPlaybackFrame>>& playFrames)
{
    if(m_playbackCb)
    {
        m_playbackCb(playFrames);
    }
}

std::ostream& VideoPlayback::getDecoderLogFunc()
{
    return std::cout;
}