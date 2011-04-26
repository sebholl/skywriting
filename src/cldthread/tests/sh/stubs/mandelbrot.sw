size_x = 10000;
size_y = 10000;
offsetx = "-0.75";
offsety = "0.0";
n_tiles_x = env["MAN_SPLITX"];
n_tiles_y = env["MAN_SPLITY"];
param_maxit = 1000;
param_scale = "2.75";

mandelbrot_png = spawn_exec("cloudapp", {"app_ref" : ref("file:///opt/skywriting/src/cldthread/tests/bin/mandelbrot"), "app_args" : [size_x, size_y, offsetx, offsety, n_tiles_x, n_tiles_y, param_maxit, param_scale] }, 1);

return *(mandelbrot_png[0]);

