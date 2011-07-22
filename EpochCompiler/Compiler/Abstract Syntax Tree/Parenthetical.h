#pragma once

namespace AST
{

	typedef boost::variant
		<
			Undefined,
			Deferred<PreOperatorStatement, boost::intrusive_ptr<PreOperatorStatement> >,
			Deferred<PostOperatorStatement, boost::intrusive_ptr<PostOperatorStatement> >,
			Deferred<Expression, boost::intrusive_ptr<Expression> >
		> Parenthetical;

}

