# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: game.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='game.proto',
  package='gs',
  syntax='proto2',
  serialized_options=None,
  serialized_pb=_b('\n\ngame.proto\x12\x02gs\"3\n\nPlayerInfo\x12\n\n\x02id\x18\x01 \x02(\x05\x12\x0c\n\x04nick\x18\x02 \x02(\x0c\x12\x0b\n\x03\x61ge\x18\x03 \x02(\x05\"!\n\x0cLoginRequest\x12\x11\n\tplayer_id\x18\x01 \x02(\x05\".\n\rLoginResponse\x12\x1d\n\x05pinfo\x18\x01 \x02(\x0b\x32\x0e.gs.PlayerInfo2;\n\x0bGameService\x12,\n\x05login\x12\x10.gs.LoginRequest\x1a\x11.gs.LoginResponse')
)




_PLAYERINFO = _descriptor.Descriptor(
  name='PlayerInfo',
  full_name='gs.PlayerInfo',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='gs.PlayerInfo.id', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='nick', full_name='gs.PlayerInfo.nick', index=1,
      number=2, type=12, cpp_type=9, label=2,
      has_default_value=False, default_value=_b(""),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='age', full_name='gs.PlayerInfo.age', index=2,
      number=3, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=18,
  serialized_end=69,
)


_LOGINREQUEST = _descriptor.Descriptor(
  name='LoginRequest',
  full_name='gs.LoginRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='player_id', full_name='gs.LoginRequest.player_id', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=71,
  serialized_end=104,
)


_LOGINRESPONSE = _descriptor.Descriptor(
  name='LoginResponse',
  full_name='gs.LoginResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='pinfo', full_name='gs.LoginResponse.pinfo', index=0,
      number=1, type=11, cpp_type=10, label=2,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=106,
  serialized_end=152,
)

_LOGINRESPONSE.fields_by_name['pinfo'].message_type = _PLAYERINFO
DESCRIPTOR.message_types_by_name['PlayerInfo'] = _PLAYERINFO
DESCRIPTOR.message_types_by_name['LoginRequest'] = _LOGINREQUEST
DESCRIPTOR.message_types_by_name['LoginResponse'] = _LOGINRESPONSE
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

PlayerInfo = _reflection.GeneratedProtocolMessageType('PlayerInfo', (_message.Message,), dict(
  DESCRIPTOR = _PLAYERINFO,
  __module__ = 'game_pb2'
  # @@protoc_insertion_point(class_scope:gs.PlayerInfo)
  ))
_sym_db.RegisterMessage(PlayerInfo)

LoginRequest = _reflection.GeneratedProtocolMessageType('LoginRequest', (_message.Message,), dict(
  DESCRIPTOR = _LOGINREQUEST,
  __module__ = 'game_pb2'
  # @@protoc_insertion_point(class_scope:gs.LoginRequest)
  ))
_sym_db.RegisterMessage(LoginRequest)

LoginResponse = _reflection.GeneratedProtocolMessageType('LoginResponse', (_message.Message,), dict(
  DESCRIPTOR = _LOGINRESPONSE,
  __module__ = 'game_pb2'
  # @@protoc_insertion_point(class_scope:gs.LoginResponse)
  ))
_sym_db.RegisterMessage(LoginResponse)



_GAMESERVICE = _descriptor.ServiceDescriptor(
  name='GameService',
  full_name='gs.GameService',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  serialized_start=154,
  serialized_end=213,
  methods=[
  _descriptor.MethodDescriptor(
    name='login',
    full_name='gs.GameService.login',
    index=0,
    containing_service=None,
    input_type=_LOGINREQUEST,
    output_type=_LOGINRESPONSE,
    serialized_options=None,
  ),
])
_sym_db.RegisterServiceDescriptor(_GAMESERVICE)

DESCRIPTOR.services_by_name['GameService'] = _GAMESERVICE

# @@protoc_insertion_point(module_scope)
