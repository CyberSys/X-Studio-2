#pragma once
#include "XmlReader.h"
#include "TemplateFile.h"

namespace Logic
{
   namespace IO
   {
      
      /// <summary>Reader for the xml document templates file</summary>
      class LogicExport TemplateFileReader : public XmlReader
      {
         // ------------------------ TYPES --------------------------
      protected:

         // --------------------- CONSTRUCTION ----------------------
      public:
         TemplateFileReader(StreamPtr in);
         virtual ~TemplateFileReader();

         NO_COPY(TemplateFileReader);	// Default copy semantics
         NO_MOVE(TemplateFileReader);	// Default move semantics

         // ------------------------ STATIC -------------------------

         // --------------------- PROPERTIES ------------------------

         // ---------------------- ACCESSORS ------------------------			

         // ----------------------- MUTATORS ------------------------
      public:
         TemplateList  ReadFile();

      protected:
         TemplateFile  ReadTemplate(XmlNodePtr node);

         // -------------------- REPRESENTATION ---------------------

      protected:
      };

   }
}

using namespace Logic::IO;
