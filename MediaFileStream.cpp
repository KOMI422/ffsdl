#include "MediaFileStream.h"

MediaFileStream::MediaFileStream()
{
    m_pFlvFile = NULL;
}

MediaFileStream::~MediaFileStream()
{
    if(m_pFlvFile != NULL)
    {
        fclose(m_pFlvFile);
        m_pFlvFile = NULL;
    }
}

bool MediaFileStream::openFile(const std::string& path)
{
    m_pFlvFile = fopen(path.c_str(), "rb");
    if(m_pFlvFile == NULL)
    {
        return false;
    }

    return true;
}

bool MediaFileStream::readFrame(std::string& frameData, std::string& frameHeader, 
        uint32_t& dts, uint32_t& pts, FrameType& type)
{
    checkLoadFile();

    if(m_cacheFlvTags.empty())
        return false;

    while(true)
    {
        std::shared_ptr<FlvTag> pTag = m_cacheFlvTags.front();
        m_cacheFlvTags.pop_front();

        if(pTag->isVideoTag())
        {
            std::shared_ptr<AVCVideoFlvTag> pVTag = 
                std::dynamic_pointer_cast<AVCVideoFlvTag>(pTag);
            if(pVTag->isAvcHeader())
            {
                m_videoHeader = pVTag->getRawData();
                continue;
            }
            else
            {
                frameData = pVTag->getRawData();
                frameHeader = m_videoHeader;
                dts = pVTag->getTagTimeStamp();
                pts = pVTag->getCts() + dts;
                type = VIDEO;
                break;
            }
        }
        else if(pTag->isAudioTag())
        {
            std::shared_ptr<AACAudioFlvTag> pATag = 
                std::dynamic_pointer_cast<AACAudioFlvTag>(pTag);
            if(pATag->isAACHeader())
            {
                m_audioHeader = pATag->getRawData();
                continue;
            }
            else
            {
                frameData = pATag->getRawData();
                frameHeader = m_audioHeader;
                dts = pts = pATag->getTagTimeStamp();
                type = AUDIO;
                break;
            }
        }
    }

    return true;
}

void MediaFileStream::checkLoadFile()
{
    if(!m_cacheFlvTags.empty())
        return;

    int nRead = fread(m_readBuffer, 1, sizeof(m_readBuffer), m_pFlvFile);
    if(nRead > 0)
    {
        std::vector<std::shared_ptr<FlvTag>> tagVec;
        m_flvReader.appendAndParse(std::string(m_readBuffer, nRead), tagVec);

        m_cacheFlvTags.insert(m_cacheFlvTags.end(), tagVec.begin(), tagVec.end());
    }
}