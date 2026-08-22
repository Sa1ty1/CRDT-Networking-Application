#pragma once
#include <cassert>
#include <iostream>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>
#include <functional>
#include <network/server.hpp>
#include <network/network_client.hpp>
#include <network/editor_session.hpp>

void run_io(boost::asio::io_context& io);
void run_io_until(boost::asio::io_context& io, const std::function<bool()>& condition);

void test_basic_connection_and_sync();
void test_operation_routing();
void test_late_client_sync();
void test_disconnect_and_reconnect();
void test_duplicate_operation();
void test_duplicate_client_id();
void test_hello_after_registration();
void test_sync_complete_after_live();
void test_operation_during_sync();
void test_sync_complete_during_sync_is_valid();