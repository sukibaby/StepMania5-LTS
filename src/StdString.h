// =============================================================================
//  FILE:  StdString.h
//  AUTHOR:	Joe O'Leary (with outside help noted in comments)
//  REMARKS:
//		This header file declares the CStdStr template.  This template derives
//		the Standard C++ Library basic_string<> template and add to it the
//		the following conveniences:
//			- The full MFC RString set of functions (including implicit cast)
//			- writing to/reading from COM IStream interfaces
//			- Functional objects for use in STL algorithms
//
//		From this template, we intstantiate two classes:  CStdStringA and
//		CStdStringW.  The name "CStdString" is just a #define of one of these,
//		based upone the _UNICODE macro setting
//
//		This header also declares our own version of the MFC/ATL UNICODE-MBCS
//		conversion macros.  Our version looks exactly like the Microsoft's to
//		facilitate portability.
//
//	NOTE:
//		If you you use this in an MFC or ATL build, you should include either
//		afx.h or atlbase.h first, as appropriate.
//
//	PEOPLE WHO HAVE CONTRIBUTED TO THIS CLASS:
//
//		Several people have helped me iron out problems and othewise improve
//		this class.  OK, this is a long list but in my own defense, this code
//		has undergone two major rewrites.  Many of the improvements became
//		necessary after I rewrote the code as a template.  Others helped me
//		improve the RString facade.
//
//		Anyway, these people are (in chronological order):
//
//			- Pete the Plumber (???)
//			- Julian Selman
//			- Chris (of Melbsys)
//			- Dave Plummer
//			- John C Sipos
//			- Chris Sells
//			- Nigel Nunn
//			- Fan Xia
//			- Matthew Williams
//			- Carl Engman
//			- Mark Zeren
//			- Craig Watson
//			- Rich Zuris
//			- Karim Ratib
//			- Chris Conti
//			- Baptiste Lepilleur
//			- Greg Pickles
//			- Jim Cline
//			- Jeff Kohn
//			- Todd Heckel
//			- Ullrich Pollähne
//			- Joe Vitaterna
//			- Joe Woodbury
//			- Aaron (no last name)
//			- Joldakowski (???)
//			- Scott Hathaway
//			- Eric Nitzche
//			- Pablo Presedo
//			- Farrokh Nejadlotfi
//			- Jason Mills
//			- Igor Kholodov
//			- Mike Crusader
//			- John James
//			- Wang Haifeng
//			- Tim Dowty
//          - Arnt Witteveen

// Turn off browser references
// Turn off unavoidable compiler warnings


#if defined(_MSC_VER)
	#pragma component(browser, off, references, "CStdString")
	#pragma warning (push)
	#pragma warning (disable : 4290) // C++ Exception Specification ignored
	#pragma warning (disable : 4127) // Conditional expression is constant
	#pragma warning (disable : 4097) // typedef name used as synonym for class name
	#pragma warning (disable : 4512) // assignment operator could not be generated
#endif

#ifndef STDSTRING_H
#define STDSTRING_H

// If they want us to use only standard C++ stuff (no Win32 stuff)

typedef const char*		PCSTR;
typedef char*			PSTR;
typedef const wchar_t*	PCWSTR;
typedef wchar_t*		PWSTR;

// Standard headers needed
#include <string>			// basic_string
#include <algorithm>			// for_each, etc.

#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/* In RageUtil: */
void MakeUpper( char *p, size_t iLen );
void MakeLower( char *p, size_t iLen );
void MakeUpper( wchar_t *p, size_t iLen );
void MakeLower( wchar_t *p, size_t iLen );

/**
 * @brief Inline functions on which CStdString relies on.
 *
 * Usually for generic text mapping, we rely on preprocessor macro definitions
 * to map to string functions.  However the CStdStr<> template cannot use
 * macro-based generic text mappings because its character types do not get
 * resolved until template processing which comes AFTER macro processing.  In
 * other words, UNICODE is of little help to us in the CStdStr template.
 *
 * Therefore, to keep the CStdStr declaration simple, we have these inline
 * functions.  The template calls them often.  Since they are inline (and NOT
 * exported when this is built as a DLL), they will probably be resolved away
 * to nothing.
 *
 * Without these functions, the CStdStr<> template would probably have to broken
 * out into two, almost identical classes.  Either that or it would be a huge,
 * convoluted mess, with tons of "if" statements all over the place checking the
 * size of template parameter CT.
 *
 * In several cases, you will see two versions of each function.  One version is
 * the more portable, standard way of doing things, while the other is the
 * non-standard, but often significantly faster Visual C++ way.
 */
namespace StdString
{
// -----------------------------------------------------------------------------
// sstolower/sstoupper -- convert characters to upper/lower case
// -----------------------------------------------------------------------------
//inline char sstoupper(char ch)		{ return (char)::toupper(ch); }
//inline char sstolower(char ch)		{ return (char)::tolower(ch); }
/* Our strings are UTF-8; instead of having to play around with locales,
 * let's just manually toupper ASCII only.  If we really want to play with
 * Unicode cases, we can do it ourself in RageUtil. */
/**
 * @brief Turn the character into its uppercase equivalent.
 * @param ch the character to convert.
 * @return the converted character.
 */
char sstoupper(char ch) noexcept;
wchar_t sstoupper(wchar_t ch) noexcept;
/**
 * @brief Turn the character into its lowercase equivalent.
 * @param ch the character to convert.
 * @return the converted character.
 */
char sstolower(char ch) noexcept;
wchar_t sstolower(wchar_t ch) noexcept;

// -----------------------------------------------------------------------------
// ssasn: assignment functions -- assign "sSrc" to "sDst"
// -----------------------------------------------------------------------------
typedef std::string::size_type		SS_SIZETYPE; // just for shorthand, really
typedef std::string::pointer		SS_PTRTYPE;

/**
 * @brief Assign one string to another.
 * @param sDst the destination string.
 * @param sSrc the source string.
 */
void ssasn(std::string& sDst, const std::string& sSrc) noexcept;
void ssasn(std::wstring& sDst, const std::wstring& sSrc) noexcept;
/**
 * @brief Assign one string to another.
 * @param sDst the destination string.
 * @param pA the source string.
 */
void ssasn(std::string& sDst, PCSTR pA) noexcept;
void ssasn(std::wstring& sDst, PCWSTR pA) noexcept;

#undef StrSizeType


// -----------------------------------------------------------------------------
// ssadd: string object concatenation -- add second argument to first
// -----------------------------------------------------------------------------
/**
 * @brief Concatenate one string with another.
 * @param sDst the original string.
 * @param sSrc the string being added.
 */
void ssadd(std::string& sDst, const std::string& sSrc) noexcept;
void ssadd(std::wstring& sDst, const std::wstring& sSrc) noexcept;
/**
 * @brief Concatenate one string with another.
 * @param sDst the original string.
 * @param pA the string being added.
 */
void ssadd(std::string& sDst, PCSTR pA) noexcept;
void ssadd(std::wstring& sDst, PCWSTR pA) noexcept;

// -----------------------------------------------------------------------------
// ssicmp: comparison (case insensitive )
// -----------------------------------------------------------------------------
/**
 * @brief Perform a case insensitive comparison of the two strings.
 * @param pA1 the first string.
 * @param pA2 the second string.
 * @return >0 if pA1 > pA2, <0 if pA1 < pA2, or 0 otherwise.
 */
template<typename CT>
inline int ssicmp(const CT* pA1, const CT* pA2)
{
	CT f;
	CT l;

	do
	{
		f = sstolower(*(pA1++));
		l = sstolower(*(pA2++));
	} while ( (f) && (f == l) );

	return (int)(f - l);
}

// -----------------------------------------------------------------------------
// ssupr/sslwr: Uppercase/Lowercase conversion functions
// -----------------------------------------------------------------------------
#if 0
	template<typename CT>
	inline void sslwr(CT* pT, size_t nLen)
	{
		for ( CT* p = pT; static_cast<size_t>(p - pT) < nLen; ++p)
			*p = (CT)sstolower(*p);
	}
	template<typename CT>
	inline void ssupr(CT* pT, size_t nLen)
	{
		for ( CT* p = pT; static_cast<size_t>(p - pT) < nLen; ++p)
			*p = (CT)sstoupper(*p);
	}
#endif

inline void sslwr(char *pT, size_t nLen)
{
	MakeLower( pT, nLen );
}
inline void ssupr(char *pT, size_t nLen)
{
	MakeUpper( pT, nLen );
}
inline void sslwr(wchar_t *pT, size_t nLen)
{
	MakeLower( pT, nLen );
}
inline void ssupr(wchar_t *pT, size_t nLen)
{
	MakeUpper( pT, nLen );
}

//			Now we can define the template (finally!)
// =============================================================================
// TEMPLATE: CStdStr
//		template<typename CT> class CStdStr : public std::basic_string<CT>
//
// REMARKS:
//		This template derives from basic_string<CT> and adds some MFC RString-
//		like functionality
//
//		Basically, this is my attempt to make Standard C++ library strings as
//		easy to use as the MFC RString class.
//
//		Note that although this is a template, it makes the assumption that the
//		template argument (CT, the character type) is either char or wchar_t.
// =============================================================================

//#define CStdStr _SS	// avoid compiler warning 4786
template<typename CT>
class CStdStr;

/**
 * @brief Another way to concatenate two strings together.
 * @param str1 the original string.
 * @param str2 the string to be added.
 * @return the longer string.
 */
template<typename CT>
inline
CStdStr<CT> operator+(const  CStdStr<CT>& str1, const  CStdStr<CT>& str2)
{
	CStdStr<CT> strRet(str1);
	strRet.append(str2);
	return strRet;
}
/**
 * @brief Another way to concatenate two strings together.
 * @param str the original string.
 * @param t the string to be added.
 * @return the longer string.
 */
template<typename CT>
inline
CStdStr<CT> operator+(const  CStdStr<CT>& str, CT t)
{
	// this particular overload is needed for disabling reference counting
	// though it's only an issue from line 1 to line 2

	CStdStr<CT> strRet(str);	// 1
	strRet.append(1, t);				// 2
	return strRet;
}
/**
 * @brief Another way to concatenate two strings together.
 * @param str the original string.
 * @param pA the string to be added.
 * @return the longer string.
 */
template<typename CT>
inline
CStdStr<CT> operator+(const  CStdStr<CT>& str, PCSTR pA)
{
	return CStdStr<CT>(str) + CStdStr<CT>(pA);
}
/**
 * @brief Another way to concatenate two strings together.
 * @param pA the original string.
 * @param str the string to be added.
 * @return the longer string.
 */
template<typename CT>
inline
CStdStr<CT> operator+(PCSTR pA, const  CStdStr<CT>& str)
{
	CStdStr<CT> strRet(pA);
	strRet.append(str);
	return strRet;
}


/** @brief Our wrapper for std::string. */
template<typename CT>
class CStdStr : public std::basic_string<CT>
{
	// Typedefs for shorter names.  Using these names also appears to help
	// us avoid some ambiguities that otherwise arise on some platforms

	typedef typename std::basic_string<CT>		MYBASE;	 // my base class
	typedef CStdStr<CT>				MYTYPE;	 // myself
	typedef typename MYBASE::const_pointer		PCMYSTR; // PCSTR
	typedef typename MYBASE::pointer		PMYSTR;	 // PSTR
	typedef typename MYBASE::iterator		MYITER;  // my iterator type
	typedef typename MYBASE::const_iterator		MYCITER; // you get the idea...
	typedef typename MYBASE::reverse_iterator	MYRITER;
	typedef typename MYBASE::size_type		MYSIZE;
	typedef typename MYBASE::value_type		MYVAL;
	typedef typename MYBASE::allocator_type		MYALLOC;
	typedef typename MYBASE::traits_type		MYTRAITS;

public:

	// CStdStr inline constructors
	CStdStr()
	{
	}

	CStdStr(const MYTYPE& str) : MYBASE(str)
	{
	}

	CStdStr(const std::string& str): MYBASE(str)
	{
	}

	CStdStr(PCMYSTR pT, MYSIZE n) : MYBASE(pT, n)
	{
	}

	CStdStr(PCSTR pA)
	{
		*this = pA;
	}

	CStdStr(MYCITER first, MYCITER last)
		: MYBASE(first, last)
	{
	}

	CStdStr(MYSIZE nSize, MYVAL ch, const MYALLOC& al=MYALLOC())
		: MYBASE(nSize, ch, al)
	{
	}

	// CStdStr inline assignment operators
	MYTYPE& operator=(const MYTYPE& str)
	{
		ssasn(*this, str);
		return *this;
	}

	MYTYPE& operator=(const std::string& str)
	{
		ssasn(*this, str);
		return *this;
	}

	MYTYPE& operator=(PCSTR pA)
	{
		ssasn(*this, pA);
		return *this;
	}

	MYTYPE& operator=(CT t)
	{
		this->assign(1, t);
		return *this;
	}

	// -------------------------------------------------------------------------
	// CStdStr inline concatenation.
	// -------------------------------------------------------------------------
	MYTYPE& operator+=(const MYTYPE& str)
	{
		ssadd(*this, str);
		return *this;
	}

	MYTYPE& operator+=(const std::string& str)
	{
		ssadd(*this, str);
		return *this;
	}

	MYTYPE& operator+=(PCSTR pA)
	{
		ssadd(*this, pA);
		return *this;
	}

	MYTYPE& operator+=(CT t)
	{
		this->append(1, t);
		return *this;
	}


	// addition operators -- global friend functions.
	friend	MYTYPE	operator+ <>(const MYTYPE& str1,	const MYTYPE& str2);
	friend	MYTYPE	operator+ <>(const MYTYPE& str,	CT t);
	friend	MYTYPE	operator+ <>(const MYTYPE& str,	PCSTR sz);
	friend	MYTYPE	operator+ <>(PCSTR pA,				const MYTYPE& str);

	// -------------------------------------------------------------------------
	// CStdStr -- Direct access to character buffer.  In the MS' implementation,
	// the at() function that we use here also calls _Freeze() providing us some
	// protection from multithreading problems associated with ref-counting.
	// -------------------------------------------------------------------------
	CT* GetBuffer(int nMinLen=-1)
	{
		if ( static_cast<int>(this->size()) < nMinLen )
			this->resize(static_cast<MYSIZE>(nMinLen));

		return this->empty() ? const_cast<CT*>(this->data()) : &(this->at(0));
	}

	void ReleaseBuffer(int nNewLen=-1)
	{
		this->resize(static_cast<MYSIZE>(nNewLen > -1 ? nNewLen : MYTRAITS::length(this->c_str())));
	}

	// Array-indexing operators.  Required because we defined an implicit cast
	// to operator const CT* (Thanks to Julian Selman for pointing this out)
	CT& operator[](int nIdx)
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	const CT& operator[](int nIdx) const
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	CT& operator[](unsigned int nIdx)
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	const CT& operator[](unsigned int nIdx) const
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	CT& operator[](long unsigned int nIdx){
	  return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	const CT& operator[](long unsigned int nIdx) const {
	  return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	CT& operator[](long long unsigned int nIdx){
	  return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	const CT& operator[](long long unsigned int nIdx) const {
	  return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}
#ifndef SS_NO_IMPLICIT_CASTS
	operator const CT*() const
	{
		return this->c_str();
	}
#endif
};


// =============================================================================
//						END OF CStdStr INLINE FUNCTION DEFINITIONS
// =============================================================================

//	Now typedef our class names based upon this humongous template
/** @brief Typedef the class names based on the template */
typedef CStdStr<char>		CStdStringA;	// a better std::string
#define CStdString				CStdStringA


// -----------------------------------------------------------------------------
// FUNCTIONAL COMPARATORS:
// REMARKS:
//		These structs give us functional classes (which may be used in
//		Standard C++ Library collections and algorithms) that perform
//		case-insensitive comparisons of CStdString objects.  This is
//		useful for maps in which the key may be the proper string but
//		in the wrong case.
// -----------------------------------------------------------------------------

#define StdStringLessNoCase		SSLNCA
#define StdStringEqualsNoCase		SSENCA

struct StdStringLessNoCase
{
	inline
	bool operator()(const CStdStringA& sLeft, const CStdStringA& sRight) const
	{ return ssicmp(sLeft.c_str(), sRight.c_str()) < 0; }
};
struct StdStringEqualsNoCase
{
	inline
	bool operator()(const CStdStringA& sLeft, const CStdStringA& sRight) const
	{ return ssicmp(sLeft.c_str(), sRight.c_str()) == 0; }
};

// These std::swap specializations come courtesy of Mike Crusader.

//namespace std
//{
//	inline void swap(CStdStringA& s1, CStdStringA& s2) throw()
//	{
//		s1.swap(s2);
//	}
//}

}	// namespace StdString

typedef StdString::CStdString RString;
// FIXME: separate these into functions that either modify the argument, or
// return a new string leaving the original unmodified.
inline RString MakeLower(RString&& s) {
  for (size_t i = 0; i < s.size(); ++i) {
    s[i] = tolower(s[i]);
  }
  return std::move(s);
}
inline RString& MakeLower(RString& s) {
  for (size_t i = 0; i < s.size(); ++i) {
    s[i] = tolower(s[i]);
  }
  return s;
}
inline RString MakeUpper(RString&& s) {
  for (size_t i = 0; i < s.size(); ++i) {
    s[i] = toupper(s[i]);
  }
  return std::move(s);
}
inline RString& MakeUpper(RString& s) {
  for (size_t i = 0; i < s.size(); ++i) {
    s[i] = toupper(s[i]);
  }
  return s;
}
inline bool EqualsNoCase(const RString& s1, const RString& s2) {
  if (s1.size() != s2.size()) {
    return false;
  }
  for (size_t i = 0; i < s1.size(); ++i) {
    if (tolower(s1[i]) != tolower(s2[i])) {
      return false;
    }
  }
  return true;
}
inline int CompareNoCase(const RString& s1, const RString& s2) {
  size_t len = s1.length();
  size_t other_len = s2.length();
  for (size_t i = 0; i < std::min(len, other_len); ++i) {
    int a = tolower(s1[i]);
    int b = tolower(s2[i]);
    if (a - b != 0) {
      return a - b;
    }
  }
  return len - other_len;
}
inline RString Left(const RString& s, int n) {
  n = std::max(n, 0);
  n = std::min(n, static_cast<int>(s.size()));
  return s.substr(0, n);
}
inline RString Right(const RString& s, int n) {
  n = std::max(n, 0);
  n = std::min(n, static_cast<int>(s.size()));
  return s.substr(static_cast<int>(s.size()) - n);
}
inline void Replace(RString& s, const RString& a, const RString& b) {
  size_t idx = 0;
  size_t a_len = a.length();
  size_t b_len = b.length();
  while (idx = s.find(a, idx), idx != RString::npos) {
    s.replace(idx, a_len, b);
    idx += b_len;
  }
}
inline void Replace(RString& s, char a, char b) {
  size_t len = s.length();
  for (size_t i = 0; i < len; ++i) {
    if (s[i] == a) {
      s[i] = b;
    }
  }
}

#if defined(_MSC_VER)
	#pragma warning (pop)
#endif

#endif	// #ifndef STDSTRING_H

/**
 * @file
 * @author Joseph M. O'Leary (c) 1999
 * @section LICENSE
 * COPYRIGHT:
 *	1999 Joseph M. O'Leary.  This code is free.  Use it anywhere you want.
 *	Rewrite it, restructure it, whatever.  Please don't blame me if it makes
 *	your $30 billion dollar satellite explode in orbit.  If you redistribute
 *	it in any form, I'd appreciate it if you would leave this notice here.
 *
 *	If you find any bugs, please let me know:
 *
 *			jmoleary@earthlink.net
 *			http://home.earthlink.net/~jmoleary
 */
