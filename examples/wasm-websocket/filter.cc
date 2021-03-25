// NOLINT(namespace-envoy)
#include <algorithm>
#include <google/protobuf/stubs/status.h>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"
#include "proxy_wasm_intrinsics_lite.pb.h"

#include "google/protobuf/util/json_util.h"

#include "examples/envoy-wasm-websocket/echo/echo.pb.h"
#include "examples/envoy-wasm-websocket/filter.h"

static constexpr char EnforcerServiceName[] = "api.EnforcerWebSocketService";
static constexpr char PublishFrameData[] = "PublishFrameData";

using google::protobuf::util::JsonParseOptions;
using google::protobuf::util::error::Code;
using google::protobuf::util::Status;

using echo::WebSocketFrameRequest;
using echo::WebSocketFrameResponse;
using echo::Config;


static RegisterContextFactory register_MgwWebSocketContext(CONTEXT_FACTORY(MgwWebSocketContext),
                                                      ROOT_FACTORY(MgwWebSocketRootContext),
                                                      "my_root_id");

bool ExampleRootContext::onStart(size_t) {
  LOG_INFO("onStart");
  return true;
}

bool ExampleRootContext::onConfigure(size_t config_size) {
  LOG_INFO("onConfigure called");
  proxy_set_tick_period_milliseconds(1000); // 1 sec
  const WasmDataPtr configuration = getBufferBytes(WasmBufferType::PluginConfiguration, 0, config_size);

    JsonParseOptions json_options;
    const Status options_status = JsonStringToMessage(
        configuration->toString(),
        &config_,
        json_options);
    if (options_status != Status::OK) {
      LOG_WARN("Cannot parse plugin configuration JSON string: " + configuration->toString());
      return false;
    }
    LOG_INFO("Loading Config: " + config_.clustername());
  return true;
}

void ExampleRootContext::onTick() { LOG_TRACE("onTick"); }

void ExampleContext::onCreate() { LOG_INFO(std::string("onCreate " + std::to_string(id()))); }

FilterHeadersStatus ExampleContext::onRequestHeaders(uint32_t, bool) {
  LOG_INFO(std::string("onRequestHeaders called ") + std::to_string(id()));
  this->stream_handler_ = std::shared_ptr<StreamHanlderClient>(new MyGrpcCallStreamHandler(this));
  //this->stream_handler_ = MyGrpcCallStreamHandler<EchoRequest, EchoReply>(new MyGrpcCallStreamHandler(this));
  GrpcService grpc_service_1;
  ExampleRootContext *a1 = dynamic_cast<ExampleRootContext*>(root());
  grpc_service_1.mutable_envoy_grpc()->set_cluster_name(a1->config_.clustername());  
  std::string grpc_service_string_1;
  grpc_service_1.SerializeToString(&grpc_service_string_1);
  HeaderStringPairs initial_metadata_1;
  initial_metadata_1.push_back(std::pair("parent", "bar"));
  auto res1 = root()->grpcStreamHandler(grpc_service_string_1, EchoServerServiceName, SayHelloBidiStream, initial_metadata_1, this->stream_handler_);
  if (res1 != WasmResult::Ok) {
    LOG_ERROR("Calling gRPC server failed: " + toString(res1));
  }else{
    this->is_stream_ = true;
    LOG_INFO(std::string("gRPC stream initiated"));     
  }
  auto result = getRequestHeaderPairs();
  auto pairs = result->pairs();
  LOG_INFO(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    LOG_INFO(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }

  std::string jwt_string = "Hello !";
  // if (!getValue(
  //         {"metadata", "filter_metadata", "envoy.filters.http.jwt_authn", "my_payload", "sub"}, &jwt_string)) {
  //   LOG_ERROR(std::string("filter_metadata Error ") + std::to_string(id()));
  // }

  LOG_INFO(">>>>>>>>>>>>>  Calling GRPC for sub:" + jwt_string);
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

FilterHeadersStatus ExampleContext::onResponseHeaders(uint32_t, bool) {
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

FilterDataStatus ExampleContext::onRequestBody(size_t body_buffer_length,
                                               bool /* end_of_stream */) {
  auto body = getBufferBytes(WasmBufferType::HttpRequestBody, 0, body_buffer_length);
  LOG_INFO(std::string("onRequestBody ") + std::string(body->view()));
  std::string jwt_string = "Hello !";
  EchoRequest request;
  request.set_name(jwt_string);
  if(this->is_stream_ == true){
    LOG_INFO(std::string("stream available sending message"));
    auto res = this->stream_handler_->send(request, false);
    if (res != WasmResult::Ok) {
      LOG_INFO(std::string("error sending gRPC >>>>>>>")+ toString(res));
    }
    LOG_INFO(std::string("grpc sent:"+ toString(res)));
  }
  // if(this->is_stream_ == true){
  //   GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>* handler = dynamic_cast<std::unique_ptr<GrpcStreamHandler<google::protobuf::Value, google::protobuf::Value>>> (this->stream_handler_);
  //   if(handler != nullptr){
  //     handler->send(request, false);
  //   }
  // }
  return FilterDataStatus::Continue;
}

FilterDataStatus ExampleContext::onResponseBody(size_t /* body_buffer_length */,
                                                bool /* end_of_stream */) {
  //setBuffer(WasmBufferType::HttpResponseBody, 0, 12, "Hello, world");
  return FilterDataStatus::Continue;
}

void ExampleContext::onDone() { LOG_WARN(std::string("onDone " + std::to_string(id()))); }

void ExampleContext::onLog() { LOG_WARN(std::string("onLog " + std::to_string(id()))); }

void ExampleContext::onDelete() { LOG_WARN(std::string("onDelete " + std::to_string(id()))); }

// void ExampleContext::updateConnectionStatus(bool status){
//   this->is_stream_ = status;
// }

void ExampleContext::updateFilterState(ResponseStatus state){
  LOG_INFO("updateFilterState"+ std::string(state));
}

void ExampleContext::updateHandlerState(HanlderState state){
  LOG_INFO("updateHandlerState"+ std::string(state));
}