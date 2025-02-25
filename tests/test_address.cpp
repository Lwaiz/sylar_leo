/**
  ******************************************************************************
  * @file           : test_address.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/24
  ******************************************************************************
  */

#include "sylar/address.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    std::vector<sylar::Address::ptr> addrs;

    SYLAR_LOG_INFO(g_logger) << "begin";
    //bool v = sylar::Address::Lookup(addrs, "www.baidu.com");
    bool v = sylar::Address::Lookup(addrs, "localhost:4080");
    SYLAR_LOG_INFO(g_logger) << "end";
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = sylar::Address::LookupAny("localhost:4080");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << *addr;
    } else {
        SYLAR_LOG_INFO(g_logger) << "error";
    }

}


void test_iface() {
    std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t >> results;

    bool v = sylar::Address::GetInterfaceAddresses(results);
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto&i : results) {
        SYLAR_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
                     << i.second.second;
    }
}

void test_ipv4() {
    auto addr = sylar::IPAddress::Create("39.100.72.123");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    }
}


int main() {
    //test();
    //test_iface();
    test_ipv4();

    return 0;
}
