#pragma once

#include "afxcmn.h"
#include "ToolBarEx.h"
#include "LanguageEdit.h"
#include "LanguageDocument.h"
#include "LanguageStringView.h"

/// <summary>User interface</summary>
NAMESPACE_BEGIN2(GUI,Views)

   /// <summary>Language document view</summary>
   class LanguageEditView : public CFormView
   {
      // ------------------------ TYPES --------------------------
   public:
	   enum { IDD = IDR_LANGUAGEVIEW };
	  
      // --------------------- CONSTRUCTION ----------------------
   protected:
	   LanguageEditView();           
   public:
	   virtual ~LanguageEditView();

      // ------------------------ STATIC -------------------------
      DECLARE_DYNCREATE(LanguageEditView)
      DECLARE_MESSAGE_MAP()

      // --------------------- PROPERTIES ------------------------
	  
      // ---------------------- ACCESSORS ------------------------			
   public:
      LanguageDocument*   GetDocument() const;

      // ----------------------- MUTATORS ------------------------
   public:
      

   protected:
      void AdjustLayout();
      void DisplayProperties();
      void DoDataExchange(CDataExchange* pDX) override;   

      handler void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;
      afx_msg void OnCommandChangeMode(UINT nID);
      afx_msg void OnCommandChangeText(UINT nID);
      afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
      virtual void OnInitialUpdate();
      afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
      afx_msg void OnSize(UINT nType, int cx, int cy);
      handler void onStringSelectionChanged();

      // -------------------- REPRESENTATION ---------------------
   protected:
      SelectionChangedHandler  fnStringSelectionChanged;
      LanguageEdit             RichEdit;
      ToolBarEx                ToolBar;
   };


NAMESPACE_END2(GUI,Views)

