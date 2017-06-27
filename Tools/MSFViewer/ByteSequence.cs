using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MSFViewer
{
    class ByteSequence
    {
        public ByteSequence(byte[] completebuffer)
        {
            CachedResponse = completebuffer;
        }

        public ByteSequence(byte[] buffer, int readoffset, int size)
        {
            RawBuffer = buffer;
            ReadOffset = readoffset;
            Size = size;
        }

        private byte[] RawBuffer;
        private int ReadOffset;
        private int Size;

        private byte[] CachedResponse;

        public byte[] GetRawBytes()
        {
            if (CachedResponse != null)
                return CachedResponse;

            CachedResponse = RawBuffer.Skip(ReadOffset).Take(Size).ToArray();
            RawBuffer = null;

            return CachedResponse;
        }

        public bool HasData()
        {
            if (CachedResponse != null)
                return true;

            if ((RawBuffer != null) && (Size > 0))
                return true;

            return false;
        }
    }


    class MaskedByteSequence : ByteSequence
    {
        public MaskedByteSequence(byte[] buffer, int readoffset, int size, string mask)
            : base(buffer, readoffset, size)
        {
            Mask = mask;
        }

        protected string Mask;

        public override string ToString()
        {
            return Mask;
        }
    }


    class TypedByteSequence<T> : MaskedByteSequence
    {
        public TypedByteSequence(byte[] buffer, int readoffset, int size, T extractedvalue)
            : base(buffer, readoffset, size, null)
        {
            ExtractedValue = extractedvalue;
            Mask = MakeString();
        }

        public T ExtractedValue;

        private string MakeString()
        {
            if (typeof(T) == typeof(string))
                return ExtractedValue as string;

            if (!typeof(T).IsPrimitive)
                return ExtractedValue.ToString();

            return $"{ExtractedValue} (0x{ExtractedValue:x})";
        }
    }
}
