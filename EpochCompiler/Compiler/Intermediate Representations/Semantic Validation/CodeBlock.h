//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a code block
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"


namespace IRSemantics
{

	// Forward declarations
	class Assignment;
	class Statement;
	class PreOpStatement;
	class PostOpStatement;
	class Entity;


	class CodeBlockEntry
	{
	// Destruction
	public:
		virtual ~CodeBlockEntry()
		{ }
	};

	class CodeBlock
	{
	// Construction and destruction
	public:
		CodeBlock()
		{ }

		~CodeBlock();

	// Non-copyable
	private:
		CodeBlock(const CodeBlock& other);
		CodeBlock& operator = (const CodeBlock& rhs);

	// Entry management
	public:
		void AddEntry(CodeBlockEntry* entry);

		const std::vector<CodeBlockEntry*>& GetEntries() const
		{ return Entries; }

	// Lexical scopes
	public:
		VM::EpochTypeID GetVariableTypeByID(StringHandle identifier) const;

	// Internal state
	private:
		std::vector<CodeBlockEntry*> Entries;
	};


	class CodeBlockAssignmentEntry : public CodeBlockEntry
	{
	// Construction and destruction
	public:
		explicit CodeBlockAssignmentEntry(Assignment* assignment);
		virtual ~CodeBlockAssignmentEntry();

	// Non-copyable
	private:
		CodeBlockAssignmentEntry(const CodeBlockAssignmentEntry& other);
		CodeBlockAssignmentEntry& operator = (const CodeBlockAssignmentEntry& rhs);

	// Property access
	public:
		const Assignment& GetAssignment() const
		{ return *MyAssignment; }

	// Internal state
	private:
		Assignment* MyAssignment;
	};

	class CodeBlockStatementEntry : public CodeBlockEntry
	{
	// Construction and destruction
	public:
		explicit CodeBlockStatementEntry(Statement* statement);
		virtual ~CodeBlockStatementEntry();

	// Non-copyable
	private:
		CodeBlockStatementEntry(const CodeBlockStatementEntry& other);
		CodeBlockStatementEntry& operator = (const CodeBlockStatementEntry& rhs);

	// Statement access
	public:
		const Statement& GetStatement() const
		{ return *MyStatement; }

	// Internal state
	private:
		Statement* MyStatement;
	};

	class CodeBlockPreOpStatementEntry : public CodeBlockEntry
	{
	// Construction and destruction
	public:
		explicit CodeBlockPreOpStatementEntry(PreOpStatement* statement);
		virtual ~CodeBlockPreOpStatementEntry();

	// Non-copyable
	private:
		CodeBlockPreOpStatementEntry(const CodeBlockPreOpStatementEntry& other);
		CodeBlockPreOpStatementEntry& operator = (const CodeBlockPreOpStatementEntry& rhs);

	// Statement access
	public:
		const PreOpStatement& GetStatement() const
		{ return *MyStatement; }

	// Internal state
	private:
		PreOpStatement* MyStatement;
	};

	class CodeBlockPostOpStatementEntry : public CodeBlockEntry
	{
	// Construction and destruction
	public:
		explicit CodeBlockPostOpStatementEntry(PostOpStatement* statement);
		virtual ~CodeBlockPostOpStatementEntry();

	// Non-copyable
	private:
		CodeBlockPostOpStatementEntry(const CodeBlockPostOpStatementEntry& other);
		CodeBlockPostOpStatementEntry& operator = (const CodeBlockPostOpStatementEntry& rhs);

	// Statement access
	public:
		const PostOpStatement& GetStatement() const
		{ return *MyStatement; }

	// Internal state
	private:
		PostOpStatement* MyStatement;
	};

	class CodeBlockInnerBlockEntry : public CodeBlockEntry
	{
	// Construction and destruction
	public:
		explicit CodeBlockInnerBlockEntry(CodeBlock* block);
		virtual ~CodeBlockInnerBlockEntry();

	// Non-copyable
	private:
		CodeBlockInnerBlockEntry(const CodeBlockInnerBlockEntry& other);
		CodeBlockInnerBlockEntry& operator = (const CodeBlockInnerBlockEntry& rhs);

	// Internal code access
	public:
		const CodeBlock& GetCode() const
		{ return *MyCodeBlock; }

	// Internal state
	private:
		CodeBlock* MyCodeBlock;
	};

	class CodeBlockEntityEntry : public CodeBlockEntry
	{
	// Construction and destruction
	public:
		explicit CodeBlockEntityEntry(Entity* entity);
		virtual ~CodeBlockEntityEntry();

	// Non-copyable
	private:
		CodeBlockEntityEntry(const CodeBlockEntityEntry& other);
		CodeBlockEntityEntry& operator = (const CodeBlockEntityEntry& rhs);

	// Accessors
	public:
		Entity& GetEntity() const
		{ return *MyEntity; }

	// Internal state
	private:
		Entity* MyEntity;
	};

}

