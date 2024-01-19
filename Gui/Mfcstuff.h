/*********************************************************************

                        MfcStuff.h

**********************************************************************/



#ifndef __MFCSTUFF_H__
#define __MFCSTUFF_H__





/****************************************************************************************/
/************************** Usefull MFC classes *****************************************/




#ifdef _DEBUG
    #define ASSERT(exp)                                                 \
    {                                                                   \
        if(!(exp))                                                      \
        {                                                               \
            char line[32];                                              \
            sprintf( line, ", line %d!!!\n",  __LINE__ );               \
            OutputDebugString("Assertion failure on file " __FILE__ );  \
            OutputDebugString(line );                                   \
        }                                                               \
    }
#else
    #define ASSERT(exp)
#endif


class CObject
{
public:

// Object model (types, destruction, allocation)
	//virtual CRuntimeClass* GetRuntimeClass() const;
	virtual ~CObject() {};  // virtual destructors are necessary

	// Diagnostic allocations
	//void* operator new(size_t, void* p);
	//void* operator new(size_t nSize);
	//void operator delete(void* p);

//#ifdef _DEBUG
	// for file name/line number tracking using DEBUG_NEW
	//void* operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
//#endif

	// Disable the copy constructor and assignment by default so you will get
	//   compiler errors instead of unexpected behaviour if you pass objects
	//   by value or assign objects.
protected:
	CObject() {};

    void DebugString( LPCSTR format, LPCSTR text );
    void DebugString( LPCSTR format, LPCSTR text, int i );

private:
	//CObject(const CObject& objectSrc);              // no implementation
	//void operator=(const CObject& objectSrc);       // no implementation

// Attributes
public:
	//BOOL IsSerializable() const;
	//BOOL IsKindOf(const CRuntimeClass* pClass) const;

// Overridables
	//virtual void Serialize(CArchive& ar);

	// Diagnostic Support
    virtual void AssertValid() const = 0;
	//virtual void Dump(CDumpContext& dc) const;

// Implementation
public:
	//static CRuntimeClass AFXAPI_DATA classCObject;
};



/****************************************************************************************/
/****************************** CString definition **************************************/


class CString : public CObject
{
private:

// Implementation
protected:
	// lengths/sizes in characters
	//  (note: an extra character is always allocated)
	char* m_pchData;            // actual string (zero terminated)
	int m_nDataLength;          // does not include terminating 0
	int m_nAllocLength;         // does not include terminating 0

	// implementation helpers
	void Init();

	void AllocBuffer(int nLen);

    void AssignCopy(int nSrcLen, const char* pszSrcData);

    void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;

    void ConcatCopy(int nSrc1Len, const char* pszSrc1Data, int nSrc2Len, const char* pszSrc2Data)
    {
        // -- master concatenation routine
        // Concatenate two sources
        // -- assume that 'this' is a new CString object

	    int nNewLen = nSrc1Len + nSrc2Len;
	    AllocBuffer(nNewLen);
	    memcpy(m_pchData, pszSrc1Data, nSrc1Len);
	    memcpy(&m_pchData[nSrc1Len], pszSrc2Data, nSrc2Len);
    }

    void ConcatInPlace(int nSrcLen, const char* pszSrcData);



public:

    // Constructors
	CString()
    {
        Init();
    }

    CString(const CString& stringSrc)
    {
        stringSrc.AllocCopy(*this, stringSrc.m_nDataLength, 0, 0);
    }


	CString(char ch, int nRepeat = 1)
    {
	    if (nRepeat < 1)
		   // return empty string if invalid repeat count
		   Init();
	    else
	    {
		    AllocBuffer(nRepeat);
		    memset(m_pchData, ch, nRepeat);
	    }
    }

	CString(const char* psz);

    CString(const char* pch, int nLength)
    {
	    if (nLength == 0)
		   Init();
	    else
	    {
		    AllocBuffer(nLength);
		    memcpy(m_pchData, pch, nLength);
	    }
    }


   ~CString();

    // Attributes & Operations
	// as an array of characters
	int  GetLength() const { return m_nDataLength; }
	int  IsEmpty()   const { return m_nDataLength == 0; }


	void Empty();                      // free up the data

	char GetAt(int nIndex)      const { return m_pchData[nIndex];}
	char operator[](int nIndex) const { return m_pchData[nIndex];}

	void SetAt(int nIndex, char ch) { m_pchData[nIndex] = ch; }
	operator const char*() const	{ return (const char*)m_pchData; }


	// overloaded assignment
    const CString& operator =(const CString& stringSrc)
    {
	    AssignCopy(stringSrc.m_nDataLength, stringSrc.m_pchData);
	    return *this;
    }
    const CString& operator =(char ch)
    {
	    AssignCopy(1, &ch);
	    return *this;
    }

    const CString& operator =(const char* psz);

	// string concatenation
    const CString& operator+=(const CString& string)
    {
	    ConcatInPlace(string.m_nDataLength, string.m_pchData);
	    return *this;
    }
    const CString& operator+=(char ch)
    {
	    ConcatInPlace(1, &ch);
	    return *this;
    }

    const CString& operator+=(const char* psz);

    friend	CString operator+(const CString& string1, const CString& string2);
	friend	CString operator+(const CString& string, char ch);
	friend	CString operator+(char ch, const CString& string);
	friend	CString operator+(const CString& string, const char* psz);
	friend	CString operator+(const char* psz, const CString& string);

	// string comparison
	int Compare(const char* psz)       const { return strcmp(m_pchData, psz); }
	int CompareNoCase(const char* psz) const { return stricmp(m_pchData, psz); }
	int Collate(const char* psz)       const { return strcoll(m_pchData, psz); }

	// simple sub-string extraction
	CString Mid(int nFirst, int nCount) const
    {
    	// out-of-bounds requests return sensible things
	    if (nFirst + nCount > m_nDataLength)
		   nCount = m_nDataLength - nFirst;
	    if (nFirst > m_nDataLength)
		   nCount = 0;

	    CString dest;
	    AllocCopy(dest, nCount, nFirst, 0);
	    return dest;
    }
	CString Mid(int nFirst) const {	return Mid(nFirst, m_nDataLength - nFirst);}
    CString Left(int nCount) const
    {
    	if (nCount > m_nDataLength)
	    	nCount = m_nDataLength;

	    CString dest;
	    AllocCopy(dest, nCount, 0, 0);
	    return dest;
    }
    CString Right(int nCount) const
    {
    	if (nCount > m_nDataLength)
	    	nCount = m_nDataLength;

	    CString dest;
	    AllocCopy(dest, nCount, m_nDataLength-nCount, 0);
	    return dest;
    }

    CString SpanIncluding(const char* pszCharSet) const { return Left(strspn (m_pchData, pszCharSet));}
    CString SpanExcluding(const char* pszCharSet) const { return Left(strcspn(m_pchData, pszCharSet));}


	// upper/lower/reverse conversion
	void MakeUpper()   { strupr(m_pchData); }
	void MakeLower()   { strlwr(m_pchData); }
	void MakeReverse() { strrev(m_pchData); }

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(char ch) const                    // like "C" strchr
    {
	    // find a single character (strchr)
	    char* psz = strchr(m_pchData, ch);
	    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
    }

    int ReverseFind(char ch) const
    {
	    // find a single character (start backwards, strrchr)
	    char* psz = (char*) strrchr(m_pchData, ch);
	    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
    }

    int FindOneOf(const char* pszCharSet) const
    {
	    char* psz = (char*) strpbrk(m_pchData, pszCharSet);
	    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
    }

	// look for a specific sub-string
	int Find(const char* pszSub) const         // like "C" strstr
    {
    	char* psz = (char*) strstr(m_pchData, pszSub);
	    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
    }

	// Access to string implementation buffer as "C" character array
	char* GetBuffer(int nMinBufLength);

    void ReleaseBuffer(int nNewLength = -1)
    //void CString::ReleaseBuffer(int nNewLength)
    {
	    if (nNewLength == -1)
		   nNewLength = strlen(m_pchData); // zero terminated

        m_nDataLength = nNewLength;
	    m_pchData[m_nDataLength] = '\0';
    }

    char* GetBufferSetLength(int nNewLength)
    {
    	GetBuffer(nNewLength);
	    m_nDataLength = nNewLength;
	    m_pchData[m_nDataLength] = '\0';
	    return m_pchData;
    }

    void AssertValid() const {};
};


/**************************** End of CString definition *********************************/
/****************************************************************************************/


/****************************************************************************************/
/****************************** CString Extensions **************************************/

CString operator +(const CString& string, const CString& string2);
CString operator +(const CString& string, const char* psz);
CString operator +(const char* psz, const CString& string);
CString operator +(const CString& string1, char ch);
CString operator +(char ch, const CString& string);

int operator==(const CString& s1, const CString& s2);
int operator==(const CString& s1, const char* s2);
int operator==(const char* s1   , const CString& s2);
int operator!=(const CString& s1, const CString& s2);
int operator!=(const CString& s1, const char* s2);
int operator!=(const char* s1   , const CString& s2);
int operator< (const CString& s1, const CString& s2);
int operator< (const CString& s1, const char* s2);
int operator< (const char* s1   , const CString& s2);
int operator> (const CString& s1, const CString& s2);
int operator> (const CString& s1, const char* s2);
int operator> (const char* s1   , const CString& s2);
int operator<=(const CString& s1, const CString& s2);
int operator<=(const CString& s1, const char* s2);
int operator<=(const char* s1   , const CString& s2);
int operator>=(const CString& s1, const CString& s2);
int operator>=(const CString& s1, const char* s2);
int operator>=(const char* s1   , const CString& s2);


/*************************** End of CString Extensions *****************************/
/***********************************************************************************/




/***********************************************************************************/
/**************************** CPtrArray definition *********************************/


class CPtrArray : public CObject
{

    //DECLARE_DYNAMIC(CPtrArray)
public:

// Construction
    CPtrArray();

// Attributes
    int GetSize() const;
    int GetUpperBound() const;
    void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
    // Clean up
    void FreeExtra();
    void RemoveAll();

    // Accessing elements
    void* GetAt(int nIndex) const;
    void SetAt(int nIndex, void* newElement);
    void*& ElementAt(int nIndex);

    // Potentially growing the array
    void SetAtGrow(int nIndex, void* newElement);
    int Add(void* newElement);

    // overloaded operator helpers
    void* operator[](int nIndex) const;
    void*& operator[](int nIndex);

    // Operations that move elements around
    void InsertAt(int nIndex, void* newElement, int nCount = 1);
    void RemoveAt(int nIndex, int nCount = 1);
    void InsertAt(int nStartIndex, CPtrArray* pNewArray);

// Implementation
protected:
    void** m_pData;   // the actual array of data
    int m_nSize;      // # of elements (upperBound - 1)
    int m_nMaxSize;   // max allocated
    int m_nGrowBy;    // grow amount

public:
    ~CPtrArray();
//#ifdef _DEBUG
//    void Dump(CDumpContext&) const;
    void AssertValid() const { return; }
//#endif
};

/**************************** End of CPtrArray definition *******************************/
/****************************************************************************************/


/************************** End of Usefull MFC classes **********************************/
/****************************************************************************************/


#endif      /* __MFCSTUFF_H__  */


