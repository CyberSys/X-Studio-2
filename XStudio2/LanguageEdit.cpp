#include "stdafx.h"
#include "LanguageEdit.h"
#include "OleBitmap.h"
#include "Logic/RichStringWriter.h"
#include "Logic/LanguagePage.h"

/// <summary>User interface controls</summary>
NAMESPACE_BEGIN2(GUI,Controls)

   // --------------------------------- CONSTANTS ----------------------------------
   
   /// <summary>Matches an opening tag with up to four properties</summary>
   const wregex LanguageEdit::MatchComplexTag
   (
      L"\\[" 
      L"(article|declare|ranking|select|text|var)" 
      L"(\\s+(args|cols|colwidth|colspacing|name|neg|script|state|title|type|value)=[\"\'][^\"\']*[\"\'])?"  // Apostrophes or inverted commas
      L"(\\s+(args|cols|colwidth|colspacing|name|neg|script|state|title|type|value)=[\"\'][^\"\']*[\"\'])?" 
      L"(\\s+(args|cols|colwidth|colspacing|name|neg|script|state|title|type|value)=[\"\'][^\"\']*[\"\'])?" 
      L"(\\s+(args|cols|colwidth|colspacing|name|neg|script|state|title|type|value)=[\"\'][^\"\']*[\"\'])?" 
      L"\\s?/?"  // Possibly self closing
      L"\\]"
   );   

   /// <summary>Matches a unix style colour code</summary>
   const wregex LanguageEdit::MatchColourCode
   (
      L"(\\\\033A|\\\\033B|\\\\033C|\\\\033G|\\\\033O|\\\\033M|\\\\033R|\\\\033W|\\\\033X|\\\\033Y|\\\\033Z|\\\\053)" 
   );

   /// <summary>Matches an opening tag without properties or a closing tag</summary>
   const wregex LanguageEdit::MatchSimpleTag
   (
      L"\\[/?" // Possibly closing
      L"(b|i|u|left|right|center|justify" 
      L"|article|author|title|text|select" 
      L"|black|blue|cyan|green|grey|orange|magenta|red|silver|white|yellow)"
      L"\\]"
   );

   /// <summary>Matches an substring reference</summary>
   const wregex LanguageEdit::MatchSubString
   (
      L"\\{"
      L"\\d+" 
      L"(,\\d+)?" 
      L"\\}"
   );

   /// <summary>Matches a mission director variable</summary>
   const wregex LanguageEdit::MatchVariable
   (
      L"\\{"
      L"[a-z\\.]+" 
      L"@" 
      L"[_ \\w\\d\\.]+" 
      L"\\}"
   );
   
   // --------------------------------- APP WIZARD ---------------------------------
  
   IMPLEMENT_DYNAMIC(LanguageEdit, RichEditEx)

   BEGIN_MESSAGE_MAP(LanguageEdit, RichEditEx)
      ON_CONTROL_REFLECT(EN_CHANGE, &LanguageEdit::OnTextChange)
      /*ON_COMMAND_RANGE(ID_EDIT_SELECT_ALL, ID_EDIT_REDO, &LanguageEdit::OnCommandChangeText)
      ON_COMMAND_RANGE(ID_EDIT_COPY, ID_EDIT_CUT, &LanguageEdit::OnCommandChangeText)
      ON_COMMAND_RANGE(ID_EDIT_PASTE, ID_EDIT_PASTE, &LanguageEdit::OnCommandChangeText)
      ON_COMMAND_RANGE(ID_EDIT_CLEAR, ID_EDIT_CLEAR, &LanguageEdit::OnCommandChangeText)
      ON_COMMAND_RANGE(ID_EDIT_BOLD, ID_EDIT_ADD_BUTTON, &LanguageEdit::OnCommandChangeText)
      ON_COMMAND_RANGE(ID_VIEW_SOURCE, ID_VIEW_DISPLAY, &LanguageEdit::OnCommandChangeMode)
      ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, &LanguageEdit::OnQueryClipboardCommand)
      ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &LanguageEdit::OnQueryClipboardCommand)
      ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &LanguageEdit::OnQueryClipboardCommand)
      ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, &LanguageEdit::OnQueryClipboardCommand)
      ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &LanguageEdit::OnQueryClipboardCommand)
      ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &LanguageEdit::OnQueryClipboardCommand)
      ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, &LanguageEdit::OnQueryFormatCommand)
      ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_BOLD, ID_EDIT_ADD_BUTTON, &LanguageEdit::OnQueryFormatCommand)
      ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SOURCE, ID_VIEW_DISPLAY, &LanguageEdit::OnQueryModeCommand)*/
   END_MESSAGE_MAP()
   
   // -------------------------------- CONSTRUCTION --------------------------------

   LanguageEdit::LanguageEdit() : Mode(EditMode::Edit), Callback(nullptr), Document(nullptr)
   {
   }

   LanguageEdit::~LanguageEdit()
   {
   }

   // ------------------------------- STATIC METHODS -------------------------------

   /// <summary>Creates a bitmap for a button</summary>
   /// <param name="wnd">Edit window.</param>
   /// <param name="txt">button text</param>
   /// <param name="id">button ID.</param>
   /// <returns>New button bitmap</returns>
   /// <exception cref="Logic::Win32Exception">Drawing error</exception>
   HBITMAP LanguageEdit::ButtonData::CreateBitmap(LanguageEdit* wnd, const RichString& txt, const wstring& id)
   {
      CClientDC dc(wnd);
      CBitmap   bmp;
      CFont     font;
      CDC       memDC;
            
      // Create memory DC
      if (!memDC.CreateCompatibleDC(&dc))
         throw Win32Exception(HERE, L"Unable to create memory DC");

      // Load bitmap
      CRect rcButton(0,0, 160,19);
      if (!bmp.LoadBitmapW(IDB_RICH_BUTTON))
         throw Win32Exception(HERE, L"Unable to load empty button image");

      // Setup DC
      font.CreatePointFont(9*10, L"Arial");
      auto prevBmp = memDC.SelectObject(&bmp);
      auto prevFont = memDC.SelectObject(&font);
      
      // Draw button text onto bitmap
      memDC.SetBkMode(TRANSPARENT);
      RichTextRenderer::DrawLine(&memDC, rcButton, txt, RenderFlags::RichText);

      // Cleanup without destroying bitmap
      memDC.SelectObject(prevBmp);
      memDC.SelectObject(prevFont);

      // Detach/return bitmap
      return (HBITMAP)bmp.Detach();
   }

   // ------------------------------- PUBLIC METHODS -------------------------------
   
   /// <summary>Clears the text and resets font to Arial 10pt without firing EN_CHANGED</summary>
   void  LanguageEdit::Clear()
   {
      // Pause updates
      FreezeWindow(true);

      // Clear
      SetWindowText(L"");
      SetDefaultCharFormat(DefaultCharFormat());
      
      // Reset Undo
      EmptyUndoBuffer();

      // Resume updates
      FreezeWindow(false);
   }

   /// <summary>Gets contents as a string delimited by single char (\n) line breaks.</summary>
   /// <returns></returns>
   wstring  LanguageEdit::GetAllText() const
   {
      auto txt = __super::GetAllText();

      // Convert '\v' to '\n'
      for (auto& ch : txt)
         if (ch == '\v')
            ch = '\n';

      return txt;
   }

   /// <summary>Gets the edit mode.</summary>
   /// <returns></returns>
   LanguageEdit::EditMode  LanguageEdit::GetEditMode() const
   {
      return Mode;
   }

   /// <summary>Initializes the specified document.</summary>
   /// <param name="doc">The document.</param>
   /// <exception cref="Logic::ArgumentNullException">doc is nullptr</exception>
   void  LanguageEdit::Initialize(LanguageDocument* doc)
   {
      REQUIRED(doc);

      // Store document
      Document = doc;

      // Initially disable
      SetReadOnly(TRUE);

      // Initialize
      __super::Initialize(MessageBackground);

      // Create+set callback
      Callback.Attach(new EditCallback(this), true);
      SetOLECallback(Callback);
   }

   /// <summary>Inserts a button at the caret.</summary>
   /// <param name="txt">Button source text.</param>
   /// <param name="id">Button identifier.</param>
   void LanguageEdit::InsertButton(const wstring& txt, const wstring& id)
   {
      try
      {
         IOleClientSitePtr clientSite;
         ILockBytesPtr     lockBytes;
         IStoragePtr       storage;
         CBitmap           bitmap;
         HRESULT           hr;
         
	      // Get container site
         OleDocument->GetClientSite(&clientSite);

	      // Initialize a Storage Object
         if (FAILED(hr=::CreateILockBytesOnHGlobal(NULL, TRUE, &lockBytes))
          || FAILED(hr=::StgCreateDocfileOnILockBytes(lockBytes, STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, &storage)))
            throw ComException(HERE, hr);
         
         // Create a static picture OLE Object 
         bitmap.Attach(ButtonData::CreateBitmap(this, RichStringParser(txt, Alignment::Centre).Output, id));
         IOleObjectPtr picture = OleBitmap::CreateStatic(bitmap, clientSite, storage);
         
         // Notify object it's embedded
         OleSetContainedObject(picture, TRUE);

         // Lookup CLSID
         CLSID  clsid;
         if (FAILED(hr=picture->GetUserClassID(&clsid)))
            throw ComException(HERE, hr);
         
         // Define object
         ReObject  reObject;
         reObject.clsid    = clsid;
         reObject.cp       = REO_CP_SELECTION;
         reObject.dvaspect = DVASPECT_CONTENT;
         reObject.poleobj  = picture;
         reObject.polesite = clientSite;
         reObject.pstg     = storage;

         // Associate button data
         reObject.dwUser = reinterpret_cast<DWORD>(new ButtonData(txt, id));

         // Insert the object at the current location
         OleDocument->InsertObject(&reObject);
      }
      catch (_com_error& e) {
         Console.Log(HERE, ComException(HERE, e));
      }
      catch (ExceptionBase& e) {
         Console.Log(HERE, e);
      }
   }

   /// <summary>Displays the currently selected string in the manner appropriate to the editing mode.</summary>
   /// <exception cref="Logic::ArgumentNullException">Not initialized</exception>
   /// <exception cref="Logic::NotImplementedException">Mode is DISPLAY</exception>
   void  LanguageEdit::Refresh()
   {
      REQUIRED(Document);  // Ensure initialized

      // Disable when no string selected
      SetReadOnly(!Document->SelectedString ? TRUE : FALSE);

      // Nothing: Clear text
      if (!Document->SelectedString)
         Clear();

      // Editor: Display RichText
      else if (Mode == EditMode::Edit)
         SetRichText(Document->SelectedString->RichText);

      // Display: Format for display
      else if (Mode == EditMode::Display)
         throw NotImplementedException(HERE, L"Display mode");

      // Source: Display SourceText
      else if (Mode == EditMode::Source)
      {
         // Clear previous
         Clear();

         // Display/highlight sourceText
         SetWindowText(Document->SelectedString->Text.c_str());
         UpdateHighlighting();
      }
   }

   /// <summary>Changes the edit mode.</summary>
   /// <param name="m">mode.</param>
   /// <exception cref="Logic::ArgumentNullException">Not initialized</exception>
   /// <exception cref="Logic::NotImplementedException">Mode is DISPLAY</exception>
   void  LanguageEdit::SetEditMode(EditMode m)
   {
      REQUIRED(Document);  // Ensure initialized

      // Preserve edit mode
      auto prev = GetEditMode();

      try
      {
         // Skip if already set, or edit disabled
         if (m == prev || !Document->SelectedString)
            return;

         // Change mode + redisplay   
         Mode = m;
         Refresh();
      }
      // Error: Revert mode
      catch (ExceptionBase&) {
         Mode = prev;
         throw;
      }
   }

   /// <summary>Toggles the formatting.</summary>
   /// <param name="f">CHARFORMAT effect.</param>
   void  LanguageEdit::ToggleFormatting(DWORD fx)
   {
      CharFormat cf(fx, fx);
      GetSelectionCharFormat(cf);
      cf.dwEffects = (cf.dwEffects & fx ? cf.dwEffects & ~fx : cf.dwEffects | fx);
      SetSelectionCharFormat(cf);
   }

   // ------------------------------ PROTECTED METHODS -----------------------------
   
   /// <summary>Generates the source text.</summary>
   /// <returns></returns>
   wstring  LanguageEdit::GetSourceText()
   { 
      // TODO: Write author/title/text tags etc.

      // Get content
      wstring content = RichStringWriter(TextDocument, Document->SelectedString->TagType).Write();

      return content;
   }

   /// <summary>Highlights the match.</summary>
   /// <param name="pos">position.</param>
   /// <param name="length">length or -1 for entire text.</param>
   /// <param name="cf">formatting.</param>
   void  LanguageEdit::HighlightMatch(UINT pos, UINT length, CharFormat& cf)
   {
      SetSel(pos, (length != -1 ? pos + length : -1));
      SetSelectionCharFormat(cf);
   }

   /// <summary>Destroys the button data stored with a button image</summary>
   /// <param name="obj">The object.</param>
   void  LanguageEdit::OnButtonRemoved(IOleObjectPtr obj)
   {
      try
      {
         REQUIRED(obj);

         // Linear search for matching object
         for (int i = 0; i < OleDocument->GetObjectCount(); ++i)
         {
            ReObject reObject;
            
            // Get IOleObject
            OleDocument->GetObjectW(i, &reObject, REO_GETOBJ_POLEOBJ);     // BUG: Returns failure but actually succeeds
            IOleObjectPtr obj2(reObject.poleobj, false);

            // Match: Delete button data
            if (obj == obj2)
            {
               // Ensure data exists
               if (!reObject.dwUser)
                  throw AlgorithmException(HERE, L"Retrieved OLE object has no button data");

               // DEBUG:
               //Console << "OnButtonRemoved: Destroying button=" << reinterpret_cast<ButtonData*>(reObject.dwUser)->Text << ENDL;

               // Delete button data
               delete reinterpret_cast<ButtonData*>(reObject.dwUser);
               return;
            }
         }

         // Error: Not found
         throw AlgorithmException(HERE, L"Unable to destroy button data");
      }
      catch (_com_error& e) {
         Console.Log(HERE, ComException(HERE, e));
      }
      catch (ExceptionBase& e) {
         Console.Log(HERE, e);
      }
   }

   /// <summary>Changes the edit mode.</summary>
   /// <param name="nID">The command identifier.</param>
   /*void LanguageEdit::OnCommandChangeMode(UINT nID)
   {
      AfxMessageBox(L"LanguageEdit::OnCommandChangeMode");

      try
      {
         switch (nID)
         {
         case ID_VIEW_SOURCE:    SetEditMode(LanguageEdit::EditMode::Source);   break;
         case ID_VIEW_EDITOR:    SetEditMode(LanguageEdit::EditMode::Edit);     break;
         case ID_VIEW_DISPLAY:   SetEditMode(LanguageEdit::EditMode::Display);  break;
         }
      }
      catch (ExceptionBase& e) { 
         theApp.ShowError(HERE, e, L"Unable to change editor mode");
      }
   }*/

   /// <summary>Perform text formatting command.</summary>
   /// <param name="nID">The command identifier.</param>
   //void LanguageEdit::OnCommandChangeText(UINT nID)
   //{
   //   static int LastButtonID = 1;  

   //   AfxMessageBox(L"LanguageEdit::OnCommandChangeText");

   //   // Execute
   //   switch (nID)
   //   {
   //   // Clipboard: Set based on selection
   //   case ID_EDIT_CUT:     Cut();     break;
   //   case ID_EDIT_COPY:    Copy();    break;
   //   case ID_EDIT_PASTE:   PasteFormat(CF_UNICODETEXT);   break;
   //   case ID_EDIT_CLEAR:   ReplaceSel(L"", TRUE);         break;

   //   // Undo/Redo:
   //   case ID_EDIT_UNDO:    Undo();    break;
   //   case ID_EDIT_REDO:    Redo();    break;

   //   // Button: Insert at caret
   //   case ID_EDIT_ADD_BUTTON:
   //      InsertButton(GuiString(L"Button %d", LastButtonID++), L"id");
   //      break;

   //   // Colour: Not implemented
   //   case ID_EDIT_COLOUR:
   //      break;

   //   // Format: Require editor mode
   //   case ID_EDIT_BOLD:       ToggleFormatting(CFE_BOLD);       break;
   //   case ID_EDIT_ITALIC:     ToggleFormatting(CFE_ITALIC);     break;
   //   case ID_EDIT_UNDERLINE:  ToggleFormatting(CFE_UNDERLINE);  break;

   //   // Alignment: Require editor mode
   //   case ID_EDIT_LEFT:       SetParaFormat(ParaFormat(PFM_ALIGNMENT, Alignment::Left));     break;
   //   case ID_EDIT_RIGHT:      SetParaFormat(ParaFormat(PFM_ALIGNMENT, Alignment::Right));    break;
   //   case ID_EDIT_CENTRE:     SetParaFormat(ParaFormat(PFM_ALIGNMENT, Alignment::Centre));   break;
   //   case ID_EDIT_JUSTIFY:    SetParaFormat(ParaFormat(PFM_ALIGNMENT, Alignment::Justify));  break;
   //   }
   //}

   
   /// <summary>Queries the state of editing mode commands.</summary>
   /// <param name="pCmd">The command.</param>
   //void LanguageEdit::OnQueryModeCommand(CCmdUI* pCmd) 
   //{
   //   // Disable if no string displayed
   //   if (IsReadOnly())
   //   {
   //      pCmd->Enable(FALSE);
   //      pCmd->SetCheck(FALSE);
   //   }
   //   else
   //   {
   //      // Check correct state
   //      pCmd->Enable();
   //      switch (pCmd->m_nID)
   //      {
   //      case ID_VIEW_SOURCE:    pCmd->SetCheck(GetEditMode() == LanguageEdit::EditMode::Source);   break;
   //      case ID_VIEW_EDITOR:    pCmd->SetCheck(GetEditMode() == LanguageEdit::EditMode::Edit);     break;
   //      case ID_VIEW_DISPLAY:   pCmd->SetCheck(GetEditMode() == LanguageEdit::EditMode::Display);  break;
   //      }
   //   }
   //}

   /// <summary>Queries the state of clipboard commands.</summary>
   /// <param name="pCmd">The command.</param>
   //void LanguageEdit::OnQueryClipboardCommand(CCmdUI* pCmd) 
   //{
   //   // Disable if no string displayed
   //   if (IsReadOnly())
   //   {
   //      pCmd->Enable(FALSE);
   //      pCmd->SetCheck(FALSE);
   //   }
   //   else
   //   {
   //      // Check correct state
   //      pCmd->SetCheck(FALSE);
   //      switch (pCmd->m_nID)
   //      {
   //      // Clipboard: Set based on selection
   //      case ID_EDIT_CUT:
   //      case ID_EDIT_COPY:    
   //         return pCmd->Enable(HasSelection());

   //      // Paste/Delete: Always enabled
   //      case ID_EDIT_PASTE:   
   //      case ID_EDIT_CLEAR:   
   //         return pCmd->Enable(TRUE);

   //      // Undo:
   //      case ID_EDIT_UNDO:
   //         pCmd->Enable(CanUndo());
   //         pCmd->SetText( GuiString(CanUndo() ? L"Undo %s" : L"Undo", GetString(GetUndoName())).c_str() );
   //         break;
   //      // Redo:
   //      case ID_EDIT_REDO:
   //         pCmd->Enable(CanRedo());
   //         pCmd->SetText( GuiString(CanRedo() ? L"Redo %s" : L"Redo", GetString(GetRedoName())).c_str() );
   //         break;
   //      }
   //   }
   //}

   /// <summary>Queries the state of formatting commands.</summary>
   /// <param name="pCmd">The command.</param>
   //void LanguageEdit::OnQueryFormatCommand(CCmdUI* pCmd) 
   //{
   //   // Disable if no string displayed
   //   if (IsReadOnly())
   //   {
   //      pCmd->Enable(FALSE);
   //      pCmd->SetCheck(FALSE);
   //   }
   //   else
   //   {
   //      CharFormat  cf(CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE, 0);
   //      ParaFormat  pf(PFM_ALIGNMENT);

   //      // Check correct state
   //      pCmd->SetCheck(FALSE);
   //      switch (pCmd->m_nID)
   //      {
   //      // Button: Require editor mode
   //      case ID_EDIT_ADD_BUTTON:
   //         pCmd->Enable(GetEditMode() == LanguageEdit::EditMode::Edit);
   //         break;

   //      // Colour: Not implemented
   //      case ID_EDIT_COLOUR:
   //         pCmd->Enable(FALSE);
   //         break;

   //      // Format: Require editor mode
   //      case ID_EDIT_BOLD:       
   //      case ID_EDIT_ITALIC:     
   //      case ID_EDIT_UNDERLINE:  
   //         if (GetEditMode() == LanguageEdit::EditMode::Edit)
   //         {
   //            pCmd->Enable(TRUE);
   //            GetSelectionCharFormat(cf);
   //         
   //            // Check formatting 
   //            switch (pCmd->m_nID)
   //            {
   //            case ID_EDIT_BOLD:       return pCmd->SetCheck(cf.dwEffects & CFE_BOLD ? TRUE : FALSE);
   //            case ID_EDIT_ITALIC:     return pCmd->SetCheck(cf.dwEffects & CFE_ITALIC ? TRUE : FALSE);
   //            case ID_EDIT_UNDERLINE:  return pCmd->SetCheck(cf.dwEffects & CFE_UNDERLINE ? TRUE : FALSE);
   //            }
   //         }
   //         break;

   //      // Alignment: Require editor mode
   //      case ID_EDIT_LEFT:
   //      case ID_EDIT_RIGHT:
   //      case ID_EDIT_CENTRE:
   //      case ID_EDIT_JUSTIFY:
   //         if (GetEditMode() == LanguageEdit::EditMode::Edit)
   //         {
   //            pCmd->Enable(TRUE);
   //            GetParaFormat(pf);
   //         
   //            // Check alignment
   //            switch (pCmd->m_nID)
   //            {
   //            case ID_EDIT_LEFT:     return pCmd->SetCheck(pf.wAlignment == PFA_LEFT    ? TRUE : FALSE);
   //            case ID_EDIT_RIGHT:    return pCmd->SetCheck(pf.wAlignment == PFA_RIGHT   ? TRUE : FALSE);
   //            case ID_EDIT_CENTRE:   return pCmd->SetCheck(pf.wAlignment == PFA_CENTER  ? TRUE : FALSE);
   //            case ID_EDIT_JUSTIFY:  return pCmd->SetCheck(pf.wAlignment == PFA_JUSTIFY ? TRUE : FALSE);
   //            }
   //         }
   //         break;
   //      }
   //   }
   //}


   /// <summary>Supply tooltip data</summary>
   /// <param name="data">The data.</param>
   void LanguageEdit::OnRequestTooltip(CustomTooltip::TooltipData* data)
   {
      // None: show nothing
      *data = CustomTooltip::NoTooltip;
   }

   /// <summary>Saves the current text into the currently selected string</summary>
   void LanguageEdit::OnTextChange()
   {
      try
      {
         // Ensure selection exists
         REQUIRED(Document->SelectedString);

         // Save contents
         switch (GetEditMode())
         {
         // Source: Save verbatim + highlight
         case EditMode::Source:
            Document->SelectedString->Text = GetAllText();
            UpdateHighlighting();
            break;

         // Edit: Generate source + save
         case EditMode::Edit:
            //String->Text = GetSourceText();

            // DEBUG:
            Console << "TextChanged=" << GetSourceText() << ENDL;
            break;
         }
      }
      catch (ExceptionBase& e) {
         Console.Log(HERE, e);
      }

      // Reset tootlip
      __super::OnTextChange();
   }

   /// <summary>Replace edit contents with rich text.</summary>
   /// <param name="richStr">The string.</param>
   void  LanguageEdit::SetRichText(const RichString& richStr)
   {
      // Freeze
      FreezeWindow(true);
      SuspendUndo(true);

      // Clear contents
      SetWindowText(L"");
      SetDefaultCharFormat(DefaultCharFormat());
      
      // Assemble characters into phrases of contiguous formatting separated by buttons, insert them, then align paragraph.
      for (const RichParagraph& para : richStr.Paragraphs)
      {
         RichPhrase phrase;

         // Paragraph start index
         auto paraBegin = GetSelection().cpMin;

         // Insert text/buttons
         for (const RichElementPtr& element : para.Content)
         {
            // Character: Extend phrase
            if (element->Type == ElementType::Character)
            {
               RichCharacter* chr = reinterpret_cast<RichCharacter*>(element.get());

               // Start phrase
               if (phrase.Empty())
                  phrase = RichPhrase(*chr);
               // Extend phrase
               else if (phrase.Format == chr->Format && phrase.Colour == chr->Colour)
                  phrase += chr->Char;
               else
               {  // Commit phrase
                  InsertPhrase(phrase);
                  phrase = RichPhrase(*chr);
               }
            }
            // Button:
            else 
            {
               auto btn = reinterpret_cast<RichButton*>(element.get());

               // Commit phrase (if any)
               InsertPhrase(phrase);
               phrase.Clear();

               // DEBUG:
               Console << "Inserting Button=" << btn->Text << ENDL;

               // Insert button
               auto pos = GetSelection().cpMax;
               InsertButton(btn->Text, btn->ID);
               SetSel(pos+1, pos+1);
            }
         }

         // Commit final phrase
         InsertPhrase(phrase);
         
         // Align paragraph
         auto paraEnd = GetSelection().cpMax;
         SetSel(paraBegin, paraEnd);
         SetParaFormat(ParaFormat(PFM_ALIGNMENT, para.Align));
         SetSel(paraEnd, paraEnd);
      }
      
      // Clear selection
      SetSel(0,0);

      // Unfreeze + Clear Undo
      SuspendUndo(false);
      EmptyUndoBuffer();
      FreezeWindow(false);
   }

   /// <summary>Inserts a phrase at the caret and formats it.</summary>
   /// <param name="phr">phrase.</param>
   void LanguageEdit::InsertPhrase(const RichPhrase& phr)
   {
      if (phr.Empty())
         return;

      // Insert phrase
      SetSelectionCharFormat(CharFormat(CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_COLOR, phr.Format, phr.GetColour(RenderFlags::RichText)));
      ReplaceSel(phr.Text.c_str());

      // Move caret to end-of-phrase
      auto end = GetSelection().cpMax;
      SetSel(end, end);
   }
   
   /// <summary>Performs syntax highlighting when in Source mode</summary>
   void  LanguageEdit::UpdateHighlighting()
   {
      static const COLORREF Tag = RGB(255,174,207),         // Pale Pink
                            Property = RGB(255,174,89),     // Pale Orange
                            SubString = RGB(152,220,235),   // Pale Blue
                            Variable = RGB(152,235,156);    // Pale Green

      // Freeze
      SuspendUndo(true);
      FreezeWindow(true);

      try
      {
         CharFormat cf(CFM_COLOR | CFM_UNDERLINE | CFM_UNDERLINETYPE, NULL);
         auto text = GetAllText();

         // Clear formatting
         cf.crTextColor = RGB(255,255,255);
         HighlightMatch(0, -1, cf);

         // Highlight simple opening/closing tags
         cf.crTextColor = Tag;
         for (wsregex_iterator it(text.cbegin(), text.cend(), MatchSimpleTag), end; it != end; ++it)
            HighlightMatch(it->position(), it->length(), cf);

         // Highlight tags with properties
         for (wsregex_iterator it(text.cbegin(), text.cend(), MatchComplexTag), end; it != end; ++it)
         {
            // Highlight entire tag
            cf.crTextColor = Tag;
            HighlightMatch(it->position(0), it->length(0), cf);

            // Highlight up to four {properties,value} pairs
            cf.crTextColor = Property;
            for (UINT i = 2; (i <= 8) && (i < it->max_size()) && (it->_At(i).matched); i+=2)
               HighlightMatch(it->position(i), it->length(i), cf);
         }

         // Highlight colour codes
         cf.crTextColor = Tag;
         for (wsregex_iterator it(text.cbegin(), text.cend(), MatchColourCode), end; it != end; ++it)
            HighlightMatch(it->position(), it->length(), cf);

         // Highlight substrings
         cf.crTextColor = SubString;
         for (wsregex_iterator it(text.cbegin(), text.cend(), MatchSubString), end; it != end; ++it)
            HighlightMatch(it->position(), it->length(), cf);

         // Highlight variables
         cf.crTextColor = Variable;
         for (wsregex_iterator it(text.cbegin(), text.cend(), MatchVariable), end; it != end; ++it)
            HighlightMatch(it->position(), it->length(), cf);
      }
      catch (regex_error& e) {
         Console.Log(HERE, RegularExpressionException(HERE, e));
      }

      // UnFreeze
      FreezeWindow(false);
      SuspendUndo(false);
   }


   // ------------------------------- PRIVATE METHODS ------------------------------
   


   
NAMESPACE_END2(GUI,Controls)



