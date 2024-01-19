/*********************************************************************

                        goodies.h

**********************************************************************/


#ifndef __GOODIES_H__
#define __GOODIES_H__


#include "mfcstuff.h"


/************************************************************************************
        Usefull functions
*/


void Erop( HDriverErr* pDe, const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3,    LPCSTR arg4    );
void Erop( HDriverErr* pDe, const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3,    const int arg4 );
void Erop( HDriverErr* pDe, const int ids, LPCSTR arg1, LPCSTR arg2, const int arg3, const int arg4 );



/**********************************************************
    See if i is included in the min...max range
*/
BOOL   IsBetween(long i, long min, long max);

/**********************************************************
    See if (Start1,Size1) overlaps (Start2,Size2)
*/
BOOL   IsOverlapped(UINT32 Start1, UINT32 Size1, UINT32 Start2, UINT32 Size2);

/**********************************************************
    See if (Start1,Size1) is included or matches (Start2,Size2)
*/
int IsIncluded(UINT32 Start1, UINT32 Size1, UINT32 Start2, UINT32 Size2);


/**********************************************************
    ASCII to binary conversion
*/
int ConvertAtoI( LPCSTR a, UINT32& i );



class CDrvIntf
{
    private:
        static  HDriverDb*  m_pDb;
        static  HDriverErr* m_pDe;

    public:
        void LoadIntfPointers( HDriverDb* pDb1, HDriverErr* pDe1 );

	protected:
        CDrvIntf() {}
        void Erop( const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3,    LPCSTR arg4    );
        void Erop( const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3,    const int arg4 );
        void Erop( const int ids, LPCSTR arg1, LPCSTR arg2, const int arg3, const int arg4 );

        int  ListNetworkConfigGet(LPCSTR lpName, void* lpData,	UINT16 MaxSize, UINT16& rBytesRet );
        int  ListDeviceConfigGet( LPCSTR lpName, void* lpData,	UINT16 MaxSize, UINT16& rBytesRet );
        int  ListNetworkConfigPut(LPCSTR lpName, void* lpData,	UINT16 MaxSize );
        int  ListDeviceConfigPut( LPCSTR lpName, void* lpData,	UINT16 MaxSize );
        BOOL ListNetworkStart( LPCSTR lpName );
        BOOL ListDeviceStart(  LPCSTR lpName );
        HDriverSym* ListGet();
        int  ListUpdate( const HDriverSym *pData);	//where to put the pointer to the interface
        BOOL ListNext();    //Returns 0 if no more
        void ListEnd();
};



#endif      /* __GOODIES_H__  */


