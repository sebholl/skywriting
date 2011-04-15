size_x = 1000;
size_y = 1000;
n_tiles_x = 10;
n_tiles_y = 10;
param_scale = "0.0005";
param_maxit = 10000;

blah = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/mandelbrot"), "app_args" : [size_x, size_y, n_tiles_x, n_tiles_y, param_maxit, param_scale] }, 1);
return *(blah[0]);

