pushd %~dp0

protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe --js_out=import_style=commonjs,binary:. ./Protocol.proto


XCOPY /Y Protocol.pb.cc "../BigeumTalkServer"
XCOPY /Y Protocol.pb.h "../BigeumTalkServer"

DEL /Q /F *.pb.h
DEL /Q /F *pb.cc

pause