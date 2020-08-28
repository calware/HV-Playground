@ECHO OFF
curl http://192.168.110.128:25090/install -F "file=@./x64/Debug/SPTHv.sys" -F "execute=true" -vvv