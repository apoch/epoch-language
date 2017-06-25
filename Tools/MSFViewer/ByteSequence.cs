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
    }


    class TypedByteSequence<T> : ByteSequence
    {
        public TypedByteSequence(byte[] buffer, int readoffset, int size, T extractedvalue)
            : base(buffer, readoffset, size)
        {
            ExtractedValue = extractedvalue;
        }

        public T ExtractedValue;

        public override string ToString()
        {
            if (typeof(T) == typeof(string))
                return ExtractedValue as string;

            if (!typeof(T).IsPrimitive)
                return ExtractedValue.ToString();

            return MakeStringFromPrimitive(ExtractedValue);
        }

        private string MakeStringFromPrimitive(T v)
        {
            return $"{v} (0x{v:x})";
        }
    }
}
