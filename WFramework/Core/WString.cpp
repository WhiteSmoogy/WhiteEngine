#include "WString.h"

namespace white
{
	namespace Text
	{
		String&
			String::operator*=(size_t n)
		{
			switch (n)
			{
			case 0:
				clear();
			case 1:
				break;
			default:
				reserve(length() * n);
				white::concat(*this, n);
			}
			return *this;
		}
	} // namespace Text;
} // namespace white;