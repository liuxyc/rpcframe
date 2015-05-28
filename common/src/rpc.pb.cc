// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: rpc.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "rpc.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* RpcInnerReq_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  RpcInnerReq_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* RpcInnerReq_RPC_TYPE_descriptor_ = NULL;
const ::google::protobuf::Descriptor* RpcInnerResp_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  RpcInnerResp_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_rpc_2eproto() {
  protobuf_AddDesc_rpc_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "rpc.proto");
  GOOGLE_CHECK(file != NULL);
  RpcInnerReq_descriptor_ = file->message_type(0);
  static const int RpcInnerReq_offsets_[5] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, service_name_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, methond_name_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, request_id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, type_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, data_),
  };
  RpcInnerReq_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      RpcInnerReq_descriptor_,
      RpcInnerReq::default_instance_,
      RpcInnerReq_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerReq, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(RpcInnerReq));
  RpcInnerReq_RPC_TYPE_descriptor_ = RpcInnerReq_descriptor_->enum_type(0);
  RpcInnerResp_descriptor_ = file->message_type(1);
  static const int RpcInnerResp_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerResp, request_id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerResp, data_),
  };
  RpcInnerResp_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      RpcInnerResp_descriptor_,
      RpcInnerResp::default_instance_,
      RpcInnerResp_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerResp, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RpcInnerResp, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(RpcInnerResp));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_rpc_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    RpcInnerReq_descriptor_, &RpcInnerReq::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    RpcInnerResp_descriptor_, &RpcInnerResp::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_rpc_2eproto() {
  delete RpcInnerReq::default_instance_;
  delete RpcInnerReq_reflection_;
  delete RpcInnerResp::default_instance_;
  delete RpcInnerResp_reflection_;
}

void protobuf_AddDesc_rpc_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\trpc.proto\"\246\001\n\013RpcInnerReq\022\024\n\014service_n"
    "ame\030\001 \002(\t\022\024\n\014methond_name\030\002 \002(\t\022\022\n\nreque"
    "st_id\030\003 \002(\t\022#\n\004type\030\004 \002(\0162\025.RpcInnerReq."
    "RPC_TYPE\022\014\n\004data\030\005 \002(\t\"$\n\010RPC_TYPE\022\013\n\007ON"
    "E_WAY\020\000\022\013\n\007TWO_WAY\020\001\"0\n\014RpcInnerResp\022\022\n\n"
    "request_id\030\001 \002(\t\022\014\n\004data\030\002 \002(\t", 230);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "rpc.proto", &protobuf_RegisterTypes);
  RpcInnerReq::default_instance_ = new RpcInnerReq();
  RpcInnerResp::default_instance_ = new RpcInnerResp();
  RpcInnerReq::default_instance_->InitAsDefaultInstance();
  RpcInnerResp::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_rpc_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_rpc_2eproto {
  StaticDescriptorInitializer_rpc_2eproto() {
    protobuf_AddDesc_rpc_2eproto();
  }
} static_descriptor_initializer_rpc_2eproto_;

// ===================================================================

const ::google::protobuf::EnumDescriptor* RpcInnerReq_RPC_TYPE_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return RpcInnerReq_RPC_TYPE_descriptor_;
}
bool RpcInnerReq_RPC_TYPE_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const RpcInnerReq_RPC_TYPE RpcInnerReq::ONE_WAY;
const RpcInnerReq_RPC_TYPE RpcInnerReq::TWO_WAY;
const RpcInnerReq_RPC_TYPE RpcInnerReq::RPC_TYPE_MIN;
const RpcInnerReq_RPC_TYPE RpcInnerReq::RPC_TYPE_MAX;
const int RpcInnerReq::RPC_TYPE_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int RpcInnerReq::kServiceNameFieldNumber;
const int RpcInnerReq::kMethondNameFieldNumber;
const int RpcInnerReq::kRequestIdFieldNumber;
const int RpcInnerReq::kTypeFieldNumber;
const int RpcInnerReq::kDataFieldNumber;
#endif  // !_MSC_VER

RpcInnerReq::RpcInnerReq()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:RpcInnerReq)
}

void RpcInnerReq::InitAsDefaultInstance() {
}

RpcInnerReq::RpcInnerReq(const RpcInnerReq& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:RpcInnerReq)
}

void RpcInnerReq::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  service_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  methond_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  request_id_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  type_ = 0;
  data_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

RpcInnerReq::~RpcInnerReq() {
  // @@protoc_insertion_point(destructor:RpcInnerReq)
  SharedDtor();
}

void RpcInnerReq::SharedDtor() {
  if (service_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete service_name_;
  }
  if (methond_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete methond_name_;
  }
  if (request_id_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete request_id_;
  }
  if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete data_;
  }
  if (this != default_instance_) {
  }
}

void RpcInnerReq::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* RpcInnerReq::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return RpcInnerReq_descriptor_;
}

const RpcInnerReq& RpcInnerReq::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_rpc_2eproto();
  return *default_instance_;
}

RpcInnerReq* RpcInnerReq::default_instance_ = NULL;

RpcInnerReq* RpcInnerReq::New() const {
  return new RpcInnerReq;
}

void RpcInnerReq::Clear() {
  if (_has_bits_[0 / 32] & 31) {
    if (has_service_name()) {
      if (service_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        service_name_->clear();
      }
    }
    if (has_methond_name()) {
      if (methond_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        methond_name_->clear();
      }
    }
    if (has_request_id()) {
      if (request_id_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        request_id_->clear();
      }
    }
    type_ = 0;
    if (has_data()) {
      if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        data_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool RpcInnerReq::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:RpcInnerReq)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required string service_name = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_service_name()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->service_name().data(), this->service_name().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "service_name");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_methond_name;
        break;
      }

      // required string methond_name = 2;
      case 2: {
        if (tag == 18) {
         parse_methond_name:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_methond_name()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->methond_name().data(), this->methond_name().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "methond_name");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(26)) goto parse_request_id;
        break;
      }

      // required string request_id = 3;
      case 3: {
        if (tag == 26) {
         parse_request_id:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_request_id()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->request_id().data(), this->request_id().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "request_id");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(32)) goto parse_type;
        break;
      }

      // required .RpcInnerReq.RPC_TYPE type = 4;
      case 4: {
        if (tag == 32) {
         parse_type:
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::RpcInnerReq_RPC_TYPE_IsValid(value)) {
            set_type(static_cast< ::RpcInnerReq_RPC_TYPE >(value));
          } else {
            mutable_unknown_fields()->AddVarint(4, value);
          }
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(42)) goto parse_data;
        break;
      }

      // required string data = 5;
      case 5: {
        if (tag == 42) {
         parse_data:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_data()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->data().data(), this->data().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "data");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:RpcInnerReq)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:RpcInnerReq)
  return false;
#undef DO_
}

void RpcInnerReq::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:RpcInnerReq)
  // required string service_name = 1;
  if (has_service_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->service_name().data(), this->service_name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "service_name");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->service_name(), output);
  }

  // required string methond_name = 2;
  if (has_methond_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->methond_name().data(), this->methond_name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "methond_name");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->methond_name(), output);
  }

  // required string request_id = 3;
  if (has_request_id()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->request_id().data(), this->request_id().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "request_id");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      3, this->request_id(), output);
  }

  // required .RpcInnerReq.RPC_TYPE type = 4;
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      4, this->type(), output);
  }

  // required string data = 5;
  if (has_data()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->data().data(), this->data().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "data");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      5, this->data(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:RpcInnerReq)
}

::google::protobuf::uint8* RpcInnerReq::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:RpcInnerReq)
  // required string service_name = 1;
  if (has_service_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->service_name().data(), this->service_name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "service_name");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->service_name(), target);
  }

  // required string methond_name = 2;
  if (has_methond_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->methond_name().data(), this->methond_name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "methond_name");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->methond_name(), target);
  }

  // required string request_id = 3;
  if (has_request_id()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->request_id().data(), this->request_id().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "request_id");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        3, this->request_id(), target);
  }

  // required .RpcInnerReq.RPC_TYPE type = 4;
  if (has_type()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      4, this->type(), target);
  }

  // required string data = 5;
  if (has_data()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->data().data(), this->data().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "data");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        5, this->data(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:RpcInnerReq)
  return target;
}

int RpcInnerReq::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required string service_name = 1;
    if (has_service_name()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->service_name());
    }

    // required string methond_name = 2;
    if (has_methond_name()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->methond_name());
    }

    // required string request_id = 3;
    if (has_request_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->request_id());
    }

    // required .RpcInnerReq.RPC_TYPE type = 4;
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }

    // required string data = 5;
    if (has_data()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->data());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void RpcInnerReq::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const RpcInnerReq* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const RpcInnerReq*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void RpcInnerReq::MergeFrom(const RpcInnerReq& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_service_name()) {
      set_service_name(from.service_name());
    }
    if (from.has_methond_name()) {
      set_methond_name(from.methond_name());
    }
    if (from.has_request_id()) {
      set_request_id(from.request_id());
    }
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_data()) {
      set_data(from.data());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void RpcInnerReq::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void RpcInnerReq::CopyFrom(const RpcInnerReq& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool RpcInnerReq::IsInitialized() const {
  if ((_has_bits_[0] & 0x0000001f) != 0x0000001f) return false;

  return true;
}

void RpcInnerReq::Swap(RpcInnerReq* other) {
  if (other != this) {
    std::swap(service_name_, other->service_name_);
    std::swap(methond_name_, other->methond_name_);
    std::swap(request_id_, other->request_id_);
    std::swap(type_, other->type_);
    std::swap(data_, other->data_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata RpcInnerReq::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = RpcInnerReq_descriptor_;
  metadata.reflection = RpcInnerReq_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int RpcInnerResp::kRequestIdFieldNumber;
const int RpcInnerResp::kDataFieldNumber;
#endif  // !_MSC_VER

RpcInnerResp::RpcInnerResp()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:RpcInnerResp)
}

void RpcInnerResp::InitAsDefaultInstance() {
}

RpcInnerResp::RpcInnerResp(const RpcInnerResp& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:RpcInnerResp)
}

void RpcInnerResp::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  request_id_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  data_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

RpcInnerResp::~RpcInnerResp() {
  // @@protoc_insertion_point(destructor:RpcInnerResp)
  SharedDtor();
}

void RpcInnerResp::SharedDtor() {
  if (request_id_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete request_id_;
  }
  if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete data_;
  }
  if (this != default_instance_) {
  }
}

void RpcInnerResp::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* RpcInnerResp::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return RpcInnerResp_descriptor_;
}

const RpcInnerResp& RpcInnerResp::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_rpc_2eproto();
  return *default_instance_;
}

RpcInnerResp* RpcInnerResp::default_instance_ = NULL;

RpcInnerResp* RpcInnerResp::New() const {
  return new RpcInnerResp;
}

void RpcInnerResp::Clear() {
  if (_has_bits_[0 / 32] & 3) {
    if (has_request_id()) {
      if (request_id_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        request_id_->clear();
      }
    }
    if (has_data()) {
      if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        data_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool RpcInnerResp::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:RpcInnerResp)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required string request_id = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_request_id()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->request_id().data(), this->request_id().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "request_id");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_data;
        break;
      }

      // required string data = 2;
      case 2: {
        if (tag == 18) {
         parse_data:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_data()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->data().data(), this->data().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "data");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:RpcInnerResp)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:RpcInnerResp)
  return false;
#undef DO_
}

void RpcInnerResp::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:RpcInnerResp)
  // required string request_id = 1;
  if (has_request_id()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->request_id().data(), this->request_id().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "request_id");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->request_id(), output);
  }

  // required string data = 2;
  if (has_data()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->data().data(), this->data().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "data");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->data(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:RpcInnerResp)
}

::google::protobuf::uint8* RpcInnerResp::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:RpcInnerResp)
  // required string request_id = 1;
  if (has_request_id()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->request_id().data(), this->request_id().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "request_id");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->request_id(), target);
  }

  // required string data = 2;
  if (has_data()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->data().data(), this->data().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "data");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->data(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:RpcInnerResp)
  return target;
}

int RpcInnerResp::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required string request_id = 1;
    if (has_request_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->request_id());
    }

    // required string data = 2;
    if (has_data()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->data());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void RpcInnerResp::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const RpcInnerResp* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const RpcInnerResp*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void RpcInnerResp::MergeFrom(const RpcInnerResp& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_request_id()) {
      set_request_id(from.request_id());
    }
    if (from.has_data()) {
      set_data(from.data());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void RpcInnerResp::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void RpcInnerResp::CopyFrom(const RpcInnerResp& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool RpcInnerResp::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000003) != 0x00000003) return false;

  return true;
}

void RpcInnerResp::Swap(RpcInnerResp* other) {
  if (other != this) {
    std::swap(request_id_, other->request_id_);
    std::swap(data_, other->data_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata RpcInnerResp::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = RpcInnerResp_descriptor_;
  metadata.reflection = RpcInnerResp_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
