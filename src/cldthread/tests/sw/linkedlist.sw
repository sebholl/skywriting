cloudapp = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/linkedlist"), "app_args" : ["15"] }, 1);
return **(cloudapp[0]);

