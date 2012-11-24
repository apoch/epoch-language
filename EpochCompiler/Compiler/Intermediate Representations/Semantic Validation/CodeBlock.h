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
	class Namespace;
	struct InferenceContext;


	//
	// Interface implemented by each code block entry wrapper class
	//
	// We store code block entries in wrappers so they can all be placed
	// in a homogeneous container in the code block IR node itself, and
	// we use the wrappers to forward requests for validation and type
	// inference on to the actual contained IR nodes. This eliminates the
	// need for all IR nodes to derive from a common base class just for
	// the sake of storing them in a code block, although it does cost
	// a small amount of runtime overhead.
	//
	class CodeBlockEntry
	{
	// Destruction
	public:
		virtual ~CodeBlockEntry()
		{ }

	// Validation
	public:
		virtual bool Validate(const Namespace& curnamespace) const = 0;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors) = 0;

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const = 0;

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription* target) = 0;
	};


	//
	// IR node class for storing a block of code
	//
	class CodeBlock
	{
	// Construction and destruction
	public:
		explicit CodeBlock(ScopeDescription* scope, bool ownsscope = true);
		~CodeBlock();

		CodeBlock* Clone() const;

	// Non-copyable
	private:
		CodeBlock(const CodeBlock& other);
		CodeBlock& operator = (const CodeBlock& rhs);

	// Entry management
	public:
		void AddEntry(CodeBlockEntry* entry);

		const std::vector<CodeBlockEntry*>& GetEntries() const
		{ return Entries; }

	// Lexical scope management
	public:
		EPOCHCOMPILER void AddVariable(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, bool isreference, VariableOrigin origin);
		Metadata::EpochTypeID GetVariableTypeByID(StringHandle identifier) const;
		bool GetVariableLocalOffset(StringHandle identifier, const std::map<Metadata::EpochTypeID, size_t>& sumtypesizes, size_t& outframes, size_t& outoffset, size_t& outsize) const;
		bool GetVariableParamOffset(StringHandle identifier, const std::map<Metadata::EpochTypeID, size_t>& sumtypesizes, size_t& outframes, size_t& outoffset, size_t& outsize) const;

		ScopeDescription* GetScope()
		{ return Scope; }

		const ScopeDescription* GetScope() const
		{ return Scope; }

	// Validation
	public:
		bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		bool TypeInference(Namespace& curnamespace, InferenceContext& context, CompileErrors& errors);

	// Shadowing checking
	public:
		EPOCHCOMPILER bool ShadowingCheck(StringHandle identifier, CompileErrors& errors);

	// Optimization
	public:
		void HoistScopes(ScopeDescription* target);

	// Internal state
	private:
		std::vector<CodeBlockEntry*> Entries;
		ScopeDescription* Scope;
		bool OwnsScope;
	};


	//
	// Concrete wrapper class for an entry in a code block
	// which forwards to an assignment IR node
	//
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
		virtual bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const;

	// Property access
	public:
		const Assignment& GetAssignment() const
		{ return *MyAssignment; }

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription*)
		{ }

	// Internal state
	private:
		Assignment* MyAssignment;
	};


	//
	// Concrete wrapper class for an entry in a code block
	// which forwards to a statement IR node
	//
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
		virtual bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const;

	// Statement access
	public:
		const Statement& GetStatement() const
		{ return *MyStatement; }

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription*)
		{ }

	// Internal state
	private:
		Statement* MyStatement;
	};

	
	//
	// Concrete wrapper class for an entry in a code block
	// which forwards to a pre-operator IR node
	//
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
		virtual bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const;

	// Statement access
	public:
		const PreOpStatement& GetStatement() const
		{ return *MyStatement; }

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription*)
		{ }

	// Internal state
	private:
		PreOpStatement* MyStatement;
	};



	//
	// Concrete wrapper class for an entry in a code block
	// which forwards to a post-operator IR node
	//
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
		virtual bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const;

	// Statement access
	public:
		const PostOpStatement& GetStatement() const
		{ return *MyStatement; }

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription*)
		{ }

	// Internal state
	private:
		PostOpStatement* MyStatement;
	};


	//
	// Concrete wrapper class for an entry in a code block
	// which forwards to a nested code block IR node
	//
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
		virtual bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const;

	// Internal code access
	public:
		const CodeBlock& GetCode() const
		{ return *MyCodeBlock; }

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription* target);

	// Internal state
	private:
		CodeBlock* MyCodeBlock;
	};


	//
	// Concrete wrapper class for an entry in a code block
	// which forwards to an entity invocation IR node
	//
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
		virtual bool Validate(const Namespace& curnamespace) const;

	// Type inference
	public:
		virtual bool TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors);

	// Deep copies
	public:
		virtual CodeBlockEntry* Clone() const;

	// Accessors
	public:
		Entity& GetEntity() const
		{ return *MyEntity; }

	// Optimization
	public:
		virtual void HoistScopes(ScopeDescription* target);

	// Internal state
	private:
		Entity* MyEntity;
	};

}

