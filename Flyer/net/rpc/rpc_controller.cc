#include "Flyer/net/rpc/rpc_controller.h" 

namespace Flyer {

void RpcController::Reset() {
    m_err_code = 0;
    m_err_info = "";
    m_msg_id = "";
    m_is_failed = false;
    m_is_cancled = false;
    m_local_addr = nullptr;
    m_peer_addr = nullptr;
    m_timeout = 1000;
}

bool RpcController::Failed() const {
    return m_is_failed;
}

std::string RpcController::ErrorText() const {
    return m_err_info;
}

void RpcController::StartCancel() {
    m_is_cancled = true;
}

bool RpcController::IsCanceled() const {
    return m_is_cancled;
}

void RpcController::SetFailed(const std::string& reason) {
    m_err_info = reason;
    m_is_failed = true;
}

void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {

}

void RpcController::setError(int32_t error_code, const std::string error_info) {
    m_err_code = error_code;
    m_err_info = error_info;
    m_is_failed = true;
}

int32_t RpcController::getErrorCode() {
    return m_err_code;
}

std::string RpcController::getErrorInfo() {
    return m_err_info;
}

void RpcController::setMsgId(const std::string& msg_id) {
    m_msg_id = msg_id;
}

std::string RpcController::getMsgId() {
    return m_msg_id;
}

void RpcController::setLocalAddr(NetAddr::s_ptr addr) {
    m_local_addr = addr;
}

void RpcController::setPeerAddr(NetAddr::s_ptr addr) {
    m_peer_addr = addr;
}

NetAddr::s_ptr RpcController::getLocalAddr() {
    return m_local_addr;
}

NetAddr::s_ptr RpcController::getPeerAddr() {
    return m_peer_addr;
}

void RpcController::setTimeout(int timeout) {
    m_timeout = timeout;
}

int RpcController::getTimeout() {
    return m_timeout;
}


}