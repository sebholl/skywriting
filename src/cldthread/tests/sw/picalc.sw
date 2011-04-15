blah = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/pi"), "app_args" : ["1000", "20"] }, 1);
return *(blah[0]);

