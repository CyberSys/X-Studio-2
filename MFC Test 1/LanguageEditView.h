#pragma once

#include "afxcmn.h"
#include "LanguageDocument.h"

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
	   LanguageEditView();           // protected constructor used by dynamic creation
	   virtual ~LanguageEditView();

      // ------------------------ STATIC -------------------------
   public:
      DECLARE_DYNCREATE(LanguageEditView)
   protected:
      DECLARE_MESSAGE_MAP()

      // --------------------- PROPERTIES ------------------------
	  
      // ---------------------- ACCESSORS ------------------------			
   public:
   #ifdef _DEBUG
	   virtual void AssertValid() const;
	   virtual void Dump(CDumpContext& dc) const;
   #endif   
      LanguageDocument* GetDocument() const;

      // ----------------------- MUTATORS ------------------------
   protected:
      void AdjustLayout();

	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support   
      virtual void OnInitialUpdate();
      afx_msg void OnSize(UINT nType, int cx, int cy);

      // -------------------- REPRESENTATION ---------------------
   public:
      CRichEditCtrl RichEdit;
      
   };

   #ifndef _DEBUG  
   inline LanguageDocument* LanguageEditView::GetDocument() const
      { return reinterpret_cast<LanguageDocument*>(m_pDocument); }
   #endif


NAMESPACE_END2(GUI,Views)

