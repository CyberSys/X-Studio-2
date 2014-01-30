#pragma once
#include "GameDataWnd.h"
#include "Logic/SyntaxLibrary.h"

/// <summary>User interface</summary>
NAMESPACE_BEGIN(GUI)
   
   /// <summary>Dockable game data window</summary>
   class CCommandWnd : public CGameDataWnd
   {
      // ------------------------ TYPES --------------------------
   private:
	  
      // --------------------- CONSTRUCTION ----------------------
      
   public:
      CCommandWnd();
      virtual ~CCommandWnd();
       
      // ------------------------ STATIC -------------------------
   protected:
      DECLARE_MESSAGE_MAP()
      
      // --------------------- PROPERTIES ------------------------
	   
      // ---------------------- ACCESSORS ------------------------			
      
      // ----------------------- MUTATORS ------------------------
   protected:
      virtual void PopulateGroupCombo();
      virtual void PopulateItems(const wstring& searchTerm, UINT selectedGroup);

      // -------------------- REPRESENTATION ---------------------

   protected:
   };
   
NAMESPACE_END(GUI)
