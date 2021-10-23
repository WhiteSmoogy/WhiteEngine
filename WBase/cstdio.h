#ifndef WBase_cstdio_h
#define WBase_cstdio_h 1

#include "sutility.h" // for noncopyable;
#include "cassert.h" // for wdef.h, <cstdio> and wconstraint;
#include "iterator_op.hpp" // for iterator_operators_t, is_undereferenceable;

#include <cstdarg> // for std::va_list;
#include <memory> // for std::unique_ptr;
#include <ios> // for std::ios_base::openmode;

namespace white
{
	using stdex::byte;

	/*!
	\brief �ر������塣
	\pre �����ǿա�
	*/
	inline int
		setnbuf(std::FILE* stream) wnothrow
	{
		wconstraint(stream);
		return std::setvbuf(stream, {}, _IONBF, 0);
	}

	/*!
	\brief ���� ISO C ��׼�����ֻ����������
	*/
	class WB_API ifile_iterator : public std::iterator<std::input_iterator_tag,
		byte, ptrdiff_t, const byte*, const byte&>, public white::iterator_operators_t<
		ifile_iterator, std::iterator_traits<wimpl(std::iterator<
			std::input_iterator_tag, byte, ptrdiff_t, const byte*, const byte&>) >>
	{
	protected:
		using traits_type = std::iterator<std::input_iterator_tag, byte, ptrdiff_t,
			const byte*, const byte&>;

	public:
		using char_type = byte;
		using int_type = int;

	private:
		/*!
		*/
		std::FILE* stream{};
		char_type value;

	public:
		/*!
		\brief �޲������졣
		\post <tt>!stream</tt> ��

		���������������
		*/
		wconstfn
			ifile_iterator()
			: value()
		{}
		/*!
		\brief ���죺ʹ����ָ�롣
		\pre ���ԣ� <tt>ptr</tt> ��
		\post <tt>stream == ptr</tt> ��
		*/
		explicit
			ifile_iterator(std::FILE* ptr)
			: stream(ptr)
		{
			wconstraint(ptr);
			++*this;
		}
		/*!
		\brief ���ƹ��죺Ĭ��ʵ�֡�
		*/
		wconstfn
			ifile_iterator(const ifile_iterator&) = default;
		~ifile_iterator() = default;

		ifile_iterator&
			operator=(const ifile_iterator&) = default;

		wconstfn reference
			operator*() const wnothrow
		{
			return value;
		}

		/*
		\brief ǰ��������
		\pre ���ԣ���ָ��ǿա�
		\return �������á�
		\note ������ EOF ʱ����ָ��Ϊ��ָ�롣

		ʹ�� std::fgetc ���ַ���
		*/
		ifile_iterator&
			operator++();

		friend bool
			operator==(const ifile_iterator& x, const ifile_iterator& y)
		{
			return x.stream == y.stream;
		}

		wconstfn std::FILE*
			get_stream() const
		{
			return stream;
		}

		/*!
		\brief ������д���ַ���
		*/
		//@{
		//! \pre ���ԣ� <tt>!stream</tt> ��
		int_type
			sputbackc(char_type c)
		{
			wconstraint(stream);
			return std::ungetc(c, stream);
		}
		//! \pre ���ԣ� <tt>!stream || steram == s</tt> ��
		int_type
			sputbackc(char_type c, std::FILE* s)
		{
			wconstraint(!stream || stream == s);
			stream = s;
			return sputbackc(c);
		}

		//! \pre ��Ӷ��ԣ� <tt>!stream</tt> ��
		int_type
			sungetc()
		{
			return sputbackc(value);
		}
		//! \pre ��Ӷ��ԣ� <tt>!stream || steram == s</tt> ��
		int_type
			sungetc(std::FILE* s)
		{
			return sputbackc(value, s);
		}
		//@}
	};


	/*!
	\ingroup is_undereferenceable
	\brief �ж� ifile_iterator ʵ���Ƿ�ȷ��Ϊ���ɽ����á�
	*/
	inline bool
		is_undereferenceable(const ifile_iterator& i) wnothrow
	{
		return !i.get_stream();
	}

	/*!
	\brief ISO C/C++ ��׼��������ӿڴ�ģʽת����
	*/
	//@{
	/*!
	\see ISO C++11 Table 132 ��
	\note ���� std::ios_base::ate ��
	\see http://wg21.cmeerw.net/lwg/issue596 ��
	*/
	WB_API WB_PURE const char*
		openmode_conv(std::ios_base::openmode) wnothrow;
	/*!
	\brief ISO C/C++ ��׼��������ӿڴ�ģʽת����
	\return ��ʧ�ܣ������ղ������Σ�Ϊ std::ios_base::openmode() ������Ϊ��Ӧ��ֵ��
	\see ISO C11 7.21.5.3/3 ��
	\note ˳���ϸ��޶���
	\note ֧�� x ת����
	*/
	WB_API WB_PURE std::ios_base::openmode
		openmode_conv(const char*) wnothrow;
	//@}
}


#endif
