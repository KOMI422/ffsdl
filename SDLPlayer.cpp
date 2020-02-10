#include "SDLPlayer.h"
#include <cstring>

uint32_t SDLPlayer::onTimer(uint32_t interval, void* ptr)
{
    if(((SDLPlayer*)ptr)->run())
    {
        return interval;
    }
    else
    {
        ((SDLPlayer*)ptr)->m_timerWaitCond.signal();
        return 0;
    }
    
}

uint32_t SDLPlayer::SDLPlayer_EventType = 0;

SDLPlayer::SDLPlayer(uint32_t w, uint32_t h, bool autoScale)
{
    SDL_Init(SDL_INIT_TIMER);
    m_pPlayerWnd = NULL;
    m_pPlayerRenderer = NULL;
    m_pPlayerTexture = NULL;
    m_wndWidth = w;
    m_wndHeight = h;
    m_wndRenderRect.x = 0;
    m_wndRenderRect.y = 0;
    m_wndRenderRect.w = w;
    m_wndRenderRect.h = h;

    m_pPlayerWnd = SDL_CreateWindow(
        "SDLPlayer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, 0
    );
    SDL_SetWindowResizable(m_pPlayerWnd, SDL_TRUE);

    m_pPlayerRenderer = SDL_CreateRenderer(
        m_pPlayerWnd, -1, 0
    );

    m_pPlayerTexture = NULL;
    // m_pPlayerTexture = SDL_CreateTexture(
    //     m_pPlayerRenderer, SDL_PIXELFORMAT_IYUV, 
    //     SDL_TEXTUREACCESS_STREAMING, 1920, 1080
    // );

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
    m_isFlvStreamEnd = false;
    m_flvStream.openFile(url);
    VideoPlayback_cb cb = std::bind(&SDLPlayer::videoPlaybackCb, this, std::placeholders::_1);
    m_videoPlayback = m_mediaPlayer.startPlayVideo(cb);

    AudioPlayback_cb audCb = std::bind(&SDLPlayer::audioPlaybackCb, this, std::placeholders::_1);
    m_audioPlayback = m_mediaPlayer.startPlayAudio(audCb);

    SDL_AtomicSet(&m_running, 1);
    m_timer = SDL_AddTimer(1, &SDLPlayer::onTimer, this);
}

void SDLPlayer::stopPlay()
{
    // if(!m_pThread)
    //     return;

    SDL_AtomicSet(&m_running, 0);

    m_timerWaitCond.wait();
    SDL_RemoveTimer(m_timer);
}

bool SDLPlayer::run()
{
    if(!m_isFlvStreamEnd)
    {
        MediaFileStream::FrameType type;
        std::string frameHeader;
        std::string frameData;
        uint32_t dts = 0;
        uint32_t pts = 0;
        int readRet = m_flvStream.readFrame(frameData, frameHeader, dts, pts, type);
        if (readRet > 0)
        {
            if (type == MediaFileStream::VIDEO)
            {
                //AVC End of Sequence
                if (!frameData.empty())
                {
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
            }
            else if (type == MediaFileStream::AUDIO)
            {
                if (frameHeader != m_audioHeader)
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
        else if (readRet < 0)
        {
            m_isFlvStreamEnd = true;
            m_mediaPlayer.flushMedia();
        }
    }

    m_mediaPlayer.tickTrigger(SDL_GetTicks());

    return SDL_AtomicGet(&m_running) > 0;
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

        // SDL_Log("Render Frame pts=%u size=%u", pFrame->pts, toRenderFrames.size());

        if(m_pPlayerTexture == NULL)
        {
            m_pPlayerTexture = SDL_CreateTexture(
                m_pPlayerRenderer, SDL_PIXELFORMAT_IYUV,
                SDL_TEXTUREACCESS_STREAMING, pFrame->width, pFrame->height);
            recalcWndRenderRect();
        }

        SDL_LockTexture(m_pPlayerTexture, NULL, &pixels, &pitch);
        memcpy(pixels, (const void*)pFrame->yuvContent.data(), pFrame->yuvContent.size());
        pitch = pFrame->width;
        SDL_UnlockTexture(m_pPlayerTexture);

        SDL_RenderClear(m_pPlayerRenderer);
        SDL_RenderCopy(m_pPlayerRenderer, m_pPlayerTexture, NULL, &m_wndRenderRect);
        SDL_RenderPresent(m_pPlayerRenderer);
    }
}

void SDLPlayer::onWndSizeChangedEvt(uint32_t w, uint32_t h)
{
    m_wndWidth = w;
    m_wndHeight = h;

    recalcWndRenderRect();
}

void SDLPlayer::recalcWndRenderRect()
{
    if(m_pPlayerTexture == NULL)
    {
        m_wndRenderRect.x = 0;
        m_wndRenderRect.y = 0;
        m_wndRenderRect.w = m_wndWidth;
        m_wndRenderRect.h = m_wndHeight;
    }
    else
    {
        int32_t textureW = 0;
        int32_t textureH = 0;
        SDL_QueryTexture(m_pPlayerTexture, NULL, NULL, &textureW, &textureH);

        double wRatio = (textureW * 1.0) / m_wndWidth;
        double hRatio = (textureH * 1.0) / m_wndHeight;

        double scaleRatio = (wRatio > hRatio) ? wRatio : hRatio;
        m_wndRenderRect.w = (uint32_t)(textureW / scaleRatio);
        m_wndRenderRect.h = (uint32_t)(textureH / scaleRatio);

        m_wndRenderRect.x = (m_wndWidth - m_wndRenderRect.w) / 2;
        m_wndRenderRect.y = (m_wndHeight - m_wndRenderRect.h) / 2;
    }
    
}

void SDLPlayer::videoPlaybackCb(const std::vector<std::shared_ptr<VideoPlaybackFrame>>& frameVec)
{
    SDL_LockMutex(m_pRenderFrameMutex);
    m_renderFrames.insert(m_renderFrames.end(), frameVec.begin(), frameVec.end());
    SDL_UnlockMutex(m_pRenderFrameMutex);

    // SDL_Log("videoCb size=%u pts=%u tick=%u", 
    //     m_renderFrames.size(), frameVec[0]->pts, SDL_GetTicks());
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