/*

    OpenPnp-Capture: a video capture subsystem.

    Platform independent context class.

    Copyright (c) 2017 Jason von Nieda, Niels Moseley.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <vector>
#include "context.h"
#include "logging.h"
#include "stream.h"

Context::Context() :
    m_streamCounter(0)
{
    //NOTE: derived platform dependent class must enumerate
    //      the devices here and place them in m_devices.
}

Context::~Context()
{
    // delete stream objects
    auto iter = m_streams.begin();
    while(iter != m_streams.end())
    {
        delete iter->second;
        iter++;
    }
    LOG(LOG_DEBUG, "Context destroyed\n");
}

const char* Context::getDeviceName(CapDeviceID id) const
{
    if (id >= m_devices.size())
    {
        LOG(LOG_ERR,"Device with ID %d not found", id);
        return NULL; // no such device ID!
    }
    if (m_devices[id] == nullptr)
    {
        LOG(LOG_ERR,"Internal device pointer is NULL");
        return NULL; // device pointer is NULL!
    }
    return m_devices[id]->m_name.c_str();
}

uint32_t Context::getDeviceCount() const
{
    return m_devices.size();
}


int32_t Context::getNumFormats(CapDeviceID index) const
{
    if (index >= m_devices.size())
    {
        LOG(LOG_ERR,"Device with ID %d not found", index);
        return -1; // no such device ID!
    }
    if (m_devices[index] == nullptr)
    {
        LOG(LOG_ERR,"Internal device pointer is NULL");
        return -1; // device pointer is NULL!
    }
    return m_devices[index]->m_formats.size();
}


bool Context::getFormatInfo(CapDeviceID index, CapFormatID formatID, CapFormatInfo *info) const
{
    if (index >= m_devices.size())
    {
        LOG(LOG_ERR,"Device with ID %d not found", index);
        return false; // no such device ID!
    }
    if (m_devices[index] == nullptr)
    {
        LOG(LOG_ERR,"Internal device pointer is NULL");
        return false; // device pointer is NULL!
    }
    if (formatID < m_devices[index]->m_formats.size())
    {
        *info = m_devices[index]->m_formats[formatID];
    }
    else
    {
        LOG(LOG_ERR,"Invalid format ID (got %d but max ID is %d)\n", formatID, m_devices[index]->m_formats.size());
        return false; // invalid format ID 
    }
    return true;
}

int32_t Context::openStream(CapDeviceID id, CapFormatID formatID)
{
    deviceInfo *device = nullptr;

    if (m_devices.size() > id)
    {
        device = m_devices[id];
    }
    else
    {
        LOG(LOG_ERR, "openStream: No devices found\n");
        return -1;
    }

    // lookup desired format
    if (formatID >= device->m_formats.size())
    {
        LOG(LOG_ERR, "openStream: Requested format index out of range\n");
        return -1;        
    }

    Stream *s = createPlatformStream();

    if (!s->open(this, device, device->m_formats[formatID].width,
                 device->m_formats[formatID].height,
                 device->m_formats[formatID].fourcc))
    {
        LOG(LOG_ERR, "Could not open stream for device %s\n", device->m_name.c_str());
        return -1;
    }
    else
    {
        printf("[DBG ] FOURCC = ");
        uint32_t fcc = s->getFOURCC();
        for(uint32_t i=0; i<4; i++)
        {            
            printf("%c", (fcc & 0xff));
            fcc >>= 8;
        }
        printf("\n");
    }

    int32_t streamID = storeStream(s);
    return streamID;
}

bool Context::closeStream(int32_t streamID)
{
    if (streamID < 0)
    {
        LOG(LOG_ERR, "closeStream was called with a negative stream ID\n");
        return false;
    }

    // remove and delete stream from collection
    if (!removeStream(streamID))
    {
        LOG(LOG_ERR, "could not remove stream with ID %d from m_streams.\n", streamID);
    }
    
    return true;
}

uint32_t Context::isOpenStream(int32_t streamID)
{
    if (streamID < 0)
    {
        LOG(LOG_ERR, "isOpenStream was called with a negative stream ID\n");
        return 0;
    }    

    if (static_cast<uint32_t>(streamID) >= m_streams.size())
    {
        LOG(LOG_ERR, "isOpenStream was called with an out-of-bounds stream ID\n");
        return 0;        
    }

    return m_streams[streamID]->isOpen() ? 1 : 0;
}

bool Context::captureFrame(int32_t streamID, uint8_t *RGBbufferPtr, size_t RGBbufferBytes)
{
    if (streamID < 0)
    {
        LOG(LOG_ERR, "captureFrame was called with a negative stream ID\n");
        return false;
    }    

    if (static_cast<uint32_t>(streamID) >= m_streams.size())
    {
        LOG(LOG_ERR, "captureFrame was called with an out-of-bounds stream ID\n");
        return false;
    }
    
    return m_streams[streamID]->captureFrame(RGBbufferPtr, RGBbufferBytes);
}

bool Context::hasNewFrame(int32_t streamID)
{
    if (streamID < 0)
    {
        LOG(LOG_ERR, "hasNewFrame was called with a negative stream ID\n");
        return false;
    }    

    if (static_cast<uint32_t>(streamID) >= m_streams.size())
    {
        LOG(LOG_ERR, "hasNewFrame was called with an out-of-bounds stream ID\n");
        return false;        
    }

    return m_streams[streamID]->hasNewFrame();
}

uint32_t Context::getStreamFrameCount(int32_t streamID)
{
    if (streamID < 0)
    {
        LOG(LOG_ERR, "getStreamFrameCount was called with a negative stream ID\n");
        return 0;
    }    

    if (static_cast<uint32_t>(streamID) >= m_streams.size())
    {
        LOG(LOG_ERR, "getStreamFrameCount was called with an out-of-bounds stream ID\n");
        return 0;        
    }

    return m_streams[streamID]->getFrameCount();
}

/** Lookup a stream by ID and return a pointer
    to it if it exists. If it doesnt exist, 
    return NULL */
Stream* Context::lookupStreamByID(int32_t ID)
{
    auto it = m_streams.find(ID);
    if (it != m_streams.end())
    {
        return it->second;
    }
    return nullptr;
}

/** Store a stream pointer in the m_streams map
    and return its unique ID */
int32_t Context::storeStream(Stream *stream)
{   
    int32_t ID = m_streamCounter++; 
    m_streams.insert(std::pair<int32_t,Stream*>(ID, stream));    
    return ID;
}

/** Remove a stream from the m_streams map.
    Return true if this was successful */
bool Context::removeStream(int32_t ID)
{
    auto it = m_streams.find(ID);
    if (it != m_streams.end())
    {
        delete it->second;
        m_streams.erase(it);
        return true;
    }
    return false;
}

bool Context::getStreamPropertyLimits(int32_t streamID, uint32_t propertyID, int32_t *min, int32_t *max)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->getPropertyLimits(propertyID, min, max);
}

bool Context::setStreamAutoProperty(int32_t streamID, uint32_t propertyID, bool enable)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->setAutoProperty(propertyID, enable);
}

bool Context::setStreamProperty(int32_t streamID, uint32_t propertyID, int32_t value)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->setProperty(propertyID, value);
}

#if 0
bool Context::setStreamExposure(int32_t streamID, int32_t value)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->setExposure(value);
}

bool Context::setStreamAutoExposure(int32_t streamID, bool enable)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->setAutoExposure(enable);
}

bool Context::getStreamExposureLimits(int32_t streamID, int32_t *min, int32_t *max)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->getExposureLimits(min, max);
}

/** Get the focus min and max in 'camera' units */
bool Context::getStreamFocusLimits(int32_t streamID, int32_t *min, int32_t *max)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->getFocusLimits(min, max);
}

/** Set the focus in 'camera' units */
bool Context::setStreamFocus(int32_t streamID, int32_t value)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->setFocus(value);
}

/** Set enable/disable the automatic focus */
bool Context::setStreamAutoFocus(int32_t streamID, bool enable)
{
    Stream* stream = Context::lookupStreamByID(streamID);
    if (stream == nullptr) return false;
    return stream->setAutoFocus(enable);
}

#endif

/** convert a FOURCC uint32_t to human readable form */
std::string fourCCToString(uint32_t fourcc)
{
    std::string v;
    for(uint32_t i=0; i<4; i++)
    {
        v += static_cast<char>(fourcc & 0xFF);
        fourcc >>= 8;
    }
    return v;
};
