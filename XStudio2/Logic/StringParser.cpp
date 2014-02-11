#include "stdafx.h"
#include "StringParser.h"
#include <algorithm>

namespace Logic
{
   namespace Language
   {
      /// <summary>Matches an opening tag (at beginning of string) and captures the name</summary>
      const wregex  StringParser::IsOpeningTag = wregex(L"^\\[([a-z]+)(?:\\s+[a-z]+\\s*=\\s*'\\w+')*\\]");

      /// <summary>Matches any closing tag (at beginning of string) and captures the name</summary>
      const wregex  StringParser::IsClosingTag = wregex(L"^\\[/?([a-z]+)\\]");
      
      /// <summary>Matches any opening/closing tag (at beginning of string) without properties and captures the name</summary>
      const wregex  StringParser::IsBasicTag = wregex(L"^\\[/?([a-z]+)\\]");

      /// <summary>Matches multiple tag properties captures both name and value</summary>
      const wregex  StringParser::IsTagProperty = wregex(L"\\s+([a-z]+)\\s*=\\s*'(\\w+)'");

      /// <summary>Matches opening and closing [author] tags (at beginning of string) and captures the text</summary>
      const wregex  StringParser::IsAuthorDefition = wregex(L"^[author](.*)[/author]");

      /// <summary>Matches opening and closing [title] tags (at beginning of string) and captures the text</summary>
      const wregex  StringParser::IsTitleDefition = wregex(L"^[title](.*)[/title]");
   
      // -------------------------------- CONSTRUCTION --------------------------------

      /// <summary>Creates a rich-text parser from an input string</summary>
      /// <param name="str">string containing rich-text tags</param>
      /// <exception cref="Logic::AlgorithmException">Error in parsing algorithm</exception>
      /// <exception cref="Logic::ArgumentException">Error in parsing algorithm</exception>
      /// <exception cref="Logic::Language::RichTextException">Error in formatting tags</exception>
      StringParser::StringParser(const wstring& str) : Input(str), Alignments(TagClass::Paragraph)
      {
         Parse();
      }


      StringParser::~StringParser()
      {
      }

      // ------------------------------- STATIC METHODS -------------------------------

      /// <summary>Gets the associated paragraph alignment of a paragraph tag</summary>
      /// <param name="t">tag type</param>
      /// <returns></returns>
      /// <exception cref="Logic::ArgumentException">Not a paragraph tag</exception>
      Alignment StringParser::GetAlignment(TagType t) 
      {
         switch (t)
         {
         case TagType::Left:     return Alignment::Left;
         case TagType::Right:    return Alignment::Right;
         case TagType::Centre:   return Alignment::Centre;
         case TagType::Justify:  return Alignment::Justify;
         }
         
         // Not paragraph tag
         throw ArgumentException(HERE, L"t", GuiString(L"Cannot get alignment for %s tag", GetString(t).c_str()));
      }

      /// <summary>Gets the class of a tag</summary>
      /// <param name="t">tag type</param>
      /// <returns></returns>
      /// <exception cref="Logic::ArgumentException">Tag is unrecognised</exception>
      StringParser::TagClass  StringParser::GetClass(TagType t)
      {
         switch (t)
         {
         case TagType::Bold:
         case TagType::Underline:
         case TagType::Italic:
            return TagClass::Character;

         case TagType::Left:
         case TagType::Right:
         case TagType::Centre:
         case TagType::Justify:
            return TagClass::Paragraph;

         case TagType::Text:
         case TagType::Select:
         case TagType::Author:
         case TagType::Title:
         case TagType::Rank:
            return TagClass::Special;

         case TagType::Black:
         case TagType::Blue:
         case TagType::Cyan:
         case TagType::Green:
         case TagType::Magenta:
         case TagType::Orange:
         case TagType::Red:
         case TagType::Silver:
         case TagType::Yellow:
         case TagType::White:
            return TagClass::Colour;
         }

         throw ArgumentException(HERE, L"t", L"Unrecognised tags have no class");
      }

      
      /// <summary>Convert tag class to string</summary>
      /// <param name="c">tag class.</param>
      /// <returns></returns>
      wstring  StringParser::GetString(TagClass c)
      {
         const wchar* str[] = { L"Character", L"Paragraph", L"Colour", L"Special" };

         return str[(UINT)c];
      }
      
      /// <summary>Convert tag type to string</summary>
      /// <param name="t">tag type.</param>
      /// <returns></returns>
      wstring  StringParser::GetString(TagType t)
      {
         const wchar* str[] = { L"Bold", L"Underline", L"Italic", L"Left", L"Right", L"Centre", L"Justify", L"Text", L"Select", L"Author", 
                                L"Title", L"Rank",L"Black", L"Blue", L"Cyan", L"Green", L"Magenta", L"Orange", L"Red", L"Silver", L"Yellow", 
                                L"White", L"Unrecognised"  };

         return str[(UINT)t];
      }

      /// <summary>Identifies a tag type from it's name</summary>
      /// <param name="name">tag name WITHOUT square brackets</param>
      /// <returns></returns>
      StringParser::TagType  StringParser::IdentifyTag(const wstring& name)
      {
         switch (name.length())
         {
         case 1:
            if (name == L"b")
               return TagType::Bold;
            else if (name == L"i")
               return TagType::Italic;
            else if (name == L"u")
               return TagType::Underline;
            break;

         case 3:
            if (name == L"red")
               return TagType::Red;
            break;

         case 4:
            if (name == L"left")
               return TagType::Left;
            else if (name == L"blue")
               return TagType::Blue;
            else if (name == L"cyan")
               return TagType::Cyan;
            else if (name == L"text")
               return TagType::Text;
            else if (name == L"rank")
               return TagType::Rank;
            break;

         case 5:
            if (name == L"right")
               return TagType::Right;
            else if (name == L"title")
               return TagType::Title;
            else if (name == L"green")
               return TagType::Green;
            else if (name == L"black")
               return TagType::Black;
            else if (name == L"white")
               return TagType::White;
            break;

         case 6:
            if (name == L"center")
               return TagType::Centre;
            else if (name == L"select")
               return TagType::Select;
            else if (name == L"author")
               return TagType::Author;
            else if (name == L"orange")
               return TagType::Orange;
            else if (name == L"yellow")
               return TagType::Yellow;
            else if (name == L"silver")
               return TagType::Silver;
            break;

         case 7:
            if (name == L"magenta")
               return TagType::Magenta;
            else if (name == L"justify")
               return TagType::Justify;
            break;
         }

         // Unrecognised;
         return TagType::Unrecognised;
      }

      // ------------------------------- PUBLIC METHODS -------------------------------

      /// <summary>Parses input string</summary>
      /// <exception cref="Logic::AlgorithmException">Error in parsing algorithm</exception>
      /// <exception cref="Logic::ArgumentException">Error in parsing algorithm</exception>
      /// <exception cref="Logic::Language::RichTextException">Error in formatting tags</exception>
      void  StringParser::Parse()
      {
         RichParagraph* Paragraph;
         Colour TextColour = Colour::Default;
         bool   Escaped = false;

         // Iterate thru input
         for (auto ch = Input.begin(); ch != Input.end(); ++ch)
         {
            switch (*ch)
            {
            // Escape/ColourCode: 
            case '\\': 
               // ColourCode: Read and override existing stack colour, if any
               if (MatchColourCode(ch))
                  TextColour = ReadColourCode(ch);
               else
               {  // Backslash: Append as text, escape next character
                  Escaped = true;
                  *Paragraph += new RichChar('\\', TextColour, Formatting.Current);
               }
               continue;

            // Open/Close tag:
            case '[':
               // Escaped/Not-a-tag: Append as text
               if (Escaped || !MatchTag(ch))
                  *Paragraph += new RichChar('[', TextColour, Formatting.Current);
               else 
               {
                  // RichTag: Read entire tag. Adjust colour/formatting/paragraph
                  RichTag t = ReadTag(ch);
                  switch (t.Class)
                  {
                  // Paragraph: Add/Remove alignment. Append new paragraph?
                  case TagClass::Paragraph:
                     Alignments.PushPop(t);
                     if (t.Opening)
                     {
                        Output += RichParagraph(GetAlignment(t.Type));
                        Paragraph = &Output.Paragraphs.back();
                     }
                     break;

                  // Formatting: Add/Remove current formatting
                  case TagClass::Character:
                     Formatting.PushPop(t);
                     break;

                  // Colour: Add/Remove current colour. Reset any colourCode override
                  case TagClass::Colour:
                     Colours.PushPop(t);
                     TextColour = Colours.Current;
                     break;
                  }
               }
               break;

            // Char: Append to current paragraph
            default:
               *Paragraph += new RichChar(*ch, TextColour, Formatting.Current);
               break;
            }

            // Revert to de-escaped
            Escaped = false;
         }
      }

      // ------------------------------ PROTECTED METHODS -----------------------------

      // ------------------------------- PRIVATE METHODS ------------------------------
   
      /// <summary>Matches a unix style colour code.</summary>
      /// <param name="pos">position of backslash</param>
      /// <returns></returns>
      bool  StringParser::MatchColourCode(CharIterator pos) const
      {
         // Check for EOF?
         if (pos+5 >= Input.end())
            return false;

         // Match '\\033'
         if (pos[0] != '\\' || pos[1] != '0' || pos[2] != '3' || pos[3] != '3')
            return false;

         // Match colour character
         switch (pos[4])
         {
         case 'A':   // Silver
         case 'B':   // Blue
         case 'C':   // Cyan
         case 'G':   // Green
         case 'O':   // Orange
         case 'M':   // Magenta
         case 'R':   // Red
         case 'W':   // White
         case 'X':   // Default
         case 'Y':   // Yellow
         case 'Z':   // Black
            return true;
         }

         // Failed
         return false;
      }

      /// <summary>Matches any opening or closing tag</summary>
      /// <param name="pos">position of opening bracket</param>
      /// <returns></returns>
      bool  StringParser::MatchTag(CharIterator pos) const
      {
         wsmatch match;

         // Match tag using RegEx
         if (regex_search(pos, Input.end(), match, IsOpeningTag) || regex_search(pos, Input.end(), match, IsClosingTag))
            // Ignore unrecognised tags
            return IdentifyTag(match[1].str()) != TagType::Unrecognised;

         // Not a tag
         return false;
      }


      /// <summary>Reads the unix style colour code and advances the iterator</summary>
      /// <param name="pos">position of backslash</param>
      /// <returns></returns>
      /// <exception cref="Logic::AlgorithmException">Previously matched colour code is unrecognised</exception>
      Colour  StringParser::ReadColourCode(CharIterator& pos)
      {
         Colour c;

         // Match colour character
         switch (*pos)
         {
         case 'A':   c = Colour::Silver;  break;
         case 'B':   c = Colour::Blue;    break;
         case 'C':   c = Colour::Cyan;    break;
         case 'G':   c = Colour::Green;   break;
         case 'O':   c = Colour::Orange;  break;
         case 'M':   c = Colour::Purple;  break;
         case 'R':   c = Colour::Red;     break;
         case 'W':   c = Colour::White;   break;
         case 'X':   c = Colour::Default; break;
         case 'Y':   c = Colour::Yellow;  break;
         case 'Z':   c = Colour::Black;   break;
         default:
            throw AlgorithmException(HERE, GuiString(L"Previously matched colour code '%c' is unrecognised", *pos));
         }

         // Consume + return colour
         pos += 5;
         return c;
      }

      /// <summary>Reads the entire tag and advances the iterator</summary>
      /// <param name="pos">position of opening bracket</param>
      /// <returns></returns>
      /// <exception cref="Logic::Language::RichTextException">Closing tag doesn't match currently open tag</exception>
      StringParser::RichTag  StringParser::ReadTag(CharIterator& pos)
      {
         wsmatch matches;

         // BASIC: Open/Close Tag without properties
         if (regex_search(pos, Input.cend(), matches, IsBasicTag))
         {
            // Identify open/close
            bool opening = (pos[1] != '\\');
            wstring name = matches[1].str();
            
            // Identify type
            switch (TagType type = IdentifyTag(name))
            {
            // Author: Return author
            case TagType::Author:
               // Match [author](text)[/author]
               if (!regex_search(pos, Input.cend(), matches, IsAuthorDefition))
                  throw RichTextException(HERE, L"Invalid author tag");

               // Advance iterator.  Return author text
               pos += matches[0].length();
               return RichTag(TagType::Author, matches[1].str());

            // Title: Return title
            case TagType::Title:
               // Match [title](text)[/title]
               if (!regex_search(pos, Input.cend(), matches, IsTitleDefition))
                  throw RichTextException(HERE, L"Invalid title tag");

               // Advance iterator.  Return title text
               pos += matches[0].length();
               return RichTag(TagType::Title, matches[1].str());

            // Default: Advance iterator to ']' + return
            default:
               pos += matches[0].length();
               return RichTag(type, opening);
            }
         }
         // COMPLEX: Open tag with properties
         else
         {
            PropertyList props;

            // Match tag name, identify start/end of properties
            regex_search(pos, Input.cend(), matches, IsOpeningTag);

            // Match properties
            for (wsregex_iterator it(pos+matches[1].length(), pos+matches[0].length(), IsTagProperty), eof; it != eof; ++it)
               // Extract {name,value} from 2st/3rd match
               props.push_back( Property((*it)[1].str(), (*it)[2].str()) );
            
            // Advance Iterator + Return tag
            pos += matches[0].length();
            return RichTag(IdentifyTag(matches[1].str()), props);
         }
      }

   }
}
