size_x = 8192;
size_y = 8192;
n_tiles_x = 2;
n_tiles_y = 2;
param_scale = "0.00025";
param_maxit = 1000;
x = -1;
y = -1;

mandelbrot_png = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/mandelbrot"), "app_args" : [size_x, size_y, n_tiles_x, n_tiles_y, x, y, param_maxit, param_scale] }, 1);

return *(mandelbrot_png[0]);

