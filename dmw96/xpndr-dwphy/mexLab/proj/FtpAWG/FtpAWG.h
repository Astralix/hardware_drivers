// FtpAWG.h : main header file for the FTPAWG DLL
//

#if !defined(AFX_FTPAWG_H__E57DD8EF_6958_47FB_98D1_76B3A81DF87C__INCLUDED_)
#define AFX_FTPAWG_H__E57DD8EF_6958_47FB_98D1_76B3A81DF87C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CFtpAWGApp
// See FtpAWG.cpp for the implementation of this class
//

class CFtpAWGApp : public CWinApp
{
public:
	CFtpAWGApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFtpAWGApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CFtpAWGApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTPAWG_H__E57DD8EF_6958_47FB_98D1_76B3A81DF87C__INCLUDED_)
