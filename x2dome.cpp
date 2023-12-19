#include "x2dome.h"


X2Dome::X2Dome(const char* pszSelection,
							 const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface*	pTheSkyXForMounts,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*			pIniUtil,
					MutexInterface*						pIOMutex,
					TickCountInterface*					pTickCount)
{

    m_nPrivateISIndex				= nISIndex;
	m_pSerX							= pSerX;
	m_pTheSkyXForMounts				= pTheSkyXForMounts;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;

	m_bLinked = false;
    
    if (m_pIniUtil)
    {
        char szIpAddress[255];
        m_Dragonfly.setRelayPulseTime(m_pIniUtil->readDouble(PARENT_KEY, CHILD_KEY_RELAY_PULSE, 1.0));
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_IP, "192.168.1.123", szIpAddress, 255);
        m_Dragonfly.setCheckSafe(m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_SAFE, 0) == 1 );
        m_Dragonfly.setCheckMountParked(m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_MOUNT, 0) == 1 );
        m_sIpAddress.assign(szIpAddress);
    }
}


X2Dome::~X2Dome()
{
	if (m_pSerX)
		delete m_pSerX;
	if (m_pTheSkyXForMounts)
		delete m_pTheSkyXForMounts;
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;

}


int X2Dome::establishLink(void)
{
    int nErr = SB_OK;

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    nErr = m_Dragonfly.Connect(m_sIpAddress);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

	return nErr;
}

int X2Dome::terminateLink(void)
{
    X2MutexLocker ml(GetMutex());
    m_Dragonfly.Disconnect();
	m_bLinked = false;
	return SB_OK;
}

 bool X2Dome::isLinked(void) const
{
	return m_bLinked;
}


int X2Dome::queryAbstraction(const char* pszName, void** ppVal)
{
    *ppVal = NULL;

    if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    else if (!strcmp(pszName, DomeHasHighlyRelaibleOpenCloseSensors_Name))
        *ppVal = dynamic_cast<DomeHasHighlyRelaibleOpenCloseSensors*>(this);

    return SB_OK;
}

#pragma mark - UI binding

int X2Dome::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    double dTime;
    bool bCheckSafe;
    bool bCheckMountParked;
    char szIpAddress[255];
    
    if (NULL == ui)
        return ERR_POINTER;

    nErr = ui->loadUserInterface("Dragonfly.ui", deviceType(), m_nPrivateISIndex);
    if (nErr)
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

    if(m_bLinked) {
        dx->setEnabled("IPAddress",false); // can't modify ip address when connected
    }
    else {
        dx->setEnabled("IPAddress",true);
    }
    dx->setText("IPAddress", m_sIpAddress.c_str());
    dx->setPropertyDouble("relayDuration", "value", m_Dragonfly.getRelayPulseTime());
    dx->setChecked("checkSafe", m_Dragonfly.getCheckSafe()==true);
    dx->setChecked("checkMountParked", m_Dragonfly.getCheckMountParked()==true);

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
        dx->propertyDouble("relayDuration", "value", dTime);
        bCheckSafe = (dx->isChecked("checkSafe")!=0);
        bCheckMountParked = (dx->isChecked("checkMountParked")!=0);

        m_Dragonfly.setRelayPulseTime(dTime);
        m_Dragonfly.setCheckSafe(bCheckSafe);
        m_Dragonfly.setCheckMountParked(bCheckMountParked);
        if(!m_bLinked) {
            dx->propertyString("IPAddress", "text", szIpAddress, 255);
            m_sIpAddress.assign(szIpAddress);
            m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_IP, szIpAddress);
        }
        m_pIniUtil->writeDouble(PARENT_KEY, CHILD_KEY_RELAY_PULSE, dTime);
        m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_SAFE, bCheckSafe?1:0);
        m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_MOUNT, bCheckMountParked?1:0);
    }
    return nErr;

}

void X2Dome::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    // int nErr = SB_OK;

    if (!strcmp(pszEvent, "on_timer")) {
        if(m_bLinked) {
        }
    }
}


//
//HardwareInfoInterface
//
#pragma mark - HardwareInfoInterface

void X2Dome::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "Dragonfly RoR controller";
}

void X2Dome::deviceInfoNameLong(BasicStringInterface& str) const
{
    str = "Lunatico Astonomy Dragonfly controller";
}

void X2Dome::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    str = "Lunatico Astonomy Dragonfly controller";
}

 void X2Dome::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
     if(m_bLinked) {
         std::string sFirmware;
         X2MutexLocker ml(GetMutex());
         m_Dragonfly.getFirmwareVersion(sFirmware);
         str = sFirmware.c_str();
     }
     else
         str = "N/A";
}

void X2Dome::deviceInfoModel(BasicStringInterface& str)
{
    str = "Lunatico Astonomy Dragonfly";
}

//
//DriverInfoInterface
//
#pragma mark - DriverInfoInterface

 void	X2Dome::driverInfoDetailedInfo(BasicStringInterface& str) const
{
    str = "Dragonfly RoR controller X2 plugin by Rodolphe Pineau";
}

double	X2Dome::driverInfoVersion(void) const
{
	return PLUGIN_VERSION;
}

//
//DomeDriverInterface
//
#pragma mark - DomeDriverInterface

int X2Dome::dapiGetAzEl(double* pdAz, double* pdEl)
{
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    *pdAz = m_Dragonfly.getCurrentAz();
    *pdEl = m_Dragonfly.getCurrentEl();
    return SB_OK;
}

int X2Dome::dapiGotoAzEl(double dAz, double dEl)
{
    int nErr;

    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.gotoAzimuth(dAz);
    if(nErr)
        return ERR_CMDFAILED;

    else
        return SB_OK;
}

int X2Dome::dapiAbort(void)
{

    X2MutexLocker ml(GetMutex());

    m_Dragonfly.abortMove();
    
	return SB_OK;
}

int X2Dome::dapiOpen(void)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        return ERR_NOLINK;
    }

    nErr =m_Dragonfly.openRoof();
    if(nErr)
        return ERR_CMDFAILED;

	return SB_OK;
}

int X2Dome::dapiClose(void)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        return ERR_NOLINK;
    }

    nErr = m_Dragonfly.closeRoof();
    if(nErr)
        return ERR_CMDFAILED;

	return SB_OK;
}

int X2Dome::dapiPark(void)
{
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

	m_Dragonfly.parkDome();

	return SB_OK;
}

int X2Dome::dapiUnpark(void)
{
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

	m_Dragonfly.unparkDome();
	return SB_OK;
}

int X2Dome::dapiFindHome(void)
{
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

	m_Dragonfly.findHome();
    return SB_OK;
}

int X2Dome::dapiIsGotoComplete(bool* pbComplete)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.isGoToComplete(*pbComplete);
    if(nErr)
        return ERR_CMDFAILED;
    return SB_OK;
}

int X2Dome::dapiIsOpenComplete(bool* pbComplete)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.isOpenComplete(*pbComplete);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    return SB_OK;
}

int	X2Dome::dapiIsCloseComplete(bool* pbComplete)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.isCloseComplete(*pbComplete);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    return SB_OK;
}

int X2Dome::dapiIsParkComplete(bool* pbComplete)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.isParkComplete(*pbComplete);
    if(nErr)
        return ERR_CMDFAILED;

    return SB_OK;
}

int X2Dome::dapiIsUnparkComplete(bool* pbComplete)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.isUnparkComplete(*pbComplete);
    if(nErr)
        return ERR_CMDFAILED;

    return SB_OK;
}

int X2Dome::dapiIsFindHomeComplete(bool* pbComplete)
{
    int nErr;
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.isFindHomeComplete(*pbComplete);
    if(nErr)
        return ERR_CMDFAILED;

    return SB_OK;
}

int X2Dome::dapiSync(double dAz, double dEl)
{
    int nErr;

    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    nErr =m_Dragonfly.syncDome(dAz, dEl);
    if(nErr)
        return ERR_CMDFAILED;
	return SB_OK;
}
