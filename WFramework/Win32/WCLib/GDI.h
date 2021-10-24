/*!	\file GDI.h
\ingroup Win32/WCLib
\brief Win32 GDI �ӿڡ�
*/

#ifndef WFrameWork_Win32_GDI_h
#define WFrameWork_Win32_GDI_h 1

#include <WFramework/WCLib/HostGUI.h>

namespace white {
	namespace Drawing {
		using Pixel = uint32;
		using BitmapPtr = Pixel*;
		using ConstBitmapPtr = const Pixel*;

#define PixelGetA(pixel) static_cast<uint8>((pixel & 0xFF000000) >> 24)
#define PixelGetR(pixel) static_cast<uint8>((pixel & 0x00FF0000) >> 16)
#define PixelGetG(pixel) static_cast<uint8>((pixel & 0x0000FF00) >> 8)
#define PixelGetB(pixel) static_cast<uint8>((pixel & 0x000000FF) >> 0)
#define PixelCtor(A,R,G,B) static_cast<Pixel>(A << 24 | R << 16 | G << 8 | B)

		template<bool _bSwapLR, bool _bSwapUD, typename _tOut, typename _tIn,
			typename _fBlitScanner>
			inline void
			BlitLines(_fBlitScanner scanner, _tOut dst, _tIn src, const Size& ds,
				const Size& ss, const Point& dp, const Point& sp, const Size& sc)
		{
			throw white::unimplemented();
		}


		/*!
		\ingroup BlitScanner
		\brief ɨ���ߣ���ָ��ɨ��˳����һ�����ء�
		\warning ������������Ч�ԡ�
		*/
		//@{
		template<bool _bDec>
		struct CopyLine
		{
			/*!
			\brief ���Ƶ�����ָ����һ�����ء�
			\tparam _tOut ������������ͣ���Ҫ֧�� + ������һ��Ӧ���������������
			\tparam _tIn ������������͡�
			\pre ���ԣ��Է��������ʼ�����������ж�Ϊ���ɽ����á�
			*/
			//@{
			template<typename _tOut, typename _tIn>
			void
				operator()(_tOut& dst_iter, _tIn& src_iter, SDst delta_x) const
			{
				using white::is_undereferenceable;

				WAssert(delta_x == 0 || !is_undereferenceable(dst_iter),
					"Invalid output iterator found."),
					YAssert(delta_x == 0 || !is_undereferenceable(src_iter),
						"Invalid input iterator found.");
				std::copy_n(src_iter, delta_x, dst_iter);
				// NOTE: Possible undefined behavior. See $2015-02
				//	@ %Documentation::Workflow::Annual2015.
				wunseq(src_iter += delta_x, dst_iter += delta_x);
			}
			template<typename _tOut, typename _tPixel>
			void
				operator()(_tOut& dst_iter, white::pseudo_iterator<_tPixel> src_iter,
					SDst delta_x) const
			{
				using white::is_undereferenceable;

				WAssert(delta_x == 0 || !is_undereferenceable(dst_iter),
					"Invalid output iterator found."),
					std::fill_n(dst_iter, delta_x, Deref(src_iter));
				// NOTE: Possible undefined behavior. See $2015-02
				//	@ %Documentation::Workflow::Annual2015.
				dst_iter += delta_x;
			}
			//@}
		};
	}
}

#if WF_Hosted
namespace platform_ex
{
	namespace Windows {
#	if WFL_Win32
		//@{
		//! \brief ʹ�� ::DeleteObject ɾ�������ɾ������
		struct WF_API GDIObjectDelete
		{
			using pointer = void*;

			void
				operator()(pointer) const wnothrow;
		};
		//@}


		/*!
		\brief ����ָ����С�ļ���λͼ��
		\post �ڶ������ǿա�
		\exception �쳣�������� white::CheckArithmetic �׳���
		\throw Win32Exception ::CreateDIBSection ����ʧ�ܡ�
		\throw std::runtime_error �����������ʧ�ܣ� ::CreateDIBSection ʵ�ִ��󣩡�
		\return �ǿվ����

		���� ::CreateDIBSection ���� 32 λλͼ��ʹ�ü��ݻ�����ָ�����ڶ�������
		*/
		WF_API::HBITMAP
			CreateCompatibleDIBSection(const white::Drawing::Size&,
				white::Drawing::BitmapPtr&);
#	endif


		/*!
		\note ���ظ�ʽ�� platform::Pixel ���ݡ�
		\warning ����������
		*/
		//@{
		/*!
		\brief ������Ļ���档
		\note Android ƽ̨�����ؿ�����ʵ�ֵĻ������Ŀ�
		*/
		class WF_API ScreenBuffer
		{
		private:
#	if WFL_HostedUI_XCB || WFL_Android
			/*!
			\invariant bool(p_impl) ��
			*/
			unique_ptr<ScreenBufferData> p_impl;
			/*!
			\brief ���������������Ļ�������ʵ�ʿ�ȡ�
			*/
			white::SDst width;
#	elif WFL_Win32
			white::Drawing::Size size;
			white::Drawing::BitmapPtr p_buffer;
			/*!
			\invariant \c bool(p_bitmap) ��
			*/
			unique_ptr_from<GDIObjectDelete> p_bitmap;
#	endif

		public:
			//! \brief ���죺ʹ��ָ���Ļ�������С�͵��ڻ�����������ؿ�ࡣ
			ScreenBuffer(const white::Drawing::Size&);
#	if WFL_HostedUI_XCB || WFL_Android
			/*!
			\brief ���죺ʹ��ָ���Ļ�������С�����ؿ�ࡣ
			\throw Exception ���ؿ��С�ڻ�������С��
			\since build 498
			*/
			ScreenBuffer(const white::Drawing::Size&, white::SDst);
#	endif
			ScreenBuffer(ScreenBuffer&&) wnothrow;
			~ScreenBuffer();

			/*!
			\brief ת�Ƹ�ֵ��ʹ�ý�����
			*/
			PDefHOp(ScreenBuffer&, =, ScreenBuffer&& sbuf) wnothrow
				ImplRet(swap(sbuf, *this), *this)

#	if WFL_HostedUI_XCB || WFL_Android
				//! \since build 492
				white::Drawing::BitmapPtr
				GetBufferPtr() const wnothrow;
			//! \since build 566
			white::Drawing::Graphics
				GetContext() const wnothrow;
			//! \since build 498
			white::Drawing::Size
				GetSize() const wnothrow;
			//! \since build 498
			white::SDst
				GetStride() const wnothrow;
#	elif WFL_Win32
				//@{
				DefGetter(const wnothrow, white::Drawing::BitmapPtr, BufferPtr, p_buffer)
				DefGetter(const wnothrow, ::HBITMAP, NativeHandle,
					::HBITMAP(p_bitmap.get()))
				DefGetter(const wnothrow, const white::Drawing::Size&, Size, size)

				/*!
				\brief �ӻ��������²��� Alpha Ԥ�ˡ�
				\pre ���ԣ������ǿա�
				\post ::HBITMAP �� rgbReserved Ϊ 0 ��
				\warning ֱ�Ӹ��ƣ�û�б߽�ʹ�С��顣ʵ�ʴ洢����� 32 λ ::HBITMAP ���ݡ�
				*/
				WB_NONNULL(1) void
				Premultiply(white::Drawing::ConstBitmapPtr) wnothrow;
#	endif

			/*!
			\brief �������ô�С��
			\note ����Сһ��ʱ�޲������������·��䣬�ɵ��±�������ͻ�����ָ��ĸı䡣
			*/
			void
				Resize(const white::Drawing::Size&);

			/*!
			\brief �ӻ��������¡�
			\pre ��Ӷ��ԣ������ǿա�
			\pre Android ƽ̨����������С�����ؿ����ȫһ�¡�
			\post Win32 ƽ̨�� \c ::HBITMAP �� \c rgbReserved Ϊ 0 ��
			\warning ֱ�Ӹ��ƣ�û�б߽�ʹ�С��顣
			\warning Win32 ƽ̨��ʵ�ʴ洢����� 32 λ ::HBITMAP ���ݡ�
			\warning Android ƽ̨��ʵ�ʴ洢����� 32 λ RGBA8888 ���ݡ�
			*/
			WB_NONNULL(1) void
				UpdateFrom(white::Drawing::ConstBitmapPtr);
			//@}

#	if WFL_Win32
			/*!
			\brief �ӻ���������ָ���߽������
			\pre ��Ӷ��ԣ������ǿա�
			\post \c ::HBITMAP �� \c rgbReserved Ϊ 0 ��
			\warning ֱ�Ӹ��ƣ�û�б߽�ʹ�С��顣
			\warning ʵ�ʴ洢����� 32 λ ::HBITMAP ���ݡ�
			*/
			WB_NONNULL(1) void
				UpdateFromBounds(white::Drawing::ConstBitmapPtr,
					const white::Drawing::Rect&) wnothrow;

			/*!
			\pre ��Ӷ��ԣ���������ǿա�
			*/
			void
				UpdatePremultipliedTo(NativeWindowHandle, white::Drawing::AlphaType = 0xFF,
					const white::Drawing::Point& = {});
#	endif

			/*!
			\pre ��Ӷ��ԣ���������ǿա�
			\since build 589
			*/
			void
				UpdateTo(NativeWindowHandle, const white::Drawing::Point& = {}) wnothrow;

#	if WFL_Win32
			/*!
			\pre ��Ӷ��ԣ���������ǿա�
			*/
			void
				UpdateToBounds(NativeWindowHandle, const white::Drawing::Rect&,
					const white::Drawing::Point& = {}) wnothrow;
#	endif

			/*!
			\brief ������
			*/
			WF_API friend void
				swap(ScreenBuffer&, ScreenBuffer&) wnothrow;
		};


		/*!
		\brief ������Ļ���򻺴档
		*/
		class WF_API ScreenRegionBuffer : private ScreenBuffer
		{
		private:
			white::mutex mtx{};

		public:
			using ScreenBuffer::ScreenBuffer;

			DefGetter(wnothrow, ScreenBuffer&, ScreenBufferRef, *this)

				//! \since build 589
				PDefH(white::locked_ptr<ScreenBuffer>, Lock, )
				ImplRet({ &GetScreenBufferRef(), mtx })
		};
		//@}

		//@{
		/*!
		\brief �����ڴ���棺���洰���ϵĶ�άͼ�λ���״̬��
		\note �����ڴ�������������Ȩ��
		*/
		class WF_API WindowMemorySurface
		{
		private:
			::HDC h_owner_dc, h_mem_dc;

		public:
			WindowMemorySurface(::HDC);
			~WindowMemorySurface();

			DefGetter(const wnothrow, ::HDC, OwnerHandle, h_owner_dc)
				DefGetter(const wnothrow, ::HDC, NativeHandle, h_mem_dc)

				void
				UpdateBounds(ScreenBuffer&, const white::Drawing::Rect&,
					const white::Drawing::Point& = {}) wnothrow;

			void
				UpdatePremultiplied(ScreenBuffer&, NativeWindowHandle,
					white::Drawing::AlphaType = 0xFF, const white::Drawing::Point& = {})
				wnothrow;
		};


		class WF_API WindowDeviceContextBase
		{
		protected:
			NativeWindowHandle hWindow;
			::HDC hDC;

			WindowDeviceContextBase(NativeWindowHandle h_wnd, ::HDC h_dc) wnothrow
				: hWindow(h_wnd), hDC(h_dc)
			{}
			DefDeDtor(WindowDeviceContextBase)

		public:
			DefGetter(const wnothrow, ::HDC, DeviceContextHandle, hDC)
				DefGetter(const wnothrow, NativeWindowHandle, WindowHandle, hWindow)
		};


		/*!
		\brief �����豸�����ġ�
		\note �����豸������������Ȩ��
		*/
		class WF_API WindowDeviceContext : public WindowDeviceContextBase
		{
		protected:
			/*!
			\pre ��Ӷ��ԣ������ǿա�
			\throw white::LoggedEvent ��ʼ��ʧ�ܡ�
			*/
			WindowDeviceContext(NativeWindowHandle);
			~WindowDeviceContext();
		};


		/*!
		\brief ��ҳ��λͼ���ݡ�
		\note ��������ʵ�ֵķǹ����ӿڡ�
		*/
		class PaintStructData
		{
		public:
			struct Data;

		private:
			//! \note ���ֺ� \c ::PAINTSTRUCT �����Ƽ��ݡ�
#if WFL_Win64
			white::aligned_storage_t<72, 8> ps;
#else
			white::aligned_storage_t<64, 4> ps;
#endif
			//! \invariant <tt>&pun.get() == &ps</tt>
			white::pun_ref<Data> pun;

		public:
			PaintStructData();
			~PaintStructData();

			DefGetter(const wnothrow, Data&, , pun.get())
		};


		/*!
		\brief ���������豸�����ġ�
		\note �����豸������������Ȩ��
		*/
		class WF_API WindowRegionDeviceContext : protected PaintStructData,
			public WindowDeviceContextBase, private white::noncopyable
		{
		protected:
			/*!
			\pre ��Ӷ��ԣ������ǿա�
			\throw white::LoggedEvent ��ʼ��ʧ�ܡ�
			*/
			WindowRegionDeviceContext(NativeWindowHandle);
			~WindowRegionDeviceContext();

		public:
			/*!
			\brief �жϱ����Ƿ���Ȼ��Ч�������������»��ơ�
			*/
			bool
				IsBackgroundValid() const wnothrow;

			/*!
			\exception �쳣�������� white::CheckArithmetic �׳���
			*/
			white::Drawing::Rect
				GetInvalidatedArea() const;
		};
		//@}


		/*!
		\brief ��ʾ������棺������ʾ�����ϵĶ�άͼ�λ���״̬��
		\warning ����������
		*/
		template<typename _type = WindowDeviceContext>
		class GSurface : public _type, public WindowMemorySurface
		{
		public:
			/*!
			\pre ��Ӷ��ԣ������ǿա�
			\exception white::LoggedEvent �����Ӷ����ʼ��ʧ�ܡ�
			*/
			GSurface(NativeWindowHandle h_wnd)
				: _type(white::Nonnull(h_wnd)),
				WindowMemorySurface(_type::GetDeviceContextHandle())
			{}
		};


		
	}
}
#endif


#endif