[[stage(compute), workgroup_size(1)]]
fn f() {
  let a = vec3<f32>(1.0, 2.0, 3.0);
  let b = 4.0;
  let r : vec3<f32> = (a - b);
}
