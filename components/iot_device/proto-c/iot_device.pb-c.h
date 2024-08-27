/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: iot_device.proto */

#ifndef PROTOBUF_C_iot_5fdevice_2eproto__INCLUDED
#define PROTOBUF_C_iot_5fdevice_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _IotDeviceInfo IotDeviceInfo;
typedef struct _IotValue IotValue;
typedef struct _IotAttribute IotAttribute;
typedef struct _IotAttribute__ParamsEntry IotAttribute__ParamsEntry;
typedef struct _IotMetadata IotMetadata;
typedef struct _IotService IotService;
typedef struct _IotAttributeData IotAttributeData;
typedef struct _IotAttributeReadRequest IotAttributeReadRequest;
typedef struct _IotAttributeWriteRequest IotAttributeWriteRequest;
typedef struct _IotAttributeResponse IotAttributeResponse;


/* --- enums --- */

typedef enum _IotValueType {
  IOT_VALUE_TYPE__IOT_VAL_TYPE_BOOLEAN = 0,
  IOT_VALUE_TYPE__IOT_VAL_TYPE_INTEGER = 1,
  IOT_VALUE_TYPE__IOT_VAL_TYPE_FLOAT = 2,
  IOT_VALUE_TYPE__IOT_VAL_TYPE_LONG = 3,
  IOT_VALUE_TYPE__IOT_VAL_TYPE_STRING = 4,
  IOT_VALUE_TYPE__IOT_VAL_TYPE_UNKNOWN = 5
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(IOT_VALUE_TYPE)
} IotValueType;

/* --- messages --- */

struct  _IotDeviceInfo
{
  ProtobufCMessage base;
  char *uuid;
  char *name;
  char *type;
  size_t n_attributes;
  IotAttribute **attributes;
  size_t n_services;
  IotService **services;
  IotMetadata *metadata;
};
#define IOT_DEVICE_INFO__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_device_info__descriptor) \
    , (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, 0,NULL, 0,NULL, NULL }


typedef enum {
  IOT_VALUE__TYPE__NOT_SET = 0,
  IOT_VALUE__TYPE_BOOL_VALUE = 1,
  IOT_VALUE__TYPE_INT_VALUE = 2,
  IOT_VALUE__TYPE_LONG_VALUE = 3,
  IOT_VALUE__TYPE_FLOAT_VALUE = 4,
  IOT_VALUE__TYPE_STRING_VALUE = 5
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(IOT_VALUE__TYPE)
} IotValue__TypeCase;

struct  _IotValue
{
  ProtobufCMessage base;
  IotValue__TypeCase type_case;
  union {
    protobuf_c_boolean bool_value;
    uint32_t int_value;
    uint64_t long_value;
    float float_value;
    char *string_value;
  };
};
#define IOT_VALUE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_value__descriptor) \
    , IOT_VALUE__TYPE__NOT_SET, {0} }


struct  _IotAttribute__ParamsEntry
{
  ProtobufCMessage base;
  char *key;
  IotValue *value;
};
#define IOT_ATTRIBUTE__PARAMS_ENTRY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_attribute__params_entry__descriptor) \
    , (char *)protobuf_c_empty_string, NULL }


struct  _IotAttribute
{
  ProtobufCMessage base;
  char *name;
  protobuf_c_boolean is_primary;
  IotValue *value;
  size_t n_params;
  IotAttribute__ParamsEntry **params;
};
#define IOT_ATTRIBUTE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_attribute__descriptor) \
    , (char *)protobuf_c_empty_string, 0, NULL, 0,NULL }


struct  _IotMetadata
{
  ProtobufCMessage base;
  char *mac_address;
  char *model;
  char *firmware_version;
  char *last_updated;
};
#define IOT_METADATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_metadata__descriptor) \
    , (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string }


struct  _IotService
{
  ProtobufCMessage base;
  char *name;
  protobuf_c_boolean enabled;
  protobuf_c_boolean is_core_service;
};
#define IOT_SERVICE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_service__descriptor) \
    , (char *)protobuf_c_empty_string, 0, 0 }


struct  _IotAttributeData
{
  ProtobufCMessage base;
  char *name;
  IotValue *value;
};
#define IOT_ATTRIBUTE_DATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_attribute_data__descriptor) \
    , (char *)protobuf_c_empty_string, NULL }


struct  _IotAttributeReadRequest
{
  ProtobufCMessage base;
  size_t n_name;
  char **name;
};
#define IOT_ATTRIBUTE_READ_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_attribute_read_request__descriptor) \
    , 0,NULL }


/*
 ** 
 */
struct  _IotAttributeWriteRequest
{
  ProtobufCMessage base;
  size_t n_attributes;
  IotAttributeData **attributes;
};
#define IOT_ATTRIBUTE_WRITE_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_attribute_write_request__descriptor) \
    , 0,NULL }


struct  _IotAttributeResponse
{
  ProtobufCMessage base;
  size_t n_attributes;
  IotAttributeData **attributes;
};
#define IOT_ATTRIBUTE_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&iot_attribute_response__descriptor) \
    , 0,NULL }


/* IotDeviceInfo methods */
void   iot_device_info__init
                     (IotDeviceInfo         *message);
size_t iot_device_info__get_packed_size
                     (const IotDeviceInfo   *message);
size_t iot_device_info__pack
                     (const IotDeviceInfo   *message,
                      uint8_t             *out);
size_t iot_device_info__pack_to_buffer
                     (const IotDeviceInfo   *message,
                      ProtobufCBuffer     *buffer);
IotDeviceInfo *
       iot_device_info__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_device_info__free_unpacked
                     (IotDeviceInfo *message,
                      ProtobufCAllocator *allocator);
/* IotValue methods */
void   iot_value__init
                     (IotValue         *message);
size_t iot_value__get_packed_size
                     (const IotValue   *message);
size_t iot_value__pack
                     (const IotValue   *message,
                      uint8_t             *out);
size_t iot_value__pack_to_buffer
                     (const IotValue   *message,
                      ProtobufCBuffer     *buffer);
IotValue *
       iot_value__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_value__free_unpacked
                     (IotValue *message,
                      ProtobufCAllocator *allocator);
/* IotAttribute__ParamsEntry methods */
void   iot_attribute__params_entry__init
                     (IotAttribute__ParamsEntry         *message);
/* IotAttribute methods */
void   iot_attribute__init
                     (IotAttribute         *message);
size_t iot_attribute__get_packed_size
                     (const IotAttribute   *message);
size_t iot_attribute__pack
                     (const IotAttribute   *message,
                      uint8_t             *out);
size_t iot_attribute__pack_to_buffer
                     (const IotAttribute   *message,
                      ProtobufCBuffer     *buffer);
IotAttribute *
       iot_attribute__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_attribute__free_unpacked
                     (IotAttribute *message,
                      ProtobufCAllocator *allocator);
/* IotMetadata methods */
void   iot_metadata__init
                     (IotMetadata         *message);
size_t iot_metadata__get_packed_size
                     (const IotMetadata   *message);
size_t iot_metadata__pack
                     (const IotMetadata   *message,
                      uint8_t             *out);
size_t iot_metadata__pack_to_buffer
                     (const IotMetadata   *message,
                      ProtobufCBuffer     *buffer);
IotMetadata *
       iot_metadata__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_metadata__free_unpacked
                     (IotMetadata *message,
                      ProtobufCAllocator *allocator);
/* IotService methods */
void   iot_service__init
                     (IotService         *message);
size_t iot_service__get_packed_size
                     (const IotService   *message);
size_t iot_service__pack
                     (const IotService   *message,
                      uint8_t             *out);
size_t iot_service__pack_to_buffer
                     (const IotService   *message,
                      ProtobufCBuffer     *buffer);
IotService *
       iot_service__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_service__free_unpacked
                     (IotService *message,
                      ProtobufCAllocator *allocator);
/* IotAttributeData methods */
void   iot_attribute_data__init
                     (IotAttributeData         *message);
size_t iot_attribute_data__get_packed_size
                     (const IotAttributeData   *message);
size_t iot_attribute_data__pack
                     (const IotAttributeData   *message,
                      uint8_t             *out);
size_t iot_attribute_data__pack_to_buffer
                     (const IotAttributeData   *message,
                      ProtobufCBuffer     *buffer);
IotAttributeData *
       iot_attribute_data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_attribute_data__free_unpacked
                     (IotAttributeData *message,
                      ProtobufCAllocator *allocator);
/* IotAttributeReadRequest methods */
void   iot_attribute_read_request__init
                     (IotAttributeReadRequest         *message);
size_t iot_attribute_read_request__get_packed_size
                     (const IotAttributeReadRequest   *message);
size_t iot_attribute_read_request__pack
                     (const IotAttributeReadRequest   *message,
                      uint8_t             *out);
size_t iot_attribute_read_request__pack_to_buffer
                     (const IotAttributeReadRequest   *message,
                      ProtobufCBuffer     *buffer);
IotAttributeReadRequest *
       iot_attribute_read_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_attribute_read_request__free_unpacked
                     (IotAttributeReadRequest *message,
                      ProtobufCAllocator *allocator);
/* IotAttributeWriteRequest methods */
void   iot_attribute_write_request__init
                     (IotAttributeWriteRequest         *message);
size_t iot_attribute_write_request__get_packed_size
                     (const IotAttributeWriteRequest   *message);
size_t iot_attribute_write_request__pack
                     (const IotAttributeWriteRequest   *message,
                      uint8_t             *out);
size_t iot_attribute_write_request__pack_to_buffer
                     (const IotAttributeWriteRequest   *message,
                      ProtobufCBuffer     *buffer);
IotAttributeWriteRequest *
       iot_attribute_write_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_attribute_write_request__free_unpacked
                     (IotAttributeWriteRequest *message,
                      ProtobufCAllocator *allocator);
/* IotAttributeResponse methods */
void   iot_attribute_response__init
                     (IotAttributeResponse         *message);
size_t iot_attribute_response__get_packed_size
                     (const IotAttributeResponse   *message);
size_t iot_attribute_response__pack
                     (const IotAttributeResponse   *message,
                      uint8_t             *out);
size_t iot_attribute_response__pack_to_buffer
                     (const IotAttributeResponse   *message,
                      ProtobufCBuffer     *buffer);
IotAttributeResponse *
       iot_attribute_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   iot_attribute_response__free_unpacked
                     (IotAttributeResponse *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*IotDeviceInfo_Closure)
                 (const IotDeviceInfo *message,
                  void *closure_data);
typedef void (*IotValue_Closure)
                 (const IotValue *message,
                  void *closure_data);
typedef void (*IotAttribute__ParamsEntry_Closure)
                 (const IotAttribute__ParamsEntry *message,
                  void *closure_data);
typedef void (*IotAttribute_Closure)
                 (const IotAttribute *message,
                  void *closure_data);
typedef void (*IotMetadata_Closure)
                 (const IotMetadata *message,
                  void *closure_data);
typedef void (*IotService_Closure)
                 (const IotService *message,
                  void *closure_data);
typedef void (*IotAttributeData_Closure)
                 (const IotAttributeData *message,
                  void *closure_data);
typedef void (*IotAttributeReadRequest_Closure)
                 (const IotAttributeReadRequest *message,
                  void *closure_data);
typedef void (*IotAttributeWriteRequest_Closure)
                 (const IotAttributeWriteRequest *message,
                  void *closure_data);
typedef void (*IotAttributeResponse_Closure)
                 (const IotAttributeResponse *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    iot_value_type__descriptor;
extern const ProtobufCMessageDescriptor iot_device_info__descriptor;
extern const ProtobufCMessageDescriptor iot_value__descriptor;
extern const ProtobufCMessageDescriptor iot_attribute__descriptor;
extern const ProtobufCMessageDescriptor iot_attribute__params_entry__descriptor;
extern const ProtobufCMessageDescriptor iot_metadata__descriptor;
extern const ProtobufCMessageDescriptor iot_service__descriptor;
extern const ProtobufCMessageDescriptor iot_attribute_data__descriptor;
extern const ProtobufCMessageDescriptor iot_attribute_read_request__descriptor;
extern const ProtobufCMessageDescriptor iot_attribute_write_request__descriptor;
extern const ProtobufCMessageDescriptor iot_attribute_response__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_iot_5fdevice_2eproto__INCLUDED */
