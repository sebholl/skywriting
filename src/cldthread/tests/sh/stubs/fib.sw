nthfib = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/fib"), "app_args" : [ env["FIB_N"], env["FIB_MEM"] ] }, 1);
return *(nthfib[0]);

