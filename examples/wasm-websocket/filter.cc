// NOLINT(namespace-envoy)
#include <algorithm>
#include <google/protobuf/message.h>
#include <google/protobuf/stubs/status.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <sstream>

#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"

#include "google/protobuf/util/json_util.h"

#include "examples/wasm-websocket/api/api.pb.h"
#include "examples/wasm-websocket/filter.h"

static constexpr char EnforcerServiceName[] = "api.EnforcerWebSocketService";
static constexpr char PublishFrameData[] = "PublishFrameData";

using google::protobuf::util::JsonParseOptions;
using google::protobuf::util::error::Code;
using google::protobuf::util::Status;

using api::WebSocketFrameRequest;
using api::WebSocketFrameResponse;
using config::Config;


static RegisterContextFactory register_MgwWebSocketContext(CONTEXT_FACTORY(MgwWebSocketContext),
                                                      ROOT_FACTORY(MgwWebSocketRootContext),
                                                      "my_root_id");

bool MgwWebSocketRootContext::onStart(size_t) {
  LOG_INFO("onStart");
  return true;
}

bool MgwWebSocketRootContext::onConfigure(size_t config_size) {
  LOG_INFO("onConfigure called");
  proxy_set_tick_period_milliseconds(10000); // 1 sec
  const WasmDataPtr configuration = getBufferBytes(WasmBufferType::PluginConfiguration, 0, config_size);

    JsonParseOptions json_options;
    // auto *c = dynamic_cast<google::protobuf::Message*>(&config_);
    const Status options_status = google::protobuf::util::JsonStringToMessage(
        configuration->toString(),
        &config_, json_options);
    if (options_status != Status::OK) {
      LOG_WARN("Cannot parse plugin configuration JSON string: " + configuration->toString());
      return false;
    }
    LOG_INFO("Loading Config: " + config_.clustername());
  return true;
}

// void MgwWebSocketRootContext::onTick() { //LOG_TRACE("onTick"); }

void MgwWebSocketContext::onCreate() { LOG_INFO(std::string("onCreate " + std::to_string(id()))); }

FilterHeadersStatus MgwWebSocketContext::onRequestHeaders(uint32_t, bool) {
  LOG_INFO(std::string("onRequestHeaders called ") + std::to_string(id()));
  this->stream_handler_ = new MgwGrpcStreamHandler(this);
  std::stringstream pointer_address;
  pointer_address << this->stream_handler_;
  LOG_INFO("handler pointer created >>>"+ pointer_address.str());
  //this->stream_handler_ = MyGrpcCallStreamHandler<EchoRequest, EchoReply>(new MyGrpcCallStreamHandler(this));
  GrpcService grpc_service;
  MgwWebSocketRootContext *r = dynamic_cast<MgwWebSocketRootContext*>(root());
  grpc_service.mutable_envoy_grpc()->set_cluster_name(r->config_.clustername());  
  std::string grpc_service_string;
  grpc_service.SerializeToString(&grpc_service_string);
  HeaderStringPairs initial_metadata;
  initial_metadata.push_back(std::pair("parent", "bar"));
  auto res1 = root()->grpcStreamHandler(grpc_service_string, EnforcerServiceName, PublishFrameData, initial_metadata, std::unique_ptr<GrpcStreamHandlerBase>(this->stream_handler_));
  if (res1 != WasmResult::Ok) {
    LOG_INFO(">>>>>>>>>>>>>>>>>>> Calling gRPC server failed: " + toString(res1));
  }else{
    this->handler_state_ = HandlerState::OK;
    LOG_INFO(std::string(">>>>>>>>>>>>>>>>>>>> gRPC stream initiated"));     
  }
  std::string jwt_string = "Hello !";
  WebSocketFrameRequest request;
  request.set_name(jwt_string);
  google::protobuf::Struct metadata;
  // auto buf1 = getProperty<std::string>({"metadata", "filter_metadata", "envoy.filters.http.ext_authz","envoy.filters.http.ext_authz"});
  // if (buf1.has_value()) {
  //   // if (buf1.value()->size() == 0) {
  //   //   metadata = null;
  //   // }
  //   bool val = metadata.ParseFromArray(buf1.value()->data(), buf1.value()->size());
  //   if (val == true){
  //     LOG_INFO("TRUEEEEEEE");
  //   }else{
  //     LOG_INFO("FALSEEEE");
  //   }
  // }
  
  if (!getMessageValue<google::protobuf::Struct>(
          {"dynamic_metadata", "filter_metadata", "envoy.filters.http.ext_authz"}, &metadata)) {
    LOG_ERROR(std::string("filter_metadata Error ") + std::to_string(id()));
  }

  std::string api_key;
  if (!getValue(
          {"metadata", "filter_metadata", "envoy.filters.http.ext_authz", "apiKey"}, &api_key)) {
    LOG_ERROR(std::string("filter_metadata Error ") + std::to_string(id()));
  }
  LOG_INFO("apikey ::::::::::"+ std::string(api_key));


  // *request.mutable_ext_metadata() = metadata;
  // if(this->handler_state_ == HandlerState::OK){
  //   LOG_INFO(std::string("stream available sending message"));
  //   auto res = this->stream_handler_->send(request, false);
  //   if (res != WasmResult::Ok) {
  //     LOG_INFO(std::string("error sending gRPC >>>>>>>")+ toString(res));
  //   }
  //   LOG_INFO(std::string("grpc sent:"+ toString(res)));
  // }



  // auto buf1 = getProperty<std::string>({"metadata", "filter_metadata", "envoy.filters.http.ext_authz"});
  // if (buf1.has_value()){
  //   decltype(buf1) type;
  //   LOG_INFO("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM buf1");
  // }

  // auto buf2 = getProperty<std::string>({"metadata", "filter_metadata", "envoy.filters.http.ext_authz", "envoy.filters.http.ext_authz"});
  // if (buf2.has_value()){
  //   LOG_INFO("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM buf2");
  // }

  // auto buf3 = getProperty<std::string>({"metadata", "filter_metadata", "envoy.filters.http.ext_authz","apiKey"});
  // if (buf3.has_value()){
  //   LOG_INFO("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM buf3");
  // }



  
  
  // auto result = getRequestHeaderPairs();
  // auto pairs = result->pairs();
  // LOG_INFO(std::string("headers: ") + std::to_string(pairs.size()));
  // for (auto& p : pairs) {
  //   LOG_INFO(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  // }

  //std::string jwt_string = "Hello !";
  // if (!getValue(
  //         {"metadata", "filter_metadata", "envoy.filters.http.jwt_authn", "my_payload", "sub"}, &jwt_string)) {
  //   LOG_ERROR(std::string("filter_metadata Error ") + std::to_string(id()));
  // }

  //LOG_INFO(">>>>>>>>>>>>>  Calling GRPC for sub:" + jwt_string);
  // ExampleRootContext *a = dynamic_cast<ExampleRootContext*>(root());
  // GrpcService grpc_service;
  // grpc_service.mutable_envoy_grpc()->set_cluster_name(a->config_.clustername());  
  // std::string grpc_service_string;
  // grpc_service.SerializeToString(&grpc_service_string);

  // EchoRequest request;
  // request.set_name(jwt_string);
  // std::string st2r = request.SerializeAsString();
  // HeaderStringPairs initial_metadata;
  // initial_metadata.push_back(std::pair("parent", "bar"));
  // auto res =  root()->grpcCallHandler(grpc_service_string, EchoServerServiceName, SayHelloMethodName, initial_metadata, st2r, 1000,
  //                             std::unique_ptr<GrpcCallHandlerBase>(new MyGrpcCallHandler(this)));

  // if (res != WasmResult::Ok) {
  //   LOG_ERROR("Calling gRPC server failed: " + toString(res));
  // }                         

  return FilterHeadersStatus::Continue;

  //addRequestHeader("fromenvoy", "newheadervalue");
  //return FilterHeadersStatus::Continue;
}

FilterHeadersStatus MgwWebSocketContext::onResponseHeaders(uint32_t, bool) {
  LOG_INFO(std::string("onResponseHeaders called ") + std::to_string(id()));
  auto result = getResponseHeaderPairs();
  auto pairs = result->pairs();
  LOG_INFO(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    LOG_INFO(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  //addResponseHeader("X-Wasm-custom", "FOO");
  //replaceResponseHeader("content-type", "text/plain; charset=utf-8");
  //removeResponseHeader("content-length");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus MgwWebSocketContext::onRequestBody(size_t body_buffer_length,
                                               bool /* end_of_stream */) {
  auto body = getBufferBytes(WasmBufferType::HttpRequestBody, 0, body_buffer_length);
  LOG_INFO(std::string("onRequestBody ") + std::string(body->view()));
  auto data = body->view();
  int opcode = data[0] & 0x0F;
  std::string s = std::to_string(opcode);
  LOG_INFO(s);
  // std::string jwt_string = "Hello !";
  // WebSocketFrameRequest request;
  // request.set_name(jwt_string);
  // google::protobuf::Struct metadata;
  // if (!getMessageValue<google::protobuf::Struct>(
  //         {"metadata", "filter_metadata", "envoy.filters.http.ext_authz"}, &metadata)) {
  //   LOG_ERROR(std::string("filter_metadata Error ") + std::to_string(id()));
  // }
  // *request.mutable_metadata() = metadata;
  // if(this->handler_state_ == HandlerState::OK){
  //   LOG_INFO(std::string("stream available sending message"));
  //   auto res = this->stream_handler_->send(request, false);
  //   if (res != WasmResult::Ok) {
  //     LOG_INFO(std::string("error sending gRPC >>>>>>>")+ toString(res));
  //   }
  //   LOG_INFO(std::string("grpc sent:"+ toString(res)));
  // }
  
  

  // if(this->is_stream_ == true){
  //   GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>* handler = dynamic_cast<std::unique_ptr<GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>>> (this->stream_handler_);
  //   if(handler != nullptr){
  //     handler->send(request, false);
  //   }
  // }
  return FilterDataStatus::Continue;
}

FilterDataStatus MgwWebSocketContext::onResponseBody(size_t /* body_buffer_length */,
                                                bool /* end_of_stream */) {
  //setBuffer(WasmBufferType::HttpResponseBody, 0, 12, "Hello, world");
  return FilterDataStatus::Continue;
}

void MgwWebSocketContext::onDone() { LOG_WARN(std::string("onDone " + std::to_string(id()))); }

void MgwWebSocketContext::onLog() { LOG_WARN(std::string("onLog " + std::to_string(id()))); }

void MgwWebSocketContext::onDelete() { 
  LOG_WARN(std::string("onDelete " + std::to_string(id())));
  std::stringstream pointer_address;
  pointer_address << this->stream_handler_;
  LOG_INFO("handler pointer delete >>>"+ pointer_address.str());
  this->stream_handler_->close();
 }

// void ExampleContext::updateConnectionStatus(bool status){
//   this->is_stream_ = status;
// }

void MgwWebSocketContext::updateFilterState(ResponseStatus status){
  LOG_INFO(std::string("updateFilterState") + std::to_string(static_cast<int>(status)));
}

void MgwWebSocketContext::updateHandlerState(HandlerState state){
    LOG_INFO(std::string("updateHandlerState") + std::to_string(static_cast<int>(state)));
    this->handler_state_ = state;
}