#ifndef LBase_CHRLib_CharacterMapping_h
#define LBase_CHRLib_CharacterMapping_h 1

#include "WBase/wmacro.h"
#include "WFramework/CHRLib/Encoding.h"
#include "WBase/cassert.h"

namespace CHRLib
{
	using CharSet::Encoding;
	

	/*!
	\brief Ĭ���ַ����롣
	*/
	wconstexpr const Encoding CS_Default = CharSet::UTF_8;


	/*!
	\ingroup metafunctions
	\brief ȡָ���ַ����Ͷ�Ӧ��Ĭ�϶������롣
	*/
	//@{
	template<typename _tChar>
	struct MapFixedEncoding : std::integral_constant<Encoding, CS_Default>
	{};

	template<>
	struct MapFixedEncoding<ucs2_t>
		: std::integral_constant<Encoding, CharSet::csUnicode>
	{};

	template<>
	struct MapFixedEncoding<ucs4_t>
		: std::integral_constant<Encoding, CharSet::csUCS4>
	{};
	//@}


	/*!
	\brief ȡ c_ptr ָ��Ĵ����˫�ֽ��ַ���
	\pre ���ԣ� \c c_ptr ��
	*/
	inline PDefH(ucs2_t, FetchBiCharBE, const char* c_ptr)
		ImplRet(wconstraint(c_ptr), ucs2_t((*c_ptr << CHAR_BIT) | c_ptr[1]))

		/*!
		\brief ȡ c_ptr ָ���С����˫�ֽ��ַ���
		\pre ���ԣ� \c c_ptr ��
		*/
		inline PDefH(ucs2_t, FetchBiCharLE, const char* c_ptr)
		ImplRet(wconstraint(c_ptr), ucs2_t((c_ptr[1] << CHAR_BIT) | *c_ptr))


		/*!
		\brief ����ת�������
		*/
	enum class ConversionResult
	{
		OK = 0, //!< ת���ɹ���
		BadState, //!< ת��״̬����
		BadSource, //!< Դ���ݲ��ɴ��Խ�磩��
		Invalid, //!< ����У��ʧ�ܣ��粻���ɴ������ֽ����У���
		Unhandled //!< δ��������������Ľ��ޣ���
	};


	/*!
	\brief ����ת��״̬��
	*/
	struct ConversionState
	{
		/*!
		\brief ��ǰ״̬������
		*/
		std::uint_fast8_t Index;
		union
		{
			ucsint_t Wide;
			ucs4_t UCS4;
			/*!
			\brief �ֽ����У����ַ����ֽڱ�ʾ��
			*/
			byte Sequence[sizeof(ucsint_t)];
		} Value;

		wconstfn
			ConversionState(std::size_t n = 0)
			: Index(std::uint_fast8_t(n)), Value()
		{}
	};

	//! \relates ConversionState
	//@{
	wconstfn PDefH(std::uint_fast8_t&, GetIndexOf, ConversionState& st)
		ImplRet(st.Index)
		wconstfn PDefH(ucsint_t&, GetWideOf, ConversionState& st)
		ImplRet(st.Value.Wide)
		wconstfn PDefH(byte*, GetSequenceOf, ConversionState& st)
		ImplRet(st.Value.Sequence)
		//@}


		/*!
		\brief ȡһ��״̬����������
		*/
		template<typename _type>
	wconstfn _type&
		GetIndexOf(_type& st)
	{
		return st;
	}


	/*!
	\brief ȡָ���̶�����Ĺ̶��ַ���ȡ�
	\return δ��������䳤���뷵�� 0 ������Ϊָ��������ÿ���ַ�ռ�õ��ֽ�����
	\note UTF-16 ��Ϊ UCS-2 ��
	*/
	WB_API std::size_t
		FetchFixedCharWidth(Encoding);

	/*!
	\brief ȡָ�����������ַ���ȡ�
	\return δ������뷵�� 0 ������Ϊָ��������ÿ���ַ�������ռ�õ��ֽ�����
	\note UTF-16 ��Ϊ UCS-2 ��
	*/
	WB_API std::size_t
		FetchMaxCharWidth(Encoding);

	/*!
	\brief ȡָ���䳤���������ַ���ȡ�
	\return δ��������̶����뷵�� 0 ������Ϊָ��������ÿ���ַ�������ռ�õ��ֽ�����
	\note UTF-16 ��Ϊ UCS-2 ��
	*/
	WB_API std::size_t
		FetchMaxVariantCharWidth(Encoding);
}

#endif
