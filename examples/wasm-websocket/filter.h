#pragma once

#include "proxy_wasm_intrinsics.h"
#include <memory>

#include "examples/wasm-websocket/handler_impl.h"
#include "examples/wasm-websocket/handler.h"


class MgwWebSocketRootContext: public RootContext{
public:
  explicit MgwWebSocketRootContext(uint32_t id, std::string_view root_id) : RootContext(id, root_id) {}

  bool onStart(size_t) override;
  bool onConfigure(size_t) override;
  void onTick() override;

  api::Config config_;
};

class MgwWebSocketContext : public Context , 
                            public HandlerCallbacks {
public:
  explicit MgwWebSocketContext(uint32_t id, RootContext* root) : Context(id, root) {}

  void onCreate() override;
  FilterHeadersStatus onRequestHeaders(uint32_t headers, bool end_of_stream) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterHeadersStatus onResponseHeaders(uint32_t headers, bool end_of_stream) override;
  FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) override;
  void onDone() override;
  void onLog() override;
  void onDelete() override;
  
  void updateFilterState(ResponseStatus status) override;
  void updateHandlerState(HanlderState state) override;
  

private:
  MgwGrpcStreamHandler* stream_handler_{};
  bool is_stream_ = false;

};


