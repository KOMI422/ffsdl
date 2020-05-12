#ifndef _MEDIA_FILE_STREAM_H_
#define _MEDIA_FILE_STREAM_H_

#include "flvcpp/FlvReader.h"
#include <iostream>
#include <deque>
#include <chrono>

class MediaFileStream
{
public:
    enum FrameType
    {
        VIDEO = 0,
        AUDIO = 1,
    };  
public:
    MediaFileStream();
    virtual ~MediaFileStream();

    bool openFile(const std::string& path);
    
    int32_t readFrame(std::string& frameData, std::string& frameHeader, 
        uint32_t& dts, uint32_t& pts, FrameType& type);
    bool isEndOfFile() const { return m_readSpeedCtrl.isEndOfFile; }
private:
    int32_t checkLoadFile();
private:
    FlvReader m_flvReader;
    FILE* m_pFlvFile;
    char m_readBuffer[1024 * 1024];

    std::string m_videoHeader;
    std::string m_audioHeader;
    std::deque<std::shared_ptr<FlvTag>> m_cacheFlvTags;

    struct ReadSpeedControl
    {
        bool isEndOfFile;
        uint32_t fileBitRate;
        uint32_t bufferSize;
        char* pBuffer;

        ReadSpeedControl()
        {
            isEndOfFile = false;
            fileBitRate = 0;
            bufferSize = 0;
            pBuffer = NULL;
        }

        void updateBitRate(uint32_t br)
        {
            if(br != fileBitRate)
            {
                std::cout << "UpdateBitRate " << fileBitRate << "->" << br << std::endl;
                fileBitRate = br;
                bufferSize = fileBitRate / 8;

                if(pBuffer != NULL)
                    delete[] pBuffer;
                pBuffer = new char[bufferSize];
            }
        }

        uint32_t calcReadSize(const std::deque<std::shared_ptr<FlvTag>>& cacheTags)
        {
            if(isEndOfFile)
                return 0;

            uint32_t cacheBufLen = 0;
            if(cacheTags.size() >= 2)
            {
                cacheBufLen = cacheTags.back()->getTagTimeStamp() - cacheTags.front()->getTagTimeStamp();
            }

            if(cacheBufLen >= 1000)
            {
                return 0;
            }

            uint32_t readSize = bufferSize;// * interval.count() / 1000;

            return readSize;
        }
    };
    ReadSpeedControl m_readSpeedCtrl;

    struct OutSpeedControl
    {
        bool isStartControl;
        uint32_t baseTagTs;
        std::chrono::steady_clock::time_point baseClockTs;

        OutSpeedControl()
        {
            isStartControl = false;
            baseTagTs = 0;
            baseClockTs = std::chrono::steady_clock::now();
        }
        
        bool isUnderControl(uint32_t curTagTs)
        {
            if(!isStartControl)
            {
                isStartControl = true;
                baseTagTs = curTagTs;
                baseClockTs = std::chrono::steady_clock::now();
                return false;
            }

            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            std::chrono::milliseconds clockInterval = std::chrono::duration_cast<std::chrono::milliseconds>(now - baseClockTs);
            uint32_t tagInterval = curTagTs - baseTagTs;

            if(tagInterval > clockInterval.count() + 30 * 1000 && clockInterval.count() >= 1000)
            {
                //时间戳跳跃
                baseTagTs = curTagTs;
                return false;
            }

            bool underControl = false;
            if(tagInterval > clockInterval.count() && (tagInterval - clockInterval.count()) >= 1000)
            {
                underControl = true;
            }

            return underControl;
        }
    };
    OutSpeedControl m_outSpeedCtrl;
};

#endif
