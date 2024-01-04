//
//  Dragonfly.h
//  CDragonfly
//
//  Created by Rodolphe Pineau on 2023-11-25
//  Dragonfly X2 plugin

#ifndef __Dragonfly__
#define __Dragonfly__
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>


#ifdef WIN32
#include <time.h>
#include <WinSock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

// C++ includes
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>




#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

#define PLUGIN_VERSION  1.0

// #define PLUGIN_DEBUG 3

#define BUFFER_SIZE 4096
#define MAX_TIMEOUT 500

// error codes
enum DragonflyErrors {PLUGIN_OK=0, NOT_CONNECTED, CANT_CONNECT, BAD_CMD_RESPONSE, COMMAND_FAILED, NO_DATA_TIMEOUT, ERR_PARSE};

// Error code
enum DragonflyRoofState {OPEN=0, MOVING, CLOSED , UNKNOWN};
enum RoofAction {IDLE=0, OPENING, CLOSING};


class CDragonfly
{
public:
    CDragonfly();
    ~CDragonfly();

    int         Connect(std::string sIpAddress);
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; }

    int         getFirmwareVersion(std::string &sVersion);
    // Dome commands
    int syncDome(double dAz, double dEl);
    int parkDome(void);
    int unparkDome(void);
    int gotoAzimuth(double dNewAz);
    int openRoof();
    int closeRoof();
	int	findHome();

    // command complete functions
    int isGoToComplete(bool &bComplete);
    int isOpenComplete(bool &bComplete);
    int isCloseComplete(bool &bComplete);
    int isParkComplete(bool &bComplete);
    int isUnparkComplete(bool &bComplete);
    int isFindHomeComplete(bool &bComplete);

    double getCurrentAz();
    double getCurrentEl();

    int abortMove();
    
    void        setRelayPulseTime(double dTimne);
    double      getRelayPulseTime();
    
    void        setCheckSafe(bool bCheckSafe);
    bool        getCheckSafe();
    void        setCheckMountParked(bool bCheckSafe);
    bool        getCheckMountParked();

protected:

    bool            m_bIsConnected;
    int             m_nRoofState;
    double          m_dCurrentAzPosition;
    double          m_dCurrentElPosition;
    int             m_RoofAction;
    bool            m_bCheckSafe;
    bool            m_bCheckMountParked;
    std::string     m_sVersion;
    
    int             m_nRelayActiveDurationMs;
    
    int             readResponse(std::string &sResp, int nTimeout = MAX_TIMEOUT, char cEndOfResponse = '#');
    int             deviceCommand(std::string sCmd, std::string &sResp, int nTimeout = MAX_TIMEOUT);
    int             getState();
    int             getSafeState(bool &bIsSafe);
    int             getSafeMountState(bool &bIsSafe);
    int             parseFields(const std::string sIn, std::vector<std::string> &svFields, char cSeparator);

    std::string&    trim(std::string &str, const std::string &filter );
    std::string&    ltrim(std::string &str, const std::string &filter);
    std::string&    rtrim(std::string &str, const std::string &filter);
    std::string     findField(std::vector<std::string> &svFields, const std::string& token);

    // network
#ifdef WIN32
    WSADATA m_WSAData;
    SOCKET m_iSockfd;
#else
    int m_iSockfd;
#endif
    int m_nServerlen;  // Length of server address
    struct sockaddr_in m_Serveraddr; // Struct for server address
    //#endif

#ifdef PLUGIN_DEBUG
    // timestamp for logs
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
#endif

};

#endif
