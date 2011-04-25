size_x = 8192;
size_y = 8192;
offsetx = "-0.75";
offsety = "0.0";
n_tiles_x = 2;
n_tiles_y = 2;
param_maxit = 500;
param_scale = "3.0";

mandelbrot_png = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/mandelbrot"), "app_args" : [size_x, size_y, offsetx, offsety, n_tiles_x, n_tiles_y, param_maxit, param_scale] }, 1);

return *(mandelbrot_png[0]);

