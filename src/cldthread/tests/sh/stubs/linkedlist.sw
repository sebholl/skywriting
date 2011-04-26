cloudapp = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/linkedlist"), "app_args" : [ env["LIST_SIZE"] ] }, 1);
return **(cloudapp[0]);

