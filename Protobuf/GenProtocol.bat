pushd %~dp0

protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe --js_out=import_style=commonjs,binary:. ./Protocol.proto
protoc.exe --python_out=pyi_out:./ ./Protocol.proto


XCOPY /Y Protocol.pb.cc "../BigeumTalkServer"
XCOPY /Y Protocol.pb.h "../BigeumTalkServer"

XCOPY /Y Protocol_pb2.py "../DummyClient"
XCOPY /Y Protocol_pb2.pyi "../DummyClient"

pause