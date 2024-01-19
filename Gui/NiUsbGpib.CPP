/********************************************************************************************************

                        NiUsbGpib.cpp


    NiUsbGpib.CPP is the core of the dll. 
    It contains several entry points called during compilation time.

    1. StartCompile(). is called once for all driver instances of DRIVER_KEY type at 
       the very beginning of the compilation. 
       It gives the absolute path to the project directory. 
       If needed, it should be copied into an internal buffer. 
   
    2. NetPass1() offers a database entry point (the 'HDriverDb& rDb' parameter) 
       from where all declared devices can be scanned, and in an inner loop all points 
       for each device. 
       This function must return the UIOT space needed by this driver instance for 
       config, input and outputs areas. 
       The config area contains info passed to the runtime. The compiler already assumes 
       DRIVER_INSTHEAD is there, so we need to declare the remaining of the DRIVER_INST struct 
       as well as any lists the runtime might need (device list, IO points lists, 
       device names list, ...).  
       The present example perform more thorough driver/device config checking at the very 
       beginning of NetPass1() when calling Network.Check(). As mentioned above, 
       thorough checks cannot be performed by DevSort(), DevCheck() and DevValid(). 
       Additionally here we have the 'HDriverErr& rDe' parameter which offers strong 
       support for reporting error messages with parameters. 
    
    3. NetPass2() offers additional parameters: UIOT offsets for the 3 requested areas 
       (config, input and output) and the  'rFileHandle.Write()'  necessary to fill the 
       config area with info for the runtime. We must use this function to write as many 
       bytes as the ConfigTotal size we declared at NetPass1(). 
       Additionally NetPass2() must assign a UIOT byte and bit offset for all defined IO points.
       So it scans all defined devices and for every device calls and for each device 
       scans all defined points. 
       Here we have UIOT offsets for the current device IO spaces. 
       We scan all declared points and retrieve the point id. 
       The point id is set for each point in the RCD point lists. 
       It usually is the point's bit offset within the device's input/output space.
       So, when having the point id we can easily obtain the point's offset.
       We convert the 'device byte offset within the UIOT' and the 'point bit offset within the device'
       into the 'point byte offset within the UIOT' and the 'point bit offset within the UIOT byte'.
       Finnally we call:
            pPnt->PutDataOffset( PntOffset);    // store here the UIOT byte offset for our point
            pPnt->PutBit( Point & 7);           // and the bit offset in that byte.
       to assign UIOT byte and bit offset for every IO tag/point. 
       NetPass2() may not be called if NetPass1() reported errors.
    
    4. DevParam() is called for special functions at device level to retrieve the address of the target device.
       This entry point is used to fill in SPECIAL_INST fields declared in the DEV_PARAM_TYPE paragraph 
       with the PT_DEVICE format. 
       The PT_DEVICE format can lso be used with functions at driver level. In this case 
       the function must be configured with an edit field where the user should type in 
       the target device name. Again, the DevParam() function will provide the actual address. 

    5. EndCompile(). is called one for all driver instances of DRIVER_KEY type at 
       the very end of the compilation. It should be used to perform all necessary cleanup. 

**************************************************************************************************************/


#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include <assert.h>

#include "drvrsym.h"
#include "drvrio.h"
#include "resource.h"
#include "driver.h"         // NiUsbGpib driver specifics
#include "goodies.h"        // IsIncluded()
#include "network.h"        // interface to CNetwork and CNetworkArray


#define  DRIVER_IOD
#include "drvrdll.h"
#undef   DRIVER_IOD



/*************************************************************************
            Reservations        
            (Warning: static data is valid only within the same dll call!)
*/


static CNetworkArray* parrNetwork = NULL;




/************************************************************************************
 ************************************************************************************
 
        dll (io3) entry points
*/





/*************************************************************************************/



// Called only once for all drivers of the same type at the beginning of the compilation
extern "C" void PASCAL StartCompile( DRIVER_KEY NetKey, LPCSTR projpath)
{
    OutputDebugString("StartCompile() \n");
    parrNetwork = new CNetworkArray( NetKey, projpath );
    return;
}

 
//Ask driver to return size of the two areas of the UIOT for this instance
extern "C" int FAR PASCAL NetPass1( 
        HDriverDb FAR & rDb,
        DRIVER_KEY      /* NetKey */,   //Allows multiple drivers to be in the same DLL
        LPCSTR          lpNetName   ,
        UINT32 FAR &    rConfigTotal,   //Size including fixed header of network instance data
        UINT32 FAR &    rInputTotal ,   //Size of network user variable data
        UINT32 FAR &    rOutputTotal,   //Size of network user variable data
        HDriverErr FAR& rDe)
{

    int rc = SUCCESS;

    if( parrNetwork != NULL )
    {
        int NetNo = parrNetwork->GetSize();

        if( NetNo < MAX_DRV_INSTANCES )
        {
            CNetwork* pNetwork = new CNetwork( lpNetName );

            if( pNetwork != NULL )
            {
                pNetwork->LoadIntfPointers( &rDb, &rDe );

                rc = pNetwork->Load();

                if( rc == SUCCESS )
                    rc = parrNetwork->Check( pNetwork );

                if( rc == SUCCESS )
                {
                    parrNetwork->Add( pNetwork );
                    pNetwork->ReportSizes( rConfigTotal, rInputTotal, rOutputTotal );
                }
                else
                    delete  pNetwork;

            }
            else
            {
                rc = FAILURE;
                Erop( &rDe, IDS_CP_NO_MEMORY, "", "", "", "");
            }
        }
        else
        {
            rc = FAILURE;
            Erop( &rDe, IDS_CP_TOO_MANY_DRIVERS, "", "", "", NetNo);
        }
    }
    else
    {
        rc = FAILURE;
        Erop( &rDe, IDS_CP_NO_MEMORY, "", "", "", "");
    }
    
    return rc;
}





// Tell driver where this instance configuration data is in the UIOT
// so it can write it to the open file handle provided
extern "C" int FAR PASCAL NetPass2Cfg(
        HDriverDb FAR &   rDb,
        DRIVER_KEY        /*NetKey*/,   //Allows multiple drivers to be in the same DLL
        LPCSTR            lpNetName,
        HStreamAbs &      rFileHandle,
        UINT32            CfgOfs,       //Start offset including fixed header of network instance data
        UINT32            /*CfgEnd*/,
        UINT32            InputOfs,     //Start offset of network user input variable data
        UINT32            OutputOfs,    //Start offset of network user outputvariable data
        UINT32            IODelta,      //Number of bytes between differnet sections of IO table
        HDriverErr FAR &  rDe)
{

    int rc = SUCCESS;   // LoadStaticTables( rDb, lpNetName, &rDe);
    

    CNetwork* pNetwork = parrNetwork->GetNetwork( lpNetName );

    if( pNetwork != NULL )
    {
        pNetwork->LoadIntfPointers( &rDb, &rDe );       // we may get relocated pointers in NetPass2
        pNetwork->UpdateOffsets( CfgOfs, InputOfs, OutputOfs, IODelta );
        pNetwork->WriteConfigData( rFileHandle );
    }
    else
    {
        rc = FAILURE;
        Erop( &rDe, IDS_CP_CANNOT_FIND_NETWORK, lpNetName, "", "", "");
    }

    return rc;
}




    


/********************************************************************************************
 *  For the given network device, return the device magic identifier.
 *  Used for special function attached to devices.
 *  The runtime needs to know which is the device the special function is called for.
 *  In our example we return the device address which is a good device identifier.
 */
extern "C" int FAR PASCAL DevParam(  //Non-zero on error/problem
        HDriverDb FAR & rDb,
        DRIVER_KEY      /* NetKey */,
        LPCSTR          lpDevName,
        DEVPARAM &      rDp)
{
    UINT16    Size;
    DEVCONFIG Device;

    memset(&Device, 0, sizeof(Device));
    rDb.ConfigGet( lpDevName, &Device, sizeof(Device), Size, TRUE);

    rDp = (DEVPARAM)Device.Address;
        
    return SUCCESS;
}




// Called only once for all drivers of the same type when the compilation ends
extern "C" void PASCAL EndCompile( DRIVER_KEY NetKey )
{
    NetKey = NetKey;
    OutputDebugString("EndCompile() \n");

    if( parrNetwork != NULL)
    {
        delete parrNetwork;
        parrNetwork = NULL;
    }

    return;
}





/////////////////////////////////////////////////////////////////////////


