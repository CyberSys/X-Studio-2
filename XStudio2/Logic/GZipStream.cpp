#include "stdafx.h"
#include "GZipStream.h"

namespace Logic
{
   namespace IO
   {
      // -------------------------------- CONSTRUCTION --------------------------------

      /// <summary>Creates a GZip stream using another stream as input</summary>
      /// <param name="src">The input stream</param>
      /// <param name="op">Whether to compress or decompress</param>
      /// <exception cref="Logic::ArgumentException">Stream is not readable</exception>
      /// <exception cref="Logic::ArgumentNullException">Stream is null</exception>
      /// <exception cref="Logic::GZipException">Unable to inititalise stream</exception>
      GZipStream::GZipStream(StreamPtr  src, Operation  op) : StreamFacade(src), Mode(op)
      {
         // Clear structs
         ZeroMemory(&ZStream, sizeof(ZStream));
         ZeroMemory(&ZHeader, sizeof(ZHeader));

         if (Mode == Operation::Decompression)
         {
            // Ensure stream has read/write access
            if (!src->CanRead())
               throw ArgumentException(HERE, L"src", GuiString(ERR_NO_READ_ACCESS));

            // Init stream + extract header
            if (inflateInit2(&ZStream, WINDOW_SIZE+DETECT_HEADER) != Z_OK) 
               throw GZipException(HERE, ZStream.msg);

            // Allocate + set input buffer
            Buffer.reset(new byte[src->GetLength()]);
            ZStream.next_in = Buffer.get();
         }
         else
         {
            // Ensure stream has write access
            if (!src->CanWrite())
               throw ArgumentException(HERE, L"src", GuiString(ERR_NO_WRITE_ACCESS));

            // Init stream
            if (deflateInit2(&ZStream, Z_BEST_COMPRESSION, Z_DEFLATED, WINDOW_SIZE+DETECT_HEADER, 9, Z_DEFAULT_STRATEGY) != Z_OK)
               throw GZipException(HERE, ZStream.msg);

            // Init header
            ZHeader.name = (Bytef*)"filename";
            if (deflateSetHeader(&ZStream, &ZHeader) != Z_OK)
               throw GZipException(HERE, ZStream.msg);

            // Allocate + set output buffer
            Buffer.reset(new byte[COMPRESS_BUFFER]);
            ZStream.next_out = Buffer.get();
            ZStream.avail_out = COMPRESS_BUFFER;
         }
      }

      /// <summary>Closes the stream without throwing</summary>
      GZipStream::~GZipStream()
      {
         SafeClose();
      }

      // ------------------------------- PUBLIC METHODS -------------------------------
      
      /// <summary>Stream is not seekable.</summary>
      /// <returns></returns>
      bool  GZipStream::CanSeek() const
      { 
         return false; 
      }

      /// <summary>Closes the stream.</summary>
      /// <exception cref="Logic::GZipException">Unable to close stream</exception>
      /// <exception cref="Logic::IOException">An I/O error occurred</exception>
      void  GZipStream::Close()
      {
         if (!IsClosed())
         {
            // Decompression
            if (Mode == Operation::Decompression && inflateEnd(&ZStream) != Z_OK)
               throw GZipException(HERE, ZStream.msg);
            
            // Compression
            else if (Mode == Operation::Compression && deflateEnd(&ZStream) != Z_OK)
               throw GZipException(HERE, ZStream.msg);

            StreamFacade::Close();
         }
      }

      /// <summary>Gets the uncompressed stream length.</summary>
      /// <returns></returns>
      /// <exception cref="Logic::IOException">An I/O error occurred</exception>
      /// <exception cref="Logic::NotSupportedException">Mode is compression</exception>
      DWORD  GZipStream::GetLength()
      {
         DWORD pos = StreamFacade::GetPosition(),
               size = 0;

         // Compress: Not supported
         if (Mode == Operation::Compression)
            throw NotSupportedException(HERE, L"Cannot get uncompressed length when compressing");

         // Decompress: Extract uncompressed length from last four bytes
         StreamFacade::Seek(-4, SeekOrigin::End);
         StreamFacade::Read((byte*)&size, 4);
         StreamFacade::Seek(pos, SeekOrigin::Begin);
         return size;
      }

      /// <summary>Gets the current position.</summary>
      /// <returns></returns>
      DWORD  GZipStream::GetPosition() const
      {
         return ZStream.total_out;
      }

      /// <summary>Closes the stream without throwing.</summary>
      void  GZipStream::SafeClose()
      {
         if (!IsClosed())
         {
            // Close ZLib stream
            if (Mode == Operation::Decompression)
               inflateEnd(&ZStream);
            else
               deflateEnd(&ZStream);

            // Close underlying stream
            StreamFacade::SafeClose();
         }
      }

      /// <summary>Not supported</summary>
      /// <param name="offset">The offset.</param>
      /// <param name="mode">The mode.</param>
      /// <exception cref="Logic::NotSupportedException">Always</exception>
      void  GZipStream::Seek(LONG  offset, SeekOrigin  mode)
      {
         throw NotSupportedException(HERE, GuiString(ERR_NO_SEEK_ACCESS));
      }

      /// <summary>Not supported</summary>
      /// <param name="offset">The offset.</param>
      /// <param name="mode">The mode.</param>
      /// <exception cref="Logic::NotSupportedException">Always</exception>
      void  GZipStream::SetLength(DWORD  length)
      {
         throw NotSupportedException(HERE, L"Resizing not allowed");
      }

      /// <summary>Reads/decompresses from the stream into the specified buffer.</summary>
      /// <param name="buffer">The destination buffer</param>
      /// <param name="length">The length of the buffer</param>
      /// <returns>Number of bytes read</returns>
      /// <exception cref="Logic::ArgumentNullException">Buffer is null</exception>
      /// <exception cref="Logic::NotSupportedException">Input stream is not readable</exception>
      /// <exception cref="Logic::GZipException">Unable to decompress data</exception>
      /// <exception cref="Logic::IOException">An I/O error occurred</exception>
      DWORD  GZipStream::Read(BYTE* output, DWORD length)
      {
         REQUIRED(output);

         // Ensure we can read
         if (!StreamFacade::CanRead())
            throw NotSupportedException(HERE, GuiString(ERR_NO_READ_ACCESS));

         // Supply output buffer
         ZStream.next_out = output;
         ZStream.avail_out = length;

         // Re-Fill input buffer if necessary
         if (ZStream.avail_in == 0)
            ZStream.avail_in = StreamFacade::Read(ZStream.next_in, StreamFacade::GetLength());
         
         // Decompress
         switch (int res = inflate(&ZStream, Z_FINISH))
         {
         // Success/EOF: Return count decompressed
         case Z_STREAM_END:
         case Z_BUF_ERROR:
         case Z_OK:
            return length - ZStream.avail_out;

         // Error: throw
         default:
            throw GZipException(HERE, ZStream.msg);
         }
      }

      /// <summary>Writes/compresses the specified buffer to the stream</summary>
      /// <param name="buffer">The buffer.</param>
      /// <param name="length">The length of the buffer.</param>
      /// <returns>Number of bytes written</returns>
      /// <exception cref="Logic::NotImplementedException">Always</exception>
      DWORD  GZipStream::Write(const BYTE* input, DWORD length)
      {
         REQUIRED(input);

         // Ensure we can write
         if (!StreamFacade::CanWrite())
            throw NotSupportedException(HERE, GuiString(ERR_NO_WRITE_ACCESS));

         // Supply input buffer
         ZStream.next_in = const_cast<BYTE*>(input);
         ZStream.avail_in = length;

         // Re-Fill output buffer if necessary
         /*if (ZStream.avail_out == 0)
            ZStream.avail_out = StreamFacade::Read(ZStream.next_in, StreamFacade::GetLength());*/
         
         // Compress all input into the output buffer
         switch (int res = deflate(&ZStream, Z_FINISH))
         {
         // Success: Write to underlying stream + Return count
         case Z_STREAM_END:
         /*case Z_BUF_ERROR:
         case Z_OK:*/
            return StreamFacade::Write(Buffer.get(), COMPRESS_BUFFER - ZStream.avail_out);

         // Error: throw
         default:
            throw GZipException(HERE, ZStream.msg);
         }
      }

      // ------------------------------ PROTECTED METHODS -----------------------------

		// ------------------------------- PRIVATE METHODS ------------------------------

      /// <summary>Determines whether the stream is closed.</summary>
      /// <returns></returns>
      bool   GZipStream::IsClosed() const
      {
         return ZStream.zalloc == Z_NULL;
      }

      // -------------------------------- NESTED CLASSES ------------------------------
   }
}
