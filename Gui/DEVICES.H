/*********************************************************************

                        devices.h

**********************************************************************/


#ifndef __DEVICES_H__
#define __DEVICES_H__


#include "points.h"



/****************************************************************************************/
/**************************** Usefull NIUSBGPIB classes **********************************/



class CDevice : public CObject, public CDrvIntf
{
    private:
        CIOPointArray   m_arrInput;
        CIOPointArray   m_arrOutput;
        HDriverSym*     m_pDev;

    public:
        UINT16        m_DevType;
        LPCSTR        m_pName;        // this name is stored on the heap in the *m_pDev block
        DEVICE_INST   m_DeviceInst;
        DEVICE_IO     m_Input;
        DEVICE_IO     m_Output;
                        
    private:
        int             DevValid();
        int             LoadDevice();
        int             CheckDeviceType();
        UINT16          InputSizer();
        UINT16          OutputSizer();
        CIOPointArray*  GetPointArray( UINT16 Point, SymAccess Access, UINT16& PointOffset );

        int  ListConfigGet( DEVCONFIG& Devconfig, UINT16& rBytesRet );
        int  ListConfigPut( DEVCONFIG& Devconfig );
        BOOL ListStart();

    public:
        int     Load( UINT32 NameOfs, UINT32 InputOfs, UINT32 OutputOfs );
        int     Check( CDevice* pDevice );
        void    UpdateOffsets( UINT32 CfgOfs, UINT32 InputOfs, UINT32 OutputOfs, UINT32 IODelta );

        // Accessing elements
        UINT16  GetInputSize();         // Device  input size
        UINT16  GetOutputSize();        // Device output size
        UINT16  GetUiotInputSize();     // Device  input size or 0 if no  input tags defined
        UINT16  GetUiotOutputSize();    // Device output size or 0 if no output tags defined

        CDevice( HDriverSym* pDevSym );
        virtual ~CDevice();

        void AssertValid() const { return; }
};


/******************************************************************************/

class CDeviceArray : public CPtrArray, public CDrvIntf
{   
    private:
        LPCSTR m_pNetName;

    public:
        int  Check( CDevice* pDevice );
        void UpdateOffsets( UINT32 CfgOfs, UINT32 InputOfs, UINT32 OutputOfs, UINT32 IODelta );
        
        // Accessing elements
        CDevice*  GetAt(int nIndex) const  {return (CDevice* )CPtrArray::GetAt(nIndex);}
        CDevice*& ElementAt(int nIndex)    {return (CDevice*&)CPtrArray::ElementAt(nIndex);}

        // overloaded operator helpers
        CDevice*  operator[](int nIndex) const {return GetAt(nIndex);}
        CDevice*& operator[](int nIndex)       {return ElementAt(nIndex);}

        CDeviceArray( LPCSTR pNetName );
        virtual ~CDeviceArray();
};



/************************** End of Usefull NIUSBGPIB classes **********************************/
/*********************************************************************************************/


#endif      /* __DEVICES_H__  */


