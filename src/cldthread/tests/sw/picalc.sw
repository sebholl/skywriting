pi_task = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/pi"), "app_args" : ["10000000", "20"] }, 1);
return *(pi_task[0]);

