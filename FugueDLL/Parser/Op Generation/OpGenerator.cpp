//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Functions for generating operations from parsed code
//
// Note that while the bulk of op generation is handled by these functions,
// the parser state machine will occasionally create operations directly for
// bits of lower-level functionality, e.g. flow control.
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Op Generation/OpValidation.h"
#include "Parser/Tracing.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"

#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"


#include "Utility/Strings.h"


using namespace Parser;


//
// Add an operation to the current code block.
//
void ParserState::PushOperation(const std::wstring& operationname)
{
	AddOperationToCurrentBlock(CreateOperation(operationname));
	MergeDeferredOperations();
	PopParameterCount();
}


//
// Create an operation pointer given the name of the operation/function
// to invoke. This operation object is later used for serialization or
// execution of the parsed source code. See the Operation class and its
// derivatives for additional details.
//
VM::OperationPtr ParserState::CreateOperation(const std::wstring& operationname)
{
	if(operationname.empty())		// This occurs when dealing with parenthetical infix expressions
	{
		ReportFatalError("Parenthetical expressions are not permitted by themselves");

		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();

		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(operationname == Keywords::Assign)
		return CreateOperation_Assign();
	else if(operationname == Keywords::Add)
		return CreateOperation_Add();
	else if(operationname == Keywords::Subtract)
		return CreateOperation_Subtract();
	else if(operationname == Keywords::Multiply)
		return CreateOperation_Multiply();
	else if(operationname == Keywords::Divide)
		return CreateOperation_Divide();
	else if(operationname == Keywords::Concat)
		return CreateOperation_Concat();
	else if(operationname == Keywords::Equal)
		return CreateOperation_Equal();
	else if(operationname == Keywords::NotEqual)
		return CreateOperation_NotEqual();
	else if(operationname == Keywords::Less)
		return CreateOperation_Less();
	else if(operationname == Keywords::Greater)
		return CreateOperation_Greater();
	else if(operationname == Keywords::LessEqual)
		return CreateOperation_LessEqual();
	else if(operationname == Keywords::GreaterEqual)
		return CreateOperation_GreaterEqual();
	else if(operationname == Keywords::DebugWrite)
		return CreateOperation_DebugWrite();
	else if(operationname == Keywords::DebugRead)
		return CreateOperation_DebugRead();
	else if(operationname == Keywords::DebugCrashVM)
		return CreateOperation_DebugCrashVM();
	else if(operationname == Keywords::Cast)
		return CreateOperation_Cast();
	else if(operationname == Keywords::ReadTuple)
		return CreateOperation_ReadTuple();
	else if(operationname == Keywords::AssignTuple)
		return CreateOperation_AssignTuple();
	else if(operationname == Keywords::ReadStructure)
		return CreateOperation_ReadStructure();
	else if(operationname == Keywords::AssignStructure)
		return CreateOperation_AssignStructure();
	else if(operationname == Keywords::Break)
		return CreateOperation_Break();
	else if(operationname == Keywords::Return)
		return CreateOperation_Return();
	else if(operationname == Keywords::SizeOf)
		return CreateOperation_SizeOf();
	else if(operationname == Keywords::Length)
		return CreateOperation_Length();
	else if(operationname == Keywords::Member)
		return CreateOperation_Member();
	else if(operationname == Keywords::Or)
		return CreateOperation_Or();
	else if(operationname == Keywords::And)
		return CreateOperation_And();
	else if(operationname == Keywords::Xor)
		return CreateOperation_Xor();
	else if(operationname == Keywords::Not)
		return CreateOperation_Not();
	else if(operationname == Keywords::List)
		return CreateOperation_ConsList();
	else if(operationname == Keywords::Map)
		return CreateOperation_Map();
	else if(operationname == Keywords::Reduce)
		return CreateOperation_Reduce();
	else if(operationname == Keywords::Message)
		return CreateOperation_Message();
	else if(operationname == Keywords::AcceptMessage)
		return CreateOperation_AcceptMessage();
	else if(operationname == Keywords::Caller)
	{
		ReportFatalError("This function can only be used when sending messages");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	else if(operationname == Keywords::Sender)
	{
		ReportFatalError("This function can only be used when replying to messages");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	else if(operationname == Keywords::Else || operationname == Keywords::ElseIf)
	{
		ReportFatalError("Missing the opening if() for this conditional");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	else if(operationname == Keywords::Future)
		return CreateOperation_Future();
	else
	{
		if(CurrentScope->HasTupleType(operationname))
		{
			const VM::TupleType& t = CurrentScope->GetTupleType(operationname);
			const std::vector<std::wstring>& members = t.GetMemberOrder();
			std::vector<VM::Operation*>& allops = Blocks.back().TheBlock->GetAllOperations();

			if(PassedParameterCount.size() > 1)
			{
				if(PassedParameterCount.top() != members.size())
					throw SyntaxException("Incorrect number of parameters");
			}
			else
			{
				if(PassedParameterCount.top() != members.size() + 1)
					throw SyntaxException("Incorrect number of parameters");
			}

			for(size_t i = 0; i < members.size(); ++i)
			{
				TheStack.pop_back();

				if(t.GetMemberType(members[i]) == VM::EpochVariableType_Integer16)
				{
					VM::Operation* op = Blocks.back().TheBlock->GetOperationFromEnd(members.size() - i, *CurrentScope);
					VM::Operations::PushIntegerLiteral* pushop = dynamic_cast<VM::Operations::PushIntegerLiteral*>(op);

					if(pushop)
					{
						Integer32 litval = pushop->GetValue();
						Integer16 lit16val;
						std::wstringstream stream;
						stream << litval;
						if(!(stream >> lit16val))
							throw SyntaxException("Overflow converting integer to integer16");
						Blocks.back().TheBlock->ReplaceOperationFromEnd(members.size() - i, VM::OperationPtr(new VM::Operations::PushInteger16Literal(lit16val)), *CurrentScope);
					}
				}
				else if(t.GetMemberType(members[i]) == VM::EpochVariableType_Function)
				{
					VM::Operation* op = Blocks.back().TheBlock->GetOperationFromEnd(members.size() - i, *CurrentScope);
					VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);

					if(pushop)
					{
						VM::Operation* pushedop = pushop->GetNestedOperation();
						VM::Operations::GetVariableValue* getvalop = dynamic_cast<VM::Operations::GetVariableValue*>(pushedop);
						if(getvalop)
						{
							if(!CurrentScope->GetFunctionSignature(t.GetMemberTypeHintString(members[i])).DoesFunctionMatchSignature(CurrentScope->GetFunction(getvalop->GetAssociatedIdentifier()), *CurrentScope))
								throw SyntaxException("Function does not match the required function signature");

							Blocks.back().TheBlock->ReplaceOperationFromEnd(members.size() - i, VM::OperationPtr(new VM::Operations::BindFunctionReference(getvalop->GetAssociatedIdentifier())), *CurrentScope);
						}
						else
							throw ParserFailureException("Failure while trying to pass a function reference");
					}
					else
						throw ParserFailureException("Failure while trying to pass a function reference");
				}
			}

			size_t opcount = ValidateTupleInit(members, operationname, allops, allops.size() - 1);

			if(PassedParameterCount.size() > 1)
			{
				VM::OperationPtr ret(new VM::Operations::IntegerConstant(static_cast<Integer32>(CurrentScope->GetTupleTypeID(operationname))));
				TypeAnnotationOps.insert(std::make_pair(ret.get(), opcount));
				return ret;
			}

			// Remove stack push of the identifier because this isn't a function call
			Blocks.back().TheBlock->RemoveOperationFromEnd(opcount, *CurrentScope);

			// Reverse the order that members are pushed onto the stack
			ReverseOps(Blocks.back().TheBlock, opcount);

			// Rearrange type annotation operations
			for(std::map<VM::Operation*, size_t>::iterator iter = TypeAnnotationOps.begin(); iter != TypeAnnotationOps.end(); )
			{
				bool movedop = false;
				unsigned index = 0;
				for(std::vector<VM::Operation*>::iterator opiter = allops.begin(); opiter != allops.end(); ++opiter)
				{
					VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(*opiter);
					if((*opiter == iter->first) || (pushop && pushop->GetNestedOperation() == iter->first))
					{
						VM::Operation* savedop = allops[index];

						size_t i;
						for(i = index; i < (index + iter->second); ++i)
							allops[i] = allops[i + 1];

						allops[i] = savedop;
						movedop = true;
						break;
					}

					++index;
				}

				if(movedop)
					iter = TypeAnnotationOps.erase(iter);
				else
					++iter;
			}

			// Be sure we push the type information onto the stack
			AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(static_cast<Integer32>(CurrentScope->GetTupleTypeID(operationname)))));

			CurrentScope->AddTupleVariable(operationname, TheStack.back().StringValue);
			VM::OperationPtr ret(new VM::Operations::InitializeValue(ParsedProgram->PoolStaticString(TheStack.back().StringValue)));
			TheStack.pop_back();
			return ret;
		}
		else if(CurrentScope->HasStructureType(operationname))
		{
			const VM::StructureType& t = CurrentScope->GetStructureType(operationname);
			const std::vector<std::wstring>& members = t.GetMemberOrder();
			std::vector<VM::Operation*>& allops = Blocks.back().TheBlock->GetAllOperations();

			if(PassedParameterCount.size() > 1)
			{
				if(PassedParameterCount.top() != (members.size() + 1))
					throw SyntaxException("Incorrect number of parameters");
			}
			else
			{
				if(PassedParameterCount.top() == 2)
				{
					VM::Operations::GetVariableValue* getvalueop = dynamic_cast<VM::Operations::GetVariableValue*>(allops.back()->GetNestedOperation());
					if(getvalueop)
					{
						if(getvalueop->GetType(*CurrentScope) == VM::EpochVariableType_Structure)
						{
							IDType hint = CurrentScope->GetScopeOwningVariable(getvalueop->GetAssociatedIdentifier())->GetVariableStructureTypeID(getvalueop->GetAssociatedIdentifier());
							if(hint != CurrentScope->GetStructureTypeID(operationname))
								throw SyntaxException("Incorrect parameter type");

							Blocks.back().TheBlock->RemoveOperationFromEnd(1, *CurrentScope);
							TheStack.pop_back();
							CurrentScope->AddStructureVariable(operationname, TheStack.back().StringValue);
							VM::OperationPtr ret(new VM::Operations::InitializeValue(ParsedProgram->PoolStaticString(TheStack.back().StringValue)));
							TheStack.pop_back();
							return ret;
						}
						else
							throw SyntaxException("Incorrect number of parameters");
					}
				}
				else if(PassedParameterCount.top() != members.size() + 1)
					throw SyntaxException("Incorrect number of parameters");
			}

			for(size_t i = 0; i < members.size(); ++i)
			{
				if(t.GetMemberType(members[i]) == VM::EpochVariableType_Integer16)
				{
					VM::Operation* op = Blocks.back().TheBlock->GetOperationFromEnd(members.size() - i, *CurrentScope);
					VM::Operations::PushIntegerLiteral* pushop = dynamic_cast<VM::Operations::PushIntegerLiteral*>(op);

					if(pushop)
					{
						Integer32 litval = pushop->GetValue();
						Integer16 lit16val;
						std::wstringstream stream;
						stream << litval;
						if(!(stream >> lit16val))
							throw SyntaxException("Overflow converting integer to integer16");
						Blocks.back().TheBlock->ReplaceOperationFromEnd(members.size() - i, VM::OperationPtr(new VM::Operations::PushInteger16Literal(lit16val)), *CurrentScope);
					}
				}
				else if(t.GetMemberType(members[i]) == VM::EpochVariableType_Function)
				{
					VM::Operation* op = Blocks.back().TheBlock->GetOperationFromEnd(members.size() - i, *CurrentScope);
					VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);

					if(pushop)
					{
						VM::Operation* pushedop = pushop->GetNestedOperation();
						VM::Operations::GetVariableValue* getvalop = dynamic_cast<VM::Operations::GetVariableValue*>(pushedop);
						if(getvalop)
						{
							if(!CurrentScope->GetFunctionSignature(t.GetMemberTypeHintString(members[i])).DoesFunctionMatchSignature(CurrentScope->GetFunction(getvalop->GetAssociatedIdentifier()), *CurrentScope))
								throw SyntaxException("Function does not match the required function signature");

							Blocks.back().TheBlock->ReplaceOperationFromEnd(members.size() - i, VM::OperationPtr(new VM::Operations::BindFunctionReference(getvalop->GetAssociatedIdentifier())), *CurrentScope);
						}
						else
							throw ParserFailureException("Failure while trying to pass a function reference");
					}
					else
						throw ParserFailureException("Failure while trying to pass a function reference");
				}
			}

			size_t opcount = ValidateStructInit(operationname, *Blocks.back().TheBlock, 1);


			// TODO - WTF was this supposed to accomplish?
			/*if(PassedParameterCount.size() > 1)
			{
				VM::OperationPtr ret(new VM::Operations::IntegerConstant(static_cast<Integer32>(CurrentScope->GetStructureTypeID(operationname))));
				TypeAnnotationOps.insert(std::make_pair(ret.get(), opcount));
				return ret;
			}*/

			// Remove stack push of the identifier because this isn't a function call
			std::auto_ptr<VM::Operation> idpushop(Blocks.back().TheBlock->GetOperationFromEnd(members.size() + 1, *CurrentScope));
			Blocks.back().TheBlock->EraseOperation(idpushop.get());

			// Reverse the order that members are pushed onto the stack
			ReverseOpsAsGroups(Blocks.back().TheBlock, members.size());

			// Rearrange type annotation operations
			for(std::map<VM::Operation*, size_t>::iterator iter = TypeAnnotationOps.begin(); iter != TypeAnnotationOps.end(); )
			{
				bool movedop = false;
				unsigned index = 0;
				for(std::vector<VM::Operation*>::iterator opiter = allops.begin(); opiter != allops.end(); ++opiter)
				{
					VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(*opiter);
					if((*opiter == iter->first) || (pushop && pushop->GetNestedOperation() == iter->first))
					{
						VM::Operation* savedop = allops[index];

						size_t i;
						for(i = index; i < (index + iter->second); ++i)
							allops[i] = allops[i + 1];

						allops[i] = savedop;
						movedop = true;
						break;
					}

					++index;
				}

				if(movedop)
					iter = TypeAnnotationOps.erase(iter);
				else
					++iter;
			}

			for(size_t i = 0; i < members.size(); ++i)
				TheStack.pop_back();

			// TODO - fix copy constructors

			// Be sure we push the type information onto the stack
			// (but only if we are not generating a copy constructor)
			//if(isfunctioninit)
			//	TheStack.pop_back();
			//else
				AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(static_cast<Integer32>(CurrentScope->GetStructureTypeID(operationname)))));

			CurrentScope->AddStructureVariable(operationname, TheStack.back().StringValue);
			VM::OperationPtr ret(new VM::Operations::InitializeValue(ParsedProgram->PoolStaticString(TheStack.back().StringValue)));
			TheStack.pop_back();
			return ret;
		}
		else if(CurrentScope->IsFunctionSignature(operationname))
		{
			const VM::FunctionSignature& signature = CurrentScope->GetFunctionSignature(operationname);
			const std::vector<VM::EpochVariableTypeID>& paramtypes = signature.GetParamTypes();
			if(paramtypes.size() != PassedParameterCount.top())
			{
				std::wostringstream stream;
				stream << operationname << L"() function expects " << paramtypes.size() << (paramtypes.size() == 1 ? L" parameter" : L"parameters");
				throw SyntaxException(narrow(stream.str()).c_str());
			}

			unsigned i = 0;
			for(std::vector<VM::EpochVariableTypeID>::const_iterator iter = paramtypes.begin(); iter != paramtypes.end(); ++iter)
			{
				ValidateOperationParameter(i, *iter, signature);
				TheStack.pop_back();
				++i;
			}
			
			return VM::OperationPtr(new VM::Operations::InvokeIndirect(ParsedProgram->PoolStaticString(operationname)));
		}


		VM::FunctionBase* func = NULL;

		try
		{
			func = CurrentScope->GetFunction(operationname);
		}
		catch(std::exception& e)
		{
			ReportFatalError(e.what());
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		const std::vector<std::wstring>& paramlist = func->GetParams().GetMemberOrder();
		if(paramlist.size() != PassedParameterCount.top())
		{
			std::wostringstream stream;
			stream << operationname << L"() function expects " << paramlist.size() << (paramlist.size() == 1 ? L" parameter (" : L" parameters (") << PassedParameterCount.top() << " provided)";
			throw SyntaxException(narrow(stream.str()).c_str());
		}

		unsigned i = 0;
		for(std::vector<std::wstring>::const_iterator iter = paramlist.begin(); iter != paramlist.end(); ++iter)
		{
			VM::EpochVariableTypeID type = func->GetParams().GetVariableType(*iter);
			ValidateOperationParameter(i, type, func->GetParams());

			TheStack.pop_back();
			++i;
		}

		return VM::OperationPtr(new VM::Operations::Invoke(func, false));
	}
}


//
// Perform type checking on a structure initialization
//
// Given a sequence of instructions that is meant to initialize a
// structure variable, check the corresponding types to ensure that
// all type requirements of the struct initialization are met.
//
size_t ParserState::ValidateStructInit(const std::wstring& structtypename, VM::Block& block, size_t offsetfromend)
{
	const VM::StructureType& structtype = CurrentScope->GetStructureType(structtypename);
	IDType hint = CurrentScope->GetStructureTypeID(structtypename);

	const std::vector<VM::Operation*>& ops = block.GetAllOperations();

	if(ops.empty() || ops.size() < structtype.GetMemberOrder().size())
		throw ParserFailureException("Not enough instructions present to fully initialize structure");

	size_t opindex = ops.size() - 1;

	for(std::vector<std::wstring>::const_reverse_iterator iter = structtype.GetMemberOrder().rbegin(); iter != structtype.GetMemberOrder().rend(); ++iter)
	{
		VM::EpochVariableTypeID membertype = structtype.GetMemberType(*iter);
		VM::Operation* op = ops[opindex];

		if(membertype == VM::EpochVariableType_Function)
		{
			// TODO - validate function pointers
		}
		else if(membertype == VM::EpochVariableType_Structure)
		{
			// TODO - validate nested structures
		}
		else if(op->GetType(*CurrentScope) != membertype)
		{
			std::ostringstream stream;
			stream << "Type mismatch - structure member \"" << narrow(*iter) << "\"";
			ReportFatalError(stream.str().c_str());
		}
		
		opindex -= op->GetNumParameters(*CurrentScope);
		--opindex;
	}

	return ops.size() - opindex;
}


//
// Perform type checking on a tuple initialization
//
// Given a sequence of instructions that is meant to initialize a
// tuple variable, check the corresponding types to ensure that
// all type requirements of the tuple initialization are met.
//
size_t ParserState::ValidateTupleInit(const std::vector<std::wstring>& members, const std::wstring& tupletypename, std::vector<VM::Operation*>& ops, size_t maxop)
{
	const VM::TupleType& tupletype = CurrentScope->GetTupleType(tupletypename);

	size_t ret = members.size();
	size_t remainingmembers = ret;
	size_t opindex = maxop;
	size_t memberindex = members.size() - 1;
	
	while(remainingmembers > 0)
	{
		VM::EpochVariableTypeID membertype = tupletype.GetMemberType(members[memberindex]);

		if(membertype != ops[opindex]->GetType(*CurrentScope))
		{
			std::ostringstream stream;
			stream << "Type mismatch - parameter " << memberindex + 1;
			ReportFatalError(stream.str().c_str());
		}
		else
		{
			size_t paramcount = ops[opindex]->GetNumParameters(*CurrentScope);
			while(paramcount > 0)
			{
				--opindex;
				++ret;

				paramcount += ops[opindex]->GetNumParameters(*CurrentScope);
				--paramcount;
			}
		}

		--opindex;
		--remainingmembers;
		--memberindex;
	}

	return ret;
}


//
// Helper wrapper representing a group of related operations
//
// This is useful for allowing multiple-instruction sequences that
// result in a single value passed to a function or constructor.
// Each instruction in the sequence is wrapped into this structure
// and treated as if the sequence itself is an indivisible unit.
//
struct MultiOp
{
	std::vector<VM::Operation*> TheOps;
	size_t StoreAllOps(size_t startindex, std::vector<VM::Operation*>& ops) const
	{
		for(std::vector<VM::Operation*>::const_reverse_iterator iter = TheOps.rbegin(); iter != TheOps.rend(); ++iter)
			ops[startindex++] = *iter;

		return startindex;
	}
};

//
// Reverse the given set of trailing operations in a code block
//
// In some cases we need to reverse the order of operations, e.g. when
// dealing with certain calling conventions.
//
void ParserState::ReverseOps(VM::Block* block, size_t numops)
{
	if(numops <= 1)
		return;

	std::vector<VM::Operation*>& originalops = block->GetAllOperations();
	size_t index = originalops.size() - 1;
	size_t originalnumops = numops;

	struct autocleanup
	{
		~autocleanup()
		{
			for(std::vector<MultiOp*>::iterator iter = opgroups.begin(); iter != opgroups.end(); ++iter)
				delete *iter;
		}

		std::vector<MultiOp*> opgroups;
	} safety;

	while(numops)
	{
		size_t numparams = 0;

		std::auto_ptr<MultiOp> multiop(new MultiOp);
		do
		{
			numparams += originalops[index]->GetNumParameters(*CurrentScope);
			multiop->TheOps.push_back(originalops[index]);
			--numops;
			--index;
		} while(numparams-- > 0);
		safety.opgroups.push_back(multiop.release());
	}

	size_t startindex = index + 1;
	for(std::vector<MultiOp*>::const_iterator iter = safety.opgroups.begin(); iter != safety.opgroups.end(); ++iter)
		startindex = (*iter)->StoreAllOps(startindex, originalops);
}


// TODO - figure out why we need two different versions of this logic, and fix one or the other if necessary
void ParserState::ReverseOpsAsGroups(VM::Block* block, size_t numops)
{
	if(numops <= 1)
		return;

	std::vector<VM::Operation*>& originalops = block->GetAllOperations();
	size_t index = originalops.size() - 1;
	size_t originalnumops = numops;

	struct autocleanup
	{
		~autocleanup()
		{
			for(std::vector<MultiOp*>::iterator iter = opgroups.begin(); iter != opgroups.end(); ++iter)
				delete *iter;
		}

		std::vector<MultiOp*> opgroups;
	} safety;

	while(numops)
	{
		size_t numparams = 0;

		std::auto_ptr<MultiOp> multiop(new MultiOp);
		do
		{
			numparams += originalops[index]->GetNumParameters(*CurrentScope);
			multiop->TheOps.push_back(originalops[index]);
			--index;
		} while(numparams-- > 0);
		safety.opgroups.push_back(multiop.release());

		--numops;
	}

	size_t startindex = index + 1;
	for(std::vector<MultiOp*>::const_iterator iter = safety.opgroups.begin(); iter != safety.opgroups.end(); ++iter)
		startindex = (*iter)->StoreAllOps(startindex, originalops);
}

