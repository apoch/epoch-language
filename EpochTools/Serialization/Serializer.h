//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Declarations for bytecode serialization subsystem
//

#pragma once


namespace Serialization
{

	class Serializer
	{
	// Construction
	public:
		Serializer(const void* bytecode, size_t size);

	// Non-copyable
	private:
		Serializer(const Serializer& rhs);
		Serializer& operator = (const Serializer& rhs);

	// Writer interface
	public:
		void Write(const std::wstring& filename) const;

	// Internal tracking
	private:
		const void* BytecodeBuffer;
		const size_t BytecodeSize;
	};

}

