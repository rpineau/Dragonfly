//
//  Dragonfly.cpp
//  CDragonfly
//
//  Created by Rodolphe Pineau on 2023-11-25
//  Dragonfly X2 plugin

#include "Dragonfly.h"

CDragonfly::CDragonfly()
{
    // set some sane values
    m_bIsConnected = false;

    m_dCurrentAzPosition = 0.0;
    m_dCurrentElPosition = 0.0;

    m_nRoofState = UNKNOWN;
    m_RoofAction = IDLE;

    m_nRelayActiveDurationMs = 1000; // 1 second by default
    m_bCheckSafe = false;
    m_bCheckMountParked = false;

#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\Dragonfly-Log.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/Dragonfly-Log.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/Dragonfly-Log.txt";
#endif
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

#if defined PLUGIN_DEBUG
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CDragonfly] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CDragonfly] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif


}

CDragonfly::~CDragonfly()
{

}

int CDragonfly::Connect(std::string sIpAddress)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    struct hostent *server;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Called." << std::endl;
    m_sLogFile.flush();
#endif

    m_iSockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = gethostbyname(sIpAddress.c_str());

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_Sockfd : " << m_iSockfd << std::endl;
    m_sLogFile.flush();
#endif

    if (m_iSockfd < 0 || !server) {
        m_bIsConnected = false;
        return ERR_COMMOPENING;
    }

    memset(&m_Serveraddr, 0, sizeof(m_Serveraddr));

    m_Serveraddr.sin_family = AF_INET;
    memcpy(&m_Serveraddr.sin_addr.s_addr, server->h_addr, server->h_length);
    m_Serveraddr.sin_port = htons(10000); // default Dragonfly port
    m_nServerlen = sizeof(m_Serveraddr);

    m_bIsConnected = true;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] connected to " << sIpAddress << std::endl;
    m_sLogFile.flush();
#endif

    nErr = getFirmwareVersion(m_sVersion);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] no response from device at " << sIpAddress << std::endl;
        m_sLogFile.flush();
#endif
        m_bIsConnected = false;
        return ERR_NORESPONSE;
    }
    
    syncDome(m_dCurrentAzPosition,m_dCurrentElPosition);
    return nErr;
}


void CDragonfly::Disconnect()
{
    if(m_bIsConnected) {
    }
    m_bIsConnected = false;
}


int CDragonfly::domeCommand(std::string sCmd, std::string &sResp, int nTimeout)
{
    int nErr = PLUGIN_OK;
    unsigned long  ulBytesWrite = 0;
#ifdef SB_WIN_BUILD
    typedef int socklen_t;
    DWORD tvwin;
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [domeCommand] sending : '" << sCmd << "' " << std::endl;
    m_sLogFile.flush();
#endif


#if defined SB_LINUX_BUILD || defined SB_MAC_BUILD
    // Mac and Unix use a timeval struct to set the timout
    // Set timeout to MAX_TIMEOUT (multiply by 1000 as MAX_TIMEOUT is in ms)
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = MAX_TIMEOUT*1000;

    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_RCVTIMEO,(const char *) &tv,sizeof(tv));
    if (nErr) {
        return COMMAND_FAILED;
    }
    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_SNDTIMEO,(const char *) &tv,sizeof(tv));
    if (nErr) {
        return COMMAND_FAILED;
    }
#endif

#ifdef SB_WIN_BUILD
    // Timeout measured in milliseconds
    // Set timeout to MAX_TIMEOUT (MAX_TIMEOUT is in ms)
    tvwin = MAX_TIMEOUT;
    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tvwin, sizeof(tvwin));
    if (nErr) {
        return COMMAND_FAILED;
    }
    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tvwin, sizeof(tvwin));
    if (nErr) {
        return COMMAND_FAILED;
    }
#endif

    // Send command to the Dragonfly
    ulBytesWrite = sendto(m_iSockfd, sCmd.c_str(), sCmd.size(), 0,  (const sockaddr*) &m_Serveraddr,  m_nServerlen);
    if (ulBytesWrite < 0) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [domeCommand] error sending command." << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_TXTIMEOUT;
    }
    nErr = readResponse(sResp);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [domeCommand] error reading response, nErr : '" << nErr << "' " << std::endl;
        m_sLogFile.flush();
#endif

        return nErr;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [domeCommand] response : '" << sResp << "' " << std::endl;
    m_sLogFile.flush();
#endif
    if(sResp.size()==0)
        return ERR_CMDFAILED;
    return nErr;

}

int CDragonfly::readResponse(std::string &sResp, int nTimeout, char cEndOfResponse)
{
    int nErr = PLUGIN_OK;
    long lBytesRead = 0;
    long lTotalBytesRead = 0;
    char szRespBuffer[BUFFER_SIZE];
    char *pszBufPtr;
    int nBufferLen = BUFFER_SIZE -1;
    sockaddr retserver;

#ifdef SB_WIN_BUILD
    typedef int socklen_t;
#endif

    socklen_t lenretserver = sizeof(retserver);

    memset(szRespBuffer, 0, BUFFER_SIZE);
    sResp.clear();
    pszBufPtr = szRespBuffer;

    do {
        // Read response from socket
        lBytesRead = recvfrom(m_iSockfd, pszBufPtr, nBufferLen - lTotalBytesRead , 0, &retserver, &lenretserver);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] lBytesRead : " << lBytesRead << std::endl;
        m_sLogFile.flush();
#endif
        if(lBytesRead == -1) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 3
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] socket read error." << std::endl;
            m_sLogFile.flush();
#endif
            return BAD_CMD_RESPONSE;
        }
        lTotalBytesRead += lBytesRead;
        pszBufPtr+=lBytesRead;
    } while (lTotalBytesRead < BUFFER_SIZE  && *(pszBufPtr-1) != cEndOfResponse);


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] response in szRespBuffer : '" << szRespBuffer << "' " << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] lTotalBytesRead         : " << lTotalBytesRead << std::endl;
    m_sLogFile.flush();
#endif

    if(lTotalBytesRead>1) {
        *(pszBufPtr-1) = 0; //remove the end of response character
        sResp.assign(szRespBuffer);
    }
    return nErr;
}

int CDragonfly::getFirmwareVersion(std::string &sVersion)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::vector<std::string> respFields;

    nErr = domeCommand("!seletek version#", sResp); // pulse relay 1 for 1 second
     if(nErr) {
         return nErr;
     }
    nErr = parseFields(sResp, respFields, ':');
     if(nErr) {
         return nErr;
     }
    
     if(respFields.size()>1) {
         if(respFields[1].find("error")!=-1) {
             return ERR_CMDFAILED;
         }
         sVersion = respFields[1];
     }
     else {
         sVersion.clear();
         nErr = ERR_CMDFAILED;
     }
     
    return nErr;
}


int CDragonfly::syncDome(double dAz, double dEl)
{

    m_dCurrentAzPosition = dAz;
    m_dCurrentElPosition = dEl;

    return PLUGIN_OK;
}

int CDragonfly::parkDome()
{
    return PLUGIN_OK;
}

int CDragonfly::unparkDome()
{
    return PLUGIN_OK;
}

int CDragonfly::gotoAzimuth(double dNewAz)
{
    int nErr = PLUGIN_OK;


    m_dCurrentAzPosition = dNewAz;
    if(m_nRoofState == OPEN)
        m_dCurrentElPosition = 90.0;
    else
        m_dCurrentElPosition = 0.0;

    return nErr;
}

int CDragonfly::openRoof()
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::stringstream ssCmd;
    bool bIsSafe;
    bool bIsMountParked;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [openRoof] Opening roof." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = getState();
    if(nErr)
        return nErr;

    nErr = getSafeState(bIsSafe);
    if(nErr) {
        return ERR_CMDFAILED;
    }

    if(!bIsSafe)
        return ERR_CMDFAILED;

    nErr = getSafeMountState(bIsMountParked);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    if(!bIsMountParked)
        return ERR_CMDFAILED;
    
    ssCmd << "!relio rlpulse 0 0 " << m_nRelayActiveDurationMs << "#";
    // for motor controller with a Open/Stop/Close system like the Aleko, we only need to activate the relay for a short period
    nErr = domeCommand(ssCmd.str(), sResp); // pulse relay 1 for 1 second
    m_RoofAction = OPENING;
    return nErr;
}

int CDragonfly::closeRoof()
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::stringstream ssCmd;
    bool bIsSafe;
    bool bIsMountParked;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [closeRoof] Closing roof." << std::endl;
    m_sLogFile.flush();
#endif
    nErr = getState();
    if(nErr)
        return nErr;

    nErr = getSafeState(bIsSafe);
    if(nErr) {
        return ERR_CMDFAILED;
    }

    if(!bIsSafe)
        return ERR_CMDFAILED;

    nErr = getSafeMountState(bIsMountParked);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    if(!bIsMountParked)
        return ERR_CMDFAILED;

    ssCmd << "!relio rlpulse 0 0 " << m_nRelayActiveDurationMs << "#";
    // for motor controller with a Open/Stop/Close system like the Aleko, we only need to activate the relay for a short period
    nErr = domeCommand(ssCmd.str(), sResp); // pulse relay 1 for 1 second
    m_RoofAction = CLOSING;

    return nErr;
}

int CDragonfly::findHome()
{
	int nErr = PLUGIN_OK;

	if(!m_bIsConnected)
		return NOT_CONNECTED;

	return nErr;
}

int CDragonfly::isGoToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;
    bComplete = true;

    return nErr;
}

int CDragonfly::isOpenComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    bComplete = false;

    if(!m_bIsConnected)
        return NOT_CONNECTED;
    
    nErr = getState();
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isOpenComplete] error getting state : " << nErr << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }
    
    if(m_nRoofState == OPEN) {
        m_RoofAction = IDLE;
        bComplete = true;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isOpenComplete] m_nRoofState : " << m_nRoofState << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isOpenComplete] bComplete : " << (bComplete?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CDragonfly::isCloseComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    bComplete = false;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    nErr = getState();
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isCloseComplete] error getting state : " << nErr << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }

    if(m_nRoofState == CLOSED) {
        m_RoofAction = IDLE;
        bComplete = true;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isCloseComplete] m_nRoofState : " << m_nRoofState << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isCloseComplete] bComplete : " << (bComplete?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}


int CDragonfly::isParkComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    bComplete = true;
    return nErr;
}

int CDragonfly::isUnparkComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;


    bComplete = true;

    return nErr;
}

int CDragonfly::isFindHomeComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    bComplete = true;

    return nErr;

}


int CDragonfly::abortMove()
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::stringstream ssCmd;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [abortMove] aborting roof move." << std::endl;
    m_sLogFile.flush();
#endif
    if(m_RoofAction!=IDLE) {
        ssCmd << "!relio rlpulse 0 0 " << m_nRelayActiveDurationMs << "#";
        // for motor controller with a Open/Stop/Close system like the Aleko, we only need to activate the relay for a short period
        nErr = domeCommand(ssCmd.str(), sResp); // pulse relay 1 for 1 second
        if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [abortMove] error : " << nErr << std::endl;
            m_sLogFile.flush();
#endif
        }
        m_RoofAction = IDLE;
    }
    return nErr;
}

void CDragonfly::setRelayPulseTime(double dTimne)
{
    m_nRelayActiveDurationMs = int(dTimne*1000);
}

double CDragonfly::getRelayPulseTime()
{
    return m_nRelayActiveDurationMs/1000;
}

void CDragonfly::setCheckSafe(bool bCheckSafe)
{
    m_bCheckSafe = bCheckSafe;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCheckSafe] m_bCheckSafe : " << (m_bCheckSafe?"Yes":"No")<< std::endl;
    m_sLogFile.flush();
#endif

}

bool CDragonfly::getCheckSafe()
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCheckSafe] m_bCheckSafe : " << (m_bCheckSafe?"Yes":"No")<< std::endl;
    m_sLogFile.flush();
#endif
    return m_bCheckSafe;
}

void CDragonfly::setCheckMountParked(bool bCheckMountParked)
{
    m_bCheckMountParked = bCheckMountParked;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCheckMountParked] m_bCheckSafe : " << (m_bCheckMountParked?"Yes":"No")<< std::endl;
    m_sLogFile.flush();
#endif

}

bool CDragonfly::getCheckMountParked()
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCheckMountParked] m_bCheckSafe : " << (m_bCheckMountParked?"Yes":"No")<< std::endl;
    m_sLogFile.flush();
#endif
    return m_bCheckMountParked;
}



#pragma mark - Getter / Setter


double CDragonfly::getCurrentAz()
{
    return m_dCurrentAzPosition;
}

double CDragonfly::getCurrentEl()
{

    return m_dCurrentElPosition;
}

int CDragonfly::getState()
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::vector<std::string> vFields;
    int input0, input1;
    
    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] Getting roof state." << std::endl;
    m_sLogFile.flush();
#endif

    // read input 0
    nErr = domeCommand("!relio sndgrd 0 0#", sResp);
    if(nErr)
        return nErr;

    nErr = parseFields(sResp, vFields, ':');
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] Error parsing response : " << sResp << std::endl;
        m_sLogFile.flush();
#endif
        m_nRoofState = UNKNOWN;
        return nErr;
    }
    
    input0=0;
    if(vFields.size()>1) {
        if(vFields[1].find("error")!=-1) {
            return ERR_CMDFAILED;
        }
        input0 = std::stoi(vFields[1]);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] input0 : " << input0 << std::endl;
        m_sLogFile.flush();
#endif
    }

    // read input 1
    nErr = domeCommand("!relio sndgrd 0 1#", sResp);
    if(nErr)
        return nErr;

    nErr = parseFields(sResp, vFields, ':');
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] Error parsing response : " << sResp << std::endl;
        m_sLogFile.flush();
#endif
        m_nRoofState = UNKNOWN;
        return nErr;
    }

    input1 = 0;
    if(vFields.size()>1) {
        if(vFields[1].find("error")!=-1) {
            return ERR_CMDFAILED;
        }
        input1 = std::stoi(vFields[1]);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] input1 : " << input1 << std::endl;
        m_sLogFile.flush();
#endif
    }

    if (input0 == 0 && input1 == 0 && m_RoofAction != IDLE) {
        m_nRoofState = MOVING;
    }
    else if (input0 == 1 && input1 == 0) {
        m_nRoofState = OPEN;
    }
    else if (input0 == 0 && input1 == 1) {
        m_nRoofState = CLOSED;
    }
    else {
        m_nRoofState = UNKNOWN;
    }


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] m_nRoofState : " << m_nRoofState << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CDragonfly::getSafeState(bool &bIsSafe)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::vector<std::string> vFields;
    int safeInput;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    if(!m_bCheckSafe) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeState] input 3 safety check DISABLED." << std::endl;
        m_sLogFile.flush();
#endif
        bIsSafe = true;
        return PLUGIN_OK;
    }

    bIsSafe = false;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeState] input 3 state for safety check." << std::endl;
    m_sLogFile.flush();
#endif

    // read input 2
    nErr = domeCommand("!relio sndgrd 0 2#", sResp);
    if(nErr)
        return nErr;

    nErr = parseFields(sResp, vFields, ':');
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getState] Error parsing response : " << sResp << std::endl;
        m_sLogFile.flush();
#endif
        bIsSafe = false;
        m_nRoofState = UNKNOWN;
        return nErr;
    }
    
    safeInput=0;
    if(vFields.size()>1) {
        if(vFields[1].find("error")!=-1) {
            return ERR_CMDFAILED;
        }
        safeInput = std::stoi(vFields[1]);
    }

    if(safeInput == 0) {
        bIsSafe = true;
    }
    else {
        bIsSafe = false;
    }
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeState] bIsSafe      : " << (bIsSafe?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CDragonfly::getSafeMountState(bool &bIsSafe)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::vector<std::string> vFields;
    int safeInput;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    if(!m_bCheckMountParked) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeMountState] input 8 safety check DISABLED." << std::endl;
        m_sLogFile.flush();
#endif
        bIsSafe = true;
        return PLUGIN_OK;
    }

    bIsSafe = false;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeMountState] input 8 state for safety check." << std::endl;
    m_sLogFile.flush();
#endif

    // read input 2
    nErr = domeCommand("!relio sndgrd 0 7#", sResp);
    if(nErr)
        return nErr;

    nErr = parseFields(sResp, vFields, ':');
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeMountState] Error parsing response : " << sResp << std::endl;
        m_sLogFile.flush();
#endif
        bIsSafe = false;
        m_nRoofState = UNKNOWN;
        return nErr;
    }
    
    safeInput=0;
    if(vFields.size()>1) {
        if(vFields[1].find("error")!=-1) {
            return ERR_CMDFAILED;
        }
        safeInput = std::stoi(vFields[1]);
    }

    if(safeInput == 0) {
        bIsSafe = true;
    }
    else {
        bIsSafe = false;
    }
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSafeMountState] bIsSafe      : " << (bIsSafe?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CDragonfly::parseFields(const std::string sIn, std::vector<std::string> &svFields, char cSeparator)
{
    int nErr = PLUGIN_OK;
    std::string sSegment;
    std::stringstream ssTmp(sIn);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [parseFields] Called." << std::endl;
    m_sLogFile.flush();
#endif
    if(sIn.size() == 0)
        return ERR_PARSE;

    svFields.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, cSeparator))
    {
        svFields.push_back(sSegment);
    }

    if(svFields.size()==0) {
        nErr = ERR_PARSE;
    }
    return nErr;
}

std::string& CDragonfly::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& CDragonfly::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& CDragonfly::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}

std::string CDragonfly::findField(std::vector<std::string> &svFields, const std::string& token)
{
    for(int i=0; i<svFields.size(); i++){
        if(svFields[i].find(token)!= -1) {
            return svFields[i];
        }
    }
    return std::string();
}


#ifdef PLUGIN_DEBUG
const std::string CDragonfly::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#endif
