#include "AudioPlayback.h"

const uint32_t AudioPlayback::TICK_FLUSH = UINT32_MAX;

AudioPlayback::AudioPlayback()
{
}

bool AudioPlayback::setupPlayback(const AudioPlaybackConfig& cfg)
{
    FFDecoderConfigs decCfg;
    decCfg.codecId = FFDecoderConfigs::AAC;
    decCfg.decodeHeader = cfg.audioHeader;

    return m_decoder.setupDecoder(decCfg);
}

void AudioPlayback::resetPlayback()
{
    m_decoder.resetDecoder();
}

void AudioPlayback::appendContent(const AudioPlaybackPacket& content)
{
    FFDecodePacket pkt;
    pkt.dts = pkt.pts = content.dts;
    pkt.packetData = content.content;

    FFDecodeFrame frame;
    if(m_decoder.decode(pkt, frame))
    {
        std::shared_ptr<AudioPlaybackFrame> pPlayFrame = 
            std::make_shared<AudioPlaybackFrame>();

        pPlayFrame->pts = frame.pts;
        pPlayFrame->pcmContent = frame.frameData;
        pPlayFrame->sampleRate = frame.sampleRate;
        pPlayFrame->channels = frame.channels;

        m_playbackFrames.push_back(pPlayFrame);

        if(m_playbackMode == FLUSH_MODE)
            tickTrigger(TICK_FLUSH);
    }
}

void AudioPlayback::flushContent()
{
    std::vector<FFDecodeFrame> frames;
    m_decoder.flushDecoder(frames);

    for(FFDecodeFrame& frame : frames)
    {
        std::shared_ptr<AudioPlaybackFrame> pPlayFrame = 
            std::make_shared<AudioPlaybackFrame>();

        pPlayFrame->pts = frame.pts;
        pPlayFrame->pcmContent = frame.frameData;
        pPlayFrame->sampleRate = frame.sampleRate;
        pPlayFrame->channels = frame.channels;

        m_playbackFrames.push_back(pPlayFrame);
    }

    if(m_playbackMode == FLUSH_MODE)
        tickTrigger(TICK_FLUSH);
}

void AudioPlayback::tickTrigger(uint32_t tickTime)
{
    std::vector<std::shared_ptr<AudioPlaybackFrame>> playFrames;
    while(!m_playbackFrames.empty())
    {
        std::shared_ptr<AudioPlaybackFrame> pFrame = m_playbackFrames.front();
        if(pFrame->pts <= tickTime)
        {
            if(m_playbackMode == TICK_MODE)
            {
                //音频帧有固定的播放时长，不像视频能快放，超过播放时间的音频帧只能丢弃
                playFrames.clear();
            }
            playFrames.push_back(pFrame);
            m_playbackFrames.pop_front();
        }
        else
        {
            break;
        }
    }

    if(!playFrames.empty())
    {
        // std::shared_ptr<AudioPlaybackFrame> pFrame = playFrames[0];
        // uint32_t diff = tickTime - pFrame->pts;
        // if (diff > 0)
        // {
        //     std::cout << "diff=" << diff << std::endl;
        // }
        notifyPlaybackFrames(playFrames);
    }
}

void AudioPlayback::notifyPlaybackFrames(
    const std::vector<std::shared_ptr<AudioPlaybackFrame>>& playFrames)
{
    if(m_playbackCb)
    {
        m_playbackCb(playFrames);
    }
}
