//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class for handling user output.
//


namespace UI
{
	//
	// This class is designed as essentially a drop-in replacement for std::wcout.
	// Since the actual method of writing the messages in the buffer is controlled
	// by OutputMessage, the class can easily be repurposed to work with a GUI or
	// even a network connection rather than a standard console.
	//
	class OutputStream
	{
	// Destruction
	public:
		//
		// Ensure any remaining messages are dumped when going out of scope
		//
		~OutputStream()
		{
			Flush();
		}

	// Stream operations
	public:
		//
		// Flush the buffer to the final output destination
		//
		OutputStream& Flush()
		{
			std::wstring outstr = Stream.str();
			if(!outstr.empty())
			{
				OutputMessage(outstr);
				Stream.str(L"");
			}
			return *this;
		}


	// Stream output operations
	public:
		//
		// Collect a value in the output buffer
		//
		template<typename T>
		OutputStream& operator << (const T& val)
		{
			Stream << val;
			return *this;
		}

		//
		// Perform a standard operation on the output buffer stream.
		// This provides support for controls like std::endl.
		//
		OutputStream& operator << (std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& (__cdecl *ptr)(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >&))
		{
			(*ptr)(Stream);
			return Flush();
		}


	// Internal storage
	private:
		std::wostringstream Stream;
	};
}