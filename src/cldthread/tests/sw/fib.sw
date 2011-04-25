nthfib = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/fib"), "app_args" : ["7"] }, 1);
return *(nthfib[0]);

