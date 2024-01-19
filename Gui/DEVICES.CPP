/*********************************************************************

                        devices.cpp

**********************************************************************/


#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include <ctype.h>

#include "drvrsym.h"
#include "resource.h"       // IDS_ERR_...
#include "driver.h"         // NiUsbGpib driver specifics
#include "goodies.h"        // interface to goodies.cpp
#include "devices.h"        // interface to devices.cpp



/*********************************************************************************************/
/****************************** Usefull NiUsbGpib classes *************************************/



/*********************************************************************************************/
/******************************* CDevice Implementation **************************************/

CDevice::CDevice( HDriverSym* pDevSym )
    : m_arrInput( pDevSym->GetName() ), m_arrOutput( pDevSym->GetName() )
{
    m_pDev  = pDevSym;
    m_pName = pDevSym->GetName();
    m_DevType = (UINT16)m_pDev->GetId();
    memset( &m_DeviceInst, 0, sizeof(DEVICE_INST) );
    memset( &m_Input,      0, sizeof(DEVICE_IO)   );
    memset( &m_Output,     0, sizeof(DEVICE_IO)   );

    DebugString( "creating device %s\n", m_pName );
}

CDevice::~CDevice()
{
    DebugString( "removing device  %s\n", m_pName );
    delete m_pDev; 
}


CIOPointArray* CDevice::GetPointArray( UINT16 Point, SymAccess Access, UINT16& PointOffset )
{
    // Point        is the point id as defined in the RCD file. 
    // PointOffset  is the point offset (in bits) in the device space. 
    // PointOffset  is calculated using Point:  PointOffset = function( Point ); 
    // In our simple case: PointOffset = Point;
    
    CIOPointArray* pPointArray = NULL; 

    if( Access == saInput)
    {
        PointOffset = Point;
        pPointArray = &m_arrInput;
    }

    if( Access == saOutput)
    {
        PointOffset = Point;
        pPointArray = &m_arrOutput;
    }

    return  pPointArray;
}



// makes sure  m_DeviceInst  is can safely be used at run time
int CDevice::DevValid( )
{
    int rc = SUCCESS;
    return rc;
}



// test if the device is configured and load config info into DEVICE_INST
int CDevice::LoadDevice()
{
    int rc = SUCCESS;

    DEVCONFIG   DeviceCfg;  // where config data is loaded from symdb
    UINT16      Size;       // the actual size of the config block as recored in symdb

    memset( &DeviceCfg, 0, sizeof(DEVCONFIG));
    rc = ListConfigGet( DeviceCfg, Size );

    if( rc == SUCCESS )
    {
        if( Size                      == sizeof(DEVCONFIG)          && 
            DeviceCfg.DriverId        == NETCONFIG_ID               &&  
            DeviceCfg.DeviceId        == DEVCONFIG_ID               &&  
            DeviceCfg.DeviceVersMajor == ( DEVCONFIG_VERS >> 16 )   &&  
            DeviceCfg.DeviceVersMinor >= ( DEVCONFIG_VERS & 0xffff )
          )
        {

            m_DeviceInst.Address    = DeviceCfg.Address;
            m_DeviceInst.bCritical  = (UINT8)DeviceCfg.bCritical;
            m_DeviceInst.Type       = m_DevType;
			//m_DeviceInst.Input		= 

            {
                UINT32 l;
                int rc1 = ConvertAtoI( (LPCSTR)DeviceCfg.ProductCode, l );

                if( rc1 == SUCCESS && l <= 0xffff )
                    m_DeviceInst.ProductCode = (UINT16)l;
                else
                {
                    rc = IDS_CP_DEVCONFIG_INVALID_PCODE;
                    Erop( rc, m_pName, (LPCSTR)DeviceCfg.ProductCode, 0, 0);
                }
            }

			//m_DeviceInst.Input.Size = InputSizer();               // let runtime know how large is the  input image of this device
			//m_DeviceInst.Output.Size = OutputSizer();              // let runtime know how large is the output image of this device


            m_Input.Size  = InputSizer();               // let runtime know how large is the  input image of this device
            m_Output.Size = OutputSizer();              // let runtime know how large is the output image of this device

            m_arrInput.m_UiotSize  = m_Input.Size;      // let the CIOPointArray know how large is
            m_arrOutput.m_UiotSize = m_Output.Size;     // the area where points can be defined
        }
        else
        {
            rc = IDS_CP_DEVICE_NOT_CONFIGURED;
            Erop( rc, m_pName, "", "", "" );
        }    
    }
    else
    {
        rc = IDS_CP_CANNOT_GET_DEVCONFIG;
        Erop( rc, m_pName, "", "", "" );
    }


    return rc;
}




int CDevice::Load( UINT32 NameOfs, UINT32 InputOfs, UINT32 OutputOfs )
{
    int rc = CheckDeviceType( );

    if( rc == SUCCESS )
        rc = LoadDevice();

    if(rc == SUCCESS)
        rc = DevValid();

    if( rc == SUCCESS )
    {
        m_DeviceInst.ofsName = NameOfs;

        if( ListStart())    /* scan all points */
        {
            do
            {
                int rc1 = SUCCESS;
                
                HDriverSym* pPnt     = ListGet();
                SymAccess   Access   = pPnt->GetAccess();     // saInput, saOutput
                UINT16      Point    = pPnt->GetPoint();      // point offset in bits   (0xffff for strobe out)
                LPCSTR      pPntName = pPnt->GetName();       // user's name for that I/O tag
        
                UINT16         PointOffset = 0;
                CIOPointArray* pPointArray = GetPointArray( Point, Access, PointOffset );

                if( pPointArray != NULL )
                {
                    CIOPoint* pNewPoint = new CIOPoint( pPnt, PointOffset );

                    rc1 = pPointArray->Check( pNewPoint );

                    if( rc1 == SUCCESS )
                        pPointArray->Add( pNewPoint );
                    else
                        delete pNewPoint;               // also deletes *pPnt 
                }
                else
                {
                    rc1 = IDS_CP_INVALID_POINT;
                    Erop( rc1, pPntName, m_pName, "", "");
                    delete pPnt;
                }

                if( rc1 != SUCCESS )
                    rc = FAILURE;

            } while( ListNext());
        }
        ListEnd();

        if( m_arrInput.GetSize() )                  // if there are any input points defined here
        {
            m_Input.ofsUiot = InputOfs;             //      set a UIOT offset for them
            m_Input.Address = m_DeviceInst.Address; //      runtime needs it to generate pSrc
            m_Input.bUsed   = 1;
        }
        else                                        // else
            m_arrInput.m_UiotSize =  0;             //      do not reserve UIOT space

        if( m_arrOutput.GetSize() )                 // if there are any output points defined here
        {
            m_Output.ofsUiot = OutputOfs;           //      set a UIOT offset for them
            m_Output.Address = m_DeviceInst.Address;//      runtime needs it to generate pDst
            m_Output.bUsed   = 1;
        }
        else                                        // else
            m_arrOutput.m_UiotSize =  0;            //      do not reserve UIOT space
    
    }

    DebugString( "CDevice::Load(). Name=%s, rc=%d\n", m_pName, rc );
    
    return rc;
}





int CDevice::CheckDeviceType()
{
    int rc = SUCCESS;

    switch( m_DevType )
    {
      //  case DEVICE_4W_INPUT:
      //  case DEVICE_4W_OUTPUT:
      //  case DEVICE_4W_IORO:
      //  case DEVICE_4W_IANDO:
		case DEVICE_GPIB_STAT:
                    break;

        default: 
                    rc = IDS_CP_UNKNOWN_DEVICE;
                    Erop( rc, m_pName, "", "", m_DevType );

    }

    return rc;
}

UINT16 CDevice::InputSizer()
{
    UINT16 Size = 0;

    switch( m_DevType )
    {
//        case DEVICE_4W_INPUT:
//        case DEVICE_4W_IORO:
//        case DEVICE_4W_IANDO:
	case DEVICE_GPIB_STAT:
                    Size = sizeof(UINT16)*2;

    }
    return Size;
}

UINT16 CDevice::OutputSizer()
{
    UINT16 Size = 0;

  //  switch( m_DevType )
  //  {
  //      case DEVICE_4W_OUTPUT:
  //      case DEVICE_4W_IORO:
  //      case DEVICE_4W_IANDO:
  //                  Size = sizeof(UINT16)*4;

   // }
    return Size;
}


// if there are input tags defined --> returns the device input size
// else --> returns 0
UINT16 CDevice::GetUiotInputSize()  
{
    return m_arrInput.m_UiotSize;
}


// if there are output tags defined --> returns the device output size
// else --> returns 0
UINT16 CDevice::GetUiotOutputSize()
{
    return m_arrOutput.m_UiotSize;
}


UINT16 CDevice::GetInputSize()         // Device  input size
{
    return m_Input.Size;
}


UINT16 CDevice::GetOutputSize()        // Device output size
{
    return m_Output.Size;
}


int CDevice::Check( CDevice* pDevice )      // makes sure "this" can live with pDevice
{
    int rc = SUCCESS;

    if( m_DeviceInst.Address == pDevice->m_DeviceInst.Address )
    {
        rc = IDS_CP_SAME_DEVICE_ADDRESS;
    }

    return rc; 
}


// called in NetPass2 when we have real UIOT offsets for Config, Input and Output areas
void CDevice::UpdateOffsets( UINT32 CfgOfs, UINT32 InputOfs, UINT32 OutputOfs, UINT32 IODelta )
{
    m_DeviceInst.ofsName += CfgOfs;

    {
        UINT32 PrgInputOfs = 0;

        if( m_arrInput.m_UiotSize )     // if there are any input tags defined here
        {
            PrgInputOfs = m_Input.ofsUiot + InputOfs;
            m_arrInput.UpdateOffsets( PrgInputOfs );
            m_Input.ofsUiot = PrgInputOfs + IODelta;
        }

        m_pDev->PutInputOffset(  PrgInputOfs  );
    }

    {
        UINT32 PrgOutputOfs = 0;

        if( m_arrOutput.m_UiotSize )    // if there are any output tags defined here
        {
            PrgOutputOfs = m_Output.ofsUiot + OutputOfs;
            m_arrOutput.UpdateOffsets( PrgOutputOfs );
            m_Output.ofsUiot = PrgOutputOfs + IODelta;
        }

        m_pDev->PutOutputOffset( PrgOutputOfs );
    }

    ListUpdate( m_pDev );
}



int  CDevice::ListConfigGet( DEVCONFIG& Devconfig, UINT16& rBytesRet )
{
    return ListDeviceConfigGet( m_pName, &Devconfig, sizeof(DEVCONFIG), rBytesRet );
}

int  CDevice::ListConfigPut( DEVCONFIG& Devconfig )
{
    return ListDeviceConfigPut( m_pName, &Devconfig, sizeof(DEVCONFIG) );
}

BOOL CDevice::ListStart()
{
    return ListDeviceStart( m_pName );
}



/*************************************************************************/

CDeviceArray::CDeviceArray( LPCSTR pNetworkName ) 
{
    SetSize( 0, MAX_DEVICES );
    m_pNetName = pNetworkName;
    DebugString( "creating CDeviceArray for %s\n", m_pNetName );
}

CDeviceArray::~CDeviceArray()  
{ 
    DebugString( "removing CDeviceArray\n", "" );
    for(int i = 0; i < GetSize(); i++ )
        delete GetAt( i );
    RemoveAll(); 
}


// makes sure  pDevice can live with all previousely defined devices
int CDeviceArray::Check( CDevice* pDevice )
{
    int    rc    = SUCCESS;

    for( int DevIndex = 0; DevIndex < GetSize() && (rc == SUCCESS) ; DevIndex++ )
    {
        CDevice* pDev = GetAt( DevIndex );
        int      rc1  = pDevice->Check( pDev );

        if( rc1 != SUCCESS )
        {
            Erop( rc1, pDevice->m_pName, pDev->m_pName, m_pNetName, pDevice->m_DeviceInst.Address );
            rc = FAILURE;
        }
    }

    return rc; 
}


// called in NetPass2 when we have real UIOT offsets for Config, Input and Output areas
void CDeviceArray::UpdateOffsets( UINT32 CfgOfs, UINT32 InputOfs, UINT32 OutputOfs, UINT32 IODelta )
{
    for( int DevIndex = 0; DevIndex < GetSize() ; DevIndex++ )
    {
        CDevice* pDevice = GetAt( DevIndex );
        pDevice->UpdateOffsets( CfgOfs, InputOfs, OutputOfs, IODelta );
    }
}


/***************************** End of CDevice Implementation *********************************/
/*********************************************************************************************/




/************************** End of Usefull NiUsbGpib classes **********************************/
/*********************************************************************************************/


