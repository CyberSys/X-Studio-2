// GameDataWorker.cpp : implementation file
//

#include "stdafx.h"
#include "GameDataWorker.h"
#include "XFileSystem.h"
#include "SyntaxLibrary.h"
#include "StringLibrary.h"
#include "ScriptObjectLibrary.h"
#include "LegacySyntaxFileReader.h"
#include "GameObjectLibrary.h"
#include "DescriptionLibrary.h"
#include "PreferencesLibrary.h"

namespace Logic
{
   namespace Threads
   {
      
      // -------------------------------- CONSTRUCTION --------------------------------
   
      GameDataWorker::GameDataWorker() : BackgroundWorker((ThreadProc)ThreadMain)
      {
      }

      GameDataWorker::~GameDataWorker()
      {
      }

      // ------------------------------- STATIC METHODS -------------------------------

      /// <summary>Clears all game data.</summary>
      void  GameDataWorker::Clear()
      {
         // Strings
         StringLib.Clear();

         // script/game objects
         ScriptObjectLib.Clear();
         GameObjectLib.Clear();

         // Descriptions
         DescriptionLib.Clear();

         // syntax file
         SyntaxLib.Clear();
      }

      /// <summary>Loads game data</summary>
      /// <param name="data">arguments.</param>
      /// <returns></returns>
      DWORD WINAPI GameDataWorker::ThreadMain(GameDataWorkerData* data)
      {
         try
         {
            XFileSystem vfs;
            HRESULT  hr;

            // Init COM
            if (FAILED(hr=CoInitialize(NULL)))
               throw ComException(HERE, hr);

            // Feedback
            Console << Cons::UserAction << L"Loading " << VersionString(data->Version) << L" game data from " << data->GameFolder << ENDL;
            data->SendFeedback(ProgressType::Operation, 0, VString(L"Loading %s game data from '%s'", VersionString(data->Version).c_str(), data->GameFolder.c_str()));

            // Build VFS. 
            vfs.Enumerate(data->GameFolder, data->Version, data);

            // language files
            StringLib.Enumerate(vfs, data->Language, data);

            // script/game objects
            ScriptObjectLib.Enumerate(data);
            GameObjectLib.Enumerate(vfs, data);

            // Descriptions
            DescriptionLib.Enumerate(data);

            // legacy syntax file
            SyntaxLib.Enumerate(data);

            // Cleanup
            data->SendFeedback(Cons::UserAction, ProgressType::Succcess, 0, VString(L"Loaded %s game data successfully", VersionString(data->Version).c_str()));
            CoUninitialize();
            return 0;
         }
         catch (ExceptionBase& e)
         {
            // Feedback
            Console.Log(HERE, e);
            Console << ENDL;
            data->SendFeedback(Cons::Error, ProgressType::Failure, 0, GuiString(L"Failed to load game data : ") + e.Message);

            // BEEP!
            MessageBeep(MB_ICONERROR);

            // Cleanup
            CoUninitialize();
            return 0;
         }
      }

      // ------------------------------- PUBLIC METHODS -------------------------------
   
      // ------------------------------ PROTECTED METHODS -----------------------------

      // ------------------------------- PRIVATE METHODS ------------------------------

   }
}




