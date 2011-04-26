nthfib = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/fib"), "app_args" : [ env["FIB_N"], "1"] }, 1);
return *(nthfib[0]);

