
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

// Exclude rarely-used stuff from Windows headers
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            
#endif

#include "../targetver.h"

// Tweaks
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS        // some CString constructors will be explicit
#define _AFX_ALL_WARNINGS     // turns off MFC's hiding of some common and often safely ignored warning messages

 
// MFC
#include <afxwin.h>              // MFC core and standard components
#include <afxext.h>              // MFC extensions
#include <afxcmn.h>             // MFC support for Windows Common Controls
#include <afxcontrolbars.h>     // MFC support for ribbons and control bars


// Visual Leak Detector
#include <vld.h>


// STL
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <memory>    // shared/unique ptr
using namespace std;

/// <summary>BugFix for Release version optimizing away [w]string::npos</summary>
/// <remarks>See https://connect.microsoft.com/VisualStudio/feedback/details/586959/std  (Bug 586959) for details</remarks>
#if _MSC_VER >= 1600
const wstring::size_type wstring::npos = (wstring::size_type) -1;
#endif

// COM
#include <comdef.h>

// Utilities
#undef _UTIL_LIB
#undef _LOGIC_DLL
#include "../Utils/Utils.h"
#include "../Logic/ConsoleWnd.h"

// Application 
#include "Application.h"

// Preferences
#include "../Logic/PreferencesLibrary.h"

// Import Resource IDs 
#include "../Resources/Resources.h"


// Common controls v6.0
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
