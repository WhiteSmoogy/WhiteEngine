/*!	\file HostedGUI.h
\ingroup WCLib
\ingroup WCLibLimitedPlatforms
\brief ���� GUI �ӿڡ�
*/

#ifndef WFrameWork_WCLib_HostGUI_h
#define WFrameWork_WCLib_HostGUI_h 1

#include <WBase/operators.hpp>
#include <WBase/type_traits.hpp>
#include <WBase/string.hpp>
#include <WFramework/WCLib/FCommon.h>
#include <WFramework/Adaptor/WAdaptor.h>

namespace platform {
#if WFL_Win32
	using SPos = long;
	using SDst = unsigned long;
#else
	using SPos = white::int16;
	using SDst = white::uint16;
#endif
}

namespace white {
	using platform::SPos;
	using platform::SDst;

	namespace Drawing {

		class Size;
		class Rect;

		/*!
		\brief ��Ļ��Ԫ�顣
		\warning ����������
		*/
		template<typename _type>
		class GBinaryGroup : private equality_comparable<GBinaryGroup<_type>>
		{
			static_assert(is_nothrow_copyable<_type>(),
				"Invalid type found.");

		public:
			static const GBinaryGroup Invalid; //!< ��Ч��������Ļ����ϵ�У�����

			_type X = 0, Y = 0; //!< ������

								/*!
								\brief �޲������졣
								\note ���ʼ����
								*/
			wconstfn DefDeCtor(GBinaryGroup)
				/*!
				\brief ���ƹ��죺Ĭ��ʵ�֡�
				*/
				wconstfn DefDeCopyCtor(GBinaryGroup)
				/*!
				\brief ���죺ʹ�� Size ����
				*/
				explicit wconstfn
				GBinaryGroup(const Size&) wnothrow;
			/*!
			\brief ���죺ʹ�� Rect ����
			*/
			explicit wconstfn
				GBinaryGroup(const Rect&) wnothrow;
			/*!
			\brief ���죺ʹ������������
			\tparam _tScalar1 ��һ�����������͡�
			\tparam _tScalar2 �ڶ������������͡�
			\warning ģ������� _type ���Ų�ͬʱ��ʽת�����ܸı���ţ�����֤Ψһ�����
			*/
			template<typename _tScalar1, typename _tScalar2>
			wconstfn
				GBinaryGroup(_tScalar1 x, _tScalar2 y) wnothrow
				: X(_type(x)), Y(_type(y))
			{}
			/*!
			\brief ���죺ʹ�ô����ԡ�
			\note ʹ�� std::get ȡ��������ȡǰ����������
			*/
			template<typename _tPair>
			wconstfn
				GBinaryGroup(const _tPair& pr) wnothrow
				: X(std::get<0>(pr)), Y(std::get<1>(pr))
			{}

			DefDeCopyAssignment(GBinaryGroup)

				/*!
				\brief �����㣺ȡ�ӷ���Ԫ��
				*/
				wconstfn PDefHOp(GBinaryGroup, -, ) const wnothrow
				ImplRet(GBinaryGroup(-X, -Y))

				/*!
				\brief �ӷ���ֵ��
				*/
				PDefHOp(GBinaryGroup&, +=, const GBinaryGroup& val) wnothrow
				ImplRet(wunseq(X += val.X, Y += val.Y), *this)
				/*!
				\brief ������ֵ��
				*/
				PDefHOp(GBinaryGroup&, -=, const GBinaryGroup& val) wnothrow
				ImplRet(wunseq(X -= val.X, Y -= val.Y), *this)

				wconstfn DefGetter(const wnothrow, _type, X, X)
				wconstfn DefGetter(const wnothrow, _type, Y, Y)

				DefSetter(, _type, X, X)
				DefSetter(, _type, Y, Y)

				/*!
				\brief �ж��Ƿ�����Ԫ�ء�
				*/
				wconstfn DefPred(const wnothrow, Zero, X == 0 && Y == 0)

				/*!
				\brief ѡ��������á�
				\note �ڶ�����Ϊ true ʱѡ���һ����������ѡ��ڶ�������
				*/
				//@{
				PDefH(_type&, GetRef, bool b = true) wnothrow
				ImplRet(b ? X : Y)
				PDefH(const _type&, GetRef, bool b = true) const wnothrow
				ImplRet(b ? X : Y)
				//@}
		};

		//! \relates GBinaryGroup
		//@{
		template<typename _type>
		const GBinaryGroup<_type> GBinaryGroup<_type>::Invalid{
			std::numeric_limits<_type>::lowest(), std::numeric_limits<_type>::lowest() };

		/*!
		\brief �Ƚϣ���Ļ��Ԫ����ȹ�ϵ��
		*/
		template<typename _type>
		wconstfn bool
			operator==(const GBinaryGroup<_type>& x, const GBinaryGroup<_type>& y) wnothrow
		{
			return x.X == y.X && x.Y == y.Y;
		}

		/*!
		\brief �ӷ�����Ļ��Ԫ�顣
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator+(const GBinaryGroup<_type>& x, const GBinaryGroup<_type>& y) wnothrow
		{
			return GBinaryGroup<_type>(x.X + y.X, x.Y + y.Y);
		}

		/*!
		\brief ��������Ļ��Ԫ�顣
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator-(const GBinaryGroup<_type>& x, const GBinaryGroup<_type>& y) wnothrow
		{
			return GBinaryGroup<_type>(x.X - y.X, x.Y - y.Y);
		}

		/*!
		\brief ���ˣ���Ļ��Ԫ�顣
		*/
		template<typename _type, typename _tScalar>
		wconstfn GBinaryGroup<_type>
			operator*(const GBinaryGroup<_type>& val, _tScalar l) wnothrow
		{
			return GBinaryGroup<_type>(val.X * l, val.Y * l);
		}

		/*!
		\brief ת�á�
		*/
		template<class _tBinary>
		wconstfn _tBinary
			Transpose(const _tBinary& obj) wnothrow
		{
			return _tBinary(obj.Y, obj.X);
		}

		//@{
		//! \brief ת�ñ任����ʱ����תֱ�ǡ�
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			TransposeCCW(const GBinaryGroup<_type>& val) wnothrow
		{
			return GBinaryGroup<_type>(val.Y, -val.X);
		}

		//! \brief ת�ñ任��˳ʱ����תֱ�ǡ�
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			TransposeCW(const GBinaryGroup<_type>& val) wnothrow
		{
			return GBinaryGroup<_type>(-val.Y, val.X);
		}
		//@}

		/*!
		\brief ȡ������
		*/
		//@{
		template<size_t _vIdx, typename _type>
		wconstfn _type&
			get(GBinaryGroup<_type>& val)
		{
			static_assert(_vIdx < 2, "Invalid index found.");

			return _vIdx == 0 ? val.X : val.Y;
		}
		template<size_t _vIdx, typename _type>
		wconstfn const _type&
			get(const GBinaryGroup<_type>& val)
		{
			static_assert(_vIdx < 2, "Invalid index found.");

			return _vIdx == 0 ? val.X : val.Y;
		}
		//@}

		/*!
		\brief ת��Ϊ�ַ�����
		\note ʹ�� ADL ��
		*/
		template<typename _type>
		string
			to_string(const GBinaryGroup<_type>& val)
		{
			using white::to_string;

			return quote(to_string(val.X) + ", " + to_string(val.Y), '(', ')');
		}
		//@}


		/*!
		\brief ��Ļ��ά�㣨ֱ�������ʾ����
		*/
		using Point = GBinaryGroup<SPos>;


		/*!
		\brief ��Ļ��ά������ֱ�������ʾ����
		*/
		using Vec = GBinaryGroup<SPos>;


		/*!
		\brief ��Ļ�����С��
		\warning ����������
		*/
		class WF_API Size : private equality_comparable<Size>
		{
		public:
			/*!
			\brief ��Ч����
			*/
			static const Size Invalid;

			SDst Width, Height; //!< ��͸ߡ�

			/*!
			\brief �޲������졣
			\note ���ʼ����
			*/
			wconstfn
				Size() wnothrow
				: Width(0), Height(0)
			{}
			/*!
			\brief ���ƹ��졣
			*/
			wconstfn
				Size(const Size& s) wnothrow
				: Width(s.Width), Height(s.Height)
			{}
			/*!
			\brief ���죺ʹ�� Rect ����
			*/
			explicit wconstfn
				Size(const Rect&) wnothrow;
			/*!
			\brief ���죺ʹ����Ļ��Ԫ�顣
			*/
			template<typename _type>
			explicit wconstfn
				Size(const GBinaryGroup<_type>& val) wnothrow
				: Width(static_cast<SDst>(val.X)), Height(static_cast<SDst>(val.Y))
			{}
			/*!
			\brief ���죺ʹ������������
			*/
			template<typename _tScalar1, typename _tScalar2>
			wconstfn
				Size(_tScalar1 w, _tScalar2 h) wnothrow
				: Width(static_cast<SDst>(w)), Height(static_cast<SDst>(h))
			{}

			DefDeCopyAssignment(Size)

			/*!
			\brief �ж��Ƿ�Ϊ�ջ�ǿա�
			*/
			wconstfn DefBoolNeg(explicit wconstfn, Width != 0 || Height != 0)

			/*!
			\brief ������һ����Ļ�����С�Ľ���
			\note ����ɷ�����Сֵ���졣
			*/
			PDefHOp(Size&, &=, const Size& s) wnothrow
			ImplRet(wunseq(Width = min(Width, s.Width),
				Height = min(Height, s.Height)), *this)

			/*!
			\brief ������һ����Ļ��׼���εĲ���
			\note ����ɷ������ֵ���졣
			*/
			PDefHOp(Size&, |=, const Size& s) wnothrow
			ImplRet(wunseq(Width = max(Width, s.Width),
				Height = max(Height, s.Height)), *this)

			/*!
			\brief ת������Ļ��ά������
			\note ��Width �� Height ������Ϊ����� X �� Y������
			*/
			wconstfn DefCvt(const wnothrow, Vec, Vec(Width, Height))

			/*!
			\brief �ж��Ƿ�Ϊ�߶Σ������������һ����ֵ���� 0 ��
			*/
			wconstfn DefPred(const wnothrow, LineSegment,
				!((Width == 0) ^ (Height == 0)))
			/*!
			\brief �ж��Ƿ�Ϊ���ϸ�Ŀվ������򣺰����վ��κ��߶Ρ�
			*/
			wconstfn DefPred(const wnothrow, UnstrictlyEmpty, Width == 0 || Height == 0)

			/*!
			\brief ѡ��������á�
			\note �ڶ�����Ϊ true ʱѡ���һ����������ѡ��ڶ�������
			*/
			//@{
			PDefH(SDst&, GetRef, bool b = true) wnothrow
			ImplRet(b ? Width : Height)
			PDefH(const SDst&, GetRef, bool b = true) const wnothrow
			ImplRet(b ? Width : Height)
			//@}
		};


		wconstfn PDefHOp(bool, == , const Size& x, const Size& y) wnothrow
			ImplRet(x.Width == y.Width && x.Height == y.Height)

		/*!
		\brief �ӷ���ʹ����Ļ��Ԫ�����Ļ�����С������Ӧ��ӹ�����Ļ��Ԫ�顣
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator+(GBinaryGroup<_type> val, const Size& s) wnothrow
		{
			// XXX: Conversion to '_type' might be implementation-defined.
			return { val.X + _type(s.Width), val.Y + _type(s.Height) };
		}

		/*!
		\brief ������ʹ����Ļ��Ԫ�����Ļ�����С������Ӧ��ӹ�����Ļ��Ԫ�顣
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator-(GBinaryGroup<_type> val, const Size& s) wnothrow
		{
			// XXX: Conversion to '_type' might be implementation-defined.
			return { val.X - _type(s.Width), val.Y - _type(s.Height) };
		}

		class WF_API Rect : private Point, private Size,
			private white::equality_comparable<Rect>
		{
		public:
			/*!
			\brief ��Ч����
			*/
			static const Rect Invalid;

			/*!
			\brief ���ϽǺ����ꡣ
			\sa Point::X
			*/
			using Point::X;
			/*!
			\brief ���Ͻ������ꡣ
			\sa Point::Y
			*/
			using Point::Y;
			/*!
			\brief ����
			\sa Size::Width
			*/
			using Size::Width;
			/*!
			\brief ��
			\sa Size::Height
			*/
			using Size::Height;


			/*!
			\brief �޲������죺Ĭ��ʵ�֡�
			*/
			DefDeCtor(Rect)
				/*!
				\brief ���죺ʹ����Ļ��ά�㡣
				*/
				explicit wconstfn
				Rect(const Point& pt) wnothrow
				: Point(pt), Size()
			{}
			/*!
			\brief ���죺ʹ�� Size ����
			*/
			wconstfn
				Rect(const Size& s) wnothrow
				: Point(), Size(s)
			{}
			/*!
			\brief ���죺ʹ����Ļ��ά��� Size ����
			*/
			wconstfn
				Rect(const Point& pt, const Size& s) wnothrow
				: Point(pt), Size(s)
			{}
			/*!
			\brief ���죺ʹ����Ļ��ά��ͱ�ʾ��������� SDst ֵ��
			*/
			wconstfn
				Rect(const Point& pt, SDst w, SDst h) wnothrow
				: Point(pt.X, pt.Y), Size(w, h)
			{}
			/*!
			\brief ���죺ʹ�ñ�ʾλ�õ����� SPos ֵ�� Size ����
			*/
			wconstfn
				Rect(SPos x, SPos y, const Size& s) wnothrow
				: Point(x, y), Size(s.Width, s.Height)
			{}
			/*!
			\brief ���죺ʹ�ñ�ʾλ�õ����� SPos ֵ�ͱ�ʾ��С������ SDst ֵ��
			*/
			wconstfn
				Rect(SPos x, SPos y, SDst w, SDst h) wnothrow
				: Point(x, y), Size(w, h)
			{}
			/*!
			\brief ���ƹ��죺Ĭ��ʵ�֡�
			*/
			wconstfn DefDeCopyCtor(Rect)

				DefDeCopyAssignment(Rect)
				//@{
				Rect&
				operator=(const Point& pt) wnothrow
			{
				wunseq(X = pt.X, Y = pt.Y);
				return *this;
			}
			Rect&
				operator=(const Size& s) wnothrow
			{
				wunseq(Width = s.Width, Height = s.Height);
				return *this;
			}
			//@}

			/*!
			\brief ������һ����Ļ��׼���εĽ���
			\note ��������Ϊ Rect() ������Ϊ���������������е������Ρ�
			*/
			Rect&
				operator&=(const Rect&) wnothrow;

			/*!
			\brief ������һ����Ļ��׼���εĲ���
			\note ���Ϊ�������������е���С���Ρ�
			*/
			Rect&
				operator|=(const Rect&) wnothrow;

			/*!
			\brief �ж��Ƿ�Ϊ�ա�
			\sa Size::operator!
			*/
			using Size::operator!;

			/*!
			\brief �ж��Ƿ�ǿա�
			\sa Size::bool
			*/
			using Size::operator bool;

			/*!
			\brief �жϵ� (px, py) �Ƿ��ھ����ڻ���ϡ�
			*/
			bool
				Contains(SPos px, SPos py) const wnothrow;
			/*!
			\brief �жϵ� pt �Ƿ��ھ����ڻ���ϡ�
			*/
			PDefH(bool, Contains, const Point& pt) const wnothrow
				ImplRet(Contains(pt.X, pt.Y))
				/*!
				\brief �жϾ����Ƿ��ھ����ڻ���ϡ�
				\note �վ������ǲ���������
				*/
				bool
				Contains(const Rect&) const wnothrow;
			/*!
			\brief �жϵ� (px, py) �Ƿ��ھ����ڡ�
			*/
			bool
				ContainsStrict(SPos px, SPos py) const wnothrow;
			/*!
			\brief �жϵ� pt �Ƿ��ھ����ڡ�
			*/
			PDefH(bool, ContainsStrict, const Point& pt) const wnothrow
				ImplRet(ContainsStrict(pt.X, pt.Y))
				/*!
				\brief �жϾ����Ƿ��ھ����ڻ���ϡ�
				\note �վ������ǲ���������
				*/
				bool
				ContainsStrict(const Rect&) const wnothrow;
			/*!
			\brief �жϾ����Ƿ�Ϊ�߶Σ����Ϳ�������һ����ֵ���� 0 ��
			\sa Size::IsLineSegment
			*/
			using Size::IsLineSegment;
			/*!
			\brief �жϾ����Ƿ�Ϊ���ϸ�Ŀվ������򣺰����վ��κ��߶Ρ�
			\sa Size::IsUnstrictlyEmpty
			*/
			using Size::IsUnstrictlyEmpty;

			// XXX: Conversion to 'SPos' might be implementation-defined.
			wconstfn DefGetter(const wnothrow, SPos, Bottom, Y + SPos(Height))
				/*!
				\brief ȡ���Ͻ�λ�á�
				*/
				wconstfn DefGetter(const wnothrow, const Point&, Point,
					static_cast<const Point&>(*this))
				/*!
				\brief ȡ���Ͻ�λ�����á�
				*/
				DefGetter(wnothrow, Point&, PointRef, static_cast<Point&>(*this))
				// XXX: Conversion to 'SPos' might be implementation-defined.
				wconstfn DefGetter(const wnothrow, SPos, Right, X + SPos(Width))
				/*!
				\brief ȡ��С��
				*/
				wconstfn DefGetter(const wnothrow, const Size&, Size,
					static_cast<const Size&>(*this))
				/*!
				\brief ȡ��С���á�
				*/
				DefGetter(wnothrow, Size&, SizeRef, static_cast<Size&>(*this))
				//@{
				using Point::GetX;
			using Point::GetY;

			using Point::SetX;
			using Point::SetY;
			//@}
		};


		using AlphaType = stdex::octet;
		using MonoType = stdex::octet;
	}
}

#include <WFramework/WCLib/Host.h>
#include <WFramework/Core/WEvent.hpp>
#include <WFramework/Core/WString.h>

#if WFL_Win32
#include <atomic>
#endif

#if WF_Hosted
#if WFL_Win32
//@{
struct HBITMAP__;
struct HBRUSH__;
struct HDC__;
struct HINSTANCE__;
struct HWND__;
using HBITMAP = ::HBITMAP__*;
using HBRUSH = ::HBRUSH__*;
using HDC = ::HDC__*;
using HINSTANCE = ::HINSTANCE__*;
using HWND = ::HWND__*;
#		if WFL_Win64
using LPARAM = long long;
#		else
using LPARAM = long;
#		endif
using LRESULT = ::LPARAM;
using WPARAM = std::uintptr_t;
using WNDPROC = ::LRESULT(__stdcall*)(::HWND, unsigned, ::WPARAM, ::LPARAM);
//@}
//@{
struct tagWNDCLASSW;
struct tagWNDCLASSEXW;
using WNDCLASSW = ::tagWNDCLASSW;
using WNDCLASSEXW = ::tagWNDCLASSEXW;
//@}
#endif


namespace platform_ex
{

#	if WFL_Win32
	using NativeWindowHandle = ::HWND;
	using WindowStyle = unsigned long;
#	endif


	//! \warning ����������
	//@{
	class WF_API HostWindowDelete
	{
	public:
		using pointer = NativeWindowHandle;

		void
			operator()(pointer) const wnothrow;
	};
	//@}

	//@{
	/*!
	\typedef MessageID
	\brief �û�������Ϣ���͡�
	*/
	/*!
	\typedef MessageHandler
	\brief �û�������Ϣ��Ӧ�������͡�
	\note ʹ�� XCB ��ƽ̨��ʵ�ʲ�������ͬ <tt>::xcb_generic_event_t*</tt> ��
	*/

#	if WFL_HostedUI_XCB
	using MessageID = std::uint8_t;
	using MessageHandler = void(void*);
#	elif WFL_Win32
	using MessageID = unsigned;
	using MessageHandler = void(::WPARAM, ::LPARAM, ::LRESULT&);
#	endif
	//@}

#	if WFL_HostedUI_XCB || WFL_Win32
	/*!
	\brief ������Ϣת���¼�ӳ�䡣
	*/
	using MessageMap = map<MessageID, white::GEvent<MessageHandler>>;
#	endif

#	if WFL_Win32
	/*!
	\brief ���ʹ��ָ�����ȼ����� ::DefWindowProcW ���� Windows ��Ϣ�Ĵ�������
	\relates MessageMap
	\todo ������ֵ��
	*/
	WF_API void
		BindDefaultWindowProc(NativeWindowHandle, MessageMap&, unsigned,
			white::EventPriority = 0);
#	endif


	/*!
	\brief �����������á�
	\note ����������Ȩ��
	\warning ����������
	*/
	class WF_API WindowReference : private nptr<NativeWindowHandle>
	{
	public:
		DefDeCtor(WindowReference)
			using nptr::nptr;
		DefDeCopyMoveCtorAssignment(WindowReference)

#	if WFL_Win32
			bool
			IsMaximized() const wnothrow;
		bool
			IsMinimized() const wnothrow;
		bool
			IsVisible() const wnothrow;
#	endif

#	if WFL_HostedUI_XCB
		//@{
		DefGetterMem(const, white::Drawing::Rect, Bounds, Deref())
			DefGetterMem(const, white::Drawing::Point, Location, Deref())
			DefGetterMem(const, white::Drawing::Size, Size, Deref())
			//@}
#	elif WFL_Win32
		/*!
		\exception �쳣�������� white::CheckArithmetic �׳���
		*/
		white::Drawing::Rect
			GetBounds() const;
		//@{
		white::Drawing::Rect
			GetClientBounds() const;
		white::Drawing::Point
			GetClientLocation() const;
		white::Drawing::Size
			GetClientSize() const;
		//@}
		white::Drawing::Point
			GetCursorLocation() const;
		white::Drawing::Point
			GetLocation() const;
#	elif WFL_Android
		white::SDst
			GetHeight() const;
#	endif
		DefGetter(const wnothrow, NativeWindowHandle, NativeHandle, get())
#	if WFL_Win32
			/*!
			\brief ȡ��͸���ȡ�
			\pre �������� WS_EX_LAYERED ��ʽ��
			\pre ֮ǰ�����ڴ˴����ϵ��ù� SetOpacity �� ::SetLayeredWindowAttributes ��
			*/
			white::Drawing::AlphaType
			GetOpacity() const;
		WindowReference
			GetParent() const;
		//! \exception �쳣�������� white::CheckArithmetic �׳���
		white::Drawing::Size
			GetSize() const;
#	elif WFL_Android
			DefGetter(const, white::Drawing::Size, Size, { GetWidth(), GetHeight() })
			white::SDst
			GetWidth() const;
#	endif

#	if WFL_HostedUI_XCB
		//@{
		DefSetterMem(, const white::Drawing::Rect&, Bounds, Deref())

			PDefH(void, Close, )
			ImplRet(Deref().Close())

			/*!
			\brief �������ֵ�����ǿ��򷵻����á�
			\throw std::runtime_error ����Ϊ�ա�
			*/
			XCB::WindowData&
			Deref() const;

		PDefH(void, Hide, )
			ImplRet(Deref().Hide())

			PDefH(void, Invalidate, )
			ImplRet(Deref().Invalidate())

			PDefH(void, Move, const white::Drawing::Point& pt)
			ImplRet(Deref().Move(pt))

			PDefH(void, Resize, const white::Drawing::Size& s)
			ImplRet(Deref().Resize(s))
			//@}

			PDefH(void, Show, )
			ImplRet(Deref().Show())
#	elif WFL_Win32
		/*!
		\brief ������ָ���Ŀͻ����߽����ô��ڱ߽硣
		\note �̰߳�ȫ��
		*/
		void
			SetBounds(const white::Drawing::Rect&);
		/*!
		\brief ������ָ���Ŀͻ����߽����ô��ڱ߽硣
		\exception �쳣�������� white::CheckArithmetic �׳���
		*/
		void
			SetClientBounds(const white::Drawing::Rect&);
		/*!
		\brief ���ò�͸���ȡ�
		\pre �������� WS_EX_LAYERED ��ʽ��
		*/
		void
			SetOpacity(white::Drawing::AlphaType);
		/*!
		\brief ���ñ��������֡�
		*/
		void
			SetText(const wchar_t*);

		//! \note �̰߳�ȫ��
		void
			Close();

		/*!
		\brief ��Ч�����ڿͻ�����
		*/
		void
			Invalidate();

		/*!
		\brief �ƶ����ڡ�
		\note �̰߳�ȫ��
		*/
		void
			Move(const white::Drawing::Point&);
		/*!
		\brief ������ָ���Ŀͻ���λ���ƶ����ڡ�
		*/
		void
			MoveClient(const white::Drawing::Point&);

		/*!
		\brief �������ڴ�С��
		\note �̰߳�ȫ��
		*/
		void
			Resize(const white::Drawing::Size&);

		/*!
		\brief ������ָ���Ŀͻ�����С�������ڴ�С��
		\exception �쳣�������� white::CheckArithmetic �׳���
		\note �̰߳�ȫ��
		*/
		void
			ResizeClient(const white::Drawing::Size&);

		/*!
		\brief ��ʾ���ڡ�
		\note ʹ�� <tt>::ShowWindowAsync</tt> ʵ�֣����������ã�ֱ�Ӵ��ݲ�����
		\note Ĭ�ϲ���Ϊ \c SW_SHOWNORMAL ��ָ�������ڱ���С����ָ��Ҽ���ڡ�
		\return �첽�����Ƿ�ɹ���
		*/
		bool
			Show(int = 1) wnothrow;
#	endif

	protected:
		using nptr::get_ref;
	};


#	if WFL_HostedUI_XCB || WFL_Android
	/*!
	\brief ����ָ��ͼ�νӿ������ĵ������ڡ�
	\pre ��Ӷ��ԣ���������ǿա�
	*/
	WF_API void
		UpdateContentTo(NativeWindowHandle, const white::Drawing::Rect&,
			const white::Drawing::ConstGraphics&);
#	elif WFL_Win32
	
	/*!
	\brief ��ָ�������������ͻ�����С�������ı�����ʽ�͸�����ʽ���������������ڡ�
	\note ����Ĭ�ϲ����ֱ�Ϊ \c WS_POPUP �� \c WS_EX_LTRREADING ��
	\exception �쳣�������� white::CheckArithmetic �׳���
	*/
	WF_API NativeWindowHandle
		CreateNativeWindow(const wchar_t*, const white::Drawing::Size&, const wchar_t*
			= L"", WindowStyle = 0x80000000L, WindowStyle = 0x00000000L);
#	endif


#	if WFL_HostedUI_XCB || WFL_Android
	/*!
	\brief ��Ļ�������ݡ�
	\note �ǹ���ʵ�֡�
	*/
	class ScreenBufferData;
#	endif

#if WFL_Win32
	/*!
	\brief �����ࡣ
	*/
	class WF_API WindowClass final : private white::noncopyable
	{
	private:
		wstring name;
		unsigned short atom;
		::HINSTANCE h_instance;

	public:
		/*!
		*/
		//@{
		//! \throw Win32Exception ������ע��ʧ�ܡ�
		//@{
		/*!
		\pre ��Ӷ��ԣ���һ�����ǿա�
		\note Ӧ�ó���ʵ���������Ϊ����ʹ�� <tt>::GetModuleHandleW()</tt> ��
		\note ���ڹ��̲���Ϊ��ʱ��Ϊ HostWindow::WindowProcedure ��
		\note Ĭ�ϻ�ˢ�������� <tt>::HBRUSH(COLOR_MENU + 1)</tt> ��
		\sa HostWindow::WindowProcedure
		*/
		WB_NONNULL(1)
			WindowClass(const wchar_t*, ::WNDPROC = {}, unsigned = 0,
				::HBRUSH = ::HBRUSH(4 + 1), ::HINSTANCE = {});
		WindowClass(const ::WNDCLASSW&);
		WindowClass(const ::WNDCLASSEXW&);
		//@}
		/*!
		\pre ��Ӷ��ԣ���һ����������ָ��ǿա�
		\pre ԭ�ӱ�ʾ��ע��Ĵ����ࡣ
		\pre ʵ�������ע��ʱʹ�õ�ֵ��ȡ�
		\throw std::invalid_argument ԭ��ֵ���� \c 0 ��
		\note ʹ��ָ�����ƺ�ԭ�Ӳ�ȡ�ô����������Ȩ�����Ʋ�Ӱ��ԭ�ӡ�
		*/
		WindowClass(wstring_view, unsigned short, ::HINSTANCE);
		//@}
		~WindowClass();

		//@{
		DefGetter(const wnothrow, unsigned short, Atom, atom)
			DefGetter(const wnothrow, ::HINSTANCE, InstanceHandle, h_instance)
			//@}
			DefGetter(const wnothrow, const wstring&, Name, name)
	};

	wconstexpr const wchar_t WindowClassName[]{ L"WFramework Window" };
#endif



	/*!
	\brief �������ڡ�
	\note Android ƽ̨���������ü�����
	\invariant <tt>bool(GetNativeHandle())</tt> ��
	*/
	class WF_API HostWindow : private WindowReference, private white::noncopyable
	{
	public:
#	if WFL_HostedUI_XCB
		const XCB::Atom::NativeType WM_PROTOCOLS, WM_DELETE_WINDOW;
#	endif
#	if WFL_HostedUI_XCB || WFL_Win32
		/*!
		\brief ������Ϣת���¼�ӳ�䡣
		*/
		platform_ex::MessageMap MessageMap;
#	endif

		/*!
		\brief ʹ��ָ�����������ʼ���������ڡ�
		\pre ��Ӷ��ԣ�����ǿա�
		\pre ʹ�� XCB ��ƽ̨����Ӷ��ԣ�����ǿա�
		\pre ʹ�� XCB ��ƽ̨�����ͨ�� <tt>new XCB::WindowData</tt> �õ���
		\pre Win32 ƽ̨�����ԣ������Ч��
		\pre Win32 ƽ̨�����ԣ������ʾ�Ĵ����ڱ��߳��ϴ�����
		\pre Win32 ƽ̨�����ԣ� \c GWLP_USERDATA ���ݵ��� \c 0 ��
		\throw GeneralEvent ʹ�� XCB ��ƽ̨�����ڴ����� XCB ���ӷ�������
		\throw GeneralEvent Win32 ƽ̨�������������� WindowClassName ��

		���������ʼ���������ڲ�ȡ������Ȩ���� Win32 ƽ̨��ʼ�� HID ������Ϣ��ע��
		\c WM_DESTROY ��Ϣ��ӦΪ���� <tt>::PostQuitMessage(0)</tt> ��
		*/
		HostWindow(NativeWindowHandle);
		DefDelMoveCtor(HostWindow)
			virtual
			~HostWindow();

#	if WFL_Win32
		using WindowReference::IsMaximized;
		using WindowReference::IsMinimized;
		using WindowReference::IsVisible;
#	endif

#	if WFL_HostedUI_XCB
		//@{
		using WindowReference::GetBounds;
		using WindowReference::GetLocation;

		using WindowReference::SetBounds;
		//@}
#	elif WFL_Win32
		using WindowReference::GetBounds;
		//@{
		using WindowReference::GetClientBounds;
		using WindowReference::GetClientLocation;
		using WindowReference::GetClientSize;
		//@}
		using WindowReference::GetCursorLocation;
		using WindowReference::GetLocation;
#	elif WFL_Android
		using WindowReference::GetHeight;
#	endif
		//@{
		using WindowReference::GetNativeHandle;
#	if WFL_Win32
		using WindowReference::GetOpacity;
		using WindowReference::GetParent;
#	endif
		using WindowReference::GetSize;
#	if WFL_Android
		using WindowReference::GetWidth;
#	endif

#	if WFL_Win32
		using WindowReference::SetBounds;
		using WindowReference::SetClientBounds;
		using WindowReference::SetOpacity;
		using WindowReference::SetText;
#	endif

#	if WFL_HostedUI_XCB || WFL_Win32
		using WindowReference::Close;
		//@}

		using WindowReference::Invalidate;

#		if WFL_Win32
		/*!
		\brief ȡ��Դ��ڵĿ���Ӧ����ĵ��λ�á�
		\note Ĭ������߽�Ϊ�ͻ���������������Ϊ��Ч��ʵ��Ϊֱ�ӷ��ز�����
		\return ��������ʾ��λ����Ч�� white::Drawing::Point::Invalie ��
		����Ϊ��Դ�������߽�ĵ�ǰ������ꡣ
		*/
		virtual white::Drawing::Point
			MapPoint(const white::Drawing::Point&) const;
#		endif

		//@{
		using WindowReference::Move;

#		if WFL_Win32
		using WindowReference::MoveClient;

		using WindowReference::Resize;

		using WindowReference::ResizeClient;
#		endif

		using WindowReference::Show;
		//@}
#		if WFL_Win32

		/*!
		\brief ���ڹ��̡�
		\pre ���ھ���ǿգ���Ӧ�� GWLP_USERDATA ��Ϊ��֤�ص�ʱ�ɷ��ʵ� HostWindow ָ�룬
		���ֵ��
		���ʴ��ھ����Ӧ�� GWLP_USERDATA �򣬵��洢��ֵ�ǿ�ʱ��Ϊ HostWindow ��ָ��ֵ��
		����������Ϣӳ��ַ���ִ����Ϣ�����򣬵���Ĭ�ϴ��ڴ�����̡�
		*/
		static ::LRESULT __stdcall
			WindowProcedure(::HWND, unsigned, ::WPARAM, ::LPARAM) wnothrowv;
#		endif
#	endif
	};


	/*!
	\brief ��������������
	\warning ����������
	*/
	class WF_API WindowInputHost
	{
	public:
		HostWindow& Window;

#	if WFL_Win32
	public:
		//! \brief �������롣
		std::atomic<short> RawMouseButton{ 0 };

	private:
		/*!
		\brief ��ʶ�����������
		\see https://src.chromium.org/viewvc/chrome/trunk/src/ui/base/ime/win/imm32_manager.cc
		IMM32Manager::CreateImeWindow ��ע�͡�
		*/
		bool has_hosted_caret;
		//! \brief ��������ַ�������
		white::recursive_mutex input_mutex{};
		//! \brief ���뷨����ַ�����
		white::String comp_str{};
		//! \brief ��Դ��ڵ����������λ�û��档
		white::Drawing::Point caret_location{ white::Drawing::Point::Invalid };
#	endif
	public:
		//! \brief ���죺ʹ��ָ���������ã������ʼ������������Ϣӳ�䡣
		WindowInputHost(HostWindow&);
		~WindowInputHost();

#if WFL_Win32
		/*!
		\brief �������뷨״̬��
		\note �̰߳�ȫ��������ʡ�
		*/
		template<typename _func>
		auto
			AccessInputString(_func f) -> decltype(f(comp_str))
		{
			using namespace white;
			lock_guard<recursive_mutex> lck(input_mutex);

			return f(comp_str);
		}

		/*!
		\brief �������뷨�༭����ѡ����λ�á�
		\note λ��Ϊ��Դ��ڿͻ��������ꡣ
		\note ��λ��Ϊ Drawing::Point::Invalid ����ԡ�
		\sa caret_location
		*/
		//@{
		//! \note �̰߳�ȫ��
		//@{
		//! \note ȡ�����λ�á�
		void
			UpdateCandidateWindowLocation();
		//! \note �������������»��档
		void
			UpdateCandidateWindowLocation(const white::Drawing::Point&);
		//@}

		//! \note �����汾�������ڲ�ʵ�֡�
		void
			UpdateCandidateWindowLocationUnlocked();
		//@}
#endif
	};


#	if WFL_Win32
	/*!
	\brief �򿪵ļ�����ʵ����
	\todo �� Win32 ����ƽ̨ʵ�֡�
	*/
	class WF_API Clipboard : private white::noncopyable, private white::nonmovable
	{
	private:
		class Data;

	public:
		//! \brief ��ʽ��ʶ���͡�
		using FormatType = unsigned;

		Clipboard(NativeWindowHandle);
		~Clipboard();

		/*!
		\brief �ж�ָ����ʽ�Ƿ���á�
		\note Win32 ƽ̨��������ʱ���ܻ�ı� <tt>::GetLastError()</tt> �Ľ����
		*/
		static bool
			IsAvailable(FormatType) wnothrow;

		/*!
		\brief ���ָ����ʽ�Ƿ���ã����ɹ��������á�
		\throw Win32Exception ֵ�����á�
		\sa IsAvailable
		*/
		static void
			CheckAvailable(FormatType);

		//! \brief ������������ݡ�
		void
			Clear() wnothrow;

		//! \brief ���ص�ǰ�򿪼����岢�����Ĵ��ھ����
		static NativeWindowHandle
			GetOpenWindow() wnothrow;

		bool
			Receive(white::string&);
		bool
			Receive(white::String&);

	private:
		bool
			ReceiveRaw(FormatType, std::function<void(const Data&)>);

	public:
		/*!
		\pre ���ԣ��ַ�������������ָ��ǿա�
		*/
		//@{
		void
			Send(string_view);
		void
			Send(u16string_view);
		//@}

	private:
		void
			SendRaw(FormatType, void*);
	};


	/*!
	\brief ִ�� Shell ���
	\pre ��Ӷ��ԣ���һ�����ǿա�
	\throw LoggedEvent һ�����ʧ�ܡ�
	\throw std::bad_alloc ����ʧ�ܣ��洢����ʧ�ܡ�
	\warning �����ܱ�֤��ʹ�� COM ʱ��Ҫ�� COM ���ʵ���ʼ����
	\warning �����Ӽ�飺����ִ��·��ָ��Ϊ�ĵ�ʱ���������ӦΪ�ա�
	\warning �����Ӽ�飺����ִ��·��Ϊ���·��ʱ������Ŀ¼·����ӦΪ���·����
	\sa https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx

	ʹ�� ::ShellExecuteW ִ�� Shell ���
	�����ֱ��ʾ����ִ��·��������������Ƿ�ʹ�ù���ԱȨ��ִ�С�����Ŀ¼·����
	ʹ�õĳ�ʼ����ѡ�Ĭ��Ϊ \c SW_SHOWNORMAL �����Լ������ĸ����ھ����
	::ShellExecuteW �ķ���ֵ�������� 32 ��Ϊ���󡣺� Win32 ������ֱ�Ӷ�Ӧ��ȵķ���ֵ
	���׳�Ϊ Win32 �쳣��
	���� 0 �� SE_ERR_OOM ʱ�׳� std::bad_alloc ��
	δ���ĵ���ȷ�Ĵ�����Ϊδ֪�����׳� LoggedEvent ��
	��������ֵ������ӳ����׳� Win32 �쳣��
	���� SE_ERR_SHARE ӳ��Ϊ Win32 ������ ERROR_SHARING_VIOLATION ��
	���� SE_ERR_DLLNOTFOUND ӳ��Ϊ Win32 ������ ERROR_DLL_NOT_FOUND ��
	���� SE_ERR_ASSOCINCOMPLETE �� SE_ERR_NOASSOC ӳ��Ϊ Win32 ������
	ERROR_NO_ASSOCIATION ��
	���� SE_ERR_DDETIMEOUT �� SE_ERR_DDEFAIL �� SE_ERR_DDEBUSY ӳ��Ϊ Win32 ������
	ERROR_DDE_FAIL ��
	�Զ������ֵӳ��Ĵ����룬�׳����쳣��Ƕ���쳣��
	��ײ��쳣Ϊ ystdex::wrap_mixin_t<std::runtime_error, int> ���͡�
	*/
	WF_API WB_NONNULL(1) void
		ExecuteShellCommand(const wchar_t*, const wchar_t* = {}, bool = {},
			const wchar_t* = {}, int = 1, NativeWindowHandle = {});
#	endif

} // namespace platform_ex;


#endif

#endif