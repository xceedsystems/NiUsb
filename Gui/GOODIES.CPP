/*********************************************************************

                        goodies.cpp

**********************************************************************/


#include "stdafx.h"


#include <windows.h>
#include <stdio.h>          // sprintf()

#include "drvrsym.h"
#include "driver.h"         // NiUsbGpib driver specifics
#include "goodies.h"        // interface to goodies.cpp



/************************************************************************************
        Usefull functions
*/




BOOL IsBetween( long i, long min, long max)
{
    return i >= min && i <= max;
}





/**********************************************************
    Both units should have size !!!
    If they overlap --> return 1  else return 0;
*/
int IsOverlapped(UINT32 Start1, UINT32 Size1, UINT32 Start2, UINT32 Size2)
{
    if(Size1 && Size2)        // There is no overlapping without sizes!
    {
        if(Start2 > Start1)
        {
            if(Start2 - Start1 < Size1)  return 1;
        }
        else
        {
            if(Start1 - Start2 < Size2)  return 1;
        }
    }
    return 0;
}


/**********************************************************
    Both units should have size !!!
    If 1 matches  2    return 3
    If 1 includes 2,   return 1
    If 2 includes 1,   return -1
    else           return 0;
*/
static int InclusionTest(UINT32 Start1, UINT32 Size1, UINT32 Start2, UINT32 Size2)
{
    if(!IsOverlapped(Start1, Size1, Start2, Size2)) 
        return 0;                       // There is no inclusion without overlapping!

    if(Start1 == Start2 && Size1 == Size2)
        return 3;                       // Units completely match

    if(Start2 > Start1)
    {
        if(Start2 + Size2 <= Start1 + Size1)  return 1;
    }
    else
    {
        if(Start1 + Size1 <= Start2 + Size2)  return -1;
    }

    return 0;
}


/**********************************************************
    See if 1 is included or matches 2
*/
int IsIncluded(UINT32 Start1, UINT32 Size1, UINT32 Start2, UINT32 Size2)
{
    int code = InclusionTest(Start1, Size1, Start2, Size2);
    return code == -1 || code == 3;
}




/*************************************************************/

// a version of strtoul that understands -16#ff_ff, not 0xffff
static unsigned long SSIstrtoul(const char *nptr, char **endptr, int inBase, int* outBase)
{
    // skip white space
    while (isspace(*nptr)) {
        ++nptr;
    }
    
    // check sign
    BOOL minus = FALSE;
    if (*nptr == '-') {
        minus = TRUE;
        ++nptr;
    } else if (*nptr == '+') {
        ++nptr;
    }

    // if no base, assume base 10
    unsigned long radix = inBase;
    char *internalEndptr;
    if (!endptr) {
        endptr = &internalEndptr;
    }
    if (!radix) {
        for (radix = 0 ; *nptr ; ++nptr) {
            if (isdigit(*nptr)) {
                int digit = *nptr - '0';
                // if digit fits in ulong, add it, else quit
                if ((ULONG_MAX - digit)/10 >= radix) {
                    radix = radix*10 + digit;
                } else {
                    radix = ULONG_MAX;
                    break;
                }
            } else {
                break;
            }
        }
        // using nptr to be lazy, but need to cast it for return
        *endptr = (char*)nptr;
/* do we want to support 16  # 8f?
        while (isspace(*nptr)) {
            ++nptr;
        }
*/
        // if end of str != #, then base 10 correct
        // done if no more string, or radix > 36
        if (*nptr != '#' || !*(nptr + 1) || radix > 36) {
            if (outBase) {
                *outBase = 10;
            }
            //extra casting to remove warning about unary minus on unsigned variable
            return (minus ? (unsigned long) -((long) radix) : radix);
        } else {    // base precedes #
            ++nptr;
        }
    }
    if (outBase) {
        *outBase = int(radix);
    }
    // step through string constructing value
	unsigned long retVal = 0;
    for ( retVal = 0 ; *nptr ; ++nptr) {
        if (isalnum(*nptr)) {
            int digit = isdigit(*nptr) ?
                        (*nptr - '0') : (tolower(*nptr) - 'a' + 10);
            if (digit >= int(radix)) {  // if digit not valid for base, quit
                break;
            } else {    // valid digit
                // if digit fits in ulong, add it, else quit
                if ((ULONG_MAX - digit)/radix >= retVal) {
                    retVal = retVal*radix + digit;
                } else {
                    retVal = ULONG_MAX;
                    break;
                }
            }
        } else if (*nptr != '_') {      // quit if not _
            break;
        }
    }
    // using nptr to be lazy, but need to cast it for return
    *endptr = (char*)nptr;
    //extra casting to remove warning about unary minus on unsigned variable
    return (minus ? (unsigned long) -((long) retVal) : retVal);
}



int ConvertAtoI( LPCSTR a, UINT32& i )
{
    int  rc = SUCCESS;
    char *endptr;

    unsigned long l = SSIstrtoul( a, &endptr, 0, NULL );

    while (isspace(*endptr))
        endptr++;

    if(*endptr)
        rc = FAILURE;
    else
        i = (int)l;

    return rc;
}

/*************************************************************/



#ifdef WIN32
// win32 compiler doesn't provide hmodule automatically
extern "C" {
    HMODULE _hModule;
}
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD , LPVOID)
{
    _hModule = hInstance;
    return 1;       // ok
}
#endif  // WIN32



/************************************************************************************
 
        Usefull functions
*/

void Erop(HDriverErr* pDe, const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3, LPCSTR arg4)
{
    if(pDe != NULL)
    {
        LPCSTR args[4];
        args[0] = arg1;
        args[1] = arg2;
        args[2] = arg3;
        args[3] = arg4;
        pDe->ErrorDialog( _hModule, ids, sizeof(args)/sizeof(LPCSTR), args);
    }
}


void Erop(HDriverErr* pDe, const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3, const int arg4)
{
    char i4[32];
    sprintf(i4, "%d", arg4);
    Erop(pDe, ids, arg1, arg2, arg3, i4);
}

void Erop(HDriverErr* pDe, const int ids, LPCSTR arg1, LPCSTR arg2, const int arg3, const int arg4)
{
    char i3[32];
    sprintf(i3, "%d", arg3);
    Erop(pDe, ids, arg1, arg2, i3, arg4);
}




/*********************************************************************************************/
/****************************** CDrvIntf Implementation **************************************/


HDriverDb*  CDrvIntf::m_pDb = NULL;
HDriverErr* CDrvIntf::m_pDe = NULL;


void CDrvIntf::LoadIntfPointers( HDriverDb* pDb, HDriverErr* pDe )
{
    m_pDb = pDb;
    m_pDe = pDe;
}

void CDrvIntf::Erop( const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3, LPCSTR arg4 )
{
    ::Erop( m_pDe, ids, arg1, arg2, arg3, arg4 );
}

void CDrvIntf::Erop( const int ids, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3, const int arg4 )
{
    ::Erop( m_pDe, ids, arg1, arg2, arg3, arg4 );
}

void CDrvIntf::Erop( const int ids, LPCSTR arg1, LPCSTR arg2, const int arg3, const int arg4 )
{
    ::Erop( m_pDe, ids, arg1, arg2, arg3, arg4 );
}



int  CDrvIntf::ListNetworkConfigGet(LPCSTR lpName, void* lpData, UINT16 MaxSize, UINT16& rBytesRet )
{
    return m_pDb->ConfigGet( lpName, lpData, MaxSize, rBytesRet, FALSE );
}

int  CDrvIntf::ListDeviceConfigGet( LPCSTR lpName, void* lpData, UINT16 MaxSize, UINT16& rBytesRet )
{
    return m_pDb->ConfigGet( lpName, lpData, MaxSize, rBytesRet, TRUE );
}

int  CDrvIntf::ListNetworkConfigPut(LPCSTR lpName, void*lpData, UINT16 MaxSize )
{
    return m_pDb->ConfigPut( lpName, lpData, MaxSize, FALSE );
}

int  CDrvIntf::ListDeviceConfigPut( LPCSTR lpName, void*lpData, UINT16 MaxSize )
{
    return m_pDb->ConfigPut( lpName, lpData, MaxSize, TRUE );
}

BOOL CDrvIntf::ListNetworkStart( LPCSTR lpName )
{
    return m_pDb->ListStart( lpName, FALSE );
}

BOOL CDrvIntf::ListDeviceStart(  LPCSTR lpName )
{
    return m_pDb->ListStart( lpName, TRUE );
}

HDriverSym* CDrvIntf::ListGet()
{
    return m_pDb->ListGet();
}

int  CDrvIntf::ListUpdate( const HDriverSym *pData) 	//where to put the pointer to the interface
{
    return m_pDb->ListUpdate( pData );
}

BOOL CDrvIntf::ListNext()    //Returns 0 if no more
{
    return m_pDb->ListNext();
}

void CDrvIntf::ListEnd()
{
    m_pDb->ListEnd();
}



/**************************** End of CDrvIntf Implementation *********************************/
/*********************************************************************************************/


