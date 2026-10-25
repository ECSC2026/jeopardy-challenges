
#pragma once

#include <cstdint>


enum RequestField : uint32_t {
    RequestField_MethodID = 0x075BD0EA
};


enum ResponseField : uint32_t {
    ResponseField_Result = 0x8834B03A
};


enum ResultCode : uint32_t {
    ResultCode_Success = 0xF5BE76C5,
    ResultCode_InternalError = 0x4B80B6B4,
    ResultCode_InvalidRequest = 0x3122BFA7
};


enum InsecureMethodID : uint32_t {
    InsecureMethodID_KeyAgreement = 0xE9DC0847,
    InsecureMethodID_Proxy = 0x3BA9BEDC
};


enum SecureMethodID : uint32_t {
    SecureMethodID_ResetGame = 0x997DDAA3,
    SecureMethodID_SolveLevel = 0x704A32EB
};


enum KeyAgreementRequestField : uint32_t {
    KeyAgreementRequestField_ClientPublicKey = 0xF4F4F477,
    KeyAgreementRequestField_ClientNonce = 0x6CB648F8
};


enum KeyAgreementResponseField : uint32_t {
    KeyAgreementResponseField_ServerPublicKey = 0x9523F7C6,
    KeyAgreementResponseField_ServerNonce = 0xC548C780,
    KeyAgreementResponseField_Signature = 0xE67A9176
};


enum ProxyRequestField : uint32_t {
    ProxyRequestField_Payload = 0x2169BA88
};


enum ProxyResponseField : uint32_t {
    ProxyResponseField_Payload = 0x3BA159BD
};


enum ResetGameResponseField : uint32_t {
    ResetGameResponseField_Color = 0x03F597F3
};


enum SolveLevelRequestField : uint32_t {
    SolveLevelRequestField_Hash = 0xE8CE6188
};


enum SolveLevelResponseField : uint32_t {
    SolveLevelResponseField_Color = 0x36A961FA,
    SolveLevelResponseField_Flag = 0x2BACC4EA
};


enum Color : uint32_t {
    Color_Red = 0x43FBA9BA,
    Color_Green = 0x2675C224,
    Color_Blue = 0xC83C9461,
    Color_Yellow = 0xA098F73C
};
