#include <stdio.h>
#include <string.h>

#include "../../licensedinterfaces/domedriverinterface.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/domehasreliableopenclosesensors.h"

#include "Dragonfly.h"


class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;


#define PARENT_KEY			"Dragonfly"
#define CHILD_KEY_RELAY_PULSE   "RelayPulseTime"
#define CHILD_KEY_IP    "IpAddress"
#define CHILD_KEY_SAFE    "checkSafeInput3"

#if defined(SB_WIN_BUILD)
#define DEF_PORT_NAME					"COM1"
#elif defined(SB_MAC_BUILD)
#define DEF_PORT_NAME					"/dev/cu.KeySerial1"
#elif defined(SB_LINUX_BUILD)
#define DEF_PORT_NAME					"/dev/COM0"
#endif

#define LOG_BUFFER_SIZE 256
/*!
\brief The X2Dome example.

\ingroup Example

Use this example to write an X2Dome driver.
*/
class X2Dome: public DomeDriverInterface, public ModalSettingsDialogInterface, public DomeHasHighlyRelaibleOpenCloseSensors
{
public:

	/*!Standard X2 constructor*/
	X2Dome(	const char* pszSelectionString,
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface* pTheSkyXForMounts,
					SleeperInterface*				pSleeper,
					BasicIniUtilInterface*			pIniUtil,
					MutexInterface*					pIOMutex,
					TickCountInterface*				pTickCount);
	virtual ~X2Dome();

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{
	virtual DeviceType							deviceType(void) {return DriverRootInterface::DT_DOME;}
	virtual int									queryAbstraction(const char* pszName, void** ppVal);
	//@}

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{
	virtual int									establishLink(void)						;
	virtual int									terminateLink(void)						;
	virtual bool								isLinked(void) const					;
	//@}

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{
	virtual void deviceInfoNameShort(BasicStringInterface& str) const					;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const					;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const		;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)					;
	virtual void deviceInfoModel(BasicStringInterface& str)							;
	//@}

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const	;
	virtual double								driverInfoVersion(void) const								;
	//@}

    virtual int        initModalSettingsDialog(void){return 0;}
    virtual int        execModalSettingsDialog(void);
    virtual void    uiEvent(X2GUIExchangeInterface*, const char*);


	//DomeDriverInterface
	virtual int dapiGetAzEl(double* pdAz, double* pdEl);
	virtual int dapiGotoAzEl(double dAz, double dEl);
	virtual int dapiAbort(void);
	virtual int dapiOpen(void);
	virtual int dapiClose(void);
	virtual int dapiPark(void);
	virtual int dapiUnpark(void);
	virtual int dapiFindHome(void);
	virtual int dapiIsGotoComplete(bool* pbComplete);
	virtual int dapiIsOpenComplete(bool* pbComplete);
	virtual int	dapiIsCloseComplete(bool* pbComplete);
	virtual int dapiIsParkComplete(bool* pbComplete);
	virtual int dapiIsUnparkComplete(bool* pbComplete);
	virtual int dapiIsFindHomeComplete(bool* pbComplete);
	virtual int dapiSync(double dAz, double dEl);

    virtual SerXInterface::Parity	parity() const				{return SerXInterface::B_NOPARITY;}
    virtual void					setParity(const SerXInterface::Parity& parity){};
    virtual bool					isParityFixed() const		{return true;}


private:

	SerXInterface 									*	GetSerX() {return m_pSerX; }
	TheSkyXFacadeForDriversInterface				*	GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface								*	GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface							*	GetSimpleIniUtil() {return m_pIniUtil; }
	MutexInterface									*	GetMutex()  {return m_pIOMutex;}
	TickCountInterface								*	GetTickCountInterface() {return m_pTickCount;}

	SerXInterface									*	m_pSerX;
	TheSkyXFacadeForDriversInterface				*	m_pTheSkyXForMounts;
	SleeperInterface								*	m_pSleeper;
	BasicIniUtilInterface							*	m_pIniUtil;
	MutexInterface									*	m_pIOMutex;
	TickCountInterface								*	m_pTickCount;

    void portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const;


	int         m_nPrivateISIndex;
	bool        m_bLinked;
    CDragonfly       m_Dragonfly;
    char        mLogBuffer[LOG_BUFFER_SIZE];

    std::string     m_sIpAddress;
    
};
