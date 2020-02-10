#define SDL_MAIN_HANDLED

#include <iostream>
#include <string>
#include "ffcodec/FFDecoder.h"
#include "flvcpp/FlvReader.h"
#include "SDL2/SDL.h"
#include "SDLPlayer.h"

void test();
void testAudio();

int main(int argc, char* argv[])
{
    // testAudio();
    // return 0;

    if(argc < 2)
        return -1;

    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    SDLPlayer::SDLPlayer_EventType = SDL_RegisterEvents(1);

    SDLPlayer* pPlayer = new SDLPlayer(960,540);
    bool started = false;

    uint64_t tickTime = 0;
    bool running = true;
    while(running)
    {
        SDL_Event evt;
        if(SDL_WaitEventTimeout(&evt, 1) != 1)
        {
            if(!started)
            {
                pPlayer->startPlay(std::string(argv[1]));
                started = true;
            }
            else
            {
                // pPlayer->renderVideo();
            }
        }
        else
        {
            if (evt.type == SDL_QUIT)
            {
                running = false;
                continue;
            }
            else if(evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                pPlayer->onWndSizeChangedEvt(evt.window.data1, evt.window.data2);
            }
            else if (evt.type == SDLPlayer::SDLPlayer_EventType)
            {
                SDLPlayer *player = (SDLPlayer *)evt.user.data1;
                player->renderVideo();
            }
        }
    }

    pPlayer->stopPlay();
    delete pPlayer;

    SDL_Log("Quit");

    SDL_Quit();
    return 0;
}

void testAudio()
{
    FILE* pIn = fopen("animal_aac.flv", "rb");
    char buf[500 * 1024] = { 0 };

    FILE* pOut = fopen("animal.pcm", "wb");

    FFDecoder decoder;
    FFDecoderConfigs decCfgs;
    decCfgs.codecId = FFDecoderConfigs::AAC;

    FlvReader flvReader;
    int frameCount = 0;

    while(!feof(pIn))
    {
        int nRead = fread(buf, 1, sizeof(buf), pIn);

        std::vector<std::shared_ptr<FlvTag>> tagVec;
        if(!flvReader.appendAndParse(std::string(buf, nRead), tagVec))
        {
            continue;
        }

        for(std::shared_ptr<FlvTag> pTag : tagVec)
        {
            if(!pTag->isAudioTag())
                continue;

            frameCount++;
            std::shared_ptr<AACAudioFlvTag> pATag = 
                std::dynamic_pointer_cast<AACAudioFlvTag>(pTag);
            if(pATag->isAACHeader())
            {
                decCfgs.decodeHeader = pATag->getRawData();
                decoder.setupDecoder(decCfgs);
            }
            else
            {
                FFDecodePacket pkt;
                pkt.pts = pkt.dts = pATag->getTagTimeStamp();
                pkt.packetData = pATag->getRawData();

                FFDecodeFrame frame;

                std::cout << "FlvTag pts=" << pkt.pts << " dts=" << pkt.dts
                    << " size=" << pkt.packetData.size() << std::endl;

                if(decoder.decode(pkt, frame))
                {
                    fwrite(frame.frameData.data(), 1, frame.frameData.size(), pOut);
                }
            }
            
        }
    }
}

void test()
{
    FILE* pIn = fopen("animal.flv", "rb");
    char buf[500 * 1024] = { 0 };

    FILE* pOut = fopen("animal.pcm", "wb");

    FFDecoder decoder;
    FFDecoderConfigs decCfgs;
    decCfgs.codecId = FFDecoderConfigs::AAC;

    FlvReader flvReader;
    int frameCount = 0;

    while(!feof(pIn) && frameCount < 50)
    {
        int nRead = fread(buf, 1, sizeof(buf), pIn);

        std::vector<std::shared_ptr<FlvTag>> tagVec;
        if(!flvReader.appendAndParse(std::string(buf, nRead), tagVec))
        {
            continue;
        }

        for(std::shared_ptr<FlvTag> pTag : tagVec)
        {
            if(!pTag->isVideoTag())
                continue;

            frameCount++;
            std::shared_ptr<AVCVideoFlvTag> pVTag = 
                std::dynamic_pointer_cast<AVCVideoFlvTag>(pTag);
            if(pVTag->isAvcHeader())
            {
                decCfgs.decodeHeader = pVTag->getRawData();
                decoder.setupDecoder(decCfgs);
            }
            else
            {
                FFDecodePacket pkt;
                pkt.dts = pVTag->getTagTimeStamp();
                pkt.pts = pkt.dts + pVTag->getCts();
                pkt.packetData = pVTag->getRawData();

                FFDecodeFrame frame;

                std::cout << "FlvTag pts=" << pkt.pts << " dts=" << pkt.dts
                    << " size=" << pkt.packetData.size() << std::endl;

                if(decoder.decode(pkt, frame))
                {
                    fwrite(frame.frameData.data(), 1, frame.frameData.size(), pOut);
                }
            }
            
        }
    }
}
