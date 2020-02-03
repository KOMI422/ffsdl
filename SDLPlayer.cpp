#include "SDLPlayer.h"
#include <cstring>

int AudioThread::threadEntry(void* ptr)
{
    // ((AudioThread*)ptr)->run();
    return 0;
}

AudioThread::AudioThread()
{
    m_pThread = NULL;
    SDL_AtomicSet(&m_running, 0);
}

void AudioThread::start()
{
    if(m_pThread != NULL)
        return;

    SDL_AtomicSet(&m_running, 1);
    m_pThread = SDL_CreateThread(AudioThread::threadEntry, "AudioThread", this);
}

void AudioThread::stop()
{
    SDL_AtomicSet(&m_running, 0);
}

void AudioThread::handleAudio(const std::string& pcmData, uint32_t sampleRate, uint32_t channels)
{

}

int SDLPlayer::threadEntry(void *ptr)
{
    ((SDLPlayer*)ptr)->run();
    return 0;
}

uint32_t SDLPlayer::SDLPlayer_EventType = 0;

SDLPlayer::SDLPlayer(uint32_t w, uint32_t h, bool autoScale)
{
    m_pPlayerWnd = NULL;
    m_pPlayerRenderer = NULL;
    m_pPlayerTexture = NULL;
    m_pThread = NULL;

    m_pPlayerWnd = SDL_CreateWindow(
        "SDLPlayer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, 0
    );

    m_pPlayerRenderer = SDL_CreateRenderer(
        m_pPlayerWnd, -1, 0
    );

    m_pPlayerTexture = SDL_CreateTexture(
        m_pPlayerRenderer, SDL_PIXELFORMAT_IYUV, 
        SDL_TEXTUREACCESS_STREAMING, w, h
    );

    m_pRenderFrameMutex = SDL_CreateMutex();

    SDL_zero(m_audioSpec);
    m_audioSpec.format = AUDIO_F32;
    m_audioSpec.channels = 2;
    m_audioSpec.freq = 44100;
    m_audioSpec.samples = 8192;
    m_audioSpec.callback = NULL;
    m_audioDevId = SDL_OpenAudioDevice(NULL, 0, &m_audioSpec, NULL, 0);
    SDL_PauseAudioDevice(m_audioDevId, 0);
}

SDLPlayer::~SDLPlayer()
{
    if(m_pPlayerTexture)
        SDL_DestroyTexture(m_pPlayerTexture);

    if(m_pPlayerRenderer)
        SDL_DestroyRenderer(m_pPlayerRenderer);

    if(m_pPlayerWnd)
        SDL_DestroyWindow(m_pPlayerWnd);

    if(m_pRenderFrameMutex)
        SDL_DestroyMutex(m_pRenderFrameMutex);

    if(m_audioDevId > 0)
        SDL_CloseAudioDevice(m_audioDevId);
}

void SDLPlayer::startPlay(const std::string& url)
{
    if(m_pThread)
        return;
    
    m_flvStream.openFile(url);
    SDL_AtomicSet(&m_running, 1);
    m_pThread = SDL_CreateThread(SDLPlayer::threadEntry, "Player_IO", this);
}

void SDLPlayer::stopPlay()
{
    if(!m_pThread)
        return;

    SDL_AtomicSet(&m_running, 0);
    SDL_WaitThread(m_pThread, NULL);
}

void SDLPlayer::run()
{
    VideoPlayback_cb cb = std::bind(&SDLPlayer::videoPlaybackCb, this, std::placeholders::_1);
    m_videoPlayback = m_mediaPlayer.startPlayVideo(cb);

    AudioPlayback_cb audCb = std::bind(&SDLPlayer::audioPlaybackCb, this, std::placeholders::_1);
    m_audioPlayback = m_mediaPlayer.startPlayAudio(audCb);
    while(SDL_AtomicGet(&m_running) > 0)
    {
        MediaFileStream::FrameType type;
        std::string frameHeader;
        std::string frameData;
        uint32_t dts = 0;
        uint32_t pts = 0;
        if(!m_flvStream.readFrame(frameData, frameHeader, dts, pts, type))
        {
            SDL_AtomicSet(&m_running, 0);
            continue;
        }

        if(type == MediaFileStream::VIDEO)
        {
            //AVC End of Sequence
            if(frameData.empty())
                continue;
            
            if (frameHeader != m_videoHeader)
            {
                m_videoHeader = frameHeader;

                VideoPlaybackConfig cfg;
                cfg.decodeHeader = m_videoHeader;
                m_mediaPlayer.updateVideoPlayConfig(m_videoPlayback, cfg);
            }

            VideoPlaybackPacket pkt;
            pkt.content = frameData;
            pkt.pts = pts;
            pkt.dts = dts;
            m_mediaPlayer.playVideo(m_videoPlayback, pkt);
        }
        else if(type == MediaFileStream::AUDIO)
        {
            if(frameHeader != m_audioHeader)
            {
                m_audioHeader = frameHeader;
                
                AudioPlaybackConfig cfg;
                cfg.audioHeader = m_audioHeader;
                m_mediaPlayer.updateAudioPlayConfig(m_audioPlayback, cfg);
            }

            AudioPlaybackPacket pkt;
            pkt.content = frameData;
            pkt.dts = dts;
            m_mediaPlayer.playAudio(m_audioPlayback, pkt);
        }
    }
}

void SDLPlayer::renderVideo()
{
    std::vector<std::shared_ptr<VideoPlaybackFrame>> toRenderFrames;
    SDL_LockMutex(m_pRenderFrameMutex);
    toRenderFrames.swap(m_renderFrames);
    SDL_UnlockMutex(m_pRenderFrameMutex);

    for(std::shared_ptr<VideoPlaybackFrame> pFrame : toRenderFrames)
    {
        void* pixels = NULL;
        int pitch = 0;

        // SDL_Log("Render Frame pts=%u", pFrame->pts);

        SDL_LockTexture(m_pPlayerTexture, NULL, &pixels, &pitch);
        memcpy(pixels, (const void*)pFrame->yuvContent.data(), pFrame->yuvContent.size());
        pitch = pFrame->width;
        SDL_UnlockTexture(m_pPlayerTexture);

        SDL_RenderClear(m_pPlayerRenderer);
        SDL_RenderCopy(m_pPlayerRenderer, m_pPlayerTexture, NULL, NULL);
        SDL_RenderPresent(m_pPlayerRenderer);
    }
}

void SDLPlayer::videoPlaybackCb(const std::vector<std::shared_ptr<VideoPlaybackFrame>>& frameVec)
{
    SDL_LockMutex(m_pRenderFrameMutex);
    m_renderFrames.insert(m_renderFrames.end(), frameVec.begin(), frameVec.end());
    SDL_UnlockMutex(m_pRenderFrameMutex);

    SDL_Event renderEvent;
    SDL_zero(renderEvent);
    renderEvent.type = SDLPlayer_EventType;
    renderEvent.user.code = RENDER_VIDEO;
    renderEvent.user.data1 = this;
    SDL_PushEvent(&renderEvent);
}

void SDLPlayer::audioPlaybackCb(const std::vector<std::shared_ptr<AudioPlaybackFrame>>& frameVec)
{
    for(const std::shared_ptr<AudioPlaybackFrame> pFrame : frameVec)
    {
        SDL_QueueAudio(m_audioDevId, pFrame->pcmContent.data(), pFrame->pcmContent.size());
    }
}