#ifndef WBase_lmacro_h
#define Lbase_lmacro_h 1

#include "wdef.h"

/*
以下名构词缩写的含义：
Ctor constructor
Cvt converter
De default
Decl declare
Def define
Del deleted
Dtor destructor
Expr expression
Fn function
Fwd forward
H head
I interface
Impl implement
Mem member
Op operator
P partially
PDecl pre-declare
Pred predicate
Ret returning
S statically
Tmpl template

以下宏参数缩写的含义：
_a argument
_alist arguments list
_attr attributes
_b base
_e expression
_i interface
_m member
_n name
_op operator
_p parameter
_plist parameters list
_q qualifier(s)
_sig signature
_t type
*/

//通用头定义。
#define PDefH(_t, _n, ...) \
	_t \
	_n(__VA_ARGS__)
#define PDefHOp(_t, _op, ...) \
	PDefH(_t, operator _op, __VA_ARGS__)

#define PDefCvt(_t) \
	operator _t()


//简单通用函数实现。
//prefix "Impl" = Implementation;
#define ImplExpr(...) \
		{ \
		(__VA_ARGS__), void(); \
		}
#define ImplRet(...) \
		{ \
		return __VA_ARGS__; \
		}
#define ImplThrow(...) \
		{ \
		throw __VA_ARGS__; \
		}
// NOTE: GCC complains about 'void(wunseq(__VA_ARGS__))'.
#define ImplUnseq(...) \
		{ \
		wunused(wunseq(__VA_ARGS__)); \
		}

//!
//@{
//! \brief 指定表达式在 \c try 块中求值。
#define TryExpr(...) \
	try \
	ImplExpr(__VA_ARGS__)
//! \brief 指定表达式在 \c try 块中求值并返回。
#define TryRet(...) \
	try \
	ImplRet(__VA_ARGS__)

//! \brief 指定表达式作为异常处理器。
#define CatchExpr(_ex, ...) \
	catch(_ex) \
	ImplExpr(__VA_ARGS__)
//! \brief 指定异常处理器忽略指定异常。
#define CatchIgnore(_ex) \
	catch(_ex) \
	{}
//! \brief 指定返回语句作为异常处理器。
#define CatchRet(_ex, ...) \
	catch(_ex) \
	ImplRet(__VA_ARGS__)
//! \brief 指定抛出指定异常作为异常处理器。
#define CatchThrow(_ex, ...) \
	catch(_ex) \
	ImplThrow(__VA_ARGS__)
//@}

//基类同名函数映射和成员同名函数映射实现。
//prefix "Impl" = Implement;
#define ImplBodyBase(_b, _n, ...) \
	ImplRet(_b::_n(__VA_ARGS__))
#define ImplBodyMem(_m, _n, ...) \
	ImplRet((_m)._n(__VA_ARGS__))


//简单通用成员函数定义。
//prefix "Def" = Define;
/*!
\def DefDeDtor
\brief 定义默认析构函数。
\note CWG defect 906 的解决禁止显式默认虚函数，但 CWG defect 1135
的解决撤销了这一限制。 ISO C++11 最终没有此限制。
\note ISO C++11 不需要显式使用异常规范，可自动推导（参见 ISO C++11 12.4/3 ）。
显式异常规范导致使用隐式继承需要保证成员函数的异常规范限制。
*/

#define DefDeCtor(_t) \
	_t() = default;
#define DefDelCtor(_t) \
	_t() = delete;

#define DefDeCopyCtor(_t) \
	_t(const _t&) = default;
#define DefDelCopyCtor(_t) \
	_t(const _t&) = delete;

#define DefDeMoveCtor(_t) \
	_t(_t&&) = default;
#define DefDelMoveCtor(_t) \
	_t(_t&&) = delete;

//! \since build v1.3
#define DefDeCopyMoveCtor(_t) \
	DefDeCopyCtor(_t) \
	DefDeMoveCtor(_t)

#define DefDeDtor(_t) \
	~_t() = default;
#define DefDelDtor(_t) \
	~_t() = delete;

#define ImplDeDtor(_t) \
	_t::DefDeDtor(_t)

#define ImplDeCtor(_t) \
	_t::DefDeCtor(_t)

#define DefVrDtor(_t) \
	virtual ~_t() = default;
#define ImplEmptyDtor(_t) \
	inline _t::DefDeDtor(_t)

#define DefDeCopyAssignment(_t) \
	_t& operator=(const _t&) = default;
#define DefDelCopyAssignment(_t) \
	_t& operator=(const _t&) = delete;

#define DefDeMoveAssignment(_t) \
	_t& operator=(_t&&) = default;
#define DefDelMoveAssignment(_t) \
	_t& operator=(_t&&) = delete;


//! \since build v1.3
#define DefDeCopyMoveAssignment(_t) \
	DefDeCopyAssignment(_t) \
	DefDeMoveAssignment(_t)

//! \since build v1.3
#define DefDeCopyMoveCtorAssignment(_t) \
	DefDeCopyMoveCtor(_t) \
	DefDeCopyMoveAssignment(_t)

#define DefCvt(_q, _t, ...) \
	operator _t() _q \
	ImplRet(__VA_ARGS__)
#define DefCvtBase(_q, _t, _b) \
	DefCvt(_q, _t, _b::operator _t())
#define DefCvtMem(_q, _t, _m) \
	DefCvt(_q, _t, (_m).operator _t())

/*!
\brief 定义表示否定的 \c operator! 。
\pre 表达式 <tt>bool(*this)</tt> 合式且无异常抛出。
\since build v1.3
*/
#define DefNeg \
	PDefHOp(bool, !, ) const wnothrow \
		ImplRet(!bool(*this))

/*!
\brief 定义表示显式转换 bool 操作符及其否定的 \c operator! 。
\pre 表达式合式且无异常抛出。
\since build v1.3
*/
#define DefBoolNeg(_spec, ...) \
	DefNeg \
	_spec DefCvt(const wnothrow, bool, __VA_ARGS__)

#define DefPred(_q, _n, ...) \
	bool WPP_Concat(Is, _n)() _q \
	ImplRet(__VA_ARGS__)
#define DefPredBase(_q, _n, _b) \
	DefPred(_q, _n, _b::WPP_Concat(Is, _n)())
#define DefPredMem(_q, _n, _m) \
	DefPred(_q, _n, (_m).WPP_Concat(Is, _n)())

#define DefGetter(_q, _t, _n, ...) \
	_t \
	WPP_Concat(Get, _n)() _q \
	ImplRet(__VA_ARGS__)
#define DefGetterBase(_q, _t, _n, _b) \
	DefGetter(_q, _t, _n, _b::WPP_Concat(Get, _n)())
#define DefGetterMem(_q, _t, _n, _m) \
	DefGetter(_q, _t, _n, (_m).WPP_Concat(Get, _n)())

#define DefSetter(_q,_t, _n, _m) \
	void WPP_Concat(Set, _n)(_t _tempArgName) _q \
	ImplExpr((_m) = _tempArgName)
#define DefSetterDe(_q,_t, _n, _m, _defv) \
	void WPP_Concat(Set, _n)(_t _tempArgName = _defv) _q \
	ImplExpr((_m) = _tempArgName)
#define DefSetterBase(_q,_t, _n, _b) \
	void WPP_Concat(Set, _n)(_t _tempArgName) _q \
	ImplExpr(_b::WPP_Concat(Set, _n)(_tempArgName))
#define DefSetterBaseDe(_q,_t, _n, _b, _defv) \
	void WPP_Concat(Set, _n)(_t _tempArgName = _defv) _q\
	ImplExpr(_b::WPP_Concat(Set, _n)(_tempArgName))
#define DefSetterMem(_q,_t, _n, _m) \
	void WPP_Concat(Set, _n)(_t _tempArgName) _q\
	ImplExpr((_m).WPP_Concat(Set, _n)(_tempArgName))
#define DefSetterMemDe(_q,_t, _n, _m, _defv) \
	void WPP_Concat(Set, _n)(_t _tempArgName = _defv) _q \
	ImplExpr((_m).WPP_Concat(Set, _n)(_tempArgName))
#define DefSetterEx(_q,_t, _n, _m, ...) \
	void WPP_Concat(Set, _n)(_t _tempArgName) _q\
	ImplExpr((_m) = (__VA_ARGS__))
#define DefSetterDeEx(_q,_t, _n, _m, _defv, ...) \
	void WPP_Concat(Set, _n)(_t _tempArgName = _defv) _q \
	ImplExpr((_m) = (__VA_ARGS__))


/*!
\def DefClone
\brief 动态复制。
\note 需要在满足 \c CopyConstructible 的类的定义内。
\note 如需要多态复制，需要显示前置 \c virtual 或加入 \c override 等指示符。
\since build 409
*/
#define DefClone(_q, _t) \
	PDefH(_t*, clone, ) _q \
		ImplRet(new _t(*this))


/*!
\def DefSwap
\brief 交换成员。
\note 需要对应类型具有接受两个左值引用的 swap 成员函数。
\since build 409
*/
#define DefSwap(_q, _t) \
	PDefH(void, swap, _t& _x, _t& _y) _q \
		ImplExpr(_x.swap(_y))


//成员函数和模板映射。


/*!
\brief 传递函数。
\since build 266
*/
#define DefFwdFn(_q, _t, _n, ...) \
	inline _t \
	_n() _q \
		{ \
		return (__VA_ARGS__); \
		}

/*!
\brief 传递模板。
\since build 266
*/
#define DefFwdTmpl(_q, _t, _n, ...) \
	template<typename... _tParams> \
	inline _t \
	_n(_tParams&&... args) _q \
		{ \
		return (__VA_ARGS__); \
		}

/*!
\brief 自动推断返回类型的传递模板。
*/
#define DefFwdTmplAuto(_n, ...) \
	DefFwdTmpl(wnoexcept_spec(decltype(__VA_ARGS__)(__VA_ARGS__)) \
		-> decltype(__VA_ARGS__), auto, _n, __VA_ARGS__)

/*!	\defgroup InterfaceTypeMacros Interface Type Macros
\brief 接口类型宏。
\since build 161
*/
//@{

#define WInterface struct

#define implements public

/*!
\def _lInterfaceHead
\brief 定义接口类型头部。
\sa ImplEmptyDtor
*/
#define YInterfaceHead(_n) { \
protected: \
	DefDeCtor(_n) \
	DefDeCopyCtor(_n) \
\
public:

#define FwdDeclI(_n) _lInterface _n;

/*!
\def DeclI
\brief 定义接口类型。
\since build 362
*/
#define DeclI(_attr, _n) \
	WInterface _attr _n \
	YInterfaceHead(_n) \
	virtual ~_n();

/*
\def DeclDerivedI
\brief 定义派生接口类型。
\note 由于接口定义为 struct 类型，因此通常只需指定是否为 virtual 继承。
\since build 362
*/
#define DeclDerivedI(_attr, _n, ...) \
	WInterface _attr _n : __VA_ARGS__ \
	YInterfaceHead(_n) \
	~_n() ImplI(__VA_ARGS__);

// ImplI = Implements Interface;
#define ImplI(...) override

//抽象实现：保留接口供派生类实现（可以提供接口函数的默认实现）。
// ImplA = Implements Abstractly;
#define ImplA(...)

#define DeclIEntry(_sig) virtual _sig = 0;

#define EndDecl };


/*!
\brief 静态接口。
\since build 266
*/
#define DeclSEntry(...)
/*!
\brief 静态接口实现。
\since build 266
*/
#define ImplS(...)
//@}


#define DefIEntryImpl(_sig) virtual _sig override; 

/*!
\brief 定义直接派生类。
\note 仅构造函数。不定义成员，避免对象切片或其它成员存储泄漏问题。
\since build 352
*/
#define DefExtendClass(_attr, _n, ...) \
	class _attr _n : __VA_ARGS__ \
		{ \
	public: \
		_n(); \
		};


/*!
\brief 位掩码类型操作。
\note 参见 ISO C++11 17.5.2.1.3[bitmask.types] 。
\since build 270
*/
//@{
#define DefBitmaskAnd(_tBitmask, _tInt) \
	wconstfn _tBitmask operator&(_tBitmask _x, _tBitmask _y) \
		ImplRet(static_cast<_tBitmask>( \
			static_cast<_tInt>(_x) & static_cast<_tInt>(_y)))

#define DefBitmaskOr(_tBitmask, _tInt) \
	wconstfn _tBitmask operator|(_tBitmask _x, _tBitmask _y) \
		ImplRet(static_cast<_tBitmask>( \
			static_cast<_tInt>(_x) | static_cast<_tInt>(_y)))

#define DefBitmaskXor(_tBitmask, _tInt) \
	wconstfn _tBitmask operator^(_tBitmask _x, _tBitmask _y) \
		ImplRet(static_cast<_tBitmask>( \
			static_cast<_tInt>(_x) ^ static_cast<_tInt>(_y)))

#define DefBitmaskNot(_tBitmask, _tInt) \
	wconstfn _tBitmask operator~(_tBitmask _x) \
		ImplRet(static_cast<_tBitmask>(~static_cast<_tInt>(_x)))

#define DefBitmaskAndAssignment(_tBitmask, _tInt) \
	inline _tBitmask& operator&=(_tBitmask& _x, _tBitmask _y) \
		ImplRet(_x = _x & _y)

#define DefBitmaskOrAssignment(_tBitmask, _tInt) \
	inline _tBitmask& operator|=(_tBitmask& _x, _tBitmask _y) \
		ImplRet(_x = _x | _y)

#define DefBitmaskXorAssignment(_tBitmask, _tInt) \
	inline _tBitmask& operator^=(_tBitmask& _x, _tBitmask _y) \
		ImplRet(_x = _x ^ _y)

#define DefBitmaskOperations(_tBitmask, _tInt) \
	DefBitmaskAnd(_tBitmask, _tInt) \
	DefBitmaskOr(_tBitmask, _tInt) \
	DefBitmaskXor(_tBitmask, _tInt) \
	DefBitmaskNot(_tBitmask, _tInt) \
	DefBitmaskAndAssignment(_tBitmask, _tInt) \
	DefBitmaskOrAssignment(_tBitmask, _tInt) \
	DefBitmaskXorAssignment(_tBitmask, _tInt)

#define DefBitmaskEnum(_tEnum) \
	DefBitmaskOperations(_tEnum, typename std::underlying_type<_tEnum>::type)



#endif