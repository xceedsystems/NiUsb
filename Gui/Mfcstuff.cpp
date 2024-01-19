/*********************************************************************

                        MfcStuff.cpp

**********************************************************************/


#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>          // snprintf()
#include <string.h>         // memcpy()

#include "mfcstuff.h"


/****************************************************************************************/
/************************** Usefull MFC classes *****************************************/



/****************************************************************************************/
/**************************** CObject Implementation ************************************/


#ifdef _DEBUG
    void CObject::DebugString( LPCSTR format, LPCSTR text, int i )
    {
        static char msg[1024];
        _snprintf( msg, sizeof(msg), format, text, i );
        OutputDebugString( msg );
    }
#else
    void CObject::DebugString( LPCSTR, LPCSTR, int) {}
#endif

void CObject::DebugString( LPCSTR format, LPCSTR text )
{
    DebugString( format, text, 0 );
}



/************************** Enf of CObject Implementation *******************************/
/****************************************************************************************/


/****************************************************************************************/
/************************** CPtrArray Implementation ************************************/



//////////////////////////////////////////////////////////////////////////////


#define SIZE_T_MAX  10000


CPtrArray::CPtrArray()
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CPtrArray::~CPtrArray()
{
    //ASSERT_VALID(this);

    delete [] (BYTE*)m_pData;
}

void CPtrArray::SetSize(int nNewSize, int nGrowBy /* = -1 */)
{
    //ASSERT_VALID(this);
    ASSERT(nNewSize >= 0);

    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing
        delete [] (BYTE*)m_pData;
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
        #ifdef SIZE_T_MAX
                ASSERT((long)nNewSize * sizeof(void*) <= SIZE_T_MAX);  // no overflow
        #endif
        m_pData = (void**) new BYTE[nNewSize * sizeof(void*)];

        memset(m_pData, 0, nNewSize * sizeof(void*));  // zero fill

        m_nSize = m_nMaxSize = nNewSize;
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));

        }

        m_nSize = nNewSize;
    }
    else
    {
        // Otherwise grow array
        int nNewMax;
        if (nNewSize < m_nMaxSize + m_nGrowBy)
            nNewMax = m_nMaxSize + m_nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        #ifdef SIZE_T_MAX
                ASSERT((long)nNewMax * sizeof(void*) <= SIZE_T_MAX);  // no overflow
        #endif
        void** pNewData = (void**) new BYTE[nNewMax * sizeof(void*)];

        // copy new data from old
        memcpy(pNewData, m_pData, m_nSize * sizeof(void*));

        // construct remaining elements
        ASSERT(nNewSize > m_nSize);

        memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));


        // get rid of old stuff (note: no destructors called)
        delete [] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

void CPtrArray::FreeExtra()
{
    //ASSERT_VALID(this);

    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size
        #ifdef SIZE_T_MAX
                ASSERT((long)m_nSize * sizeof(void*) <= SIZE_T_MAX);  // no overflow
        #endif
        void** pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (void**) new BYTE[m_nSize * sizeof(void*)];
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
        }

        // get rid of old stuff (note: no destructors called)
        delete [] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}


int   CPtrArray::GetSize() const          { return m_nSize; }
int   CPtrArray::GetUpperBound() const    { return m_nSize-1; }
void  CPtrArray::RemoveAll()              { SetSize(0); }

void* CPtrArray::GetAt(int nIndex) const  
{ 
    ASSERT(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex]; 
}

void CPtrArray::SetAt(int nIndex, void* newElement)  
{ 
    ASSERT(nIndex >= 0 && nIndex < m_nSize);
    m_pData[nIndex] = newElement; 
}

void*& CPtrArray::ElementAt(int nIndex)
{ 
    ASSERT(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex]; 
}

int CPtrArray::Add(void* newElement)
{ 
    int nIndex = m_nSize;
    SetAtGrow(nIndex, newElement);
    return nIndex; 
}

void*  CPtrArray::operator[](int nIndex) const { return GetAt(nIndex);     }
void*& CPtrArray::operator[](int nIndex)       { return ElementAt(nIndex); }


/////////////////////////////////////////////////////////////////////////////

void CPtrArray::SetAtGrow(int nIndex, void* newElement)
{
    //ASSERT_VALID(this);
    ASSERT(nIndex >= 0);

    if (nIndex >= m_nSize)
        SetSize(nIndex+1);
    m_pData[nIndex] = newElement;
}

void CPtrArray::InsertAt(int nIndex, void* newElement, int nCount /*=1*/)
{
    //ASSERT_VALID(this);
    ASSERT(nIndex >= 0);    // will expand to meet need
    ASSERT(nCount > 0);     // zero or negative size not allowed

    if (nIndex >= m_nSize)
    {
        // adding after the end of the array
        SetSize(nIndex + nCount);  // grow so nIndex is valid
    }
    else
    {
        // inserting in the middle of the array
        int nOldSize = m_nSize;
        SetSize(m_nSize + nCount);  // grow it to new size
        // shift old data up to fill gap
        memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
            (nOldSize-nIndex) * sizeof(void*));

        // re-init slots we copied from

        memset(&m_pData[nIndex], 0, nCount * sizeof(void*));

    }

    // insert new value in the gap
    ASSERT(nIndex + nCount <= m_nSize);
    while (nCount--)
        m_pData[nIndex++] = newElement;
}

void CPtrArray::RemoveAt(int nIndex, int nCount /* = 1 */)
{
    //ASSERT_VALID(this);
    ASSERT(nIndex >= 0);
    ASSERT(nCount >= 0);
    ASSERT(nIndex + nCount <= m_nSize);

    // just remove a range
    int nMoveCount = m_nSize - (nIndex + nCount);

    if (nMoveCount)
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
            nMoveCount * sizeof(void*));
    m_nSize -= nCount;
}

void CPtrArray::InsertAt(int nStartIndex, CPtrArray* pNewArray)
{
    //ASSERT_VALID(this);
    ASSERT(pNewArray != NULL);
    //ASSERT(pNewArray->IsKindOf(RUNTIME_CLASS(CPtrArray)));
    //ASSERT_VALID(pNewArray);
    ASSERT(nStartIndex >= 0);

    if (pNewArray->GetSize() > 0)
    {
        InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
        for (int i = 0; i < pNewArray->GetSize(); i++)
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
    }
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

/************************** End of CPtrArray Implementation *****************************/
/****************************************************************************************/





/****************************************************************************************/
/****************************** CString helper functions ********************************/

    // static class data, special inlines

static const char  afxChNil = '0';

    // For an empty string, m_???Data will point here
    // (note: avoids a lot of NULL pointer tests when we call standard
    //  C runtime libraries

//const CString afxEmptyString;

static void SafeDelete(char* pch)
{
	if(pch != NULL && pch != (char*)&afxChNil)
	    delete [] pch;
}

static int SafeStrlen(const char* psz) { return (psz == NULL) ? 0 : strlen(psz); }


/****************************** End of CString helper functions *************************/
/****************************************************************************************/


/****************************************************************************************/
/****************************** CString Implementation **********************************/


CString::CString(const char* psz)
{
	int nLen;
	if ((nLen = SafeStrlen(psz)) == 0) Init();
	else
	{
		AllocBuffer(nLen);
		memcpy(m_pchData, psz, nLen);
	}
}


CString::~CString() 
{
    SafeDelete(m_pchData);
}


void CString::Init()
{
	m_nDataLength = m_nAllocLength = 0;
    m_pchData = (char*)&afxChNil;
}


void CString::AllocBuffer(int nLen)
{
    if (nLen == 0)	Init();
    else
    {
	    m_pchData = new char[nLen+1];       // may throw an exception
        m_pchData[nLen]= '\0';
	    m_nDataLength  = nLen;
	    m_nAllocLength = nLen;
    }
}

void CString::AssignCopy(int nSrcLen, const char* pszSrcData)
{
    // check if it will fit
    if (nSrcLen > m_nAllocLength)
    {
	    // it won't fit, allocate another one
	    Empty();
	    AllocBuffer(nSrcLen);
    }
    if (nSrcLen != 0) memcpy(m_pchData, pszSrcData, nSrcLen);
    m_nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
{
    // will clone the data attached to this string
    // allocating 'nExtraLen' characters
    // Places results in uninitialized string 'dest'
    // Will copy the part or all of original data to start of new string

    int nNewLen = nCopyLen + nExtraLen;

    if (nNewLen == 0) dest.Init();
    else
    {
	    dest.AllocBuffer(nNewLen);
	    memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen);
    }
}


void CString::ConcatInPlace(int nSrcLen, const char* pszSrcData)
{
	//  -- the main routine for += operators
    // if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (m_nDataLength + nSrcLen > m_nAllocLength)
	{
		// we have to grow the buffer, use the Concat in place routine
		char* pszOldData = m_pchData;
		ConcatCopy(m_nDataLength, pszOldData, nSrcLen, pszSrcData);
		SafeDelete(pszOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(&m_pchData[m_nDataLength], pszSrcData, nSrcLen);
		m_nDataLength += nSrcLen;
	}
	m_pchData[m_nDataLength] = '\0';
}


void CString::Empty()                       // free up the data
{
	SafeDelete(m_pchData);
	Init();
}

const CString& CString::operator =(const char* psz)
{
	AssignCopy(SafeStrlen(psz), psz);
	return *this;
}


const CString& CString::operator+=(const char* psz)
{
	ConcatInPlace(SafeStrlen(psz), psz);
	return *this;
}


// Access to string implementation buffer as "C" character array
char* CString::GetBuffer(int nMinBufLength)
{
    if (nMinBufLength > m_nAllocLength)
	{
		// we have to grow the buffer
		char* pszOldData = m_pchData;
		int nOldLen = m_nDataLength;        // AllocBuffer will tromp it

		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pszOldData, nOldLen);
		m_nDataLength = nOldLen;
		m_pchData[m_nDataLength] = '\0';

		SafeDelete(pszOldData);
	}

	// return a pointer to the character storage for this string
    return m_pchData;
}



/************************** End of CString Implementation *******************************/
/****************************************************************************************/


/****************************************************************************************/
/****************************** CString Extensions **************************************/

CString operator +(const CString& string, const CString& string2)
{
	CString s;
	s.ConcatCopy(string.m_nDataLength, string.m_pchData, string2.m_nDataLength, string2.m_pchData);
	return s;
}


CString operator +(const CString& string, const char* psz)
{
	CString s;
	s.ConcatCopy(string.m_nDataLength, string.m_pchData, SafeStrlen(psz), psz);
	return s;
}


CString operator +(const char* psz, const CString& string)
{
	CString s;
	s.ConcatCopy(SafeStrlen(psz), psz, string.m_nDataLength, string.m_pchData);
	return s;
}

CString operator +(const CString& string1, char ch)
{
	CString s;
	s.ConcatCopy(string1.m_nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

CString operator +(char ch, const CString& string)
{
	CString s;
	s.ConcatCopy(1, &ch, string.m_nDataLength, string.m_pchData);
	return s;
}

/////////////////////////////////////////////////////////////////////////////


int operator==(const CString& s1, const CString& s2) { return s1.Compare(s2) == 0; }
int operator==(const CString& s1, const char* s2)    { return s1.Compare(s2) == 0; }
int operator==(const char* s1   , const CString& s2) { return s2.Compare(s1) == 0; }
int operator!=(const CString& s1, const CString& s2) { return s1.Compare(s2) != 0; }
int operator!=(const CString& s1, const char* s2)    { return s1.Compare(s2) != 0; }
int operator!=(const char* s1   , const CString& s2) { return s2.Compare(s1) != 0; }
int operator< (const CString& s1, const CString& s2) { return s1.Compare(s2) <  0; }
int operator< (const CString& s1, const char* s2)    { return s1.Compare(s2) <  0; }
int operator< (const char* s1   , const CString& s2) { return s2.Compare(s1) >  0; }
int operator> (const CString& s1, const CString& s2) { return s1.Compare(s2) >  0; }
int operator> (const CString& s1, const char* s2)    { return s1.Compare(s2) >  0; }
int operator> (const char* s1   , const CString& s2) { return s2.Compare(s1) <  0; }
int operator<=(const CString& s1, const CString& s2) { return s1.Compare(s2) <= 0; }
int operator<=(const CString& s1, const char* s2)    { return s1.Compare(s2) <= 0; }
int operator<=(const char* s1   , const CString& s2) { return s2.Compare(s1) >= 0; }
int operator>=(const CString& s1, const CString& s2) { return s1.Compare(s2) >= 0; }
int operator>=(const CString& s1, const char* s2)    { return s1.Compare(s2) >= 0; }
int operator>=(const char* s1   , const CString& s2) { return s2.Compare(s1) <= 0; }

///////////////////////////////////////////////////////////////////////////////

/*************************** End of CString Extensions *****************************/
/***********************************************************************************/



/************************** Usefull MFC classes *****************************************/
/****************************************************************************************/



