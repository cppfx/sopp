//
// Copyright (c) 2025 cppfx.xyz
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <boost/assert.hpp>
#include <fstream>

using std::string_literals::operator""s;

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

namespace sopp
{
	class output final:
		virtual public std::enable_shared_from_this<sopp::output>
	{
	private:
		std::ostringstream
			__out,
			__log,
			__err
		;
	public:
		output() = default;
		virtual ~output() = default;
	public:
		std::ostringstream & out()
		{
			return __out;
		}
		std::ostringstream & log()
		{
			return __log;
		}
		std::ostringstream & err()
		{
			return __err;
		}
	};

	class fatal_error final:
		virtual public std::runtime_error
	{
	public:
		fatal_error(const std::string & msg):
			std::runtime_error{msg}
		{
		}
		virtual ~fatal_error() = default;
	};

	class args_error_show_help:
		virtual public std::exception
	{
	public:
		args_error_show_help() = default;
		virtual ~args_error_show_help() = default;
	};

	class args_error_show_msg:
		virtual public std::runtime_error
	{
	public:
		args_error_show_msg(
			const std::string & msg
		):
			std::runtime_error{msg}
		{
		}
		virtual ~args_error_show_msg() = default;
	};

	class args final:
		virtual public std::enable_shared_from_this<sopp::args>
	{
	private:
		enum class args_next
		{
			key_only,
			key_value,
			value_only,
			is_end
		};
	private:
		std::vector<std::string> __args;
		std::vector<std::string> __transformed_args;
	private:
		const std::vector<std::string> __key_only_key{"-d", "-dd", "-ddd", "-v"};
		const std::vector<std::string> __key_value_key{"-a", "-o", "-l", "-e", "-r"};
	private:
		bool __d_mode{false};
		bool __dd_mode{false};
		bool __ddd_mode{false};
		bool __verbose{false};
		std::string __allfile = "";
		std::string __outfile = "";
		std::string __logfile = "";
		std::string __errfile = "";

		std::string __host;
		std::string __port;
		std::string __target;
		std::string __json_string;
	private:
		const std::string __vp = "vp";	// value prefix: used for preventing empty duplicated value set.
	public:
		args(int argc, char ** argv)
		{
			for (int i=0; i<argc; ++i)
				__args.emplace_back(argv[i]);
		}
		virtual ~args() = default;
	public:
		bool d_mode() const { return __d_mode;}
		bool dd_mode() const { return __dd_mode;}
		bool ddd_mode() const { return __ddd_mode;}
		bool verbose() const {return __verbose;}
	
		std::string allfile() const {return __allfile;}
		std::string outfile() const {return __outfile;}
		std::string logfile() const {return __logfile;}
		std::string errfile() const {return __errfile;}

		std::string host() const {return __host;}
		std::string port() const {return __port;}
		std::string target() const {return __target;}
		std::string json_string() const {return __json_string;}
	public:
		void print_help() const
		{
			std::clog
				<< "|========\n"
				<< "| Sopp c++ json-rpc http client application.\n"
				<< "| Boost Software License\n"
				<< "|========\n"
				<< "\n"
				<< "sopp [-d|-dd|-ddd] [-v] [-a file] [-o file] [-l file] [-e file] <-r host> <json string>\n"
				<< "sopp [-h|-help|--h|--help|help]\n"
				<< "\n"
				<< "\n"
				<< "-d        ---- Developer Mode: do not request server.\n"
				<< "-dd       ---- Developer Mode: request server.\n"
				<< "-ddd      ---- Developer Mode: request server. More verbose.\n"
				<< "\n"
				<< "-v        ---- Verbose print: Write result, log, error to console.\n"
				<< "\n"
				<< "-a file        ---- Write result, log, error all to one file.\n"
				<< "-o file        ---- Write result to file.\n"
				<< "-l file        ---- Write log to file.\n"
				<< "-e file        ---- Write error to file.\n"
				<< "-r host        ---- Request host.\n"
				<< "\n"
				<< "-h, -help, --h, --help, help        ---- show this help.\n"
				<< "\n"
				<< ">>>> No -v -a -o -l -e: output result to console; log and error are not outputed.\n"
				<< "\n"
				<< ">>>> host format: ip_address:port/location\n"
				<< ">>>>	For example: 127.0.0.1:8080/api/v1.0\n"
				<< ">>>> <-r host> and <json string> are required, other args are optional.\n"
				<< "\n"
				<< ">>>> -d, -dd, -ddd are mutex with each other.\n"
				<< ">>>>\n"
				<< ">>>> -v is mutex with any one of -a -o -l -e .\n"
				<< ">>>> -a is mutex with any one of -v -o -l -e .\n"
				<< ">>>>\n"
				<< ">>>> Fatal error occured (sopp::fatal_error exception):\n"
				<< ">>>>	print error to console unconditionally and quit program.\n"
				<< std::endl;
			;
		}
	public:
		void parse()
		{
			this->parse_args();
			if (__ddd_mode)
			{
				this->report("before removing vp from filenames:");
			}
			this->remove_vp();
			if (__ddd_mode)
			{
				this->report("after removing vp from filenames:");
			}
			this->parse_host();
			if (__ddd_mode)
			{
				this->report("after parsing host:");
			}
			this->parse_transformed_args();
			this->mutex_test();
			this->require_host();
			this->report("Final parsed args:");
		}
	private:
		void parse_args()
		{
			if (__args.size() == 0)
				throw sopp::args_error_show_msg{"Program error. Please submit to developers."};
			if (__args.size() == 1)
				throw sopp::args_error_show_help();
			if (__args.size() == 2)
			{
				if (
					__args[1] == "-h"
						||
					__args[1] == "--h"
						||
					__args[1] == "-help"
						||
					__args[1] == "--help"
						||
					__args[1] == "help"
				)
				{
					throw sopp::args_error_show_help();
				}
			}

			// else
			if (__args.size() < 3)
			{
				throw sopp::args_error_show_msg{
					"Args Error: requires at least <host> and <json string>.\n\n"s
						+
					"Print help:\n\n"
						+
					"sopp help\n"
				};
			}

			// else

			while (true)
			{
				auto [type, key, value] = this->get_next_tkv();
				if (type == sopp::args::args_next::is_end)
					break;
				switch (type)
				{
				case sopp::args::args_next::key_only:
					if (key == "-d")
					{
						if (__d_mode)
							throw sopp::args_error_show_msg{"Duplicated: -d"};
						else
							__d_mode = true;
					}
					else if (key == "-dd")
					{
						if (__dd_mode)
							throw sopp::args_error_show_msg{"Duplicated: -dd"};
						else
							__dd_mode = true;
					}
					else if (key == "-ddd")
					{
						if (__ddd_mode)
							throw sopp::args_error_show_msg{"Duplicated: -ddd"};
						else
							__ddd_mode = true;
					}
					else if (key == "-v")
					{
						if (__verbose)
							throw sopp::args_error_show_msg{"Duplicated: -v"};
						else
							__verbose = true;
					}
					else
					{
						throw sopp::args_error_show_msg{
							"Code Error, unimplemented key: "s + key
								+ " .\nPlease submit to developers."
						};
					}
					break;
				case sopp::args::args_next::key_value:
					if (key == "-a")
					{
						if (! __allfile.empty())
							throw sopp::args_error_show_msg{
								"Duplicted:\n"s
									+
								"\t" + key + " " + value
							};
						else
							__allfile = __vp + value;
					}
					else if (key == "-o")
					{
						if (! __outfile.empty())
							throw sopp::args_error_show_msg{
								"Duplicted:\n"s
									+
								"\t" + key + " " + value
							};
						else
							__outfile = __vp + value;
					}
					else if (key == "-l")
					{
						if (! __logfile.empty())
							throw sopp::args_error_show_msg{
								"Duplicted:\n"s
									+
								"\t" + key + " " + value
							};
						else
							__logfile = __vp + value;
					}
					else if (key == "-e")
					{
						if (! __errfile.empty())
							throw sopp::args_error_show_msg{
								"Duplicted:\n"s
									+
								"\t" + key + " " + value
							};
						else
							__errfile = __vp + value;
					}
					else if (key == "-r")
					{
						if (! __host.empty())
							throw sopp::args_error_show_msg{
								"Duplicted:\n"s
									+
								"\t" + key + " " + value
							};
						else
							__host = __vp + value;
					}
					else
					{
						throw sopp::args_error_show_msg{
							"Code Error, unimplemented key: "s + key
								+ " .\nPlease submit to developers."
						};
					}
					break;
				case sopp::args::args_next::value_only:
					__transformed_args.push_back(value);
					break;
				default:
					break;
				}
			}
		}
	private:
		std::tuple<sopp::args::args_next, std::string, std::string> get_next_tkv()
		{
			static auto itr = __args.begin() + 1;
			if (itr == __args.end())
			{
				return std::tuple{
					sopp::args::args_next::is_end,
					""s,
					""s
				};
			}
			// else
			const std::string key = *itr;
			bool is_key_only_key = false;
			bool is_key_value_key = false;
			for (const std::string & k: __key_only_key)
			{
				if (*itr == k)
					is_key_only_key = true;
			}
			for (const std::string & k: __key_value_key)
			{
				if (*itr == k)
					is_key_value_key = true;
			}
			if (is_key_only_key)
			{
				BOOST_ASSERT(! is_key_value_key);	// Just code test.
				return std::tuple{
					sopp::args::args_next::key_only,
					*itr++,
					""s
				};
			}
			// else
			if (is_key_value_key)
			{
				BOOST_ASSERT(! is_key_only_key);	// Just code test.
				BOOST_ASSERT(itr != __args.end());	// Just code test.
				const std::string key = *itr++;
				if (itr == __args.end())
					throw sopp::args_error_show_msg(
						"Read an arg option key that requires a value: "s + key
					);
				return std::tuple{
					sopp::args::args_next::key_value,
					key,
					*itr++
				};
			}
			// else
			BOOST_ASSERT(! is_key_value_key);	// Just code test.
			BOOST_ASSERT(! is_key_only_key);	// Just code test.
			if (itr != __args.end())
			{
				return std::tuple{
					sopp::args::args_next::value_only,
					""s,
					*itr++
				};
			}

			// else

			// All conditions are met, the following code is just for to discard warning.

			return std::tuple{
				sopp::args::args_next::is_end,
				""s,
				""s
			};
		}
	private:
		void parse_host()
		{
			if (__host == "")
				throw sopp::args_error_show_msg{
					"<-r host> is not specified or its value is empty!"
				};

			auto pos = __host.find_first_of("/");
			std::string str2;
			if (pos == std::string::npos)
			{
				__target = "";
				str2 = __host;
			}
			else
			{
				__target = __host.substr(pos);
				str2 = __host.substr(0, pos);
			}

			// then

			auto pos2 = str2.find_last_of(":");
			if (pos2 == std::string::npos)
			{
				__port = "";
				__host = str2;
				return;
			}
			else
			{
				__host = str2.substr(0, pos2);
				__port = str2.substr(pos2+1);
				{// test port number
					auto pn = static_cast<std::int32_t>(std::stoi(__port));
					if (pn < 1 || pn > 65535 || __port != std::to_string(pn))
					{
						throw sopp::args_error_show_msg{
							"Invalid port number, port number must be a number, and:\n"s
								+
							"\t0 is not allowed in this program.\n"
								+
							"\t1 <= port number <= 65535\n"
								+
							"\tNon-number characters are not allowed to be added to the number."
								+
							"\n\n"
						};
					}
				}
				if (__host.starts_with("[") && __host.ends_with("]"))	// process IPv6
				{
					__host = __host.substr(1);
					__host = __host.substr(0, __host.size()-1);
				}
				return;
			}
		}
	private:
		void parse_transformed_args()
		{
			if (__transformed_args.size() == 0)
				throw sopp::args_error_show_msg{
					"<json string> is not specified, it is required."
				};
			__json_string = __transformed_args[0];
			if (__transformed_args.size() > 1u)
			{
				std::string error = "Additional args are not allowed. If you want some new features, please submit to developers.\n";
				if (__d_mode || __dd_mode || __ddd_mode)
				{
					error += "Unwanted args:\n";
					for (std::uint32_t i=1; i<__transformed_args.size(); ++i)
					{
						error += "\t"s +
							(std::ostringstream{} << (std::quoted(__transformed_args[i]))).str()
							 + "\n";
					}
				}
				throw sopp::args_error_show_msg{
					error
				};
			}
		}
	private:
		void mutex_test()
		{
			{
				int counter = 0;
				if (__d_mode)
					++counter;
				if (__dd_mode)
					++counter;
				if (__ddd_mode)
					++counter;
				if (counter > 1)
					throw sopp::args_error_show_msg{
						"-d, -dd, -ddd are mutex with each other!"
					};
			}
			{
				if (__verbose)
				{
					if (
						! __allfile.empty()
							||
						! __outfile.empty()
							||
						! __logfile.empty()
							||
						! __errfile.empty()
					)
						throw sopp::args_error_show_msg{
							"-v is mutex with -a, -o, -l, -e"
						};
				}
			}
			{
				if (! __allfile.empty())
				{
					if (
						__verbose
							||
						! __outfile.empty()
							||
						! __logfile.empty()
							||
						! __errfile.empty()
					)
						throw sopp::args_error_show_msg{
							"-a is mutex with -v, -o, -l, -e"
						};
				}
			}
		}
	private:
		void require_host()
		{
			std::string error_msg =
				"Require at least any one of __host and __port is not empty.\n"s
					+
				"If host is empty, host will be default 127.0.0.1\n"
					+
				"If port is empty, port will be default http port 80\n\n";

			if (__ddd_mode)
				std::clog << error_msg << std::flush;
			if (__host.empty() && __port.empty())
				throw sopp::args_error_show_msg{error_msg};
			if (__host.empty())
				__host = "127.0.0.1";
			else if (__port.empty())
				__port = "80";
			if (__target.empty())
				__target = "/";
		}
	private:
		void remove_vp()
		{
			for (std::string & value:
				{
					std::ref(__allfile),
					std::ref(__outfile),
					std::ref(__logfile),
					std::ref(__errfile),
					std::ref(__host)
				}
			)
			{
				if (value.starts_with(__vp))
					value = value.substr(__vp.size());
			}
		}
	private:
		void report(const std::string & label) const
		{
			if (__d_mode || __dd_mode || __ddd_mode)
			{
				std::clog << "================================================================\n";
				std::clog << "================================================================\n";
				std::clog << "========\n";
				std::clog << "======== " << label << std::endl << std::endl;
			}
			if (__d_mode || __dd_mode || __ddd_mode)
			{
				std::clog << "__d_mode: " << __d_mode << std::endl;
				std::clog << "__dd_mode: " << __dd_mode << std::endl;
				std::clog << "__ddd_mode: " << __ddd_mode << std::endl;
				std::clog << "__verbose: " << __verbose << std::endl;
				std::clog << "__allfile: " << std::quoted(__allfile) << std::endl;
				std::clog << "__outfile: " << std::quoted(__outfile) << std::endl;
				std::clog << "__logfile: " << std::quoted(__logfile) << std::endl;
				std::clog << "__errfile: " << std::quoted(__errfile) << std::endl;
				std::clog << "__host: " << std::quoted(__host) << std::endl;
				std::clog << "__port: " << std::quoted(__port) << std::endl;
				std::clog << "__target: " << std::quoted(__target) << std::endl;
				std::clog << "__json_string: " << std::quoted(__json_string) << std::endl;
			}
			if (__ddd_mode)
			{
				std::clog << "__args:\n";
				for (const auto & arg: __args)
					std::clog << "\t" << std::quoted(arg) << std::endl;
				std::clog << "__transformed_args:\n";
				for (const auto & arg: __transformed_args)
					std::clog << "\t" << std::quoted(arg) << std::endl;
			}
			if (__d_mode || __dd_mode || __ddd_mode)
				std::clog << "================================================================\n\n";
		}
	};	// class args
}	// namespace sopp

namespace sopp::net
{
	class sopp_client final:
		virtual public std::enable_shared_from_this<sopp::net::sopp_client>
	{
	private:
		std::shared_ptr<sopp::output> __out{nullptr};
		std::shared_ptr<sopp::args> __args{nullptr};
		asio::ip::tcp::endpoint __connected_ep{};
	private:
		std::unique_ptr<asio::ip::tcp::resolver> __resolver{nullptr};
		std::unique_ptr<beast::tcp_stream> __tcp_stream{nullptr};
	public:
		sopp_client(
			std::shared_ptr<sopp::output> out__,
			std::shared_ptr<sopp::args> args__
		):
			__out{out__},
			__args{args__}
		{
		}
	public:
		asio::awaitable<void> start()
		{
			auto ep_list = co_await this->resolve();
			co_await this->connect(std::move(ep_list));
			co_await this->request();
			co_await this->receive();
			co_return;
		}
	private:
		asio::awaitable<asio::ip::tcp::resolver::results_type> resolve()
		{
			__resolver = std::make_unique<asio::ip::tcp::resolver>(co_await asio::this_coro::executor);
			auto [ec, ep_list] = co_await __resolver->async_resolve(
				__args->host(),
				__args->port(),
				asio::as_tuple
			);
			if (ec)
				throw std::system_error{ec, "Resolve Error!"};
			if (__args->verbose())
				std::clog << "Realtime Status: Resolve OK!" << std::endl;
			__out->log()<< "Resolve OK!" << std::endl;
			// else
			co_return ep_list;
		}
	private:
		asio::awaitable<void> connect(auto && ep_list)
		{
			__tcp_stream = std::make_unique<beast::tcp_stream>(co_await asio::this_coro::executor);
			auto [ec, ep] = co_await __tcp_stream->async_connect(
				std::move(ep_list),
				asio::as_tuple
			);
			if (ec)
				throw std::system_error{ec, "Connect Error!"};
			if (__args->verbose())
				std::clog << "Realtime Status: Connected to: " << ep << std::endl;
			__out->log() << "Connected to: " << ep << std::endl;
			__connected_ep = std::move(ep);
			co_return;
		}
	private:
		asio::awaitable<void> request()
		{
			http::request<http::string_body> request;
			request.version(11);
			request.target(__args->target());
			request.method(http::verb::post);
			request.set(http::field::host, (std::ostringstream{} << __connected_ep).str());
			request.set(http::field::user_agent, "c++ sopp json-rpc client");
			request.set(http::field::content_type, "application/x-www-form-urlencoded");
			request.body() = __args->json_string();
			request.prepare_payload();
			auto [ec, bytes] = co_await http::async_write(
				*__tcp_stream,
				request,
				asio::as_tuple
			);
			if (bytes < 1 || (ec && ec != asio::error::eof))
				throw std::system_error{ec, "Request Error!"};
			if (__args->verbose())
				std::clog << "Realtime Status: Request OK!" << std::endl;
			__out->log() << "Request OK!" << std::endl;
			co_return;
		}
	private:
		asio::awaitable<void> receive()
		{
			http::response<http::string_body> response;
			beast::flat_buffer buffer;
			auto [ec, bytes] = co_await http::async_read(
				*__tcp_stream,
				buffer,
				response,
				asio::as_tuple
			);
			if (bytes < 1 || (ec && ec != asio::error::eof))
				throw std::system_error{ec, "Receive Error!"};
			if (__args->verbose())
				std::clog << "Realtime Status: Receive OK!" << std::endl;
			__out->log() << "Receive OK!" << std::endl;
			__out->log() << "Received ==========>\n" << response << "<========= Received End.\n\n";
			__out->out() << response.body() << std::flush;
			co_return;
		}
	};	// class sopp_client final
}	// namespace sopp::net

int main(int argc, char ** argv)
{
	std::cout << std::boolalpha;
	std::cerr << std::boolalpha;
	std::clog << std::boolalpha;

	std::shared_ptr<sopp::output> out = nullptr;
	std::shared_ptr<sopp::args> args = nullptr;

	// [out-test] Make, test and guarantee output object.
	try
	{
		for (int i=0; i<3; ++i)
		{
			out = std::make_shared<sopp::output>();
			for (std::ostringstream & ou:
				{
					std::ref(out->out()),
					std::ref(out->log()),
					std::ref(out->err())
				}
			)
			{
				if (ou.str() != "")
					throw sopp::fatal_error{"sopp::output object test: empty test failed."};
				ou << "Rea @!^< &^ae\r ab\r" << std::flush;
				ou << " Rea e@ _ ^^ 	\t\n" << '\r';
				ou << "\n" << std::flush;
				ou << std::endl;
				if (ou.str() != "Rea @!^< &^ae\r ab\r Rea e@ _ ^^ \t\t\n\r\n\n")
					throw sopp::fatal_error{"sopp::output object test: write test failed!"};
			}
		}
		out = std::make_shared<sopp::output>();
		if (! out)
			throw sopp::fatal_error{"sopp::output object out is not created!"};
	}
	catch (const sopp::fatal_error & e)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [out-test] (c++ sopp::fatal_error) :\n"
			<< e.what()
			<< std::endl;
		return 1;
	}
	catch (const std::exception & e)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [out-test] (c++ std::exception) :\n"
			<< e.what()
			<< std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [out-test] (c++ unknown error) .\n"
			<< std::endl;
		return 1;
	}
	// [out-test] ends.

	// [parse-args]
	try
	{
		args = std::make_shared<sopp::args>(argc, argv);
		if (! args)
			throw sopp::fatal_error{"args object is not created."};
		args->parse();
		if (! args)
			throw sopp::fatal_error{"Post-Test: args object is not created."};
	}
	catch (const sopp::args_error_show_help & e)
	{
		if (args)
			args->print_help();
		else
			std::cerr << "args object is not created.\n";
		return 1;
	}
	catch (const sopp::args_error_show_msg & e)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [parse-args] (c++ sopp::args_error_msg) :\n"
			<< e.what()
			<< std::endl;
		return 1;
	}
	catch (const sopp::fatal_error & e)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [parse-args] (c++ sopp::fatal_error) :\n"
			<< e.what()
			<< std::endl;
		return 1;
	}
	catch (const std::exception & e)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [parse-args] (c++ std::exception) :\n"
			<< e.what()
			<< std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "----------------------------------------------------------------------\n";
		std::cerr << "Fatal Error [parse-args] (c++ unknown error) .\n"
			<< std::endl;
		return 1;
	}
	// [parse-args] ends.

	// [net-request]
	try
	{
		asio::thread_pool pool{32u};
		std::promise<void> exc_pro;
		std::future<void> exc_fu = exc_pro.get_future();
		asio::co_spawn(
			pool.get_executor(),
			std::bind(
				&sopp::net::sopp_client::start,
				std::make_shared<sopp::net::sopp_client>(
					out,
					args
				)
			),
			[&exc_pro] (std::exception_ptr eptr)
			{
				if (eptr)
					exc_pro.set_exception(eptr);
				else
					exc_pro.set_value();
			}
		);
		exc_fu.get();	// This will throw exception if it has.
		pool.join();
	}
	catch (const std::system_error & e)
	{
		out->err()
			<< "----------------------------------------------------------------------\n"
			<< "----------------------------------------------------------------------\n"
			<< "Fatal Error [net-request] (c++ std::system_error) :\n"
			<< e.what()
			<< std::endl;
	}
	catch (const std::exception & e)
	{
		out->err()
			<< "----------------------------------------------------------------------\n"
			<< "----------------------------------------------------------------------\n"
			<< "Fatal Error [net-request] (c++ std::exception) :\n"
			<< e.what()
			<< std::endl;
	}
	catch (...)
	{
		out->err()
			<< "----------------------------------------------------------------------\n"
			<< "----------------------------------------------------------------------\n"
			<< "Fatal Error [net-request] (c++ unknown error) .\n"
			<< std::endl;
	}
	// [net-request] ends.
	
	// todo: from this line to end.

	bool cout_to_file = false;
	bool cerr_to_file = false;
	bool clog_to_file = false;

	if (! args->allfile().empty())
	{
		std::ofstream file{args->allfile(), std::ios::trunc};
		file << "==== std::cerr ====\n";
		file << out->err().str() << std::endl;
		file << "==== std::clog ====\n";
		file << out->log().str() << std::endl;
		file << "==== std::cout ====\n";
		file << out->out().str() << std::endl;
		cout_to_file = true;
		cerr_to_file = true;
		clog_to_file = true;
	}

	if (! args->outfile().empty())
	{
		std::ofstream file{args->outfile(), std::ios::trunc};
		file << out->out().str() << std::flush;
		cout_to_file = true;
	}

	if (! args->logfile().empty())
	{
		std::ofstream file{args->logfile(), std::ios::trunc};
		file << out->log().str() <<std::endl;
		clog_to_file = true;
	}

	if (! args->errfile().empty())
	{
		std::ofstream file{args->errfile(), std::ios::trunc};
		file << out->err().str() << std::endl;
		cerr_to_file = true;
	}

	if (cout_to_file && cerr_to_file && clog_to_file)
	{
		return 1;	// no std::out console output, return non-zero.
	}

	// Exception Propagate
	if (! cerr_to_file && ! out->err().str().empty())
	{
		std::cerr << "_|\n"
			<< out->err().str() << std::flush;
		return 1;
	}

	// Verbose mode, no matter ever.
	if (args->verbose() && ! out->log().str().empty())
	{
		std::clog << "_$|\n"
			<< "|||\n|||\n|||\n"
			<< out->log().str() << std::flush;
	}

	if (! cout_to_file || (args->allfile().empty() && args->outfile().empty()))
	{
		std::cout << out->out().str() << std::flush;
		return 0;
	}
}

