//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a code block
//

#pragma once


// Dependencies
#include "Compiler/ExportDef.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include "Metadata/ScopeDescription.h"


class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Assignment;
	class Statement;
	class PreOpStatement;
	class PostOpStatement;
	class CodeBlock;
	class Entity;
	class Program;
	struct InferenceContext;


	class CodeBlockEntry
	{
	// Destruction
	public:
		virtual ~CodeBlockEntry()
		{ }

	// Validation
	public:
		virtual bool Validate(const Program& program) const = 0;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(Program&, CodeBlock&, CompileErrors&)
		{ return true; }

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) = 0;
	};

	class CodeBlock
	{
	// Construction and destruction
	public:
		explicit CodeBlock(ScopeDescription* scope, bool ownsscope = true);
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
		EPOCHCOMPILER void AddVariable(const std::wstring& identifier, StringHandle identifierhandle, VM::EpochTypeID type, bool isreference, VariableOrigin origin);
		VM::EpochTypeID GetVariableTypeByID(StringHandle identifier) const;

		ScopeDescription* GetScope()
		{ return Scope; }

		const ScopeDescription* GetScope() const
		{ return Scope; }

	// Validation
	public:
		bool Validate(const IRSemantics::Program& program) const;

	// Compile time code execution
	public:
		bool CompileTimeCodeExecution(IRSemantics::Program& program, CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(IRSemantics::Program& program, InferenceContext& context, CompileErrors& errors);

	// Internal state
	private:
		std::vector<CodeBlockEntry*> Entries;
		ScopeDescription* Scope;
		bool OwnsScope;
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

	// Validation
	public:
		virtual bool Validate(const IRSemantics::Program& program) const;

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

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

	// Validation
	public:
		virtual bool Validate(const IRSemantics::Program& program) const;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(IRSemantics::Program& program, CodeBlock& activescope, CompileErrors& errors);

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

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

	// Validation
	public:
		virtual bool Validate(const IRSemantics::Program& program) const;

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

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

	// Validation
	public:
		virtual bool Validate(const IRSemantics::Program& program) const;

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

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

	// Validation
	public:
		virtual bool Validate(const IRSemantics::Program& program) const;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(IRSemantics::Program& program, CodeBlock& activescope, CompileErrors& errors);

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

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

	// Validation
	public:
		virtual bool Validate(const IRSemantics::Program& program) const;

	// Compile time code execution
	public:
		virtual bool CompileTimeCodeExecution(IRSemantics::Program& program, CodeBlock& activescope, CompileErrors& errors);

	// Type inference
	public:
		virtual bool TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Accessors
	public:
		Entity& GetEntity() const
		{ return *MyEntity; }

	// Internal state
	private:
		Entity* MyEntity;
	};

}

