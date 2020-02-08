#ifndef _SDL_PLAYER_H_
#define _SDL_PLAYER_H_

#include <string>
#include "SDL2/SDL.h"
#include "MediaFileStream.h"
#include "MediaPlayer.h"

class SDLPlayer
{
public:
    static int threadEntry(void* ptr);
    static uint32_t onTimer(uint32_t interval, void* ptr);
    static uint32_t SDLPlayer_EventType;

    enum SDLPlayer_EventCode
    {
        RENDER_VIDEO = 1,
    };
public:
    SDLPlayer(uint32_t w, uint32_t h, bool autoScale = false);
    virtual ~SDLPlayer();

    void startPlay(const std::string& url);
    void stopPlay();

    void renderVideo();
    void tickTrigger(uint64_t millisecond) { m_mediaPlayer.tickTrigger(millisecond); }
private:
    bool run();
    void videoPlaybackCb(const std::vector<std::shared_ptr<VideoPlaybackFrame>>& frameVec);
    void audioPlaybackCb(const std::vector<std::shared_ptr<AudioPlaybackFrame>>& frameVec);
private:
    SDL_Window* m_pPlayerWnd;
    SDL_Renderer* m_pPlayerRenderer;
    SDL_Texture* m_pPlayerTexture;
    SDL_mutex* m_pRenderFrameMutex;
    std::vector<std::shared_ptr<VideoPlaybackFrame>> m_renderFrames;

    SDL_AudioDeviceID m_audioDevId;
    SDL_AudioSpec m_audioSpec;

    MediaFileStream m_flvStream;
    MediaPlayer m_mediaPlayer;
    bool m_isFlvStreamEnd;

    SDL_TimerID m_timer;
    SDL_Thread* m_pThread;
    SDL_atomic_t m_running;
    std::string m_videoHeader;
    VideoPlayContext_Ptr m_videoPlayback;
    std::string m_audioHeader;
    AudioPlayContext_Ptr m_audioPlayback;
};

#endif