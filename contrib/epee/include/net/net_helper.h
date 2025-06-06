  // Copyright (c) 2019, anonimal, <anonimal@zano.org>
// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 




#pragma once

//#include <Winsock2.h>
//#include <Ws2tcpip.h>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/preprocessor/selection/min.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include "net/net_utils_base.h"
#include "misc_language.h"
#include "misc_helpers.h"
//#include "profile_tools.h"
#include "../string_tools.h"

#ifndef MAKE_IP
#define MAKE_IP( a1, a2, a3, a4 )	(a1|(a2<<8)|(a3<<16)|(a4<<24))
#endif


namespace epee
{
  namespace net_utils
  {

    template<bool is_ssl>
    struct socket_backend;


    template<>
    struct socket_backend<true>
    {
      socket_backend(boost::asio::io_service& _io_service): m_ssl_context(boost::asio::ssl::context::sslv23), m_socket(_io_service, m_ssl_context)
      {
        // Create a context that uses the default paths for
        // finding CA certificates.
        m_ssl_context.set_default_verify_paths();
        /*m_socket.set_verify_mode(boost::asio::ssl::verify_peer);
        m_socket.set_verify_callback(
          boost::bind(&socket_backend::verify_certificate, this, _1, _2));*/

      }

      /*
      bool verify_certificate(bool preverified,
        boost::asio::ssl::verify_context& ctx)
      {
        std::cout << "verify_certificate (preverified " << preverified << " ) ...\n";
        // The verify callback can be used to check whether the certificate that is
        // being presented is valid for the peer. For example, RFC 2818 describes
        // the steps involved in doing this for HTTPS. Consult the OpenSSL
        // documentation for more details. Note that the callback is called once
        // for each certificate in the certificate chain, starting from the root
        // certificate authority.

        // In this example we will simply print the certificate's subject name.
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::cout << "Verifying " << subject_name << "\n";

        // dummy verification
        return true;
      }*/

      void set_domain(const std::string& domain_name)
      {
        SSL_set_tlsext_host_name(m_socket.native_handle(), domain_name.c_str());
      }

      boost::asio::ip::tcp::socket& get_socket()
      {
        return m_socket.next_layer();
      }

      auto& get_stream()
      {
        return m_socket;
      }

      void on_after_connect()
      {
        LOG_PRINT_L2("SSL Handshake....");
        m_socket.handshake(boost::asio::ssl::stream_base::client);
        LOG_PRINT_L2("SSL Handshake OK");
      }

    private: 
      boost::asio::ssl::context m_ssl_context;
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;
    };

    template<>
    struct socket_backend<false>
    {
      socket_backend(boost::asio::io_service& _io_service): m_socket(_io_service)
      {}

      boost::asio::ip::tcp::socket& get_socket()
      {
        return m_socket;
      }

      void set_domain(const std::string& domain_name)
      {
        
      }

      boost::asio::ip::tcp::socket& get_stream()
      {
        return m_socket;
      }

      void on_after_connect()
      {

      }

      void reset()
      {

      }
    private:
      boost::asio::ip::tcp::socket m_socket;
    };


    template<bool is_ssl>
    struct socket_backend_resetable
    {
      socket_backend_resetable(boost::asio::io_service& _io_service) : mr_io_service(_io_service), m_pbackend(std::make_shared<socket_backend<is_ssl>>(_io_service))
      {}

      boost::asio::ip::tcp::socket& get_socket()
      {
        return m_pbackend->get_socket();
      }

      void set_domain(const std::string& domain_name)
      {
        return m_pbackend->set_domain(domain_name);
      }

      auto& get_stream()
      {
        return m_pbackend->get_stream();
      }

      void on_after_connect()
      {
        return m_pbackend->on_after_connect();
      }

      void reset()
      {
        m_pbackend = std::make_shared<socket_backend<is_ssl>>(mr_io_service);
      }

    private: 
      boost::asio::io_service& mr_io_service;
      std::shared_ptr<socket_backend<is_ssl>> m_pbackend;
    };



    template<bool is_ssl>
    class blocked_mode_client_t
    {
      struct handler_obj
      {
        handler_obj(boost::system::error_code& error, size_t& bytes_transferred) :ref_error(error), ref_bytes_transferred(bytes_transferred)
        {}
        handler_obj(const handler_obj& other_obj) :ref_error(other_obj.ref_error), ref_bytes_transferred(other_obj.ref_bytes_transferred)
        {}

        boost::system::error_code& ref_error;
        size_t& ref_bytes_transferred;

        void operator()(const boost::system::error_code& error, // Result of operation.
          std::size_t bytes_transferred           // Number of bytes read.
          )
        {
          ref_error = error;
          ref_bytes_transferred = bytes_transferred;
        }
      };

    public:
      inline
        blocked_mode_client_t() :m_sct_back(m_io_service),
        m_initialized(false),
        m_connected(false),
        m_deadline(m_io_service),
        m_shutdowned(0),
        m_connect_timeout{},
        m_reciev_timeout{}
      {


        m_initialized = true;


        // No deadline is required until the first socket operation is started. We
        // set the deadline to positive infinity so that the actor takes no action
        // until a specific deadline is set.
        m_deadline.expires_at(boost::posix_time::pos_infin);

        // Start the persistent actor that checks for deadline expiry.
        check_deadline();

      }
      inline
        ~blocked_mode_client_t()
      {
        NESTED_TRY_ENTRY();

        //profile_tools::local_coast lc("~blocked_mode_client()", 3);
        shutdown();

        NESTED_CATCH_ENTRY(__func__);
      }

      inline void set_recv_timeout(int reciev_timeout)
      {
        m_reciev_timeout = reciev_timeout;
      }

      inline
        bool connect(const std::string& addr, int port, unsigned int connect_timeout, unsigned int reciev_timeout, const std::string& bind_ip = "0.0.0.0")
      {
        return connect(addr, std::to_string(port), connect_timeout, reciev_timeout, bind_ip);
      }

      inline
        bool connect(const std::string& addr, const std::string& port, unsigned int connect_timeout, unsigned int reciev_timeout, const std::string& bind_ip = "0.0.0.0")
      {
        LOG_PRINT_L1("Connecting to " << addr << ":" << port << ", cn_timeout: " << connect_timeout << ", rv_timeout: " << reciev_timeout);
        m_connect_timeout = connect_timeout;
        m_reciev_timeout = reciev_timeout;
        m_connected = false;
        if (!m_reciev_timeout)
          m_reciev_timeout = m_connect_timeout;

        try
        {
          m_sct_back.get_socket().close();
          // Get a list of endpoints corresponding to the server name.

          m_sct_back.reset();
          //////////////////////////////////////////////////////////////////////////

          boost::asio::ip::tcp::resolver resolver(m_io_service);
          boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), addr, port);
          boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
          boost::asio::ip::tcp::resolver::iterator end;
          if (iterator == end)
          {
            LOG_ERROR("Failed to resolve " << addr);
            return false;
          }

          //////////////////////////////////////////////////////////////////////////
          m_sct_back.set_domain(addr);

          //boost::asio::ip::tcp::endpoint remote_endpoint(boost::asio::ip::address::from_string(addr.c_str()), port);
          boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);


          m_sct_back.get_socket().open(remote_endpoint.protocol());
          if (bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "")
          {
            boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(addr.c_str()), 0);
            m_sct_back.get_socket().bind(local_endpoint);
          }


          m_deadline.expires_from_now(boost::posix_time::milliseconds(m_connect_timeout));


          boost::system::error_code ec = boost::asio::error::would_block;

          //m_sct_back.get_socket().connect(remote_endpoint);
          m_sct_back.get_socket().async_connect(remote_endpoint, boost::lambda::var(ec) = boost::lambda::_1);
          while (ec == boost::asio::error::would_block)
          {
            m_io_service.run_one();
          }

          if (!ec && m_sct_back.get_socket().is_open())
          {
            m_sct_back.on_after_connect();
            m_connected = true;       
            m_deadline.expires_at(boost::posix_time::pos_infin);
            LOG_PRINT_L1("Connected OK: " << addr << ":" << port);
            return true;
          }
          else
          {
            LOG_PRINT("Error on connect to " << addr << ":" << port << ", message: " << ec.message(), LOG_LEVEL_3);
            return false;
          }

        }
        catch (const boost::system::system_error& er)
        {
          LOG_PRINT("Some problems at connect, message: " << er.what(), LOG_LEVEL_4);
          return false;
        }
        catch (...)
        {
          LOG_PRINT("Some fatal problems.", LOG_LEVEL_4);
          return false;
        }

        return true;
      }


      inline
        bool disconnect()
      {
        try
        {
          if (m_connected)
          {
            m_connected = false;
            m_sct_back.get_socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);

          }
        }

        catch (const boost::system::system_error& /*er*/)
        {
          //LOG_ERROR("Some problems at disconnect, message: " << er.what());
          return false;
        }
        catch (...)
        {
          //LOG_ERROR("Some fatal problems.");
          return false;
        }
        return true;
      }


      inline
        bool send(const std::string& buff)
      {

        try
        {
          m_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

          // Set up the variable that receives the result of the asynchronous
          // operation. The error code is set to would_block to signal that the
          // operation is incomplete. Asio guarantees that its asynchronous
          // operations will never fail with would_block, so any other value in
          // ec indicates completion.
          boost::system::error_code ec = boost::asio::error::would_block;

          // Start the asynchronous operation itself. The boost::lambda function
          // object is used as a callback and will update the ec variable when the
          // operation completes. The blocking_udp_client.cpp example shows how you
          // can use boost::bind rather than boost::lambda.
          boost::asio::async_write(m_sct_back.get_stream(), boost::asio::buffer(buff), boost::lambda::var(ec) = boost::lambda::_1);

          // Block until the asynchronous operation has completed.
          while (ec == boost::asio::error::would_block)
          {
            m_io_service.run_one();
          }

          if (ec)
          {
            LOG_PRINT_L3("Problems at write: " << ec.message());
            m_connected = false;
            return false;
          }
          else
          {
            m_deadline.expires_at(boost::posix_time::pos_infin);
          }
        }

        catch (const boost::system::system_error& er)
        {
          LOG_ERROR("Some problems at connect, message: " << er.what());
          return false;
        }
        catch (...)
        {
          LOG_ERROR("Some fatal problems.");
          return false;
        }

        return true;
      }

      inline
        bool send(const void* data, size_t sz)
      {
        try
        {
          /*
          m_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

          // Set up the variable that receives the result of the asynchronous
          // operation. The error code is set to would_block to signal that the
          // operation is incomplete. Asio guarantees that its asynchronous
          // operations will never fail with would_block, so any other value in
          // ec indicates completion.
          boost::system::error_code ec = boost::asio::error::would_block;

          // Start the asynchronous operation itself. The boost::lambda function
          // object is used as a callback and will update the ec variable when the
          // operation completes. The blocking_udp_client.cpp example shows how you
          // can use boost::bind rather than boost::lambda.
          boost::asio::async_write(m_sct_back.get_socket(), boost::asio::buffer(data, sz), boost::lambda::var(ec) = boost::lambda::_1);

          // Block until the asynchronous operation has completed.
          while (ec == boost::asio::error::would_block)
          {
          m_io_service.run_one();
          }
          */
          boost::system::error_code ec;

          size_t writen = m_sct_back.get_stream().write_some(boost::asio::buffer(data, sz), ec);



          if (!writen || ec)
          {
            LOG_PRINT_L3("Problems at write: " << ec.message());
            m_connected = false;
            return false;
          }
          else
          {
            m_deadline.expires_at(boost::posix_time::pos_infin);
          }
        }

        catch (const boost::system::system_error& er)
        {
          LOG_ERROR("Some problems at send, message: " << er.what());
          m_connected = false;
          return false;
        }
        catch (...)
        {
          LOG_ERROR("Some fatal problems.");
          return false;
        }

        return true;
      }

      bool is_connected()
      {
        return m_connected && m_sct_back.get_socket().is_open();
        //TRY_ENTRY()
        //return m_socket.is_open();
        //CATCH_ENTRY_L0("is_connected", false)
      }

      inline
        bool recv(std::string& buff)
      {

        try
        {
          // Set a deadline for the asynchronous operation. Since this function uses
          // a composed operation (async_read_until), the deadline applies to the
          // entire operation, rather than individual reads from the socket.
          m_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

          // Set up the variable that receives the result of the asynchronous
          // operation. The error code is set to would_block to signal that the
          // operation is incomplete. Asio guarantees that its asynchronous
          // operations will never fail with would_block, so any other value in
          // ec indicates completion.
          //boost::system::error_code ec = boost::asio::error::would_block;

          // Start the asynchronous operation itself. The boost::lambda function
          // object is used as a callback and will update the ec variable when the
          // operation completes. The blocking_udp_client.cpp example shows how you
          // can use boost::bind rather than boost::lambda.

          boost::system::error_code ec = boost::asio::error::would_block;
          size_t bytes_transfered = 0;

          handler_obj hndlr(ec, bytes_transfered);

          char local_buff[10000] = { 0 };
          //m_socket.async_read_some(boost::asio::buffer(local_buff, sizeof(local_buff)), hndlr);
          boost::asio::async_read(m_sct_back.get_stream(), boost::asio::buffer(local_buff, sizeof(local_buff)), boost::asio::transfer_at_least(1), hndlr);

          // Block until the asynchronous operation has completed.
          while (ec == boost::asio::error::would_block && !boost::interprocess::ipcdetail::atomic_read32(&m_shutdowned))
          {
            m_io_service.run_one();
          }


          if (ec)
          {
            LOG_PRINT_L4("READ ENDS: Connection err_code " << ec.value());
            if (ec == boost::asio::error::eof)
            {
              LOG_PRINT_L4("Connection err_code eof.");
              //connection closed there, empty
              return true;
            }

            LOG_PRINT_L3("Problems at read: " << ec.message());
            m_connected = false;
            return false;
          }
          else
          {
            LOG_PRINT_L4("READ ENDS: Success. bytes_tr: " << bytes_transfered);
            m_deadline.expires_at(boost::posix_time::pos_infin);
          }

          /*if(!bytes_transfered)
          return false;*/

          buff.assign(local_buff, bytes_transfered);
          return true;
        }

        catch (const boost::system::system_error& er)
        {
          LOG_ERROR("Some problems at read, message: " << er.what());
          m_connected = false;
          return false;
        }
        catch (...)
        {
          LOG_ERROR("Some fatal problems at read.");
          return false;
        }



        return false;

      }

      inline bool recv_n(std::string& buff, int64_t sz)
      {

        try
        {
          // Set a deadline for the asynchronous operation. Since this function uses
          // a composed operation (async_read_until), the deadline applies to the
          // entire operation, rather than individual reads from the socket.
          m_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

          // Set up the variable that receives the result of the asynchronous
          // operation. The error code is set to would_block to signal that the
          // operation is incomplete. Asio guarantees that its asynchronous
          // operations will never fail with would_block, so any other value in
          // ec indicates completion.
          //boost::system::error_code ec = boost::asio::error::would_block;

          // Start the asynchronous operation itself. The boost::lambda function
          // object is used as a callback and will update the ec variable when the
          // operation completes. The blocking_udp_client.cpp example shows how you
          // can use boost::bind rather than boost::lambda.

          buff.resize(static_cast<size_t>(sz));
          boost::system::error_code ec = boost::asio::error::would_block;
          size_t bytes_transfered = 0;


          handler_obj hndlr(ec, bytes_transfered);

          //char local_buff[10000] = {0};
          boost::asio::async_read(m_sct_back.get_stream(), boost::asio::buffer((char*)buff.data(), buff.size()), boost::asio::transfer_at_least(buff.size()), hndlr);

          // Block until the asynchronous operation has completed.
          while (ec == boost::asio::error::would_block && !boost::interprocess::ipcdetail::atomic_read32(&m_shutdowned))
          {
            m_io_service.run_one();
          }

          if (ec)
          {
            LOG_PRINT_L3("Problems at read: " << ec.message());
            m_connected = false;
            return false;
          }
          else
          {
            m_deadline.expires_at(boost::posix_time::pos_infin);
          }

          if (bytes_transfered != buff.size())
          {
            LOG_ERROR("Transferred missmatch with transfer_at_least value: m_bytes_transferred=" << bytes_transfered << " at_least value=" << buff.size());
            return false;
          }

          return true;
        }

        catch (const boost::system::system_error& er)
        {
          LOG_ERROR("Some problems at read, message: " << er.what());
          m_connected = false;
          return false;
        }
        catch (...)
        {
          LOG_ERROR("Some fatal problems at read.");
          return false;
        }



        return false;
      }

      bool shutdown()
      {
        m_deadline.cancel();
        boost::system::error_code ignored_ec;
        m_sct_back.get_socket().cancel(ignored_ec);
        m_sct_back.get_socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        m_sct_back.get_socket().close(ignored_ec);
        boost::interprocess::ipcdetail::atomic_write32(&m_shutdowned, 1);
        m_connected = false;
        return true;
      }

      void set_connected(bool connected)
      {
        m_connected = connected;
      }
      boost::asio::io_service& get_io_service()
      {
        return m_io_service;
      }

      boost::asio::ip::tcp::socket& get_socket()
      {
        return m_sct_back.get_socket();
      }

    private:

      void check_deadline()
      {
        // Check whether the deadline has passed. We compare the deadline against
        // the current time since a new asynchronous operation may have moved the
        // deadline before this actor had a chance to run.
        if (m_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
        {
          // The deadline has passed. The socket is closed so that any outstanding
          // asynchronous operations are cancelled. This allows the blocked
          // connect(), read_line() or write_line() functions to return.
          LOG_PRINT_L2("Timed out socket");
          m_connected = false;
          m_sct_back.get_socket().close();

          // There is no longer an active deadline. The expiry is set to positive
          // infinity so that the actor takes no action until a new deadline is set.
          m_deadline.expires_at(boost::posix_time::pos_infin);
        }

        // Put the actor back to sleep.
        m_deadline.async_wait(boost::bind(&blocked_mode_client_t<is_ssl>::check_deadline, this));
      }



    protected:
      boost::asio::io_service m_io_service;
      socket_backend_resetable<is_ssl> m_sct_back;//socket_backend<is_ssl> m_sct_back;
      int m_connect_timeout;
      int m_reciev_timeout;
      bool m_initialized;
      bool m_connected;
      boost::asio::deadline_timer m_deadline;
      volatile uint32_t m_shutdowned;
    };


    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<bool is_ssl>
    class async_blocked_mode_client_t : public blocked_mode_client_t<is_ssl>
    {
    public:
      async_blocked_mode_client_t() :m_send_deadline(blocked_mode_client_t<is_ssl>::m_io_service)
      {

        // No deadline is required until the first socket operation is started. We
        // set the deadline to positive infinity so that the actor takes no action
        // until a specific deadline is set.
        m_send_deadline.expires_at(boost::posix_time::pos_infin);

        // Start the persistent actor that checks for deadline expiry.
        check_send_deadline();
      }
      ~async_blocked_mode_client_t()
      {
        m_send_deadline.cancel();
      }

      bool shutdown()
      {
        blocked_mode_client_t<is_ssl>::shutdown();
        m_send_deadline.cancel();
        return true;
      }

      inline
        bool send(const void* data, size_t sz)
      {
        try
        {
          /*
          m_send_deadline.expires_from_now(boost::posix_time::milliseconds(m_reciev_timeout));

          // Set up the variable that receives the result of the asynchronous
          // operation. The error code is set to would_block to signal that the
          // operation is incomplete. Asio guarantees that its asynchronous
          // operations will never fail with would_block, so any other value in
          // ec indicates completion.
          boost::system::error_code ec = boost::asio::error::would_block;

          // Start the asynchronous operation itself. The boost::lambda function
          // object is used as a callback and will update the ec variable when the
          // operation completes. The blocking_udp_client.cpp example shows how you
          // can use boost::bind rather than boost::lambda.
          boost::asio::async_write(m_socket, boost::asio::buffer(data, sz), boost::lambda::var(ec) = boost::lambda::_1);

          // Block until the asynchronous operation has completed.
          while(ec == boost::asio::error::would_block)
          {
          m_io_service.run_one();
          }*/

          boost::system::error_code ec;

          size_t writen = blocked_mode_client_t<is_ssl>::m_sct_back.get_socket().write_some(boost::asio::buffer(data, sz), ec);

          if (!writen || ec)
          {
            LOG_PRINT_L3("Problems at write: " << ec.message());
            return false;
          }
          else
          {
            m_send_deadline.expires_at(boost::posix_time::pos_infin);
          }
        }

        catch (const boost::system::system_error& er)
        {
          LOG_ERROR("Some problems at connect, message: " << er.what());
          return false;
        }
        catch (...)
        {
          LOG_ERROR("Some fatal problems.");
          return false;
        }

        return true;
      }


    private:

      boost::asio::deadline_timer m_send_deadline;

      void check_send_deadline()
      {
        // Check whether the deadline has passed. We compare the deadline against
        // the current time since a new asynchronous operation may have moved the
        // deadline before this actor had a chance to run.
        if (m_send_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
        {
          // The deadline has passed. The socket is closed so that any outstanding
          // asynchronous operations are cancelled. This allows the blocked
          // connect(), read_line() or write_line() functions to return.
          LOG_PRINT_L3("Timed out socket");
          blocked_mode_client_t<is_ssl>::m_sct_back.get_socket().close();

          // There is no longer an active deadline. The expiry is set to positive
          // infinity so that the actor takes no action until a new deadline is set.
          m_send_deadline.expires_at(boost::posix_time::pos_infin);
        }

        // Put the actor back to sleep.
        m_send_deadline.async_wait(boost::bind(&async_blocked_mode_client_t<is_ssl>::check_send_deadline, this));
      }
    };

    typedef blocked_mode_client_t<false> blocked_mode_client;
    typedef async_blocked_mode_client_t<false> async_blocked_mode_client;
  }
}
