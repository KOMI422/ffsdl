#include "MediaFileStream.h"

MediaFileStream::MediaFileStream()
{
    m_pFlvFile = NULL;
    m_readSpeedCtrl.updateBitRate(500 * 1000);
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

int32_t MediaFileStream::readFrame(std::string& frameData, std::string& frameHeader, 
        uint32_t& dts, uint32_t& pts, FrameType& type)
{
    if(!m_readSpeedCtrl.isEndOfFile)
    {
        checkLoadFile();
    }

    while(true)
    {
        if (m_cacheFlvTags.empty())
            return m_readSpeedCtrl.isEndOfFile ? -1 : 0;

        std::shared_ptr<FlvTag> pTag = m_cacheFlvTags.front();
        if(m_outSpeedCtrl.isUnderControl(pTag->getTagTimeStamp()))
            return 0;

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
        else if(pTag->isScriptDataTag())
        {
            std::shared_ptr<ScriptDataTag> pDTag = 
                std::dynamic_pointer_cast<ScriptDataTag>(pTag);

            uint32_t videoKBitRate = atoi(pDTag->getPropertyValue("videodatarate", "500").c_str());
            uint32_t audioKBitRate = atoi(pDTag->getPropertyValue("audiodatarate", "100").c_str());
            std::cout << "MetaData: videoKbps=" << videoKBitRate << " audioKbps=" << audioKBitRate << std::endl;

            uint32_t fileBitRate = (videoKBitRate + audioKBitRate) * 1000;
            fileBitRate = (fileBitRate == 0) ? 500000 : fileBitRate;
            m_readSpeedCtrl.updateBitRate(fileBitRate * 1.5);
            // m_readSpeedCtrl.updateBitRate(15 * 1000 * 1000);
            continue;
        }
    }

    return 1;
}

int32_t MediaFileStream::checkLoadFile()
{
    // if(!m_cacheFlvTags.empty())
    //     return;
    uint32_t readSize = m_readSpeedCtrl.calcReadSize(m_cacheFlvTags);
    if(readSize == 0)
        return 0;

    // std::cout << "CheckLoadFile size=" << readSize << std::endl;
    int nRead = fread(m_readBuffer, 1, readSize, m_pFlvFile);
    if(nRead > 0)
    {
        std::vector<std::shared_ptr<FlvTag>> tagVec;
        m_flvReader.appendAndParse(std::string(m_readBuffer, nRead), tagVec);

        m_cacheFlvTags.insert(m_cacheFlvTags.end(), tagVec.begin(), tagVec.end());
    }

    m_readSpeedCtrl.isEndOfFile = (nRead < readSize) ? feof(m_pFlvFile) : false;
    return m_readSpeedCtrl.isEndOfFile ? -1 : nRead;
}