/*! \file Core\LSLBuilder.h
\ingroup Engine
\brief LSL Builder Function¡£
*/
#ifndef WE_Core_WSLBuilder_H
#define WE_Core_WSLBuilder_H 1

#include <WScheme/SContext.h>
#include <WScheme/WScheme.h>
#include <WScheme/WSchemREPL.h>

namespace platform::lsl {
	using REPLContext = scheme::v1::REPLContext;

	using namespace scheme;
	using namespace v1;

	namespace context {

		LiteralPasses::HandlerType FetchNumberLiteral();
	}

	namespace math {
		void RegisterTypeLiteralAction(REPLContext& context);

		void RegisterMathDotLssFile(REPLContext& context);
	}


	namespace access {
		namespace details {
			template<typename _target>
			_target static_value_cast(TermNode& term) {
				return AccessTerm<_target>(term);
			}

			template<typename _target,typename _head, typename... tails>
			_target static_value_cast(TermNode & term) {
				if (auto pHead = AccessTermPtr<_head>(term))
					return static_cast<_target>(*pHead);
				else
					return static_value_cast<_target, tails...>(term);
			}
		}


		template<typename _target,typename... _types>
		_target static_value_cast(TermNode & term) {
			return details::static_value_cast<_target, _types...>(term);
		}
	}
}

#endif