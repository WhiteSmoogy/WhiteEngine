/*!	\file WCoreUtilities.h
\ingroup WFrameWork/Core
\brief ����ʵ��ģ�顣
*/

#ifndef WFrameWork_Core_WCoreUtilities_h
#define WFrameWork_Core_WCoreUtilities_h 1

#include <WBase/algorithm.hpp>
#include <WFramework/Core/WException.h>

namespace white {
	/*!
	\throw LoggedEvent ��Χ���ʧ�ܡ�
	*/
	//@{
	//! \brief ��鴿����ֵ��ָ�����͵ķ�Χ�ڡ�
	template<typename _tDst, typename _type>
	inline _tDst
		CheckScalar(_type val, const std::string& name = {}, RecordLevel lv = Err)
	{
		using common_t = common_type_t<_tDst, _type>;

		if WB_UNLIKELY(common_t(val) > common_t(std::numeric_limits<_tDst>::max()))
			throw LoggedEvent(name + " value out of range.", lv);
		return _tDst(val);
	}

	//! \brief ���Ǹ�������ֵ��ָ�����͵ķ�Χ�ڡ�
	template<typename _tDst, typename _type>
	inline _tDst
		CheckNonnegativeScalar(_type val, const std::string& name = {},
			RecordLevel lv = Err)
	{
		if (val < 0)
			// XXX: Use more specified exception type.
			throw LoggedEvent("Failed getting nonnegative " + name + " value.", lv);
		return CheckScalar<_tDst>(val, name, lv);
	}

	//! \brief �����������ֵ��ָ�����͵ķ�Χ�ڡ�
	template<typename _tDst, typename _type>
	inline _tDst
		CheckPositiveScalar(_type val, const std::string& name = {},
			RecordLevel lv = Err)
	{
		if (!(0 < val))
			// XXX: Use more specified exception type.
			throw LoggedEvent("Failed getting positive " + name + " value.", lv);
		return CheckScalar<_tDst>(val, name, lv);
	}

	/*!
	\brief �ж� i �Ƿ��ڱ����� [_type(0), b] �С�
	\pre ���ԣ� <tt>_type(0) < b</tt> ��
	*/
	template<typename _type>
	inline WB_STATELESS bool
		IsInClosedInterval(_type i, _type b) wnothrow
	{
		WAssert(_type(0) < b,
			"Zero element as lower bound is not less than upper bound.");
		return !(i < _type(0) || b < i);
	}
	/*!
	\brief �ж� i �Ƿ��ڱ����� [a, b] �С�
	\pre ���ԣ� <tt>a < b</tt> ��
	*/
	template<typename _type>
	inline WB_STATELESS bool
		IsInClosedInterval(_type i, _type a, _type b) wnothrow
	{
		WAssert(a < b, "Lower bound is not less than upper bound.");
		return !(i < a || b < i);
	}

	//@}

	template<typename _tDst, typename _type>
	inline _tDst
		CheckLowerBound(_type val, const std::string& name = {}, RecordLevel lv = Err)
	{
		using namespace white;
		// XXX: See WG21 N3387.
		// TODO: Add and use safe %common_arithmetic_type interface instead?
		using common_t = typename white::common_int_type<_tDst, _type>::type;

		if (!(common_t(val) < common_t(std::numeric_limits<_tDst>::min())))
			return _tDst(val);
		throw
			LoggedEvent("Value of '" + name + "' is less than lower boundary.", lv);
	}

	//! \brief �������������ֵ������ָ�����͵��Ͻ硣
	template<typename _tDst, typename _type>
	inline _tDst
		CheckUpperBound(_type val, const std::string& name = {}, RecordLevel lv = Err)
	{
		using namespace white;
		// XXX: See WG21 N3387.
		// TODO: Add and use safe %common_arithmetic_type interface instead?
		using common_t = typename white::common_int_type<_tDst, _type>::type;

		if ((std::is_signed<common_t>() && std::is_unsigned<_tDst>()
			&& white::integer_width<common_t>() <= white::integer_width<_tDst>())
			|| !(common_t(std::numeric_limits<_tDst>::max()) < common_t(val)))
			return _tDst(val);
		throw LoggedEvent("Value of '" + name + "' is greater than upper boundary.",
			lv);
	}

	//! \brief �������������ֵ��ָ�����͵ķ�Χ�ڡ�
	template<typename _tDst, typename _type>
	inline _tDst
		CheckArithmetic(_type val, const std::string& name = {}, RecordLevel lv = Err)
	{
		return
			CheckUpperBound<_tDst>(CheckLowerBound<_tDst>(val, name, lv), name, lv);
	}


	//! \brief ���Ǹ�����������ֵ��ָ�����͵ķ�Χ�ڡ�
	template<typename _tDst, typename _type>
	inline _tDst
		CheckNonnegative(_type val, const std::string& name = {}, RecordLevel lv = Err)
	{
		if (val < 0)
			// XXX: Use more specified exception type.
			throw LoggedEvent("Failed getting nonnegative " + name + " value.", lv);
		return CheckUpperBound<_tDst>(val, name, lv);
	}
	template<typename _tDst, typename _type>
	inline _tDst
		CheckPositive(_type val, const std::string& name = {}, RecordLevel lv = Err)
	{
		if (!(0 < val))
			// XXX: Use more specified exception type.
			throw LoggedEvent("Failed getting positive " + name + " value.", lv);
		return CheckUpperBound<_tDst>(val, name, lv);
	}

	/*!
	\brief �����ַ���������

	���ڴ洢�����в������ַ���������
	*/
	class WF_API ArgumentsVector final
	{
	public:
		vector<string> Arguments{};

		/*!
		\brief ����ֵ��
		\pre ��Ӷ��ԣ� �ڶ�������Ԫ�ض��Ƿǿ�ָ�롣
		\exception LoggedEvent ����Ĳ�����С�� 0 ��
		\sa CheckNonnegative

		���ò���������ֵ��
		�����ֱ�ָ�����������ͱ�ʾ NTMBS ��ָ�����顣
		*/
		void
			Reset(int, char*[]);
	};


	/*!
	\brief ��ѯ�ڶ�����ָ�����ƵĻ�������д���һ������
	\pre ���ԣ������ǿա�
	\note �����������޸ĵ�һ������
	\return �Ƿ��޸ĵ�һ������
	*/
	WF_API WB_NONNULL(2) bool
		FetchEnvironmentVariable(string&, const char*);
}
#endif
