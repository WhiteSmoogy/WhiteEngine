/*!	\file HostedGUI.h
\ingroup WCLib
\ingroup WCLibLimitedPlatforms
\brief 宿主 GUI 接口。
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
		\brief 屏幕二元组。
		\warning 非虚析构。
		*/
		template<typename _type>
		class GBinaryGroup : private equality_comparable<GBinaryGroup<_type>>
		{
			static_assert(is_nothrow_copyable<_type>(),
				"Invalid type found.");

		public:
			static const GBinaryGroup Invalid; //!< 无效（不在屏幕坐标系中）对象。

			_type X = 0, Y = 0; //!< 分量。

								/*!
								\brief 无参数构造。
								\note 零初始化。
								*/
			wconstfn DefDeCtor(GBinaryGroup)
				/*!
				\brief 复制构造：默认实现。
				*/
				wconstfn DefDeCopyCtor(GBinaryGroup)
				/*!
				\brief 构造：使用 Size 对象。
				*/
				explicit wconstfn
				GBinaryGroup(const Size&) wnothrow;
			/*!
			\brief 构造：使用 Rect 对象。
			*/
			explicit wconstfn
				GBinaryGroup(const Rect&) wnothrow;
			/*!
			\brief 构造：使用两个纯量。
			\tparam _tScalar1 第一分量纯量类型。
			\tparam _tScalar2 第二分量纯量类型。
			\warning 模板参数和 _type 符号不同时隐式转换可能改变符号，不保证唯一结果。
			*/
			template<typename _tScalar1, typename _tScalar2>
			wconstfn
				GBinaryGroup(_tScalar1 x, _tScalar2 y) wnothrow
				: X(_type(x)), Y(_type(y))
			{}
			/*!
			\brief 构造：使用纯量对。
			\note 使用 std::get 取分量。仅取前两个分量。
			*/
			template<typename _tPair>
			wconstfn
				GBinaryGroup(const _tPair& pr) wnothrow
				: X(std::get<0>(pr)), Y(std::get<1>(pr))
			{}

			DefDeCopyAssignment(GBinaryGroup)

				/*!
				\brief 负运算：取加法逆元。
				*/
				wconstfn PDefHOp(GBinaryGroup, -, ) const wnothrow
				ImplRet(GBinaryGroup(-X, -Y))

				/*!
				\brief 加法赋值。
				*/
				PDefHOp(GBinaryGroup&, +=, const GBinaryGroup& val) wnothrow
				ImplRet(wunseq(X += val.X, Y += val.Y), *this)
				/*!
				\brief 减法赋值。
				*/
				PDefHOp(GBinaryGroup&, -=, const GBinaryGroup& val) wnothrow
				ImplRet(wunseq(X -= val.X, Y -= val.Y), *this)

				wconstfn DefGetter(const wnothrow, _type, X, X)
				wconstfn DefGetter(const wnothrow, _type, Y, Y)

				DefSetter(, _type, X, X)
				DefSetter(, _type, Y, Y)

				/*!
				\brief 判断是否是零元素。
				*/
				wconstfn DefPred(const wnothrow, Zero, X == 0 && Y == 0)

				/*!
				\brief 选择分量引用。
				\note 第二参数为 true 时选择第一分量，否则选择第二分量。
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
		\brief 比较：屏幕二元组相等关系。
		*/
		template<typename _type>
		wconstfn bool
			operator==(const GBinaryGroup<_type>& x, const GBinaryGroup<_type>& y) wnothrow
		{
			return x.X == y.X && x.Y == y.Y;
		}

		/*!
		\brief 加法：屏幕二元组。
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator+(const GBinaryGroup<_type>& x, const GBinaryGroup<_type>& y) wnothrow
		{
			return GBinaryGroup<_type>(x.X + y.X, x.Y + y.Y);
		}

		/*!
		\brief 减法：屏幕二元组。
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator-(const GBinaryGroup<_type>& x, const GBinaryGroup<_type>& y) wnothrow
		{
			return GBinaryGroup<_type>(x.X - y.X, x.Y - y.Y);
		}

		/*!
		\brief 数乘：屏幕二元组。
		*/
		template<typename _type, typename _tScalar>
		wconstfn GBinaryGroup<_type>
			operator*(const GBinaryGroup<_type>& val, _tScalar l) wnothrow
		{
			return GBinaryGroup<_type>(val.X * l, val.Y * l);
		}

		/*!
		\brief 转置。
		*/
		template<class _tBinary>
		wconstfn _tBinary
			Transpose(const _tBinary& obj) wnothrow
		{
			return _tBinary(obj.Y, obj.X);
		}

		//@{
		//! \brief 转置变换：逆时针旋转直角。
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			TransposeCCW(const GBinaryGroup<_type>& val) wnothrow
		{
			return GBinaryGroup<_type>(val.Y, -val.X);
		}

		//! \brief 转置变换：顺时针旋转直角。
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			TransposeCW(const GBinaryGroup<_type>& val) wnothrow
		{
			return GBinaryGroup<_type>(-val.Y, val.X);
		}
		//@}

		/*!
		\brief 取分量。
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
		\brief 转换为字符串。
		\note 使用 ADL 。
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
		\brief 屏幕二维点（直角坐标表示）。
		*/
		using Point = GBinaryGroup<SPos>;


		/*!
		\brief 屏幕二维向量（直角坐标表示）。
		*/
		using Vec = GBinaryGroup<SPos>;


		/*!
		\brief 屏幕区域大小。
		\warning 非虚析构。
		*/
		class WF_API Size : private equality_comparable<Size>
		{
		public:
			/*!
			\brief 无效对象。
			*/
			static const Size Invalid;

			SDst Width, Height; //!< 宽和高。

			/*!
			\brief 无参数构造。
			\note 零初始化。
			*/
			wconstfn
				Size() wnothrow
				: Width(0), Height(0)
			{}
			/*!
			\brief 复制构造。
			*/
			wconstfn
				Size(const Size& s) wnothrow
				: Width(s.Width), Height(s.Height)
			{}
			/*!
			\brief 构造：使用 Rect 对象。
			*/
			explicit wconstfn
				Size(const Rect&) wnothrow;
			/*!
			\brief 构造：使用屏幕二元组。
			*/
			template<typename _type>
			explicit wconstfn
				Size(const GBinaryGroup<_type>& val) wnothrow
				: Width(static_cast<SDst>(val.X)), Height(static_cast<SDst>(val.Y))
			{}
			/*!
			\brief 构造：使用两个纯量。
			*/
			template<typename _tScalar1, typename _tScalar2>
			wconstfn
				Size(_tScalar1 w, _tScalar2 h) wnothrow
				: Width(static_cast<SDst>(w)), Height(static_cast<SDst>(h))
			{}

			DefDeCopyAssignment(Size)

			/*!
			\brief 判断是否为空或非空。
			*/
			wconstfn DefBoolNeg(explicit wconstfn, Width != 0 || Height != 0)

			/*!
			\brief 求与另一个屏幕区域大小的交。
			\note 结果由分量最小值构造。
			*/
			PDefHOp(Size&, &=, const Size& s) wnothrow
			ImplRet(wunseq(Width = min(Width, s.Width),
				Height = min(Height, s.Height)), *this)

			/*!
			\brief 求与另一个屏幕标准矩形的并。
			\note 结果由分量最大值构造。
			*/
			PDefHOp(Size&, |=, const Size& s) wnothrow
			ImplRet(wunseq(Width = max(Width, s.Width),
				Height = max(Height, s.Height)), *this)

			/*!
			\brief 转换：屏幕二维向量。
			\note 以Width 和 Height 分量作为结果的 X 和 Y分量。
			*/
			wconstfn DefCvt(const wnothrow, Vec, Vec(Width, Height))

			/*!
			\brief 判断是否为线段：长或宽中有且一个数值等于 0 。
			*/
			wconstfn DefPred(const wnothrow, LineSegment,
				!((Width == 0) ^ (Height == 0)))
			/*!
			\brief 判断是否为不严格的空矩形区域：包括空矩形和线段。
			*/
			wconstfn DefPred(const wnothrow, UnstrictlyEmpty, Width == 0 || Height == 0)

			/*!
			\brief 选择分量引用。
			\note 第二参数为 true 时选择第一分量，否则选择第二分量。
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
		\brief 加法：使用屏幕二元组和屏幕区域大小分量对应相加构造屏幕二元组。
		*/
		template<typename _type>
		wconstfn GBinaryGroup<_type>
			operator+(GBinaryGroup<_type> val, const Size& s) wnothrow
		{
			// XXX: Conversion to '_type' might be implementation-defined.
			return { val.X + _type(s.Width), val.Y + _type(s.Height) };
		}

		/*!
		\brief 减法：使用屏幕二元组和屏幕区域大小分量对应相加构造屏幕二元组。
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
			\brief 无效对象。
			*/
			static const Rect Invalid;

			/*!
			\brief 左上角横坐标。
			\sa Point::X
			*/
			using Point::X;
			/*!
			\brief 左上角纵坐标。
			\sa Point::Y
			*/
			using Point::Y;
			/*!
			\brief 长。
			\sa Size::Width
			*/
			using Size::Width;
			/*!
			\brief 宽。
			\sa Size::Height
			*/
			using Size::Height;


			/*!
			\brief 无参数构造：默认实现。
			*/
			DefDeCtor(Rect)
				/*!
				\brief 构造：使用屏幕二维点。
				*/
				explicit wconstfn
				Rect(const Point& pt) wnothrow
				: Point(pt), Size()
			{}
			/*!
			\brief 构造：使用 Size 对象。
			*/
			wconstfn
				Rect(const Size& s) wnothrow
				: Point(), Size(s)
			{}
			/*!
			\brief 构造：使用屏幕二维点和 Size 对象。
			*/
			wconstfn
				Rect(const Point& pt, const Size& s) wnothrow
				: Point(pt), Size(s)
			{}
			/*!
			\brief 构造：使用屏幕二维点和表示长宽的两个 SDst 值。
			*/
			wconstfn
				Rect(const Point& pt, SDst w, SDst h) wnothrow
				: Point(pt.X, pt.Y), Size(w, h)
			{}
			/*!
			\brief 构造：使用表示位置的两个 SPos 值和 Size 对象。
			*/
			wconstfn
				Rect(SPos x, SPos y, const Size& s) wnothrow
				: Point(x, y), Size(s.Width, s.Height)
			{}
			/*!
			\brief 构造：使用表示位置的两个 SPos 值和表示大小的两个 SDst 值。
			*/
			wconstfn
				Rect(SPos x, SPos y, SDst w, SDst h) wnothrow
				: Point(x, y), Size(w, h)
			{}
			/*!
			\brief 复制构造：默认实现。
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
			\brief 求与另一个屏幕标准矩形的交。
			\note 若相离结果为 Rect() ，否则为包含于两个参数中的最大矩形。
			*/
			Rect&
				operator&=(const Rect&) wnothrow;

			/*!
			\brief 求与另一个屏幕标准矩形的并。
			\note 结果为包含两个参数中的最小矩形。
			*/
			Rect&
				operator|=(const Rect&) wnothrow;

			/*!
			\brief 判断是否为空。
			\sa Size::operator!
			*/
			using Size::operator!;

			/*!
			\brief 判断是否非空。
			\sa Size::bool
			*/
			using Size::operator bool;

			/*!
			\brief 判断点 (px, py) 是否在矩形内或边上。
			*/
			bool
				Contains(SPos px, SPos py) const wnothrow;
			/*!
			\brief 判断点 pt 是否在矩形内或边上。
			*/
			PDefH(bool, Contains, const Point& pt) const wnothrow
				ImplRet(Contains(pt.X, pt.Y))
				/*!
				\brief 判断矩形是否在矩形内或边上。
				\note 空矩形总是不被包含。
				*/
				bool
				Contains(const Rect&) const wnothrow;
			/*!
			\brief 判断点 (px, py) 是否在矩形内。
			*/
			bool
				ContainsStrict(SPos px, SPos py) const wnothrow;
			/*!
			\brief 判断点 pt 是否在矩形内。
			*/
			PDefH(bool, ContainsStrict, const Point& pt) const wnothrow
				ImplRet(ContainsStrict(pt.X, pt.Y))
				/*!
				\brief 判断矩形是否在矩形内或边上。
				\note 空矩形总是不被包含。
				*/
				bool
				ContainsStrict(const Rect&) const wnothrow;
			/*!
			\brief 判断矩形是否为线段：长和宽中有且一个数值等于 0 。
			\sa Size::IsLineSegment
			*/
			using Size::IsLineSegment;
			/*!
			\brief 判断矩形是否为不严格的空矩形区域：包括空矩形和线段。
			\sa Size::IsUnstrictlyEmpty
			*/
			using Size::IsUnstrictlyEmpty;

			// XXX: Conversion to 'SPos' might be implementation-defined.
			wconstfn DefGetter(const wnothrow, SPos, Bottom, Y + SPos(Height))
				/*!
				\brief 取左上角位置。
				*/
				wconstfn DefGetter(const wnothrow, const Point&, Point,
					static_cast<const Point&>(*this))
				/*!
				\brief 取左上角位置引用。
				*/
				DefGetter(wnothrow, Point&, PointRef, static_cast<Point&>(*this))
				// XXX: Conversion to 'SPos' might be implementation-defined.
				wconstfn DefGetter(const wnothrow, SPos, Right, X + SPos(Width))
				/*!
				\brief 取大小。
				*/
				wconstfn DefGetter(const wnothrow, const Size&, Size,
					static_cast<const Size&>(*this))
				/*!
				\brief 取大小引用。
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


	//! \warning 非虚析构。
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
	\brief 用户界面消息类型。
	*/
	/*!
	\typedef MessageHandler
	\brief 用户界面消息响应函数类型。
	\note 使用 XCB 的平台：实际参数类型同 <tt>::xcb_generic_event_t*</tt> 。
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
	\brief 窗口消息转发事件映射。
	*/
	using MessageMap = map<MessageID, white::GEvent<MessageHandler>>;
#	endif

#	if WFL_Win32
	/*!
	\brief 添加使用指定优先级调用 ::DefWindowProcW 处理 Windows 消息的处理器。
	\relates MessageMap
	\todo 处理返回值。
	*/
	WF_API void
		BindDefaultWindowProc(NativeWindowHandle, MessageMap&, unsigned,
			white::EventPriority = 0);
#	endif


	/*!
	\brief 本机窗口引用。
	\note 不具有所有权。
	\warning 非虚析构。
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
		\exception 异常中立：由 white::CheckArithmetic 抛出。
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
			\brief 取不透明度。
			\pre 窗口启用 WS_EX_LAYERED 样式。
			\pre 之前必须在此窗口上调用过 SetOpacity 或 ::SetLayeredWindowAttributes 。
			*/
			white::Drawing::AlphaType
			GetOpacity() const;
		WindowReference
			GetParent() const;
		//! \exception 异常中立：由 white::CheckArithmetic 抛出。
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
			\brief 检查引用值，若非空则返回引用。
			\throw std::runtime_error 引用为空。
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
		\brief 按参数指定的客户区边界设置窗口边界。
		\note 线程安全。
		*/
		void
			SetBounds(const white::Drawing::Rect&);
		/*!
		\brief 按参数指定的客户区边界设置窗口边界。
		\exception 异常中立：由 white::CheckArithmetic 抛出。
		*/
		void
			SetClientBounds(const white::Drawing::Rect&);
		/*!
		\brief 设置不透明度。
		\pre 窗口启用 WS_EX_LAYERED 样式。
		*/
		void
			SetOpacity(white::Drawing::AlphaType);
		/*!
		\brief 设置标题栏文字。
		*/
		void
			SetText(const wchar_t*);

		//! \note 线程安全。
		void
			Close();

		/*!
		\brief 无效化窗口客户区。
		*/
		void
			Invalidate();

		/*!
		\brief 移动窗口。
		\note 线程安全。
		*/
		void
			Move(const white::Drawing::Point&);
		/*!
		\brief 按参数指定的客户区位置移动窗口。
		*/
		void
			MoveClient(const white::Drawing::Point&);

		/*!
		\brief 调整窗口大小。
		\note 线程安全。
		*/
		void
			Resize(const white::Drawing::Size&);

		/*!
		\brief 按参数指定的客户区大小调整窗口大小。
		\exception 异常中立：由 white::CheckArithmetic 抛出。
		\note 线程安全。
		*/
		void
			ResizeClient(const white::Drawing::Size&);

		/*!
		\brief 显示窗口。
		\note 使用 <tt>::ShowWindowAsync</tt> 实现，非阻塞调用，直接传递参数。
		\note 默认参数为 \c SW_SHOWNORMAL ，指定若窗口被最小化则恢复且激活窗口。
		\return 异步操作是否成功。
		*/
		bool
			Show(int = 1) wnothrow;
#	endif

	protected:
		using nptr::get_ref;
	};


#	if WFL_HostedUI_XCB || WFL_Android
	/*!
	\brief 更新指定图形接口上下文的至窗口。
	\pre 间接断言：本机句柄非空。
	*/
	WF_API void
		UpdateContentTo(NativeWindowHandle, const white::Drawing::Rect&,
			const white::Drawing::ConstGraphics&);
#	elif WFL_Win32
	
	/*!
	\brief 按指定窗口类名、客户区大小、标题文本、样式和附加样式创建本机顶级窗口。
	\note 最后的默认参数分别为 \c WS_POPUP 和 \c WS_EX_LTRREADING 。
	\exception 异常中立：由 white::CheckArithmetic 抛出。
	*/
	WF_API NativeWindowHandle
		CreateNativeWindow(const wchar_t*, const white::Drawing::Size&, const wchar_t*
			= L"", WindowStyle = 0x80000000L, WindowStyle = 0x00000000L);
#	endif


#	if WFL_HostedUI_XCB || WFL_Android
	/*!
	\brief 屏幕缓存数据。
	\note 非公开实现。
	*/
	class ScreenBufferData;
#	endif

#if WFL_Win32
	/*!
	\brief 窗口类。
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
		//! \throw Win32Exception 窗口类注册失败。
		//@{
		/*!
		\pre 间接断言：第一参数非空。
		\note 应用程序实例句柄参数为空则使用 <tt>::GetModuleHandleW()</tt> 。
		\note 窗口过程参数为空时视为 HostWindow::WindowProcedure 。
		\note 默认画刷参数等于 <tt>::HBRUSH(COLOR_MENU + 1)</tt> 。
		\sa HostWindow::WindowProcedure
		*/
		WB_NONNULL(1)
			WindowClass(const wchar_t*, ::WNDPROC = {}, unsigned = 0,
				::HBRUSH = ::HBRUSH(4 + 1), ::HINSTANCE = {});
		WindowClass(const ::WNDCLASSW&);
		WindowClass(const ::WNDCLASSEXW&);
		//@}
		/*!
		\pre 间接断言：第一参数的数据指针非空。
		\pre 原子表示已注册的窗口类。
		\pre 实例句柄和注册时使用的值相等。
		\throw std::invalid_argument 原子值等于 \c 0 。
		\note 使用指定名称和原子并取得窗口类的所有权。名称不影响原子。
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
	\brief 宿主窗口。
	\note Android 平台：保持引用计数。
	\invariant <tt>bool(GetNativeHandle())</tt> 。
	*/
	class WF_API HostWindow : private WindowReference, private white::noncopyable
	{
	public:
#	if WFL_HostedUI_XCB
		const XCB::Atom::NativeType WM_PROTOCOLS, WM_DELETE_WINDOW;
#	endif
#	if WFL_HostedUI_XCB || WFL_Win32
		/*!
		\brief 窗口消息转发事件映射。
		*/
		platform_ex::MessageMap MessageMap;
#	endif

		/*!
		\brief 使用指定宿主句柄初始化宿主窗口。
		\pre 间接断言：句柄非空。
		\pre 使用 XCB 的平台：间接断言：句柄非空。
		\pre 使用 XCB 的平台：句柄通过 <tt>new XCB::WindowData</tt> 得到。
		\pre Win32 平台：断言：句柄有效。
		\pre Win32 平台：断言：句柄表示的窗口在本线程上创建。
		\pre Win32 平台：断言： \c GWLP_USERDATA 数据等于 \c 0 。
		\throw GeneralEvent 使用 XCB 的平台：窗口从属的 XCB 连接发生错误。
		\throw GeneralEvent Win32 平台：窗口类名不是 WindowClassName 。

		检查句柄，初始化宿主窗口并取得所有权。对 Win32 平台初始化 HID 输入消息并注册
		\c WM_DESTROY 消息响应为调用 <tt>::PostQuitMessage(0)</tt> 。
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
		\brief 取相对窗口的可响应输入的点的位置。
		\note 默认输入边界为客户区，输入总是视为有效；实现为直接返回参数。
		\return 若参数表示的位置无效则 white::Drawing::Point::Invalie ，
		否则为相对窗口输入边界的当前点的坐标。
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
		\brief 窗口过程。
		\pre 窗口句柄非空；对应的 GWLP_USERDATA 域为保证回调时可访问的 HostWindow 指针，
		或空值。
		访问窗口句柄对应的 GWLP_USERDATA 域，当存储的值非空时作为 HostWindow 的指针值，
		访问其中消息映射分发和执行消息；否则，调用默认窗口处理过程。
		*/
		static ::LRESULT __stdcall
			WindowProcedure(::HWND, unsigned, ::WPARAM, ::LPARAM) wnothrowv;
#		endif
#	endif
	};


	/*!
	\brief 窗口输入宿主。
	\warning 非虚析构。
	*/
	class WF_API WindowInputHost
	{
	public:
		HostWindow& Window;

#	if WFL_Win32
	public:
		//! \brief 鼠标键输入。
		std::atomic<short> RawMouseButton{ 0 };

	private:
		/*!
		\brief 标识宿主插入符。
		\see https://src.chromium.org/viewvc/chrome/trunk/src/ui/base/ime/win/imm32_manager.cc
		IMM32Manager::CreateImeWindow 的注释。
		*/
		bool has_hosted_caret;
		//! \brief 输入组合字符串锁。
		white::recursive_mutex input_mutex{};
		//! \brief 输入法组合字符串。
		white::String comp_str{};
		//! \brief 相对窗口的宿主插入符位置缓存。
		white::Drawing::Point caret_location{ white::Drawing::Point::Invalid };
#	endif
	public:
		//! \brief 构造：使用指定窗口引用，按需初始化光标和输入消息映射。
		WindowInputHost(HostWindow&);
		~WindowInputHost();

#if WFL_Win32
		/*!
		\brief 访问输入法状态。
		\note 线程安全：互斥访问。
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
		\brief 更新输入法编辑器候选窗口位置。
		\note 位置为相对窗口客户区的坐标。
		\note 若位置为 Drawing::Point::Invalid 则忽略。
		\sa caret_location
		*/
		//@{
		//! \note 线程安全。
		//@{
		//! \note 取缓存的位置。
		void
			UpdateCandidateWindowLocation();
		//! \note 首先无条件更新缓存。
		void
			UpdateCandidateWindowLocation(const white::Drawing::Point&);
		//@}

		//! \note 无锁版本，仅供内部实现。
		void
			UpdateCandidateWindowLocationUnlocked();
		//@}
#endif
	};


#	if WFL_Win32
	/*!
	\brief 打开的剪贴板实例。
	\todo 非 Win32 宿主平台实现。
	*/
	class WF_API Clipboard : private white::noncopyable, private white::nonmovable
	{
	private:
		class Data;

	public:
		//! \brief 格式标识类型。
		using FormatType = unsigned;

		Clipboard(NativeWindowHandle);
		~Clipboard();

		/*!
		\brief 判断指定格式是否可用。
		\note Win32 平台：不可用时可能会改变 <tt>::GetLastError()</tt> 的结果。
		*/
		static bool
			IsAvailable(FormatType) wnothrow;

		/*!
		\brief 检查指定格式是否可用，若成功则无作用。
		\throw Win32Exception 值不可用。
		\sa IsAvailable
		*/
		static void
			CheckAvailable(FormatType);

		//! \brief 清除剪贴板内容。
		void
			Clear() wnothrow;

		//! \brief 返回当前打开剪贴板并关联的窗口句柄。
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
		\pre 断言：字符串参数的数据指针非空。
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
	\brief 执行 Shell 命令。
	\pre 间接断言：第一参数非空。
	\throw LoggedEvent 一般调用失败。
	\throw std::bad_alloc 调用失败：存储分配失败。
	\warning 当不能保证不使用 COM 时，要求 COM 被适当初始化。
	\warning 不附加检查：命令执行路径指定为文档时，命令参数应为空。
	\warning 不附加检查：命令执行路径为相对路径时，工作目录路径不应为相对路径。
	\sa https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx

	使用 ::ShellExecuteW 执行 Shell 命令。
	参数分别表示命令执行路径、命令参数、是否使用管理员权限执行、工作目录路径、
	使用的初始窗口选项（默认为 \c SW_SHOWNORMAL ），以及关联的父窗口句柄。
	::ShellExecuteW 的返回值若不大于 32 则为错误。和 Win32 错误码直接对应相等的返回值
	被抛出为 Win32 异常。
	返回 0 和 SE_ERR_OOM 时抛出 std::bad_alloc ；
	未被文档明确的错误视为未知错误，抛出 LoggedEvent ；
	其它返回值被重新映射后抛出 Win32 异常：
	返回 SE_ERR_SHARE 映射为 Win32 错误码 ERROR_SHARING_VIOLATION ；
	返回 SE_ERR_DLLNOTFOUND 映射为 Win32 错误码 ERROR_DLL_NOT_FOUND ；
	返回 SE_ERR_ASSOCINCOMPLETE 和 SE_ERR_NOASSOC 映射为 Win32 错误码
	ERROR_NO_ASSOCIATION ；
	返回 SE_ERR_DDETIMEOUT 、 SE_ERR_DDEFAIL 和 SE_ERR_DDEBUSY 映射为 Win32 错误码
	ERROR_DDE_FAIL 。
	对多个返回值映射的错误码，抛出的异常是嵌套异常，
	其底层异常为 ystdex::wrap_mixin_t<std::runtime_error, int> 类型。
	*/
	WF_API WB_NONNULL(1) void
		ExecuteShellCommand(const wchar_t*, const wchar_t* = {}, bool = {},
			const wchar_t* = {}, int = 1, NativeWindowHandle = {});
#	endif

} // namespace platform_ex;


#endif

#endif