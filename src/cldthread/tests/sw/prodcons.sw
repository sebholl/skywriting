cloudapp = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/prodcons"), "app_args" : ["100"] }, 1);
return *(cloudapp[0]);

