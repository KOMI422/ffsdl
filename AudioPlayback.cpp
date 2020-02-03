#include "AudioPlayback.h"

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
    }
}

void AudioPlayback::tickTrigger(uint32_t tickTime)
{
    std::vector<std::shared_ptr<AudioPlaybackFrame>> playFrames;
    while(!m_playbackFrames.empty())
    {
        std::shared_ptr<AudioPlaybackFrame> pFrame = m_playbackFrames.front();
        if(pFrame->pts <= tickTime)
        {
            //音频帧有固定的播放时长，不像视频能快放，超过播放时间的音频帧只能丢弃
            playFrames.clear();
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
