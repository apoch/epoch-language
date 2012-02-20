#pragma once


namespace boost { namespace spirit
{
	BOOST_SPIRIT_TERMINAL(adapttokens);

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<qi::domain, tag::adapttokens> // enables adapttokens
      : mpl::true_ {};
}}


namespace boost { namespace spirit { namespace qi
{
    using spirit::adapttokens;
    using spirit::adapttokens_type;

    template <typename Subject>
    struct adapttokens_directive : unary_parser<adapttokens_directive<Subject> >
    {
        typedef Subject subject_type;
        adapttokens_directive(Subject const& subject)
          : subject(subject) {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef iterator_range<Iterator> type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, Attribute& attr) const
        {
            qi::skip_over(first, last, skipper);

			if(first == last)
				return false;

			std::wstring::const_iterator tokenfirst = first->value().begin();
			std::wstring::const_iterator tokenlast = first->value().end();

            if(subject.parse(tokenfirst, tokenlast, context, skipper, unused) && (tokenfirst == tokenlast))
            {
				spirit::traits::assign_to(first->value().begin(), first->value().end(), attr);
				++first;
                return true;
            }
            return false;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("adapttokens", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::adapttokens, Subject, Modifiers>
    {
        typedef adapttokens_directive<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject, unused_type) const
        {
            return result_type(subject);
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<qi::adapttokens_directive<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
        , typename Iterator>
    struct handles_container<qi::adapttokens_directive<Subject>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}
