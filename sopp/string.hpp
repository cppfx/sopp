//
// Copyright (c) 2026 cppfx.xyz
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <string>
#include <iomanip>
#include <sstream>

namespace sopp
{
	class string
	{
	private:
		const std::string __blanks = " \n\t\r\v";
		std::string __str;
	public:
		virtual ~string() = default;
	public:
		string(const std::string & str__):
			__str{str__}
		{
		}
	public:
		std::string quoted() const
		{
			return (std::ostringstream{} << std::quoted(__str)).str();
		}
	public:
		std::string strip() const
		{
			if (__str.size() < 1u)
				return "";

			if (__str.size() == 1u)
			{
				if (__blanks.contains(__str[0]))
					return "";
				else
					return __str;
			}

			// first position of non-blank.
			std::uint32_t pos;
			for (pos=0; pos<__str.size(); ++pos)
			{
				if (! __blanks.contains(__str[pos]))
					break;
			}
			if (pos >= __str.size())
				return "";

			// last position of non-blank
			std::uint32_t epos;
			for (epos=__str.size()-1; epos>=0; --epos)
			{
				if (! __blanks.contains(__str[epos]))
					break;
			}
			if (epos < pos)
				return "";

			return __str.substr(pos, epos-pos+1u);
		}
	public:
		bool contains_blank_if_strip() const
		{
			const std::string tmp = this->strip();
			for (char x: tmp)
			{
				if (__blanks.contains(x))
					return true;
			}
			return false;
		}
	public:
		bool is_number() const
		{
			try
			{
				return std::to_string(std::stoll(__str)) == __str;
			}
			catch (...)
			{
				return false;
			}
		}
	public:
		bool is_non_negative_number() const
		{
			try
			{
				return std::to_string(static_cast<std::uint64_t>(std::stoll(__str))) == __str;
			}
			catch (...)
			{
				return false;
			}
		}
		bool is_negative_number() const
		{
			try
			{
				auto num = std::stoll(__str);
				if (num >= 0)
					return false;
				// else
				return std::to_string(num) == __str;
			}
			catch (...)
			{
				return false;
			}
		}
	};	// class string
}	// namespace sopp

