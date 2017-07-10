/*

    OpenPnp-Capture: a video capture subsystem.

    Created by Niels Moseley on 7/11/17.
    Copyright © 2017 Niels Moseley. All rights reserved.

    Platform independent context class to keep track
    of the global state.

*/

#ifndef openpnp_context_h
#define openpnp_context_h

#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <string>
#include <map>
#include <stdint.h>

#include "openpnp-capture.h"

#ifdef _WIN32
    #pragma comment(lib, "strmiids")
    #include "../win/deviceinfo.h"
#elif __linux__
    #include "../linux/deviceinfo.h"
#else
    #include "../mac/deviceinfo.h"
#endif


class Stream;   // pre-declaration

/** context base class keeps track of all the platform independent
    objects and information */

class Context
{
public:
    /** Create a context for the library.
        Device enumeration is perform in the constructor,
        so all devices must be present in the system when
        the Context is created or devices will not be found.

        Re-enumeration support is pending.
    */
    Context();
    virtual ~Context();

    /** Get the UTF-8 device name of a device with index/ID id */
    const char* getDeviceName(CapDeviceID id) const;

    /** Return the number of devices found */
    uint32_t    getDeviceCount() const;

    /** Opens a stream to a device with index/ID id and returns the stream ID.
        If an error occurs (device not found), -1 is returned.

        If the stream is succesfully opnened, capturing starts automatically
        until the stream (or its associated context) is closed with closeStream.

        Note: for now, only one stream per device is supported but opening more
              streams might or might not work.
    */
    int32_t openStream(CapDeviceID id);

    /** close the stream to a device */
    bool closeStream(int32_t streamID);

    /** returns 1 if the stream is open and capturing, else 0 */
    uint32_t isOpenStream(int32_t streamID);

    /** returns true if succeeds, else false */
    bool captureFrame(int32_t streamID, uint8_t *RGBbufferPtr, size_t RGBbufferBytes);

    /** returns true if the stream has a new frame, false otherwise */
    bool hasNewFrame(int32_t streamID);

    /** returns the number of frames captured during the lifetime of the stream */
    uint32_t getStreamFrameCount(int32_t streamID);

    bool setStreamExposure(int32_t streamID, int32_t value);

    bool setStreamAutoExposure(int32_t streamID, bool enable);

    bool getStreamExposureLimits(int32_t streamID, int32_t *min, int32_t *max);

protected:
    /** Enumerate DirectShow capture devices and put their 
        information into the m_devices array 
        
        Implement this function in a platform-dependent
        derived class.
    */
    virtual bool enumerateDevices() = 0;

    /** Lookup a stream by ID and return a pointer
        to it if it exists. If it doesnt exist, 
        return NULL */
    Stream* lookupStreamByID(int32_t ID);

    /** Store a stream pointer in the m_streams map
        and return its unique ID */
    int32_t storeStream(Stream *stream);

    /** Remove a stream from the m_streams map.
        Return true if this was successful */
    bool removeStream(int32_t ID);



    /** Convert a wide character string to an UTF-8 string 
        
        Implement this function in a platform-dependent
        derived class.    
    */
    virtual std::string wstringToString(const std::wstring &wstr) = 0;

    /** Convert a wide charater string to an UTF-8 string
        
        Implement this function in a platform-dependent
        derived class.    
    */
    virtual std::string wcharPtrToString(const wchar_t *str) = 0;
    

    std::vector<deviceInfo>     m_devices;  ///< list of enumerated devices
    std::map<int32_t, Stream*>  m_streams;  ///< collection of streams
    int32_t                     m_streamCounter;
};

#endif