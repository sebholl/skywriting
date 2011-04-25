cloudapp = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/helloworld") }, 1);
return *(cloudapp[0]);

