//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Helper routines for traversing a generated code tree
//

#pragma once


// Dependencies
#include "Virtual Machine/SelfAware.h"
#include "Validator/Validator.h"
#include "Serialization/SerializationTraverser.h"


//
// Invoke the actual traverser logic for the requested types
//
// This architecture is used to help alleviate dependencies between
// node object types (e.g. operation classes) and traverser wrapper
// types (e.g. the serializer). A more typical OO approach would be
// to use virtual functions to handle serialization and other forms
// of traversal; however, such approaches require tight coupling of
// the traversal code and the node implementations. Instead, we use
// a generic traversal interface, so that all nodes need to know is
// how to traverse themselves and any child nodes. Node classes now
// simply inherit from the SelfAware base, which provides functions
// for invoking various traverser types via virtual functions. That
// interface then passes along the actual traversal request to this
// template function, effectively creating a double dispatch system
// based on the traverser and node types. The net result is that we
// can implement the actual logic for each traverser type without a
// corresponding set of virtual functions in every node class - the
// traverser logic is implemented via templates in one central code
// location, rather than having it spread across every node class.
//
template <class TraverserType, class NodeType>
void TraversalHelper(TraverserType& traverser, NodeType& node)
{
	traverser.TraverseNode<NodeType>(node);
}


//
// Dispatch a traversal call for the validation traverser
//
template <class SelfType>
void VM::SelfAware<SelfType>::Traverse(Validator::ValidationTraverser& traverser)
{
	TraversalHelper<Validator::ValidationTraverser, SelfType>(traverser, *static_cast<SelfType*>(this));
}

//
// Dispatch a traversal call for the serialization traverser
//
template <class SelfType>
void VM::SelfAware<SelfType>::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraversalHelper<Serialization::SerializationTraverser, SelfType>(traverser, *static_cast<SelfType*>(this));
}


//
// Retrieve the token associated with a given node type
//
// Note that while the serialization logic provides the actual
// string data of the token, the token itself can be used by
// many different subsystems. For example, these tokens are
// used by the language extension system to pass sections of
// the code tree to external libraries.
//
template <class SelfType>
const std::wstring& VM::SelfAware<SelfType>::GetToken() const
{
	return Serialization::GetToken<SelfType>();
}
