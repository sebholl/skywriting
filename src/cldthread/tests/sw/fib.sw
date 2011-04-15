blah = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/fib"), "app_args" : ["80"] }, 1);
return *(blah[0]);

