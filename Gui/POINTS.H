/*********************************************************************

                        points.h

**********************************************************************/



#ifndef __POINTS_H__
#define __POINTS_H__



#include "goodies.h"



/****************************************************************************************/
/**************************** Usefull NIUSBGPIB classes **********************************/



class CIOPoint : public CObject, public CDrvIntf
{
    private:
        HDriverSym* m_pPnt;

    public:
        LPCSTR  m_pName;          // I/O tag name
        UINT16  m_Size;           // Point size in bits
        UINT16  m_Offset;         // Point bit offset in the device's space

    private:
        UINT16  GetPointSize( SymType St );

    public:
        int  Check( CIOPoint* pPoint );
        void UpdateOffsets( UINT32 UiotOffset );
        
        // Accessing elements

        // Constructor
        CIOPoint( HDriverSym* pPntSym, UINT16 PointOffset );
        virtual ~CIOPoint();

        void AssertValid() const { return; }
};


class CIOPointArray : public CPtrArray, public CDrvIntf
{   
    private:
        LPCSTR  m_pDevName;       // device name, used only in error messages

    public:
        UINT16  m_UiotSize;       /* UIOT space reserved for this I/O image */

    public:
        int  Check( CIOPoint* pPoint );
        void UpdateOffsets( UINT32 UiotOffset );
        
        // Accessing elements
        CIOPoint*  GetAt(int nIndex) const  {return (CIOPoint* )CPtrArray::GetAt(nIndex);}
        CIOPoint*& ElementAt(int nIndex)    {return (CIOPoint*&)CPtrArray::ElementAt(nIndex);}

        // overloaded operator helpers
        CIOPoint*  operator[](int nIndex) const {return GetAt(nIndex);}
        CIOPoint*& operator[](int nIndex)       {return ElementAt(nIndex);}

        CIOPointArray( LPCSTR pDevName );
        virtual ~CIOPointArray();
};




/************************** End of Usefull NIUSBGPIB classes **********************************/
/*********************************************************************************************/


#endif      /* __POINTS_H__  */


