#ifndef _MEDIA_FILE_STREAM_H_
#define _MEDIA_FILE_STREAM_H_

#include "flvcpp/FlvReader.h"
#include <iostream>
#include <deque>

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
    
    bool readFrame(std::string& frameData, std::string& frameHeader, 
        uint32_t& dts, uint32_t& pts, FrameType& type);
private:
    void checkLoadFile();
private:
    FlvReader m_flvReader;
    FILE* m_pFlvFile;
    char m_readBuffer[1024 * 1024];

    std::string m_videoHeader;
    std::string m_audioHeader;
    std::deque<std::shared_ptr<FlvTag>> m_cacheFlvTags;
};

#endif
